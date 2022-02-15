/*
 * DetectLocale.cc
 *
 *  Created on: 08.02.2022
 *      Author: martin
 */

#include "DetectLocale.h"
#include <locale>
#include <string_utils.h>
#include "utf8_util.h"
#include "read_file.h"
#include <format.h>
#include <cpp_util.h>

using namespace Tools;

DetectLocale DETECT_LOCALE;

DetectLocale::DetectLocale()
: is_utf8( false ),
  input_encoding(),
  language2encoding()
{
	init();
}

void DetectLocale::init()
{
	language2encoding["en"] = "ASCII";
	language2encoding["de"] = "ISO-8859-1"; // Deutsch
	language2encoding["it"] = "ISO-8859-1"; // Italienisch

	language2encoding["sk"] = "ISO-8859-2"; // Slowenisch
	language2encoding["sl"] = "ISO-8859-2"; // Slowakisch
	language2encoding["hr"] = "ISO-8859-2"; // Slowenisch

	language2encoding["tk"] = "ISO-8859-3"; // Türkisch
	language2encoding["mt"] = "ISO-8859-3"; // Maltesich
	language2encoding["et"] = "ISO-8859-3"; // Esperanto



	char *pcLocale = std::setlocale( LC_MESSAGES, "" );

	if( pcLocale == NULL ) {
		is_utf8 = false;
		return;
	}

	std::string locale = pcLocale;

	std::string converted_locale = toupper( locale );

	if( converted_locale.find( "UTF-8" ) != std::string::npos ) {
		is_utf8 = true;
		input_encoding = "UTF-8";
	}

	if( converted_locale.find( "UTF8" ) != std::string::npos ) {
		is_utf8 = true;
		input_encoding = "UTF-8";
	}

	if( !is_utf8 ) {
		// decode de_AT.UTF-8
		//        de_AT@EURO
		std::string::size_type pos_dot = locale.find( '.' );
		std::string str_encoding;

		if( pos_dot != std::string::npos  ) {
			str_encoding = locale.substr( pos_dot + 1 );

			std::string::size_type pos_modifier = str_encoding.find( '@' );

			if( pos_modifier != std::string::npos ) {
				str_encoding = str_encoding.substr(0,pos_modifier);
			}

			input_encoding = str_encoding;
		} else {

			// decode the language de_AT
			std::string::size_type pos_underscore = locale.find( '_' );

			if( pos_underscore != std::string::npos ) {
				input_encoding = locale.substr(0,pos_underscore);
			}
		}

		// try to detect one of ore predefined encodings for languages
		std::map<std::string,std::string>::iterator it = language2encoding.find( input_encoding );
		if( it != language2encoding.end() ) {
			input_encoding = it->second;
		}
	}
}

std::wstring DetectLocale::inputString2wString( const std::string & str )
{
	if( Utf8Util::isUtf8( str ) ) {
		return Utf8Util::utf8toWString( str );
	}

	std::string utf8_str;

	if( !input_encoding.empty() ) {
		if( READ_FILE.convert( str, "ISO-8859-1", "UTF-8", utf8_str ) ) {
			return Utf8Util::utf8toWString( utf8_str );
		} else {
			// std::cout << "error: " << READ_FILE.getError() << "str: '" << str << "'" << std::endl;
		}
	}

	throw REPORT_EXCEPTION( format( "cannot convert %s to utf8 detected input encoding: %s", str, input_encoding ) );

	return std::wstring();
}

std::string DetectLocale::wString2output( const std::wstring & str )
{
	std::string utf8_str = Utf8Util::wStringToUtf8( str );

	if( is_utf8 ) {
		return utf8_str;
	}

	std::string out_str;

	return READ_FILE.convert( utf8_str, "UTF-8", input_encoding );
}

std::string DetectLocale::w2out( const std::wstring & out )
{
	return DETECT_LOCALE.wString2output( out );
}

std::wstring DetectLocale::in2w( const std::string & in )
{
	return DETECT_LOCALE.inputString2wString( in );
}
