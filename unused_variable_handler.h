/*
 * unused_variable_handler.h
 *
 *  Created on: 13.02.2014
 *      Author: martin
 */

#ifndef UNUSED_VARIABLE_HANDLER_H_
#define UNUSED_VARIABLE_HANDLER_H_

#include "fix_from_compile_log.h"
#include <vector>

class UnusedVariableHandler : public FixFromCompileLog::Handler
{
protected:
	class UnusedVarWarnigs : public Location
	{
	public:
		std::wstring var_name;
		std::wstring compile_log_line;
		bool fixed;

	public:
		UnusedVarWarnigs( const Location & location )
		: Location( location ),
		  var_name(),
		  compile_log_line(),
		  fixed(false)
		{}

	};

	std::vector<UnusedVarWarnigs > unused_variables_locations;
	bool comment_only;

public:
	UnusedVariableHandler( bool comment_only );

	void read_compile_log_line( const std::wstring & line ) override;

	bool want_file( const FixFromCompileLog::File & file ) override;

	void fix_file( FixFromCompileLog::File & file ) override;

	void report_unfixed_compile_logs() override;

	virtual void fix_warning( UnusedVarWarnigs & warning, std::wstring & content );

	static void replace_line( std::wstring & buffer, std::wstring::size_type pos, const std::wstring & new_line );

	static std::wstring::size_type find_var_name( const std::wstring line, const std::wstring & var_name );
};


#endif /* UNUSED_VARIABLE_HANDLER_H_ */
