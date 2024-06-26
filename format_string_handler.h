/*
 * format_string_handler.h
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */

#ifndef FORMAT_STRING_HANDLER_H_
#define FORMAT_STRING_HANDLER_H_

#include <vector>
#include "fix_from_compile_log.h"

class FormatStringHandler : public FixFromCompileLog::Handler
{
protected:
	class FormatWarnigs : public Location
	{
	public:
		std::wstring format;
		int argnum;
		std::wstring expected_type;
		std::wstring target_type;
		std::wstring compile_log_line;
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
		std::wstring format;
		std::wstring target_type;
		std::wstring correct_type;

		FixTable( const std::wstring & format_,
				  const std::wstring & target_type_,
				  const std::wstring & correct_type_ )
		: format( format_ ),
		  target_type( target_type_ ),
		  correct_type( correct_type_ )
		{}
	};

	std::vector<FormatWarnigs > format_warnings_locations;
	std::vector<FixTable> fix_table;

public:
	FormatStringHandler();

	virtual void read_compile_log_line( const std::wstring & line );

	virtual bool want_file( const FixFromCompileLog::File & file );

	virtual void fix_file( FixFromCompileLog::File & file );

	virtual void report_unfixed_compile_logs();

	virtual void fix_warning( FormatWarnigs & warning, std::wstring & content );

protected:
	virtual bool is_interested_in_line( const std::wstring & line );

	virtual void strip_target_type( FormatWarnigs & location ) {}
};


#endif /* FORMAT_STRING_HANDLER_H_ */
