/*
 * add_wdgassign.cc
 *
 *  Created on: 04.07.2014
 *      Author: martin
 */

#include "utils.h"
#include "add_wdgassign.h"
#include <format.h>
#include "CpputilsDebug.h"
#include <cppdir.h>
#include <string_utils.h>

using namespace Tools;

AddWamasWdgAssignMenu::AddWamasWdgAssignMenu( const std::wstring & assign_function )
	: SHELL_CREATE_FUNCTION( assign_function )
{

}

std::wstring AddWamasWdgAssignMenu::patch_file( const std::wstring & file )
{
	std::wstring::size_type pos = 0;

	std::wstring res(file);

	bool add_include_line = false;

	do {
		pos = find_function( SHELL_CREATE_FUNCTION, res, pos );

		if( pos == std::string::npos )
		{
			break;
		}

		CPPDEBUG( format( "found %s at line %d", w2out(SHELL_CREATE_FUNCTION), get_linenum(res,pos) ));

		std::wstring shell;
		std::wstring::size_type assign_pos;

		std::wstring line_before = get_whole_line( res, pos );
		if( ( assign_pos = line_before.find( L"=" ) ) != std::wstring::npos ) {
			shell = strip(line_before.substr(0,assign_pos));
		}

		if( shell.empty() )
		{
			pos += SHELL_CREATE_FUNCTION.size();
			continue;
		}

		// wenn schon WamasWdgAssignMenu dannach dasteht, dann brauch ma nix mehr machen
		std::wstring::size_type end_of_function = res.find(L"}", pos );
		std::wstring::size_type existing_wdg_assign = res.find( L"WamasWdgAssignMenu", pos );

		if( existing_wdg_assign != std::wstring::npos && end_of_function != std::string::npos )
		{
			if( end_of_function >  existing_wdg_assign )
			{
				pos += SHELL_CREATE_FUNCTION.size();
				continue;
			}
		}



		Function func;
		std::wstring::size_type start, end;

		if( get_function( res, pos, start, end, &func ) ) {

			std::wstring::size_type indent_count = line_before.find_first_not_of( L" \t" );

			std::wstring indent;

			if( indent_count != std::string::npos )
			{
				indent = line_before.substr( 0,indent_count );
			}

			std::wstring::size_type line_end = res.find(L"\n", end );

			CppDir::File cppfile( file_name );

			std::wstring assig_menu_string = wformat( L"\n%sWamasWdgAssignMenu(%s,\"%s\");", indent, shell, in2w(cppfile.get_name()));

			res.insert( line_end, assig_menu_string );

			pos += assig_menu_string.size();

			add_include_line = true;

		} // if get_function

		pos += SHELL_CREATE_FUNCTION.size();

	} while( pos != std::wstring::npos && pos < res.size() );

	CPPDEBUG( format( "add_include_line %d", add_include_line));

	if( add_include_line )
	{
		std::wstring::size_type include_pos = res.rfind( L"#include" );

		if( include_pos != std::string::npos )
		{
			res.insert(include_pos,L"#include <wamaswdg.h>\n");
		}
	}


	return res;
}


bool AddWamasWdgAssignMenu::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}


