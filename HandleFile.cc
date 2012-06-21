#include "HandleFile.h"

HandleFile::HandleFile()
: keywords()
{

}

HandleFile::~HandleFile()
{

}

bool HandleFile::should_skip_file( const std::string & file ) const
{
  // Schlüsselworte anhand denen wir versuchen zu erkennen, ob diese Datei verändert
  // werde soll..

  for( unsigned int i = 0; i < keywords.size(); i++ )
    if( file.find( keywords[i]) != std::string::npos )
      return false;

  return true;
}

