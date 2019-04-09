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
#include <cpp_util.h>

using namespace Tools;

UnusedVariableHandler::UnusedVariableHandler( bool comment_only_ )
	: unused_variables_locations(),
	  comment_only( comment_only_ )
{

}

void UnusedVariableHandler::read_compile_log_line( const std::string & line )
{
	if( line.find( "warning: unused variable") == std::string::npos )
		return;

	UnusedVarWarnigs location = get_location_from_line( line );

	std::vector<std::string> sl = split_simple( line, " ");

	std::vector<std::string>::reverse_iterator it = sl.rbegin();

	// GCC 4.8 style: warning: unused variable 'dbrv' [-Wunused-variable]

	if( *it->begin() == '[' && *it->rbegin() == ']' ) {
		it++;
	}

	if( it == sl.rend() ) {
		return;
	}

	location.var_name = *it;
	location.var_name = strip( location.var_name, "'‘’");
	location.compile_log_line = line;

	DEBUG( format( "%s %s", location, location.var_name ) );

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
			std::cout << "(unfixed) " << unused_variables_locations[i].compile_log_line << '\n';
		}
	}
}

void UnusedVariableHandler::fix_warning( UnusedVarWarnigs & warning, std::string & content )
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

	// find first space, or comma before the var name
	std::string::size_type start = line.find_last_of( ",", var_start);

	std::string new_line;

	if( start == std::string::npos ) {
		start = line.find_last_of( " ,\t", var_start);

		// we have no comma till the end, remove the complete line
		std::string::size_type next_v = line.find_first_of( ",;", var_start + warning.var_name.size() );

		if( next_v == std::string::npos ) {
			return;
		}

		if( line[next_v] == ';' ) {
			// remove the complete line
			if( comment_only )
			{
				new_line = "// " + line;

			} else {

				// search backwards to a ; or start of the line
				std::string::size_type xstart = line.rfind( ";", var_start );

				if( xstart == std::string::npos ) {
					new_line = line.substr( next_v + 1);
				}
			}

			DEBUG( new_line );

			replace_line( content, pos, new_line );

			warning.fixed = true;

			return;
		} else {
			DEBUG( format("start: %d var_start: %d for '%s'", start, var_start, warning.var_name) );
		}
	}

	std::string::size_type end = line.find_first_of( ",;",  var_start + warning.var_name.size() );

	if( start == std::string::npos )
		return;

	if( end == std::string::npos )
		return;

	std::string left  = line.substr(0, start );

	if( line[start] == ',' && line[end] == ',' ) {
		start += 1;

	} else if( line[end] == ',' ) {

		end += 1;
	}

	std::string right = line.substr( end);
	std::string middle = line.substr( start, end - start );

	if( comment_only )
	{
		new_line = left + " /* " + middle + " */ " + right;

		DEBUG( new_line );
	} else {

		new_line = left;

		if( !left.empty() && *left.rbegin() != ' ' )
			new_line += " ";

		new_line += right;

	}

	replace_line( content, pos, new_line );

	warning.fixed = true;
}

void UnusedVariableHandler::replace_line( std::string & buffer, std::string::size_type pos, const std::string & new_line )
{
	std::string left = buffer.substr( 0, pos );

	std::string::size_type end_of_line = buffer.find('\n', pos );

	std::string right = buffer.substr( end_of_line );

	buffer = left + new_line + right;
}

std::string::size_type UnusedVariableHandler::find_var_name( const std::string line, const std::string & var_name )
{
	 std::string::size_type pos  = 0;

	 do
	 {
		 pos = line.find( var_name, pos );

		 if( pos == std::string::npos )
			 return pos;

		 bool failed = false;

		 if( pos > 0 ) {
			 switch( line[pos-1] )
			 {
			 case ' ': break;
			 case '\t': break;
			 case ',': break;
			 case ';': break;
			 case '*': break;
			 default:
				 DEBUG( format("prevsign: %c Dec: %d", line[pos-1], (int)line[pos-1] ) );
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
			 case ' ': break;
			 case '[': break;
			 case '=': break;
			 case '\t': break;
			 case ',': break;
			 case ';': break;
			 default:
				 DEBUG( format("nextsign: '%c' Dec: %d", line[pos+1], (int)line[pos+1] ) );
				 failed = true;
				 break;
			 }

			 if( failed ) {
				 pos++;
				 continue;
			 }
		 }

		 return pos;

	 } while( pos != std::string::npos );

	 return std::string::npos;
}
