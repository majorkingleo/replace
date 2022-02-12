#include <getline.h>
#include <vector>
#include <string_utils.h>

using namespace Tools;

std::string getline( const std::string & s, std::string::size_type pos )
{
  std::string::size_type end = s.find( '\n', pos );

  return s.substr( pos, end - pos );
}


std::string::size_type get_pos_for_line( const std::string & content, int line )
{
	std::string::size_type pos = 0;

	for( int count = 1; count < line; count++ )
	{
		pos = content.find('\n', pos );

		if( pos == std::string::npos )
			return pos;

		pos++;
	}

	return pos;
}


std::string diff_lines( const std::string & orig, std::string & modded )
{
  std::vector<std::string> sl_orig, sl_modded;

  sl_orig = split_simple( orig, "\n" );
  sl_modded = split_simple( modded, "\n" );

  std::string res;

  for( unsigned i = 0; i < sl_orig.size() && i < sl_modded.size(); i++ )
	{
	  if( sl_orig[i] != sl_modded[i] )
		{
		  if( !res.empty() )
			res += '\n';

		  res += "\t" + strip( sl_orig[i] ) + " => " + strip( sl_modded[i] );
		}
	}

  return res;
}

std::wstring diff_lines( const std::wstring & orig, std::wstring & modded )
{
  std::vector<std::wstring> sl_orig, sl_modded;

  sl_orig = split_simple( orig, L"\n" );
  sl_modded = split_simple( modded, L"\n" );

  std::wstring res;

  for( unsigned i = 0; i < sl_orig.size() && i < sl_modded.size(); i++ )
	{
	  if( sl_orig[i] != sl_modded[i] )
		{
		  if( !res.empty() )
			res += '\n';

		  res += L"\t" + strip( sl_orig[i] ) + L" => " + strip( sl_modded[i] );
		}
	}

  return res;
}
