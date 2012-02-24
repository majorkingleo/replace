#include "find_first_of.h"

std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a )

{
	std::vector<std::string> list;
	list.push_back(a);

	return find_first_of( file, start, list );
}

std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a,
									   const std::string & b )
{
	std::vector<std::string> list;
	list.push_back(a);
	list.push_back(b);

	return find_first_of( file, start, list );
}


std::string::size_type find_first_of( const std::string & file,
										std::string::size_type start,
									   const std::string & a,
									   const std::string & b,
									   const std::string & c )
{
	std::vector<std::string> list;
	list.push_back(a);
	list.push_back(b);
	list.push_back(c);

	return find_first_of( file, start, list );
}

std::string::size_type find_first_of( const std::string & file,
									   std::string::size_type start,
									   const std::vector<std::string> & sl )
{
	std::vector<std::string::size_type> pos_list;

	for( unsigned i = 0; i < sl.size(); i++ )
	{
		pos_list.push_back(file.find(sl[i], start));
	}

	std::string::size_type min = std::string::npos;

	for( unsigned i = 0; i < pos_list.size(); i++ )
	{
		if( pos_list[i] != std::string::npos )
		{
			if( min == std::string::npos )
				min = pos_list[i];
			else if( pos_list[i] < min )
				min = pos_list[i];

		}
	}

	return min;
}
