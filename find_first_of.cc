#include "find_first_of.h"

std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a )

{
	std::vector<std::wstring> list;
	list.push_back(a);

	return find_first_of( file, start, list );
}

std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a,
									   const std::wstring & b )
{
	std::vector<std::wstring> list;
	list.push_back(a);
	list.push_back(b);

	return find_first_of( file, start, list );
}


std::wstring::size_type find_first_of( const std::wstring & file,
										std::wstring::size_type start,
									   const std::wstring & a,
									   const std::wstring & b,
									   const std::wstring & c )
{
	std::vector<std::wstring> list;
	list.push_back(a);
	list.push_back(b);
	list.push_back(c);

	return find_first_of( file, start, list );
}

std::wstring::size_type find_first_of( const std::wstring & file,
									   std::wstring::size_type start,
									   const std::vector<std::wstring> & sl )
{
	std::vector<std::wstring::size_type> pos_list;

	for( unsigned i = 0; i < sl.size(); i++ )
	{
		pos_list.push_back(file.find(sl[i], start));
	}

	std::wstring::size_type min = std::wstring::npos;

	for( unsigned i = 0; i < pos_list.size(); i++ )
	{
		if( pos_list[i] != std::wstring::npos )
		{
			if( min == std::wstring::npos )
				min = pos_list[i];
			else if( pos_list[i] < min )
				min = pos_list[i];

		}
	}

	return min;
}
