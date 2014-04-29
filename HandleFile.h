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


public:
	HandleFile();
	virtual ~HandleFile();

	bool should_skip_file( const std::string & file ) const;

	virtual std::string patch_file( const std::string & file ) { return file; }

	virtual bool want_file( const FILE_TYPE & file_type ) { return false; }
};


#endif /* HANDLEFILE_H_ */
