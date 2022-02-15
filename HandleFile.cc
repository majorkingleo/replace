#include "HandleFile.h"
#include "DetectLocale.h"

HandleFile::HandleFile()
: keywords()
{

}

HandleFile::~HandleFile()
{

}

bool HandleFile::should_skip_file( const std::wstring & file ) const
{
  // Schlüsselworte anhand denen wir versuchen zu erkennen, ob diese Datei verändert
  // werde soll..

  for( unsigned int i = 0; i < keywords.size(); i++ )
    if( file.find( keywords[i]) != std::string::npos )
      return false;

  return true;
}

std::string HandleFile::w2out( const std::wstring & out )
{
	return DETECT_LOCALE.wString2output( out );
}

std::wstring HandleFile::in2w( const std::string & in )
{
	return DETECT_LOCALE.inputString2wString( in );
}
