#pragma once

#include <string>
#include <vector>

namespace dce::utils
{
std::string trim(const std::string& input);
std::string sanitizeCommandInput(const std::string& input);
std::vector<std::string> splitWhitespace(const std::string& input);
std::string toUpper(std::string input);
} // namespace dce::utils
