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
	static const std::wstring KEY_WORD;
	std::vector<std::wstring> casts;

public:
	FixScopedCStr();


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file ) override;
};



#endif /* CORRECT_STRFORM_H_ */
