#include "remove_versid_ch.h"
#include <format.h>
#include "getline.h"
#include "find_first_of.h"
#include "utils.h"
#include <debug.h>

using namespace Tools;

RemoveVersidCh::RemoveVersidCh( bool noheader_ )
: RemoveVersid(),
  noheader(noheader_)
{
	keywords.push_back( L"VERSID" );
    keywords.push_back( L"<versid.h>" );
    keywords.push_back( L"\"versid.h\"" );
    keywords.push_back( L"$Log:" );
    keywords.push_back( L"SCCSID" );

    header_template = make_header_template();
	// test
}

std::wstring RemoveVersidCh::make_header_template() const
{
	time_t now = time(NULL);
	struct tm* tm = localtime(&now);

	return wformat(
			L"/**\n"
			L"* @file\n"
			L"* @todo describe file content\n"
			L"* @author Copyright (c) %s SSI Schaefer IT Solutions\n"
			L"*/", tm->tm_year + 1900 );
}

std::wstring RemoveVersidCh::cut_revision_history( const std::wstring & file ) const
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
	if( line.find(L"+*") != 0 )
		return file;

	std::wstring::size_type end_of_comment = find_first_of(
			file, pos,
				L"************/",
				L"======================*/"
			);

	if( end_of_comment == std::wstring::npos )
		return file;

	end_of_comment = file.rfind(L'\n',end_of_comment);

	std::wstring result = file.substr(0,pos);
	result += file.substr(end_of_comment);

	return result;
}

std::wstring RemoveVersidCh::cut_versid_h( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0,
												L"<versid.h>",
												L"\"versid.h\"" );

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::wstring line = get_whole_line( file, pos );

	if( line.find(L"#include") == std::wstring::npos )
		return file;

	std::wstring::size_type begin_of_line = file.rfind(L'\n', pos);
	std::wstring::size_type end_of_line = file.find(L'\n', pos);

	if( begin_of_line == std::wstring::npos ||
	    end_of_line == std::wstring::npos )
		return file;

	std::wstring result = file.substr(0,begin_of_line);
	result += file.substr(end_of_line);

	return result;
}

std::wstring RemoveVersidCh::cut_versid_ch_makro( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0,
												L"VERSIDH",
												L"VERSIDC" );

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::wstring::size_type start, end;
	Function func;

	if( !get_function( file, pos, start, end, &func ) )
		return file;

	std::wstring result = file.substr(0,pos);
	result += file.substr(end+1);

	return result;
}

std::wstring RemoveVersidCh::cut_the_easy_stuff( const std::wstring & file ) const
{
	std::wstring result = cut_revision_history( file );
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

std::wstring RemoveVersidCh::cut_static_versid( const std::wstring & file ) const
{
	std::wstring::size_type pos = find_first_of( file, 0,
												L"VERSID_H",
												L"VERSID_C",
												L"VERSID");

	if( pos == std::wstring::npos )
		return file;

	if( is_in_string( file, pos ) )
		return file;

	std::wstring line = get_whole_line( file, pos );

	if( line.find( L"static" ) == std::wstring::npos ) {
		DEBUG("no static found");
		return file;
	}

	if( line.find( L"char" ) == std::wstring::npos ) {
		DEBUG("no char found");
		return file;
	}

	std::wstring::size_type start = file.rfind(L'\n',pos);

	pos = file.find(L'\n', pos) + 1;

	line = get_line( file, pos );

	if( line.find(L"defined") == std::wstring::npos ||
	    line.find(L"__GNUC__") == std::wstring::npos ) {
		DEBUG("defined(__GNUC__) not found");
		return file;
	}

	pos += line.size() + 1;

	line = get_line( file, pos );

	if( line.find(L"__attribute__ ((unused))") == std::wstring::npos ) {
		DEBUG( "__attribute__ ((unused)) not found");
		return file;
	}

	pos = file.find(L';', pos);

	if( pos == std::wstring::npos ) {
		DEBUG( "; not found");
		return file;
	}

	std::wstring result = file.substr(0,start);
	result += file.substr(pos+1);

	DEBUG( wformat(L"cutting: >>%s<<",  file.substr(start,pos-start)) );

	return result;
}

std::wstring RemoveVersidCh::remove_versid( const std::wstring & file_)
{
	if( should_skip_file( file_ ) )
	{
		return file_;
	}

	std::wstring file = cut_static_versid( file_ );

	// bis zum ersten include suche, das nicht #include <versid.h> ist

	std::wstring::size_type start = 0;
	std::wstring::size_type pos = 0;

	while( true )
	{
		pos = find_first_of( file, start, L"#include", L"#ifdef", L"#ifndef");
		// pos = file.find("#include", start);

		if( pos == std::wstring::npos ) {
			return file;
		}

		std::wstring::size_type pos2 = start;
		std::wstring::size_type pos2_start = start;

		while( pos2 < pos )
		{
			pos2 = file.find(L"#if", pos2_start);

			if( pos2 != std::wstring::npos && pos2 < pos )
			{
				std::wstring line = getline( file, pos2 );

				if( line.find(L"__lint") != std::wstring::npos )
				{
					pos2_start = pos2 + 3;
					continue;
				}

				if( line == L"#if 0" )
				{
					pos2_start = pos2 + 3;
					continue;
				}

				// ab hier wegschneiden
				pos = pos2;
			}
		}

		std::wstring line  = getline( file, pos );

		if( line.find(L"<versid.h>") != std::wstring::npos ||
			line.find(L"\"versid.h\"") != std::wstring::npos ||
			line.find(L"__LINT__") != std::wstring::npos ||
			line.find(L"lint") != std::wstring::npos) {
			start = pos + line.size();
			continue;
		}

		break;
	}

	if( pos == 0 ) // nichts hat sich ge�ndert
	{
		return cut_the_easy_stuff(file);
	}

	std::wstring stripped_file = file.substr( pos );

	std::wstring result;

	if( !noheader ) {
		if( stripped_file.find(L"PROJECT:") == std::wstring::npos )
			result += header_template + L"\n";
	}

	result += stripped_file;

	result = cut_the_easy_stuff(result);

	return result;
}


bool RemoveVersidCh::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::HEADER:
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}
