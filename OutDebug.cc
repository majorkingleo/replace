#include "OutDebug.h"
#include <iostream>

OutDebug::OutDebug()
: Debug()
{

}


void OutDebug::add( const char *file, unsigned line, const char *function, const std::string & s )
{
	std::cout << file
			<< ':' << line
			<< " " // << function
			<< s
			<< '\n';
}

