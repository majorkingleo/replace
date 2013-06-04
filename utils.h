 /* This code is from clp (C) by mobbi */

#ifndef UTILS_H
#define UTILS_H


#include <string>
#include "tools/range.h"
#include "tools/pairs.h"
#include "ref.h"

class ArgEnumType {
 public:
  enum ETYPE
    {
      FIRST__ = -1,
      UNKNOWN = 0,
      INT,
      LONG,
      STRING,
      FLOAT,
      DOUBLE,
      CHAR,
      STRUCT,
      SVOID,
      CSTRING, // const string
      REAL_STRUCT, // the type is a complete struct
      LAST__
    };

  std::string array_length;

  ArgEnumType();
  ArgEnumType( const ArgEnumType & other );

  virtual ~ArgEnumType();
};

class CopyArgType
{
 public:
  void operator()( ArgEnumType & dest, ArgEnumType & source )
  {
    dest.array_length = source.array_length;
  }
};


typedef Tools::EnumRange<ArgEnumType,CopyArgType> ArgType;

class Function
{
public:
    std::string name;
    unsigned line;
    ArgType ret_type;
    std::string ret_string;
    std::vector<std::string> args;
    bool is_impl;
    bool is_static;
    std::string impl;

  Function()
    : line(0), is_impl(false), is_static(false)
  {}

    ~Function() {}
};


bool is_in( char c, const std::string &values );

unsigned get_linenum( const std::string &s, std::string::size_type pos );

inline std::string get_line( const std::string &s, std::string::size_type pos )
{
	if( pos == std::string::npos )
		return std::string();

    std::string::size_type p = s.find( '\n', pos );
    return s.substr( pos, p - pos );
}

template <class T> bool is_in_string( const T &s, std::string::size_type pos )
{
  if( pos == std::string::npos )
    return false;

  if( s.find( '"' ) == std::string::npos )
	return false;

  std::string::size_type start = pos;

  while( start )
    {
      if( s[start] == '\n' )
        {
          start++;
          break;
        }

      start--;
    }

  std::string line = get_line( s, start );

  if( line.find( '"' ) == std::string::npos )
	return false;

  Tools::Pairs pairs( line );

  if( pairs.is_in_pair( pos - start ) )
    return true;

  return false;
}

bool is_in_string( const std::string &s, std::string::size_type pos );

std::string::size_type find_function( const std::string & function, 
									  const std::string &s, 
									  std::string::size_type start = 0 );

std::string erase( const std::string & s, const std::string & what );

std::string strip_ifnull( const std::string &s );

std::string strip_ifdefelse( const std::string &s, 
			     const std::string & start = "#ifdef",
			     const std::string & end = "#endif",
			     const std::string & middle = "#else" );

std::string strip_comments( const std::string &s );

void strip_cpp_comment( std::string &s, const std::string & pattern = "//" );

std::string strip_stuff( const std::string &s, 
						 const std::string &start, 
						 const std::string &end );


std::vector<std::string> sequence_point_split( const std::string & s );

std::string::size_type find_function_decl( const std::string & s, std::string::size_type pos, Function & f );

template <class T> std::string get_whole_line( const T &s, std::string::size_type pos )
{
	if( pos == std::string::npos )
		return std::string();

	for( ; pos > 0; pos-- )
		if( s[pos] == '\n' )
		{
			pos++;
			break;
		}

    std::string::size_type p = s.find( '\n', pos );
    return s.substr( pos, p - pos );
}

std::string::size_type skip_spaces( const std::string & s, std::string::size_type pos, bool reverse = false );

std::string get_assignment_var( const std::string & s, std::string::size_type pos );

bool overlap( const std::vector<std::string> &list1, const std::vector<std::string> & list2 );

bool is_in_list( const std::string & s,  const std::vector<std::string> & list );

std::ostream & operator<<(std::ostream & out, const std::vector<std::string> & list );

bool get_function( const std::string &s,
                   std::string::size_type pos,
                   std::string::size_type &start,
                   std::string::size_type &end, Function *func,  bool strip_args = true );

#endif
