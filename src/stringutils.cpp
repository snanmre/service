#include "stringutils.h"

#include <algorithm>

using namespace std;

std::string stringutils::trim(const std::string& s)
{
	string str = s;

	str.erase(str.begin(),
			std::find_if(str.begin(), str.end(),
					std::not1(std::ptr_fun<int, int>(std::isspace))));
	str.erase(
			std::find_if(str.rbegin(), str.rend(),
					std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
			str.end());

	return str;
}

void stringutils::split(const std::string& str, const std::string& delim,
		std::vector<std::string>& parts, bool keepEmptyParts, bool trim)
{
	size_t end = 0;
	parts.clear();
	string copyStr = str;

	while (!copyStr.empty())
	{
		end = copyStr.find(delim);
		if (end > copyStr.length())
			end = copyStr.length();

		if (keepEmptyParts || end != 0)
		{
			if (trim)
			{
				string s = stringutils::trim(copyStr.substr(0, end));
				if (keepEmptyParts || !s.empty())
					parts.push_back(s);
			}
			else
				parts.push_back(copyStr.substr(0, end));
		}

		++end;

		if (end > copyStr.length())
			break;

		copyStr = copyStr.substr(end);
	}
}

void stringutils::splitLine(const std::string& str,
		std::vector<std::string>& parts)
{
	split(str, "\n", parts);
}
