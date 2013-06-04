/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "correct_va_multiple_malloc.h"
#include <format.h>
#include "utils.h"
#include "debug.h"
#include <sstream>

using namespace Tools;

const std::string CorrectVaMultipleMalloc::KEY_WORD = "(void**)";
const std::string CorrectVaMultipleMalloc::VA_MALLOC = "VaMultipleMalloc";

CorrectVaMultipleMalloc::CorrectVaMultipleMalloc()
{
	keywords.push_back( "VaMultipleMalloc" );
}

std::string CorrectVaMultipleMalloc::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	std::string res( file );
	std::string::size_type start_in_file = 0;
	std::string::size_type pos = 0;

	do
	{
		// file already patched ?
		if( res.find(KEY_WORD, start_in_file) == std::string::npos ) {
			return res;
		}


		pos = res.find( VA_MALLOC, start_in_file );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "%s at line %d", VA_MALLOC, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			DEBUG("unable to load VaMultipleMalloc function");
			start_in_file = pos + VA_MALLOC.size();
			continue;
		}

		// wir dürfen nur ab dem 3. Argument das void** wegreissen, denn
		// sonst kommt es zu einem Compilerfehler.
		bool changed_something = false;

		for( unsigned i = 3; i < func.args.size(); i += 2 )
		{
			std::string & arg = func.args[i];

			std::string::size_type arg_pos = arg.find(KEY_WORD);

			if( arg_pos != std::string::npos )
			{
				std::string new_arg;

				if( arg_pos > 0 )
					new_arg = arg.substr( 0, arg_pos );

				new_arg += arg.substr( arg_pos + KEY_WORD.size() );

				DEBUG( format( "arg %d %s => %s", i, arg, new_arg ));

				arg = new_arg;

				changed_something = true;
			}
		}

		if( changed_something )
		{
			std::string first_part_of_file = res.substr(0,pos);

			std::stringstream str;

			str << func.name << "(";

			for( unsigned i = 0; i < func.args.size(); i++ )
			{
				if( i > 0 )
					str << ", ";

				str << func.args[i];
			}

			DEBUG( str.str() );

			std::string second_part_of_file = res.substr(end);

			res = first_part_of_file + str.str() + second_part_of_file;
		}

		start_in_file = end;

	} while( pos != std::string::npos && start_in_file < res.size() );


	return res;
}

bool CorrectVaMultipleMalloc::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}
