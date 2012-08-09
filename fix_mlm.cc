/*
 * fix_mlm.cc
 *
 *  Created on: 05.06.2012
 *      Author: martin
 */

#include "utils.h"
#include "fix_mlm.h"
#include <format.h>
#include "debug.h"

using namespace Tools;


FixMlM::FixMlM()
{

}

std::string FixMlM::patch_file( const std::string & file )
{
	std::string::size_type pos = 0;

	std::string res(file);

	do {
		pos = find_function( "MlM", res, pos );

		if( pos == std::string::npos )
			return res;

		DEBUG( format( "found MLM at line %d", get_linenum(res,pos) ));


		std::string line_before = get_whole_line( res, pos );
		if( line_before.find( "LsMessage") != std::string::npos ) {
			//  LsMessage(MlM("Seite %ld von %ld")) is allowed
			pos += 3;
			continue;
		}

		Function func;
		std::string::size_type start, end;

		if( get_function( res, pos, start, end, &func ) ) {

			//DEBUG( "loaded function" );

			if( func.args.size()  > 0 ) {
				const std::string & arg = func.args[0];

				std::string::size_type pos_fmt = arg.find('%');

				if( pos_fmt != std::string::npos ) {
/*
					DEBUG( format("format string: %s", arg ) );
					DEBUG( format("found format string at ", pos_fmt ) );
*/
					if( pos_fmt + 1 < arg.size() ) {
						// allow %%
						if( arg[pos_fmt+1] != '%' ) {

							// make MlMsg out of it
							res.insert(pos+3,"sg");
							pos+=2;

							std::string after = get_whole_line( res, pos );

							DEBUG( format( "line %d\n- %s\n+ %s",
									get_linenum(res,pos),
									line_before, after) );
						}
					}
				} // if
			} // if
		} // if get_funtction

		pos += 3;

	} while( pos != std::string::npos && pos < res.size() );

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


