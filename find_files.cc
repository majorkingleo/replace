#include "find_files.h"
#include <cppdir.h>

using namespace Tools;

bool find_files( const std::string & path, FILE_SEARCH_LIST & files )
{
  CppDir::File file( path );

  // avoid link loop
  if( file.is_link() && file.get_link_buf() == "." )
    return false;

  if( !file )
    return false;

  if( file.get_type() == CppDir::EFILE::DIR )
    {
      CppDir::Directory dir( file );

      CppDir::Directory::file_list fl = dir.get_files();

      for( CppDir::Directory::file_list_it it = fl.begin(); it != fl.end(); it++ )
        {
          if( it->get_name() == "." ||
        	   it->get_name() == ".." ||
        	   it->get_name() == "CVS" ||
        	   it->get_name() == ".svn")
            continue;

          // std::cout << "path: " << it->get_path() << " name: " << it->get_name() << std::endl;
          if( !find_files( CppDir::concat_dir( it->get_path(), it->get_name() ), files ) )
            {
                std::cerr << "cannot open file: "
                          << CppDir::concat_dir( it->get_path(), it->get_name() )
                          << std::endl;
                continue;
            }
        }
    }
  else if( !file.is_link() )
	{
	  bool is_cpp_file = false;
	  FILE_TYPE file_type = get_file_type( file.get_name(), is_cpp_file );

	  if( file_type != FILE_TYPE::UNKNOWN ) {
		// std::cout << file.get_name() << " is_cpp_file: " << is_cpp_file << std::endl;
		files.push_back( Files( CppDir::concat_dir( file.get_path(), file.get_name() ), file_type, is_cpp_file ) );
	  }
    }

  return true;
}
