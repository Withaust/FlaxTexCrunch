#pragma once
#include <string>

class StdUtil
{
public:
    static std::size_t ReplaceAll(std::string& inout, std::string_view what, std::string_view with);
    static bool EndsWith(std::string const& value, std::string const& ending);
};