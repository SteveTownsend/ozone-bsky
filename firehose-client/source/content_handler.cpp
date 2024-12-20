/*************************************************************************
NAFO Forum Moderation Firehose Client
Copyright (c) Steve Townsend 2024

>>> SOURCE LICENSE >>>
This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation (www.fsf.org); either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

A copy of the GNU General Public License is available at
http://www.fsf.org/licensing/licenses
>>> END OF LICENSE >>>
*************************************************************************/

#include "content_handler.hpp"
#include "log_wrapper.hpp"
#include "post_processor.hpp"

content_handler::content_handler() : _is_ready(false) {}

void content_handler::set_filter(std::string const &filter_file) {
  _filter_file = filter_file;
  _matcher.set_filter(_filter_file);
  _is_ready = true;
}

std::string content_handler::get_filter() const { return _filter_file; }

void content_handler::handle(beast::flat_buffer const &beast_data) {
  if (!_is_ready)
    return;
  auto matches(_matcher.find_all_matches(beast_data));
  // confirm any matched rules with contingent string matches
  for (auto next_match = matches.begin(); next_match != matches.end();) {
    for (auto rule_key = next_match->_matches.begin();
         rule_key != next_match->_matches.end();) {
      matcher::rule this_rule = _matcher.find_rule(rule_key->get_keyword());
      if (!this_rule.matches_any_contingent(next_match->_candidate._value)) {
        rule_key = next_match->_matches.erase(rule_key);
      } else {
        ++rule_key;
      }
    }
    if (next_match->_matches.empty()) {
      next_match = matches.erase(next_match);
    } else {
      ++next_match;
    }
  }
  // No match, or all eliminated
  if (matches.empty()) {
    return;
  }
  std::string json_msg(boost::beast::buffers_to_string(beast_data.data()));

  _post_processor.wait_enqueue(payload(json_msg, matches));
}
