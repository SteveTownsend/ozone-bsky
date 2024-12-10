#include <aho_corasick/aho_corasick.hpp>
#include <exception>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <ios>

#include "matcher.hpp"
#include "testdefs.hpp"

// Demonstrate some basic assertions.
TEST(MatcherTest, BasicAssertions) {
  // Expect two strings not to be equal.
  EXPECT_STRNE("hello", "world");
  // Expect equality.
  EXPECT_EQ(7 * 6, 42);
}

TEST(MatcherTest, Parse) {
  {
    aho_corasick::wtrie trie;
    trie.insert(L"hers");
    trie.insert(L"his");
    trie.insert(L"she");
    trie.insert(L"he");
    auto result = trie.parse_text(L"ushers");
  }

  {
    aho_corasick::wtrie trie;
    trie.remove_overlaps();
    trie.insert(L"hot");
    trie.insert(L"hot chocolate");
    auto result = trie.parse_text(L"hot chocolate");
  }

  {
    aho_corasick::wtrie trie;
    trie.case_insensitive();
    trie.insert(L"casing");
    auto result = trie.parse_text(L"CaSiNg");
  }

  {
    aho_corasick::wtrie trie;
    trie.remove_overlaps().only_whole_words().case_insensitive();
    trie.insert(L"great question");
    trie.insert(L"forty-two");
    trie.insert(L"deep thought");
    auto tokens =
        trie.tokenise(L"The Answer to the Great Question... Of Life, the "
                      L"Universe and Everything... Is... Forty-two, said Deep "
                      L"Thought, with infinite majesty and calm.");
    std::wstringstream html;
    html << L"<html><body><p>";
    for (const auto &token : tokens) {
      if (token.is_match())
        html << L"<i>";
      html << token.get_fragment();
      if (token.is_match())
        html << L"</i>";
    }
    html << L"</p></body></html>";
    std::wcout << html.str() << '\n';
  }
}

TEST(MatcherTest, LoadFile) {
  std::string decorated = DataPath;
  decorated.append("filters");
  matcher my_matcher(decorated);
  {
    parser::candidate_list candidate(
        {{"description", "hate symbols include 卐"}});
    EXPECT_TRUE(candidate.size() == 1);
  }
  {
    parser::candidate_list candidate(
        {{"description", "hate symbols include ☭"}});
    EXPECT_TRUE(candidate.size() == 1);
  }
  {
    parser::candidate_list candidate(
        {{"description", "hate symbol ⚡⚡ represents the Nazi SS"}});
    EXPECT_TRUE(candidate.size() == 1);
  }
}

TEST(MatcherTest, RuleErrors) {
  // blank fields
  EXPECT_THAT([]() { matcher::rule my_rule("|blah|false|word|blah"); },
              testing::Throws<std::invalid_argument>());
  EXPECT_THAT([]() { matcher::rule my_rule("blah||true|substring|blah"); },
              testing::Throws<std::invalid_argument>());
  // wrong length
  EXPECT_THAT([]() { matcher::rule my_rule("blah|blah|true"); },
              testing::Throws<std::invalid_argument>());
  EXPECT_THAT(
      []() { matcher::rule my_rule("blah|blah|true|substring|blah|blah"); },
      testing::Throws<std::invalid_argument>());
  // last field optional
  EXPECT_NO_THROW(matcher::rule my_rule("blah|blah|false|word|"));
  EXPECT_NO_THROW(matcher::rule my_rule("blah|blah|false|substring"));
  // contingent matching strings
  EXPECT_NO_THROW(matcher::rule my_rule("blah|blah|false|substring|blah"));
  EXPECT_NO_THROW(
      matcher::rule my_rule("blah|blah|false|substring|blah,blah1,blah2"));
}

TEST(MatcherTest, Ukrainian) {
  matcher my_matcher;
  my_matcher.add_rule("Хохол|abusive|true|substring|");
  parser::candidate_list expected = {
      {"description", "russians use Хохол as a slur"},
      {"displayName", "russian slur Хохол"}};
  EXPECT_TRUE(my_matcher.check_candidates(expected));
}

TEST(MatcherTest, UkrainianRule) {
  matcher my_matcher;
  std::string rule("Хохол|abusive|false|substring|");
  my_matcher.add_rule(rule);
  parser::candidate_list expected = {{"description", rule},
                                     {"displayName", rule}};
  EXPECT_FALSE(my_matcher.all_matches_for_candidates(expected).empty());
}
