/*
 * unused_variable_handler.cc
 *
 *  Created on: 13.02.2014
 *      Author: martin
 */

#include "unused_variable_handler.h"
#include <string_utils.h>
#include <debug.h>
#include "getline.h"
#include "stderr_exception.h"
#include "DetectLocale.h"

using namespace Tools;

UnusedVariableHandler::UnusedVariableHandler( bool comment_only_ )
	: unused_variables_locations(),
	  comment_only( comment_only_ )
{

}

void UnusedVariableHandler::read_compile_log_line( const std::wstring & line )
{
	if( line.find( L"warning: unused variable") == std::wstring::npos )
		return;

	UnusedVarWarnigs location = get_location_from_line( line );

	std::vector<std::wstring> sl = split_simple( line, L" ");

	std::vector<std::wstring>::reverse_iterator it = sl.rbegin();

	// GCC 4.8 style: warning: unused variable 'dbrv' [-Wunused-variable]

	if( *it->begin() == L'[' && *it->rbegin() == L']' ) {
		it++;
	}

	if( it == sl.rend() ) {
		return;
	}

	location.var_name = *it;
	location.var_name = strip( location.var_name, L"'‘’");
	location.compile_log_line = line;

	DEBUG( wformat( L"%s %s", location, location.var_name ) );

	unused_variables_locations.push_back( location );
}

bool UnusedVariableHandler::want_file( const FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < unused_variables_locations.size(); i++ )
	{
		if( unused_variables_locations[i].file == file.name ) {
			return true;
		}
	}

	return false;
}

void UnusedVariableHandler::fix_file( FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < unused_variables_locations.size(); i++ )
	{
		if( unused_variables_locations[i].file == file.name ) {
			fix_warning( unused_variables_locations[i], file.content );
		}
	}
}

void UnusedVariableHandler::report_unfixed_compile_logs()
{
	for( unsigned i = 0; i < unused_variables_locations.size(); i++ )
	{
		if( !unused_variables_locations[i].fixed )
		{
			std::cout << "(unfixed) " << DetectLocale::w2out(unused_variables_locations[i].compile_log_line) << '\n';
		}
	}
}

void UnusedVariableHandler::fix_warning( UnusedVarWarnigs & warning, std::wstring & content )
{
	std::wstring::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::wstring::npos ) {
		throw STDERR_EXCEPTION( format( "can't get file position for warning %s", DetectLocale::w2out(warning.compile_log_line)));
	}

	std::wstring line = getline(content,  pos );

	DEBUG( line );


	// 1st find starting pos

	std::wstring::size_type var_start = find_var_name( line, warning.var_name );

	DEBUG( wformat( L"%s var_start: %d", warning.var_name, var_start) );

	if( var_start == std::wstring::npos )
		return;

	// find first space, or comma before the var name
	std::wstring::size_type start = line.find_last_of( L",", var_start);

	std::wstring new_line;

	if( start == std::wstring::npos ) {
		start = line.find_last_of( L" ,\t", var_start);

		// we have no comma till the end, remove the complete line
		std::wstring::size_type next_v = line.find_first_of( L",;", var_start + warning.var_name.size() );

		if( next_v == std::wstring::npos ) {
			return;
		}

		if( line[next_v] == ';' ) {
			// remove the complete line
			if( comment_only )
			{
				new_line = L"// " + line;

			} else {

				// search backwards to a ; or start of the line
				std::wstring::size_type xstart = line.rfind( L";", var_start );

				if( xstart == std::wstring::npos ) {
					new_line = line.substr( next_v + 1);
				}
			}

			DEBUG( new_line );

			replace_line( content, pos, new_line );

			warning.fixed = true;

			return;
		} else {
			DEBUG( wformat( L"start: %d var_start: %d for '%s'", start, var_start, warning.var_name) );
		}
	}

	std::wstring::size_type end = line.find_first_of( L",;",  var_start + warning.var_name.size() );

	if( start == std::wstring::npos )
		return;

	if( end == std::wstring::npos )
		return;

	std::wstring left  = line.substr(0, start );

	if( line[start] == L',' && line[end] == L',' ) {
		start += 1;

	} else if( line[end] == L',' ) {

		end += 1;
	}

	std::wstring right = line.substr( end);
	std::wstring middle = line.substr( start, end - start );

	if( comment_only )
	{
		new_line = left + L" /* " + middle + L" */ " + right;

		DEBUG( new_line );
	} else {

		new_line = left;

		if( !left.empty() && *left.rbegin() != L' ' )
			new_line += L" ";

		new_line += right;

	}

	replace_line( content, pos, new_line );

	warning.fixed = true;
}

void UnusedVariableHandler::replace_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line )
{
	std::wstring left = buffer.substr( 0, pos );

	std::wstring::size_type end_of_line = buffer.find(L'\n', pos );

	std::wstring right = buffer.substr( end_of_line );

	buffer = left + new_line + right;
}

std::wstring::size_type UnusedVariableHandler::find_var_name( const std::wstring line, const std::wstring & var_name )
{
	 std::wstring::size_type pos  = 0;

	 do
	 {
		 pos = line.find( var_name, pos );

		 if( pos == std::wstring::npos )
			 return pos;

		 bool failed = false;

		 if( pos > 0 ) {
			 switch( line[pos-1] )
			 {
			 case L' ': break;
			 case L'\t': break;
			 case L',': break;
			 case L';': break;
			 case L'*': break;
			 default:
				 DEBUG( wformat( L"prevsign: %c Dec: %d", line[pos-1], (int)line[pos-1] ) );
				 failed = true;
				 break;
			 }

			 if( failed ) {
				 pos++;
				 continue;
			 }
		 }

		 if( pos + var_name.size() < line.size()-1 ) {
			 switch( line[pos+var_name.size()] )
			 {
			 case L' ': break;
			 case L'[': break;
			 case L'=': break;
			 case L'\t': break;
			 case L',': break;
			 case L';': break;
			 default:
				 DEBUG( wformat( L"nextsign: '%c' Dec: %d", line[pos+1], (int)line[pos+1] ) );
				 failed = true;
				 break;
			 }

			 if( failed ) {
				 pos++;
				 continue;
			 }
		 }

		 return pos;

	 } while( pos != std::wstring::npos );

	 return std::wstring::npos;
}
