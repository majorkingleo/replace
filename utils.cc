#include <cassert>
#include <vector>
#include <iostream>

#include "utils.h"
#include "pairs.h"
#include "string_utils.h"
#include "cppdir.h"
#include "xml.h"
#include "CpputilsDebug.h"
#include <algorithm>
#include "DetectLocale.h"
#include <format.h>

using namespace Tools;

bool is_in( wchar_t c, const std::wstring &values )
{
    for( std::wstring::size_type i = 0; i < values.size(); i++ )
    {
		if( c == values[i] )
			return true;
    }

    return false;
}

unsigned get_linenum( const std::wstring &s, std::string::size_type pos )
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

template <class T> bool is_define_line( const T &s, std::wstring::size_type pos )
{
	if( pos == std::wstring::npos )
		return false;

	for( ; pos > 0; pos-- )
		if( s[pos] == L'\n' )
		{
			pos++;
			break;
		}

	return s[pos] == L'#';
}

std::wstring::size_type find_function( const std::wstring & function,
									  const std::wstring &s,
									  std::wstring::size_type start )
{
    bool found = false;

    CPPDEBUG( format("try finding function '%s'", DETECT_LOCALE.wString2output(function)));

    while( !found )
    {   
		std::wstring::size_type pos = s.find( function, start );

		if( pos == std::string::npos )
			return pos;

		if( pos + function.size() >= s.size() )
			return std::string::npos;

		CPPDEBUG( wformat( L"found %s at line %d => %s",
				function,
				get_linenum(s,pos),
				get_whole_line(s, pos) ));

		if( isalpha( s[pos-1] ) ||
		    isalpha( s[pos+function.size()] ) ||
			s[pos-1] == L'_' ||
			s[pos-1] == L'$' )
		{ 
			start = pos + function.size();
			continue;
		}

		// now an '(' has to follow and nothing in beween except spaces or newlines

		for( std::wstring::size_type i = pos + function.size();
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
				std::wstring::size_type pp = skip_spaces( s, pos, true );

				while( pp > 0 && pp != std::wstring::npos )
				  {
					if( !isalpha( s[pp] ) )
					  break;
					pp--;
				  }

				if( pp > 0 && pp != std::wstring::npos )
				  {
					if( strip( s.substr( pp, pp - pos ) ) == L"extern" )
					  {
						start = pos + function.size();
						break;
					  }
				  }

				return pos;
			  }

			if( !is_in( s[i], L" \t\n" ) ) {
				start = pos + function.size();
				break;
			}
		}
    }

    assert( 0 ); // will never be reached
}

std::wstring strip_stuff( const std::wstring &s,
						 const std::wstring &start,
						 const std::wstring &end )
{
  std::wstring::size_type pos1 = 0, pos2 = 0, last = 0;
  std::wstring ret;

  ret.reserve( s.size() );

   // std::cout << "rerun" << std::endl;

  do
	{

	  std::wstring::size_type pos3 = last;
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

	  if( pos1 == std::wstring::npos )
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

	  if( pos2 == std::wstring::npos )
		{
		  ret += s.substr( last, pos2 );
		  return ret;
		}		

	  // std::cout << "start: " << get_linenum( s, pos1 ) << " end: " << get_linenum( s, pos2 ) << std::endl;

	  ret += s.substr( last, pos1 - last );

	  std::wstring nlines = s.substr( pos1, pos2 - pos1 );

	  // std::cout << "'" << nlines << "'" << std::endl;

	  for( unsigned i = 0; i < nlines.size(); i++ )
		if( nlines[i] == L'\n' )
		  ret += L'\n';

	  last = pos2 + end.size();

	} while( last != std::wstring::npos );

  return ret;
}

void strip_cpp_comment( std::wstring &s, const std::wstring & pattern )
{
  std::wstring::size_type pos = 0;

  do
	{
	  pos = s.find( pattern, pos );

	  if( pos == std::wstring::npos )
		  return;

	  if( is_in_string( s, pos ) )
		{
		  pos += pattern.size();
		  continue;
		} 

	  // std::cout << get_line( s, pos ) << std::endl;

	  while( pos < s.size() && s[pos] != L'\n' )
		{
		  s[pos] = L' '; // mit spaces auffuellen
		  pos++;
		}

	} while( pos != std::wstring::npos );
}

