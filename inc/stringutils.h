#ifndef STRINGUTILS_H_
#define STRINGUTILS_H_

#include <sstream>
#include <string>
#include <vector>

namespace stringutils
{
std::string trim(const std::string & s);

void split(const std::string& str, const std::string& delim,
		std::vector<std::string>& parts, bool keepEmptyParts = true, bool trim =
				false);

void splitLine(const std::string& str, std::vector<std::string>& parts);

template<typename T>
static std::string to_string(T t)
{
	std::ostringstream oss;
	oss << t;
	return oss.str();
}
}

#endif /* STRINGUTILS_H_ */
