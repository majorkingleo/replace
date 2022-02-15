#include "remove_versid_rc.h"
#include "remove_versid_pdl.h"
#include <format.h>
#include "getline.h"
#include "find_first_of.h"
#include "utils.h"

using namespace Tools;

RemoveVersidRc::RemoveVersidRc()
	: RemoveVersidPdl()
{
	keywords.clear();
	keywords.push_back( L"Version" );
	keywords.push_back( L"VERSID:" );
	keywords.push_back( L"$Log:" );
}

std::wstring RemoveVersidRc::cut_versid_atom( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0, L"Version" );

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::wstring line = get_whole_line( file, pos );

	if( line.find( L"$Header" ) == std::wstring::npos )
		return file;

	std::wstring::size_type begin = file.rfind( L'\n', pos);
	std::wstring::size_type end = file.find( L'\n', pos);

	std::wstring result = file.substr(0,begin);
	result += file.substr(end);

	return result;
}


std::wstring RemoveVersidRc::remove_versid( const std::wstring & file )
{
	if( should_skip_file(file) )
		return file;

	std::wstring result = cut_revision_history( file );
	result = cut_VERSID( result );
	result = cut_versid_atom( result );

	return result;
}


bool RemoveVersidRc::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::RC_FILE:
		  return true;
	  default:
		  return false;
	}
}
