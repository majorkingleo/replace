/*
 * implicit_handler.h
 *
 *  Created on: 18.02.2014
 *      Author: martin
 */

#ifndef IMPLICIT_HANDLER_H_
#define IMPLICIT_HANDLER_H_

#include <vector>
#include "fix_from_compile_log.h"

class ImplicitHandler : public FixFromCompileLog::Handler
{
protected:
	class ImplicitWarnigs : public Location
	{
	public:
		std::string symbol;
		std::string compile_log_line;
		bool fixed;

	public:
		ImplicitWarnigs( const Location & location )
		: Location( location ),
		  symbol(),
		  compile_log_line(),
		  fixed(false)
		{}
	};

	class HeaderFile
	{
	public:
		std::string name;
		std::string path;
		std::string content;
	};

	std::vector<ImplicitWarnigs > implicit_warnings_locations;
	std::string srcdir;

	typedef std::list<HeaderFile> HEADER_FILE;
	HEADER_FILE header_files;

	std::map<std::string,std::string> symbol_header_file_map;

public:
	ImplicitHandler( const std::string & srcdir );

	virtual void read_compile_log_line( const std::string & line );

	virtual bool want_file( const FixFromCompileLog::File & file );

	virtual void fix_file( FixFromCompileLog::File & file );

	virtual void report_unfixed_compile_logs();

	virtual void fix_warning( ImplicitWarnigs & warning, std::string & content );

private:
	void search_for_header_files();

	bool is_symbol_in_header_file( const std::string & content, const std::string & symbol );

	bool insert_include_for( const ImplicitWarnigs & warning, const std::string & file_name, std::string & content, bool & already_included );
};



#endif /* IMPLICIT_HANDLER_H_ */
