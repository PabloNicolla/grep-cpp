#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cctype>
#include <stack>
#include <memory>
#include <vector>

#include "main.hpp"


static std::vector<std::string> backReference{};


constexpr bool isAnyMetaCharacter(const char c)
{
  return (
    (c == '^') ||
    (c == '$') ||

    (c == '[') ||
    (c == ']') ||
    (c == '(') ||
    (c == ')') ||

    (c == '{') ||
    (c == '}') ||

    (c == '+') ||
    (c == '*') ||
    (c == '?') ||

    (c == '.') ||

    (c == '\\') ||

    (c == '|')
    );
}

constexpr bool isQuantifier(const char c)
{
  return (
    (c == '+') ||
    (c == '*') ||
    (c == '?')
    );
}

constexpr bool isQuantifierAdvanced(const char c)
{
  return (
    (c == '+') ||
    (c == '*') ||
    (c == '?') ||
    (c == '{')
    );
}

constexpr bool isCharacterClass(const char c)
{
  return (
    (c == 'w') ||
    (c == 'W') ||
    (c == 's') ||
    (c == 'S') ||
    (c == 'd') ||
    (c == 'D') ||
    (c == 'w') ||
    (c == 'W') ||
    (c == 'x') ||
    (c == 'O') ||
    (c == 'c')
    );
}

constexpr bool isGroup(const char c)
{
  return (
    (c == '[') ||
    (c == ']') ||
    (c == '(') ||
    (c == ')')
    );
}


constexpr bool is_w(const char c)
{
  return (
    (c >= 'a' && c <= 'z') ||
    (c >= 'Z' && c <= 'Z') ||
    (c >= '0' && c <= '9') ||
    (c == '_')
    );
}
constexpr bool is_W(const char c)
{
  return !is_w(c);
}

bool is_d(const char c)
{
  return isdigit(c);
}
bool is_D(const char c)
{
  return !is_d(c);
}


std::function<bool(const char)> characterClassSelector(const char c)
{
  switch (c)
  {
  case 'w':
    return is_w;
    break;
  case 'W':
    return is_W;
    break;
  case 'd':
    return is_d;
    break;
  case 'D':
    return is_D;
    break;

  default:
    throw std::invalid_argument("Character Class must receive valid character");
    break;
  }
}


std::function<bool(const char)> plainCharacterMatcherBuilder(const std::shared_ptr<std::vector<char>>& sharedPlainChars)
{
  return [sharedPlainChars](const char c) -> bool
    {
      for (const auto plainChar : *sharedPlainChars)
      {
        if (plainChar == c)
        {
          return true;
        }
      }
      return false;
    };
}


bool matcherControl(const std::vector<std::function<bool(const char)>>& matchers, const char c)
{
  for (const auto& matcher : matchers)
  {
    if (matcher(c))
    {
      return true;
    }
  }
  return false;
}

std::vector<std::function<bool(const char)>> matcherControlSetupBuilder(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  std::vector<std::function<bool(const char)>> matchers{};
  auto sharedPlainChars = std::make_shared<std::vector<char>>();

  while (pattern_start != pattern_end)
  {
    if (*pattern_start == '.')                      // Early exit if sub_pattern contains `.`
    {
      return { [](const char) -> bool { return true; } };
    }
    else if (*pattern_start == '\\')                // handle scape character
    {
      if (pattern_start + 1 == pattern_end)
      {
        throw std::invalid_argument("Error: Dangling backslash.");
      }
      else if (isCharacterClass(*(pattern_start + 1)))
      {
        matchers.push_back(characterClassSelector(*(pattern_start + 1)));
      }
      else
      {
        sharedPlainChars->push_back(*(pattern_start + 1));
      }
      pattern_start += 2;
    }
    else                                            // plain character
    {
      sharedPlainChars->push_back(*pattern_start);
      ++pattern_start;
    }
  }

  matchers.push_back(plainCharacterMatcherBuilder(sharedPlainChars));
  return matchers;
}

