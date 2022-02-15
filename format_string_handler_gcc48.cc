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

bool FormatStringHandlerGcc48::is_interested_in_line( const std::wstring & line )
{
	/*
	 * format_string_handler.cc:36 ./h1tcp.cc:821:27: warning: format '%ld' expects argument of type 'long int', but argument 5 has type 'unsigned int' [-Wformat=]
format_string_handler.cc:36     len, sizeof(osi_conn),s);
	 *
	 */
	if( line.find( L"warning: format") == std::wstring::npos ||
	    line.find( L"expects argument of type") == std::wstring::npos ||
	    line.find( L"but argument")  == std::wstring::npos ||
	    line.find( L"has type")  == std::wstring::npos )
	{
		return false;
	}

	return true;
}


void FormatStringHandlerGcc48::strip_target_type( FormatWarnigs & location )
{
	std::wstring::size_type pos = location.target_type.find(L'\'');

	if( pos != std::wstring::npos )
	{
		location.target_type = location.target_type.substr( 0, pos );
	}
}