static void strip_define( std::wstring &s, const std::wstring & pattern, bool save_numeric_defines )
{
  std::wstring::size_type pos = 0;

  do
	{

	  std::wstring::size_type start = pos = s.find( pattern, pos );

	  if( pos == std::wstring::npos )
		  return;

	  if( is_in_string( s, pos ) )
		{
		  pos += pattern.size();
		  continue;
		} 

	  // std::cout << get_line( s, pos ) << std::endl;

	  // "# define" finden
	  
	  for( std::wstring::size_type p = pos-1; p > 0; p-- )
	  {
		  if( s[p] == L'#' )
		  {
			  pos = p;
			  break;
		  }

		  if( s[p] == L'\n' )
			  break;

		  if( isspace( s[p] ) )
			  continue;		  

		  break;
	  }

	  if( save_numeric_defines )
	    {
	      std::wstring ss = get_line( s, start );
	      std::vector<std::wstring> sl = split_simple( ss );

	      if( sl.size() == 3 )
		{
		  std::wstring sss = strip(sl[2], L"()" );

		  if( is_int( sss ) )
		    {
		      //SETUP()->numeric_defines[sl[1]] = s2x<int>( sss, 0 );
		      // std::cout << "added " << sl[1] << " value: " << sss << std::endl;
		    }
		}
	    }

	  while( pos < s.size() && s[pos] != L'\n' )
		{
		  s[pos] = L' '; // mit spaces auffuellen
		  pos++;
		}

	} while( pos != std::wstring::npos );
}

std::wstring strip_comments( const std::wstring &s )
{
  std::wstring ret( strip_stuff( s, L"/*", L"*/" ) );

  strip_cpp_comment( ret );

  // std::cout << ret << std::endl;

  strip_define( ret, L"define", true );
  strip_define( ret, L"ifndef", false );

  // std::cout << rope.string() << std::endl;

  return ret;
}

std::wstring strip_ifdefelse( const std::wstring &s,
			     const std::wstring & start,
			     const std::wstring & end,
			     const std::wstring & middle )
{
  std::wstring::size_type pos1 = 0, pos2 = 0, last = 0;
  std::wstring ret;

  ret.reserve( s.size() );

  do
	{
	  pos1 = s.find( start, last );
	  if( pos1 == std::wstring::npos )
		{
		  ret += s.substr( last, pos1 );
		  return ret;
		}

	  pos2 = s.find( end, pos1 );
	  if( pos2 == std::wstring::npos )
		{
		  ret += s.substr( last, pos2 );
		  return ret;
		}		

	  ret += s.substr( last, pos1 - last );

	  std::wstring nlines = s.substr( pos1, pos2 - pos1 );

	  // find middle
	  std::wstring::size_type pos3 = nlines.find( middle );
	  std::wstring else_tree;

	  if( pos3 != std::wstring::npos )
		{
		  // add else tree
		  else_tree = nlines.substr( pos3 + middle.size(), nlines.size() - end.size() );		 

		  // reduce nlines
		  nlines = nlines.substr( 0, pos3 + middle.size() );
		}

	  for( unsigned i = 0; i < nlines.size(); i++ )
		if( nlines[i] == L'\n' )
		  ret += L'\n';

	  ret += else_tree;

	  last = pos2 + end.size();

	} while( last != std::wstring::npos );

  return ret;
}

std::wstring strip_ifnull( const std::wstring &s )
{
  return strip_stuff( s, L"#if 0", L"#endif" );
}

std::wstring erase( const std::wstring & s, const std::wstring & what )
{
  std::vector<std::wstring> sl = split_simple( s, what );

  std::wstring ret;

  ret.reserve( s.size() );

  for( unsigned i = 0; i < sl.size(); i++ )
	ret += sl[i];

  return ret;
}


std::vector<std::wstring> sequence_point_split( const std::wstring & s )
{
	std::wstring::size_type point = s.find( L"." );
	std::wstring::size_type pointer = s.find( L"->" );

	std::vector<std::wstring> sl;

	std::wstring splitter;

	if( point != std::string::npos && pointer == std::wstring::npos )
		splitter = L".";
	else if( point == std::wstring::npos && pointer != std::wstring::npos )
		splitter = L"->";
	else if( point == std::wstring::npos && pointer == std::wstring::npos )
	{
		sl.push_back( s );
		return sl;
	} else 	if( point < pointer ) {
		splitter = L".";
	} else if( point > pointer ) {
		splitter = L"->";
	}

	std::wstring::size_type pos = s.find( splitter );

	sl.push_back( strip( s.substr( 0, pos ) ) );

	std::wstring rest = strip( s.substr( pos + splitter.size() ) );

	if( rest.find( L"." ) != std::string::npos || rest.find( L"->" ) != std::wstring::npos )
	{
		std::vector<std::wstring> ssl = sequence_point_split( rest );

		for( unsigned i = 0; i < ssl.size(); i++ )
			sl.push_back( ssl[i] );
	} else {
		sl.push_back( rest );
	}

	return sl;
}

std::wstring::size_type skip_spaces( const std::wstring & s, std::wstring::size_type pos, bool reverse  )
{
  if( pos == std::wstring::npos )
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

	return std::wstring::npos;
}


