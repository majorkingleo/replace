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
  format_warnings_locations(),
  fix_table()
{
	fix_table.push_back( FixTable( "%d",  "long",               "%ld"));
	fix_table.push_back( FixTable( "%d",  "long int",           "%ld"));
	fix_table.push_back( FixTable( "%ld", "int",                "%d"));
	fix_table.push_back( FixTable( "%d",  "unsigned long",      "%ld"));
	fix_table.push_back( FixTable( "%d",  "unsigned long int",  "%ld"));
	fix_table.push_back( FixTable( "%f",  "double",  			"%lf"));
	fix_table.push_back( FixTable( "%lf", "float",  			"%f"));
	fix_table.push_back( FixTable( "%ld", "unsigned int",  		"%u"));
	fix_table.push_back( FixTable( "%d",  "unsigned int",  		"%u"));
	fix_table.push_back( FixTable( "%d",  "unsigned long",  	"%lu"));
	fix_table.push_back( FixTable( "%ld", "unsigned long",  	"%lu"));
}

void FormatStringHandler::read_compile_log_line( const std::string & line )
{
	if( line.find( "warning: format") == std::string::npos ||
	    line.find( "expects type") == std::string::npos ||
	    line.find( "but argument")  == std::string::npos ||
	    line.find( "has type")  == std::string::npos )
		return;

	FormatWarnigs location = get_location_from_line( line );

	// ./me_kpl.c:291: warning: format ‘%s’ expects type ‘char *’, but argument 3 has type ‘int’

	// extract format
	std::string::size_type start = line.find( "format");
	std::string::size_type end = line.find( "expects", start );

	if( start == std::string::npos ||
		end == std::string::npos ) {
		return;
	}

	location.format = line.substr( start + 6, end - (start + 6) );
	location.format = strip( location.format, "'‘’ " );

	start = line.find( "type", end );
	end = line.find( "but", start );

	if( start == std::string::npos ||
		end == std::string::npos ) {
		return;
	}

	location.expected_type = line.substr( start + 4, end - (start + 4) );
	location.expected_type = strip(location.expected_type, "'‘’, " );


	start = line.find( "argument", end );
	end = line.find( "has", start );

	if( start == std::string::npos ||
			end == std::string::npos ) {
		return;
	}

	location.argnum = s2x<int>(line.substr( start + 8, end - (start + 8) ), -1);
	if( location.argnum < 0 )
		return;


	start = line.find( "type", end );

	if( start == std::string::npos ) {
		return;
	}

	location.target_type = line.substr( start + 4 );
	location.target_type = strip(location.target_type, "'‘’, " );

	/*
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
	*/

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

	if( function_end == std::string::npos )
	{
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	std::string::size_type function_start = function_end;

	int count=1;
	bool in_string = false;
	bool in_string_started = false;

	for( function_start = function_end - 1; function_start > 0; function_start-- )
	{
		// DEBUG( format( "pos: %c %s", content[function_start], get_line( content, function_start) ) );

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


	if( function_start == std::string::npos ) {
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	if( function_end == std::string::npos ) {
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

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

	if( p < 0 ) {
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	Function func;


	if( !get_function( content, p, function_start, function_end, &func, false ) )
	{
		DEBUG( format("%s:%d get_function failed", warning.file, warning.line ));
		return;
	}

	DEBUG( format("%s:%d %s", warning.file, warning.line, func.name ) );


	std::string func_name = strip( func.name );

	if( func_name.empty() )
		return;

	int format_string_pos = 0;
	int format_string_to_change  = 0;
	bool found = false;
	std::string fix_string;

	for( unsigned i = 0; i < func.args.size(); i++ )
	{
		if( func.args[i].find( warning.format ) == std::string::npos  ) {
			continue;
		}

		format_string_pos = i;

		found = false;

		// we found the format string. What we should do now?
		// let's see what the compiler wan'ts from us
		for( unsigned j = 0; j < fix_table.size(); j++ )
		{
			if( fix_table[j].format == warning.format &&
				fix_table[j].target_type == warning.target_type )
			{
				format_string_to_change = warning.argnum - (format_string_pos + 1);
				fix_string = fix_table[j].correct_type;

				DEBUG( format("%s:%d %s => format string at pos: %d pos %d %s", warning.file, warning.line, warning.compile_log_line,
						format_string_pos + 1,
						format_string_to_change,
						fix_table[j].correct_type) );

				found = true;
				break;
			}
		}

		if( !found )
		{
			DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
			return;
		}

		break;
	}


	if( !found ) {
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	std::string format_string_line_orig = func.args[format_string_pos];
	std::string new_format_string_line = func.args[format_string_pos];

	in_string = false;
	bool escape_sign = false;

	std::string::size_type format_string_start_pos = std::string::npos;

	count = 0;

	for( unsigned i = 0; i < new_format_string_line.size(); i++ )
	{
		if( new_format_string_line[i] == '"' && !escape_sign )
		{
			in_string = !in_string;
		} else if( new_format_string_line[i] == '\\' ) {
			escape_sign = !escape_sign;
			continue;
		} else if( new_format_string_line[i] == '%' && in_string ) {

			if( i + 1 < new_format_string_line.size() ) {
				// %%
				if( new_format_string_line[i+1] == '%' ) {
					continue;
				}
			}

			count++;
		}

		if( !in_string )
			continue;


		if( count == format_string_to_change )
		{
			// we wan't to change this format string
			format_string_start_pos = i;
			break;
		}
	}

	if( format_string_start_pos == std::string::npos ) {
		DEBUG( format( "no format_string_start_pos '%s'",  new_format_string_line ) );
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	std::string left = new_format_string_line.substr( 0 , format_string_start_pos );
	std::string right = new_format_string_line.substr( format_string_start_pos + warning.format.size() );

	new_format_string_line = left + fix_string + right;

	DEBUG( format( "%s => %s", format_string_line_orig , new_format_string_line) );

	// new replace the format string.

	std::string::size_type format_string_start = content.find( format_string_line_orig, function_start );

	if( format_string_start == std::string::npos ) {
		DEBUG( format("Cannot handle %s", warning.compile_log_line ) );
		return;
	}


	left = content.substr( 0, format_string_start );
	right = content.substr( format_string_start + format_string_line_orig.size() );


	content = left + new_format_string_line + right;

	warning.fixed = true;
}
