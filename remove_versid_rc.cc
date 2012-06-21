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
	keywords.push_back("Version");
	keywords.push_back("VERSID:");
	keywords.push_back("$Log:");
}

std::string RemoveVersidRc::cut_versid_atom( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0, "Version" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::string line = get_whole_line( file, pos );

	if( line.find("$Header") == std::string::npos )
		return file;

	std::string::size_type begin = file.rfind('\n', pos);
	std::string::size_type end = file.find('\n', pos);

	std::string result = file.substr(0,begin);
	result += file.substr(end);

	return result;
}


std::string RemoveVersidRc::remove_versid( const std::string & file )
{
	if( should_skip_file(file) )
		return file;

	std::string result = cut_revision_history( file );
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
