/*
 * remove_generic_cast.cc
 *
 *  Created on: 21.06.2013
 *      Author: martin
 */
#include "fix_conversion_null.h"
#include <format.h>
#include "utils.h"
#include "debug.h"
#include <sstream>
#include <getline.h>
#include <string_utils.h>
#include "unused_variable_handler.h"

using namespace Tools;

FixConversionNull::FixConversionNull( const std::string & FUNCTION_NAME_,
					  const unsigned LONG_ARG_NUM_ )
	: FUNCTION_NAME( FUNCTION_NAME_ ),
	  LONG_ARG_NUM( LONG_ARG_NUM_ )
{
	keywords.push_back( FUNCTION_NAME );
}

std::string FixConversionNull::patch_file( const std::string & file )
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

		if( func.args.size() < LONG_ARG_NUM ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		std::string var_name = strip( func.args[LONG_ARG_NUM-1] );

		DEBUG( format( "Argument Value '%s' for ArrCreate/ArrSort at Line: %d", var_name, get_linenum(res,pos)) );

		if (var_name.compare("NULL") != 0) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		DEBUG( format( "found invalid use of ArrCreate at line: %d", get_linenum(res,pos-1)) );

        std::string first_part_of_file = res.substr(0,pos);

        std::stringstream str;

        str << func.name << "(";

        for( unsigned i = 0; i < func.args.size(); i++ )
        {
            if( i > 0 )
                str << ", ";

			if (i < (func.args.size()-1)) {
				str << func.args[i];
			} else {
				str << "0";
			}
        }

        DEBUG( str.str() );

        std::string second_part_of_file = res.substr(end);

        res = first_part_of_file + str.str() + second_part_of_file;

        start_in_file = end;

	} while( pos != std::string::npos && start_in_file < res.size() );


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

void FixConversionNull::replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line )
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
