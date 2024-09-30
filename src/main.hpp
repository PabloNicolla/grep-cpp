#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>

struct RecResult
{
  std::string::const_iterator result{};
  bool is_valid{ false };
};

constexpr bool isAnyMetaCharacter(const char c);
constexpr bool isQuantifier(const char c);
constexpr bool isQuantifierAdvanced(const char c);
constexpr bool isCharacterClass(const char c);
constexpr bool isGroup(const char c);
constexpr bool is_w(const char c);
constexpr bool is_W(const char c);
bool is_d(const char c);
bool is_D(const char c);

std::function<bool(const char)> characterClassSelector(const char c);
std::function<bool(const char)> plainCharacterMatcherBuilder(const std::shared_ptr<std::vector<char>>& plainChars);

bool matcherControl(const std::vector<std::function<bool(const char)>>& matchers, const char c);
std::vector<std::function<bool(const char)>> matcherControlSetupBuilder(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
bool matcherControlSetup(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end, const char c, bool negative_group_flag = false);
bool matcherControlSetupMemoized(std::vector<std::function<bool(const char)>>& matchers, const char c, bool negative_group_flag = false);


RecResult patternControl(std::string::const_iterator text_start, std::string::const_iterator text_end,
                         std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);


RecResult quantifierLoop(std::string::const_iterator text_start, std::string::const_iterator text_end,
                         std::string::const_iterator pattern_start, std::string::const_iterator pattern_end,
                         std::string::const_iterator quantifier_start,
                         bool negative_group_flag = false, int pattern_end_offset = 0);

RecResult handleGroups(std::string::const_iterator text_start, std::string::const_iterator text_end,
                       std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);

RecResult match_pattern(const std::string& input_line, const std::string& pattern);

RecResult match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
                     std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);

RecResult handleScapedCharacter(std::string::const_iterator text_start, std::string::const_iterator text_end,
                     std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);

RecResult group_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
                     std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);

std::string::const_iterator handleQuantifier(std::string::const_iterator quantifier_start, std::string::const_iterator pattern_end, int& min, int& max);
std::string::const_iterator parseQuantifierRange(int& min, int& max, std::string::const_iterator start, std::string::const_iterator end);
std::string::const_iterator findOutermostOr(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
std::string::const_iterator findClosingParenthesis(std::string::const_iterator start, std::string::const_iterator end);

bool backReference_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
                              std::string::const_iterator back_ref_start, std::string::const_iterator back_ref_end);

RecResult match_pattern(const std::string& input_line, const std::string& pattern);