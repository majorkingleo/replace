#ifndef ADD_CAST_H
#define ADD_CAST_H

#include "HandleFile.h"

class AddCast : public HandleFile
{
	const std::wstring FUNCTION_NAME;

public:
	AddCast(const std::wstring & FUNCTION_NAME_ =  L"ArrWalkStart");


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

	void replace_line_from_start_of_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line );
};

#endif
