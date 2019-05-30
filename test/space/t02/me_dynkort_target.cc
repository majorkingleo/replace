/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2011 Salomon Automation GmbH
 */

#include "me_dynkort_target.h"
#include <wamasbox.h>
#include <te_fes.h>
#include <sqlopm.h>
#include <logtool2.h>
#include <hist_util.h>
#include <svar.h>
#include <cpp_util.h>

using namespace Tools;

namespace wamas {

std::string dynkort_question_target( const std::string & fac, MskDialog hMaskRl )
{
  char acFeldId[FELDID_LEN+1] = {0};

  StrCpyDestLen(acFeldId, Var_GetString("Llr.DefaultDynKort","AP-1") );

  int iRv = WamasBox (SHELL_OF (hMaskRl),
					  WboxNboxType,        WBOX_INPUT,
					  WboxNtext,           MlM("Dynamischen Kommissionierort auswählen"),
					  WboxNmwmTitle,       MlM("Dynamischen Kommissionierort auswählen"),
					  // FeldId
					  WboxNinputEf,           FES_FeldId_t,
					  WboxNinputEfVStruct,    acFeldId,
					  WboxNinputEfKey,        KEY_DEF,
					  WboxNinputEfAtom,       ATOM_EfDbName,
					  WboxNinputEfAtomSData,  "\""TCN_FES_FeldId"\"",
					  WboxNinputEfAtom,       ATOM_SqlTableName,
					  WboxNinputEfAtomSData,  "\""TN_FES"\"",
					  WboxNinputEfAtom,       ATOM_OpmPsqlFilterPart"1",
					  WboxNinputEfAtomSData,  "\"" TCN_FES_Fattr_DYNKORT "=1\"",
					  WboxNinputEfAtom,       ATOM_OpmPcheck,
					  WboxNinputEfAtomSData,  "1",
					  WboxNinputEfCallback,   CbSqlOm,
					  WboxNinputEfLabel,      MlM("Kommissionierort"),
					  
					  WboxNbuttonText,        MlMsg("Ok"),
					  WboxNbuttonRv,          JANEIN_J,
					  WboxNbutton,            WboxbNcancel,
					  WboxNbuttonRv,          JANEIN_N,
					  WboxNescButton,         WboxbNcancel,
					  NULL);
					  

  if (iRv != JANEIN_J) {
	return std::string();
  }

  LogPrintf( fac, LT_DEBUG, "Der Dynamische Kommissionierort %s wurde von User %s ausgewählt",
			 acFeldId, GetUserOrTaskName() );

  return acFeldId;
}

} // /namespace wamas

