#include <cassert>
#include <vector>
#include <iostream>

#include "utils.h"
#include "pairs.h"
#include "string_utils.h"
#include "cppdir.h"
#include "xml.h"
#include "debug.h"

using namespace Tools;

bool is_in( char c, const std::string &values )
{
    for( std::string::size_type i = 0; i < values.size(); i++ )
    {
		if( c == values[i] )
			return true;
    }

    return false;
}

unsigned get_linenum( const std::string &s, std::string::size_type pos )
{
    unsigned ret = 1;
    std::string::size_type p = 0;
    
    while( ( p = s.find( '\n', p ) ) != std::string::npos &&
		   pos > p )
    {
		ret++;
		p++;
    }

    return ret;
}

template <class T> bool is_define_line( const T &s, std::string::size_type pos )
{
	if( pos == std::string::npos )
		return false;

	for( ; pos > 0; pos-- )
		if( s[pos] == '\n' )
		{
			pos++;
			break;
		}

	return s[pos] == '#';
}

std::string::size_type find_function( const std::string & function, 
									  const std::string &s, 
									  std::string::size_type start )
{
    bool found = false;

    while( !found )
    {   
		std::string::size_type pos = s.find( function, start );

		if( pos == std::string::npos )
			return pos;

		if( pos + function.size() >= s.size() )
			return std::string::npos;

		DEBUG( format( "found %s at line %d => %s", function, get_linenum(s,pos), get_whole_line(s, pos) ));

		if( isalpha( s[pos-1] ) || isalpha( s[pos+function.size()] ) || 
			s[pos-1] == '_' || s[pos-1] == '$' )
		{ 
			start = pos + function.size();
			continue;
		}

		// now an '(' has to follow and nothing in beween except spaces or newlines

		for( std::string::size_type i = pos + function.size(); 
			 i < s.size(); i++ )
		{
			if( s[i] == '(' )
			  {
				// check if we are in a string
				if( is_in_string( s, pos ) )
				  {
					start = pos + function.size();
					break;
				  }

				// wen vorher extern steht, dann ist es eine Dekleration, von der ma nix wollen
				std::string::size_type pp = skip_spaces( s, pos, true );

				while( pp > 0 && pp != std::string::npos )
				  {
					if( !isalpha( s[pp] ) )
					  break;
					pp--;
				  }

				if( pp > 0 && pp != std::string::npos ) 
				  {
					if( strip( s.substr( pp, pp - pos ) ) == "extern" )
					  {
						start = pos + function.size();
						break;
					  }
				  }

				return pos;
			  }

			if( !is_in( s[i], " \t\n" ) ) {
				start = pos + function.size();
				continue;
				// return std::string::npos;
			}
		}
    }

    assert( 0 ); // will never be reached
}

std::string strip_stuff( const std::string &s, 
						 const std::string &start, 
						 const std::string &end )
{
  std::string::size_type pos1 = 0, pos2 = 0, last = 0;
  std::string ret;

  ret.reserve( s.size() );

   // std::cout << "rerun" << std::endl;

  do
	{

	  std::string::size_type pos3 = last;
	  bool cont = false;

	  do {
		cont = false;

	  	pos1 = s.find( start, pos3 );

	  	if( is_in_string( s, pos1 ) )
			{
				// std::cout << "start continue at line: " << get_linenum( s, pos1 ) << std::endl;
				pos3 = pos1 + start.size();
				cont = true;
			} 
	  } while ( cont );	

	  if( pos1 == std::string::npos )
		{
		  ret += s.substr( last, pos1 );
		  return ret;
		}

      pos3 = pos1;

	  do {
	  	cont = false;
	  	pos2 = s.find( end, pos3 );

	  	if( is_in_string( s, pos2 ) )
			{
				// std::cout << "end continue at line: " << get_linenum( s, pos1 ) << std::endl;
				pos3 = pos2 + end.size();
				cont = true;
			} 
      } while ( cont ); 

	  if( pos2 == std::string::npos )
		{
		  ret += s.substr( last, pos2 );
		  return ret;
		}		

	  // std::cout << "start: " << get_linenum( s, pos1 ) << " end: " << get_linenum( s, pos2 ) << std::endl;

	  ret += s.substr( last, pos1 - last );

	  std::string nlines = s.substr( pos1, pos2 - pos1 );

	  // std::cout << "'" << nlines << "'" << std::endl;

	  for( unsigned i = 0; i < nlines.size(); i++ )
		if( nlines[i] == '\n' )
		  ret += '\n';

	  last = pos2 + end.size();

	} while( last != std::string::npos );

  return ret;
}

