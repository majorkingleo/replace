/*
 * DetectLocale.h
 *
 *  Created on: 08.02.2022
 *      Author: martin
 */

#ifndef DETECTLOCALE_H_
#define DETECTLOCALE_H_

#include <string>
#include <map>

class DetectLocale
{
	bool is_utf8;
	std::string input_encoding;
	std::map<std::string,std::string> language2encoding;

public:
	DetectLocale();

	void init();

	bool isUtf8() const {
		return is_utf8;
	}

	const std::string & getInputEncoding() const {
		return input_encoding;
	}

	std::wstring inputString2wString( const std::string & str );
	std::string wString2output( const std::wstring & str );
};



#endif /* DETECTLOCALE_H_ */
