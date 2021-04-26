#ifndef FIX_CONVERSION_NULL_H
#define FIX_CONVERSION_NULL_H

#include "HandleFile.h"

class FixConversionNull : public HandleFile
{
	const std::string FUNCTION_NAME;
	const unsigned LONG_ARG_NUM;

public:
	FixConversionNull(const std::string & FUNCTION_NAME_ =  "ArrCreate",
			  const unsigned LONG_ARG_NUM = 4 );


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

	void replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line );
};

#endif
