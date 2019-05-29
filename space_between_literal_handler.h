/*
 * space_between_literal_handler.h
 *
 *  Created on: 29.05.2019
 *      Author: martin
 */

#ifndef SPACE_BETWEEN_LITERAL_HANDLER_H_
#define SPACE_BETWEEN_LITERAL_HANDLER_H_

#include "fix_from_compile_log.h"
#include <vector>

class SpaceBetweenLiteralHandler : public FixFromCompileLog::Handler
{
public:
	struct Pair
	{
	  std::string::size_type start;
	  std::string::size_type end;

	  Pair( std::string::size_type start_,   std::string::size_type end_ )
	    : start( start_ ) , end( end_ )
	  {}
	};

protected:
	class SpaceBetweenLiteralWarnings : public Location
	{
	public:
		std::string var_name;
		std::string compile_log_line;
		bool fixed;

	public:
		SpaceBetweenLiteralWarnings( const Location & location )
		: Location( location ),
		  var_name(),
		  compile_log_line(),
		  fixed(false)
		{}

	};

	std::vector<SpaceBetweenLiteralWarnings > locations;

public:
	SpaceBetweenLiteralHandler();

	virtual void read_compile_log_line( const std::string & line );

	virtual bool want_file( const FixFromCompileLog::File & file );

	virtual void fix_file( FixFromCompileLog::File & file );

	virtual void report_unfixed_compile_logs();

	virtual void fix_warning( SpaceBetweenLiteralWarnings & warning, std::string & content );

protected:
	static bool is_escaped( const std::string &s, std::string::size_type start );
	static std::vector<Pair> find_exclusive( const std::string &s );
	static bool is_exclusive( const std::string &s, const std::vector<Pair> &exclude, std::string::size_type pos1 );
};


#endif /* SPACE_BETWEEN_LITERAL_HANDLER_H_ */
