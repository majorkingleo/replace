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
		std::string path_name;
		std::string name;
		std::string content;
		std::string original_content;
	};

	class Handler
	{
	public:
		class Location
		{
		public:
			std::string file;
			int line;

			Location() : file(), line(0) {}
		};

	public:
		Handler() {}
		virtual ~Handler();

		virtual void read_compile_log_line( const std::string & line ) = 0;
		virtual void fix_file( File & file ) = 0;

		Location get_location_from_line( const std::string & line );

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

public:
	FixFromCompileLog( const std::string & path, // SRCDIR
					   const std::string & compile_log,
				       bool only_comment_out,
				       bool remove_unused_variables );

	void run();
	void doit();

private:
	void read_compile_log();

	void fix_from_compile_log();

	void report_unfixed_files();

	void show_diffs();
};

inline std::ostream & operator<<( std::ostream & out, const FixFromCompileLog::Handler::Location & location )
{
	out << location.file;
	out << ':';
	out << location.line;

	return out;
}

#endif /* FIX_FROM_COMPILE_LOG_H_ */