bool matcherControlSetup(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end, const char c, bool negative_group_flag)
{
  if (!negative_group_flag)
    return matcherControl(matcherControlSetupBuilder(pattern_start, pattern_end), c);
  else
    return !matcherControl(matcherControlSetupBuilder(pattern_start, pattern_end), c);
}

bool matcherControlSetupMemoized(std::vector<std::function<bool(const char)>>& matchers, const char c, bool negative_group_flag)
{
  if (!negative_group_flag)
    return matcherControl(matchers, c);
  else
    return !matcherControl(matchers, c);
}


RecResult patternControl(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return RecResult{text_start, true};                            // pattern matched
  }
  if (*pattern_start == '$' && pattern_start + 1 == pattern_end)
  {
    return text_start == text_end ? RecResult({ text_start, true }) : RecResult{};          // pattern must end here
  }
  if (text_start == text_end)
  {
    return RecResult{};                           // text ended without match
  }
  if (isQuantifierAdvanced(*pattern_start))
  {
    return RecResult{};                           // throw std::invalid_argument("Error: Invalid target for quantifier.");
  }
  if (*pattern_start == '^' || *pattern_start == '$')
  {
    return RecResult{};                           // misused meta characters making regex impossible to have a valid match
  }

  if (isGroup(*pattern_start))
  {
    return handleGroups(text_start, text_end, pattern_start, pattern_end);
  }
  if (*pattern_start == '\\')
  {
    return handleScapedCharacter(text_start, text_end, pattern_start, pattern_end);
  }
  if (pattern_start + 1 != pattern_end && isQuantifierAdvanced(*(pattern_start + 1)))
  {
    return quantifierLoop(text_start, text_end, pattern_start, pattern_end, pattern_start + 1);
  }
  // vvv will run and not break/exit process sequence vvv
  if (matcherControlSetup(pattern_start, pattern_start + 1, *text_start))
  {
    return patternControl(text_start + 1, text_end, pattern_start + 1, pattern_end);
  }
  // ^^^ must be the last condition ^^^
  return RecResult{};
}


std::string::const_iterator parseQuantifierRange(int& min, int& max, std::string::const_iterator start, std::string::const_iterator end)
{
  const auto new_end = std::find(start, end, '}');
  if (new_end == end)
  {
    throw std::runtime_error("Closing '}' not found");
  }

  auto comma = std::find(start, new_end, ',');

  // Parse the first number (min)
  std::string first_num(start, comma);
  first_num.erase(std::remove_if(first_num.begin(), first_num.end(), ::isspace), first_num.end());

  if (first_num.empty() || !std::all_of(first_num.begin(), first_num.end(), ::isdigit))
  {
    throw std::runtime_error("Invalid number format for min");
  }

  min = std::stoi(first_num);

  if (comma == new_end)
  {
    // No comma found, max = min
    max = min;
  }
  else
  {
    // Comma found, parse the second number (max)
    std::string second_num(comma + 1, new_end);
    second_num.erase(std::remove_if(second_num.begin(), second_num.end(), ::isspace), second_num.end());

    if (second_num.empty())
    {
      // Empty value between ',' and '}'
      max = -1;
    }
    else if (!std::all_of(second_num.begin(), second_num.end(), ::isdigit))
    {
      throw std::runtime_error("Invalid number format for max");
    }
    else
    {
      max = std::stoi(second_num);
    }
  }

  // Ensure min is non-negative
  if (min < 0)
  {
    throw std::runtime_error("Min cannot be negative");
  }

  // Ensure max is non-negative, except when it's -1
  if (max < -1)
  {
    throw std::runtime_error("Max cannot be negative (except -1)");
  }

  // Ensure min <= max, except when max is -1
  if (max != -1 && min > max)
  {
    throw std::runtime_error("Min cannot be greater than max");
  }

  return new_end + 1;
}

std::string::const_iterator handleQuantifier(std::string::const_iterator quantifier_start, std::string::const_iterator pattern_end, int& min, int& max)
{
  if (*quantifier_start == '*')
  {
    min = 0;
    max = -1;                       // -1 == infinite
  }
  else if (*quantifier_start == '+')
  {
    min = 1;
    max = -1;                       // -1 == infinite
  }
  else if (*quantifier_start == '?')
  {
    min = 0;
    max = 1;
  }
  else if (*quantifier_start == '{')
  {
    return parseQuantifierRange(min, max, quantifier_start + 1, pattern_end) + 1;       // handle custom quantifier {n, m} | {n} | {n,}
  }
  return quantifier_start + 1;
}

