/*
 * remove_generic_cast.cc
 *
 *  Created on: 21.06.2013
 *      Author: martin
 */
#include "fix_prmget.h"
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

FixPrmGet::FixPrmGet( const std::string & FUNCTION_NAME_,
					  const unsigned LONG_ARG_NUM_ )
	: FUNCTION_NAME( FUNCTION_NAME_ ),
	  LONG_ARG_NUM( LONG_ARG_NUM_ )
{
	keywords.push_back( FUNCTION_NAME );
}



std::string FixPrmGet::patch_file( const std::string & file )
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

		if( pos == std::string::npos ) {
			return res;
		}

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
		var_name = strip_leading( var_name, "&" );

		std::string::size_type decl_pos = 0;
		std::string decl;

		find_decl( res, pos, var_name, decl, decl_pos );

		if( decl_pos == std::string::npos || decl_pos == 0 ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		DEBUG( format( "'%s' decl: '%s' line: %d pos: %d char: 0x%X (%c)",
						var_name, decl, get_linenum(res,decl_pos), decl_pos, (int)res[decl_pos], res[decl_pos] ));

		std::string line = get_whole_line( res, decl_pos );

		DEBUG( format( "line with decl: %s", line ) );

		if( line.find( "int" ) == std::string::npos ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		DEBUG( format( "found invalid use of %s at line: %d", FUNCTION_NAME, get_linenum(res,pos-1)) );

		// wenn es sich um die Funktionsdefinition handelt, dann vorsichtig

		bool already_replaced = false;
		std::string new_line;

		std::string::size_type pfunc = line.find( "(");
		if( pfunc != std::string::npos ) {
			DEBUG( "detected function" );

			std::string::size_type pp = skip_spaces( line, pfunc, true );

			if( pp != std::string::npos ) {
				pp = rfind_first_of(line, " \t\n;,", pp );

				if( pp != std::string::npos ) {

					std::string::size_type start;
					std::string::size_type end;
					Function func;

					if( get_function( line, pp, start, end, &func, false ) ) {
						for( std::string & arg : func.args ) {
							if( arg.find( var_name ) ) {
								arg = substitude( arg, "int", "long" );
								already_replaced = true;
								break;
							}
						}

						if( already_replaced ) {
							new_line = function_to_string( line, func, pp, end+1 );
							DEBUG( format( "new: %s", new_line ) );
						}
					}
				}
			}

		}

		if( !already_replaced ) {
			new_line = substitude( line, "int", "long" );
		}

		std::string new_var_name;

		bool rename_var = false;

		if( var_name[0] == 'i' ) {
			rename_var = true;
			new_var_name = 'l' + var_name.substr(1);
			new_line = substitude( new_line, var_name, new_var_name );
		}

		replace_line_from_start_of_line( res, decl_pos, new_line );
		DEBUG( format( "old: %s", line ) );
		DEBUG( format( "new: %s", new_line ) );

		// Variable im ganzen File umbenennen, vermutlich ist das eh die richtige Taktik
		// und wenn das nicht klappt, dann muss die Methode etwas verfeinert werden
		if( rename_var ) {
			res = substitude( res, var_name, new_var_name );
		}


/*
		// Variable im Funktionsaufruf umbenennen
		if( rename_var ) {
			std::string::size_type pos_arg_in_prm_call = res.find( var_name, pos );
			if( pos_arg_in_prm_call != std::string::npos &&
			    pos_arg_in_prm_call <= end )
			{
				std::string prm_call_line = get_whole_line( res, pos_arg_in_prm_call );
				prm_call_line = substitude( prm_call_line, var_name, new_var_name );
				UnusedVariableHandler::replace_line( res, pos_arg_in_prm_call, prm_call_line );
			}
		}
*/

		start_in_file = pos + FUNCTION_NAME.size();

	} while( pos != std::string::npos && start_in_file < res.size() );


	return res;
}

bool FixPrmGet::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

void FixPrmGet::replace_line_from_start_of_line( std::string & buffer, std::string::size_type pos, const std::string & new_line )
{
	if( pos == std::string::npos )
		return;

	std::string::size_type ppos = buffer.rfind( '\n', pos );

	if( ppos == std::string::npos ) {
		ppos = 0;
	} else {
		ppos++;
	}

	UnusedVariableHandler::replace_line( buffer, ppos, new_line );
}
