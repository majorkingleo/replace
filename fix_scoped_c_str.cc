/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "fix_scoped_c_str.h"
#include <format.h>
#include "utils.h"
#include "debug.h"
#include <sstream>

using namespace Tools;

const std::string FixScopedCStr::KEY_WORD = "scoped_cstr::form";

FixScopedCStr::FixScopedCStr()
{
	keywords.push_back( KEY_WORD );

	casts.push_back( "(const char*)" );
	casts.push_back( "(char const*)" );
}

std::string FixScopedCStr::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	std::string res( file );

	std::string::size_type start_in_file = 0;
	std::string::size_type pos = 0;

	do
	{
		// file already patched ?
		pos = res.find( KEY_WORD, start_in_file );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "%s at line %d", KEY_WORD, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			DEBUG("unable to load scoped_cstr::form function");
			start_in_file = pos + KEY_WORD.size();
			continue;
		}


		if( func.args.size() < 2 ) {
			start_in_file = end;
			continue;
		}


		// aus
		//    (const char*)scoped_cstr::form( acBuf, "%s %d", xxx,yyy )
		// das machen
		//    wamas::platform:string::form( acBuf, "%s %d", xxx, yyy ).c_str()


		func.name = "wamas::platform::string::form";
		// *func.args.rbegin() += ".c_str()";

		std::string first_part_of_file = res.substr(0,pos);

		// den (const char*) cast entfernen
		std::string::size_type spaces_pos = skip_spaces( res, pos, true );

		std::string line = get_whole_line( res, pos );

		bool found_std_string = false;

		if( line.find( "std::string" ) != std::string::npos ) {
			found_std_string = true;
		}

		bool found_cast = false;

		for( const std::string & CAST : casts ) {

			std::string::size_type cast_pos = res.rfind( CAST, spaces_pos );

			// DEBUG( format( "spaces_pos: %d cast_pos: %d diff: %d", spaces_pos, cast_pos, spaces_pos - CAST.size() ));

			if( cast_pos == (spaces_pos - CAST.size()) ) {
				first_part_of_file = res.substr(0,cast_pos);
				found_cast = true;
				break;
			}
		}

		std::stringstream str;

		str << func.name << "(";

		for( unsigned i = 0; i < func.args.size(); i++ )
		{
			if( i > 0 )
				str << ", ";

			str << func.args[i];
		}

		DEBUG( str.str() );

		if( !found_std_string ) {
			end += 1; // ) weglassen
		}

		std::string second_part_of_file = res.substr(end);

		if( !found_std_string ) {
			second_part_of_file = ").c_str()" + second_part_of_file;
		}

		res = first_part_of_file + str.str() + second_part_of_file;

		start_in_file = end;

	} while( pos != std::string::npos && start_in_file < res.size() );

	return res;
}

bool FixScopedCStr::want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  if( is_cpp_file ) {
			  return true;
		  }
		  return false;

	  default:
		  return false;
	}
}
