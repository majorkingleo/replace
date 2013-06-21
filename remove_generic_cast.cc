/*
 * remove_generic_cast.cc
 *
 *  Created on: 21.06.2013
 *      Author: martin
 */
#include "remove_generic_cast.h"
#include <format.h>
#include "utils.h"
#include "debug.h"
#include <sstream>
#include <getline.h>

using namespace Tools;

const std::string RemoveGenericCast::KEY_WORD = "MskTgeneric";

RemoveGenericCast::RemoveGenericCast( const std::string & FUNCTION_NAME_ )
	: FUNCTION_NAME( FUNCTION_NAME_ )
{
	keywords.push_back( FUNCTION_NAME );
	keywords.push_back( "MskTgeneric" );
}

std::string RemoveGenericCast::patch_file( const std::string & file )
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


		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			DEBUG("unable to load MskVaAssign function");
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.name != FUNCTION_NAME )
		{
			DEBUG("function name is " + func.name);
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		// wir dürfen im 2. Argument das (MskTgeneric *) wegreissen, denn

		if( func.args.size() < 2 ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.args[1].find( KEY_WORD ) == std::string::npos ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		std::string & arg = func.args[1];
		std::string::size_type orig_len = arg.size();

		std::string::size_type end_of_cast = arg.find( ")");
		std::string::size_type begin_of_cast = arg.find( "(");

		if( end_of_cast == std::string::npos ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		arg = arg.substr(0,begin_of_cast) + arg.substr(end_of_cast+1);

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

		DEBUG( getline( res, end ));

		std::string second_part_of_file = res.substr(end);

		res = first_part_of_file + str.str() + second_part_of_file;


		// die anzahl der herausgeschnittenen Buchstaben vom Ende abziehen,
		// da wir sonst leider ein wenig test überspringen, und
		// wenn in zwei Zeilen aufeinander die Funktion aufgerufen wird,
		// wir den Zweiten Aufruf überpringen.
		start_in_file = end - (orig_len - arg.size());

	} while( pos != std::string::npos && start_in_file < res.size() );


	return res;
}

bool RemoveGenericCast::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}
