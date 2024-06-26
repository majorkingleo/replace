/*
 * fix_mlm.cc
 *
 *  Created on: 05.06.2012
 *      Author: martin
 */

#include "utils.h"
#include "fix_mlm.h"
#include <format.h>
#include "CpputilsDebug.h"

using namespace Tools;


FixMlM::FixMlM()
{

}

std::wstring FixMlM::patch_file( const std::wstring & file )
{
	std::wstring::size_type pos = 0;

	std::wstring res(file);

	do {
		pos = find_function( L"MlM", res, pos );

		if( pos == std::string::npos )
			return res;

		CPPDEBUG( format( "found MLM at line %d", get_linenum(res,pos) ));


		std::wstring line_before = get_whole_line( res, pos );
		if( line_before.find( L"LsMessage") != std::string::npos ) {
			//  LsMessage(MlM("Seite %ld von %ld")) is allowed
			pos += 3;
			continue;
		}

		Function func;
		std::wstring::size_type start, end;

		if( get_function( res, pos, start, end, &func ) ) {

			//DEBUG( "loaded function" );

			if( func.args.size()  > 0 ) {
				const std::wstring & arg = func.args[0];

				std::wstring::size_type pos_fmt = arg.find('%');

				if( pos_fmt != std::wstring::npos ) {
/*
					DEBUG( format("format string: %s", arg ) );
					DEBUG( format("found format string at ", pos_fmt ) );
*/
					if( pos_fmt + 1 < arg.size() ) {
						// allow %%
						if( arg[pos_fmt+1] != '%' ) {

							// make MlMsg out of it
							res.insert(pos+3,L"sg");
							pos+=2;

							std::wstring after = get_whole_line( res, pos );

							CPPDEBUG( format( "line %d\n- %s\n+ %s",
									get_linenum(res,pos),
									w2out(line_before), w2out(after)) );
						}
					}
				} // if
			} // if
		} // if get_funtction

		pos += 3;

	} while( pos != std::wstring::npos && pos < res.size() );

	return res;
}


bool FixMlM::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}


