/*
 * space_between_literal_handler.cc
 *
 *  Created on: 29.05.2019
 *      Author: martin
 */

#include "space_between_literal_handler.h"
#include <string_utils.h>
#include <debug.h>
#include "getline.h"
#include <cpp_util.h>
#include "unused_variable_handler.h"
#include "DetectLocale.h"

using namespace Tools;

SpaceBetweenLiteralHandler::SpaceBetweenLiteralHandler()
	: locations()
{

}

void SpaceBetweenLiteralHandler::read_compile_log_line( const std::wstring & line )
{
	if( line.find( L"-Wliteral-suffix") == std::wstring::npos )
		return;

	SpaceBetweenLiteralWarnings location = get_location_from_line( line );

	std::vector<std::wstring> sl = split_simple( line, L" ");

	std::vector<std::wstring>::reverse_iterator it = sl.rbegin();

	// GCC 4.8 style: warning: unused variable 'dbrv' [-Wunused-variable]

	if( *it->begin() == L'[' && *it->rbegin() == L']' ) {
		it++;
	}

	if( it == sl.rend() ) {
		return;
	}

	location.var_name = *it;
	location.var_name = strip( location.var_name, L"'‘’");
	location.compile_log_line = line;

	DEBUG( wformat( L"%s %s", location, location.var_name ) );

	locations.push_back( location );
}

bool SpaceBetweenLiteralHandler::want_file( const FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( locations[i].file == file.name ) {
			return true;
		}
	}

	return false;
}

void SpaceBetweenLiteralHandler::fix_file( FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( locations[i].file == file.name ) {
			fix_warning( locations[i], file.content );
		}
	}
}

void SpaceBetweenLiteralHandler::report_unfixed_compile_logs()
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( !locations[i].fixed )
		{
			std::cout << "(unfixed) " << DetectLocale::w2out(locations[i].compile_log_line) << '\n';
		}
	}
}




bool SpaceBetweenLiteralHandler::is_escaped( const std::wstring &s, std::wstring::size_type start )
{
	bool escaped = false;

	if( start > 0 )
	{
		if( s[start-1] == L'\\' )
		{
			escaped = true;

			if( start > 1 )
			{
				if( s[start-2] == L'\\' )
				{
					escaped = false;
				}
			}
		}
	}

	return escaped;
}

std::vector<SpaceBetweenLiteralHandler::Pair> SpaceBetweenLiteralHandler::find_exclusive( const std::wstring &s )
{
	std::vector<Pair> ex;

	std::wstring::size_type pos = 0, start = 0, end = 0;

	while( true )
	{

		/* find starting pos */
		while( true )
		{
			start = s.find( L'"', pos );

			if( start == std::wstring::npos ) {
				return ex;
			}

			/* is the " escaped? */
			if( is_escaped( s, start ) )
			{
				pos = start + 1;
				continue;
			}

			break;
		}

		// find second "
		pos = start + 1;
		while( true )
		{
			end = s.find( L'"', pos );

			if( end == std::wstring::npos )
				return ex;

			if( is_escaped( s, end ) )
			{
				pos = end + 1;
				continue;
			}

			break;
		}

		ex.push_back( Pair( start, end ) );
		pos = end + 1;
	}
}

bool SpaceBetweenLiteralHandler::is_exclusive( const std::wstring &s, const std::vector<SpaceBetweenLiteralHandler::Pair> &exclude, std::wstring::size_type pos1 )
{
	bool exclusive = false;
	for( unsigned i = 0; i < exclude.size(); i++ )
	{
		if( exclude[i].start < pos1 &&
				exclude[i].end > pos1 )
		{
			exclusive = true;
			break;
		}
	}

	return exclusive;
}

static std::wstring space2Pos( std::wstring::size_type pos, wchar_t mark_sign = L'^' )
{
	return fill_leading( L"", L" ", pos ) + mark_sign;
}

void SpaceBetweenLiteralHandler::fix_warning( SpaceBetweenLiteralWarnings & warning, std::wstring & content )
{
	std::wstring::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::wstring::npos ) {
		throw REPORT_EXCEPTION( format( "can't get file position for warning %s", DetectLocale::w2out(warning.compile_log_line)));
	}

	std::wstring line = getline(content,  pos );

	DEBUG( line );

	std::vector<Pair> exclude = find_exclusive( line );

	std::wstring::size_type pos1 = 0;
	std::wstring::size_type last_pos = 0;

	bool string_started = false;

	std::wstringstream res;

	do
	{
		pos1 = line.find( L'"', pos1 );

		if( pos1 == std::wstring::npos ) {
			break;
		}

		DEBUG( wformat( L"found \" at %d\n%s\n%s", pos1, line, space2Pos( pos1 ) ) );

		if( is_exclusive( line, exclude, pos1 ) )
		{
			pos1++;
			continue;
		}

		if( !string_started ) {
			string_started = true;
			res << line.substr( last_pos, pos1 - last_pos);

			// TExecSql(tid,"update map set status="STR_MASTAT_FERTIG" where parentid=:a",
			//              ^---------- hier einen Space einbauen
			if( pos1 > 0 && !isspace( line[pos1-1] ) ) {
				res << L' ';
			}

			res << line.substr(pos1,1);
			last_pos = pos1+1;
		} else {
			// jetzt einen space einbauen
			// TExecSql(tid,"update map set status="STR_MASTAT_FERTIG" where parentid=:a",
			// ---------- einen Space hier einbauen ^
			res << line.substr( last_pos, pos1 - last_pos + 1);

			bool append_space = true;
			last_pos = pos1+1;

			if( last_pos < line.size() ) {
				DEBUG( wformat( L"last sign: '%s'", line[last_pos]) );

				// TExecSql(tid,"update map set status="STR_MASTAT_FERTIG" where parentid=:a",
				// ---------------------------------------------- keinen Space hier einbauen ^
				if( !isalnum( line[last_pos] ) ) {
					append_space = false;
				}

				// wenn am Ende schon ein Leerzeichen ist, dann brauchts keine extra Leerzeichen
				if( isspace( line[last_pos] ) ) {
					append_space = false;
				}

			}

			// Nur ein Leerzeichen anhängen, wenn es nicht das Ende der Zeile ist
			if( last_pos < line.size() ) {
				if(  append_space ) {
					res << L' ';
				}
			}

			string_started = false;
		}

		pos1++;

	} while( pos1 != std::wstring::npos );

	res << line.substr( last_pos );

	std::wstring new_line = res.str();

	DEBUG( wformat( L"old line: '%s'", line ) );
	DEBUG( wformat( L"new line: '%s'", new_line ) );

	UnusedVariableHandler::replace_line( content, pos, new_line );

	warning.fixed = true;
}



