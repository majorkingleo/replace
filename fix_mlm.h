/*
 * fix_mlm.h
 *
 *  Created on: 05.06.2012
 *      Author: martin
 */

#ifndef FIX_MLM_H_
#define FIX_MLM_H_


#include "HandleFile.h"


class FixMlM : public HandleFile
{
public:
	FixMlM();

	virtual std::string patch_file( const std::string & file );
	virtual bool want_file( const FILE_TYPE & file_type );
};


#endif /* FIX_MLM_H_ */
