#include "StdUtil.h"

std::size_t StdUtil::ReplaceAll(std::string& inout, std::string_view what, std::string_view with)
{
	std::size_t count{};
	for (std::string::size_type pos{};
		inout.npos != (pos = inout.find(what.data(), pos, what.length()));
		pos += with.length(), ++count)
		inout.replace(pos, what.length(), with.data(), with.length());
	return count;
}

bool StdUtil::EndsWith(std::string const& value, std::string const& ending)
{
	if (ending.size() > value.size()) return false;
	return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}
