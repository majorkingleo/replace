#ifndef FIND_FILES_H
#define FIND_FILES_H

#include <list>
#include <string>
#include <map>
#include "get_file_type.h"

class Files
{
	FILE_TYPE type;
	bool is_cpp_file;
	std::string path;

public:
	Files( const std::string & path_, FILE_TYPE type_, bool is_cpp_file_ )
	: type( type_ ),
	  is_cpp_file( is_cpp_file_),
	  path( path_ )
	{}

	Files( const Files & other )
	: type( other.type ),
	  is_cpp_file( other.is_cpp_file ),
	  path( other.path )
	{}

	Files & operator=( const Files & other )
	{
		type = other.type;
		is_cpp_file = other.is_cpp_file;
		path = other.path;

		return *this;
	}

	const FILE_TYPE & getType() const { return type; }

	const std::string & getPath() const { return path; }

	bool isCppFile() const { return is_cpp_file; }
};

typedef std::list<Files> FILE_SEARCH_LIST;

bool find_files( const std::string & path, FILE_SEARCH_LIST & files );

#endif
