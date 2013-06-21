#ifndef REMOVE_GENERIC_CAST_H
#define REMOVE_GENERIC_CAST_H

#include "HandleFile.h"

class RemoveGenericCast : public HandleFile
{
	static const std::string KEY_WORD;
	const std::string FUNCTION_NAME;

public:
	RemoveGenericCast(const std::string & FUNCTION_NAME_ =  "MskVaAssign" );


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );
};

#endif