RecResult quantifierLoop(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end,
  std::string::const_iterator quantifier_start,                   // pattern_start < quantifier_start < pattern_end
  bool negative_group_flag, int pattern_end_offset)
{
  auto matchers = matcherControlSetupBuilder(pattern_start, quantifier_start - pattern_end_offset);
  int min{};
  int max{};
  auto new_pattern_start = handleQuantifier(quantifier_start, pattern_end, min, max);

  // Find the maximum number of matches possible
  auto text_it = text_start;
  int max_matches = 0;
  while (text_it != text_end && (max == -1 || max_matches < max) &&
    matcherControlSetupMemoized(matchers, *text_it, negative_group_flag)) {
    ++max_matches;
    ++text_it;
  }

  // Try matches from max_matches down to min
  for (int matches = max_matches; matches >= min; --matches) {
    auto current_text_start = text_start + matches;
    auto rec_result = patternControl(current_text_start, text_end, new_pattern_start, pattern_end);
    if (rec_result.is_valid) {
      return rec_result;
    }
  }

  return RecResult{};
}


RecResult handleGroups(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (*pattern_start == '[')
  {
    bool isNegativeGroup = false;
    auto group_end = std::find(pattern_start, pattern_end, ']');

    if (group_end == pattern_end)
    {
      throw std::runtime_error("Closing ']' not found");
    }

    if (*(pattern_start + 1) == '^')
    {
      isNegativeGroup = true;
      ++pattern_start;
    }

    if (group_end + 1 != pattern_end && isQuantifierAdvanced(*(group_end + 1)))
    {
      return quantifierLoop(text_start, text_end, pattern_start, pattern_end, group_end + 1, isNegativeGroup, 1);
    }
    if (matcherControlSetup(pattern_start + 1, group_end, *text_start, isNegativeGroup))
    {
      return patternControl(text_start + 1, text_end, group_end + 1, pattern_end);
    }
  }
  if (*pattern_start == '(')
  {
    auto group_end = findClosingParenthesis(pattern_start, pattern_end);

    if (group_end == pattern_end)
    {
      throw std::runtime_error("Closing ')' not found");
    }

    int backReferenceIndex = backReference.size(); // size before recusion
    backReference.emplace_back();
    auto group_rec_result = group_match_main(text_start, text_end, pattern_start + 1, group_end);
    if (group_rec_result.is_valid)
    {
      auto groupCapturedSize = group_rec_result.result - text_start;
      backReference[backReferenceIndex] = std::string(text_start, text_start + groupCapturedSize);
      auto rec_result = patternControl(text_start + groupCapturedSize, text_end, group_end + 1, pattern_end);
      if (rec_result.is_valid)
      {
        return rec_result;
      }
    }
    backReference.pop_back();
  }

  return RecResult{};
}


RecResult handleScapedCharacter(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start + 1 == pattern_end)
  {
    return RecResult{};                       // throw std::invalid_argument("Error: Dangling backslash.");
  }

  int possibleBackRefIndex = (*(pattern_start + 1) - '0');
  
  if (pattern_start + 2 != pattern_end && isQuantifierAdvanced(*(pattern_start + 2)))         // "\xq" where x is a character and q is a quantifier
  {
    return quantifierLoop(text_start, text_end, pattern_start, pattern_end, pattern_start + 2);
  }
  else if ((possibleBackRefIndex) >= 1 && (size_t)(possibleBackRefIndex) <= backReference.size())    // TODO support > 9 and quantifier
  {
    if (backReference_match_main(text_start, text_end, backReference[possibleBackRefIndex - 1].begin(), backReference[possibleBackRefIndex - 1].end()))
    {
      return patternControl(text_start + backReference[possibleBackRefIndex - 1].size(), text_end, pattern_start + 2, pattern_end);
    }
  }
  else if (matcherControlSetup(pattern_start, pattern_start + 2, *text_start))                // "\xz" where x is a character and z is not a quantifier, or
  {                                                                                           // "\x"  where x is a character and the pattern ends after x
    return patternControl(text_start + 1, text_end, pattern_start + 2, pattern_end);
  }
  return RecResult{};
}


