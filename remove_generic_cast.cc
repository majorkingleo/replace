/*
 * remove_generic_cast.cc
 *
 *  Created on: 21.06.2013
 *      Author: martin
 */
#include "remove_generic_cast.h"
#include <format.h>
#include "utils.h"
#include "CpputilsDebug.h"
#include <sstream>
#include <getline.h>

using namespace Tools;

const std::wstring RemoveGenericCast::KEY_WORD = L"MskTgeneric";

RemoveGenericCast::RemoveGenericCast( const std::wstring & FUNCTION_NAME_ )
	: FUNCTION_NAME( FUNCTION_NAME_ )
{
	keywords.push_back( FUNCTION_NAME );
	keywords.push_back( L"MskTgeneric" );
}

std::wstring RemoveGenericCast::patch_file( const std::wstring & file )
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


		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::wstring::npos )
			return res;

		CPPDEBUG( wformat( L"%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::string::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			CPPDEBUG("unable to load MskVaAssign function");
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.name != FUNCTION_NAME )
		{
			CPPDEBUG(L"function name is " + func.name);
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		// wir dürfen im 2. Argument das (MskTgeneric *) wegreissen, denn

		if( func.args.size() < 2 ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		if( func.args[1].find( KEY_WORD ) == std::wstring::npos ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		std::wstring & arg = func.args[1];
		std::wstring::size_type orig_len = arg.size();

		std::wstring::size_type end_of_cast = arg.find( L")");
		std::wstring::size_type begin_of_cast = arg.find( L"(");

		if( end_of_cast == std::string::npos ) {
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		arg = arg.substr(0,begin_of_cast) + arg.substr(end_of_cast+1);

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

		CPPDEBUG( getline( res, end ));

		std::wstring second_part_of_file = res.substr(end);

		res = first_part_of_file + str.str() + second_part_of_file;


		// die anzahl der herausgeschnittenen Buchstaben vom Ende abziehen,
		// da wir sonst leider ein wenig test überspringen, und
		// wenn in zwei Zeilen aufeinander die Funktion aufgerufen wird,
		// wir den Zweiten Aufruf überpringen.
		start_in_file = end - (orig_len - arg.size());

	} while( pos != std::wstring::npos && start_in_file < res.size() );


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
