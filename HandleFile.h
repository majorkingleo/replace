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
	std::vector<std::string> keywords;
	std::string file_name;


public:
	HandleFile();
	virtual ~HandleFile();

	bool should_skip_file( const std::string & file ) const;

	void set_file_name( const std::string file_name_ ) { file_name = file_name_; }

	virtual std::string patch_file( const std::string & file ) { return file; }

	virtual bool want_file( const FILE_TYPE & file_type )
	{
		return false;
	}

	virtual bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file )
	{
		return want_file( file_type );
	}
};


#endif /* HANDLEFILE_H_ */
