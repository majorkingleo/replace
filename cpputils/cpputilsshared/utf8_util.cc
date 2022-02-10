/*
 * utf8_util.cc
 *
 *  Created on: 10.02.2022
 *      Author: martin
 */
#include "utf8_util.h"
#include "utf8.h"

namespace Tools {

bool Utf8Util::isUtf8( const std::string & text_ )
{
	std::string text = text_;

	std::string::iterator end_it = utf8::find_invalid(text.begin(), text.end());

	if( end_it != text.end() ) {
		return false;
	}

	return true;
}

bool Utf8Util::isAsciiOnly( const std::string & text )
{
	for( unsigned i = 0; i < text.size(); i++ ) {
		if( !isascii( text[i]) ) {
			return false;
		}
	}

	return true;
}

std::wstring Utf8Util::toWcharString16( const std::string & text )
{
	std::u16string str = utf8::utf8to16(text);
	std::wstring wstr;

	wstr.resize(str.size());

	for( unsigned i = 0; i < str.size(); i++ ) {
		wstr[i] = str[i];
	}

	return wstr;
}

std::wstring Utf8Util::toWcharString32( const std::string & text )
{
	std::u32string str = utf8::utf8to32(text);

	std::wstring wstr;

	wstr.resize(str.size());

	for( unsigned i = 0; i < str.size(); i++ ) {
		wstr[i] = str[i];
	}

	return wstr;
}

std::wstring Utf8Util::utf8toWString( const std::string & text )
{
	if( sizeof( wchar_t ) == sizeof( int16_t ) ) {
		return toWcharString16( text );
	} else {
		return toWcharString32( text );
	}
}

} // namespace Tools;


