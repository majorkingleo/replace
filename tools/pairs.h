#ifndef TOOLS_H_pairs_h
#define TOOLS_H_pairs_h

#include <string>
#include <list>
#include <vector>

namespace Tools {

/// class handling text that should be ignored within a string
template<class t_std_string> class PairsBase
{
 private:

  /// structure presenting a single Pair
  struct Pair
  {
	typename t_std_string::size_type first;  ///< the first position
    typename t_std_string::size_type second; ///< the second position
  };
  typedef std::vector<Pair> Pair_list;
  typedef typename std::vector<Pair>::iterator Pair_list_it;

  Pair_list pairs;

  const t_std_string quote_sign;

 public:
  /// extract all pairs found in line
  PairsBase( t_std_string line , typename t_std_string::size_type startpos, const t_std_string & quote_sign );
  
  /// add a pair to the list
  void add( typename t_std_string::size_type first,
		  typename t_std_string::size_type second );

  void clear();  ///< clears the list

  /// checks if the position is between two pairs
  /** returns false if pos == std::string::npos */
  bool is_in_pair( typename t_std_string::size_type pos );
 
  /// extracts all pairs by itsself
  void extract( t_std_string line, typename t_std_string::size_type startpos = 0 );

 private:
  /// checks if the position is between the two pairs
  /** returns false if pos or one of the pairs == std::string::npos */
  bool is_in_pair( typename t_std_string::size_type pos,  ///< the current position
		   typename t_std_string::size_type pair1,
		   typename t_std_string::size_type pair2 ) const ;
};

class Pairs : public PairsBase<std::string>
{
public:
	Pairs( std::string line , std::string::size_type startpos = 0, const std::string & quote_sign = "\"" )
	: PairsBase<std::string>( line, startpos, quote_sign )
	{}
};

class WPairs : public PairsBase<std::wstring>
{
public:
	WPairs( std::wstring line , std::wstring::size_type startpos = 0, const std::wstring & quote_sign = L"\"" )
	: PairsBase<std::wstring>( line, startpos, quote_sign )
	{}
};

} // namespace Tools;

template<class t_std_string>
Tools::PairsBase<t_std_string>::PairsBase( t_std_string line, typename t_std_string::size_type startpos, const t_std_string & quote_sign)
: pairs(),
  quote_sign(quote_sign)
{
  pairs.reserve(5);
  extract( line, startpos );
}

template<class t_std_string>
void Tools::PairsBase<t_std_string>::clear()
{
  pairs.clear();
}

template<class t_std_string>
void Tools::PairsBase<t_std_string>::add( typename t_std_string::size_type first,
		 typename t_std_string::size_type second )
{
  Pair pair;

  pair.first = first;
  pair.second = second;

  pairs.push_back( pair );
}

template<class t_std_string>
bool Tools::PairsBase<t_std_string>::is_in_pair(  typename t_std_string::size_type pos,
		 typename t_std_string::size_type pair1,
		 typename t_std_string::size_type pair2 ) const
{
  if( pos == std::string::npos ||
      pair1 == std::string::npos ||
      pair2 ==  std::string::npos )
    return false;

  return ((pair1 < pos) && (pos < pair2) );
}

template<class t_std_string>
bool Tools::PairsBase<t_std_string>::is_in_pair(  typename t_std_string::size_type pos )
{
  for( Pair_list_it it =  pairs.begin(); it != pairs.end(); it++ )
    {
      if( is_in_pair( pos,
		      (typename t_std_string::size_type) it->first,
		      (typename t_std_string::size_type) it->second ) )
	{
	  return true;
	}
    }
  return false;
}

template<class t_std_string>
void Tools::PairsBase<t_std_string>::extract( t_std_string line,  typename t_std_string::size_type startpos )
{
  for( typename t_std_string::size_type pair_pos = startpos;;)
    {
      bool ignore;

      typename t_std_string::size_type pair1;

      do
	{
	  pair1 = line.find( quote_sign, pair_pos + 1 );

	  ignore = false;

	  if( pair1 != t_std_string::npos && pair1 > 0 )
	    if( line[pair1 - 1] == '\\' )
	      {
		ignore = true;
		if( pair1 > 1 && line[pair1 - 2] == '\\' )
		  ignore = false;
		else
		  pair_pos++;
	      }
	} while( ignore );

      if( pair1 != t_std_string::npos )
	{
	  pair_pos = pair1;

	  typename t_std_string::size_type pair2;

	  int factor = 1;

	  do
	    {
	      pair2 = line.find( quote_sign, pair1 + factor );

	      ignore = false;

	      if( pair2 != t_std_string::npos && pair2 > 0 )
		if( line[pair2 - 1] == '\\' )
		  {
		    ignore = true;
		    if( pair2 > 1 && line[pair2 - 2] == '\\' )
		      ignore = false;
		    else
		      factor++;
		  }
	    } while( ignore );


	  if( pair2 != t_std_string::npos )
	    {
	      add( pair1,pair2 );
	      pair_pos = pair2;
	    }
	}
      else
	break;
    }
}

#endif
