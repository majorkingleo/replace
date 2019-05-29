/*
 * implicit_handler2.cc
 *
 *  Created on: 24.01.2018
 *      Author: martin
 */


#include "implicit_handler2.h"

ImplicitHandler2::ImplicitHandler2( const std::string & srcdir_ )
: ImplicitHandler( srcdir_ )
{
	warning_text = "incompatible implicit declaration of built-in function";
}
