/*
 * correct_va_multiple_malloc.h
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#ifndef CORRECT_VA_MULTIPLE_MALLOC_H_
#define CORRECT_VA_MULTIPLE_MALLOC_H_


#include "HandleFile.h"

class CorrectVaMultipleMalloc : public HandleFile
{
	static const std::wstring KEY_WORD;
	static const std::wstring VA_MALLOC;

	bool isTB2020;

public:
	CorrectVaMultipleMalloc( bool isTB2020_ = true);


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;
};



#endif /* CORRECT_VA_MULTIPLE_MALLOC_H_ */
