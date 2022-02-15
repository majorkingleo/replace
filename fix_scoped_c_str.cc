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

const std::wstring FixScopedCStr::KEY_WORD = L"scoped_cstr::form";

FixScopedCStr::FixScopedCStr()
{
	keywords.push_back( KEY_WORD );

	casts.push_back( L"(const char*)" );
	casts.push_back( L"(char const*)" );
}

std::wstring FixScopedCStr::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );

	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do
	{
		// file already patched ?
		pos = res.find( KEY_WORD, start_in_file );

		if( pos == std::wstring::npos )
			return res;

		DEBUG( wformat( L"%s at line %d", KEY_WORD, get_linenum(res,pos) ))

		Function func;
		std::wstring::size_type start, end;

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


		func.name = L"wamas::platform::string::form";
		// *func.args.rbegin() += ".c_str()";

		std::wstring first_part_of_file = res.substr(0,pos);

		// den (const char*) cast entfernen
		std::wstring::size_type spaces_pos = skip_spaces( res, pos, true );

		std::wstring line = get_whole_line( res, pos );

		bool found_std_string = false;

		if( line.find( L"std::string" ) != std::wstring::npos ) {
			found_std_string = true;
		}

		bool found_cast = false;

		for( const std::wstring & CAST : casts ) {

			std::wstring::size_type cast_pos = res.rfind( CAST, spaces_pos );

			// DEBUG( format( "spaces_pos: %d cast_pos: %d diff: %d", spaces_pos, cast_pos, spaces_pos - CAST.size() ));

			if( cast_pos == (spaces_pos - CAST.size()) ) {
				first_part_of_file = res.substr(0,cast_pos);
				found_cast = true;
				break;
			}
		}

		std::wstringstream str;

		str << func.name << L"(";

		for( unsigned i = 0; i < func.args.size(); i++ )
		{
			if( i > 0 )
				str << L", ";

			str << func.args[i];
		}

		DEBUG( str.str() );

		if( !found_std_string ) {
			end += 1; // ) weglassen
		}

		std::wstring second_part_of_file = res.substr(end);

		if( !found_std_string ) {
			second_part_of_file = L").c_str()" + second_part_of_file;
		}

		res = first_part_of_file + str.str() + second_part_of_file;

		start_in_file = end;

	} while( pos != std::wstring::npos && start_in_file < res.size() );

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
