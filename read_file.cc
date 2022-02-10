/*
 * read_file.cc
 *
 *  Created on: 10.02.2022
 *      Author: martin
 */
#include "read_file.h"
#include <debug.h>
#include <xml.h>
#include <utf8_util.h>
#include <iconv.h>
#include <cstring>
#include <cpp_util.h>

using namespace Tools;

ReadFile::ReadFile()
: error(),
  encoding()
{

}

bool ReadFile::read_file( const std::string & name, std::wstring & content )
{
	std::string file_content;

	if( !XML::read_file( name, file_content ) ) {
		error = "cannot open file";
		return false;
	}

	if( Utf8Util::isUtf8( file_content ) ) {
		content = Utf8Util::utf8toWString( file_content );
		encoding = "UTF-8";
		return true;
	}

	std::string convertet_file_content;

	if( convert( file_content, "LATIN1", "UTF-8", convertet_file_content ) ) {
		if( Utf8Util::isUtf8( convertet_file_content ) ) {
			content = Utf8Util::utf8toWString( convertet_file_content );
			encoding = "LATIN1";
			return true;
		}
	}

	error = "cannot convert file";

	DEBUG( "cannot convert file" );

	return false;
}

std::string ReadFile::convert( const std::string & s, const std::string & from, const std::string & to )
{
	std::string result;

	if( !convert( s, from, to, result ) ) {
		throw REPORT_EXCEPTION( error );
	}

	return result;
}

bool ReadFile::convert( const std::string & s, const std::string & from, const std::string & to, std::string & result )
{
    std::string ret;

    int extra_bufsize = 50;
    bool cont = false;

    iconv_t h = iconv_open( to.c_str(), from.c_str() );

    if( h == (iconv_t)-1 ) {
        return false;
    }

    do
      {
        cont = false;

        char *out_buffer = new char[s.size()+extra_bufsize];
        char *in_buffer = new char[s.size()+1];

        memcpy( in_buffer, s.c_str(), s.size() );

        in_buffer[s.size()]='\0';

        size_t size, in_left=s.size()+1, out_left=s.size()+extra_bufsize;
        char *in, *out;

        in = in_buffer;
        out = out_buffer;


        size = iconv( h, &in, &in_left, &out, &out_left );

        if( size != (size_t)-1 )
          {
            ret = out_buffer;
          } else if( errno == E2BIG ) {
            extra_bufsize*=2;
            cont = true;
            ret = "E2BIG";
          } else if( errno == EILSEQ ) {
        	error = format( "EILSEQ: %s", strerror(errno));
            return false;
          } else if( errno == EINVAL ) {
            error = format( "EINVAL: %s", strerror(errno));
            return false;
          }

        delete[] out_buffer;
        delete[] in_buffer;

      } while( cont == true );

    iconv_close( h );

    result = ret;

    return true;
}

