/**
* @file
* @todo describe file content
* @author Copyright (c) 2015 Salomon Automation GmbH
*/
#include <set>

#include <stdio.h>
#include <string.h>
#include <time.h>
#if (! defined WIN32 && ! defined _WIN32)
#include <sys/param.h>
#endif
/* ------- Owil-Headers --------------------------------------------------- */
#include <owil.h>
#include <module/owss.h>
#include <efcbm.h>
#include <owrcloader.h>
#include <module/owgr.h>
#include <cpp_util.h>
/* ------- Tools-Headers -------------------------------------------------- */
#include <lbc.h>
#include <term_util.h>
#include <dbsqlstd.h>
#include <hist_util.h>
#include <t_util.h>
#include <ml.h>
#include <makefilter.h>
#include <logtool.h>
#include <elements.h>
#include <logtool.h>
#include <sqlkey.h>
#include <efcbm.h>
#include "facility.h"
#include <cycle.h>
#include <telo2str.h>
#include <wamaswdg.h>
#include <prexec.h>
#include <ml_util.h>
#include <disperr.h>
#include <stime.h>
#include <sstring.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "tpa.h"
#include "tms.h"
#include "defs.h"
#include "proc.h"
#include "vpl.h"
#include "alma.h"
#include "facility.h"
#include "callback.h"
#include "term_util.h"
#include "parameter.h"
#include "me_printer.h"
#include "me_stp_persnr.h"
#include "status_handling.h"
#include "te_util.h"
#include "te_vpl.h"
#include "me_vplueb_rueck.h"
#include "me_vplueb.h"
#include "me_split.h"
#include "me_stp.h"
#include "me_abs.h"
#include "dcckutil.h"
#include "stp_buchen.h"
#include "vpl_util.h"
#include "vpl_split_merge.h"
#include "alma_util.h"

#include "me_prio.h"
#include "me_zut.h"
#include "me_split.h"
#include "wamas.h"

#include "me_kssplitpaleingabe.h"
#include "me_bearb_alma_cb.h"

#include "me_bearb.h"

/* ===========================================================================
* LOCAL DEFINES AND MACROS
* =========================================================================*/

/* BIT DEFINES FOR PRIO */
#define PRIO_1          0x00000001
#define PRIO_2          0x00000002
#define PRIO_3          0x00000004
#define PRIO_4          0x00000008
#define PRIO_5          0x00000010
#define PRIO_6          0x00000020
#define PRIO_7          0x00000040
#define PRIO_8          0x00000080
#define PRIO_9          0x00000100

#define MD_FAC_STP_VPLUEB (iMask == MK_S) ? MD_FAC_STP : MD_FAC_VPLUEB
#undef TANR_ODER_KSNR
#define TANR_ODER_KSNR (iMask == MK_S) ? ptPosCtx->tTpa.tpaTaNr : ptPosCtx->tVplk.vplkKsNr 
#define MY_VPLK_BLOCKSIZE 	16

using namespace Tools;
/* ===========================================================================
* LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
* =========================================================================*/
/* ------- Enums --------------------------------------------------------- */

/* ===========================================================================
* LOCAL (STATIC) Variables and Function-Prototypes
* =========================================================================*/

/* ------- Variables ----------------------------------------------------- */

static int  miAbschliessen = 0;

/* ------- Function-Prototypes ------------------------------------------- */

static int CbAuftragBearbeiten (MskDialog hMaskRl, MskStatic ef, 
		MskElement hEfRl,int reason, void *cbc, void *calldata);
static int CbAuftragPriorisieren (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata);
static int CbAuftragSperren (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
		int reason, void *cbc, void *calldata);
static int CbAuftragZuteilen (MskDialog hMaskRl, MskStatic ef, 
		MskElement hEfRl, int reason, void *cbc, void *calldata);
static int Cb_MaskEinzelnAendern (MskDialog hMaskRl, int iReason);
static int CbMask (MskDialog hMaskRl, int iReason);
static int CbAlleZuteilen(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
		int iReason, void *pvCbc, void *Calldata);
static int CbAllePriorisieren (MskDialog hMaskRl, MskStatic ef, 
		MskElement hEfRl,int reason, void *cbc, void *calldata);
static int CbAlleAuftraegeSperren (MskDialog hMaskRl, MskStatic ef, 
		MskElement hEfRl, int reason, void *cbc, void *calldata);
static int CbAuftragAbschliessen (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata);
static int CbAuftragPalSplit (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata);
static int CbAlleAuftraegePalSplit (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata);

