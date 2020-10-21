/*
 * fix_scoped_c_str.h
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#ifndef FIX_SCOPED_C_STR_H_
#define FIX_SCOPED_C_STR_H_


#include "HandleFile.h"

class FixScopedCStr : public HandleFile
{
	static const std::string KEY_WORD;
	std::vector<std::string> casts;

public:
	FixScopedCStr();


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file );
};



#endif /* CORRECT_STRFORM_H_ */
