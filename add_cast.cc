/*
 * add_cast.cc
 *
 *  Created on: 21.04.2021
 *      Author: hofer
 */
#include "add_cast.h"
#include <format.h>
#include "utils.h"
#include "find_decl.h"
#include "CpputilsDebug.h"
#include <sstream>
#include <getline.h>
#include "find_decl.h"
#include <string_utils.h>
#include "unused_variable_handler.h"

using namespace Tools;

AddCast::AddCast( const std::wstring & FUNCTION_NAME_ )
	: FUNCTION_NAME( FUNCTION_NAME_ )
{
	keywords.push_back( FUNCTION_NAME );
}

std::wstring AddCast::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );
	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find( FUNCTION_NAME, start_in_file) == std::string::npos ) {
			return res;
		}


		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::string::npos )
			return res;

		CPPDEBUG( wformat( L"%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::wstring::size_type start, end;
       int extra = 0;


          {
            // Zur�cksuchen, bis zum Ende des Bezeichners,
            std::wstring::size_type pos2 = pos;

            while( pos2 > 0 && ( isalnum( res[pos2] ) || res[pos2] == '_' || res[pos2] == '$' ) &&
                   !isspace(res[pos2]) &&
                   res[pos2] != L'=' )
              {
                pos2--;
                extra++;
              }
          }


          std::wstring var = get_assignment_var( res, pos-(extra) );
		if (var.empty()) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		std::wstring::size_type decl_pos = 0;
		std::wstring decl;

		find_decl( res, pos, var, decl, decl_pos );

		if( decl_pos == std::wstring::npos || decl_pos == 0 ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		CPPDEBUG( wformat( L"'%s' decl: '%s' line: %d", var, decl, get_linenum(res,decl_pos)) );

		std::wstring line = get_whole_line( res, decl_pos );
		size_t zStartDecl = line.find_first_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		line = line.substr(zStartDecl, line.size()-zStartDecl);
		size_t zEndDecl = line.find_first_not_of(L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_");
		line = line.substr(0, zEndDecl);

		if (line.find (L"void") != std::wstring::npos) {
			CPPDEBUG( "Do not cast to void");
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		
		std::wstring sCast = L"(" + line + L"*)";
		std::wstring sTypeId = line;

		CPPDEBUG( wformat( L"CAST:  '%s' var: '%s' decl: '%s' line: %d", sCast, var, decl, get_linenum(res,decl_pos)) );

		if( !get_function(res,pos,start,end,&func, false) ) {
			CPPDEBUG(wformat(L"unable to load %s function", FUNCTION_NAME) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.name != FUNCTION_NAME )
		{
			CPPDEBUG( wformat( L"function name is '%s'", func.name) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		
		line = get_whole_line(res, pos);

		if (line.find(sTypeId) != std::wstring::npos) {
			start_in_file = pos+FUNCTION_NAME.size();
			continue;
		}
		if (line.find(FUNCTION_NAME) == std::wstring::npos) {
			start_in_file = pos+FUNCTION_NAME.size();
			continue;
		}
		CPPDEBUG( format( "found Cast at line: %d", get_linenum(res,pos-1)) );

		std::wstring new_line = substitude( line, FUNCTION_NAME, sCast+FUNCTION_NAME );

		replace_line_from_start_of_line( res, pos, new_line );

		start_in_file = pos + FUNCTION_NAME.size() + sCast.size();

	} while( pos != std::wstring::npos && start_in_file < res.size() );


	return res;
}

bool AddCast::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

void AddCast::replace_line_from_start_of_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line )
{
	if( pos == std::string::npos )
		return;

	long ppos = pos;

	for( ; ppos > 0; ppos-- )
		if( buffer[ppos] == '\n' )
		{
			ppos++;
			break;
		}

	UnusedVariableHandler::replace_line( buffer, ppos, new_line );
}
