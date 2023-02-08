/*
 * fix_from_compile_log.h
 *
 *  Created on: 13.02.2014
 *      Author: martin
 */

#ifndef FIX_FROM_COMPILE_LOG_H_
#define FIX_FROM_COMPILE_LOG_H_

#include <string>
#include <list>
#include <ref.h>
#include <iostream>
#include "find_files.h"

class FixFromCompileLog
{
public:
	struct File
	{
		std::wstring path_name;
		std::wstring name;
		std::wstring content;
		std::wstring original_content;
		std::string encoding;
	};

	class Handler
	{
	public:
		class Location
		{
		public:
			std::wstring file;
			int line;

			Location() : file(), line(0) {}
		};

	public:
		Handler() {}
		virtual ~Handler();

		virtual void read_compile_log_line( const std::wstring & line ) {};
		virtual void fix_file( File & file ) {};

		Location get_location_from_line( const std::wstring & line );

		virtual bool want_file( const File & file ) { return false; }

		virtual void report_unfixed_compile_logs() { };
	};




protected:
	std::string path;
	std::string compile_log;
	bool only_comment_out;
	bool remove_unused_variables;

	typedef std::list< Tools::Ref<Handler> > HANDLER_LIST;
	HANDLER_LIST handlers;
	typedef std::list<File> FILE_LIST;
	FILE_LIST files;
	bool handle_implicit;
	std::set<std::string> directories_to_ignore;
	std::wstring backup_suffix;

public:
	FixFromCompileLog( const std::string & path, // SRCDIR
					   const std::string & compile_log,
				       bool only_comment_out,
				       bool remove_unused_variables,
				       bool initialize_variables_,
				       bool handle_format_strings,
				       bool handle_implicit,
					   bool handle_space_between_literal,
					   const std::set<std::string> & directories_to_ignore,
					   const std::wstring & backup_suffix_ );

	void run();
	void doit();

private:
	void read_compile_log();

	void fix_from_compile_log();

	void report_unfixed_files();

	void show_diffs();
};

inline std::wostream & operator<<( std::wostream & out, const FixFromCompileLog::Handler::Location & location )
{
	out << location.file;
	out << ':';
	out << location.line;

	return out;
}

#endif /* FIX_FROM_COMPILE_LOG_H_ */
