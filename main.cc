#include <iostream>
#include <vector>
#include <fstream>
#include <errno.h>

#include "range.h"
#include "string_utils.h"
#include "xml.h"
#include "format.h"
#include "cpp_util.h"
#include "find_files.h"
#include "remove_versid_ch.h"
#include <arg.h>
#include "file_option.h"
#include "getline.h"
#include "remove_versid_pdl.h"
#include "remove_versid_rc.h"
#include "remove_versid_pl.h"
#include "PrimanList.h"
#include "RestoreShell.h"

#include "OutDebug.h"
#include "OwCallback1.h"
#include "fix_mlm.h"
#include "correct_va_multiple_malloc.h"
#include "remove_generic_cast.h"

using namespace Tools;

std::string patch_file( const std::string & file, std::string search, std::string repl )
{
  return substitude( file, search, repl );
}

std::string diff_lines( const std::string & orig, std::string & modded )
{
  std::vector<std::string> sl_orig, sl_modded;

  sl_orig = split_simple( orig, "\n" );
  sl_modded = split_simple( modded, "\n" );
  
  std::string res;

  for( unsigned i = 0; i < sl_orig.size() && i < sl_modded.size(); i++ )
	{
	  if( sl_orig[i] != sl_modded[i] )
		{
		  if( !res.empty() )
			res += '\n';

		  res += "\t" + strip( sl_orig[i] ) + " => " + strip( sl_modded[i] );
		}
	}

  return res;
}

void usage( const std::string & prog )
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

  std::cerr << "usage: "
  			<< prog << " PATH -remove-versid [-doit]\n";
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

