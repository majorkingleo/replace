/*
 * format_string_handler.cc
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */

#include "format_string_handler.h"
#include "stderr_exception.h"
#include "debug.h"
#include "getline.h"
#include "utils.h"
#include "string_utils.h"
#include "HandleFile.h"

using namespace Tools;

FormatStringHandler::FormatStringHandler()
: FixFromCompileLog::Handler(),
  format_warnings_locations(),
  fix_table()
{
	fix_table.push_back( FixTable( L"%d",  L"long",               L"%ld"));
	fix_table.push_back( FixTable( L"%d",  L"long int",           L"%ld"));
	fix_table.push_back( FixTable( L"%ld", L"int",                L"%d"));
	fix_table.push_back( FixTable( L"%d",  L"unsigned long",      L"%ld"));
	fix_table.push_back( FixTable( L"%d",  L"unsigned long int",  L"%ld"));
	fix_table.push_back( FixTable( L"%f",  L"double",  			  L"%lf"));
	fix_table.push_back( FixTable( L"%lf", L"float",  			  L"%f"));
	fix_table.push_back( FixTable( L"%ld", L"unsigned int",  	  L"%u"));
	fix_table.push_back( FixTable( L"%d",  L"unsigned int",  	  L"%u"));
	fix_table.push_back( FixTable( L"%d",  L"unsigned long",  	  L"%lu"));
	fix_table.push_back( FixTable( L"%ld", L"unsigned long",  	  L"%lu"));
}

void FormatStringHandler::read_compile_log_line( const std::wstring & line )
{
	if( !is_interested_in_line( line ) )
	{
		return;
	}

	DEBUG( wformat( L"reading line: %s", line ))

	FormatWarnigs location = get_location_from_line( line );

	// ./me_kpl.c:291: warning: format ‘%s’ expects type ‘char *’, but argument 3 has type ‘int’

	// extract format
	std::wstring::size_type start = line.find( L"format");
	std::wstring::size_type end = line.find( L"expects", start );

	if( start == std::wstring::npos ||
		end == std::wstring::npos ) {
		return;
	}

	location.format = line.substr( start + 6, end - (start + 6) );
	location.format = strip( location.format, L"'‘’ " );

	start = line.find( L"type", end );
	end = line.find( L"but", start );

	if( start == std::wstring::npos ||
		end == std::wstring::npos ) {
		return;
	}

	location.expected_type = line.substr( start + 4, end - (start + 4) );
	location.expected_type = strip(location.expected_type, L"'‘’, " );

	DEBUG( wformat( L"expected type: '%s'", location.expected_type ) );

	start = line.find( L"argument", end );
	end = line.find( L"has", start );

	if( start == std::wstring::npos ||
		end == std::wstring::npos ) {
		DEBUG( "start or end not found" );
		return;
	}

	location.argnum = s2x<int>(line.substr( start + 8, end - (start + 8) ), -1);
	if( location.argnum < 0 ) {
		DEBUG( format( "location arg number is %d", location.argnum ));
		return;
	}


	start = line.find( L"type", end );

	if( start == std::string::npos ) {
		DEBUG( "type not found" );
		return;
	}

	location.target_type = line.substr( start + 4 );
	location.target_type = strip(location.target_type, L"'‘’, " );

	DEBUG( wformat( L"targe type: '%s'", location.target_type) );

	strip_target_type( location );

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

	DEBUG( wformat( L"%s format string: '%s' expected type: '%d' target_type: '%d' argument: %d",
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
			std::cout << "(unfixed) " << HandleFile::w2out(format_warnings_locations[i].compile_log_line) << '\n';
		}
	}
}

