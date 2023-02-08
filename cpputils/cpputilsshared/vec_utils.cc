/**
 * vector utilities
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */

#include "vec_utils.h"
#include "cpp_util.h"

namespace Tools {

  namespace {
	
	class FunktorString2ConstChar
	{
	public:
	  const char *operator()( const std::string & s ) const
	  {
		return TO_CHAR(s);
	  }
	};

  } // namespace anonymous

void addAll( std::vector<const char *> & a, const std::vector<std::string> & b )
{
  addAll( a, b,   FunktorString2ConstChar() );
}

} // /namespace Tools

