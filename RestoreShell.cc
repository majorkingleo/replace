/*
 * RestoreShell.cc
 *
 *  Created on: 21.06.2012
 *      Author: martin
 */
#include "RestoreShell.h"
#include <format.h>
#include "utils.h"
#include "debug.h"

RestoreShell::RestoreShell()
{
	keywords.push_back( "ListTXdialog" );
}

std::string RestoreShell::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	// file already patched ?
	if( file.find("GuiNrestoreShell") != std::string::npos ) {
		return file;
	}

	std::string::size_type pos = file.find( "GuiNactiveShell" );

	if( pos == std::string::npos )
		return file;

	// find begin of this line
	std::string::size_type line_begin = file.rfind( '\n', pos );

	if( line_begin == std::string::npos )
		return file;

	std::string::size_type wgd_set_pos = find_function( "WdgGuiSet", file, line_begin );

	if( wgd_set_pos == std::string::npos )
		return file;

	Function func;
	std::string::size_type start, end;

	if( !get_function(file,wgd_set_pos,start,end,&func) ) {
		DEBUG("unable to load WdgGuiSet function");
		return file;
	}

	std::string indent = file.substr( line_begin, wgd_set_pos - line_begin );

	std::string erg = file.substr(0,line_begin);

	erg += indent + "WdgGuiSet (GuiNrestoreShell, " + func.args[1] + ");";
	erg += indent + "WdgGuiSet (GuiNmakeActiveShell, " + func.args[1];

	erg += file.substr(end);

	return erg;
}

bool RestoreShell::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}
