/*
 * format_string_handler.cc
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */

#include "format_string_handler.h"
#include "cpp_utils.h"
#include "debug.h"
#include "getline.h"
#include "utils.h"

using namespace Tools;

FormatStringHandler::FormatStringHandler()
: FixFromCompileLog::Handler(),
  format_warnings_locations()
{

}

void FormatStringHandler::read_compile_log_line( const std::string & line )
{
	if( line.find( "warning: format") == std::string::npos ||
	    line.find( "expects type") == std::string::npos ||
	    line.find( "but argument")  == std::string::npos ||
	    line.find( "has type")  == std::string::npos )
		return;

	FormatWarnigs location = get_location_from_line( line );

	std::vector<std::string> sl = split_simple( line, " ");

	bool use_next = false;

	// ./me_kpl.c:291: warning: format ‘%s’ expects type ‘char *’, but argument 3 has type ‘int’

	for( unsigned i = 0; i + 9 < sl.size(); i++ )
	{
		if( sl[i] == "format" ) {
			use_next = true;
		} else if ( use_next ) {
			location.format = sl[i];
			location.format = strip( location.format, "'‘’" );

			location.expected_type = sl[i+3];
			location.expected_type = strip(location.expected_type, "'‘’," );

			location.argnum = s2x<int>( sl[i+6], -1 );

			if( location.argnum < 0 )
				return;

			location.target_type =  sl[i+9];

			location.target_type = strip(location.target_type, "'‘’" );
			break;
		}
	}

	location.compile_log_line = line;

	DEBUG( format( "%s format string: '%s' expected type: '%d' target_type: '%d' argument: %d",
					location,
					location.format,
					location.expected_type,
					location.target_type,
					location.argnum) );

	format_warnings_locations.push_back( location );
}

bool FormatStringHandler::want_file( const FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < format_warnings_locations.size(); i++ )
	{
		if( format_warnings_locations[i].file == file.name ) {
			return true;
		}
	}

	return false;
}

void FormatStringHandler::fix_file( FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < format_warnings_locations.size(); i++ )
	{
		if( format_warnings_locations[i].file == file.name ) {
			fix_warning( format_warnings_locations[i], file.content );
		}
	}
}

void FormatStringHandler::report_unfixed_compile_logs()
{
	for( unsigned i = 0; i < format_warnings_locations.size(); i++ )
	{
		if( !format_warnings_locations[i].fixed )
		{
			std::cout << "(unfixed) " << format_warnings_locations[i].compile_log_line << '\n';
		}
	}
}

void FormatStringHandler::fix_warning( FormatWarnigs & warning, std::string & content )
{
	std::string::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::string::npos ) {
		throw REPORT_EXCEPTION( format( "can't get file position for warning %s", warning.compile_log_line));
	}

	std::string::size_type function_end = content.find( ");", pos );

	if( function_end == std::string::npos )
		function_end = content.find( ")", pos );

	std::string::size_type function_start = pos;

	int count=1;
	bool in_string = false;
	bool in_string_started = false;

	for( function_start = pos - 1; function_start > 0; function_start-- )
	{
		if( content[function_start] == ')' && !in_string )
		{
			count++;
		}
		else if( content[function_start] == '('  && !in_string )
		{
			count--;
		}
		else if ( content[function_start] == '"' )
		{
			in_string = !in_string;

			if( in_string ) {
				in_string_started = true;
			}
			continue;
		} else if( content[function_start] == '\\' && in_string && in_string_started ) {
			in_string = false;
		}

		in_string_started = false;

		if( count == 0 )
			break;
	}


	if( function_start == std::string::npos )
		return;

	if( function_end == std::string::npos )
			return;

	// bis zum ersten space suchen
	bool var_started = false;

	int p = 0;

	for( p = static_cast<int>(function_start) - 1; p > 0; p-- )
	{
		if( content[p] == ' ' ||
		    content[p] == '\t' )
		{
			if( var_started )
				break;

			continue;
		}

		if( var_started )
		{
			if( content[p] == ';' ||
				content[p] == ',' ||
				content[p] == '=' )
			{
				p++;
				break;
			}
		}

		if( content[p] == '_' || isalnum(content[p]) ) {
			var_started = true;
			continue;
		}
	}

	if( p < 0 )
		return;

	Function func;


	if( !get_function( content, p, function_start, function_end, &func, false ) )
	{
		DEBUG( "get_function failed");
		return;
	}

	DEBUG( format("%s:%d %s", warning.file, warning.line, func.name ) );


	std::string func_name = strip( func.name );

	if( func_name.empty() )
		return;

}
