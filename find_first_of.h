#ifndef FIND_FIRST_OF
#define FIND_FIRST_OF

#include <vector>
#include <string>

std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a);

std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a,
									   const std::wstring & b );

std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a,
									   const std::wstring & b,
									   const std::wstring & c );

std::wstring::size_type find_first_of( const std::wstring & file,
									   std::wstring::size_type start,
									   const std::vector<std::wstring> & sl );

#endif
