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

using namespace Tools;

UninitializedVariableHandler::UninitializedVariableHandler()
	: UnusedVariableHandler( false )
{

}

void UninitializedVariableHandler::read_compile_log_line( const std::string & line )
{
	if( line.find( "warning:") == std::string::npos ||
	    line.find( "may be used uninitialized in this function") == std::string::npos )
		return;

	UnusedVarWarnigs location = get_location_from_line( line );

	std::vector<std::string> sl = split_simple( line, " ");

	// ./me_storno.c:74: warning: ‘mam’ may be used uninitialized in this function

	bool use_next = false;

	for( unsigned i = 0; i < sl.size(); i++ )
	{
		if( sl[i] == "warning:") {
			use_next = true;
		} else if( use_next ) {
			location.var_name = sl[i];
			location.var_name = strip( location.var_name, "'‘’");
			location.compile_log_line = line;

			DEBUG( format( "%s %s", location, location.var_name ) );

			unused_variables_locations.push_back( location );
			break;
		}
	}
}

void UninitializedVariableHandler::fix_warning( UnusedVarWarnigs & warning, std::string & content )
{
	std::string::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::string::npos ) {
		throw REPORT_EXCEPTION( format( "can't get file position for warning %s", warning.compile_log_line));
	}

	std::string line = getline(content,  pos );

	DEBUG( line );


	// 1st find starting pos

	std::string::size_type var_start = find_var_name( line, warning.var_name );

	DEBUG( format("%s var_start: %d", warning.var_name, var_start) );

	if( var_start == std::string::npos )
		return;

	std::string left = line.substr(0, var_start + warning.var_name.size() );
	std::string right = line.substr( var_start + warning.var_name.size() );

	std::string new_line = left + "=0" + right;

	replace_line( content, pos, new_line );

	warning.fixed = true;
}

