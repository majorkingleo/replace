/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */

#include "regex_match.h"
#include <iostream>
#include <string>
#include <boost/regex.hpp>  // Boost.Regex lib
#include <DetectLocale.h>

using namespace std;

bool regex_match( std::string regex, std::string s )
{
 boost::regex re;
 
 re.assign( regex, boost::regex_constants::icase );

 if( boost::regex_match(s, re) ) {
   return true;
 }

 return false;
}

#ifndef BOOST_NO_WREGEX
bool regex_match( std::wstring regex, std::wstring s )
{
 boost::wregex re;

 re.assign( regex, boost::regex_constants::icase );

 if( boost::regex_match(s, re) ) {
   return true;
 }

 return false;
}
#else
bool regex_match( std::wstring regex, std::wstring s )
{
	return regex_match( DETECT_LOCALE.w2out(regex), DETECT_LOCALE.w2out(s) );
}
#endif

