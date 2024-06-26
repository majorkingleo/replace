/*
 * remove_generic_cast.cc
 *
 *  Created on: 21.06.2013
 *      Author: martin
 */
#include "fix_conversion_null.h"
#include <format.h>
#include "utils.h"
#include "CpputilsDebug.h"
#include <sstream>
#include <getline.h>
#include <string_utils.h>
#include "unused_variable_handler.h"

using namespace Tools;

FixConversionNull::FixConversionNull( const std::wstring & FUNCTION_NAME_,
					  const unsigned LONG_ARG_NUM_ )
	: FUNCTION_NAME( FUNCTION_NAME_ ),
	  LONG_ARG_NUM( LONG_ARG_NUM_ )
{
	keywords.push_back( FUNCTION_NAME );
}

std::wstring FixConversionNull::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );
	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find( FUNCTION_NAME, start_in_file) == std::wstring::npos ) {
			return res;
		}


		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::wstring::npos )
			return res;

		CPPDEBUG( wformat( L"%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::wstring::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			CPPDEBUG(wformat(L"unable to load %s function", FUNCTION_NAME) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.name != FUNCTION_NAME )
		{
			CPPDEBUG( wformat(L"function name is '%s'", func.name) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.args.size() < LONG_ARG_NUM ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		std::wstring var_name = strip( func.args[LONG_ARG_NUM-1] );

		CPPDEBUG( wformat( L"Argument Value '%s' for ArrCreate/ArrSort at Line: %d", var_name, get_linenum(res,pos)) );

		if (var_name.compare(L"NULL") != 0) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		CPPDEBUG( wformat( L"found invalid use of ArrCreate at line: %d", get_linenum(res,pos-1)) );

        std::wstring first_part_of_file = res.substr(0,pos);

        std::wstringstream str;

        str << func.name << L"(";

        for( unsigned i = 0; i < func.args.size(); i++ )
        {
            if( i > 0 )
                str << L", ";

			if (i < (func.args.size()-1)) {
				str << func.args[i];
			} else {
				str << L"0";
			}
        }

        CPPDEBUG( str.str() );

        std::wstring second_part_of_file = res.substr(end);

        res = first_part_of_file + str.str() + second_part_of_file;

        start_in_file = end;

	} while( pos != std::wstring::npos && start_in_file < res.size() );


	return res;
}

bool FixConversionNull::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

void FixConversionNull::replace_line_from_start_of_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line )
{
	if( pos == std::wstring::npos )
		return;

	long ppos = pos;

	for( ; ppos > 0; ppos-- )
		if( buffer[ppos] == L'\n' )
		{
			ppos++;
			break;
		}

	UnusedVariableHandler::replace_line( buffer, ppos, new_line );
}
