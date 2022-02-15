/*
 * uninitialized_variable_handler.cc
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */
#include "uninitialized_variable_handler.h"
#include "debug.h"
#include <string_utils.h>
#include <getline.h>
#include <cpp_util.h>
#include "DetectLocale.h"

using namespace Tools;

UninitializedVariableHandler::UninitializedVariableHandler()
	: UnusedVariableHandler( false )
{

}

void UninitializedVariableHandler::read_compile_log_line( const std::wstring & line )
{
	if( line.find( L"warning:") == std::wstring::npos ||
	    line.find( L"may be used uninitialized in this function") == std::wstring::npos )
		return;

	UnusedVarWarnigs location = get_location_from_line( line );

	std::vector<std::wstring> sl = split_simple( line, L" ");

	// ./me_storno.c:74: warning: ‘mam’ may be used uninitialized in this function

	bool use_next = false;

	for( unsigned i = 0; i < sl.size(); i++ )
	{
		if( sl[i] == L"warning:") {
			use_next = true;
		} else if( use_next ) {
			location.var_name = sl[i];
			location.var_name = strip( location.var_name, L"'‘’");
			location.compile_log_line = line;

			DEBUG( wformat( L"%s %s", location, location.var_name ) );

			unused_variables_locations.push_back( location );
			break;
		}
	}
}

void UninitializedVariableHandler::fix_warning( UnusedVarWarnigs & warning, std::wstring & content )
{
	std::wstring::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::wstring::npos ) {
		throw REPORT_EXCEPTION( format( "can't get file position for warning %s", DetectLocale::w2out(warning.compile_log_line)));
	}

	std::wstring line = getline(content,  pos );

	DEBUG( line );


	// 1st find starting pos

	std::wstring::size_type var_start = find_var_name( line, warning.var_name );

	DEBUG( wformat( L"%s var_start: %d", warning.var_name, var_start) );

	if( var_start == std::string::npos )
		return;

	std::wstring left = line.substr(0, var_start + warning.var_name.size() );
	std::wstring right = line.substr( var_start + warning.var_name.size() );

	std::wstring new_line = left + L"=0" + right;

	replace_line( content, pos, new_line );

	warning.fixed = true;
}

