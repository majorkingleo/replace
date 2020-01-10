/*
 * find_decl.cc
 *
 *  Created on: 10.01.2020
 *      Author: martin
 */
#include "find_decl.h"
#include <string_utils.h>
#include <format.h>
#include <debug.h>
#include "utils.h"

using namespace Tools;

// die Funktion sucht rueckwaerts!
std::string find_decl( const std::string &s,
				      std::string::size_type start_,
				      const std::string & name,
				      std::string & decl,
				      std::string::size_type & at_pos )
{
	DEBUG( format( "trying to find decl for '%s'", name));

	std::string ret;

	if( name == "NULL" )
		return ret;

	for( std::string::size_type start = start_; start > 0 && start != std::string::npos; start-- )
	{
		std::string::size_type pos = s.rfind( name, start );

		if( pos == std::string::npos )
			return ret;

		if( s.size() < pos + name.size() )
		  continue;

		if( isalnum( s[pos+name.size()] ) || s[pos+name.size()] == '_' )
		  {
			start = pos-1;
			continue;
		  }

		if( pos > 0 )
		  {
			if( ( !isspace( s[pos-1] ) && s[pos-1] != ',' && s[pos-1] != '*' ) || is_in_string( s, pos ) )
			  {
				start = pos-1;
				continue;
			  }
		  }

		bool is_decl = false;

		// kann das eine Dekleration sein?
		for( std::string::size_type p = pos - 1; p > 0; p-- )
		{
			if( isspace( s[p] ) )
				continue;

			// log( format("char. %c", s[p] ) );

			if( s[p] == ',' || s[p] == '{' || s[p] == '*' || isalnum( s[p] ) )
			{
				is_decl = true;
				break;
			}
			break;
		}

		if( !is_decl )
		{
			start = pos;
			continue;
		}

		// Dekleration extrahieren
		at_pos = pos;
		std::string::size_type pos2 = s.find_first_of( ",;=)", pos );

		decl = s.substr( pos, pos2 - pos + 1);

		//std::cout << "decl: " << decl << std::endl;

		// bis zum naechsten ';' oder '{' suchen
		start = pos;

		for( start = pos; start > 0; start-- )
		{
			if( s[start] == ';' || s[start] == '{' || s[start] == '(' )
				break;
		}

		if( pos > start_ )
		  {
			start = start_;
			DEBUG( format( "connot find type of %s", name ) );
			return ret; // hat keinen Sinn mehr weiterzusuchen
			// continue;
		  }

		if( s[start] == '(' )
		{
			// wir sind in einer Funktion
			start = pos;
			for( start = pos; start > 0; start-- )
			{
				if( s[start] == ',' || s[start] == '(' )
					break;
			}
		}

		if( pos > start_ )
		  {
			start = start_;
			DEBUG( format( "connot find type of %s", name ) );
			return ret; // hat keinen Sinn mehr weiterzusuchen
			// continue;
		  }

		// ok, und nun vorwaerts suchen, dann muessten wir den Typ haben
		std::string::size_type end = s.find_first_of( ",;)", start+1 );

		if( end == std::string::npos )
		{
		  DEBUG( format( "connot find type of %s", name )  );
			return ret;
		}

		if( is_in_string( s, start ) )
		{
			start--;
			continue;
		}

		std::string cast = s.substr( start, end - (start-1) );

	if( cast.size() && cast[0] == '(' && cast[cast.size()-1] == ')' )
	  {
		DEBUG( format( "ignoring cast type: %s for var %s", cast, name ) );
		start--;
		continue;
	  }

	/*
		// wenn FOO foo steht, dann muss noch ein split her
		std::vector<std::string> sl = split_simple( s.substr( start+1, end - (start+1) ) );

		if( sl.size() && sl[0].find_first_of( "->.=[]{}," ) == std::string::npos )
		{
		  if( have_struct( sl[0] ) )
		  {
			  // log( format( "found type %s at %d", sl[0], get_linenum( s, start ) ) );
			  return sl[0];
		  } else {
			// std::cout << format( "not a type '%s'\n", sl[0] );
		  }
		}
	*/

	std::string f = strip( s.substr( start+1, end - (start+1) ) );

	// static rausschneiden
	if( f.find( "static" ) == 0 )
	{
		f = strip( f.substr( 6 ) );
	}

	if( f.find( "unsigned char" ) == 0 )
	  {
	    f = "char";
	  }

	if( f.find("const") == 0 ) {
		DEBUG( format( "const format detected: '%s'", f ) );

		if( f.size() > 6 ) {
			std::string::size_type pp = skip_spaces( f, 5 );

			if( pp != std::string::npos )
			{
				f = f.substr( pp);
				DEBUG( format( "removed const: '%s'", f ) );
			}
		}
	}

	std::string::size_type pp = f.find_first_of( " \t\n" );

	if( pp != std::string::npos )
	  {
		f.resize( pp );
	  }

	/* kann vorkommen, wenn da ein Funktionaufruf ist
	 * zb:
	 * char Foo[100];
	 *   Bar( Foo ); // <- wir finden dann das da
	 *   sprintf( "%s", Foo );
	 */
	if( f == name )
	{
		start = pos-1;
		continue;
	}

	if( f.find_first_of( "->.=[]{}," ) == std::string::npos )
	  {
		/*
		  DEBUG( log( format( "%s is of type %s, but what kind of stuff is that?", name, f)) );
		  if( have_struct( f ) )
		  {
			return f;
		  }
		  */
	  }

	} // for

	DEBUG( format( "connot find type of %s", name ) );

	return ret;
}



