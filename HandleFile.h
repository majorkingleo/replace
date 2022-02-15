/*
 * HandleFile.h
 *
 *  Created on: 14.12.2011
 *      Author: martin
 */

#ifndef HANDLEFILE_H_
#define HANDLEFILE_H_

#include <vector>
#include <string>
#include "get_file_type.h"

class HandleFile
{
protected:
	std::vector<std::wstring> keywords;
	std::string file_name;


public:
	HandleFile();
	virtual ~HandleFile();

	bool should_skip_file( const std::wstring & file ) const;

	void set_file_name( const std::string file_name_ ) { file_name = file_name_; }

	virtual std::wstring patch_file( const std::wstring & file ) { return file; }

	virtual bool want_file( const FILE_TYPE & file_type )
	{
		return false;
	}

	virtual bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file )
	{
		return want_file( file_type );
	}

	// converts a w string to output format, for debugging purposes
	static std::string w2out( const std::wstring & out );

	// converts an ascii, or utf8 string from imput or file io to wstring
	static std::wstring in2w( const std::string & in );
};


#endif /* HANDLEFILE_H_ */
