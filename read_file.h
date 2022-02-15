/*
 * read_file.h
 *
 *  Created on: 10.02.2022
 *      Author: martin
 */

#ifndef READ_FILE_H_
#define READ_FILE_H_

#include <string>

class ReadFile
{
	std::string error;

	std::string encoding;

public:
	ReadFile();

	bool read_file( const std::string & name, std::wstring & content );

	const std::string & getError() const {
		return error;
	}

	std::string convert( const std::string & s, const std::string & from, const std::string & to );
	bool convert( const std::string & s, const std::string & from, const std::string & to, std::string & result );

	std::string latin12utf8( const std::string & s ) {
	    return convert( s, "LATIN1", "UTF-8" );
	}

	std::string utf82latin1( const std::string & s ) {
	    return convert( s, "UTF-8", "LATIN1" );
	}

	const std::string getFileEncoding() const {
		return encoding;
	}

};

extern ReadFile READ_FILE;

#endif /* READ_FILE_H_ */
