#include "get_file_type.h"
#include <string_utils.h>

using namespace Tools;

FILE_TYPE get_file_type( const std::string & file, bool & is_cpp_file )
{
 // genauer nachschauen

  if(  file == "sap2wamas" ) {
	  return FILE_TYPE::SAP2WAMAS_FILE;
  }

  if( file  == "LLR" ) {
	  return FILE_TYPE::LLR_FILE;
  }

  if( file  == "LLR_BG" ) {
	  return FILE_TYPE::LLR_FILE;
  }

  if( file  == "tssize" ) {
	  return FILE_TYPE::TSSIZE_FILE;
  }

  std::string::size_type pos = file.rfind( '.' );

  if( pos == std::string::npos || file.size() - pos > 5 )
    return FILE_TYPE::UNKNOWN;

  is_cpp_file = false;

  std::string ext = toupper(file.substr( pos + 1 ));

  if( ext == "C" || ext == "APC" )
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
  else if( ext == "PDR" )
  	{
	  return FILE_TYPE::PDR_FILE;
    }
  else if( ext == "CFG" )
  	{
	  return FILE_TYPE::CFG_FILE;
    }
  else if( ext == "PRC" )
  	{
	  return FILE_TYPE::PRC_FILE;
    }
  else if( ext == "PMM" )
  	{
	  return FILE_TYPE::PMM_FILE;
    }
  else if( ext == "ML" )
  	{
	  return FILE_TYPE::ML_FILE;
    }
  else if( ext == "SQL" )
  	{
	  return FILE_TYPE::SQL_FILE;
    }
  else if( ext == "JAVA" )
  	{
	  return FILE_TYPE::JAVA_FILE;
    }
  else if( ext == "WSDD" )
  	{
	  return FILE_TYPE::WSDD_FILE;
    }
  else if( ext == "PY" )
  	{
	  return FILE_TYPE::PY_FILE;
    }
  else if( ext == "HTML" )
  	{
	  return FILE_TYPE::HTML_FILE;
    }
  else if( ext == "PL" )
    {
      return FILE_TYPE::PL_FILE;
    }
  else if( ext == "JSP" )
    {
      return FILE_TYPE::JSP_FILE;
    }
  else if( ext == "CMD" )
    {
      return FILE_TYPE::CMD_FILE;
    }

  return FILE_TYPE::UNKNOWN;
}

FILE_TYPE get_file_type( const std::string & file )
{
    bool dummy;
    return get_file_type( file, dummy );
}