/* ===========================================================================
* GLOBAL VARIABLES
* =========================================================================*/
/* ===========================================================================
* LOCAL (STATIC) Functions
* =========================================================================*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Gets the DB record from LB record  
-* RETURNS
-*--------------------------------------------------------------------------*/
int GetStructFromLB (MskDialog hMaskRl, long *lNrListBuffer, VPLK *ptVplk,
					 TPA *ptTpa)
{
	ListElement 	hLiEle;
	STP_VPLUEB_CTX  *ptStVpl = NULL;  
	LB_VPLUEB   	tLbVplRec;
	LB_STP   		tLbStpRec;
	int				iMask;
	int 			iRv;
	ptStVpl = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	iMask = ptStVpl->iWhichMask;
	hLiEle = LBGetSelEl(iMask== MK_S ? ptStVpl->tStp.lb : ptStVpl->tVplueb.hLb);
	if (iMask == MK_S) {
		memset (ptTpa, 0, sizeof (TPA));
		tLbStpRec = * (LB_STP *)hLiEle->data;
		iRv =   TExecSql( hMaskRl,
			"SELECT %TPA FROM TPA "
			"WHERE  TPA.TANR = :TaNr ",
			SELSTRUCT(TN_TPA, *ptTpa),
			SQLLONG(tLbStpRec.TpaRecNow.tpaTaNr),
			NULL) ;
		*lNrListBuffer = tLbStpRec.TpaRecNow.tpaTaNr;
	} else {
		memset (ptVplk, 0, sizeof (VPLK));
		tLbVplRec = * (LB_VPLUEB *)hLiEle->data;
		iRv =   TExecSql( hMaskRl,
				"SELECT %VPLK FROM VPLK "
				"WHERE  VPLK.KSNR = :KsNr ",
				SELSTRUCT(TN_VPLK, *ptVplk),
				SQLLONG(tLbVplRec.tVplk.vplkKsNr),
				NULL) ;
		*lNrListBuffer = tLbVplRec.tVplk.vplkKsNr;

	}
	return iRv;
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbBearbeiten-Callback,ruft das Menu zum alle Ändern auf 
-* RETURNS
-*--------------------------------------------------------------------------*/
int CbBearbeiten (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
				  int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX 		*ptStVpl = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, 
		MskNmaskCalldata);
	VPLK				tVplk;
	TPA					tTpa;
	long				lNrListBuffer;
	int					*pIsEinzelnAendernMaskOpen;
	int					*pIsAlleAendernMaskOpen;
	int					iAnzSel, iRv;
	int					iMask = ptStVpl->iWhichMask, nI = 0, iLast = 0;
	ListElement			hLiEle; 

	switch (reason) {
	case FCB_XF:
		if (iMask == MK_S) {
			iAnzSel = LBNumberOfSelected(ptStVpl->tStp.lb);
			pIsEinzelnAendernMaskOpen = 
				&ptStVpl->tStp.iIsEinzelnAendernMaskOpen;
			pIsAlleAendernMaskOpen = &ptStVpl->tStp.iIsAlleAendernMaskOpen;
		} else {
			iAnzSel = LBNumberOfSelected(ptStVpl->tVplueb.hLb);
			iLast = ListBufferLastElement (ptStVpl->tVplueb.hLb);
			pIsEinzelnAendernMaskOpen = 
				&ptStVpl->tVplueb.iIsEinzelnAendernMaskOpen;
			pIsAlleAendernMaskOpen = &ptStVpl->tVplueb.iIsAlleAendernMaskOpen;
			ptStVpl->tVplueb.Sheet2 = 1;
		}
		if (iAnzSel < 1) {
			if (iMask == MK_S) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,  WBOX_ALERT,
					WboxNbutton,   WboxbNok,
					WboxNmwmTitle, MlM("Transportaufträge bearbeiten"),
					WboxNtext,     MlM("Bitte Datensätze selektieren !\n"),
					NULL);
			} else {  /* iMask == MK_V */
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,  WBOX_ALERT,
					WboxNbutton,   WboxbNok,
					WboxNmwmTitle, MlM("Kommissionierscheine bearbeiten"),
					WboxNtext,     MlM("Bitte Datensätze selektieren !\n"),
					NULL);
			}
			return (-1);
		}
		if (iAnzSel > 1) {
			if (*pIsEinzelnAendernMaskOpen == 0) {
				for (nI = 0; nI <= iLast; nI++) {
					hLiEle = ListBufferGetElement (ptStVpl->tVplueb.hLb, nI);
					if (hLiEle == NULL ) {
						continue;
					}
					if (!(hLiEle->hint & LIST_HINT_SELECTED)) {
						continue;
					}
				}
				if (_alle_bearbeiten (SHELL_OF(hMaskRl), ptStVpl, 
					hMaskRl, iMask) < 0) { 
						return (EFCBM_CONTINUE);
				} else {
					MskUpdateMaskVar(hMaskRl);
					*pIsAlleAendernMaskOpen = 1;
				}
			}
		} else {
			if (*pIsAlleAendernMaskOpen == 0) {
				iRv = GetStructFromLB (hMaskRl, &lNrListBuffer, &tVplk, &tTpa);
				if (iRv < 0 ) {
					if (TSqlError(hMaskRl) != SqlNotFound) {
						TSqlRollback(hMaskRl);
						if (iMask == MK_S) {
							WamasBox(SHELL_OF(hMaskRl),
								WboxNboxType,   WBOX_ALERT,
								WboxNbutton,    WboxbNok,
								WboxNmwmTitle,  MlM("Transportauftrag lesen"),
								WboxNtext, 
									wamas::platform::string::form(MlMsg(
										"Fehler beim Lesen vom "
										"Transportauftrag\nTaNr: %d"), 
										lNrListBuffer).c_str(),
								NULL);
						} else {  /* iMask == MK_V */
							WamasBox(SHELL_OF(hMaskRl),
								WboxNboxType,   WBOX_ALERT,
								WboxNbutton,    WboxbNok,
								WboxNmwmTitle,  MlM("Kommissionierschein lesen"), 
								WboxNtext, 
									wamas::platform::string::form(MlMsg(
										"Fehler beim Lesen vom "
										"Kommissionierschein\nKsNr: %ld"), 
										lNrListBuffer).c_str(),
								NULL);
						}
						LogPrintf (MD_FAC_STP_VPLUEB,LT_ALERT,
							"Ein Fehler beim Lesen der %s "
							"ist aufgetreten!\n TSqlErr: %s",
							iMask == MK_S ?
							"Transportaufträge" : "Verplanungen",
							TSqlErrTxt(hMaskRl));
						return (-1);
					} else {
						if (iMask == MK_S) {
							WamasBox(SHELL_OF(hMaskRl),
								WboxNboxType,   WBOX_ALERT,
								WboxNbutton,    WboxbNok,
								WboxNmwmTitle,  MlM("Transportauftrag lesen"), 
								WboxNtext,
									wamas::platform::string::form(MlMsg(
										"Transportauftrag existiert "
										"nicht mehr!\nTaNr:%ld"), 
										lNrListBuffer).c_str(),
								NULL);
						} else {  /* iMask == MK_V */
							WamasBox(SHELL_OF(hMaskRl),
								WboxNboxType,   WBOX_ALERT,
								WboxNbutton,    WboxbNok,
								WboxNmwmTitle,  MlM("Kommissionierschein lesen"), 
								WboxNtext,
									wamas::platform::string::form(MlMsg(
										"Kommissionierschein existiert "
										"nicht mehr!\nKsNr:%ld"), 
										lNrListBuffer).c_str(),
								NULL);
						}
						LogPrintf (MD_FAC_STP_VPLUEB,LT_ALERT,
							"Ein Fehler beim Lesen der %s "
							"ist aufgetreten!\n TSqlErr: %s",
							iMask == MK_S ?
							"Transportaufträge" : "Verplanungen",
							TSqlErrTxt (hMaskRl));
						TSqlRollback(hMaskRl);
						return (-1);
					}
				}
				TSqlRollback(hMaskRl);
				if (_einzeln_aendern (SHELL_OF(hMaskRl), hMaskRl, 
					iMask, ptStVpl, &tVplk, &tTpa) < 0) {
						return (EFCBM_CONTINUE);
				} else {
					MskUpdateMaskVar(hMaskRl);
					*pIsEinzelnAendernMaskOpen = 1;
				}
			}
		}
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
} /* CbBearbeiten */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragSperren Callback, sperrt bzw. entsperrt den selektierten Auftrag 
-* RETURNS
-*---------------------------------------------------------------------------*/
static int CbAuftragSperren (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
							 int reason, void *cbc, void *calldata)
{
	MY_CTX      		*ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, 
		MskNmaskCalldata);
	VPLI                tVpli = {};
	TPA					tTpa = {};
	VPLK                tVplk = {};
	char                acZeit [20];
	time_t              zTime;
	int					iDbRv = 0, iRet = 0;
	int            		iMask = ptPosCtx->iWhichMask;
	switch (reason) {
	case FCB_XF:
		memset (&tVpli, 0, sizeof (VPLI));
		memset (acZeit, 0, sizeof (acZeit));
		memset (&tVplk, 0, sizeof(VPLK));
		zTime = time ((time_t *)0);
		strftime (acZeit, sizeof (acZeit) - 1, "%H:%M %d.%m",
			localtime (&zTime));
		if ((ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_WARTEN &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_FERTIG &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_NACH &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_AKTIV &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_KOMM &&
			iMask == MK_V) || 
			(ptPosCtx->tTpa.tpaStatus != TPASTATUS_WARTEN &&
			ptPosCtx->tTpa.tpaStatus != TPASTATUS_FERTIG &&
			iMask == MK_S)) {
				if (WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbuttonText,MlM("Sperren"),
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM("Sperren"),
					WboxNtext,      
						wamas::platform::string::form(MlMsg(
							"%s %d sperren?"), 
							iMask == MK_S ?
							"Transportauftrag" : "Kommissionierschein",  
							TANR_ODER_KSNR).c_str(),
					NULL) != IS_Ok) {
						return (-1);
				}
		} else if (ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_WARTEN) {
			if (WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_WARN,
				WboxNbuttonText,MlM("Entsperren"),
				WboxNbuttonRv,  IS_Ok,
				WboxNbutton,    WboxbNcancel,
				WboxNescButton, WboxbNcancel,
				WboxNmwmTitle,  MlM("Entsperren"),
				WboxNtext,      
					wamas::platform::string::form(MlMsg(
						"%s %d entsperren?"), 
						iMask == MK_S ? 
						"Transportauftrag" : "Kommissionierschein",  
						TANR_ODER_KSNR).c_str(),
				NULL) != IS_Ok) {
					return (-1);
			}
		} else if (ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_NACH &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_AKTIV) {
				if (WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbuttonText,MlM("Entsperren"),
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM("Entsperren"),
					WboxNtext,
					wamas::platform::string::form(MlMsg(
						"%s %d wurde bereits bearbeitet!"), 
						iMask == MK_S ?
						"Transportauftrag" : "Kommissionierschein",  
						TANR_ODER_KSNR).c_str(),
					NULL) != IS_Ok){
						return (-1);
				}
		}
		if (iMask == MK_S) {
			memset (&tTpa, 0, sizeof(TPA));
			memcpy(&tTpa, &ptPosCtx->tTpa, sizeof(TPA));
		} else {
			memset (&tVplk, 0, sizeof(VPLK));
			memcpy(&tVplk, &ptPosCtx->tVplk, sizeof(VPLK));
		}
		if (tTpa.tpaStatus == TPASTATUS_AKTIV && iMask == MK_S) {
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Sperren/Freigeben"),
				WboxNtext, 
					wamas::platform::string::form(MlMsg(
						"Transportauftrag %d wird"
						" bereits bearbeitet."),  
						TANR_ODER_KSNR).c_str(),
				NULL);
			TSqlRollback(hMaskRl);
			MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
			return(EFCBM_CONTINUE);
		}
		if ((tVplk.vplkStatus == VPLKSTATUS_AKTIV || 
			tVplk.vplkStatus == VPLKSTATUS_NACH) &&
			iMask == MK_V) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Freigeben"),
					WboxNtext, wamas::platform::string::form(MlMsg(
						"Kommissionierschein %d wird"
						" bereits bearbeitet."),  
						TANR_ODER_KSNR).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
				return(EFCBM_CONTINUE);
		}

		iDbRv = (iMask == MK_S) ? 
			TExecStdSql(hMaskRl, StdNselectUpdNo, TN_TPA, &tTpa) :
		TExecStdSql(hMaskRl, StdNselectUpdNo, TN_VPLK, &tVplk);

		if (iDbRv <= 0) {
			if (TSqlError(hMaskRl)!=SqlNotFound) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
					wamas::platform::string::form(MlMsg(
						"Fehler beim Selectieren %s:%d."),   
						iMask == MK_S ? "TaNr" : "KsNr",  
						TANR_ODER_KSNR).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
				return(EFCBM_CONTINUE);
			} else {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"%s:%d nicht mehr vorhanden."), 
							iMask == MK_S ? "TaNr" : "KsNr",  
							TANR_ODER_KSNR).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
				return(EFCBM_CONTINUE);
			}
		}
		if ((ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_WARTEN &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_FERTIG &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_KOMM &&
			iMask == MK_V) ||
			(ptPosCtx->tTpa.tpaStatus != TPASTATUS_WARTEN &&
			ptPosCtx->tTpa.tpaStatus != TPASTATUS_FERTIG &&
			iMask == MK_S)) {
				iRet = (iMask == MK_S) ? 
					Tpa_Sperren_Entsperren (hMaskRl, 
					TO_CHAR(MD_FAC_STP_VPLUEB), 
					tTpa.tpaTaNr, TPASTATUS_WARTEN) : 
				StatMan_SetVplkWarten (hMaskRl, 
					TO_CHAR(MD_FAC_STP_VPLUEB), 
					&tVplk);
				if (iRet < 0) {
					if (iDbRv <= 0) {
						WamasBox (SHELL_OF (hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Sperren"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Fehler beim Sperren vom %s :%d."), 
									iMask == MK_S ? "TaNr" : "KsNr",  
									TANR_ODER_KSNR).c_str(),
							NULL);
						TSqlRollback(hMaskRl);
						MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
						return(EFCBM_CONTINUE);
					}
				}
		} else {
			if ((ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_FERTIG &&
				ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_KOMM && 
				iMask == MK_V) ||
				(ptPosCtx->tTpa.tpaStatus != TPASTATUS_FERTIG && 
				iMask == MK_S)) {
					tVplk.vplkStatus = VPLKSTATUS_PAUSE;
					iRet = (iMask == MK_S) ? 
						Tpa_Sperren_Entsperren (hMaskRl, 
						TO_CHAR(MD_FAC_STP_VPLUEB),
						tTpa.tpaTaNr, TPASTATUS_FERTIG): 
					StatMan_HandleVplk (hMaskRl, 
						TO_CHAR(MD_FAC_STP_VPLUEB), 
						&tVplk); 
					CF_TRIGGER(PROC_TAM, FUNC_TAM_ATPA, 0, "Sperre");
					if (iRet < 0) {
						if (iDbRv <= 0) {
							WamasBox (SHELL_OF (hMaskRl),
								WboxNboxType,   WBOX_ALERT,
								WboxNbutton,    WboxbNok,
								WboxNmwmTitle,  MlM("Entsperren"),
								WboxNtext,
									wamas::platform::string::form(
										MlMsg("Fehler beim Entsperren vom %s :%d."), 
										iMask == MK_S ? "TaNr" : "KsNr",  
										TANR_ODER_KSNR).c_str(),
								NULL);
							TSqlRollback(hMaskRl);
							MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
							return(EFCBM_CONTINUE);
						}
					}
			}
		}
		if (iMask == MK_V) {
			tVpli.vpliKsNr = tVplk.vplkKsNr;
			tVpli.vpliIdx  = 0;
			tVpli.vpliTyp = VPLITYP_INTERN;
			tVpli.vpliArt = VPLIART_KOPF;
			SetHist(TN_VPLI, &tVpli, HIST_INSERT, GetUserOrTaskName());
			tVpli.vpliPosNr = GetNextPosNrVpli (hMaskRl, MD_FAC_VPLUEB, TN_VPLI,
				tVplk.vplkKsNr,VPLIART_KOPF, VPLITYP_INTERN, 0);
			if (tVpli.vpliPosNr == -1) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler beim Insert in VPLI\nKsNr: %ld"), 
							tVplk.vplkKsNr).c_str(), 
					NULL);
				LogPrintf (MD_FAC_VPLUEB,LT_ALERT,
					"Fehler beim GetNextPosNrVpli in VPLI. KsNr: %ld, "
					"TSqlErr: %s",
					tVplk.vplkKsNr, 
					TSqlErrTxt (hMaskRl));
				TSqlRollback(hMaskRl);
				return -1;
			}
			if (tVplk.vplkStatus == VPLKSTATUS_WARTEN) {
				StrCpy (tVpli.vpliText,
					wamas::platform::string::form(MlMsg (
						"User: %s Zeit: %s, "
						"Kommissionierschein KsNr:%d wurde gesperrt"), 
						GetUserOrTaskName(), 
						acZeit, 
						ptPosCtx->tVplk.vplkKsNr));
			} else {
				StrCpy (tVpli.vpliText,
					wamas::platform::string::form(MlMsg (
						"User: %s Zeit: %s, "
						"Kommissionierschein KsNr:%d wurde entsperrt"), 
						GetUserOrTaskName(), 
						acZeit, 
						ptPosCtx->tVplk.vplkKsNr));
			}
			tVpli.vpliText[INFTEXT_LEN] = '\0';
			iDbRv = TExecStdSql(hMaskRl, StdNinsert, TN_VPLI, &tVpli);
			if (iDbRv <= 0) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler beim Insert in VPLI\nKsNr: %d."), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return (-1);
			}
		}
		TSqlCommit (hMaskRl);
		if (iMask == MK_S) {
			LB_DeSellectAll(ptPosCtx->ptStVpl->tStp.lb);
			PosLesenStp(ptPosCtx->hFirstMask, OPT_ERRORBOX);
			MskUpdateMaskVar (ptPosCtx->hFirstMask);
		} else {
			LB_DeSellectAll(ptPosCtx->ptStVpl->tVplueb.hLb);
			if (ListUpdateVplueb(ptPosCtx->hFirstMask,
				&((STP_VPLUEB_CTX *)ptPosCtx->ptStVpl)->tVplueb, 
				MANUELL_LEVEL) < 0) {
				ptPosCtx->ptStVpl->tVplueb.Aktion = MANUELL_READ;
				TSqlRollback (ptPosCtx->hFirstMask);
			}
			MskUpdateMaskVar (ptPosCtx->hFirstMask);
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragSperren()  */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragBearbeiten-Callback, hier wird das Auftragsmenu aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragBearbeiten (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
								int reason, void *cbc, void *calldata)
{
	MY_CTX      *ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	VPLK		tVplk;
	TPA			tTpa;
	int 		iRv = 0;
	int			iMask = ptPosCtx->iWhichMask;
	switch (reason) {
	case FCB_XF:
		if (iMask == MK_S) {
			memset (&tTpa, 0, sizeof (TPA));
			tTpa.tpaTaNr = ptPosCtx->tTpa.tpaTaNr;
			iRv = TExecStdSql(hMaskRl, StdNselect, TN_TPA, &tTpa);
		} else {
			memset (&tVplk, 0, sizeof (VPLK));
			tVplk.vplkKsNr = ptPosCtx->tVplk.vplkKsNr;
			iRv = TExecStdSql(hMaskRl, StdNselect, TN_VPLK, &tVplk);
		}
		if (iRv < 0 ) {
			if (TSqlError(hMaskRl) != SqlNotFound) {
				TSqlRollback(hMaskRl);
				if (iMask == MK_S) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Transportauftrag lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Fehler beim Lesen vom"
								" Transportauftrag!\nTaNr: %d"), 
								TANR_ODER_KSNR).c_str(),
						NULL);
				} else { /* iMask == MK_V */
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Fehler beim Lesen vom"
								" Kommissionierschein!\nKsNr: %d"), 
								TANR_ODER_KSNR).c_str(),
						NULL);
				}
				LogPrintf (MD_FAC_STP_VPLUEB, LT_ALERT,
					"Ein Fehler beim Lesen der %s "
					"ist aufgetreten!\nTSqlErr: %s",
					iMask == MK_S ?
					"Transportaufträge" : "Kommissionierscheine",
					TSqlErrTxt (hMaskRl));
				return (-1);
			} else {
				if (iMask == MK_S) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Transportauftrag lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Transportauftrag existiert"
								" nicht mehr!\nTaNr: %d"), 
								TANR_ODER_KSNR).c_str(),
						NULL);
				} else {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Kommissionierschein existiert"
								" nicht mehr!\nKsNr: %d"), 
								TANR_ODER_KSNR).c_str(),
							NULL);
				}
				LogPrintf (MD_FAC_STP_VPLUEB, LT_ALERT,
					"Ein Fehler beim Lesen der %s "
					"ist aufgetreten!\n TSqlErr: %s",
					iMask == MK_S ?
					"Transportaufträge" : "Kommissionierscheine",
					TSqlErrTxt (hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
		}
		TSqlRollback(hMaskRl);
		if (iMask == MK_S) {
			if (me_stpbuchen (hMaskRl, &ptPosCtx->tStpLbCtx) < 0) { 
				return (EFCBM_CONTINUE);
			}
		} else {
			if (_auftrag_bearbeiten (GetRootShell (), &tVplk, ptPosCtx) < 0) { 
				return (EFCBM_CONTINUE);
			}
		}
		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragBearbeiten()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragZuteilen-Callback, hier wird die Zuteilungsmaske aufgerufen 
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragZuteilen (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
							  int reason, void *cbc, void *calldata)
{
	MY_CTX      *ptPosCtx;
	int			iMask;
	switch (reason) {
	case FCB_XF:
		ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
		iMask = ptPosCtx->iWhichMask;
		/* Check status */
		if (iMask == MK_S) {
			if (!(ptPosCtx->tTpa.tpaStatus == TPASTATUS_NEU ||
				ptPosCtx->tTpa.tpaStatus == TPASTATUS_SENDEN)) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Transportauftrag zuteilen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Transportauftrag hat keinen gültigen Status!\n"
								"TaNr: %d"),  
								TANR_ODER_KSNR).c_str(),
						NULL);
					return (-1);
			}
		} else {
			if (!(ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_NEU ||
				ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_BEREIT ||
				ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_WARTEN)) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein zuteilen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Kommissionierschein hat keinen gültigen Status!\n"
								"KsNr: %d"),  
								TANR_ODER_KSNR).c_str(),
						NULL);
					return (-1);
			}
		}
		if (_auftrag_zuteilen (GetRootShell (), ptPosCtx) < 0) { 
			return (EFCBM_CONTINUE);
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /*  CbAuftragZuteilen()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragPriorisieren-Callback, hier wird die Priorisierenmaske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragAbschliessen (MskDialog hMaskRl, MskStatic ef,
								  MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	MY_CTX	*ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	VPLK	tVplk;
	TPA		tTpa;
	long	lCount=0, iDbRv=0;
	int		iMask = ptPosCtx->iWhichMask;
	switch (reason) {
	case FCB_XF:
		if (iMask == MK_V) {
			memset(&tTpa, 0, sizeof(TPA));
			if (ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_NACH) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein abschliessen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Kommissionierschein hat keinen "
							"gültigen Status!\n KsNr: %ld"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				return (-1);
			}
			if (ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_NACH) {
				if (WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbuttonText,MlM ("Abschliessen"),
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM ("Abschliessen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg (
							"Auftrag KsNr: %d hat nicht Status 'FEHLT'!\n"
							"Wollen Sie trotzdem abschliessen?"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL) != IS_Ok) {
						return (-1);
				}
			}
			iDbRv = TExecSql(hMaskRl,
				"SELECT count(*) FROM TPA "
				" WHERE  TPA.BEARBID_MANDANT = :Mandant "
				" AND  TPA.BEARBID_BEARBNR = :BearbNr "
				" AND  TPA.BEARBID_BEARBKZ = :BearbKz "
				" AND  TPA.KSNR = :KsNr "
				" AND TPA.STATUS = 'AKTIV'",
				SELLONG (lCount),
				SQLMANDANT (ptPosCtx->tVplk.vplkAusId.Mandant),
				SQLBEARBNR (ptPosCtx->tVplk.vplkAusId.AusNr),
				SQLBEARBKZ (ptPosCtx->tVplk.vplkAusId.AusKz),
				SQLLONG (ptPosCtx->tVplk.vplkKsNr),
				NULL);
			if (iDbRv <= 0) {
				TDB_DispErr (hMaskRl, TN_TPA, NULL, NULL);
				return (-1);
			}
			if (lCount > 0) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein abschliessen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Es gibt einen aktiven TPA "
							"fuer diesen Auftrag !\n KsNr: %d"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				return (-1);
			}
			if (me_abs (GetRootShell (), ptPosCtx->hLb, ptPosCtx->tVplk,
				tTpa, ptPosCtx->ptStVpl,
				ptPosCtx->hFirstMask, iMask, MODE_EINZ , JANEIN_J ) < 0) {
					return (EFCBM_CONTINUE);
			}
		} else {
			/* No status check */
			memset (&tVplk, 0, sizeof(VPLK));
			if (me_abs (GetRootShell (), ptPosCtx->hLb,
				tVplk, ptPosCtx->tTpa, ptPosCtx->ptStVpl,
				ptPosCtx->hFirstMask, iMask, MODE_EINZ , JANEIN_J ) < 0) {
					return (EFCBM_CONTINUE);
			}
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragAbschliessen()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragAendern-Callback, hier wird die Änderungs-Maske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragAendern (MskDialog hMaskRl, MskStatic ef,
							 MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX 	*ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	MY_CTX      	*ptPosCtx;
	int         	iRv;
	int				iCheckOnce = 0;

	switch (reason) {
	case FCB_XF:

		ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);

		if (ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_NEU &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_BEREIT &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_AKTIV &&
			ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,  WBOX_ALERT,
					WboxNbutton,   WboxbNok,
					WboxNmwmTitle, MlM("Auftrag ändern"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Kommissionierauftrag hat keinen "
							"gültigen Status!\n KsNr: %ld"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return (-1);
		}
		if (ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_AKTIV) {
			/*
			* Wenn der Auftrag aktiv ist, wird nachgefragt,
			* ob er wirklich ändern möchte...
			* (sonst ist die Meldung etwas zu viel -> nicht pragmatisch)
			*/
			iRv = AuftragInfo (hMaskRl, &ptPosCtx->tVplk, MOD_AENDERN);
			if (iRv < 0) {
				break;
			}
			iRv = WamasBox (SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_WARN,
				WboxNbuttonText,MlM("Ändern"),
				WboxNbuttonRv,  IS_Ok,
				WboxNbutton,    WboxbNcancel,
				WboxNescButton, WboxbNcancel,
				WboxNmwmTitle,  MlM("Auftrag ändern"),
				WboxNtext,
					wamas::platform::string::form(MlMsg (
						"Wollen Sie den Auftrag mit der KsNr:%d\n"
						"wirklich ändern ?"), 
						ptPosCtx->tVplk.vplkKsNr).c_str(),
				NULL);
			if (iRv != IS_Ok) {
				TSqlRollback (hMaskRl);
				return(-1);
			}
		}
		ptMc = (STP_VPLUEB_CTX *)ptPosCtx->ptStVpl;
		ptMc->hFirstMask = ptPosCtx->hFirstMask;
		iRv = me_vpl_rueck (GetRootShell(), ptMc, iCheckOnce);
		if (iRv < 0) {
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,  WBOX_ALERT,
				WboxNbutton,   WboxbNok,
				WboxNmwmTitle, MlM("Auftrag ändern"),
				WboxNtext,
					MlM("Es ist ein Fehler beim Ändern aufgetreten !!"),
				NULL);
			TSqlRollback(hMaskRl);
		}
		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragAendern()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragStorno (MskDialog hMaskRl, MskStatic ef,
							MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	MY_CTX	*ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	VPLK	tVplk;
	TPA		tTpa;
	long	lCount=0;
	int		iMask = ptPosCtx->iWhichMask, iDbRv = 0, iRv;

	switch (reason) {
	case FCB_XF:

		if (iMask == MK_V) {
			iRv = WamasBox (SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_WARN,
				WboxNbuttonText,MlM("Storno"),
				WboxNbuttonRv,  IS_Ok,
				WboxNbutton,    WboxbNcancel,
				WboxNescButton, WboxbNcancel,
				WboxNmwmTitle,  MlM("Kommissionierschein stornieren"),
				WboxNtext,
					wamas::platform::string::form(MlMsg (
						"Wollen Sie den Auftrag mit der KsNr:%d\n"
						"wirklich stornieren?"), 
						ptPosCtx->tVplk.vplkKsNr).c_str(),
				NULL);
			if (iRv != IS_Ok) {
				return(-1);
			}
			memset(&tTpa, 0, sizeof(TPA));
			if (ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein stornieren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Kommissionierschein hat keinen "
							"gültigen Status!\n KsNr: %d"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				return (-1);
			}
			iDbRv = TExecSql(hMaskRl,
				"SELECT count(*) FROM TPA "
				" WHERE  TPA.BEARBID_MANDANT = :Mandant "
				" AND  TPA.BEARBID_BEARBNR = :BearbNr "
				" AND  TPA.BEARBID_BEARBKZ = :BearbKz "
				" AND  TPA.KSNR = :KsNr "
				" AND TPA.STATUS = 'AKTIV'",
				SELLONG (lCount),
				SQLMANDANT (ptPosCtx->tVplk.vplkAusId.Mandant),
				SQLBEARBNR (ptPosCtx->tVplk.vplkAusId.AusNr),
				SQLBEARBKZ (ptPosCtx->tVplk.vplkAusId.AusKz),
				SQLLONG (ptPosCtx->tVplk.vplkKsNr),
				NULL);

			if (iDbRv <= 0) {
				TDB_DispErr(hMaskRl, TN_TPA, NULL, NULL);
				return (-1);
			}

			if (lCount > 0) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein"
					"abschliessen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Es gibt einen aktiven TPA "
							"fuer diesen Auftrag !\n KsNr: %d"), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				return (-1);
			}
			if (me_abs (GetRootShell (), ptPosCtx->hLb, ptPosCtx->tVplk,
				tTpa, ptPosCtx->ptStVpl,
				ptPosCtx->hFirstMask, iMask, MODE_EINZ , JANEIN_N ) < 0) {
					return (EFCBM_CONTINUE);
			}
		} else {
			/* No status check */
			memset (&tVplk, 0, sizeof(VPLK));
			if (me_abs (GetRootShell (), ptPosCtx->hLb,
				tVplk, ptPosCtx->tTpa, ptPosCtx->ptStVpl,
				ptPosCtx->hFirstMask, iMask, MODE_EINZ , JANEIN_N ) < 0) {
					return (EFCBM_CONTINUE);
			}
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragStorno()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragPriorisieren-Callback, hier wird die Priorisierenmaske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragPriorisieren (MskDialog hMaskRl, MskStatic ef,
								  MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	MY_CTX      *ptPosCtx;
	int         iMask;
	switch (reason) {
	case FCB_XF:
		ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);
		iMask = ptPosCtx->iWhichMask;
		/* Check status */
		if (iMask == MK_S &&
			!(ptPosCtx->tTpa.tpaStatus == TPASTATUS_NEU ||
			ptPosCtx->tTpa.tpaStatus == TPASTATUS_SENDEN)) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Transportauftrag priorisieren"),
					WboxNtext,		 
						wamas::platform::string::form(MlMsg(
							"Transportauftrag "
							"hat keinen gültigen Status!\n"
							"TaNr: %ld"),  
							TANR_ODER_KSNR).c_str(),
					NULL);
				return (-1);
		} else {
			if (iMask == MK_V && 
				!(ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_NEU ||
				ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_BEREIT ||
				ptPosCtx->tVplk.vplkStatus == VPLKSTATUS_WARTEN)) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein priorisieren"),
						WboxNtext,      
							wamas::platform::string::form(MlMsg(
								"Kommissionierschein "
								"hat keinen gültigen Status!\n"
								"KsNr: %ld"),  
								TANR_ODER_KSNR).c_str(),
						NULL);
					return (-1);
			}
		}
		if (_auftrag_priorisieren (GetRootShell (), ptPosCtx) < 0) { 
			return (EFCBM_CONTINUE);
		}
		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAuftragPriorisieren()  */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
