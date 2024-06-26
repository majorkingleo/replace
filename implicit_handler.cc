/*
 * implicit_handler.cc
 *
 *  Created on: 18.02.2014
 *      Author: martin
 */

#include "implicit_handler.h"
#include "debug.h"
#include "getline.h"
#include "utils.h"
#include "string_utils.h"
#include "find_files.h"
#include "cppdir.h"
#include "xml.h"
#include "read_file.h"
#include "HandleFile.h"

using namespace Tools;

ImplicitHandler::ImplicitHandler( const std::string & srcdir_ )
: FixFromCompileLog::Handler(),
  implicit_warnings_locations(),
  srcdir(srcdir_),
  header_files(),
  symbol_header_file_map(),
  warning_text(L"warning: implicit declaration of function")
{
	search_for_header_files();

}

void ImplicitHandler::search_for_header_files()
{
	// add some defaults
	add_default( L"memset", L"string.h");
	add_default( L"strcpy", L"string.h");
	add_default( L"memcpy", L"string.h");
	add_default( L"bsearch", L"stdlib.h");
	add_default( L"qsort", L"stdlib.h");
	add_default( L"free", L"stdlib.h");
	add_default( L"malloc", L"stdlib.h");


	FILE_SEARCH_LIST files;

	find_files( srcdir, files );

	const char * eprefix = getenv( "EPREFIX" );
	const char * trootdir = getenv( "TROOTDIR" );
	const char * wmprootdir = getenv( "WMPROOTDIR" );

	if( eprefix != NULL )
	{
		std::string path = CppDir::concat_dir( eprefix, "/usr/include" );

		if( !find_files( path , files ) )
		{
			std::cerr << format( "cannot include files from '%s'\n", path );
		}
	} else if( trootdir != NULL ) {
		if( !find_files( trootdir, files ) )
		{
			std::cerr << format( "cannot include files from '%s'\n", trootdir );
		}

		if( wmprootdir != NULL ) {
			if( !find_files( wmprootdir, files ) )
			{
				std::cerr << format( "cannot include files from '%s'\n", wmprootdir );
			}
		}
	}

	// include system stuff

	std::string path =  "/usr/include";

	if( !find_files( path , files ) )
	{
		std::cerr << format( "cannot include files from '%s'\n", path );
	}


	for( FILE_SEARCH_LIST::iterator it = files.begin(); it != files.end(); it++ )
	{
		if( it->getType() == FILE_TYPE::HEADER )
		{
			ReadFile read_file;
			std::wstring content;
			if( !read_file.read_file( it->getPath(), content ) )
			{
				std::cerr << "cannot read " << it->getPath() << std::endl;
				continue;
			}

			HeaderFile header;

			header.path = HandleFile::in2w(it->getPath());

			CppDir::File file( it->getPath() );
			header.name = HandleFile::in2w(file.get_name());
			header.content = content;

			header_files.push_back( header );
		}
	}
}

void ImplicitHandler::add_default( const std::wstring & symbol, const std::wstring & header_file )
{
	HeaderFile header;
	header.name = header_file;
	header.content = wformat( L"   %s();  ", symbol );
	// DEBUG( format( "content: %s", header.content ));
	header_files.push_back( header );
}

void ImplicitHandler::read_compile_log_line( const std::wstring & line )
{
	// warning: implicit declaration of function ‘tdb_LockRec’

	if( line.find(warning_text) == std::string::npos )
		return;

	ImplicitWarnigs location = get_location_from_line( line );

	std::wstring::size_type start = line.find( L"function" );

	location.symbol = line.substr( start + 8 );
	location.symbol = strip( location.symbol ,  L"'‘’ " );

	// test.c:7:2: warning: incompatible implicit declaration of built-in function ‘memset’ [enabled by default]

	// uebrig bleibt:
	// memset’ [enabled by default]

	std::wstring::size_type end = location.symbol.find_first_of( L"'‘’ ");

	if( end != std::wstring::npos && end != 0 )
	{
		location.symbol = location.symbol.substr( 0, end );
		location.symbol = strip( location.symbol ,  L"'‘’ " );
	}

	location.compile_log_line = line;

	implicit_warnings_locations.push_back( location );

//	DEBUG( format( "%s symbol: %s", location, location.symbol ) );
}

