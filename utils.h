 /* This code is from clp (C) by mobbi */

#ifndef UTILS_H
#define UTILS_H


#include <string>
#include "pairs.h"
#include "range.h"
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
    std::wstring name;
    unsigned line;
    ArgType ret_type;
    std::wstring ret_string;
    std::vector<std::wstring> args;
    bool is_impl;
    bool is_static;
    std::wstring impl;

  Function()
    : line(0), is_impl(false), is_static(false)
  {}

    ~Function() {}
};


bool is_in( wchar_t c, const std::wstring &values );

unsigned get_linenum( const std::wstring &s, std::string::size_type pos );

inline std::wstring get_line( const std::wstring &s, std::wstring::size_type pos )
{
	if( pos == std::wstring::npos )
		return std::wstring();

    std::wstring::size_type p = s.find( L'\n', pos );
    return s.substr( pos, p - pos );
}

template <class T> bool is_in_string( const T &s, std::wstring::size_type pos )
{
  if( pos == std::wstring::npos )
    return false;

  if( s.find( L'"' ) == std::wstring::npos )
	return false;

  std::wstring::size_type start = pos;

  while( start )
    {
      if( s[start] == '\n' )
        {
          start++;
          break;
        }

      start--;
    }

  std::wstring line = get_line( s, start );

  if( line.find( L'"' ) == std::wstring::npos )
	return false;

  Tools::WPairs pairs( line );

  if( pairs.is_in_pair( pos - start ) )
    return true;

  return false;
}

bool is_in_string( const std::wstring &s, std::wstring::size_type pos );

std::wstring::size_type find_function( const std::wstring & function,
									   const std::wstring &s,
									   std::wstring::size_type start = 0 );

std::wstring erase( const std::wstring & s, const std::wstring & what );

std::wstring strip_ifnull( const std::wstring &s );

std::wstring strip_ifdefelse( const std::wstring &s,
			     const std::wstring & start = L"#ifdef",
			     const std::wstring & end = L"#endif",
			     const std::wstring & middle = L"#else" );

std::wstring strip_comments( const std::wstring &s );

void strip_cpp_comment( std::wstring &s, const std::wstring & pattern = L"//" );

std::wstring strip_stuff( const std::wstring &s,
						 const std::wstring &start,
						 const std::wstring &end );


std::vector<std::wstring> sequence_point_split( const std::wstring & s );

std::string::size_type find_function_decl( const std::string & s, std::string::size_type pos, Function & f );

std::wstring get_whole_line( const std::wstring & s, std::wstring::size_type pos );

std::wstring::size_type skip_spaces( const std::wstring & s, std::wstring::size_type pos, bool reverse = false );

std::wstring get_assignment_var( const std::wstring & s, std::wstring::size_type pos );

bool overlap( const std::vector<std::string> &list1, const std::vector<std::string> & list2 );

bool is_in_list( const std::string & s,  const std::vector<std::string> & list );

std::ostream & operator<<(std::ostream & out, const std::vector<std::string> & list );

bool get_function( const std::wstring &s,
                   std::wstring::size_type pos,
                   std::wstring::size_type &start,
                   std::wstring::size_type &end, Function *func,  bool strip_args = true );

std::wstring function_to_string( const std::wstring & s,
								const Function & func,
								std::wstring::size_type start,
								std::wstring::size_type end );

std::wstring::size_type rfind_first_of( const std::wstring & s, const std::wstring & delims, std::wstring::size_type start = std::wstring::npos );

#endif