bool backReference_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator back_ref_start, std::string::const_iterator back_ref_end)
{
  for (; back_ref_start != back_ref_end && text_start != text_end; ++back_ref_start, ++text_start)
  {
    if (*back_ref_start != *text_start) {
      return false;
    }
  }

  return back_ref_start == back_ref_end;
}


std::string::const_iterator findOutermostOr(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  std::stack<char> brackets;

  for (; pattern_start != pattern_end; ++pattern_start) {
    switch (*pattern_start) {
    case '(':
    case '[':
      brackets.push(*pattern_start);
      break;
    case ')':
      if (!brackets.empty() && brackets.top() == '(') {
        brackets.pop();
      }
      break;
    case ']':
      if (!brackets.empty() && brackets.top() == '[') {
        brackets.pop();
      }
      break;
    case '|':
      if (brackets.empty()) {
        return pattern_start;  // Found outermost '|'
      }
      break;
    }
  }

  return pattern_end;  // No outermost '|' found
}

std::string::const_iterator findClosingParenthesis(std::string::const_iterator start, std::string::const_iterator end)
{
  int scope = 0;
  bool foundOpeningParenthesis = false;

  for (auto it = start; it != end; ++it)
  {
    if (*it == '(')
    {
      scope++;
      foundOpeningParenthesis = true;
    }
    else if (*it == ')')
    {
      if (!foundOpeningParenthesis)
      {
        throw std::runtime_error("Closing parenthesis found before opening parenthesis");
      }
      scope--;
      if (scope == 0)
      {
        return it;  // Found the closing parenthesis in the same scope
      }
    }
  }

  if (!foundOpeningParenthesis)
  {
    throw std::runtime_error("No opening parenthesis found");
  }
  else
  {
    throw std::runtime_error("Unmatched opening parenthesis: scope never closed");
  }
}


RecResult group_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return RecResult{};
  }

  auto orPosition = findOutermostOr(pattern_start, pattern_end);

  while (orPosition != pattern_end)
  {
    auto group_rec_result = group_match_main(text_start, text_end, pattern_start, orPosition);
    if (group_rec_result.is_valid) {
      return group_rec_result;
    }
    pattern_start = orPosition + 1;
    orPosition = findOutermostOr(pattern_start, pattern_end);
  }
  
  return patternControl(text_start, text_end, pattern_start, pattern_end);
}

RecResult match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return RecResult{};
  }

  auto orPosition = findOutermostOr(pattern_start, pattern_end);

  while (orPosition != pattern_end)
  {
    auto main_rec_result = match_main(text_start, text_end, pattern_start, orPosition);
    if (main_rec_result.is_valid) {
      return main_rec_result;
    }
    pattern_start = orPosition + 1;
    orPosition = findOutermostOr(pattern_start, pattern_end);
  }

  if (*pattern_start == '^') {
    return patternControl(text_start, text_end, pattern_start + 1, pattern_end);
  }

  do
  {
    auto rec_result = patternControl(text_start, text_end, pattern_start, pattern_end);
    if (rec_result.is_valid)
    {
      return rec_result;
    }
  } while (++text_start != text_end);
  return RecResult{};
}

RecResult match_pattern(const std::string& input_line, const std::string& pattern)
{
  return match_main(input_line.begin(), input_line.end(), pattern.begin(), pattern.end());
}


int main(int argc, char* argv[])
{
  try
  {
    if (match_pattern("abc-def is abc-def, not efg, abc, or def", "(([abc]+)-([def]+)) is \\1, not ([^xyz]+), \\2, or \\3").is_valid)
    {
      return 0;
    }
    else
    {
      return 1;
    }
  }
  catch (const std::runtime_error& e)
  {
    std::cerr << e.what() << std::endl;
    return 1;
  }
}