bool ImplicitHandler::want_file( const FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < implicit_warnings_locations.size(); i++ )
	{
		if( implicit_warnings_locations[i].file == file.name ) {
			return true;
		}
	}

	return false;
}

void ImplicitHandler::fix_file( FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < implicit_warnings_locations.size(); i++ )
	{
		if( implicit_warnings_locations[i].file == file.name ) {
			fix_warning( implicit_warnings_locations[i], file.content );
		}
	}
}

void ImplicitHandler::report_unfixed_compile_logs()
{
	for( unsigned i = 0; i < implicit_warnings_locations.size(); i++ )
	{
		if( !implicit_warnings_locations[i].fixed )
		{
			std::cout << "(unfixed) " << HandleFile::w2out(implicit_warnings_locations[i].compile_log_line) << '\n';
		}
	}
}

void ImplicitHandler::fix_warning( ImplicitWarnigs & warning, std::wstring & content )
{
	for( HEADER_FILE::iterator it = header_files.begin(); it != header_files.end(); it++ )
	{
		if( it->path.find(L"cpputils") != std::wstring::npos )
			continue;

		if( is_symbol_in_header_file( it->content, warning.symbol ) )
		{
			bool already_included = false;

			if( insert_include_for( warning, it->name, content, already_included ) )
			{
				warning.fixed = true;
				break;
			} else if( already_included ) {
				warning.fixed = true;
				break;
			} else {
				DEBUG( wformat( L"can't insert include for header '%s' for symbol '%s'", it->name, warning.symbol ) );
			}
		} else {
			// DEBUG( format( "symbol: '%s' not found in header: '%s'", warning.symbol, it->name ));
		}
	}
}

bool ImplicitHandler::is_symbol_in_header_file( const std::wstring & content, const std::wstring & symbol )
{
	for( std::string::size_type pos = content.find( symbol ); pos != std::string::npos; pos = content.find( symbol, pos+1 ) )
	{
		std::wstring line = get_whole_line( content, pos );

		// DEBUG( format( "line: '%s'", line ) );

		if( line.find( L"#define" ) != std::wstring::npos ) {
			continue;
		}

		if( pos == 0 ) {
			continue;
		}

		if( pos+symbol.size()+1+1 >= content.size() ) {
			return false;
		}

		char pos_before = content[pos-1];

		switch( pos_before )
		{
		case L' ': break;
		case L'\t': break;
		case L'*': break;
		case L'\n': break;
		case L'\r': break;
		case L'<': break;
		case L'"': break;
		default:
			// this is no symbol
			continue;
		}

		char pos_after = content[pos+symbol.size()];

		switch( pos_after )
		{
		case L' ': break;
		case L'\t': break;
		case L'\n': break;
		case L'\r': break;
		case L'(': break;
		case L'>': break;
		case L';': break;
		case L'"': break;
		default:
			// this is no symbol
			continue;
		}

		return true;
	}

	// DEBUG( format("return false symbol: '%s' in content '%s'", symbol, content  ));
	return false;
}


bool ImplicitHandler::insert_include_for( const ImplicitWarnigs & warning, const std::wstring & file_name, std::wstring & content, bool & already_included )
{
	if( file_name.empty() )
		return false;

	const std::wstring include_string = wformat(L"#include \"%s\"", file_name);

	DEBUG( wformat( L"%s %s %s", warning, warning.symbol, include_string ) );

	std::string::size_type pos = content.rfind( L"#include" );

	if( pos == std::string::npos )
		return false;

	pos = content.find( L'\n', pos );

	if( pos == std::wstring::npos )
		return false;


	// include bereits vorhanden?
	if( content.find( include_string ) != std::wstring::npos ) {
		already_included = true;
		return false;
	}

	pos++;

	std::wstring left = content.substr( 0, pos );
	std::wstring right = content.substr( pos );

	if( !left.empty() && left[left.size()-1] != '\n')
	{
		left += '\n';
	}

	content = left + include_string + L"\n" + right;

	return true;
}

