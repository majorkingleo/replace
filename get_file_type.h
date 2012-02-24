#ifndef GET_FILE_TYPE_H
#define GET_FILE_TYPE_H

#include <string>
#include "range.h"

struct EnumFile {
  enum ETYPE
    {
      FIRST__ = -1,
      UNKNOWN = 0,
      HEADER,
      C_FILE,
      RC_FILE,
      PDL_FILE,
      PDS_FILE,
      PL_FILE,
      LAST__
    };
};

typedef Tools::EnumRange<EnumFile> FILE_TYPE;

FILE_TYPE get_file_type( const std::string & file, bool & is_cpp_file );
FILE_TYPE get_file_type( const std::string & file );


#endif
