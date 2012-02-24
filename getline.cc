#include <getline.h>

std::string getline( const std::string & s, std::string::size_type pos )
{
  std::string::size_type end = s.find( '\n', pos );

  return s.substr( pos, end - pos );
}
