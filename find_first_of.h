#ifndef FIND_FIRST_OF
#define FIND_FIRST_OF

#include <vector>
#include <string>

std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a);

std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a,
									   const std::string & b );

std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a,
									   const std::string & b,
									   const std::string & c );

std::string::size_type find_first_of( const std::string & file,
									   std::string::size_type start,
									   const std::vector<std::string> & sl );

#endif
