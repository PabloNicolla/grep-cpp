#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>
#include <stdexcept>
#include <cctype>
#include <stack>
#include <memory>

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
bool patternControl(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
std::string::const_iterator parseQuantifierRange(int& min, int& max, std::string::const_iterator start, std::string::const_iterator end);
std::string::const_iterator handleQuantifier(std::string::const_iterator quantifier_start, std::string::const_iterator pattern_end, int& min, int& max);
bool quantifierLoop(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end,
  std::string::const_iterator quantifier_start,
  bool negative_group_flag = false, int pattern_end_offset = 0);
bool handleGroups(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
bool match_pattern(const std::string& input_line, const std::string& pattern);
std::string::const_iterator findOutermostOr(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
bool match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
bool handleScapedCharacter(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);
bool group_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end);


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



bool patternControl(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return true;                            // pattern matched
  }
  if (*pattern_start == '$' && pattern_start + 1 == pattern_end)
  {
    return text_start == text_end;          // pattern must end here
  }
  if (text_start == text_end)
  {
    return false;                           // text ended without match
  }
  if (isQuantifierAdvanced(*pattern_start))
  {
    return false;                           // throw std::invalid_argument("Error: Invalid target for quantifier.");
  }
  if (*pattern_start == '^' || *pattern_start == '$')
  {
    return false;                           // misused meta characters making regex impossible to have a valid match
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
  return false;
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

bool quantifierLoop(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end,
  std::string::const_iterator quantifier_start,                   // pattern_start < quantifier_start < pattern_end
  bool negative_group_flag, int pattern_end_offset)
{
  auto matchers = matcherControlSetupBuilder(pattern_start, quantifier_start - pattern_end_offset);
  int min{};
  int max{};
  int index{};
  auto new_pattern_start = handleQuantifier(quantifier_start, pattern_end, min, max);

  for (; index < min && text_start != text_end; ++index, ++text_start)                // handle required minimum matches
  {
    if (!matcherControlSetupMemoized(matchers, *text_start, negative_group_flag))
    {
      return false;
    }
  }

  if (max == -1)                  // handle infinite optional matches loop
  {
    do
    {
      if (patternControl(text_start, text_end, new_pattern_start, pattern_end)) {                     // RECURSION CALL
        return true;
      }
    } while (text_start != text_end && matcherControlSetupMemoized(matchers, *(text_start++), negative_group_flag));
  }


  do                              // handle non-infinite optional matches loop
  {
    if (patternControl(text_start, text_end, new_pattern_start, pattern_end)) {                       // RECURSION CALL
      return true;
    }
  } while (max > index++ && text_start != text_end && matcherControlSetupMemoized(matchers, *(text_start++), negative_group_flag));

  return false;
}



bool handleGroups(std::string::const_iterator text_start, std::string::const_iterator text_end,
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
    auto group_end = std::find(pattern_start, pattern_end, ')');

    if (group_end == pattern_end)
    {
      throw std::runtime_error("Closing ')' not found");
    }

    return group_match_main(text_start, text_end, pattern_start + 1, group_end);
  }

  return false;
}



bool handleScapedCharacter(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start + 1 == pattern_end)
  {
    return false;                       // throw std::invalid_argument("Error: Dangling backslash.");
  }
  if (pattern_start + 2 != pattern_end && isQuantifierAdvanced(*(pattern_start + 2)))         // "\xq" where x is a character and q is a quantifier
  {
    return quantifierLoop(text_start, text_end, pattern_start, pattern_end, pattern_start + 2);
  }
  else if (matcherControlSetup(pattern_start, pattern_start + 2, *text_start))                // "\xz" where x is a character and z is not a quantifier, or
  {                                                                                           // "\x"  where x is a character and the pattern ends after x
    return patternControl(text_start + 1, text_end, pattern_start + 2, pattern_end);
  }
  return false;
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



bool group_match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return false;
  }

  auto orPosition = findOutermostOr(pattern_start, pattern_end);

  while (orPosition != pattern_end)
  {
    if (group_match_main(text_start, text_end, pattern_start, orPosition)) {
      return true;
    }
    pattern_start = orPosition + 1;
    orPosition = findOutermostOr(pattern_start, pattern_end);
  }
  
  return patternControl(text_start, text_end, pattern_start, pattern_end);
}



bool match_main(std::string::const_iterator text_start, std::string::const_iterator text_end,
  std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{
  if (pattern_start == pattern_end)
  {
    return false;
  }

  auto orPosition = findOutermostOr(pattern_start, pattern_end);

  while (orPosition != pattern_end)
  {
    if (match_main(text_start, text_end, pattern_start, orPosition)) {
      return true;
    }
    pattern_start = orPosition + 1;
    orPosition = findOutermostOr(pattern_start, pattern_end);
  }

  if (*pattern_start == '^') {
    return patternControl(text_start, text_end, pattern_start + 1, pattern_end);
  }

  do
  {
    if (patternControl(text_start, text_end, pattern_start, pattern_end))
    {
      return true;
    }
  } while (++text_start != text_end);
  return false;
}


bool match_pattern(const std::string& input_line, const std::string& pattern)
{
  return match_main(input_line.begin(), input_line.end(), pattern.begin(), pattern.end());
}

int main(int argc, char *argv[])
{
    // Flush after every std::cout / std::cerr
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;

    std::cout << "Logs from your program will appear here" << std::endl;

    if (argc != 3)
    {
        std::cerr << "Expected two arguments" << std::endl;
        return 1;
    }

    std::string flag = argv[1];
    std::string pattern = argv[2];

    if (flag != "-E")
    {
        std::cerr << "Expected first argument to be '-E'" << std::endl;
        return 1;
    }

    std::string input_line;
    std::getline(std::cin, input_line);

    try
    {
        if (match_pattern(input_line, pattern))
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    catch (const std::runtime_error &e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }
}
