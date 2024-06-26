/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "fix_sprintf.h"
#include <format.h>
#include "utils.h"
#include "string_utils.h"
#include "CpputilsDebug.h"
#include <sstream>

using namespace Tools;

const std::wstring FixSprintf::KEY_WORD = L"sprintf";

FixSprintf::FixSprintf()
{
	keywords.push_back( KEY_WORD );
}

std::wstring FixSprintf::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );

	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find(KEY_WORD, start_in_file) == std::string::npos ) {
			return res;
		}


		pos = res.find( KEY_WORD, start_in_file );

		if( pos == std::wstring::npos )
			return res;

		CPPDEBUG( wformat( L"%s at line %d", KEY_WORD, get_linenum(res,pos) ))

		Function func;
		std::wstring::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			CPPDEBUG("unable to load sprintf function");
			start_in_file = pos + KEY_WORD.size();
			continue;
		}


		if( func.args.size() < 2 ) {
			start_in_file = end;
			continue;
		}




		bool changed_something = false;
		bool create_error_message = false;

		if( func.args.size() == 2 )
		{
			// aus
			//   sprintf(acLabZiel, MlMsg("Ziel"));
			// das machen
			//   StrCpyDestLen(acLabZiel, MlMsg("Ziel"));

			func.name = L"StrCpyDestLen";

			changed_something = true;

			if( func.args.at(1).find( L"%" ) != std::wstring::npos ) {
				create_error_message = true;
			}
		}
		else
		{
			// aus
			//    sprintf( acBuf, "%s %d", xxx,yyy )
			// das machen
			//    StrCpy( acBuf, format( "%s %d", xxx, yyy ) )


			func.name = L"StrCpy";
			func.args[1] = L"format( " + func.args[1];
			*func.args.rbegin() += L")";

			changed_something = true;
		}

		if( changed_something )
		{
			static const std::wstring STRFORM = L"StrForm";

			if( strip( func.args[1] ).find( STRFORM ) != std::wstring::npos )
			{
				std::wstring a = func.args[1].substr( STRFORM.size() + 1 );
				func.args[1] = L"format" + a;
				create_error_message = false;
			}

			std::wstring first_part_of_file = res.substr(0,pos);

			std::wstringstream str;
			std::wstring indent;

			if( create_error_message ) {
				str << L"\n#error \"replace: % deteced in format string. Please check if the behavior is still correct.\"\n";

				std::wstring::size_type pos_line_break = first_part_of_file.rfind('\n');

				if( pos_line_break != std::wstring::npos ) {
					indent = first_part_of_file.substr( pos_line_break );
					first_part_of_file = first_part_of_file.substr( 0, pos_line_break );
				}
			}

			str << indent << func.name << L"(";

			for( unsigned i = 0; i < func.args.size(); i++ )
			{
				if( i > 0 )
					str << L", ";

				str << func.args[i];
			}

			CPPDEBUG( str.str() );

			std::wstring second_part_of_file = res.substr(end);

			res = first_part_of_file + str.str() + second_part_of_file;
		}

		start_in_file = end;

	} while( pos != std::wstring::npos && start_in_file < res.size() );

	return res;
}

bool FixSprintf::want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file )
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
