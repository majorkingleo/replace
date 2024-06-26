/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "fix_StrForm.h"
#include <format.h>
#include "utils.h"
#include "CpputilsDebug.h"
#include <sstream>

using namespace Tools;

const std::wstring FixStrForm::KEY_WORD = L"StrForm";

FixStrForm::FixStrForm()
{
	keywords.push_back( KEY_WORD );
}

std::wstring FixStrForm::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );

	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find(KEY_WORD, start_in_file) == std::wstring::npos ) {
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


		// aus
		//    StrForm( acBuf, "%s %d", xxx,yyy )
		// das machen
		//    TO_CHAR(format( acBuf, format( "%s %d", xxx, yyy ) ))


		func.name = L"TO_CHAR(format";
		*func.args.rbegin() += L")";

		std::wstring first_part_of_file = res.substr(0,pos);

		std::wstringstream str;

		str << func.name << L"(";

		for( unsigned i = 0; i < func.args.size(); i++ )
		{
			if( i > 0 )
				str << L", ";

			str << func.args[i];
		}

		CPPDEBUG( str.str() );

		std::wstring second_part_of_file = res.substr(end);

		res = first_part_of_file + str.str() + second_part_of_file;

		start_in_file = end;

	} while( pos != std::wstring::npos && start_in_file < res.size() );

	return res;
}

bool FixStrForm::want_file_ext( const FILE_TYPE & file_type, bool is_cpp_file )
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
