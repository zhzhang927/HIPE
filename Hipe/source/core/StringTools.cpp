//@HIPE_LICENSE@
#include <core/StringTools.h>
#include <algorithm>


/**
 * \brief Convert String to hexa code
 * \param s the string to convert
 * \param upper_case check if it's case sensitive
 * \return the value in a 64 bit int format
 */
hUInt64 ToHex(const std::string& s, bool upper_case)
{

	std::stringstream ret;

	for (std::string::size_type i = 0; i < s.length(); ++i)
		ret << std::hex << std::setfill('0') << std::setw(2) << (upper_case ? std::uppercase : std::nouppercase) << static_cast<hUInt64>(s[i]);


	hUInt64 value;
	std::stringstream iss;
	value = strtoull(ret.str().c_str(), NULL, 16);
	

	return value;
}

std::string HexAsString(hUInt64 value, int base, int minSize = 1)
{
	static char const digits[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	
	std::string results;
	while (value != 0 || results.size() < (unsigned int) minSize) {
		results += digits[value % base];
		value /= base;
	}
	std::reverse(results.begin(), results.end());
	return results;
}

std::string ToString(hUInt64 n)
{

	std::string hex = HexAsString(n, 16);

	int len = hex.length();
	std::string newString;
	for (int i = 0; i< len; i += 2)
	{
		std::string byte = hex.substr(i, 2);
		char chr = (char)(int)strtoull(byte.c_str(), nullptr, 16);
		newString.push_back(chr);
	}
	return newString;
}


std::string removeDirectoryName(std::string pathname)
{
	std::string sep;
#ifdef WIN32
	sep = "\\";
#else
	sep = "/";
#endif
	if (pathname.find(sep) == -1)
		return pathname;

	return pathname.substr(pathname.find_last_of(sep), pathname.size());

}

std::string extractDirectoryName(const std::string& str)
{
	size_t found;
	
	found = str.find_last_of("/\\");
	return str.substr(0, found);
	
}

std::string removeFileExtension(const std::string&str)
{
	size_t found;

	found = str.find_last_of(".");
	return str.substr(0, found);
}

std::string extractFileName(const std::string&str)
{
	std::string tmp = removeDirectoryName(str);
	tmp = removeFileExtension(tmp);
	size_t found;

	found = str.find_first_of("/\\");

	if (found == std::string::npos) found = 0;
	else
		found--;

	return tmp.substr(found, str.size());
}
