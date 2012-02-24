#ifndef FIND_FILES_H
#define FIND_FILES_H

#include <vector>
#include <string>
#include <map>
#include "get_file_type.h"

bool find_files( const std::string & path,
				   std::vector<std::pair<FILE_TYPE,std::string> > & files );

#endif
