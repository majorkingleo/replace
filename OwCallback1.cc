/*
 * OwCallback1.cc
 *
 *  Created on: 19.04.2012
 *      Author: martin
 */
#include "OwCallback1.h"
#include "utils.h"
#include <format.h>

using namespace Tools;

OwCallback1::OwCallback1()
{
	keywords.push_back( L"OwCallback2" );
	keywords.push_back( L"OwCallback3" );
	keywords.push_back( L"OwCallback4" );
	keywords.push_back( L"OwCallback5" );
	keywords.push_back( L"OwCallback6" );
	keywords.push_back( L"OwCallback7" );
	keywords.push_back( L"OwCallback8" );
	keywords.push_back( L"OwCallback9" );

}

void OwCallback1::fix_owcallbackx( std::wstring & s, int num )
{
	std::wstring callback_first = wformat(L"OwCallback%d", num );
	std::wstring callback_second = wformat(L"OwCallback%d", num + 1);

	for( std::string::size_type pos = 0; pos != std::wstring::npos && pos < s.size(); pos++ )
	{
		if( ( pos = s.find( callback_second, pos ) ) != std::wstring::npos )
		{
			if( is_in_string( s, pos ) )
			{
				continue;
			}

			std::wstring::size_type begin_of_pos = s.rfind(L"[", pos);
			std::wstring::size_type pos_callback1 = s.rfind(callback_first,pos);

			if( begin_of_pos == std::wstring::npos ) {
				pos = s.find( L'\n', pos );
				continue;

			} else if( pos_callback1 == std::wstring::npos ||
					pos_callback1 < begin_of_pos ) {


				// now fix it
				s[pos+10] = L'0' + num;
			}

			pos = s.find( L'\n', pos );
		} else {
			break;
		}
	}
}

std::wstring OwCallback1::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring result( file );

	for( int i = 0; i < 8; i++ ) {
		fix_owcallbackx( result, 1 );
		fix_owcallbackx( result, 2 );
		fix_owcallbackx( result, 3 );
		fix_owcallbackx( result, 4 );
		fix_owcallbackx( result, 5 );
		fix_owcallbackx( result, 6 );
		fix_owcallbackx( result, 7 );
		fix_owcallbackx( result, 8 );
	}

	return result;
}

bool OwCallback1::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::RC_FILE:
		  return true;
	  default:
		  return false;
	}
}
