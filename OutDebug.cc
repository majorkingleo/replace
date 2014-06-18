#include "OutDebug.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

OutDebug::OutDebug()
: Debug(),
  colored_output(true)
{
	char *pcTerm =  getenv( "TERM");

	if( pcTerm == NULL || !isatty(fileno(stdout)))
	{
		colored_output = false;
	}
}


void OutDebug::add( const char *file, unsigned line, const char *function, const std::string & s )
{
	if( colored_output )
	{
		std::cout << "\033[1;33m";
	}

	std::cout << file;

	if( colored_output )
	{
		std::cout << "\033[0m";
	}

	std::cout << ':' << line
			<< " " // << function
			<< s
			<< '\n';
}

