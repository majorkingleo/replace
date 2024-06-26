/*
 * reset_versid.cc
 *
 *  Created on: 21.12.2022
 *      Author: martin
 */
#include "reset_versid.h"
#include "find_first_of.h"
#include "utils.h"
#include "CpputilsDebug.h"
#include "string_utils.h"
#include <format.h>

using namespace Tools;

ResetVersid::ResetVersid()
: HandleFile()
{

}

bool ResetVersid::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::HEADER:
	  case FILE_TYPE::C_FILE:
	  case FILE_TYPE::PL_FILE:
	  case FILE_TYPE::RC_FILE:
	  case FILE_TYPE::PDL_FILE:
	  case FILE_TYPE::PDS_FILE:
	  case FILE_TYPE::PDR_FILE:
	  case FILE_TYPE::CFG_FILE:
	  case FILE_TYPE::PMM_FILE:
	  case FILE_TYPE::PRC_FILE:
	  case FILE_TYPE::ML_FILE:
	  case FILE_TYPE::SQL_FILE:
	  case FILE_TYPE::JAVA_FILE:
	  case FILE_TYPE::WSDD_FILE:
	  case FILE_TYPE::PY_FILE:
	  case FILE_TYPE::HTML_FILE:
	  case FILE_TYPE::SAP2WAMAS_FILE:
	  case FILE_TYPE::JSP_FILE:
	  case FILE_TYPE::LLR_FILE:
	  case FILE_TYPE::TSSIZE_FILE:
	  case FILE_TYPE::CMD_FILE:
		  return true;
	  default:
		  return false;
	}
}

std::wstring ResetVersid::reset_versid_once(  const std::wstring & file, const std::wstring & KEYWORD  )
{
	std::wstring::size_type pos = find_first_of( file, 0, KEYWORD );

	if( pos == std::wstring::npos ) {
		CPPDEBUG( wformat( L"keyword '%s' not found", KEYWORD ) );
		return file;
	}
/*
	if( is_in_string( file, pos ) ) {
		DEBUG( L"is in a string" );
		return file;
	}
*/

	std::wstring line = get_whole_line( file, pos );

	if( line.find( KEYWORD ) == std::wstring::npos ) {
		CPPDEBUG( wformat( L"keyword '%s' not found", KEYWORD ) );
		return file;
	}

	std::wstring::size_type begin = pos + KEYWORD.size()-1;
	std::wstring::size_type end = file.find( L'$', begin );

	std::wstring result;

	if( begin != std::wstring::npos )
		result = file.substr(0,begin);

	result += file.substr(end);

	return result;
}

/* changes this revision log
 *
+* REVISION HISTORY:
+*   $Log: me_mlmexp.h,v $
+*   Revision 1.1  2017/12/06 13:09:42  wamas
+*   MLM-Menus integrated into WAMAS-M; user:ghoer
+*
+*   Revision 1.2  2001/12/03 15:30:36  rschick
+*   Bugfix
+*   Neue Funktionen Import/Export (Wamas-K Version)
 *
 *
 * to this one
 *
+* REVISION HISTORY:
+*   $Log$
+*   Revision 1.2  2001/12/03 15:30:36  rschick
+*   Bugfix
+*   Neue Funktionen Import/Export (Wamas-K Version)
 *
*/
std::wstring ResetVersid::reset_versid_log( const std::wstring & file )
{
	static const std::wstring KEYWORD_LOG = L"$Log$";
	static const std::wstring KEYWORD_REVISION = L"Revision";

	std::wstring::size_type pos = file.find( KEYWORD_LOG );

	if( pos == std::wstring::npos ) {
		CPPDEBUG( L"keyword $Log$ not found" );
		// std::wcout << file << std::endl;
		return file;
	}

	std::wstring line = get_whole_line( file, pos );

	// +*  $Log$
	std::wstring::size_type pos_of_log = line.find_first_of( KEYWORD_LOG );

	// +*
	std::wstring next_log = line.substr( 0, pos_of_log ) + KEYWORD_REVISION;

	std::wstring::size_type next_log_pos = file.find( next_log, pos + KEYWORD_LOG.size() + 1 + KEYWORD_REVISION.size() );

	if( next_log_pos == std::wstring::npos ) {
		CPPDEBUG( wformat( L"next log pos '%s' not found", next_log ) );

		std::wstring comment_prefix = line.substr( 0, pos_of_log );
		comment_prefix = strip_trailing( comment_prefix, L" " );
		std::wstring empty_line = comment_prefix + L"\n";

		next_log_pos = file.find( empty_line, pos + KEYWORD_LOG.size() );

		if( next_log_pos == std::wstring::npos ) {
			CPPDEBUG( wformat( L"empty line pos '%s' not found", empty_line ) );
			return file;
		} else {
			// remove also this empty line
			next_log_pos += empty_line.size();
		}
	}

	CPPDEBUG( wformat( L"found next log '%s' at line: %d", next_log, get_linenum( file, next_log_pos ) ) );

	std::wstring::size_type start_of_log = file.find( L'\n', pos );

	if( start_of_log == std::wstring::npos ) {
		CPPDEBUG( L"start of log not found" );
		return file;
	}

	std::wstring first_part = file.substr( 0, start_of_log + 1 );
	std::wstring second_part = file.substr( next_log_pos );

	return first_part + second_part;
}

std::wstring ResetVersid::reset_versid(  const std::wstring & file, const std::wstring & KEYWORD )
{
	std::wstring res = file;

	while( res.find( KEYWORD ) != std::string::npos ) {
		std::wstring result = reset_versid_once( res, KEYWORD );
		if( result == res ) {
			return res;
		}

		res = result;
	}

	return res;
}



