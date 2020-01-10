#ifndef FIX_PRMGET_H
#define FIX_PRMGET_H

#include "HandleFile.h"

class FixPrmGet : public HandleFile
{
	const std::string FUNCTION_NAME;
	const unsigned LONG_ARG_NUM;

public:
	FixPrmGet(const std::string & FUNCTION_NAME_ =  "PrmGet1Parameter",
			  const unsigned LONG_ARG_NUM = 4 );


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

	void replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line );
};

#endif
