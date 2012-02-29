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
	keywords.push_back("VERSID:");
	keywords.push_back("$Log:");
	keywords.push_back("$Header:");
}

std::string  RemoveVersidPl::cut_Header( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0, "$Header:" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::string line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find("#") != 0 )
		return file;

	if( line.find("cvsroot") == std::string::npos )
		return file;

	std::string::size_type begin = file.rfind('\n', pos);
	std::string::size_type end = file.find('\n', pos);

	std::string result;

	if( begin != std::string::npos )
		result = file.substr(0,begin);

	result += file.substr(end);

	return result;
}

std::string RemoveVersidPl::remove_versid( const std::string & file )
{
	if( should_skip_file(file) )
		return file;

	std::string result = cut_revision_history( file );
	result = cut_VERSID( result );
	result = cut_Header( result );

	return result;
}

