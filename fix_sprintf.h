/*
 * correct_va_multiple_malloc.h
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#ifndef FIX_SPRINTF_H_
#define FIX_SPRINTF_H_


#include "HandleFile.h"

class FixSprintf : public HandleFile
{
	static const std::wstring KEY_WORD;

public:
	FixSprintf();


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file ) override;
};



#endif /* CORRECT_VA_MULTIPLE_MALLOC_H_ */
