/*
 * add_wdgassign.cc
 *
 *  Created on: 04.07.2014
 *      Author: martin
 */

#include "utils.h"
#include "add_wdgassign.h"
#include <format.h>
#include "debug.h"
#include <cppdir.h>

using namespace Tools;

AddWamasWdgAssignMenu::AddWamasWdgAssignMenu( const std::string & assign_function )
	: SHELL_CREATE_FUNCTION( assign_function )
{

}

std::string AddWamasWdgAssignMenu::patch_file( const std::string & file )
{
	std::string::size_type pos = 0;

	std::string res(file);

	bool add_include_line = false;

	do {
		pos = find_function( SHELL_CREATE_FUNCTION, res, pos );

		if( pos == std::string::npos )
		{
			break;
		}

		DEBUG( format( "found %s at line %d", SHELL_CREATE_FUNCTION,  get_linenum(res,pos) ));

		std::string shell;
		std::string::size_type assign_pos;

		std::string line_before = get_whole_line( res, pos );
		if( ( assign_pos = line_before.find( "=" ) ) != std::string::npos ) {
			shell = strip(line_before.substr(0,assign_pos));
		}

		if( shell.empty() )
		{
			pos += SHELL_CREATE_FUNCTION.size();
			continue;
		}

		// wenn schon WamasWdgAssignMenu dannach dasteht, dann brauch ma nix mehr machen
		std::string::size_type end_of_function = res.find("}", pos );
		std::string::size_type existing_wdg_assign = res.find( "WamasWdgAssignMenu", pos );

		if( existing_wdg_assign != std::string::npos && end_of_function != std::string::npos )
		{
			if( end_of_function >  existing_wdg_assign )
			{
				pos += SHELL_CREATE_FUNCTION.size();
				continue;
			}
		}



		Function func;
		std::string::size_type start, end;

		if( get_function( res, pos, start, end, &func ) ) {

			std::string::size_type indent_count = line_before.find_first_not_of( " \t" );

			std::string indent;

			if( indent_count != std::string::npos )
			{
				indent = line_before.substr( 0,indent_count );
			}

			std::string::size_type line_end = res.find("\n", end );

			CppDir::File cppfile( file_name );

			std::string assig_menu_string = format( "\n%sWamasWdgAssignMenu(%s,\"%s\");", indent, shell, cppfile.get_name());

			res.insert( line_end, assig_menu_string );

			pos += assig_menu_string.size();

			add_include_line = true;

		} // if get_function

		pos += SHELL_CREATE_FUNCTION.size();

	} while( pos != std::string::npos && pos < res.size() );

	DEBUG( format( "add_include_line %d", add_include_line));

	if( add_include_line )
	{
		std::string::size_type include_pos = res.rfind( "#include" );

		if( include_pos != std::string::npos )
		{
			res.insert(include_pos,"#include <wamaswdg.h>\n");
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


