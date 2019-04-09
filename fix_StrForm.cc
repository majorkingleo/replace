/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "fix_StrForm.h"
#include <format.h>
#include "utils.h"
#include "debug.h"
#include <sstream>

using namespace Tools;

const std::string FixStrForm::KEY_WORD = "StrForm";

FixStrForm::FixStrForm()
{
	keywords.push_back( KEY_WORD );
}

std::string FixStrForm::patch_file( const std::string & file )
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


		pos = res.find( KEY_WORD, start_in_file );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "%s at line %d", KEY_WORD, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			DEBUG("unable to load sprintf function");
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


		func.name = "TO_CHAR(format";
		*func.args.rbegin() += ")";

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

		start_in_file = end;

	} while( pos != std::string::npos && start_in_file < res.size() );

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
