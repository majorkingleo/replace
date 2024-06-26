#include <iostream>
#include <vector>
#include <fstream>
#include <errno.h>
#include <algorithm>

#include "range.h"
#include "string_utils.h"
#include "xml.h"
#include "format.h"
#include "stderr_exception.h"
#include "find_files.h"
#include "remove_versid_ch.h"
#include <arg.h>
#include "file_option.h"
#include "getline.h"
#include "remove_versid_pdl.h"
#include "remove_versid_rc.h"
#include "remove_versid_pl.h"

#include "OutDebug.h"
#include "remove_generic_cast.h"
#include "fix_from_compile_log.h"
#include "ColoredOutput.h"
#include "fix_sprintf.h"
#include "fix_c_headers.h"
#include "add_cast.h"
#include "DetectLocale.h"
#include "read_file.h"
#include "utf8_util.h"
#include "reset_versid.h"
#include <sys/stat.h>
#include <string.h>

using namespace Tools;

static std::string patch_file( const std::string & file, std::string search, std::string repl )
{
  return substitude( file, search, repl );
}

static std::wstring patch_file( const std::wstring & file, std::wstring search, std::wstring repl )
{
  return substitude( file, search, repl );
}

static void usage( const std::string & prog )
{
  std::cerr << "usage: "
			<< prog << " PATH -replace WHAT WITH [-doit]\n";
  std::cerr << "  eg: " 
			<< prog 
			<< " . -replace \"sdbm/\" # this finds all includes with sdbm in path\n"
			<< "                      # and removes than in preview mode\n";
  std::cerr << "  eg: " 
			<< prog
			<< " . -replace \"sdbm/\" -doit # this finds all includes with sdbm in path\n"
			<< "                            # and removes realy\n";
  std::cerr << "  eg: " 
			<< prog
			<< " . -replace \"sdbm/\" \"foobar\" -doit # this finds all includes with sdbm in path\n"
			<< "                                       # and removes them with foobar\n";
  std::cerr << "  eg: "
			<< prog
			<< " . -add-mlm -function-name maskSetMessage -function-arg 3 -function-call MlMsg\n"
			<< "            # will replace maskSetMessage(mask, 'E', \"Bitte Auslagerauftragsnummer angeben!\");\n"
			<< "            # with         maskSetMessage(mask, 'E', MlMsg(\"Bitte Auslagerauftragsnummer angeben!\"));\n";

  std::cerr << "  eg: "
 			<< prog
 			<< " . -fix-long -function-name KpIdx2Str -function-arg 1\n"
 			<< "            # will search for &KpIdx in pc = KpIdx2Str(&KpIdx, NULL);\n"
 			<< "            # and if it is an int it will be replaced into a long\n";

  std::cerr << "usage: "
  			<< prog << " PATH -remove-versid [-doit]\n";
}

static std::wstring unescape( std::wstring search, const std::wstring & what, const std::wstring & with )
{
	std::string::size_type pos = 0;

	do {
		pos = search.find( what, pos );

		if( pos == std::string::npos ) {
			return search;
		}

		if( pos == 0 )
		{
			std::wstring left = search.substr( 0, pos );
			std::wstring right = search.substr( pos + what.size() );
			search = left + with + right;
			pos += with.size();
		}

	} while( pos < search.size() && pos != std::string::npos );

	return search;
}

struct EnumMode
{
	enum ETYPE
	{
		FIRST__=-1,
		REPLACE,
		REMOVE_VERSID,
		LAST__
	};
};

typedef Tools::EnumRange<EnumMode> EMODE;

class Co : public ColoredOutput
{
public:
	std::string good( const std::string & message )
	{
		return color_output( ColoredOutput::GREEN, message );
	}

	std::string bad( const std::string & message )
	{
		return color_output( ColoredOutput::BRIGHT_RED, message );
	}
};

