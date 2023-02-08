/*
 * ColoredOutput.cc
 *
 *  Created on: 27.01.2023
 *      Author: Martin Oberzalek <oberzalek@gmx.at>
 */
#include <sstream>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>
#include "ColoredOutput.h"
#ifdef _WIN32
#	include <windows.h>
#else
#	include <unistd.h>
#endif

ColoredOutput::ColoredOutput()
: colored_output( false )
{
#ifdef _WIN32
	// https://learn.microsoft.com/en-us/windows/console/console-virtual-terminal-sequences
	// Set output mode to handle virtual terminal sequences
	HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	if (hOut == INVALID_HANDLE_VALUE) {
		return;
	}

	DWORD dwMode = 0;
	if (!GetConsoleMode(hOut, &dwMode)) {
		return;
	}

	dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	if (!SetConsoleMode(hOut, dwMode)) {
		return;
	}

	colored_output = true;
#else
	colored_output = true;
	char *pcTerm =  getenv( "TERM");
	
	if( pcTerm == NULL || !isatty(fileno(stdout)) ) {
		colored_output = false;
	}
#endif
}

std::string ColoredOutput::color_output( Color color, const std::string & text ) const
{
	if( !colored_output ) {
		return text;
	}

	std::stringstream str;

	switch( color )
	{
	case BLACK:              str << "\033[0;30m"; break;
	case RED:                str << "\033[0;31m"; break;
	case GREEN:              str << "\033[0;32m"; break;
	case YELLOW:             str << "\033[0;33m"; break;
	case BLUE:               str << "\033[0;34m"; break;
	case MAGENTA:            str << "\033[0;35m"; break;
	case CYAN:               str << "\033[0;36m"; break;
	case WHITE:              str << "\033[0;37m"; break;
	case BRIGHT_BLACK:       str << "\033[1;30m"; break;
	case BRIGHT_RED:         str << "\033[1;31m"; break;
	case BRIGHT_GREEN:       str << "\033[1;32m"; break;
	case BRIGHT_YELLOW:      str << "\033[1;33m"; break;
	case BRIGHT_BLUE:        str << "\033[1;34m"; break;
	case BRIGHT_MAGENTA:     str << "\033[1;35m"; break;
	case BRIGHT_CYAN:        str << "\033[1;36m"; break;
	case BRIGHT_WHITE:		 str << "\033[1;37m"; break;

	default:
		return text;
	}

	str << text;

	str << "\033[0m";

	return str.str();
}
