#include "remove_versid_ch.h"
#include <format.h>
#include "getline.h"
#include "find_first_of.h"
#include "utils.h"

using namespace Tools;

RemoveVersidCh::RemoveVersidCh( bool noheader_ )
: RemoveVersid(),
  noheader(noheader_)
{
	keywords.push_back("VERSID");
    keywords.push_back("<versid.h>");
    keywords.push_back("\"versid.h\"");
    keywords.push_back("$Log:");

    header_template = make_header_template();
}

std::string RemoveVersidCh::make_header_template() const
{
	time_t now = time(NULL);
	struct tm* tm = localtime(&now);

	return format(
			"/**\n"
			"* @file\n"
			"* @todo describe file content\n"
			"* @author Copyright (c) %s Salomon Automation GmbH\n"
			"*/", tm->tm_year + 1900 );
}

std::string RemoveVersidCh::cut_revision_history( const std::string & file ) const
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
	if( line.find("+*") != 0 )
		return file;

	std::string::size_type end_of_comment = find_first_of(
			file, pos,
				"************/",
				"======================*/"
			);

	if( end_of_comment == std::string::npos )
		return file;

	end_of_comment = file.rfind('\n',end_of_comment);

	std::string result = file.substr(0,pos);
	result += file.substr(end_of_comment);

	return result;
}

std::string RemoveVersidCh::cut_versid_h( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0,
												"<versid.h>",
												"\"versid.h\"" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::string line = get_whole_line( file, pos );

	if( line.find("#include") == std::string::npos )
		return file;

	std::string::size_type begin_of_line = file.rfind('\n', pos);
	std::string::size_type end_of_line = file.find('\n', pos);

	if( begin_of_line == std::string::npos ||
	    end_of_line == std::string::npos )
		return file;

	std::string result = file.substr(0,begin_of_line);
	result += file.substr(end_of_line);

	return result;
}

std::string RemoveVersidCh::cut_versid_ch_makro( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0,
												"VERSIDH",
												"VERSIDC" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::string::size_type start, end;
	Function func;

	if( !get_function( file, pos, start, end, &func ) )
		return file;

	std::string result = file.substr(0,pos);
	result += file.substr(end+1);

	return result;
}

std::string RemoveVersidCh::cut_the_easy_stuff( const std::string & file ) const
{
	std::string result = cut_revision_history( file );
	result = cut_versid_h( result );
	result = cut_versid_ch_makro( result );

	return result;
}

/* sowas herausschneiden
 *
#define _LI_ZAP_C_H
    static char VERSI D_H []
#if defined(__GNUC__)
    __attribute__ ((unused))
#endif
    = "$Header: /cvsrootb/ALPLA/llr/src/term/li_zap.h,v 1.1.1.1 2004/05/28 08:15:13 mwirnsp Exp $$Locker:  $";
#endif
 *
 */

std::string RemoveVersidCh::cut_static_versid( const std::string & file ) const
{
	std::string::size_type pos = find_first_of( file, 0,
												"VERSID_H",
												"VERSID_C" );

	if( pos == std::string::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::string line = get_whole_line( file, pos );

	if( line.find( "static" ) == std::string::npos )
		return file;

	if( line.find( "char" ) == std::string::npos )
			return file;

	std::string::size_type start = file.rfind('\n',pos);

	pos = file.find('\n', pos) + 1;

	line = get_line( file, pos );

	if( line.find("#if defined(__GNUC__)") == std::string::npos )
		return file;

	pos += line.size() + 1;

	line = get_line( file, pos );

	if( line.find("__attribute__ ((unused))") == std::string::npos )
		return file;

	pos = file.find(';', pos);

	if( pos == std::string::npos )
		return file;

	std::string result = file.substr(0,start);
	result += file.substr(pos+1);

	return result;
}

std::string RemoveVersidCh::remove_versid( const std::string & file_)
{
	if( should_skip_file( file_ ) )
	{
		return file_;
	}

	std::string file = cut_static_versid( file_ );

	// bis zum ersten include suche, das nicht #include <versid.h> ist

	std::string::size_type start = 0;
	std::string::size_type pos = 0;

	while( true )
	{
		pos = find_first_of( file, start, "#include", "#ifdef", "#ifndef");
		// pos = file.find("#include", start);

		if( pos == std::string::npos ) {
			return file;
		}

		std::string::size_type pos2 = start;
		std::string::size_type pos2_start = start;

		while( pos2 < pos )
		{
			pos2 = file.find("#if", pos2_start);

			if( pos2 != std::string::npos && pos2 < pos )
			{
				std::string line = getline( file, pos2 );

				if( line.find("__lint") != std::string::npos )
				{
					pos2_start = pos2 + 3;
					continue;
				}

				if( line == "#if 0" )
				{
					pos2_start = pos2 + 3;
					continue;
				}

				// ab hier wegschneiden
				pos = pos2;
			}
		}

		std::string line  = getline( file, pos );

		if( line.find("<versid.h>") != std::string::npos ||
			line.find("\"versid.h\"") != std::string::npos ||
			line.find("__LINT__") != std::string::npos ) {
			start = pos + line.size();
			continue;
		}

		break;
	}

	if( pos == 0 ) // nichts hat sich geändert
	{
		return cut_the_easy_stuff(file);
	}

	std::string stripped_file = file.substr( pos );

	std::string result;

	if( !noheader ) {
		if( stripped_file.find("PROJECT:") == std::string::npos )
			result += header_template + "\n";
	}

	result += stripped_file;

	result = cut_the_easy_stuff(result);

	return result;
}
