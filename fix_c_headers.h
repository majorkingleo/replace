/*
 * fix_c_headers.h
 *
 *  Created on: 28.01.2020
 *      Author: martin
 */

#ifndef FIX_C_HEADERS_H_
#define FIX_C_HEADERS_H_

#include <string>

class FixCHeaders
{
public:
	int main( const std::string & path );

private:
	std::string build_func_name( const std::string & file );
	std::string diff_file( const std::string & orig, const std::string & patched );
	std::string patch_file( const std::string & file );
	int count_open_ifdef( const std::string & file, std::string::size_type end );
	bool should_skip_file( const std::string & file );
};



#endif /* FIX_C_HEADERS_H_ */
