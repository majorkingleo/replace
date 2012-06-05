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

#include "OutDebug.h"
#include "OwCallback1.h"
#include "fix_mlm.h"

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
  
  Ref<RemoveVersid> remove_version_id_pdl;
  Ref<RemoveVersid> remove_version_id_rc;
  Ref<RemoveVersid> remove_version_id_ch;
  Ref<RemoveVersid> remove_version_id_pl;

  Ref<PrimanList> priman_list;
  Ref<OwCallback1> owcallback1;

  if( o_versid_remove.getState() ) {
	  remove_version_id_pdl = new RemoveVersidPdl();
	  remove_version_id_rc = new RemoveVersidRc();
	  remove_version_id_ch = new RemoveVersidCh(o_noheader.getState());
	  remove_version_id_pl = new RemoveVersidPl();
  }

  if( o_primanlist.getState() ) {
	  priman_list = new PrimanList();
  }

  if( o_owcallback.getState() ) {
	  owcallback1 = new OwCallback1();
  }

  Ref<FixMlM> fix_mlm;

  if( o_mlm.getState() )
	  fix_mlm = new FixMlM();

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

	  std::string file_erg;

	  switch( file_type )
	  {
	  case FILE_TYPE::RC_FILE:
		  if( o_replace.isSet() )
		  	  file_erg = patch_file( file, search , replace );

		  if( o_versid_remove.getState() )
			  file_erg = remove_version_id_rc->remove_versid(file);

		  if( o_owcallback.isSet() ) {
			  file_erg = owcallback1->patch_file(file);
		  }

		  break;

	  case FILE_TYPE::HEADER:
	  case FILE_TYPE::C_FILE:

		  if( o_replace.isSet() )
			  file_erg = patch_file( file, search , replace );

		  if( o_versid_remove.getState() )
		  		  file_erg = remove_version_id_ch->remove_versid(file);

		  if( o_primanlist.getState() )
			  file_erg = priman_list->patch_file(file);

		  if( o_mlm.getState() )
			  file_erg = fix_mlm->patch_file(file);

		  break;

	  case FILE_TYPE::PL_FILE:
		  if( o_versid_remove.getState() )
		  	  file_erg = remove_version_id_pl->remove_versid(file);
		  break;


	  case FILE_TYPE::PDL_FILE:
	  case FILE_TYPE::PDS_FILE:
		  if( o_versid_remove.getState() )
		  	  file_erg = remove_version_id_pdl->remove_versid(file);
		  break;

	  case FILE_TYPE::FIRST__:
	  case FILE_TYPE::LAST__:
	  case FILE_TYPE::UNKNOWN:
		  continue;

	  }


	  if( file_erg.empty() )
		  continue;

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
