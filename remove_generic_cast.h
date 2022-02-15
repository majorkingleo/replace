#ifndef REMOVE_GENERIC_CAST_H
#define REMOVE_GENERIC_CAST_H

#include "HandleFile.h"

class RemoveGenericCast : public HandleFile
{
	static const std::wstring KEY_WORD;
	const std::wstring FUNCTION_NAME;

public:
	RemoveGenericCast(const std::wstring & FUNCTION_NAME_ =  L"MskVaAssign" );


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;
};

#endif
