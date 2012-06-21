/*
 * OwCallback1.h
 *
 *  Created on: 19.04.2012
 *      Author: martin
 */

#ifndef OWCALLBACK1_H_
#define OWCALLBACK1_H_

#include "HandleFile.h"


class OwCallback1 : public HandleFile
{
public:
	OwCallback1();

	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

protected:
	void fix_owcallbackx( std::string & file, int num );
};


#endif /* OWCALLBACK1_H_ */
