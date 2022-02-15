/*
 * utf8_util.h
 *
 *  Created on: 10.02.2022
 *      Author: martin
 */

#ifndef CPPUTILS_CPPUTILSSHARED_UTF8_UTIL_H_
#define CPPUTILS_CPPUTILSSHARED_UTF8_UTIL_H_

#include <string>

#if __cplusplus >= 201103

namespace Tools {

class Utf8Util
{
public:
	static bool isUtf8( const std::string & text );

	static bool isAsciiOnly( const std::string & text );

	static std::wstring utf8toWString( const std::string & text );

	static std::wstring toWcharString16( const std::string & text );
	static std::wstring toWcharString32( const std::string & text );

	static std::string wStringToUtf8( const std::wstring & text );

	static std::string utf16toString( const std::wstring & text );
	static std::string utf32toString( const std::wstring & text );
};

}

#endif

#endif /* CPPUTILS_CPPUTILSSHARED_UTF8_UTIL_H_ */
