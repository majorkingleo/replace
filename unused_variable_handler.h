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
	class UnusedVarWarnigs : public Location
	{
	public:
		std::string var_name;
		std::string compile_log_line;
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

	virtual void read_compile_log_line( const std::string & line );

	virtual bool want_file( const FixFromCompileLog::File & file );

	virtual void fix_file( FixFromCompileLog::File & file );

	virtual void report_unfixed_compile_logs();

	void fix_warning( UnusedVarWarnigs & warning, std::string & content );

	void replace_line( std::string & buffer, std::string::size_type pos, const std::string & new_line );
};


#endif /* UNUSED_VARIABLE_HANDLER_H_ */
