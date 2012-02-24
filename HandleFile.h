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

class HandleFile
{
protected:
	std::vector<std::string> keywords;


public:
	HandleFile();
	virtual ~HandleFile();

	bool should_skip_file( const std::string & file ) const;
};


#endif /* HANDLEFILE_H_ */