int main( int argc, char **argv )
{

#ifdef WIN32
  std::cout << "sorry this programm can't run on WIN32; Use a unix system!\n";
  return 1;
#endif

  try {

  std::vector<std::pair<FILE_TYPE,std::string> > files;
  
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
   o_versid_remove.setDescription("remove versid infos");
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

   Arg::FlagOption o_primanlist("primanlist");
   o_primanlist.setDescription("add primanlist callbacks to Salomon lists");
   o_primanlist.setRequired(true);
   oc_primanlist.addOptionR(&o_primanlist);

   Arg::OptionChain oc_restoreshell;
   arg.addChainR(&oc_restoreshell);
   oc_restoreshell.setMinMatch(1);
   oc_restoreshell.setContinueOnFail(true);
   oc_restoreshell.setContinueOnMatch(true);

   Arg::FlagOption o_restoreshell("restoreshell");
   o_restoreshell.setDescription("add restore GuiNrestoreShell shell and GuiNmakeActiveShell to lists");
   o_restoreshell.setRequired(true);
   oc_restoreshell.addOptionR(&o_restoreshell);

   Arg::OptionChain oc_owcallback;
   arg.addChainR(&oc_owcallback);
   oc_owcallback.setMinMatch(1);
   oc_owcallback.setContinueOnFail(true);
   oc_owcallback.setContinueOnMatch(true);

    Arg::FlagOption o_owcallback("owcallback");
    o_owcallback.setDescription("correct missing OwCallback1");
    o_owcallback.setRequired(true);
    oc_owcallback.addOptionR(&o_owcallback);

    Arg::OptionChain oc_mlm;
    arg.addChainR(&oc_mlm);
    oc_mlm.setMinMatch(1);
    oc_mlm.setContinueOnFail(true);
    oc_mlm.setContinueOnMatch(true);

     Arg::FlagOption o_mlm("mlm");
     o_mlm.setDescription("correct MlM(%d) calls to MlMsg(%d)");
     o_mlm.setRequired(true);
     oc_mlm.addOptionR(&o_mlm);

     Arg::OptionChain oc_correct_va_multiple_malloc;
     arg.addChainR(&oc_correct_va_multiple_malloc);
     oc_correct_va_multiple_malloc.setMinMatch(1);
     oc_correct_va_multiple_malloc.setContinueOnFail(true);
     oc_correct_va_multiple_malloc.setContinueOnMatch(true);

     Arg::FlagOption o_correct_va_multiple_malloc("vamulmalloc");
     o_correct_va_multiple_malloc.setDescription("correct (void**) casts in VaMultipleMalloc to reduce warnings");
     o_correct_va_multiple_malloc.setRequired(true);
     oc_correct_va_multiple_malloc.addOptionR(&o_correct_va_multiple_malloc);

     Arg::OptionChain oc_remove_generic_cast;
     arg.addChainR(&oc_remove_generic_cast);
     oc_remove_generic_cast.setMinMatch(1);
     oc_remove_generic_cast.setContinueOnFail(true);
     oc_remove_generic_cast.setContinueOnMatch(true);

     Arg::FlagOption o_remove_generic_cast("genericcast");
     o_remove_generic_cast.setDescription("correct (MskTgeneric *) casts in MskVaAssign to reduce warnings");
     o_remove_generic_cast.setRequired(true);
     oc_remove_generic_cast.addOptionR(&o_remove_generic_cast);

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
    	  return 1;
      }
    }

  if( o_help.getState() )
  {
	  usage(argv[0]);
	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;
	  return 0;
  }

  if( o_debug.getState() )
  {
	  Tools::x_debug = new OutDebug();
  }

  if( !o_replace.isSet() &&
	  !o_versid_remove.isSet() &&
	  !o_primanlist.isSet() &&
	  !o_mlm.isSet() &&
	  !o_restoreshell.isSet() &&
	  !o_correct_va_multiple_malloc.isSet() &&
	  !o_remove_generic_cast.isSet() &&
	  !o_owcallback.isSet())
  {
	  usage(argv[0]);
	  std::cout << arg.getHelp(5,20,30, console_width ) << std::endl;
	  return 1;
  }

  std::string search = "";
  std::string replace = "";
  bool doit =  o_doit.getState();
  bool show_diff = false;

  if( o_replace.isSet() )
  {
	  search = o_replace.getValues()->at(0);
	  replace = o_replace.getValues()->at(1);
	  show_diff = true;
  }

  if( !find_files( o_path.getValues()->at(0), files ) )
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

  if( o_primanlist.getState() ) {
	  handlers.push_back( new PrimanList() );
  }

  if( o_owcallback.getState() ) {
	  handlers.push_back( new OwCallback1() );
  }

  if( o_restoreshell.getState() ) {
	  handlers.push_back( new RestoreShell() );
  }

  if( o_mlm.getState() )
	  handlers.push_back( new FixMlM() );

  if( o_correct_va_multiple_malloc.getState() ) {
	  handlers.push_back( new CorrectVaMultipleMalloc() );
  }

  if( o_remove_generic_cast.getState() ) {
	  handlers.push_back( new RemoveGenericCast() );
	  handlers.push_back( new RemoveGenericCast("MskUpdateVar") );
	  handlers.push_back( new RemoveGenericCast("MskQueryRl") );
	  handlers.push_back( new RemoveGenericCast("LmskGetVar") );
  }

  for( unsigned i = 0; i < files.size(); i++ )
	{
	  std::string file;
	  FILE_TYPE file_type = files[i].first;

	  if( !XML::read_file( files[i].second, file ) )
		{
		  std::cerr << "cannot open file: " << file << std::endl;
		  continue;
		}

	  DEBUG( format( "working on file %s", files[i].second ) );

	  std::string file_erg = file;

	  switch( file_type )
	  {
	  case FILE_TYPE::RC_FILE:
		  if( o_replace.isSet() )
		  	  file_erg = patch_file( file_erg, search , replace );

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
			  if( handlers[j]->want_file( file_type ) ) {
				  file_erg = handlers[j]->patch_file( file_erg );
			  }
		  }

		  if( file_erg != file )
		  {
			  std::cout << "patching file " << files[i].second << std::endl;

			  if( show_diff ) {
				  std::cout << diff_lines( file, file_erg ) << std::endl;
			  }
		  }
		  else
		  {
			  continue;
		  }
	  } catch( ReportException & err ) {
		  std::cerr << "error: " << err.what() << std::endl;
		  continue;
	  }

	  if( doit )
		{
		  if( rename( files[i].second.c_str(), TO_CHAR(files[i].second + ".save") ) != 0 )
			{
			  std::cerr << strerror(errno) << std::endl;
			}
		  std::ofstream out( files[i].second.c_str(), std::ios_base::trunc );
		  
		  if( !out )
			{
			  std::cerr << "cannot overwrite file " << files[i].second << std::endl;
			  continue;
			}
		  
		  out << file_erg;

		  out.close();
		}
	}	  

  } catch( std::exception & err ) {
	  std::cerr << "error: " << err.what() << std::endl;
	  return 10;
  }

  return 0;
}
