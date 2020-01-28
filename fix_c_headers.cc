#include <iostream>
#include <vector>
#include <fstream>
#include <stdlib.h>
#include "fix_c_headers.h"

#include "cppdir.h"
#include "range.h"
#include "string_utils.h"
#include "xml.h"
#include "format.h"
#include "find_files.h"
#include "colored_output.h"

using namespace Tools;

bool FixCHeaders::should_skip_file( const std::string & file )
{
  // Schlüsselworte anhan denen wir versuchen zu erkennen, ob dies vielleicht ein
  // C++ file ist, oder der Header file bereits gepatched wurde.

  const char  *key_words[] =
	{ "class", 
	  "vector", 
	  "std::" , 
	  "string", 
	  "extern \"C\"", 
	  "cplusplus", 
	  "template", 
	  "namespace",
	  "auto generated file",
	  "Do not edit",
	  "inline",
	  "explicit",
	  "public",
	  "private",
	  "protected",
	  "operator",
	  "virtual"	  
	};

  for( unsigned int i = 0; i < sizeof(key_words)/sizeof(key_words[0]); i++ )
	if( file.find( key_words[i]) != std::string::npos )
	  return true;

  return false;
}

int FixCHeaders::count_open_ifdef( const std::string & file, std::string::size_type end )
{
  int count_if = 0;
  int count_end = 0;

  std::string::size_type pos = 0;
  std::string::size_type start = 0;

  for( start = 0; start < end; count_if++ )
	{
	  pos = file.find( "#if", start );
	  
	  if( pos > end || pos == std::string::npos )
		break;

	  start = pos + 3;
	}

  for( start = 0; start < end; count_end++ )
	{
	  pos = file.find( "#endif", start );
	  
	  if( pos > end || pos == std::string::npos )
		break;

	  start = pos + 6;
	}

  int count = count_if - count_end;

  if( count < 0 )
	return 0;

  return count;
}


std::string FixCHeaders::patch_file( const std::string & file )
{
  std::string::size_type start = file.rfind( "#include" );

  std::string res;
  std::string middle;
  std::string end;

  res.reserve(file.size()+100);
  middle.reserve(file.size()+100);
  
  if( start != std::string::npos )
	{
	  // finde n�chste Zeile
	  for( ; start < file.size(); start++ )
		if( file[start] == '\n' )
		  {
			start++;
			break;
		  }
	}

  if( start == std::string::npos )
	{
	  start = file.rfind( "#ifndef" );

	  if( start != std::string::npos )
		{
		  // in der n�chsten Zeile sollte nun #define stehen

		  start = file.find( "#define", start );
		  
		  if( start == std::string::npos )
			start = 0;
		  else
			{
			  // zum Begin der n�chsten Zeile
			  for( ; start < file.size(); start++ )
				{
				  if( file[start] =='\n' )
					{
					  start++;
					  break;	
					}
				}
			}	  
		}
	}

  if( start != std::string::npos )
	{

	  // wie viele #if haben wir da
	  // genau so viele endif muessen
	  // wir dann nämlich suchen
	  int ifdefcount = count_open_ifdef( file, start );	  

	  if( start >= file.size() )
		{
		  res = file;
		}
	  else
		{
		  res = file.substr( 0, start );

		  // where should we place the end
		  std::string::size_type pend = std::string::npos;
		  std::string::size_type spos_start = file.rfind( "#endif" );
		  std::string::size_type pos = spos_start;

		  for( int i = 0; i < ifdefcount; i++ )
			{			  			  
			  if( pos < start || pos == std::string::npos )
				{
				  break;
				}

			  spos_start = pos;
			  pend = spos_start-1;

			  pos = file.rfind( "#endif", spos_start - 1);
			}		  
		  

		  middle = file.substr( start, pend - start );

		  if( pend != std::string::npos )
			end = file.substr( pend );
		}
	}
  else
	{
	  middle = file;
	}  

  res += 
	"\n#ifdef __cplusplus\n"
	" extern \"C\" {\n"
	"#endif\n\n";

  res += middle;

  res += 
	"\n#ifdef __cplusplus\n"
	"}\n"
	"#endif\n\n";

  res += end;

  return res;
}

