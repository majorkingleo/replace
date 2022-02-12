#ifndef GETLINE_H
#define GETLINE_H

#include <string>

std::string getline( const std::string & s, std::string::size_type pos );

std::string::size_type get_pos_for_line( const std::string & content, int line );

std::string diff_lines( const std::string & orig, std::string & modded );
std::wstring diff_lines( const std::wstring & orig, std::wstring & modded );

#endif