bool is_in_string( const std::wstring &s, std::wstring::size_type pos )
{
  if( pos == std::wstring::npos )
    return false;

  std::wstring::size_type start = pos;

  std::wstring::size_type p = s.rfind( L'\n', start );
  if( p == std::wstring::npos )
	start = 0;
  else
	start = ++p;

  std::wstring line = get_line( s, start );

  if( line.find( L'"' ) == std::wstring::npos )
	return false;

  Tools::WPairs pairs( line );

  if( pairs.is_in_pair( pos - start ) )
    return true;

  return false;
}

std::wstring get_assignment_var( const std::wstring & s, std::wstring::size_type pos )
{
  std::wstring::size_type p = skip_spaces( s, pos, true );
  
  if( p == std::wstring::npos )
    return std::wstring();

  if( s[p] != L'=' )
    return std::wstring();

  p--;

  p = skip_spaces( s, p, true );

  if( p == std::wstring::npos )
    return std::wstring();

  std::wstring::size_type end = p;

  while( p > 0 && ( isalnum( s[p] ) || s[p] == L'_' || s[p] == L'$' ) && !isspace(s[p]) )
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

bool get_function( const std::wstring &s,
                   std::wstring::size_type pos,
                   std::wstring::size_type &start,
                   std::wstring::size_type &end, Function *func, bool strip_args )
{
    int count = 1;
    start = s.find( L'(', pos );

    if( start == std::wstring::npos )
        return false;

    if( func )
        func->name = strip( s.substr( pos, start - pos ) );

    pos = start;

    bool in_string = false;
    bool escaped = false;
    std::wstring::size_type last_arg_end = pos + 1;

    do
    {
        pos++;

        if( pos >= s.size() )
            return false;

        if( s[pos] == L'\\' )
        {
            if( escaped ) // \\ gefunden
                escaped = false;
            else
                escaped = true;
            continue;
        }
        if( s[pos] == L'"' && !escaped )
             in_string = !in_string;

         escaped = false;

         if( !in_string )
         {
             if( s[pos] == L'(' )
                 count++;
             if( s[pos] == L')' )
                 count--;

             if( func && s[pos] == L',' && count == 1 )
             {
               // handle ','
               if( pos > 0 && pos < s.size() - 1 && s[pos-1] == L'\'' && s[pos+1] == L'\'' )
                 continue;

                 std::wstring ss = strip( s.substr( last_arg_end, pos - last_arg_end ) );

                 if( !ss.empty() ) {
                	 if( strip_args )
                		 func->args.push_back( ss );
                	 else
                		 func->args.push_back( s.substr( last_arg_end, pos - last_arg_end ) );
                 }

                 last_arg_end = pos + 1;
             }
         }

     } while( count > 0 );
    end = pos;

    if( func )
    {
        std::wstring ss = strip( s.substr( last_arg_end, pos - last_arg_end ) );

        if( !ss.empty() ) {
        	if( strip_args )
        		func->args.push_back( ss );
        	else
        		func->args.push_back( s.substr( last_arg_end, pos - last_arg_end ) );
        }

        std::wstring::size_type send = end;

        for( send = end+1; send < s.size(); send++ )
          {
            if( isspace( s[send] ) )
              continue;

            if( s[send] == L'{' )
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

std::wstring function_to_string( const std::wstring & res,
								const Function & func,
								std::wstring::size_type start,
								std::wstring::size_type end )
{
	std::wstring first_part_of_file = res.substr(0,start);

	std::wstringstream str;

	str << func.name << L"(";

	for( unsigned i = 0; i < func.args.size(); i++ )
	{
		if( i > 0 ) {
			str << L",";
		}

		str << func.args[i];
	}

	str << L")";

	CPPDEBUG( str.str() );

	std::wstring second_part_of_file = res.substr(end);

	return first_part_of_file + str.str() + second_part_of_file;
}

std::wstring get_whole_line( const std::wstring & s, std::wstring::size_type pos )
{
	if( pos == std::wstring::npos ) {
		return std::wstring();
	}

	std::wstring::size_type ppos = s.rfind( '\n', pos );

	if( ppos == std::string::npos ) {
		ppos = 0;
	} else {
		ppos++;
	}

    std::wstring::size_type p = s.find( '\n', ppos );
    std::wstring ret = s.substr( ppos, p - ppos );

    // std::cout << "ppos: " << ppos << " p: " << p << " >" << ret << "< " << std::endl;

    return ret;
}


std::wstring::size_type rfind_first_of( const std::wstring & s, const std::wstring & delims, std::wstring::size_type start )
{
	std::wstring copy( s );
	std::reverse(copy.begin(), copy.end());

	std::wstring::size_type start_pos = 0;

	if( start != std::wstring::npos ) {
		start_pos = s.size() - start;
	}

	std::wstring::size_type pos = copy.find_first_of( delims, start_pos );

	if( pos == std::wstring::npos ) {
		return std::wstring::npos;
	}

	return copy.size() - pos;
}
