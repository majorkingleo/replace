/*
 * implicit_handler.cc
 *
 *  Created on: 18.02.2014
 *      Author: martin
 */

#include "implicit_handler.h"
#include "cpp_utils.h"
#include "debug.h"
#include "getline.h"
#include "utils.h"
#include "find_files.h"
#include "cppdir.h"
#include "xml.h"

using namespace Tools;

ImplicitHandler::ImplicitHandler( const std::string & srcdir_ )
: FixFromCompileLog::Handler(),
  implicit_warnings_locations(),
  srcdir(srcdir_),
  header_files(),
  symbol_header_file_map()
{
	search_for_header_files();

}

void ImplicitHandler::search_for_header_files()
{
	std::vector<std::pair<FILE_TYPE,std::string> > files;

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


	for( unsigned i = 0; i < files.size(); i++ )
	{
		if( files[i].first == FILE_TYPE::HEADER )
		{
			std::string content;
			if( !XML::read_file( files[i].second, content ) )
			{
				std::cerr << "cannot read " << files[i].second << std::endl;
				continue;
			}

			HeaderFile header;

			header.path = files[i].second;

			CppDir::File file( header.path );
			header.name = file.get_name();
			header.content = content;

			header_files.push_back( header );
		}
	}

}

void ImplicitHandler::read_compile_log_line( const std::string & line )
{
	// warning: implicit declaration of function ‘tdb_LockRec’

	if( line.find( "warning: implicit declaration of function") == std::string::npos )
		return;

	ImplicitWarnigs location = get_location_from_line( line );

	std::string::size_type start = line.find( "function" );

	location.symbol = line.substr( start + 8 );
	location.symbol = strip( location.symbol ,  "'‘’ " );
	location.compile_log_line = line;

	implicit_warnings_locations.push_back( location );

	DEBUG( format( "%s symbol: %s", location, location.symbol ) );
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
			std::cout << "(unfixed) " << implicit_warnings_locations[i].compile_log_line << '\n';
		}
	}
}

void ImplicitHandler::fix_warning( ImplicitWarnigs & warning, std::string & content )
{
	for( HEADER_FILE::iterator it = header_files.begin(); it != header_files.end(); it++ )
	{
		if( it->path.find("cpputils") != std::string::npos )
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
			}
		}
	}
}

bool ImplicitHandler::is_symbol_in_header_file( const std::string & content, const std::string & symbol )
{
	for( std::string::size_type pos = content.find( symbol ); pos != std::string::npos; pos = content.find( symbol, pos+1 ) )
	{
		std::string line = get_whole_line( content, pos );

		if( line.find( "#define" ) != std::string::npos ) {
			continue;
		}

		if( pos == 0 ) {
			continue;
		}

		if( pos+symbol.size()+1+1 >= content.size() )
			return false;

		char pos_before = content[pos-1];

		switch( pos_before )
		{
		case ' ': break;
		case '\t': break;
		case '*': break;
		case '\n': break;
		case '\r': break;
		default:
			// this is no symbol
			continue;
		}

		char pos_after = content[pos+symbol.size()];

		switch( pos_after )
		{
		case ' ': break;
		case '\t': break;
		case '\n': break;
		case '\r': break;
		case '(': break;
		case ';': break;
		default:
			// this is no symbol
			continue;
		}

		return true;
	}

	return false;
}


bool ImplicitHandler::insert_include_for( const ImplicitWarnigs & warning, const std::string & file_name, std::string & content, bool & already_included )
{
	if( file_name.empty() )
		return false;

	const std::string include_string = format("#include \"%s\"", file_name);

	DEBUG( format( "%s %s %s", warning, warning.symbol, include_string ) );

	std::string::size_type pos = content.rfind( "#include" );

	if( pos == std::string::npos )
		return false;

	pos = content.find( '\n', pos );

	if( pos == std::string::npos )
		return false;


	// include bereits vorhanden?
	if( content.find( include_string ) != std::string::npos ) {
		already_included = true;
		return false;
	}

	pos++;

	std::string left = content.substr( 0, pos );
	std::string right = content.substr( pos );

	if( !left.empty() && left[left.size()-1] != '\n')
	{
		left += '\n';
	}

	content = left + include_string + "\n" + right;

	return true;
}