void strip_cpp_comment( std::string &s, const std::string & pattern )
{
  std::string::size_type pos = 0;

  do
	{
	  pos = s.find( pattern, pos );

	  if( pos == std::string::npos )
		  return;

	  if( is_in_string( s, pos ) )
		{
		  pos += pattern.size();
		  continue;
		} 

	  // std::cout << get_line( s, pos ) << std::endl;

	  while( pos < s.size() && s[pos] != '\n' )
		{
		  s[pos] = ' '; // mit spaces auffuellen
		  pos++;
		}

	} while( pos != std::string::npos );
}

static void strip_define( std::string &s, const std::string & pattern, bool save_numeric_defines )
{
  std::string::size_type pos = 0;

  do
	{

	  std::string::size_type start = pos = s.find( pattern, pos );

	  if( pos == std::string::npos )
		  return;

	  if( is_in_string( s, pos ) )
		{
		  pos += pattern.size();
		  continue;
		} 

	  // std::cout << get_line( s, pos ) << std::endl;

	  // "# define" finden
	  
	  for( std::string::size_type p = pos-1; p > 0; p-- )
	  {
		  if( s[p] == '#' )
		  {
			  pos = p;
			  break;
		  }

		  if( s[p] == '\n' )
			  break;

		  if( isspace( s[p] ) )
			  continue;		  

		  break;
	  }

	  if( save_numeric_defines )
	    {
	      std::string ss = get_line( s, start );
	      std::vector<std::string> sl = split_simple( ss );

	      if( sl.size() == 3 )
		{
		  std::string sss = strip(sl[2], "()" );

		  if( is_int( sss ) )
		    {
		      //SETUP()->numeric_defines[sl[1]] = s2x<int>( sss, 0 );
		      // std::cout << "added " << sl[1] << " value: " << sss << std::endl;
		    }
		}
	    }

	  while( pos < s.size() && s[pos] != '\n' )
		{
		  s[pos] = ' '; // mit spaces auffuellen
		  pos++;
		}

	} while( pos != std::string::npos );
}

std::string strip_comments( const std::string &s )
{
  std::string ret( strip_stuff( s, "/*", "*/" ) );

  strip_cpp_comment( ret );

  // std::cout << ret << std::endl;

  strip_define( ret, "define", true );
  strip_define( ret, "ifndef", false );

  // std::cout << rope.string() << std::endl;

  return ret;
}

std::string strip_ifdefelse( const std::string &s, 
			     const std::string & start,
			     const std::string & end,
			     const std::string & middle )
{
  std::string::size_type pos1 = 0, pos2 = 0, last = 0;
  std::string ret;

  ret.reserve( s.size() );

  do
	{
	  pos1 = s.find( start, last );
	  if( pos1 == std::string::npos )
		{
		  ret += s.substr( last, pos1 );
		  return ret;
		}

	  pos2 = s.find( end, pos1 );
	  if( pos2 == std::string::npos )
		{
		  ret += s.substr( last, pos2 );
		  return ret;
		}		

	  ret += s.substr( last, pos1 - last );

	  std::string nlines = s.substr( pos1, pos2 - pos1 );

	  // find middle
	  std::string::size_type pos3 = nlines.find( middle );
	  std::string else_tree;

	  if( pos3 != std::string::npos )
		{
		  // add else tree
		  else_tree = nlines.substr( pos3 + middle.size(), nlines.size() - end.size() );		 

		  // reduce nlines
		  nlines = nlines.substr( 0, pos3 + middle.size() );
		}

	  for( unsigned i = 0; i < nlines.size(); i++ )
		if( nlines[i] == '\n' )
		  ret += '\n';

	  ret += else_tree;

	  last = pos2 + end.size();

	} while( last != std::string::npos );

  return ret;
}

std::string strip_ifnull( const std::string &s )
{
  return strip_stuff( s, "#if 0", "#endif" );
}

std::string erase( const std::string & s, const std::string & what )
{
  std::vector<std::string> sl = split_simple( s, what );

  std::string ret;

  ret.reserve( s.size() );

  for( unsigned i = 0; i < sl.size(); i++ )
	ret += sl[i];

  return ret;
}


std::vector<std::string> sequence_point_split( const std::string & s )
{
	std::string::size_type point = s.find( "." );
	std::string::size_type pointer = s.find( "->" );

	std::vector<std::string> sl;

	std::string splitter;

	if( point != std::string::npos && pointer == std::string::npos )
		splitter = ".";
	else if( point == std::string::npos && pointer != std::string::npos )
		splitter = "->";
	else if( point == std::string::npos && pointer == std::string::npos )
	{
		sl.push_back( s );
		return sl;
	} else 	if( point < pointer ) {
		splitter = ".";
	} else if( point > pointer ) {
		splitter = "->";
	}

	std::string::size_type pos = s.find( splitter );

	sl.push_back( strip( s.substr( 0, pos ) ) );

	std::string rest = strip( s.substr( pos + splitter.size() ) );

	if( rest.find( "." ) != std::string::npos || rest.find( "->" ) != std::string::npos )
	{
		std::vector<std::string> ssl = sequence_point_split( rest );

		for( unsigned i = 0; i < ssl.size(); i++ )
			sl.push_back( ssl[i] );
	} else {
		sl.push_back( rest );
	}

	return sl;
}

