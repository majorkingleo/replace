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
#include "debug.h"
#include <sstream>
#include <getline.h>
#include "find_decl.h"
#include <string_utils.h>
#include "unused_variable_handler.h"

using namespace Tools;

AddCast::AddCast( const std::string & FUNCTION_NAME_ )
	: FUNCTION_NAME( FUNCTION_NAME_ )
{
	keywords.push_back( FUNCTION_NAME );
}

std::string AddCast::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	std::string res( file );
	std::string::size_type start_in_file = 0;
	std::string::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find( FUNCTION_NAME, start_in_file) == std::string::npos ) {
			return res;
		}


		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;
       int extra = 0;


          {
            // Zurücksuchen, bis zum Ende des Bezeichners,
            std::string::size_type pos2 = pos;

            while( pos2 > 0 && ( isalnum( res[pos2] ) || res[pos2] == '_' || res[pos2] == '$' ) &&
                   !isspace(res[pos2]) &&
                   res[pos2] != '=' )
              {
                pos2--;
                extra++;
              }
          }


          std::string var = get_assignment_var( res, pos-(extra) );
		if (var.empty()) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		std::string::size_type decl_pos = 0;
		std::string decl;

		find_decl( res, pos, var, decl, decl_pos );

		if( decl_pos == std::string::npos || decl_pos == 0 ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		DEBUG( format( "'%s' decl: '%s' line: %d", var, decl, get_linenum(res,decl_pos)) );

		std::string line = get_whole_line( res, decl_pos );
		size_t zStartDecl = line.find_first_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
		line = line.substr(zStartDecl, line.size()-zStartDecl);
		size_t zEndDecl = line.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_");
		line = line.substr(0, zEndDecl);

		if (line.find ("void") != std::string::npos) {
			DEBUG( "Do not cast to void");
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		
		std::string sCast = "(" + line + "*)";
		std::string sTypeId = line;

		DEBUG( format( "CAST:  '%s' var: '%s' decl: '%s' line: %d", sCast, var, decl, get_linenum(res,decl_pos)) );

		if( !get_function(res,pos,start,end,&func, false) ) {
			DEBUG(format("unable to load %s function", FUNCTION_NAME) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.name != FUNCTION_NAME )
		{
			DEBUG( format("function name is '%s'", func.name) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}
		
		line = get_whole_line(res, pos);

		if (line.find(sTypeId) != std::string::npos) {
			start_in_file = pos+FUNCTION_NAME.size();
			continue;
		}
		if (line.find(FUNCTION_NAME) == std::string::npos) {
			start_in_file = pos+FUNCTION_NAME.size();
			continue;
		}
		DEBUG( format( "found Cast at line: %d", get_linenum(res,pos-1)) );

		std::string new_line = substitude( line, FUNCTION_NAME, sCast+FUNCTION_NAME );

		replace_line_from_start_of_line( res, pos, new_line );

		start_in_file = pos + FUNCTION_NAME.size() + sCast.size();

	} while( pos != std::string::npos && start_in_file < res.size() );


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

void AddCast::replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line )
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
