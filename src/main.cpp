#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>

constexpr bool isAnyMetaCharacter(const char c) {
    return (
            (c == '^') ||
            (c == '$') ||

            (c == '[') ||
            (c == ']') ||
            (c == '{') ||
            (c == '}') ||
            (c == '(') ||
            (c == ')') ||

            (c == '+') ||
            (c == '*') ||
            (c == '?') ||

            (c == '.') ||

            (c == '\\') ||

            (c == '|')
    );
}

constexpr bool isQuantifier(const char c) {
    return (
            (c == '+') ||
            (c == '*') ||
            (c == '?')
    );
}

constexpr bool isCharacterClass(const char c) {
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









constexpr bool is_w(const char c) {
    return (
            (c >= 'a' && c <= 'z') ||
            (c >= 'Z' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '_')
    );
}
constexpr bool is_W(const char c) {
    return !is_w(c);
}

constexpr bool is_d(const char c) {
    return isdigit(c);
}
constexpr bool is_D(const char c) {
    return !is_d(c);
}







std::function<bool(const char)> characterClassSelector(const char c) {
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



std::function<bool(const char)> plainCharacterMatcherBuilder(const std::vector<const char>& plainChars) {
    return [&plainChars](const char c) -> bool {
        for (const auto plainChar : plainChars) {
            if (plainChar == c) {
                return true;
            }
        }
        return false;
    };
}










bool matcherControl(const std::vector<std::function<bool(const char)>>& matchers, const char c) {
    for (const auto& matcher : matchers) {
        if (matcher(c)) {
            return true;
        }
    }
    return false;
}

bool matcherControlSetup(const std::string& simple_sub_pattern, const char c) {
    std::vector<std::function<bool(const char)>> matchers{};
    std::vector<const char> plainChars{};

    auto it = simple_sub_pattern.begin();
    while (it != simple_sub_pattern.end()) {
        // Early exit if sub_pattern contains `.`
        if (*it == '.') {
            return true;
        }
        // handle scape character
        else if (*it == '\\') {
            if (it + 1 == simple_sub_pattern.end()) {
                throw std::invalid_argument("Error: Dangling backslash.");
            }
            else if (isCharacterClass(*(it + 1))) {
                matchers.push_back(characterClassSelector(*(it + 1)));
            }
            else {
                plainChars.push_back(*(it + 1));
            }
            it += 2;
        }
        // plain character
        else {
            plainChars.push_back(*it);
            ++it;
        }
    }

    matchers.push_back(plainCharacterMatcherBuilder(plainChars));
    return matcherControl(matchers, c);
}








std::function<bool(const char)> handlePositiveGroup(const std::string& groupPattern) {
    return [groupPattern](const char c) -> bool {
        for (auto it = groupPattern.begin() + 1, end = groupPattern.end() - 1; it != end; ++it) {
            if (*it == c) {
                return true;
            }
        }
        return false;
    };
}

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