#ifndef ADD_CAST_H
#define ADD_CAST_H

#include "HandleFile.h"

class AddCast : public HandleFile
{
	const std::string FUNCTION_NAME;

public:
	AddCast(const std::string & FUNCTION_NAME_ =  "ArrWalkStart");


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

	void replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line );
};

#endif
