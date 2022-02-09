/*
 * DetectLocale.cc
 *
 *  Created on: 08.02.2022
 *      Author: martin
 */

#include "DetectLocale.h"
#include <locale>

DetectLocale::DetectLocale()
: is_utf8( false )
{

}

void DetectLocale::init()
{
	char *pcLocale = std::setlocale( LC_MESSAGES, "" );

	if( pcLocale == NULL ) {
		is_utf8 = false;
		return;
	}

	std::string locale = pcLocale;

	if( locale.find( "UTF-8" ) != std::string::npos ) {
		is_utf8 = true;
	}
}