void usage2()
{
	Co co;

	std::cout << "\n"
			  << " Replace switches that are colored in " << co.good("green") << " can be used project wide.\n"
			  << " They are safe to use, you will get a compile time error if something bad happens.\n"
			  << "\n"
			  << " Replace switches that are colored in " << co.bad("red red red RED") << " can result in " << co.bad( "dangerous" ) << "\n"
			  << " behaviour. They can affect busness logic and should ony be used in subdirectories.\n"
			  << " Every changed line of code should be checked " << co.bad("twice!!")
			  << "\n"
			  << std::endl;
}

int main( int argc, char **argv )
{

#ifdef WIN32
  std::cout << "sorry this programm can't run on WIN32; Use a unix system!\n";
  return 1;
#endif

  Co co;

  try {


  FILE_SEARCH_LIST files;
  
  std::string mode;
  EMODE emode;

  Arg::Arg arg( argc, argv );

  arg.addPrefix( "-" );
  arg.addPrefix( "--" );

  arg.setMinMatch(1);

  Arg::OptionChain oc_info;
  arg.addChainR( &oc_info );
  oc_info.setMinMatch( 1 );
  oc_info.setContinueOnMatch( false );
  oc_info.setContinueOnFail( true );

  Arg::FlagOption o_help( "h" );
  o_help.addName( "help" );
  o_help.setDescription( "Show this page" );
  oc_info.addOptionR( &o_help );

  Arg::FlagOption o_version( "v" );
  o_version.addName( "version" );
  o_version.setDescription( "Show replace version number" );
  oc_info.addOptionR( &o_version );

  Arg::OptionChain oc_path;
  arg.addChainR(&oc_path);
  oc_path.setMinMatch(1);
  oc_path.setContinueOnFail(true);
  oc_path.setContinueOnMatch(true);

  Arg::EmptyFileOption o_path(Arg::FILE::DIR);
  oc_path.addOptionR(&o_path);
  o_path.setMaxValues(1);
  o_path.setMinValues(1);
  o_path.setRequired(true);

  Arg::FlagOption o_doit("doit");
  o_doit.setDescription("finaly really do the changes");
  o_doit.setRequired(false);
  arg.addOptionR( &o_doit );

  Arg::FlagOption o_debug("debug");
  o_debug.setDescription("print debugging messages");
  o_debug.setRequired(false);
  arg.addOptionR( &o_debug );

  Arg::FlagOption o_diff("diff");
  o_diff.setDescription("show diff");
  o_diff.setRequired(false);
  arg.addOptionR( &o_diff );


  Arg::StringOption o_ignore_directories("ignore-dirs");
  o_ignore_directories.setDescription("ignore directories (default: prod CSV .svn .git)");
  o_ignore_directories.setRequired(false);
  arg.addOptionR( &o_ignore_directories );

  Arg::StringOption o_backup_file_suffix("backup-file-suffix");
  o_backup_file_suffix.setDescription("default: .save");
  o_backup_file_suffix.setRequired(false);
  arg.addOptionR( &o_backup_file_suffix );


  // REPLACE CHAIN

  Arg::OptionChain oc_replace;
  arg.addChainR(&oc_replace);
  oc_replace.setMinMatch(1);
  oc_replace.setContinueOnFail(true);
  oc_replace.setContinueOnMatch(true);

  Arg::StringOption o_replace("replace");
  oc_replace.addOptionR(&o_replace);
  o_replace.setDescription("WHAT WITH");
  o_replace.setMaxValues(2);
  o_replace.setMinValues(2);

  // REMOVE Versid Chain

   Arg::OptionChain oc_versid;
   arg.addChainR(&oc_versid);
   oc_versid.setMinMatch(1);
   oc_versid.setContinueOnFail(true);
   oc_versid.setContinueOnMatch(true);

   Arg::FlagOption o_versid_remove("remove-versid");
   o_versid_remove.setDescription(co.good("remove versid infos"));
   o_versid_remove.setRequired(true);
   oc_versid.addOptionR( &o_versid_remove );

   Arg::FlagOption o_noheader( "noheader" );
   o_noheader.setDescription("do not insert the Salomon default header");
   oc_versid.addOptionR(&o_noheader);


   Arg::OptionChain oc_primanlist;
   arg.addChainR(&oc_primanlist);
   oc_primanlist.setMinMatch(1);
   oc_primanlist.setContinueOnFail(true);
   oc_primanlist.setContinueOnMatch(true);

     Arg::OptionChain oc_assign;
     arg.addChainR(&oc_assign);
     oc_assign.setMinMatch(1);
     oc_assign.setContinueOnFail(true);
     oc_assign.setContinueOnMatch(true);

     Arg::OptionChain oc_remove_generic_cast;
     arg.addChainR(&oc_remove_generic_cast);
     oc_remove_generic_cast.setMinMatch(1);
     oc_remove_generic_cast.setContinueOnFail(true);
     oc_remove_generic_cast.setContinueOnMatch(true);

     Arg::FlagOption o_remove_generic_cast("genericcast");
     o_remove_generic_cast.setDescription(co.good("correct (MskTgeneric *) casts in MskVaAssign to reduce warnings"));
     o_remove_generic_cast.setRequired(true);
     oc_remove_generic_cast.addOptionR(&o_remove_generic_cast);
     
	 Arg::OptionChain oc_addcast;
     arg.addChainR(&oc_addcast);
     oc_addcast.setMinMatch(1);
     oc_addcast.setContinueOnFail(true);
     oc_addcast.setContinueOnMatch(true);

     Arg::FlagOption o_addcast("addcast");
     o_addcast.setDescription(co.bad("adds cast to ArrWalkXxx, ArrGetXxx, malloc and realloc calls that are required after moving .c to .cc File"));
     o_addcast.setRequired(true);
     oc_addcast.addOptionR(&o_addcast);

     Arg::OptionChain oc_fix_warnings_from_compile_log;
     arg.addChainR(&oc_fix_warnings_from_compile_log);
     oc_fix_warnings_from_compile_log.setMinMatch(2);
     oc_fix_warnings_from_compile_log.setContinueOnFail(true);
     oc_fix_warnings_from_compile_log.setContinueOnMatch(true);

     Arg::FileOption o_compile_log("compile-log");
     oc_fix_warnings_from_compile_log.addOptionR(&o_compile_log);
     o_compile_log.setMaxValues(1);
     o_compile_log.setMinValues(1);
     o_compile_log.setRequired(true);
     o_compile_log.setDescription("Compiler output logfile. Create it by using 'umake clean && umake > compile.log 2>&1'");

     Arg::FlagOption o_comment_out("comment-out");
     o_comment_out.setDescription("do not remove code, just comment it out");
     o_comment_out.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_comment_out);

     Arg::FlagOption o_remove_unused_variables("unused-variable");
     o_remove_unused_variables.setDescription(co.bad("remove unused variables (can be combined with --comment-out)"));
     o_remove_unused_variables.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_remove_unused_variables);


     Arg::FlagOption o_initialize_variables("initialize-variable");
     o_initialize_variables.setDescription(co.bad("assign zero to uninitialized variables. Fix compiler warning: 'warning: ‘mam’ may be used uninitialized in this function'"));
     o_initialize_variables.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_initialize_variables);

     Arg::FlagOption o_format_string("format-string");
     o_format_string.setDescription(co.bad("autofix format string warnings"));
     o_format_string.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_format_string);

     Arg::FlagOption o_implicit("implicit");
     o_implicit.setDescription(co.good("autoinclude implicit declared functions"));
     o_implicit.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_implicit);


     Arg::FlagOption o_space_between_literal("literal");
     o_space_between_literal.setDescription(co.bad("fix \"warning: invalid suffix on literal; C++11 requires a space between literal and identifier [-Wliteral-suffix]\""));
     o_space_between_literal.setRequired(false);
     oc_fix_warnings_from_compile_log.addOptionR(&o_space_between_literal);



     Arg::OptionChain oc_sprintf;
     arg.addChainR(&oc_sprintf);
     oc_sprintf.setMinMatch(1);
     oc_sprintf.setContinueOnFail(true);
     oc_sprintf.setContinueOnMatch(true);

      Arg::FlagOption o_sprintf("sprintf");
      o_sprintf.setDescription(co.bad("replace sprintf() with StrCpy( dest, format(...)) in .cc files"));
      o_sprintf.setRequired(true);
      oc_sprintf.addOptionR(&o_sprintf);

      Arg::OptionChain oc_strform;
      arg.addChainR(&oc_strform);
      oc_strform.setMinMatch(1);
      oc_strform.setContinueOnFail(true);
      oc_strform.setContinueOnMatch(true);

      Arg::OptionChain oc_fix_c_header_file;
      arg.addChainR(&oc_fix_c_header_file);
      oc_fix_c_header_file.setMinMatch(1);
      oc_fix_c_header_file.setContinueOnFail(true);
      oc_fix_c_header_file.setContinueOnMatch(true);

      Arg::FlagOption o_fix_c_header_file("fix-c-header");
      o_fix_c_header_file.setDescription(co.good("add extern \"C\" to C header files."));
      o_fix_c_header_file.setRequired(true);
      oc_fix_c_header_file.addOptionR(&o_fix_c_header_file);

      Arg::OptionChain oc_reset_versid;
      arg.addChainR(&oc_reset_versid);
      oc_reset_versid.setMinMatch(1);
      oc_reset_versid.setContinueOnFail(true);
      oc_reset_versid.setContinueOnMatch(true);

      Arg::FlagOption o_reset_versid("reset-versid");
      o_reset_versid.setDescription(co.good("resets versids to $Header$, like it is in cvs repo"));
      o_reset_versid.setRequired(true);
      oc_reset_versid.addOptionR( &o_reset_versid );

  DetectLocale dl;

  const unsigned int console_width = 80;

  if( !arg.parse() || argc <= 1 )
    {
      if( o_version.getState() )
      {
    	  std::cout << format("%s version %s\n", argv[0], VERSION);
    	  return 0;
      } else {
    	  usage(argv[0]);
    	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;
    	  usage2();
    	  return 1;
      }
    }

  if( o_help.getState() )
  {
	  usage(argv[0]);
	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;
	  usage2();
	  return 0;
  }


  if( o_debug.getState() )
  {
	  Tools::x_debug = new OutDebug();
  }

  if( !o_path.isSet() )
  {
	  usage(argv[0]);
	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;

	  std::cout << "\n\n"
			  	<< co.color_output( ColoredOutput::BRIGHT_RED, "PATH is missing")
			  	<< "\n";

	  return 13;
  }

  if( !o_replace.isSet() &&
	  !o_versid_remove.isSet() &&
	  !o_reset_versid.isSet() &&
	  !o_remove_generic_cast.isSet() &&
	  !o_addcast.isSet() &&
	  !o_compile_log.isSet() &&
	  !o_sprintf.isSet() &&
	  !o_fix_c_header_file.isSet())
  {
	  usage(argv[0]);
	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;
	  return 1;
  }

  std::set<std::string> directories_to_ignore = {
		  "CVS",
		  ".svn",
		  ".git"
  };

  if( o_ignore_directories.isSet() ) {
	  directories_to_ignore.clear();

	  std::copy_if( o_ignore_directories.getValues()->begin(),
			  	    o_ignore_directories.getValues()->end(),
					std::inserter(directories_to_ignore,directories_to_ignore.end()),
					[]( std::string & val ) { return !val.empty(); } );

	  if( directories_to_ignore.empty() ) {
		  std::cout <<  co.color_output( co.YELLOW, dl.wString2output( L"searching in all directories" ) ) << std::endl;
	  } else {
		  std::wstring message = L"ignoring directories: ";

		  if( directories_to_ignore.size() == 1 ) {
			  message = L"ignoring directory: ";
		  }

		  std::string dirs = IterableToFormattedString( directories_to_ignore, ", ", "", -1, 10, "..." );
		  message += dl.inputString2wString( dirs );

		  std::cout << co.color_output( co.YELLOW, dl.wString2output( message ) ) << std::endl;
	  }
  }

  std::wstring backup_suffix = L".save";

  if( o_backup_file_suffix.isSet() ) {
	  backup_suffix = dl.inputString2wString( o_backup_file_suffix.getValues()->at(0) );
  }

  std::wstring search = L"";
  std::wstring replace = L"";
  bool doit =  o_doit.getState();
  bool show_diff = false;

  if( o_compile_log.isSet() )
  {

	  try {
		  FixFromCompileLog fix_from_log(
				  o_path.getValues()->at(0),
				  o_compile_log.getValues()->at(0),
				  o_comment_out.isSet(),
				  o_remove_unused_variables.isSet(),
				  o_initialize_variables.isSet(),
				  o_format_string.isSet(),
				  o_implicit.isSet(),
				  o_space_between_literal.isSet(),
				  directories_to_ignore,
				  backup_suffix );

		  fix_from_log.run();

		  if( doit )
		  {
			  fix_from_log.doit();
		  }

	  } catch( const StderrException & err ) {
		  std::cerr << err.get_simple_error() << std::endl;
		  return 11;
	  }

	  return 0;
  }

  if( o_replace.isSet() )
  {
	  search = dl.inputString2wString( o_replace.getValues()->at(0) );

	  search = unescape( search, L"\\t", L"\t" );
	  search = unescape( search, L"\\n", L"\n" );

	  CPPDEBUG( format(  "unescaped search string: '%s'", dl.wString2output( search ) ) );;

	  replace = dl.inputString2wString( o_replace.getValues()->at(1) );

	  replace = unescape( replace, L"\\t", L"\t" );
	  replace = unescape( replace, L"\\n", L"\n" );

	  CPPDEBUG( format(  "unescaped replace string: '%s'", dl.wString2output( replace ) ) );;

	  show_diff = true;
  }

  if( o_fix_c_header_file.isSet() )
  {
	  FixCHeaders fix;
	  return fix.main( o_path.getValues()->at(0), directories_to_ignore );
  }

  if( !find_files( o_path.getValues()->at(0), files, directories_to_ignore ) )
  {
	  std::cerr << "nothing found" << std::endl;
	  return 0;
  }
  
  std::vector<Ref<HandleFile> > handlers;

  if( o_versid_remove.getState() ) {

	  handlers.push_back( new RemoveVersidPdl() );
	  handlers.push_back( new RemoveVersidRc() );
	  handlers.push_back( new RemoveVersidCh(o_noheader.getState()) );
      handlers.push_back( new RemoveVersidPl() );

  }

  if( o_reset_versid.getState() ) {
	  handlers.push_back( new ResetVersid() );
  }

  if( o_sprintf.getState() ) {
	  handlers.push_back( new FixSprintf() );
  }
  if( o_remove_generic_cast.getState() ) {
	  handlers.push_back( new RemoveGenericCast() );
	  handlers.push_back( new RemoveGenericCast( L"MskUpdateVar") );
	  handlers.push_back( new RemoveGenericCast( L"MskQueryRl") );
	  handlers.push_back( new RemoveGenericCast( L"LmskGetVar") );
  }
  if (o_addcast.getState() ) {
	  handlers.push_back( new AddCast() );
	  handlers.push_back( new AddCast(L"ArrWalkNext") );
	  handlers.push_back( new AddCast(L"ArrGetEle") );
	  handlers.push_back( new AddCast(L"malloc") );
	  handlers.push_back( new AddCast(L"realloc") );
  }

  for( FILE_SEARCH_LIST::iterator it = files.begin(); it != files.end(); it++ )
	{
	  // std::string file;
	  FILE_TYPE file_type = it->getType();
	  bool is_cpp_file = it->isCppFile();

	  /*
	  if( !XML::read_file( it->getPath(), file ) )
		{
		  std::cerr << "cannot open file: " << it->getPath() << std::endl;
		  continue;
		}
	  */
	  
	  std::wstring wfile;
	  std::string encoding;

	  if( !READ_FILE.read_file( it->getPath(), wfile ) )
	    {
	      std::cerr << "cannot open or convert file: " << it->getPath() << " Error: " <<  READ_FILE.getError() <<  std::endl;
	      continue;
	    }	  

	  encoding = READ_FILE.getFileEncoding();

	  CPPDEBUG( format( "working on file %s encoding: %s",  it->getPath(), encoding ) );

	  std::wstring file_erg = wfile;

	  switch( file_type )
	  {
	  case FILE_TYPE::RC_FILE:
		  if( o_replace.isSet() )
		  	  file_erg = patch_file( file_erg, search, replace );

		  break;

	  case FILE_TYPE::HEADER:
	  case FILE_TYPE::C_FILE:

		  if( o_replace.isSet() )
			  file_erg = patch_file( file_erg, search , replace );

		  break;

	  default:
		  break;
	  }

	  try {
		  for( unsigned j = 0; j < handlers.size(); j++ )
		  {
			  if( handlers[j]->want_file_ext( file_type, is_cpp_file ) ) {
				  handlers[j]->set_file_name( it->getPath() );
				  file_erg = handlers[j]->patch_file( file_erg );
			  }
		  }

		  if( file_erg != wfile )
		  {
			  std::cout << "patching file " << it->getPath() << std::endl;

			  if( show_diff || o_diff.getState() ) {
				  std::wstring diff = diff_lines( wfile, file_erg );

				  std::string utf8_result = Utf8Util::wStringToUtf8( diff );

				  std::cout << READ_FILE.convert( utf8_result, "UTF-8", dl.getInputEncoding() ) << std::endl;
			  }
		  }
		  else
		  {
			  continue;
		  }
	  } catch( const StderrException & err ) {
		  std::cerr << "error: " << err.what() << std::endl;
		  continue;
	  }

	  if( doit )
		{
		  // keep file mode
		  struct stat stat_buf = {};
		  if( stat( it->getPath().c_str(), &stat_buf ) == -1 ) {
			  std::cerr << "cannot stat file " << it->getPath() << " error: " << strerror(errno) << std::endl;
		  }

		  std::string backup_file = it->getPath() + dl.wString2output( backup_suffix );

		  if( !backup_suffix.empty() ) {
			  if( rename( it->getPath().c_str(), backup_file.c_str() ) != 0 ) {
				  std::cerr << strerror(errno) << std::endl;
			  }
		  }

		  std::ofstream out( it->getPath().c_str(), std::ios_base::trunc );
		  
		  if( !out )
			{
			  std::cerr << "cannot overwrite file " << it->getPath() << std::endl;
			  continue;
			}
		  
		  std::string utf8_result = Utf8Util::wStringToUtf8( file_erg );

		  out << READ_FILE.convert( utf8_result, "UTF-8", encoding );

		  out.close();

		  if( chmod(  it->getPath().c_str(), stat_buf.st_mode ) == -1 ) {
			  std::cerr << "cannot chmod of file " << it->getPath() << " error: " << strerror(errno) << std::endl;
		  }
		}
	}	  

  } catch( std::exception & err ) {
	  std::cerr << "error: " << err.what() << std::endl;
	  return 10;
  }

  return 0;
}
