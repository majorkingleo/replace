/*
 * RestoreShell.cc
 *
 *  Created on: 21.06.2012
 *      Author: martin
 */
#include "RestoreShell.h"
#include <format.h>
#include "utils.h"
#include "CpputilsDebug.h"

RestoreShell::RestoreShell()
{
	keywords.push_back( L"ListTXdialog" );
}

std::wstring RestoreShell::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	// file already patched ?
	if( file.find(L"GuiNrestoreShell") != std::wstring::npos ) {
		return file;
	}

	std::wstring::size_type pos = file.find( L"GuiNactiveShell" );

	if( pos == std::wstring::npos )
		return file;

	// find begin of this line
	std::wstring::size_type line_begin = file.rfind( L'\n', pos );

	if( line_begin == std::wstring::npos )
		return file;

	std::wstring::size_type wgd_set_pos = find_function( L"WdgGuiSet", file, line_begin );

	if( wgd_set_pos == std::wstring::npos )
		return file;

	Function func;
	std::wstring::size_type start, end;

	if( !get_function(file,wgd_set_pos,start,end,&func) ) {
		CPPDEBUG("unable to load WdgGuiSet function");
		return file;
	}

	std::wstring indent = file.substr( line_begin, wgd_set_pos - line_begin );

	std::wstring erg = file.substr(0,line_begin);

	erg += indent + L"WdgGuiSet (GuiNrestoreShell, " + func.args[1] + L");";
	erg += indent + L"WdgGuiSet (GuiNmakeActiveShell, " + func.args[1];

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
