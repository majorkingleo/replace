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
	static const std::wstring KEY_WORD;

public:
	FixStrForm();


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file ) override;
};



#endif /* CORRECT_STRFORM_H_ */
