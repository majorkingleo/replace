/*
 * format_string_handler.h
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */

#ifndef FORMAT_STRING_HANDLER_H_
#define FORMAT_STRING_HANDLER_H_

#include "fix_from_compile_log.h"

class FormatStringHandler : public FixFromCompileLog::Handler
{
	class FormatWarnigs : public Location
	{
	public:
		std::string format;
		int argnum;
		std::string expected_type;
		std::string target_type;
		std::string compile_log_line;
		bool fixed;

	public:
		FormatWarnigs( const Location & location )
		: Location( location ),
		  format(),
		  argnum(0),
		  expected_type(),
		  target_type(),
		  compile_log_line(),
		  fixed(false)
		{}
	};

	class FixTable
	{
	public:
		std::string format;
		std::string target_type;
		std::string correct_type;

		FixTable( const std::string & format_,
				  const std::string & target_type_,
				  const std::string & correct_type_ )
		: format( format_ ),
		  target_type( target_type_ ),
		  correct_type( correct_type_ )
		{}
	};

	std::vector<FormatWarnigs > format_warnings_locations;
	std::vector<FixTable> fix_table;

public:
	FormatStringHandler();

	virtual void read_compile_log_line( const std::string & line );

	virtual bool want_file( const FixFromCompileLog::File & file );

	virtual void fix_file( FixFromCompileLog::File & file );

	virtual void report_unfixed_compile_logs();

	virtual void fix_warning( FormatWarnigs & warning, std::string & content );
};


#endif /* FORMAT_STRING_HANDLER_H_ */
