/*
 * add_mlm.cc
 *
 *  Created on: 18.10.2021
 *      Author: martin
 */
#include "add_mlm.h"
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


bool AddMlM::PatchThisFunctionIfArgXequals::should_i_patch_this_function( const Function & func )
{
	if( func.args.size() < ARG_NUM ) {
		return false;
	}

	DEBUG( format( "checking function: %s arg:%d = '%s' required '%s'", func.name, ARG_NUM,  func.args[ARG_NUM], VAR_NAME ) );

	if( func.args[ARG_NUM] == VAR_NAME ) {
		return true;
	}

	return false;
}

AddMlM::AddMlM( const std::string & FUNCTION_NAME_,
				const unsigned FUNCTION_ARG_NUM_,
				const std::string & FUNCTION_CALL_ )
	: FUNCTION_NAME( FUNCTION_NAME_ ),
	  FUNCTION_ARG_NUM( FUNCTION_ARG_NUM_ ),
	  FUNCTION_CALL( FUNCTION_CALL_ ),
	  vChecker()
{
	keywords.push_back( FUNCTION_NAME );
}

AddMlM::~AddMlM()
{
	for( auto checker : vChecker ) {
		delete checker;
		vChecker.clear();
	}
}

std::string AddMlM::patch_file( const std::string & file )
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

		// skip end bracket
		end += 1;

		if( func.name != FUNCTION_NAME )
		{
			DEBUG( format("function name is '%s'", func.name) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.args.size() < FUNCTION_ARG_NUM ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}


		/*
		 * should I skip this function, checker will tell it me
		 */
		bool skip_this_function = false;

		for( auto checker : vChecker ) {

			if( !checker->should_i_patch_this_function(func) ) {
				skip_this_function = true;
				break;
			}
		}

		if( skip_this_function ) {
			start_in_file = end;
			continue;
		}



		std::string var_name = strip( func.args[FUNCTION_ARG_NUM-1] );

		bool changed_something = false;

		// it is a C string
		if( var_name.find( "\"" ) == 0 ) {


			std::string & s_arg = func.args[FUNCTION_ARG_NUM-1];

			std::string::size_type start = s_arg.find("\"");
			std::string::size_type end = s_arg.rfind("\"");

			std::string leading_spaces = s_arg.substr(0,start);
			std::string trailing_spaces = s_arg.substr(end+1);

			std::string string_arg = format( "%s%s(%s)%s", leading_spaces, FUNCTION_CALL, var_name, trailing_spaces );

			func.args[FUNCTION_ARG_NUM-1] = string_arg;
			changed_something = true;
		}

		if( changed_something )
		{
			res = function_to_string( res, func, pos, end );
			start_in_file = end;
		}

		if( !changed_something ) {
			start_in_file = pos + FUNCTION_NAME.size();
		}

	} while( pos != std::string::npos && start_in_file < res.size() );


	return res;
}

bool AddMlM::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

