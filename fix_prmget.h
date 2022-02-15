#ifndef FIX_PRMGET_H
#define FIX_PRMGET_H

#include "HandleFile.h"

class FixPrmGet : public HandleFile
{
	const std::wstring FUNCTION_NAME;
	const unsigned LONG_ARG_NUM;

public:
	FixPrmGet(const std::wstring & FUNCTION_NAME_ =  L"PrmGet1Parameter",
			  const unsigned LONG_ARG_NUM = 4 );


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

	void replace_line_from_start_of_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line );
};

#endif
