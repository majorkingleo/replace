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

public:
	CorrectVaMultipleMalloc();


	virtual std::wstring patch_file( const std::wstring & file );

	virtual bool want_file( const FILE_TYPE & file_type );
};



#endif /* CORRECT_VA_MULTIPLE_MALLOC_H_ */
