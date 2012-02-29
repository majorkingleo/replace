#include "remove_versid_pdl.h"
#include <format.h>
#include "getline.h"
#include "find_first_of.h"
#include "utils.h"

using namespace Tools;

RemoveVersidPdl::RemoveVersidPdl()
	: RemoveVersid()
{
	keywords.push_back("&Versionid");
	keywords.push_back("VERSID:");
	keywords.push_back("$Log:");
}

std::string RemoveVersidPdl::cut_versid_function( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0,
												"&Versionid");

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::string::size_type start, end;
	Function func;

	if( !get_function( file, pos, start, end, &func ) )
		return file;

	end = file.find(';', end);

	std::string result = file.substr(0,pos);
	result += file.substr(end+1);

	return result;
}


std::string RemoveVersidPdl::cut_revision_history( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0,
												"REVISION HISTORY",
												"$Log");

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::string line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find("##") != 0 )
		return file;

	std::string::size_type end_of_comment = find_first_of(
			file, pos,
				"#############"
			);

	if( end_of_comment == std::string::npos )
		return file;

	end_of_comment = file.rfind('\n',end_of_comment);

	std::string result = file.substr(0,pos);
	result += file.substr(end_of_comment);

	return result;
}

std::string  RemoveVersidPdl::cut_VERSID( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0, "VERSID:" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;


	std::string line = get_whole_line( file, pos );

	// beginnt die Zeile mit einem Kommentar
	if( line.find("##") != 0 )
		return file;

	if( line.find("$Header") == std::string::npos )
		return file;

	std::string::size_type begin = file.rfind('\n', pos);
	std::string::size_type end = file.find('\n', pos);

	std::string result;

	if( begin != std::string::npos )
		result = file.substr(0,begin);

	result += file.substr(end);

	return result;
}

std::string  RemoveVersidPdl::remove_versid( const std::string & file )
{
	if( should_skip_file(file) )
		return file;

	std::string result = cut_revision_history( file );
	result = cut_VERSID( result );
	result = cut_versid_function( result );
	result = add_eof( result );

	return result;
}


std::string RemoveVersidPdl::add_eof( const std::string & file ) const
{
	if( file.find(';') != std::string::npos ) {
		return file;
	}

	return file + "\n1;";
}
