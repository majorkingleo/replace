/*
 * format_string_handler_gcc48.cc
 *
 *  Created on: 04.07.2016
 *      Author: martin
 */
#include "format_string_handler_gcc48.h"



FormatStringHandlerGcc48::FormatStringHandlerGcc48()
: FormatStringHandler()
{

}

bool FormatStringHandlerGcc48::is_interested_in_line( const std::string & line )
{
	/*
	 * format_string_handler.cc:36 ./h1tcp.cc:821:27: warning: format '%ld' expects argument of type 'long int', but argument 5 has type 'unsigned int' [-Wformat=]
format_string_handler.cc:36     len, sizeof(osi_conn),s);
	 *
	 */
	if( line.find( "warning: format") == std::string::npos ||
	    line.find( "expects argument of type") == std::string::npos ||
	    line.find( "but argument")  == std::string::npos ||
	    line.find( "has type")  == std::string::npos )
	{
		return false;
	}

	return true;
}


void FormatStringHandlerGcc48::strip_target_type( FormatWarnigs & location )
{
	std::string::size_type pos = location.target_type.find('\'');

	if( pos != std::string::npos )
	{
		location.target_type = location.target_type.substr( 0, pos );
	}
}