*  Maskencallback für die kleine Stapleranfrage-Maske 
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbMask (MskDialog hMaskRl, int iReason)
{
	int 	iRet = MSK_OK_TRANSFER;
	switch (iReason) {
	case MSK_CM:
		break;
	case MSK_DM:
		break;
	default:
		break;
	}
	return iRet;
} /* CbMask */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int _me_stp_request (OWidget parent)
{
	MskDialog   hMaskRl = NULL;
	OWidget     m_sh = NULL;
	OwrcLoadObject (ApNconfigFile, "me_stp_request.rc");
	hMaskRl = MskOpenMask (NULL, "ME_STP_REQUEST");
	if (hMaskRl == NULL) {
		return RETURN_ERROR;
	}
	if ((m_sh = SHELL_OF(hMaskRl)) == NULL) {
		MskRlMaskSet (hMaskRl, MskNmaskNoTopPlate, (Value)1);
		MskRlMaskSet (hMaskRl, MskNmaskCallback, (Value)CbMask);
	}
	m_sh=ApShellModelessCreate (parent, AP_CENTER, AP_CENTER);
	WamasWdgAssignMenu (m_sh, "ME_STP_REQUEST");
	MskCreateDialog (m_sh, hMaskRl, MlM ("Fehlartikel-Info"),
		NULL, HSL_NI, SMB_All);
	return (RETURN_ACCEPTED);
} /* _me_stp_request */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
*  Maskencallback für die kleine Auswahlmaske 
-* RETURNS
-*--------------------------------------------------------------------------*/
static int Cb_MaskEinzelnAendern (MskDialog hMaskRl, int iReason)
{
	MY_CTX  		*ptPosCtx;
	int     		iRet = MSK_OK_TRANSFER;
	int				iMask;
	switch (iReason) {
	case MSK_CM:
		ptPosCtx = (MY_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		iMask = ptPosCtx->iWhichMask;
		/* ---- VARIABLE BINDEN --  */
		MskMatchEvaluate(hMaskRl, "*", 0, MSK_MATCH_NODEF_ATTR);
		MskVaAssign(hMaskRl,        MskGetElement("InfoText1"),
			MskNvariable,   (Value)ptPosCtx->acInfo,
			NULL);
		/* ---- BUTTONS ----------- */

		EfcbmInstByName(hMaskRl, "Zuteilen",
				KEY_DEF, CbAuftragZuteilen,     NULL);
		EfcbmInstByName(hMaskRl, "Priorisieren",
				KEY_DEF, CbAuftragPriorisieren, NULL);
		EfcbmInstByName(hMaskRl, "Sperren",
				KEY_DEF, CbAuftragSperren,      NULL);
		EfcbmInstByName(hMaskRl, "Abschliessen",
				KEY_DEF, CbAuftragAbschliessen, NULL);
		EfcbmInstByName(hMaskRl, "Aendern",
				2, CbAuftragAendern, NULL);
		EfcbmInstByName(hMaskRl, "Storno",
				KEY_DEF, CbAuftragStorno, NULL);
		EfcbmInstByName(hMaskRl, "AutoSplitten",
				KEY_DEF, CbAuftragPalSplit, NULL);
		EfcbmInstByName(hMaskRl, "AlmaRefresh",
				KEY_DEF, alma::CbAuftragAlmaRefresh, NULL);
		EfcbmInstByName(hMaskRl, "Bearbeiten",
			KEY_DEF, CbAuftragBearbeiten,   NULL);


		/* ---- ACTIVATE / DEACTIVATE BUTTONS ---- */
		if (iMask == MK_S) {
			MskVaAssignMatch(hMaskRl, "*-NOTSTP-*",
					MskNattrOr, (Value)EF_ATTR_INACTIVE,
					NULL);
		} else {
			MskVaAssignMatch(hMaskRl, "*-NOTSTP-*",
					MskNattrClr, (Value)EF_ATTR_INACTIVE,
					NULL);
		}

		break;

	case MSK_DM:
		ptPosCtx = (MY_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		*ptPosCtx->piIsEinzelnAendernMaskOpen = 0;
		TSqlRollback(hMaskRl);
		if (ptPosCtx != NULL) {
			free (ptPosCtx);
			ptPosCtx = NULL;
		}
		break;
	default:
		break;
	}
	return iRet;
} /* Cb_MaskEinzelnAendern */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*   Menumaske mit verschiedenen Buttons (Bearbeiten, Fertigmelden, Pririsieren ...)
-*   Es ist möglich nur den selektierten Auftrag (von der Hauptmaske) ändern
-* RETURNS
-*--------------------------------------------------------------------------*/
int _einzeln_aendern ( OWidget w, MskDialog hMask, int iMask,
					  STP_VPLUEB_CTX *ptStVpl, VPLK *ptVplk, TPA *ptTpa)
{
	OWidget             hMsh = NULL;
	MskDialog           hMaskRl = NULL;
	MY_CTX              *ptPosCtx;
	int					*pIsEinzelnAendernMaskOpen;
	/* RC-File holen */
	OwrcLoadObject(ApNconfigFile,"vpl.rc");
	OwrcLoadObject(ApNconfigFile,"me_bearb.rc");
	hMaskRl = MskOpenMask (NULL, "ME_EINZELNAENDERN");
	if (hMaskRl == NULL) {
		LogPrintf (MD_FAC_STP_VPLUEB, LT_ALERT,
			"User: %s MskOpenMask(%s)",
			who_am_i,
			"ME_EINZELNAENDERN");
		return RETURN_ERROR;
	}
	if((hMsh = SHELL_OF(hMaskRl)) == NULL) {
		if((ptPosCtx = (MY_CTX *)malloc(sizeof(MY_CTX))) == NULL) {
			MskDestroy(hMaskRl);
			return(RETURN_ERROR);
		}
		memset(ptPosCtx, 0, sizeof(MY_CTX));
		if ((ptPosCtx->piIsEinzelnAendernMaskOpen =
			(int *)malloc(sizeof(int))) == NULL) {
				return(RETURN_ERROR);
		}
		ptPosCtx->hLb = (iMask == MK_S) ?
			&ptStVpl->tStp.lb : &ptStVpl->tVplueb.hLb;
		ptPosCtx->hFirstMask = hMask;
		if (iMask == MK_S) {
			memcpy (&ptPosCtx->tStpLbCtx,
				&ptStVpl->tStp.ptLbRec, sizeof (LB_STP));;
		}
		MskRlMaskSet(hMaskRl, MskNmaskNoTopPlate,(Value)1);
		MskRlMaskSet (hMaskRl, MskNmaskCalldata,  (Value)ptPosCtx);
		MskRlMaskSet(hMaskRl, MskNmaskCallback,   (Value)Cb_MaskEinzelnAendern);
	} else {
		ptPosCtx = (MY_CTX*)MskDialogGet(hMaskRl, MskNmaskCalldata);
		MskRlMaskSet (hMaskRl, MskNmaskCalldata,  (Value)ptPosCtx);
	}
	if (iMask == MK_S) {
		memcpy(&ptPosCtx->tTpa, ptTpa, sizeof(TPA));
		sprintf(ptPosCtx->acInfo,"       %s %ld", MlMsg("TaNr"),
			ptTpa->tpaTaNr);
		pIsEinzelnAendernMaskOpen = &ptStVpl->tStp.iIsEinzelnAendernMaskOpen;
	} else {

		memcpy(&ptPosCtx->tVplk, ptVplk, sizeof(VPLK));
		sprintf(ptPosCtx->acInfo,"       %s %ld", MlMsg("KsNr"),
				ptVplk->vplkKsNr);

		pIsEinzelnAendernMaskOpen = &ptStVpl->tVplueb.iIsEinzelnAendernMaskOpen;
	}
	ptPosCtx->iWhichMask = iMask;
	ptPosCtx->ptStVpl = ptStVpl;
	ptPosCtx->piIsEinzelnAendernMaskOpen = pIsEinzelnAendernMaskOpen; 
	hMsh=ApShellModelessCreate(w, AP_CENTER, AP_CENTER);
	WamasWdgAssignMenu(hMsh,"ME_EINZELNAENDERN");
	MskCreateDialog (hMsh, hMaskRl, 
		MlMsg("Einzeln bearbeiten"), NULL, HSL_NI, SMB_All);
	MskUpdateMaskVar (hMaskRl);
	return (RETURN_ACCEPTED);

} /*_einzeln_aendern*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAllePriorisieren,ruft die Maske zum alle Priorisieren auf
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAllePriorisieren (MskDialog hMaskRl, MskStatic ef, 
							   MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX	*ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, 
		MskNmaskCalldata);
	ListBuffer      hLb;
	ListElement     hLiEle;
	LB_STP          *ptLbStp;
	LB_VPLUEB       *ptLbVplueb;
	int             nI, iLast, iMask;
	switch (reason) {
	case FCB_XF:
		iMask = ptMc->iWhichMask;
		hLb  =  (iMask == MK_S) ? ptMc->tStp.lb : ptMc->tVplueb.hLb;
		iLast = ListBufferLastElement (hLb);
		for (nI = 0; nI <= iLast; nI++) {
			hLiEle = ListBufferGetElement (hLb, nI);
			if( hLiEle == NULL ) continue;
			if( !(hLiEle->hint & LIST_HINT_SELECTED) )
				continue;
			if (iMask == MK_S) {
				ptLbStp = (LB_STP *)hLiEle->data;
				if (ptLbStp->TpaRecInDb.tpaStatus != TPASTATUS_NEU &&
					ptLbStp->TpaRecInDb.tpaStatus != TPASTATUS_SENDEN) {
						WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Transportaufträge lesen"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Transportauftrag hat keinen gültigen "
									"Status!\n TaNr: %d"), 
									ptLbStp->TpaRecInDb.tpaTaNr).c_str(),
							NULL);
						MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
						return (-1);
				}
			} else {
				ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
				if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_NEU &&
					ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_BEREIT &&
					ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
						WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Kommissionieraufträge lesen"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Kommissionierauftrag hat keinen "
									"gültigen Status!\n KsNr: %d"), 
									ptLbVplueb->tVplk.vplkKsNr).c_str(),
							NULL);
						MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
						return (-1);
				}
			}
		}
		if (_alle_prio (GetRootShell (), ptMc) < 0) { 
			return (EFCBM_CONTINUE);
		}
		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return(EFCBM_CONTINUE);
}  /*  CbAllePriorisieren()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAlleZuteilen,ruft die Maske zum alle Zuiteilen auf 
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleZuteilen (MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
						   int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX	*ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, 
		MskNmaskCalldata);
	ListBuffer		hLb;
	ListElement  	hLiEle;
	LB_STP      	*ptLbStp;
	LB_VPLUEB      	*ptLbVplueb;
	int				nI, iLast, iMask;
	switch (reason) {
	case FCB_XF:
		iMask = ptMc->iWhichMask;
		hLb  =  (iMask == MK_S) ? ptMc->tStp.lb : ptMc->tVplueb.hLb;
		iLast = ListBufferLastElement (hLb);
		for (nI = 0; nI <= iLast; nI++) {
			hLiEle = ListBufferGetElement (hLb, nI);
			if( hLiEle == NULL ) continue;
			if( !(hLiEle->hint & LIST_HINT_SELECTED) )
				continue;
			if (iMask == MK_S) {
				ptLbStp = (LB_STP *)hLiEle->data;
				if (ptLbStp->TpaRecInDb.tpaStatus != TPASTATUS_NEU &&
					ptLbStp->TpaRecInDb.tpaStatus != TPASTATUS_SENDEN) {
						WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Transportaufträge zuteilen"), 
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Transportauftrag hat keinen gültigen " 
									"Status!\n TaNr: %d"), 
									ptLbStp->TpaRecInDb.tpaTaNr).c_str(),
							NULL);
						MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
						return (-1);
				}
			} else {
				ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
				if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_NEU &&
					ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_BEREIT &&
					ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
						WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Kommissionieraufträge zuteilen"), 
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Kommissionierauftrag hat keinen "
									"gültigen Status!\n KsNr: %d"), 
									ptLbVplueb->tVplk.vplkKsNr).c_str(),
							NULL);
						MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
						return (-1);
				}
			}
		}
		if (_alle_zuteilen (GetRootShell (), ptMc) < 0) { 
			return (EFCBM_CONTINUE);
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /*  CbAlleZuteilen()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Verschmelzen - Callback
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbVerschmelzen(MskDialog hMaskRl, MskStatic ef, MskElement hEfRl,
						  int reason, void *cbc, void *calldata)
{
	VPLK            tVplk;
	VPLK            tVplkMain;
	VPLK           *ptVplk     = NULL;
	VPLK           *ptVplkMain = NULL;
	STP_VPLUEB_CTX *ptMc       = NULL;
	ListElement     ptLbe      = NULL;
	ListBuffer      ptLb       = NULL;
	long	lMergeParent = 0;
	time_t zNow = 0;
	int nI           = 0;
	int iRv          = 0;
	int iCmp         = 0;
	int iLast        = 0;
	int iRet         = 0;
	int iRetVplp     = 0;
	int iStartStatus = 0;
	ptMc = (STP_VPLUEB_CTX *)MskDialogGet (hMaskRl, MskNmaskCalldata);
	if (ptMc == NULL || (ptLb = ptMc->tVplueb.hLb) == NULL) {
		return (-1);
	}
	switch (reason) {
	case FCB_XF:
		if (LBNumberOfSelected (ptLb) < 1) {
			TSqlRollback (hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,      MlM("Bitte Datensätze selektieren !\n"),
				NULL);
			return (-1);
		}
		if (WamasBox(SHELL_OF(hMaskRl),
			WboxNboxType,   WBOX_WARN,
			WboxNbuttonText,MlM("OK"),
			WboxNbuttonRv,  IS_Ok,
			WboxNbutton,    WboxbNcancel,
			WboxNescButton, WboxbNcancel,
			WboxNmwmTitle,  MlM("Verschmelzen!"),
			WboxNtext,      MlM("Selektierte Aufträge verschmelzen?"),
			NULL) != IS_Ok){
				TSqlRollback (hMaskRl);
				return (-1);
		}
		if (PrmGet1Parameter(hMaskRl, P_MergeParent, PRM_CACHE, &lMergeParent) != PRM_OK ) {
			TSqlRollback (hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,      
					wamas::platform::string::form(MlMsg(
						"Parameter %s konnte nicht gelesen werden!\n"), 
						P_MergeParent).c_str(),
				NULL);
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
				"Parameter %s could not be read: %s",
				P_MergeParent,
				TSqlErrTxt(hMaskRl) );
			return (-1);
		}
		iLast = ListBufferLastElement (ptLb);
		if ((iStartStatus = GetStartVplkStatus (hMaskRl, MD_FAC_VPLUEB)) < 0) {
			TSqlRollback (hMaskRl);
			return (-1);
		}
		for (nI = 0; nI <= iLast; nI++) {
			if ((ptLbe = ListBufferGetElement (ptLb, nI)) == NULL) {
				continue;
			}
			if ((ptLbe->hint & LIST_HINT_SELECTED) != LIST_HINT_SELECTED) {
				continue;
			}
			/* Spreadsheetrecord ist selektiert, weiter machen */
			ptVplk = &(((LB_VPLUEB *)(ptLbe->data))->tVplk);
			if (ptVplk->vplkStatus != VPLKSTATUS_NEU &&
				ptVplk->vplkStatus != VPLKSTATUS_BEREIT &&
				ptVplk->vplkStatus != VPLKSTATUS_WARTEN) {
					LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
						"Selektierter Auftrag KsNr: %ld kann "
						"nicht verschmolzen werden.\n"
						"Status ist ungleich StartStatus %ld != %d",
						ptVplk->vplkKsNr,
						ptVplk->vplkStatus,
						iStartStatus);
					WamasBox (SHELL_OF (hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld kann nicht verschmolzen werden!\n"
								"Status != %s"), 
								ptVplk->vplkKsNr, 
								tl2sGetShortLabelByValue (&tl2s_VPLKSTATUS, 
									iStartStatus)).c_str(),
						NULL);
					TSqlRollback (hMaskRl);
					return (-1);
			}
			if (ptVplkMain == NULL) {
				ptVplkMain = ptVplk;
			} else {
				if (lMergeParent > 0) {
					/* Hauptkriterium ist die Anz. der Positionen */
					if (ptVplkMain->vplkAnzPos < ptVplk->vplkAnzPos) {
						ptVplkMain = ptVplk;
						continue; /* keine weiteren Checks */
					} else if (ptVplkMain->vplkAnzPos > ptVplk->vplkAnzPos) {
						continue; /* der alte bleibt Parent */
					} else { /* gleich viele Positionen */
						/* Bei unterschiedlicher Auftragsnummer ist der Auftrag
						* mit der kleineren Auftragsnummer der dominante Auftrag
						*/
						iCmp = strcmp (ptVplkMain->vplkAusId.AusNr,
							ptVplk->vplkAusId.AusNr);
						if (iCmp > 0) {
							ptVplkMain = ptVplk;
						} else if (iCmp == 0) {
							/* Bei gleicher Auftragsnummer ist der Auftrag
							* mit der groesseren Sollmenge der dominante Auftrag
							*/
							if (ptVplkMain->vplkSollHE < ptVplk->vplkSollHE) {
								ptVplkMain = ptVplk;
							}
						}
					}
				} else {
					/* Bei unterschiedlicher Auftragsnummer ist der Auftrag
					* mit der kleineren Auftragsnummer der dominante Auftrag
					*/
					iCmp = strcmp (ptVplkMain->vplkAusId.AusNr,
						ptVplk->vplkAusId.AusNr);
					if (iCmp > 0) {
						ptVplkMain = ptVplk;
					} else if (iCmp == 0) {
						/* Bei gleicher Auftragsnummer ist der Auftrag
						* mit der groesseren Sollmenge der dominante Auftrag
						*/
						if (ptVplkMain->vplkSollHE < ptVplk->vplkSollHE) {
							ptVplkMain = ptVplk;
						}
					}
				}
			}
		}
		if (ptVplkMain == NULL) {
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
				"Selektierter Auftrag KsNr: %ld kann "
				"nicht verschmolzen werden.\n"
				"Interner Fehler",
				ptVplk->vplkKsNr);
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"KsNr:%ld kann nicht verschmolzen werden!\n"
						"Interner Fehler"), 
						ptVplk->vplkKsNr).c_str(),
				NULL);
			TSqlRollback (hMaskRl);
			return (-1);
		}
		LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
			"Main Vplk -> KsNr: %ld AusNr: %s AnzPos: %ld",
			ptVplkMain->vplkKsNr,
			ptVplkMain->vplkAusId.AusNr,
			ptVplkMain->vplkAnzPos);
		memset (&tVplkMain, '\0', sizeof (tVplkMain));
		tVplkMain.vplkKsNr = ptVplkMain->vplkKsNr;
		iRv = TExecStdSql (hMaskRl, StdNselectUpdNo, TN_VPLK, &tVplkMain);
		if (iRv <= 0) {
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
				"Selektierter Auftrag KsNr: %ld konnte nicht "
				"gelockt werden!",
				ptVplkMain->vplkKsNr);
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"KsNr:%ld kann nicht verschmolzen werden!\n"
						"Der Auftrag konnte nicht gelockt werden!"), 
						ptVplkMain->vplkKsNr).c_str(),
				NULL);
			TSqlRollback (hMaskRl);
			return (-1);
		}
		if ( memcmp(&tVplkMain, ptVplkMain, sizeof (tVplkMain)) != 0) {
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
				"Selektierter Auftrag KsNr: %ld kann nicht "
				"verschmolzen werden."
				"Auftragsdaten wurden inzwischen geändert",
				tVplkMain.vplkKsNr);
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"KsNr:%ld kann nicht verschmolzen werden!\n"
						"Die Auftragsdaten wurden inzwischen geändert!"), 
						tVplkMain.vplkKsNr).c_str(),
				NULL);
			TSqlRollback (hMaskRl);
			return (-1);
		}

		for (nI = 0; nI <= iLast; nI++) {
			if ((ptLbe = ListBufferGetElement (ptLb, nI)) == NULL) {
				continue;
			}
			if ((ptLbe->hint & LIST_HINT_SELECTED) != LIST_HINT_SELECTED) {
				continue;
			}
			/* Spreadsheetrecord ist selektiert, weiter machen */
			ptVplk = &(((LB_VPLUEB *)(ptLbe->data))->tVplk);
			if (ptVplk->vplkKsNr == tVplkMain.vplkKsNr) {
				LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
					"Main KsNr: %ld found",
					tVplkMain.vplkKsNr);
				continue;
			}
			iRet = CmpVplk4Verschmelzen (hMaskRl, MD_FAC_VPLUEB,
				&tVplkMain,
				ptVplk);

			if (iRet < 0) {

				TSqlRollback(hMaskRl);

				switch (iRet) {

				case -1: // FT
				default:
					/* Wichtige Auftragsdaten sind verschieden */
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Daten sind nicht gleich."), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
					break;
				case -2:
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Unterschiedliche Tour-Nr "
								"und Tour-Belegungen."), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
					break;
				case -3:
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Unterschiedliche Tour-Nr."), 
								tVplkMain.vplkKsNr, 
								ptVplk->vplkKsNr).c_str(),
						NULL);
					break;
				case -4:
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Unterschiedliche Tour-Belegungen."), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
					break;
				case -5:
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Unterschiedliche ALMA Lagerzonen-Einstellung."), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
					break;
				}
				return  (-1);

			} else if (iRet == 0) {
				/* Unterschiedlicher Liefertermin (Verschmelzen?!) */
				if (WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbuttonText,MlM ("Verschmelzen"),
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM ("Verschmelzen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"KsNr:%ld und %ld können nicht\n"
							"verknüpft werden!\n"
							"Daten sind nicht gleich.\n"
							"Unterschiedlicher Liefertermin.\n"
							"Trotzdem fortfahren?"), 
							tVplkMain.vplkKsNr,  
							ptVplk->vplkKsNr).c_str(),
					NULL) != IS_Ok) {
						TSqlRollback (hMaskRl);
						return(-1);
				} 
			}

			iRetVplp = CheckVplp4Melt (hMaskRl, tVplkMain.vplkKsNr,
				ptVplk->vplkKsNr, MD_FAC_VPLUEB );
			if (iRetVplp < 0) {
				if (iRetVplp == -105) {
					/* Mengenangaben passen nicht zusammen (PCS vs KOLLI) */
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Zuviele Positionen haben\n"
								"unterschiedliche VeEinheiten.\n"
								"(Stück - Kolli)"), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
				} else {
					/* Allgemeiner Fehler beim Lesen der VPLPs */
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Verschmelzen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"KsNr:%ld und %ld können nicht\n"
								"verschmolzen werden!\n"
								"Allgemeiner Fehler beim Lesen\n"
								"der Positionen.\n"), 
								tVplkMain.vplkKsNr,  
								ptVplk->vplkKsNr).c_str(),
						NULL);
				}
				return(-1);
			}
			memset (&tVplk, 0, sizeof (tVplk));
			tVplk.vplkKsNr = ptVplk->vplkKsNr;
			iRv = TExecStdSql (hMaskRl, StdNselectUpdNo, TN_VPLK, &tVplk);
			if (iRv <= 0) {
				LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
					"Selektierter Auftrag KsNr: %ld konnte nicht "
					"gelesen/gelockt werden.",
					tVplk.vplkKsNr);
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Verschmelzen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"KsNr:%ld kann nicht verschmolzen werden!\n"
							"Der Auftrag konnte nicht gelockt werden!"), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback (hMaskRl);
				return(-1);
			}

			if (memcmp (&tVplk, ptVplk, sizeof (tVplk)) != 0) {
				LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
					"Selektierter Auftrag KsNr: %ld kann nicht "
					"verschmolzen werden."
					"Auftragsdaten wurden inzwischen geändert",
					tVplk.vplkKsNr);
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Verschmelzen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"KsNr:%ld kann nicht verschmolzen werden!\n"
							"Die Auftragsdaten wurden inzwischen geändert!"), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback (hMaskRl);
				return (-1);
			}
			/* VPLK-Aufträge verschmelzen */
			if (iRetVplp == 100) {
				if (AuftragVerschmelzen (hMaskRl, MD_FAC_VPLUEB,
					&tVplkMain, &tVplk) < 0) {
						WamasBox (SHELL_OF (hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Verschmelzen"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Fehler beim Verschmelzen "
									" KsNr %ld -> KsNr %ld !\n"), 
								tVplkMain.vplkKsNr, 
								tVplk.vplkKsNr).c_str(),
							NULL);
						TSqlRollback (hMaskRl);
						return(-1);
				}
			} else {
				if (AuftragVerschmelzen (hMaskRl, MD_FAC_VPLUEB,
					&tVplkMain, &tVplk) < 0) {
						WamasBox (SHELL_OF (hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Verschmelzen"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Fehler beim Verschmelzen "
									" KsNr %ld -> KsNr %ld !\n"), 
									tVplkMain.vplkKsNr, 
									tVplk.vplkKsNr).c_str(),
							NULL);
						TSqlRollback (hMaskRl);
						return(-1);
				}
			}
		}
		iRv = TExecStdSql (hMaskRl, StdNselect, TN_VPLK, &tVplkMain);
		if (iRv <= 0) {
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT, "Fehler beim Lesen VPLK");
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"KsNr:%ld kann nicht verschmolzen werden!\n"
						"Der Auftrag konnte nicht gelesen werden!"), 
						tVplkMain.vplkKsNr).c_str(),
				NULL);
			TSqlRollback (hMaskRl);
			return(-1);
		}
		zNow = time (0);
		if (tVplkMain.vplkSollStartZeit < zNow) {
			iRv = WamasBox (SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_WARN,
				WboxNbuttonText,MlM("Verschmelzen"),
				WboxNbuttonRv,  IS_Ok,
				WboxNbutton,    WboxbNcancel,
				WboxNescButton, WboxbNcancel,
				WboxNmwmTitle,  MlM("Verschmelzen"),
				WboxNtext,      MlM (
					"Kommissionierung bis zur vor-\n"
					"gegebenen Stellzeit nicht möglich!\n"
					"Trotzdem verschmelzen?"),
				NULL);
			if (iRv != IS_Ok) {
				TSqlRollback (hMaskRl);
				return -1;
			}
		}
		TSqlCommit (hMaskRl);
		if (ptMc->iWhichMask == MK_S) {
			LB_DeSellectAll (ptMc->tStp.lb);
			PosLesenStp (ptMc->hFirstMask, OPT_ERRORBOX);
			MskUpdateMaskVar (ptMc->hFirstMask);
		} else {
			LB_DeSellectAll (ptMc->tVplueb.hLb);
			if (ListUpdateVplueb (ptMc->hFirstMask,
				&(((STP_VPLUEB_CTX *)(ptMc->ptStVplueb))->tVplueb), 
				MANUELL_LEVEL) < 0) {
				ptMc->tVplueb.Aktion = MANUELL_READ;
				TSqlRollback (ptMc->hFirstMask);
			}
			MskUpdateMaskVar (ptMc->hFirstMask);
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /*  CbVerschmelzen()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAlleAendern-Callback, hier wird die Änderungs-Maske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleAendern (MskDialog hMaskRl, MskStatic ef,
						  MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX	*ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl, 
		MskNmaskCalldata);
	ListBuffer      hLb;
	ListElement     hLiEle;
	LB_VPLUEB       *ptLbVplueb;
	int         	iRv, iLast, nI;
	int				iCheckOnce = 1;
	switch (reason) {
	case FCB_XF:
		/* Status check */
		hLb = ptMc->tVplueb.hLb;
		iLast = ListBufferLastElement (hLb);
		for (nI = 0; nI <= iLast; nI++) {
			hLiEle = ListBufferGetElement (hLb, nI);
			if( hLiEle == NULL ) continue;
			if( !(hLiEle->hint & LIST_HINT_SELECTED) )
				continue;
			ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
			if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_NEU &&
				ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_BEREIT &&
				ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_AKTIV &&
				ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionieraufträge ändern"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Kommissionierauftrag hat keinen "
								"gültigen Status!\n KsNr: %d"), 
								ptLbVplueb->tVplk.vplkKsNr).c_str(),
						NULL);
					MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
					return (-1);
			}
		}

		iRv = me_vpl_rueck (GetRootShell(), ptMc, iCheckOnce);
		if (iRv < 0) {
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,  WBOX_ALERT,
				WboxNbutton,   WboxbNok,
				WboxNmwmTitle, MlM("Auftrag ändern"),
				WboxNtext,
				MlM("Es ist ein Fehler beim Ändern aufgetreten !!"),
				NULL);
			TSqlRollback(hMaskRl);
		}
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /*  CbAlleAendern()  */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragPriorisieren-Callback, hier wird die Priorisierenmaske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleAuftraegeAbschliessen (MskDialog hMaskRl, MskStatic ef,
										MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX  *ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl,
		MskNmaskCalldata);
	TPA				tTpa;
	VPLK			tVplk;
	ListBuffer      hLb;
	ListElement     hLiEle;
	LB_VPLUEB       *ptLbVplueb = NULL;
	int             nI, iLast;
	int				iMask = ptMc->iWhichMask;
	switch (reason) {
	case FCB_XF:
		memset (&tTpa, 0, sizeof(TPA));
		memset (&tVplk, 0, sizeof(VPLK));
		hLb  =  (iMask == MK_S) ? ptMc->tStp.lb : ptMc->tVplueb.hLb;
		iLast = ListBufferLastElement (hLb);
		for (nI = 0; nI <= iLast; nI++) {
			hLiEle = ListBufferGetElement (hLb, nI);
			if( hLiEle == NULL ) continue;
			if( !(hLiEle->hint & LIST_HINT_SELECTED) )
				continue;
			if (iMask == MK_V) {
				ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
				if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_NACH) {						
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionieraufträge lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Kommissionierauftrag hat keinen "
								"gültigen Status!\n KsNr: %d"), 
								ptLbVplueb->tVplk.vplkKsNr).c_str(),
						NULL);
					MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
					return (-1);
				}
			}
			if (ptLbVplueb->tVplk.vplkStatus == VPLKSTATUS_NACH) {
				if (WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbuttonText,MlM ("Abschliessen"),
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM ("Abschliessen"),
					WboxNtext,
						wamas::platform::string::form(MlMsg (
							"Auftrag KsNr: %d hat nicht Status 'FEHLT'!\n"
							"Wollen Sie trotzdem abschliessen?"), 
							ptLbVplueb->tVplk.vplkKsNr).c_str(),
					NULL) != IS_Ok) {
						return (-1);
				}
			}
		}
		/* Status check for Transport orders must also be done*/
		if (iMask == MK_S) {
			if (me_abs (GetRootShell (), &ptMc->tStp.lb, tVplk, tTpa,
				ptMc, hMaskRl, iMask, MODE_ALLE , JANEIN_J ) < 0) {
					return (EFCBM_CONTINUE);
			}
		} else {
			if (me_abs (GetRootShell (), &ptMc->tVplueb.hLb, tVplk, tTpa,
				ptMc, hMaskRl, iMask, MODE_ALLE , JANEIN_J) < 0) {
					return (EFCBM_CONTINUE);
			}
		}
		/*
		hButton = (OWidget)MskElementGet(
		MskQueryRl (hMaskRl, MskGetElement("FilterLesen_B"), 0), MskNwMain);
		if (hButton) {
		WdgLeftButtonEvent (hButton, 0, 0);
		}
		*/
		miAbschliessen = 1;
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /* CbAlleAuftraegeAbschliessen() */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAuftragPriorisieren-Callback, hier wird die Priorisierenmaske aufgerufen
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleAuftraegeStorno (MskDialog hMaskRl, MskStatic ef,
								  MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX  *ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl,
		MskNmaskCalldata);
	TPA				tTpa;
	VPLK			tVplk;
	ListBuffer      hLb;
	ListElement     hLiEle;
	LB_VPLUEB       *ptLbVplueb;
	int             nI, iLast , iRv;
	int				iMask = ptMc->iWhichMask;
	switch (reason) {
	case FCB_XF:
		iRv = WamasBox (SHELL_OF(hMaskRl),
			WboxNboxType,   WBOX_WARN,
			WboxNbuttonText,MlM("Storno"),
			WboxNbuttonRv,  IS_Ok,
			WboxNbutton,    WboxbNcancel,
			WboxNescButton, WboxbNcancel,
			WboxNmwmTitle,  MlM("Kommissionierschein stornieren"),
			WboxNtext,
			MlM (
			"Wollen Sie diese Auftraege \n"
			"wirklich stornieren?"),
			NULL);
		if (iRv != IS_Ok) {
			return(-1);
		}
		memset (&tTpa, 0, sizeof(TPA));
		memset (&tVplk, 0, sizeof(VPLK));
		hLb  =  (iMask == MK_S) ? ptMc->tStp.lb : ptMc->tVplueb.hLb;
		iLast = ListBufferLastElement (hLb);
		for (nI = 0; nI <= iLast; nI++) {
			hLiEle = ListBufferGetElement (hLb, nI);
			if( hLiEle == NULL ) continue;
			if( !(hLiEle->hint & LIST_HINT_SELECTED) )
				continue;
			if (iMask == MK_V) {
				ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
				if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionieraufträge lesen"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Kommissionierauftrag hat keinen "
								"gültigen Status!\n KsNr: %d"), 
								ptLbVplueb->tVplk.vplkKsNr).c_str(),
						NULL);
					MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
					return (-1);
				}
			}
		}
		/* Status check for Transport orders must also be done*/
		if (iMask == MK_S) {
			if (me_abs (GetRootShell (), &ptMc->tStp.lb, tVplk, tTpa,
				ptMc, hMaskRl, iMask, MODE_ALLE , JANEIN_N ) < 0) {
					return (EFCBM_CONTINUE);
			}
		} else {
			if (me_abs (GetRootShell (), &ptMc->tVplueb.hLb, tVplk, tTpa,
				ptMc, hMaskRl, iMask, MODE_ALLE , JANEIN_N ) < 0) {
					return (EFCBM_CONTINUE);
			}
		}
		/*
		hButton = (OWidget)MskElementGet(
		MskQueryRl (hMaskRl, MskGetElement("FilterLesen_B"), 0), MskNwMain);
		if (hButton) {
		WdgLeftButtonEvent (hButton, 0, 0);
		}
		*/
		miAbschliessen = 1;
		MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /* CbAlleAuftraegeStorno() */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int	TpaSperren (MskDialog hMaskRl, STP_VPLUEB_CTX *ptMc) 
{
	int		iLast = 0;
	LB_STP				*pLbStpRec;
	ListElement         hLiEle;
	TPA					tTpa, tTpaOld;
	int					iDbRv = 0;
	int                 nI = 0, iRet = 0;
	if (WamasBox(SHELL_OF(hMaskRl),
		WboxNboxType,   WBOX_WARN,
		WboxNbuttonText,MlM("OK"),
		WboxNbuttonRv,  IS_Ok,
		WboxNbutton,    WboxbNcancel,
		WboxNescButton, WboxbNcancel,
		WboxNmwmTitle,  MlM("Transportauftrag sperren/entsperren"),
		WboxNtext,      
		MlM("Selektierte Transportauftraege sperren"
		" bzw. entsperren?"),
		NULL) != IS_Ok){
			return (-1);
	}
	MskTransferMaskDup(hMaskRl);
	iLast = ListBufferLastElement(ptMc->tStp.lb);  
	for (nI = 0; nI <= iLast; nI++) {
		hLiEle =  ListBufferGetElement(ptMc->tStp.lb,nI); 
		if( hLiEle == NULL ) {
			continue;
		}
		if( !(hLiEle->hint & LIST_HINT_SELECTED) )
			continue;
		pLbStpRec = (LB_STP*)hLiEle->data;
		memset (&tTpa, 0, sizeof (TPA));
		memset (&tTpaOld, 0, sizeof (TPA));
		memcpy (&tTpa, &pLbStpRec->TpaRecNow, sizeof (TPA));
		memcpy (&tTpaOld, &pLbStpRec->TpaRecNow, sizeof (TPA));
		iDbRv =  TExecStdSql(hMaskRl, StdNselectUpdNo, TN_TPA, &tTpa); 
		if (iDbRv <= 0) {
			if (TSqlError(hMaskRl)!=SqlNotFound) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler beim Lesen.TaNr:%d"),   
							pLbStpRec->TpaRecNow.tpaTaNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return(-1);
			} else {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"TaNr:%d ist nicht mehr vorhanden."), 
							pLbStpRec->TpaRecNow.tpaTaNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return(-1);
			}
		}
		if (tTpa.tpaStatus  == TPASTATUS_WARTEN) {
			/* Status Handling */
			iRet = 0;
			iRet = Tpa_Sperren_Entsperren (hMaskRl, MD_FAC_STP, 
				tTpa.tpaTaNr, TPASTATUS_FERTIG);
			if (iRet < 0) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler bei Freigabe.TaNr :%ld."), 
							tTpa.tpaTaNr).c_str(),
					NULL);
				LogPrintf (MD_FAC_STP,LT_ALERT,
					"Fehler bei SetTpaStatus TaNr: %ld, TSqlErr: %s",
					tTpa.tpaTaNr, 
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return -1;
			}
			CF_TRIGGER(PROC_TAM, FUNC_TAM_ATPA, 0, "Sperre");
		} else {
			if (tTpa.tpaStatus != TPASTATUS_FERTIG) {
				/* Status HandlingWarten */
				iRet = 0;
				iRet = Tpa_Sperren_Entsperren (hMaskRl, MD_FAC_STP, 
					tTpa.tpaTaNr, TPASTATUS_WARTEN);
				if (iRet < 0) {
					WamasBox (SHELL_OF (hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Sperren/Entsperren"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Fehler bei Freigabe. TaNr:%ld."), 
								tTpa.tpaTaNr).c_str(),
						NULL);
					LogPrintf (MD_FAC_STP,LT_ALERT,
						"Fehler beim SetTpaStatus. TaNr: %ld, "
						"TSqlErr: %s",
						tTpa.tpaTaNr, 
						TSqlErrTxt(hMaskRl));
					TSqlRollback(hMaskRl);
					return -1;
				}
			}
		}
	}
	return(1);
} /* TpaSperren () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAlleAuftrageSperren,sperrt/entsperrt alle selektierte Kommissionsscheine 
-*		Hoststatus -> SENDEN
-* RETURNS
-*--------------------------------------------------------------------------*/
int	VplSperren (MskDialog hMaskRl, STP_VPLUEB_CTX *ptMc) 
{
	LB_VPLUEB           *pLbVpluebRec;
	ListElement         hLiEle;
	VPLI                tVpli;
	VPLK                tVplk, tVplkOld;
	time_t              zTime;
	char                acZeit[20];
	int					iDbRv = 0;
	int                 iLast, nI, iRet, iSperreKz = 0;;
	memset (&tVpli, 0, sizeof (VPLI));
	memset (acZeit, 0, sizeof (acZeit));
	zTime = time ((time_t *)0);
	strftime (acZeit, sizeof (acZeit) - 1, "%H:%M %d.%m",
		localtime (&zTime));

	if (WamasBox(SHELL_OF(hMaskRl),
		WboxNboxType,   WBOX_WARN,
		WboxNbuttonText,MlM("OK"),
		WboxNbuttonRv,  IS_Ok,
		WboxNbutton,    WboxbNcancel,
		WboxNescButton, WboxbNcancel,
		WboxNmwmTitle,  
		MlM("Kommissionierschein sperren/entsperren"),
		WboxNtext,      
		MlM("Selektierte Kommissionierscheine sperren"
		" bzw. entsperren?"),
		NULL) != IS_Ok){
			return (-1);
	}
	MskTransferMaskDup(hMaskRl);
	iLast = ListBufferLastElement(ptMc->tVplueb.hLb);  
	for (nI = 0; nI <= iLast; nI++) {
		hLiEle =  ListBufferGetElement(ptMc->tVplueb.hLb, nI); 
		if( hLiEle == NULL ) {
			continue;
		}
		if( !(hLiEle->hint & LIST_HINT_SELECTED) ) {
			continue;
		}
		pLbVpluebRec = (LB_VPLUEB*)hLiEle->data;
		memset (&tVplk, 0, sizeof (VPLK));
		memset (&tVplkOld, 0, sizeof (VPLK));
		memcpy (&tVplk, &pLbVpluebRec->tVplk, sizeof (VPLK));
		memcpy (&tVplkOld, &pLbVpluebRec->tVplk, sizeof (VPLK));
		if (tVplk.vplkStatus == VPLKSTATUS_AKTIV ) {
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Sperren/Freigeben"),
				WboxNtext, 
					wamas::platform::string::form(MlMsg(
						"Selektierter Auftrag wird"
						" bereits bearbeitet.\nKsNr:%d."), 
						tVplk.vplkKsNr).c_str(),
				NULL);
			TSqlRollback(hMaskRl);
			return(-1);
		}
		iDbRv = TExecStdSql(hMaskRl, StdNselectUpdNo, TN_VPLK, &tVplk); 
		if (iDbRv <= 0) {
			if (TSqlError(hMaskRl)!=SqlNotFound) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren!"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler beim Lesen.KsNr:%d "), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return(-1);
			} else {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren!"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"KsNr:%d ist nicht mehr vorhanden."), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				return(-1);
			}
		}
		if (tVplk.vplkStatus == VPLKSTATUS_WARTEN) {
			/* Status Handling */
			iRet = 0;
			tVplk.vplkStatus = VPLKSTATUS_FERTIG;
			iRet =  StatMan_HandleVplk(hMaskRl, MD_FAC_VPLUEB,  &tVplk);
			if (iRet < 0) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Entsperren"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Fehler bei Freigabe.KsNr :%d."), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				LogPrintf (MD_FAC_VPLUEB,LT_ALERT,
					"Fehler beim StatMan_HandleVplk. "
					"KsNr: %ld, TSqlErr: |%s|",
					tVplk.vplkKsNr, 
					TSqlErrTxt(hMaskRl));
				return (-1);
			}
		} else {
			if (tVplk.vplkStatus != VPLKSTATUS_KOMM &&
				tVplk.vplkStatus != VPLKSTATUS_FERTIG) {
					iSperreKz = 1;
					/* Status HandlingWarten */
					iRet = 0;
					iRet =  StatMan_SetVplkWarten (hMaskRl, MD_FAC_VPLUEB, &tVplk);
					if (iRet < 0) {
						WamasBox (SHELL_OF (hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Sperren/Entsperren"),
							WboxNtext,
								wamas::platform::string::form(MlMsg(
									"Fehler bei Freigabe.Kommissionierschein:%d."), 
									tVplk.vplkKsNr).c_str(),
							NULL);
						LogPrintf (MD_FAC_VPLUEB,LT_ALERT,
							"Fehler beim StatMan_HandleVplk. "
							"KsNr: %ld, TSqlErr: %s",
							tVplk.vplkKsNr, 
							TSqlErrTxt (hMaskRl));
						return (-1);
					}
			} else {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Sperren/Freigeben"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Selektierter Auftrag wurde"
							" bereits bearbeitet.\nKsNr:%ld."), 
							tVplk.vplkKsNr).c_str(),
					NULL);
				TSqlRollback(hMaskRl);
				return(-1);
			}
		}
		tVpli.vpliKsNr = tVplk.vplkKsNr;
		tVpli.vpliIdx  = 0;
		tVpli.vpliTyp = VPLITYP_INTERN;
		tVpli.vpliArt = VPLIART_KOPF;
		SetHist(TN_VPLI, &tVpli, HIST_INSERT, GetUserOrTaskName());
		tVpli.vpliPosNr = GetNextPosNrVpli (hMaskRl, MD_FAC_VPLUEB, TN_VPLI,
			tVplk.vplkKsNr,VPLIART_KOPF, VPLITYP_INTERN, 0);
		if (tVpli.vpliPosNr == -1) {
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Sperren/Entsperren"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"Fehler beim Insert in VPLI\nKsNr: %ld"), 
						tVplk.vplkKsNr).c_str(), 
				NULL);
			LogPrintf (MD_FAC_VPLUEB, LT_ALERT,
				"Fehler beim GetNextPosNrVpli in VPLI. "
				"KsNr:%ld, TSqlErr: %s",
				tVplk.vplkKsNr,
				TSqlErrTxt (hMaskRl));
			TSqlRollback(hMaskRl);
			return (-1);
		}
		if (iSperreKz == 1) {
			StrCpy (tVpli.vpliText,
				wamas::platform::string::form(MlMsg ("User: %s Zeit: %s, "
					"Kommissionierschein KsNr:%ld wurde entsperrt"), 
					GetUserOrTaskName(), 
					acZeit, 
					tVplk.vplkKsNr));
		} else {
			StrCpy (tVpli.vpliText,
				wamas::platform::string::form(MlMsg ("User: %s Zeit: %s, "
					"Kommissionierschein KsNr:%ld wurde gesperrt"), 
					GetUserOrTaskName(), 
					acZeit, 
					tVplk.vplkKsNr));
		}
		tVpli.vpliText[INFTEXT_LEN] = '\0';
		iDbRv = TExecStdSql(hMaskRl, StdNinsert, TN_VPLI, &tVpli);
		if (iDbRv <= 0) {
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Sperren/Entsperren"),
				WboxNtext,
					wamas::platform::string::form(MlMsg(
						"Fehler beim Insert in VPLI\nKsNr: %ld."), 
						tVplk.vplkKsNr).c_str(),
				NULL);
			LogPrintf (MD_FAC_VPLUEB,LT_ALERT,
				"Fehler beim Insert in VPLI. KsNr: %ld. TSqlErr: %s",
				tVplk.vplkKsNr,
				TSqlErrTxt(hMaskRl));
			return (-1);
		}
	}
	return(1);
}  /*  VplSperren()  */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Splittet Scheine nach Paletten-Größe
-*
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleAuftraegePalSplit (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata) {


	STP_VPLUEB_CTX  *ptMc = (STP_VPLUEB_CTX *)MskDialogGet(hMaskRl,
			MskNmaskCalldata);

	switch (reason) {

	case FCB_XF:
		{
			ListBuffer 		hLb 	= ptMc->tVplueb.hLb;
			ListElement     hLiEle  = NULL;
			LB_VPLUEB       *ptLbVplueb;
			int 			iLast 	= ListBufferLastElement (hLb);
			vector <VPLK> 	vVplk;
			set<string>		sUniqueLazNr;

			for (int nI = 0; nI <= iLast; nI++) {

				hLiEle = ListBufferGetElement (hLb, nI);
				if( hLiEle == NULL ) {
					continue;
				}
				if( !(hLiEle->hint & LIST_HINT_SELECTED) ) {
					continue;
				}

				ptLbVplueb = (LB_VPLUEB *)hLiEle->data;
				if (ptLbVplueb->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein splitten"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Splitten ist nicht möglich.\n"
								"Kommissionierschein %ld hat nicht den Status GESPERRT."), 
								ptLbVplueb->tVplk.vplkKsNr).c_str(),
						NULL);
					vVplk.clear();
					MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
					break;
				}

				if ( ptLbVplueb->tVplk.vplkBearbPos > 0) {
					WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Kommissionierschein splitten"),
						WboxNtext,
							wamas::platform::string::form(MlMsg(
								"Splitten ist nicht möglich.\n"
								"Kommissionierschein %ld muss Bearbeitungsstand "
								"von 0 Prozent haben."), 
								ptLbVplueb->tVplk.vplkKsNr).c_str(),
						NULL);
					vVplk.clear();
					MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
					break;
				}

				vVplk.push_back(ptLbVplueb->tVplk);
				sUniqueLazNr.insert(ptLbVplueb->tVplk.vplkLazNr);
			}
			if (sUniqueLazNr.size() > 1) {
				if (WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbutton,    WboxbNok,
					WboxNbuttonRv,  IS_Ok,
					WboxNbutton,    WboxbNcancel,
					WboxNescButton, WboxbNcancel,
					WboxNmwmTitle,  MlM("Kommissionierschein splitten"),
					WboxNtext,
						MlM("Es wurden Kommissionierscheine aus\n"
						    "unterschiedlichen Lagerzonen selektiert.\n"
						    "Trotzdem splitten ?"),
					NULL) != IS_Ok) {

						vVplk.clear();
				}
			}

			if (!vVplk.empty()) {
				_me_kssplitpaleingabe(GetRootShell(), &vVplk);
			}
			MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		}
		break;

	default:
		break;
	}
	return EFCBM_CONTINUE;
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Splittet Schein nach Paletten-Größe
-*
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAuftragPalSplit (MskDialog hMaskRl, MskStatic ef,
		MskElement hEfRl, int reason, void *cbc, void *calldata) {

	MY_CTX	*ptPosCtx = (MY_CTX *)MskDialogGet(hMaskRl, MskNmaskCalldata);

	switch (reason) {

	case FCB_XF:
		{
			if (ptPosCtx->tVplk.vplkStatus != VPLKSTATUS_WARTEN) {
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein splitten"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Splitten ist nicht möglich.\n"
							"Kommissionierschein %ld hat nicht den Status GESPERRT."), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				MskCbClose (hMaskRl, ef, hEfRl, reason, cbc);
				break;
			}
			if ( ptPosCtx->tVplk.vplkBearbPos > 0) {
				WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Kommissionierschein splitten"),
					WboxNtext,
						wamas::platform::string::form(MlMsg(
							"Splitten ist nicht möglich.\n"
							"Kommissionierschein %ld muss Bearbeitungsstand "
							"von 0 Prozent haben."), 
							ptPosCtx->tVplk.vplkKsNr).c_str(),
					NULL);
				MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
				break;
			}

			vector <VPLK> vKsNr;
			vKsNr.push_back(ptPosCtx->tVplk);
			_me_kssplitpaleingabe(GetRootShell(), &vKsNr);
		}

		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return EFCBM_CONTINUE;
}



