/*
 * fix_StrForm.h
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#ifndef FIX_STRFORM_H_
#define FIX_STRFORM_H_


#include "HandleFile.h"

class FixStrForm : public HandleFile
{
	static const std::string KEY_WORD;

public:
	FixStrForm();


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file );
};



#endif /* CORRECT_STRFORM_H_ */
