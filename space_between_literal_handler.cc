/*
 * space_between_literal_handler.cc
 *
 *  Created on: 29.05.2019
 *      Author: martin
 */

#include "space_between_literal_handler.h"
#include <string_utils.h>
#include <debug.h>
#include "getline.h"
#include <cpp_util.h>
#include "unused_variable_handler.h"

using namespace Tools;

SpaceBetweenLiteralHandler::SpaceBetweenLiteralHandler()
	: locations()
{

}

void SpaceBetweenLiteralHandler::read_compile_log_line( const std::string & line )
{
	if( line.find( "-Wliteral-suffix") == std::string::npos )
		return;

	SpaceBetweenLiteralWarnings location = get_location_from_line( line );

	std::vector<std::string> sl = split_simple( line, " ");

	std::vector<std::string>::reverse_iterator it = sl.rbegin();

	// GCC 4.8 style: warning: unused variable 'dbrv' [-Wunused-variable]

	if( *it->begin() == '[' && *it->rbegin() == ']' ) {
		it++;
	}

	if( it == sl.rend() ) {
		return;
	}

	location.var_name = *it;
	location.var_name = strip( location.var_name, "'‘’");
	location.compile_log_line = line;

	DEBUG( format( "%s %s", location, location.var_name ) );

	locations.push_back( location );
}

bool SpaceBetweenLiteralHandler::want_file( const FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( locations[i].file == file.name ) {
			return true;
		}
	}

	return false;
}

void SpaceBetweenLiteralHandler::fix_file( FixFromCompileLog::File & file )
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( locations[i].file == file.name ) {
			fix_warning( locations[i], file.content );
		}
	}
}

void SpaceBetweenLiteralHandler::report_unfixed_compile_logs()
{
	for( unsigned i = 0; i < locations.size(); i++ )
	{
		if( !locations[i].fixed )
		{
			std::cout << "(unfixed) " << locations[i].compile_log_line << '\n';
		}
	}
}




bool SpaceBetweenLiteralHandler::is_escaped( const std::string &s, std::string::size_type start )
{
  bool escaped = false;

  if( start > 0 )
    {
      if( s[start-1] == '\\' )
	{
	  escaped = true;

	  if( start > 1 )
	    {
	      if( s[start-2] == '\\' )
		{
		  escaped = false;
		}
	      }
	}
    }

  return escaped;
}

std::vector<SpaceBetweenLiteralHandler::Pair> SpaceBetweenLiteralHandler::find_exclusive( const std::string &s )
{
  std::vector<Pair> ex;

  std::string::size_type pos = 0, start = 0, end = 0;

  while( true )
    {

      /* find starting pos */
      while( true )
	{
	  start = s.find( '"', pos );

	  if( start == std::string::npos )
	    return ex;

	  /* is the " escaped? */
	  if( is_escaped( s, start ) )
	    {
	      pos = start + 1;
	      continue;
	    }

	  break;
	}

      // find second "
      pos = start + 1;
      while( true )
	{
	  end = s.find( '"', pos );

	  if( end == std::string::npos )
	    return ex;

	  if( is_escaped( s, end ) )
	    {
	      pos = end + 1;
	      continue;
	    }

	  break;
	}

      ex.push_back( Pair( start, end ) );
      pos = end + 1;
    }
}

bool SpaceBetweenLiteralHandler::is_exclusive( const std::string &s, const std::vector<SpaceBetweenLiteralHandler::Pair> &exclude, std::string::size_type pos1 )
{
  bool exclusive = false;
  for( unsigned i = 0; i < exclude.size(); i++ )
    {
      if( exclude[i].start < pos1 &&
	  exclude[i].end > pos1 )
        {
	  exclusive = true;
	  break;
	}
    }

  return exclusive;
}


void SpaceBetweenLiteralHandler::fix_warning( SpaceBetweenLiteralWarnings & warning, std::string & content )
{
	std::string::size_type pos = get_pos_for_line( content, warning.line );

	if( pos == std::string::npos ) {
		throw REPORT_EXCEPTION( format( "can't get file position for warning %s", warning.compile_log_line));
	}

	std::string line = getline(content,  pos );

	DEBUG( line );

	std::vector<Pair> exclude = find_exclusive( line );

	std::string::size_type pos1 = 0;
	std::string::size_type last_pos = 0;

	bool string_started = false;

	std::stringstream res;

	do
	{
		pos1 = line.find( '"', pos1 );

		if( pos1 == std::string::npos ) {
			break;
		}

		if( is_exclusive( line, exclude, pos1 ) )
		{
			pos1++;
			continue;
		}

		if( !string_started ) {
			string_started = true;
			res << line.substr( last_pos, pos1 - last_pos + 1);
			last_pos = pos1+1;
		} else {
			// jetzt einen space einbauen
			res << line.substr( last_pos, pos1 - last_pos + 1)
				<< ' ';
			last_pos = pos1+1;
			string_started = false;
		}

		pos1++;

	} while( pos1 != std::string::npos );

	res << line.substr( last_pos );

	std::string new_line = res.str();

	UnusedVariableHandler::replace_line( content, pos, new_line );

	warning.fixed = true;
}