std::string FixCHeaders::diff_file( const std::string & orig, const std::string & patched )
{
  std::string cmd = "diff -u " + orig + " " + patched;

  FILE *f = popen( cmd.c_str(), "r" );

  if( f == NULL )
	{
	  std::cerr << "command: " << cmd << " failed\n";
	  return std::string();
	}

  std::string res;
  char acBuffer[100];

  while( !feof(f) )
	{
	  size_t len = fread( acBuffer, 1, sizeof(acBuffer)-1, f);
	  acBuffer[len] = '\0';
	  res += acBuffer;
	}
  
  pclose(f);

  return res;
}

std::string FixCHeaders::build_func_name( const std::string & file )
{
  std::string res = "patch_" + substitude(file, "/", "_" );

  res = substitude(res, ".", "_" );

  return res;
}

int FixCHeaders::main( const std::string & path )
{
  ColoredOutput colored_output;
  FILE_SEARCH_LIST files;
 
  if( !find_files( path, files ) )
	{
	  std::cerr << "nothing found" << std::endl;
	  return 0;
	}
  
  static const std::string PATCH_FILE="patch_c_headers.sh";

  std::ofstream batch_file( PATCH_FILE.c_str(), std::ios_base::trunc );

  if( !batch_file )
	{
	  std::cerr << "cannot open " << PATCH_FILE << std::endl;
	  return 4;
	}

  batch_file << "#!/usr/bin/env bash\n\n";
  batch_file << "do_cvsedit=0\n\n";
  batch_file << "# uncomment the next line for unediting all\n";
  batch_file << "# do_unedit=1\n\n";

  std::vector<std::string> patched_files;

  const std::string tmp_file = "/tmp/patched_file.c";

  for( const Files & h_file : files )
	{
	  if( h_file.getType() != FILE_TYPE::HEADER ) {
		  continue;
	  }

	  std::string file;

	  if( !XML::read_file( h_file.getPath(), file ) )
		{
		  std::cerr << "cannot open file: " << file << std::endl;
		  continue;
		}

	  if( should_skip_file( file ) )
		{
		  continue;
		}

	  file = patch_file( file );

	  std::ofstream out( tmp_file.c_str(), std::ios_base::trunc );

	  if( !out )
		{
		  std::cerr << "cannot open temp file " << tmp_file << std::endl;
		  continue;
		}

	  out << file;

	  out.close();
	  
	  std::string diff = diff_file(  h_file.getPath(),  tmp_file );

	  batch_file << build_func_name( h_file.getPath() )
				 << " ()\n{\n"
		//				 << " echo '" << files[i] << "'\n"
				 << " if test \"${do_cvsedit}XX\" = \"1XX\" ; then\n"
				 << "     cvs edit " <<  h_file.getPath() << "\n"
				 << " fi\n\n"
				 << " patch -p1 <<FILE_PATCH\n"
				 << diff
				 << "FILE_PATCH\n"
				 <<"}\n\n";

	  patched_files.push_back( h_file.getPath() );

	  std::cout << h_file.getPath() << std::endl;
	}

  batch_file << "\n\nfunction patch_all ()\n"
			 << "{\n";

  for( unsigned int i = 0 ; i < patched_files.size(); i++ )
	{
	  batch_file << build_func_name(  patched_files[i] ) << '\n';
	}  

  batch_file << " exit 0\n" 
			 <<	"}\n\n";

  batch_file << "\n\nfunction unedit_all ()\n"
			 << "{\n";
			 
  for( unsigned int i = 0 ; i < patched_files.size(); i++ )
	{
	  batch_file << "  echo '" << patched_files[i] << "'\n";
	  batch_file << "  cvs unedit " <<  patched_files[i] << "<<!\ny\n!\n";
	}

  batch_file << " exit 0\n" 
			 <<	"}\n\n";

  batch_file << "if test \"${do_unedit}XX\" = \"1XX\" ; then\n"
			 <<	" unedit_all\n"
			 << "else\n"
			 << " patch_all\n"
			 << "fi\n\n";


  batch_file.close();

  //  unlink( tmp_file.c_str() );

  std::string cmd = format("chmod a+x %s", PATCH_FILE);

  system( cmd.c_str() );

  std::cout << "generated: " << colored_output.color_output( ColoredOutput::BRIGHT_GREEN,  PATCH_FILE ) << std::endl;

  return 0;
}
