/*
 * remove_versid_pl.cc
 *
 *  Created on: 09.12.2011
 *      Author: martin
 */
#include "remove_versid_pl.h"
#include "find_first_of.h"
#include <string_utils.h>
#include "utils.h"

using namespace Tools;

RemoveVersidPl::RemoveVersidPl()
	: RemoveVersidPdl()
{
	keywords.clear();
	keywords.push_back( L"VERSID:" );
	keywords.push_back( L"$Log:" );
	keywords.push_back( L"$Header:" );
}

std::wstring  RemoveVersidPl::cut_Header( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0, L"$Header:" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::wstring line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find( L"#" ) != 0 )
		return file;

	if( line.find( L"cvsroot" ) == std::wstring::npos )
		return file;

	std::wstring::size_type begin = file.rfind( L'\n', pos);
	std::wstring::size_type end = file.find( L'\n', pos);

	std::wstring result;

	if( begin != std::wstring::npos )
		result = file.substr(0,begin);

	result += file.substr(end);

	return result;
}

std::wstring RemoveVersidPl::remove_versid( const std::wstring & file )
{
	if( should_skip_file(file) )
		return file;

	std::wstring result = cut_revision_history( file );
	result = cut_VERSID( result );
	result = cut_Header( result );

	return result;
}

bool RemoveVersidPl::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::PL_FILE:
		  return true;
	  default:
		  return false;
	}
}

