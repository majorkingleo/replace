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
	  PDR_FILE,
	  CFG_FILE,
	  PMM_FILE,
	  PRC_FILE,
      PL_FILE,
	  ML_FILE,
	  SQL_FILE,
	  JAVA_FILE,
	  WSDD_FILE,
	  PY_FILE,
	  HTML_FILE,
	  SAP2WAMAS_FILE,
	  JSP_FILE,
	  LLR_FILE,
	  TSSIZE_FILE,
	  CMD_FILE,
      LAST__
    };
};

typedef Tools::EnumRange<EnumFile> FILE_TYPE;

FILE_TYPE get_file_type( const std::string & file, bool & is_cpp_file );
FILE_TYPE get_file_type( const std::string & file );


#endif
