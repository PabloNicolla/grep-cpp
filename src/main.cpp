#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>

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

constexpr bool isGroup (const char c)
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

constexpr bool is_d(const char c)
{
    return isdigit(c);
}
constexpr bool is_D(const char c)
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







std::function<bool(const char)> plainCharacterMatcherBuilder(const std::vector<const char>& plainChars) 
{
    return [&plainChars](const char c) -> bool 
    {
        for (const auto plainChar : plainChars)
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

bool matcherControlSetup(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end, const char c)
{
    std::vector<std::function<bool(const char)>> matchers{};
    std::vector<const char> plainChars{};

    while (pattern_start != pattern_end)
    {
        if (*pattern_start == '.')                      // Early exit if sub_pattern contains `.`
        {
            return true;
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
                plainChars.push_back(*(pattern_start + 1));
            }
            pattern_start += 2;
        }
        else                                            // plain character
        {
            plainChars.push_back(*pattern_start);
            ++pattern_start;
        }
    }

    matchers.push_back(plainCharacterMatcherBuilder(plainChars));
    return matcherControl(matchers, c);
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
        //handle groups
    }

    // if (*pattern_start == '|') // must be handled elsewhere{}

    if (*pattern_start == '\\')
    {
        if (pattern_start + 1 == pattern_end)
        {
            return false; // throw std::invalid_argument("Error: Dangling backslash.");
        }
        // "\xq" where x is character and q is quantifier
        if (pattern_start + 2 != pattern_end && isQuantifierAdvanced(*(pattern_start + 2)))
        {
            // handle quantifier
        } 
        // "\xz" where x is character and z is not quantifier
        // "\x"  where x is character and the pattern ends after x
        else if (matcherControlSetup(pattern_start, pattern_start + 2, *text_start))
        {
            return patternControl(text_start + 1, text_end, pattern_start + 2, pattern_end);
        }
    }
    if (pattern_start + 1 != pattern_end && isQuantifierAdvanced(*(pattern_start + 1)))
    {
        // handle quantifier
    }
    if (matcherControlSetup(pattern_start, pattern_start + 1, *text_start))
    {
        return patternControl(text_start + 1, text_end, pattern_start + 1, pattern_end);
    }
    return false;
}

bool handleQuantifier(std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)
{

}

bool quantifierLoop(std::string::const_iterator text_start, std::string::const_iterator text_end, std::string::const_iterator pattern_start, std::string::const_iterator pattern_end)


std::function<bool(const char)> handleGroups(const std::string &groupPattern) {
    // if (groupPattern.size() < 2) {
    //     throw std::invalid_argument("Group Pattern too short");
    // }

    if (groupPattern[1] != '^') return handlePositiveGroup(groupPattern);
    else if (groupPattern[1] == '^') return handleNegativeGroup(groupPattern);
}

bool match_pattern(const std::string &input_line, const std::string &pattern)
{
    if (pattern.length() == 1)
    {
        return input_line.find(pattern) != std::string::npos;
    }
    else if (pattern == "\\d")
    {
        return std::find_if(input_line.begin(), input_line.end(), [](const auto c) -> bool { return std::isdigit(c); }) != input_line.end();
    }
    else if (pattern == "\\w")
    {
        return std::find_if(input_line.begin(), input_line.end(), isW) != input_line.end();
    }
    else if (pattern.length() > 1 && pattern[0] == '[' && pattern.back() == ']')
    {
        auto predicate = handleGroups(pattern);
        return std::find_if(input_line.begin(), input_line.end(), predicate) != input_line.end();
    }
    else
    {
        throw std::runtime_error("Unhandled pattern " + pattern);
    }
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


void match