std::string::size_type skip_spaces( const std::string & s, std::string::size_type pos, bool reverse  )
{
  if( pos == std::string::npos )
	return pos;

	if( reverse )
	{
		for( ; pos > 0; pos -- )
			if( !isspace( s[pos] ) )
				return pos;
	} else {
		for( ; pos < s.size(); pos++ )
			if( !isspace( s[pos] ) )
				return pos;
	}

	return std::string::npos;
}


bool is_in_string( const std::string &s, std::string::size_type pos )
{
  if( pos == std::string::npos )
    return false;

  std::string::size_type start = pos;

  std::string::size_type p = s.rfind( '\n', start );
  if( p == std::string::npos )
	start = 0;
  else
	start = ++p;

  std::string line = get_line( s, start );

  if( line.find( '"' ) == std::string::npos )
	return false;

  Tools::Pairs pairs( line );

  if( pairs.is_in_pair( pos - start ) )
    return true;

  return false;
}

std::string get_assignment_var( const std::string & s, std::string::size_type pos )
{
  std::string::size_type p = skip_spaces( s, pos, true );
  
  if( p == std::string::npos )
    return std::string();    

  if( s[p] != '=' )
    return std::string();

  p--;

  p = skip_spaces( s, p, true );

  if( p == std::string::npos )
    return std::string();

  std::string::size_type end = p;

  while( p > 0 && ( isalnum( s[p] ) || s[p] == '_' || s[p] == '$' ) && !isspace(s[p]) )
    {
      p--;
    }

  return s.substr( p+1, (end - p) );
}

std::ostream & operator<<(std::ostream & out, const std::vector<std::string> & list )
{
  for( unsigned i = 0; i < list.size(); i++ )	  
	{
	  if( i > 0 )
		out << ',';
	  
	  out << list[i];
	}
  return out;
}

bool is_in_list( const std::string & s,  const std::vector<std::string> & list )
{
  for( unsigned i = 0; i < list.size(); i++ )
	if( s == list[i] )
	  return true;
  
  return false;
}

bool overlap( const std::vector<std::string> &list1, const std::vector<std::string> & list2 )
{
  for( unsigned i = 0; i < list1.size(); i++ )
	{
	  if( is_in_list( list1[i], list2 ) )
		{
		  return true;
		}
	}
  
  return false;
}

bool get_function( const std::string &s,
                   std::string::size_type pos,
                   std::string::size_type &start,
                   std::string::size_type &end, Function *func )
{
    int count = 1;
    start = s.find( '(', pos );

    if( start == std::string::npos )
        return false;

    if( func )
        func->name = strip( s.substr( pos, start - pos ) );

    pos = start;

    bool in_string = false;
    bool escaped = false;
    std::string::size_type last_arg_end = pos + 1;

    do
    {
        pos++;

        if( pos >= s.size() )
            return false;

        if( s[pos] == '\\' )
        {
            if( escaped ) // \\ gefunden
                escaped = false;
            else
                escaped = true;
            continue;
        }
        if( s[pos] == '"' && !escaped )
             in_string = !in_string;

         escaped = false;

         if( !in_string )
         {
             if( s[pos] == '(' )
                 count++;
             if( s[pos] == ')' )
                 count--;

             if( func && s[pos] == ',' && count == 1 )
             {
               // handle ','
               if( pos > 0 && pos < s.size() - 1 && s[pos-1] == '\'' && s[pos+1] == '\'' )
                 continue;

                 std::string ss = strip( s.substr( last_arg_end, pos - last_arg_end ) );

                 if( !ss.empty() )
                     func->args.push_back( ss );

                 last_arg_end = pos + 1;
             }
         }

     } while( count > 0 );
    end = pos;

    if( func )
    {
        std::string ss = strip( s.substr( last_arg_end, pos - last_arg_end ) );

        if( !ss.empty() )
            func->args.push_back( ss );

        std::string::size_type send = end;

        for( send = end+1; send < s.size(); send++ )
          {
            if( isspace( s[send] ) )
              continue;

            if( s[send] == '{' )
              func->is_impl = true;

            break;
          }

        /*
        if( func->is_impl )
          {
            break_count = 1;
            for( ; send < s.size(); send++ )
              {
                if( s[send] == '}' && !is_in_string( s, send ) )
                  break_count--;
                else if( s[send] == '{' !is_in_string( s, send ) )
                  break_count++;

                if( break_count <= 0 ) {
                  func->impl = s.substr( end, send - end );
                  break;
                }
              }
            }
        */
    }

    return true;
}