/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  CbAlleAuftrageSperren,sperrt/entsperrt alle selektierte Kommissionsscheine 
-*		Hoststatus -> SENDEN
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbAlleAuftraegeSperren (MskDialog hMaskRl, MskStatic ef, 
								   MskElement hEfRl, int reason, void *cbc, void *calldata)
{
	STP_VPLUEB_CTX   	*ptMc = (STP_VPLUEB_CTX *)MskRlMaskGet(hMaskRl, 
		MskNmaskCalldata);
	int                 iMask = ptMc->iWhichMask;
	switch (reason) {
	case FCB_XF:
		if (iMask == MK_S) {
			if (TpaSperren (hMaskRl,  ptMc) < 0) {
				TSqlRollback(hMaskRl);
			} else {
				TSqlCommit(hMaskRl);
			};
		} else {
			if (VplSperren (hMaskRl,  ptMc) < 0) {
				TSqlRollback(hMaskRl);
			} else {
				TSqlCommit(hMaskRl);
			}
		}
		if (iMask == MK_S) {
			LB_DeSellectAll(ptMc->tStp.lb);
			PosLesenStp(ptMc->hFirstMask, OPT_ERRORBOX);
			MskUpdateMaskVar (hMaskRl);
		} else {
			LB_DeSellectAll(ptMc->tVplueb.hLb);
			if (ListUpdateVplueb(ptMc->hFirstMask,
				&(((STP_VPLUEB_CTX *)(ptMc->ptStVplueb))->tVplueb),
				MANUELL_LEVEL) < 0) {
				ptMc->tVplueb.Aktion=MANUELL_READ;
				TSqlRollback(ptMc->hFirstMask);
			}
			MskUpdateMaskVar (ptMc->hFirstMask);
		}
		MskCbClose(hMaskRl, ef, hEfRl, reason, cbc);
		break;
	default:
		break;
	}
	return (EFCBM_CONTINUE);
}  /*  CbAlleAuftraegeSperren()  */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
*  Maskencallback für die kleine Auswahlmaske 
-* RETURNS
-*--------------------------------------------------------------------------*/
static int Cb_MaskAlleAendern (MskDialog hMaskRl, int iReason)
{
	STP_VPLUEB_CTX  *ptPosCtx;
	STP_VPLUEB_CTX  *ptStVpl;
	int         	iRet = MSK_OK_TRANSFER, nEle = 0;
	switch(iReason) {
	case MSK_CM:
		ptPosCtx = (STP_VPLUEB_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		/* ---- VARIABLE BINDEN --  */
		MskMatchEvaluate(hMaskRl, "*", 0, MSK_MATCH_NODEF_ATTR);
		MskVaAssign(hMaskRl,        MskGetElement("InfoText1"),
			MskNvariable,   (Value)ptPosCtx->acInfo,
			NULL);
		/* ---- BUTTONS ----------- */

		EfcbmInstByName(hMaskRl, "Zuteilen",
				KEY_DEF, CbAlleZuteilen,     NULL);
		EfcbmInstByName(hMaskRl, "Priorisieren",
				KEY_DEF, CbAllePriorisieren, NULL);
		EfcbmInstByName(hMaskRl, "Sperren",
				KEY_DEF, CbAlleAuftraegeSperren,      NULL);
		EfcbmInstByName(hMaskRl, "Abschliessen",
				2, CbAlleAuftraegeAbschliessen, NULL);
		EfcbmInstByName(hMaskRl, "Aendern",
				KEY_DEF, CbAlleAendern, NULL);
		EfcbmInstByName(hMaskRl, "Button_Verschmelzen",
				KEY_DEF, CbVerschmelzen,    NULL);
		EfcbmInstByName(hMaskRl, "Storno",
				KEY_DEF, CbAlleAuftraegeStorno, NULL);
		EfcbmInstByName(hMaskRl, "AutoSplitten",
				KEY_DEF, CbAlleAuftraegePalSplit, NULL);
		EfcbmInstByName(hMaskRl, "AlmaRefresh",
				KEY_DEF, alma::CbAlleAuftraegeAlmaRefresh, NULL);

		/* ---- ACTIVATE / DEACTIVATE BUTTONS ---- */
		if (ptPosCtx->iWhichMask == MK_S) {
			MskVaAssignMatch(hMaskRl, "*-NOTSTP-*",
					MskNattrOr, (Value)EF_ATTR_INACTIVE,
					NULL);
		} else {
			MskVaAssignMatch(hMaskRl, "*-NOTSTP-*",
					MskNattrClr, (Value)EF_ATTR_INACTIVE,
					NULL);
		}

		break;
	case MSK_RA:
		ptPosCtx = (STP_VPLUEB_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		if (ptPosCtx->iWhichMask == MK_S) {
			nEle = LBNumberOfSelected(ptPosCtx->tStp.lb);
			ptPosCtx->tStp.iAnzahlSel = nEle;
		} else {
			nEle = LBNumberOfSelected(ptPosCtx->tVplueb.hLb);
			ptPosCtx->tVplueb.iAnzahlSel = nEle;
		}
		MskUpdateMaskVar (hMaskRl);
		break;
	case MSK_DM:
		ptPosCtx = (STP_VPLUEB_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		ptStVpl = (STP_VPLUEB_CTX *)ptPosCtx->ptStVplueb;
		TSqlRollback(hMaskRl);
		if (ptPosCtx->iWhichMask == MK_S) {
			ptStVpl->tStp.iIsAlleAendernMaskOpen = 0;
		} else {
			ptStVpl->tVplueb.iIsAlleAendernMaskOpen = 0;
			if (miAbschliessen == 1) {
				if (ListUpdateVplueb(ptStVpl->hFirstMask,
						&(((STP_VPLUEB_CTX *)
								(ptPosCtx->ptStVplueb))->tVplueb),
								MANUELL_LEVEL) < 0) {
					ptStVpl->tVplueb.Aktion=MANUELL_READ;
					TSqlRollback(ptStVpl->hFirstMask);
				}
				MskUpdateMaskVar (ptStVpl->hFirstMask);
			}
		}
		miAbschliessen = 0;
		if (ptPosCtx != NULL) {
			free (ptPosCtx);
			ptPosCtx = NULL;
		}
		break;
	default:
		break;
	}
	return iRet;
} /* Cb_MaskAlleAendern */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  _alle_bearbeiten,erzeugt die Maske für Funktion alle Ändern 
-* RETURNS
-*--------------------------------------------------------------------------*/
int _alle_bearbeiten ( OWidget w, STP_VPLUEB_CTX *ptStVpl, 
					  MskDialog hFirstMask, int iMask)
{
	MskDialog           hMaskRl = NULL;
	OWidget             hMsh = NULL;
	STP_VPLUEB_CTX      *ptPosCtx;
	/* RC-File holen */
	OwrcLoadObject(ApNconfigFile,"vpl.rc");
	OwrcLoadObject(ApNconfigFile,"me_bearb.rc");
	hMaskRl = MskOpenMask (NULL, "ALLE_BEARB");
	if (hMaskRl == NULL) {
		return (RETURN_ERROR);
	}
	if((hMsh = SHELL_OF(hMaskRl)) == NULL) {
		if((ptPosCtx = (STP_VPLUEB_CTX *)malloc(sizeof(STP_VPLUEB_CTX))) == NULL) {
			MskDestroy(hMaskRl);
			return(RETURN_ERROR);
		}
		memset(ptPosCtx, 0, sizeof(STP_VPLUEB_CTX));
		if (iMask == MK_S) {
			ptPosCtx->tStp.lb = ptStVpl->tStp.lb;
			ptPosCtx->iWhichMask = iMask;
		} else {
			ptPosCtx->tVplueb.hLb = ptStVpl->tVplueb.hLb;
			ptPosCtx->iWhichMask = iMask;
		}
		ptPosCtx->ptStVplueb = ptStVpl;
		MskRlMaskSet(hMaskRl, MskNmaskNoTopPlate,(Value)1);
		MskRlMaskSet (hMaskRl, MskNmaskCalldata,  (Value)ptPosCtx);
		MskRlMaskSet(hMaskRl, MskNmaskCallback,   (Value)Cb_MaskAlleAendern);
	} else {
		ptPosCtx = (STP_VPLUEB_CTX*)MskDialogGet(hMaskRl, MskNmaskCalldata);
	}
	ptPosCtx->iWhichSel = ptStVpl->iWhichSel;
	ptPosCtx->hFirstMask = hFirstMask;
	hMsh=ApShellModelessCreate(w, AP_CENTER, AP_CENTER);
	WamasWdgAssignMenu(hMsh,"ALLE_BEARB");
	MskCreateDialog (hMsh, hMaskRl, MlM("Alle bearbeiten"), 
		NULL, HSL_NI, SMB_All);

	sprintf (ptPosCtx->acInfo, "         %d %s %s ",
		(iMask == MK_S) ?
		ptPosCtx->tStp.iAnzahlSel :
	ptPosCtx->tVplueb.iAnzahlSel,
		(iMask == MK_S) ?  MlMsg("TaNr") : MlMsg("KsNr"),
		MlMsg("ändern"));


	MskUpdateMaskVar (hMaskRl);

	return RETURN_ACCEPTED;
} /*_alle_bearbeiten*/
