#include "distributed_cache_engine/utils/string_utils.h"

#include <algorithm>
#include <cctype>
#include <sstream>

using namespace std;

namespace dce::utils
{
namespace
{
bool isEscapeSequenceTerminator(unsigned char character)
{
    return character >= 0x40U && character <= 0x7EU;
}
} // namespace

std::string trim(const std::string& input)
{
    const auto begin = find_if_not(
        input.begin(),
        input.end(),
        [](unsigned char character) { return isspace(character) != 0; }
    );

    if (begin == input.end())
    {
        return {};
    }

    const auto reverse_begin = find_if_not(
        input.rbegin(),
        input.rend(),
        [](unsigned char character) { return isspace(character) != 0; }
    );

    return string(begin, reverse_begin.base());
}

std::string sanitizeCommandInput(const std::string& input)
{
    string sanitized;
    sanitized.reserve(input.size());

    for (size_t index = 0; index < input.size(); ++index)
    {
        const unsigned char character = static_cast<unsigned char>(input[index]);

        if (character == 0x1BU)
        {
            if (index + 1U < input.size())
            {
                const unsigned char next = static_cast<unsigned char>(input[index + 1U]);
                if (next == '[' || next == 'O')
                {
                    index += 2U;
                    while (index < input.size() &&
                           !isEscapeSequenceTerminator(static_cast<unsigned char>(input[index])))
                    {
                        ++index;
                    }
                    continue;
                }

                ++index;
            }

            continue;
        }

        if (character == '\b' || character == 0x7FU)
        {
            if (!sanitized.empty())
            {
                sanitized.pop_back();
            }
            continue;
        }

        if (character == '\r' || character == '\n')
        {
            continue;
        }

        if (iscntrl(character) != 0)
        {
            continue;
        }

        sanitized.push_back(isspace(character) != 0 ? ' ' : static_cast<char>(character));
    }

    return trim(sanitized);
}

std::vector<std::string> splitWhitespace(const std::string& input)
{
    istringstream stream{input};
    vector<string> tokens;
    string token;

    while (stream >> token)
    {
        tokens.push_back(token);
    }

    return tokens;
}

std::string toUpper(std::string input)
{
    transform(
        input.begin(),
        input.end(),
        input.begin(),
        [](unsigned char character) {
            return static_cast<char>(toupper(character));
        }
    );
    return input;
}
} // namespace dce::utils
