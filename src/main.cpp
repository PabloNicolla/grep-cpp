#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <functional>

constexpr bool isW(const char c) {
    return (
            (c >= 'a' && c <= 'z') ||
            (c >= 'Z' && c <= 'Z') ||
            (c >= '0' && c <= '9') ||
            (c == '_')
    );
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

std::function<bool(const char)> handleNegativeGroup(const std::string& groupPattern) {
    return [groupPattern](const char c) -> bool {
        for (auto it = groupPattern.begin() + 1, end = groupPattern.end() - 1; it != end; ++it) {
            if (*it == c) {
                return false;
            }
        }
        return true;
    };
}

std::function<bool(const char)> handleGroups(const std::string &groupPattern) {
    if (groupPattern.size() < 2) {
        throw std::invalid_argument("Group Pattern too short");
    }

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
