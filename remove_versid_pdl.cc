#include "remove_versid_pdl.h"
#include <format.h>
#include "getline.h"
#include "find_first_of.h"
#include "utils.h"

using namespace Tools;

RemoveVersidPdl::RemoveVersidPdl()
	: RemoveVersid()
{
	keywords.push_back( L"&Versionid" );
	keywords.push_back( L"VERSID:" );
	keywords.push_back( L"$Log:" );
}

std::wstring RemoveVersidPdl::cut_versid_function( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0,
												L"&Versionid");

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::wstring::size_type start, end;
	Function func;

	if( !get_function( file, pos, start, end, &func ) )
		return file;

	end = file.find( L';', end);

	std::wstring result = file.substr(0,pos);
	result += file.substr(end+1);

	return result;
}


std::wstring RemoveVersidPdl::cut_revision_history( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0,
												L"REVISION HISTORY",
												L"$Log");

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::wstring line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find( L"##" ) != 0 )
		return file;

	std::wstring::size_type end_of_comment = find_first_of(
			file, pos,
				L"#############"
			);

	if( end_of_comment == std::wstring::npos )
		return file;

	end_of_comment = file.rfind( L'\n',end_of_comment);

	std::wstring result = file.substr(0,pos);
	result += file.substr(end_of_comment);

	return result;
}

std::wstring  RemoveVersidPdl::cut_VERSID( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0, L"VERSID:" );

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::wstring line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find( L"##" ) != 0 )
		return file;

	if( line.find( L"$Header" ) == std::wstring::npos )
		return file;

	std::wstring::size_type begin = file.rfind( L'\n', pos);
	std::wstring::size_type end = file.find( L'\n', pos);

	std::wstring result;

	if( begin != std::wstring::npos )
		result = file.substr(0,begin);

	result += file.substr(end);

	return result;
}

std::wstring  RemoveVersidPdl::remove_versid( const std::wstring & file )
{
	if( should_skip_file(file) )
		return file;

	std::wstring result = cut_revision_history( file );
	result = cut_VERSID( result );
	result = cut_versid_function( result );
	result = add_eof( result );

	return result;
}


std::wstring RemoveVersidPdl::add_eof( const std::wstring & file ) const
{
	if( file.find( L';' ) != std::wstring::npos ) {
		return file;
	}

	return file + L"\n1;";
}


bool RemoveVersidPdl::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::PDL_FILE:
	  case FILE_TYPE::PDS_FILE:
		  return true;
	  default:
		  return false;
	}
}
