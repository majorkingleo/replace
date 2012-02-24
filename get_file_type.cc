#include "get_file_type.h"
#include <string_utils.h>

using namespace Tools;

FILE_TYPE get_file_type( const std::string & file, bool & is_cpp_file )
{
 // genauer nachschauen

  std::string::size_type pos = file.rfind( '.' );

  if( pos == std::string::npos || file.size() - pos > 5 )
    return FILE_TYPE::UNKNOWN;

  is_cpp_file = false;

  std::string ext = toupper(file.substr( pos + 1 ));

  if( ext == "C" )
    {
      return FILE_TYPE::C_FILE;
    }
  else if( ext == "CC" || ext == "CPP" || ext == "CXX" )
    {
      is_cpp_file = true;
      return FILE_TYPE::C_FILE;
    }
  else if( ext == "H" )
    {
      return FILE_TYPE::HEADER;
    }
  else if( ext == "HH" )
    {
      is_cpp_file = true;
      return FILE_TYPE::HEADER;
    }
  else if( ext == "RC" )
    {
      return FILE_TYPE::RC_FILE;
    }
  else if( ext == "PDL" )
    {
      return FILE_TYPE::PDL_FILE;
    }
  else if( ext == "PDS" )
    {
      return FILE_TYPE::PDS_FILE;
    }
  else if( ext == "PL" )
    {
      return FILE_TYPE::PL_FILE;
    }

  return FILE_TYPE::UNKNOWN;
}

FILE_TYPE get_file_type( const std::string & file )
{
    bool dummy;
    return get_file_type( file, dummy );
}
