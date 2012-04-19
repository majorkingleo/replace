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
	keywords.push_back( "OwCallback2" );
	keywords.push_back( "OwCallback3" );
	keywords.push_back( "OwCallback4" );
	keywords.push_back( "OwCallback5" );
	keywords.push_back( "OwCallback6" );
	keywords.push_back( "OwCallback7" );
	keywords.push_back( "OwCallback8" );
	keywords.push_back( "OwCallback9" );

}

void OwCallback1::fix_owcallbackx( std::string & s, int num )
{
	std::string callback_first = format("OwCallback%d", num );
	std::string callback_second = format("OwCallback%d", num + 1);

	for( std::string::size_type pos = 0; pos != std::string::npos && pos < s.size(); pos++ )
	{
		if( ( pos = s.find( callback_second, pos ) ) != std::string::npos )
		{
			if( is_in_string( s, pos ) )
			{
				continue;
			}

			std::string::size_type begin_of_pos = s.rfind("[", pos);
			std::string::size_type pos_callback1 = s.rfind(callback_first,pos);

			if( begin_of_pos == std::string::npos ) {
				pos = s.find( '\n', pos );
				continue;

			} else if( pos_callback1 == std::string::npos ||
					pos_callback1 < begin_of_pos ) {


				// now fix it
				s[pos+10] = '0' + num;
			}

			pos = s.find( '\n', pos );
		} else {
			break;
		}
	}
}

std::string OwCallback1::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	std::string result( file );

	fix_owcallbackx( result, 1 );
	fix_owcallbackx( result, 2 );
	fix_owcallbackx( result, 3 );
	fix_owcallbackx( result, 4 );
	fix_owcallbackx( result, 5 );
	fix_owcallbackx( result, 6 );
	fix_owcallbackx( result, 7 );
	fix_owcallbackx( result, 8 );

	return result;
}
