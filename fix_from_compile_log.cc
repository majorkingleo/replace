/*
 * fix_from_compile_log.cc
 *
 *  Created on: 13.02.2014
 *      Author: martin
 */


#include "fix_from_compile_log.h"
#include <iostream>
#include <xml.h>
#include <cpp_utils.h>
#include <errno.h>
#include "unused_variable_handler.h"
#include <iterator.h>
#include <cppdir.h>
#include "getline.h"
#include <stdio.h>
#include <fstream>
#include "debug.h"
#include <set>
#include "uninitialized_variable_handler.h"
#include "format_string_handler.h"
#include "format_string_handler_gcc48.h"
#include "implicit_handler.h"
#include "implicit_handler2.h"
#include "space_between_literal_handler.h"

using namespace Tools;

FixFromCompileLog::Handler::~Handler()
{

}

FixFromCompileLog::Handler::Location FixFromCompileLog::Handler::get_location_from_line( const std::string & line )
{
	std::vector<std::string> sl = split_simple( line, ":");

	if( sl.size() < 2 ) {
		throw REPORT_EXCEPTION( format("cannot extract location from line: '%s'", line ) );
	}

	Location location;

	location.file = sl[0];
	location.line = s2x<int>(sl[1],-1);

	if( location.line == -1 ) {
		throw REPORT_EXCEPTION( format("cannot extract location from line: '%s'", line ) );
	}

	CppDir::File file( location.file );

	location.file = file.get_name();

	return location;
}


FixFromCompileLog::FixFromCompileLog( const std::string & path_,
									  const std::string & compile_log_,
									  bool only_comment_out_,
									  bool remove_unused_variables_,
									  bool initialize_variables_,
									  bool handle_format_strings,
									  bool handle_implicit_,
									  bool handle_space_between_literal )
: path( path_ ),
  compile_log( compile_log_ ),
  only_comment_out( only_comment_out_ ),
  remove_unused_variables(remove_unused_variables_),
  handlers(),
  files(),
  handle_implicit(handle_implicit_)
{

	if( remove_unused_variables_ ) {
		handlers.push_back( new UnusedVariableHandler( only_comment_out ) );
	}

	if( initialize_variables_ ) {
		handlers.push_back( new UninitializedVariableHandler() );
	}

	if( handle_format_strings ) {
		handlers.push_back( new FormatStringHandler() );
		handlers.push_back( new FormatStringHandlerGcc48() );
	}

	if( handle_implicit ) {
			handlers.push_back( new ImplicitHandler(path) );
			handlers.push_back( new ImplicitHandler2(path) );
	}

	if( handle_space_between_literal ) {
			handlers.push_back( new SpaceBetweenLiteralHandler() );
	}
}

void FixFromCompileLog::run()
{
	read_compile_log();

	FILE_SEARCH_LIST all_files;

	if( !find_files( path, all_files ) )
	{
		throw REPORT_EXCEPTION( format("no .c or .cc files found at %s", path ) );
	}

	std::set<std::string> file_names;
	std::set<std::string> already_warned_file_names;

	for( FILE_SEARCH_LIST::iterator fit = all_files.begin(); fit != all_files.end(); fit++ )
	{
		if(  fit->getType() != FILE_TYPE::C_FILE )
			continue;

		File file;

		file.path_name = fit->getPath();
		CppDir::File f( file.path_name );
		file.name = f.get_name();

		if( file_names.find(file.name) != file_names.end() ) {

			if(already_warned_file_names.find(file.name) == already_warned_file_names.end() )
			{
				std::cout << format("warning duplicate filename %s detected. Can't be automatically fixed.\n", file.name );
				already_warned_file_names.insert( file.name );
			}

			for( FILE_LIST::iterator it = files.begin(); it != files.end(); it++ ) {
				if( it->name == file.name ) {
					files.erase(it);
					break;
				}
			}

			continue;
		}

		file_names.insert( file.name );

		files.push_back( file );
	}

	fix_from_compile_log();

	if( !handle_implicit )
		show_diffs();

	report_unfixed_files();
}

void FixFromCompileLog::read_compile_log()
{
	std::string content;

	if( !XML::read_file( compile_log, content ) )
	{
		throw REPORT_EXCEPTION( format("cannot open file %s for reading", compile_log, strerror(errno)) );
	}

	std::vector<std::string> lines = split_simple( content, "\n");

	for( Iterator<HANDLER_LIST::iterator> it = handlers.begin(); it != handlers.end(); it++ )
	{
		for( unsigned  i = 0; i < lines.size(); i++ )
		{
			it->read_compile_log_line( lines[i] );
		}
	}
}


void FixFromCompileLog::fix_from_compile_log()
{
	for( FILE_LIST::iterator fit = files.begin(); fit != files.end(); fit++ )
	{
		for( Iterator<HANDLER_LIST::iterator> it = handlers.begin(); it != handlers.end(); it++ )
		{
			if( it->want_file( *fit ) )
			{
				if( fit->content.empty() )
				{
					if( !XML::read_file( fit->path_name, fit->content ) )
					{
						std::cerr << format("cannot open file %s for reading (%s).\n", fit->path_name, strerror(errno) );
						continue;
					}

					fit->original_content = fit->content;
				}

				it->fix_file( *fit );
			}
		}
	}
}

void FixFromCompileLog::report_unfixed_files()
{
	for( Iterator<HANDLER_LIST::iterator> it = handlers.begin(); it != handlers.end(); it++ )
	{
		it->report_unfixed_compile_logs();
	}
}

void FixFromCompileLog::show_diffs()
{
	for( FILE_LIST::iterator fit = files.begin(); fit != files.end(); fit++ )
	{
		if( fit->content != fit->original_content ) {

			std::cout << "patching file " << fit->path_name << std::endl;
			DEBUG( diff_lines( fit->original_content,  fit->content  ) );
		}
	}
}

void FixFromCompileLog::doit()
{
	for( FILE_LIST::iterator fit = files.begin(); fit != files.end(); fit++ )
	{
		if( fit->content != fit->original_content ) {

			if( rename( fit->path_name.c_str(), TO_CHAR(fit->path_name + ".save") ) != 0 )
			{
				std::cerr << strerror(errno) << std::endl;
			}

			std::ofstream out( fit->path_name.c_str(), std::ios_base::trunc );

			if( !out )
			{
				std::cerr << "cannot overwrite file " << fit->path_name << std::endl;
				continue;
			}

			out << fit->content;

			out.close();
		}
	}
}