void FormatStringHandler::fix_warning( FormatWarnigs & warning, std::wstring & content )
{

	DEBUG( format("Working now on '%s'", HandleFile::w2out(warning.compile_log_line)) );

	std::wstring::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::string::npos ) {
		throw STDERR_EXCEPTION( format( "can't get file position for warning %s", HandleFile::w2out(warning.compile_log_line)));
	}

	std::wstring::size_type function_end = content.find( L")", pos );

	/*
	if( function_end == std::wstring::npos )
		function_end = content.find( L")", pos );
	*/
	if( function_end == std::string::npos )
	{
		DEBUG( format("Cannot handle %s", HandleFile::w2out(warning.compile_log_line), __LINE__ ) );
		return;
	}

	std::wstring::size_type function_start = function_end;

	int count=1;
	bool in_string = false;
	bool in_string_started = false;

	for( function_start = function_end - 1; function_start > 0; function_start-- )
	{
		// DEBUG( format( "pos: %c %s", content[function_start], get_line( content, function_start) ) );

		if( content[function_start] == L')' && !in_string )
		{
			count++;
		}
		else if( content[function_start] == L'('  && !in_string )
		{
			count--;
		}
		else if ( content[function_start] == L'"' )
		{
			in_string = !in_string;

			if( in_string ) {
				in_string_started = true;
			}
			continue;
		} else if( content[function_start] == L'\\' && in_string && in_string_started ) {
			in_string = false;
		}

		in_string_started = false;

		if( count == 0 )
			break;
	}


	if( function_start == std::wstring::npos ) {
		DEBUG( wformat(L"Cannot handle %s (%d)", warning.compile_log_line, __LINE__ ) );
		return;
	}

	if( function_end == std::string::npos ) {
		DEBUG( wformat(L"Cannot handle %s (%d)", warning.compile_log_line, __LINE__ ) );
		return;
	}

	// bis zum ersten space suchen
	bool var_started = false;

	int p = 0;

	for( p = static_cast<int>(function_start) - 1; p > 0; p-- )
	{
		if( content[p] == L' ' ||
		    content[p] == L'\t' )
		{
			if( var_started )
				break;

			continue;
		}

		if( var_started )
		{
			if( content[p] == L';' ||
				content[p] == L',' ||
				content[p] == L'=' )
			{
				p++;
				break;
			}
		}

		if( content[p] == L'_' || isalnum(content[p]) ) {
			var_started = true;
			continue;
		}
	}

	if( p <= 0 ) {
		DEBUG( wformat(L"Cannot handle %s (%d)", warning.compile_log_line, __LINE__ ) );
		return;
	}

	Function func;


	if( !get_function( content, p, function_start, function_end, &func, false ) )
	{
		DEBUG( wformat(L"%s:%d get_function failed", warning.file, warning.line ));
		return;
	}

	DEBUG( wformat(L"%s:%d %s", warning.file, warning.line, func.name ) );


	std::wstring func_name = strip( func.name );

	if( func_name.empty() ) {
		DEBUG( "Function name is empty" );
		return;
	}

	int format_string_pos = 0;
	int format_string_to_change  = 0;
	bool found = false;
	std::wstring fix_string;

	for( unsigned i = 0; i < func.args.size(); i++ )
	{
		if( func.args[i].find( warning.format ) == std::wstring::npos  ) {
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
				DEBUG( wformat( L"'%s' == '%s' && '%s' == '%s'",
									   fix_table[j].format, warning.format,
									   fix_table[j].target_type, warning.target_type ));

				format_string_to_change = warning.argnum - (format_string_pos + 1);
				fix_string = fix_table[j].correct_type;

				DEBUG( wformat(L"%s:%d %s => format string at pos: %d pos %d %s", warning.file, warning.line, warning.compile_log_line,
						format_string_pos + 1,
						format_string_to_change,
						fix_table[j].correct_type) );

				found = true;
				break;
			} else {
				DEBUG( wformat( L"'%s' != '%s' && '%s' != '%s'",
					   fix_table[j].format, warning.format,
					   fix_table[j].target_type, warning.target_type ));
			}
		}

		if( !found )
		{
			DEBUG( wformat(L"Cannot handle %s", warning.compile_log_line ) );
			return;
		}

		break;
	}


	if( !found ) {
		DEBUG( wformat(L"Cannot handle %s", warning.compile_log_line ) );
		return;
	}

	std::wstring format_string_line_orig = func.args[format_string_pos];
	std::wstring new_format_string_line = func.args[format_string_pos];

	in_string = false;
	bool escape_sign = false;

	std::string::size_type format_string_start_pos = std::string::npos;

	count = 0;

	for( unsigned i = 0; i < new_format_string_line.size(); i++ )
	{
		if( new_format_string_line[i] == L'"' && !escape_sign )
		{
			in_string = !in_string;
		} else if( new_format_string_line[i] == L'\\' ) {
			escape_sign = !escape_sign;
			continue;
		} else if( new_format_string_line[i] == L'%' && in_string ) {

			if( i + 1 < new_format_string_line.size() ) {
				// %%
				if( new_format_string_line[i+1] == L'%' ) {
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
		DEBUG( wformat( L"no format_string_start_pos '%s'",  new_format_string_line ) );
		DEBUG( wformat(L"Cannot handle %s (%d)", warning.compile_log_line, __LINE__ ) );
		return;
	}

	std::wstring left = new_format_string_line.substr( 0 , format_string_start_pos );
	std::wstring right = new_format_string_line.substr( format_string_start_pos + warning.format.size() );

	new_format_string_line = left + fix_string + right;

	DEBUG( wformat( L"%s => %s", format_string_line_orig , new_format_string_line) );

	// new replace the format string.

	std::string::size_type format_string_start = content.find( format_string_line_orig, function_start );

	if( format_string_start == std::string::npos ) {
		DEBUG( wformat(L"Cannot handle %s (%d)", warning.compile_log_line, __LINE__ ) );
		return;
	}


	left = content.substr( 0, format_string_start );
	right = content.substr( format_string_start + format_string_line_orig.size() );


	content = left + new_format_string_line + right;

	warning.fixed = true;
}


bool FormatStringHandler::is_interested_in_line( const std::wstring & line )
{
	if( line.find( L"warning: format") == std::wstring::npos ||
	    line.find( L"expects type") == std::wstring::npos ||
	    line.find( L"but argument")  == std::wstring::npos ||
	    line.find( L"has type")  == std::wstring::npos )
	{
		return false;
	}

	return true;
}
