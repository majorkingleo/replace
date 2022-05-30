#include "OutDebug.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <DetectLocale.h>

OutDebug::OutDebug(  ColoredOutput::Color color_ )
: Debug(),
  ColoredOutput(),
  color( color_ ),
  prefix(),
  print_line_and_file_info( true )
{

}


void OutDebug::add( const char *file, unsigned line, const char *function, const std::string & s )
{
	if( print_line_and_file_info ) {
		std::cout << color_output( color, file );
		std::cout << ':' << line
				  << " ";
	}

	if( !prefix.empty() ) {
		 std::cout << color_output( color, DetectLocale::w2out(prefix) );
		 std::cout << " ";
	}

	std::cout << s << '\n';
}

void OutDebug::add( const char *file, unsigned line, const char *function, const std::wstring & s )
{
	add( file, line, function,  DetectLocale::w2out(s) );
}
