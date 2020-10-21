/**
 * @brief GD071 Transporteinheit bearbeiten
 * @file
 * @author Copyright (c) 2008 Salomon Automation GmbH
 */

#include <wamasbox.h>
#include <array.h>
#include <sstring.h>

#include <l_me_wartung.h>
#include <efcbm.h>
#include <t_util.h>
#include <disperr.h>
#include <sqlopm.h>
#include <hist_util.h>
#include <errmsg.h>

#include <TEK.h>
#include <TEP.h>
#include <TPA.h>
#include <KOST.h>
#include <BSCHL.h>
#include <TEKPRN.h>
#include <PRNTULABEL.h>
#include <WEVK.h>
#include <AUSK.h>
#include <INVBA.h>
#include <LST.h>
#include <V_GDART.h>
#include <V_GDTEP.h>
#include <V_GDTEK.h>
#include <INVK.h>
#include <te_TEK.h>
#include <te_TEP.h>
#include <te_TEKPRN.h>
#include <te_V_GDART.h>
#include <te_BSCHL.h>
#include <te_KOST.h>
#include <te_LST.h>
#include <IV_THM.h>
#include <IV_TTS.h>
#include <IV_P_WETEK.h>
#include <KST.h>

#include <if_logging_s.h>
#include <if_opmsg_s.h>
#include <if_frmwrk_g.h>
#include <if_frmwrk_s.h>
#include <if_fwrefnr_s.h>
#include <if_stockbase_s.h>
#include <if_stdemand_s.h>
#include <if_article_s.h>
#include <if_accode_s.h>
#include <if_givg_s.h>
#include <if_givg_g.h>
#include <if_slobase_s.h>
#include <if_tpobase_s.h>
#include <if_beleg_s.h>
#include <if_clients_s.h>
#include <if_param_s.h>
#include <if_trgutil_s.h>
#include <if_govplbase_s.h>
#include <if_opmsg_g.h>
#include <if_gihostmux_s.h>
#include <if_slosearch_s.h>
#include <if_gowobase_s.h>
#include <if_gofausbase_s.h>
#include <if_ifcnvybl_s.h>

#include "li_te.h"
#include "me_tebearb_delete.h"
#include <buildtu.h>
#include <stockbase_util.h>
#include <zap_util.h>
#include <stockbase_tuid_util.h>
#include <compress2te.h>
#include <seriennr_util.h>
#include <tucheck_util.h>
#include <tts_util.h>
#include "me_enterserials.h"
#include <wamas/wms/tektrigger.h>
#include <wamas/wms/teptrigger.h>
#include <wamas/wms/matq_util.h>
#include <ConvenientDateIf.h>

#define _ME_TEBEARB_C
#include "me_tebearb.h"
#undef _ME_TEBEARB_C

#define TEBEARB_TEPFOUND		0x00000001L
#define TEBEARB_MNGTOHIGH		0x00000002L
#define TEBEARB_NOTEMPTY		0x00000004L
#define TEBEARB_MISCHTE			0x00000008L
#define BLOCKSIZE				100

/* ATTENTION The Defines TEBEARB_Std,TEBEARB_We and TEBEARB_Wa must
always be the sameas in the RC-File */
#define TEBEARB_Std				0x00000001L
#define TEBEARB_We				0x00000002L
#define TEBEARB_WeNach			0x00000004L
#define TEBEARB_Wa				0x00000008L

// if it it is a supplier return gi-process
#define TEBEARB_WeRetoure		0x00000020L

#define ATOM_UniAprAction    	"UniAprAction"
#define ATOM_UniAprMyContext    "UniAprMyContext"
#define MAX_SCANDATA_TEBEARB	(100)

#define ACTION_NONE			0L
#define ACTION_INSERT		1L
#define ACTION_UPDATE		2L
#define ACTION_DELETE		3L
#define ACTION_OK			4L

/**
* the index of the listbuffers in the main unimenu
*/
#define MD_LBC_IDX_TEP      0

/**
* name of the TEP listbuffer
*/
#define MD_TEP_LBC_NAME     TN_TEP

typedef enum _eReasonTeWalk {
TEWALK_INIT,		/* any action, except ACTION_NONE  */

TEK_INSERT,			/* eather for each TEK */
TEK_UPDATE,
TEK_DELETE,

TE_COMPLETE, 		/* not with delete TE */

TEP_INSERT,			/* eather for each TEP */
TEP_UPDATE,
TEP_DELETE,
TEP_NONE,

TEWALK_FINISH		/* any action, except ACTION_NONE  */
} eReasonTeWalk;
typedef struct _TeBearbCalldata {
	TOriginalCaller eOrigCaller;
	KRETK	tKretk;
	long lNeutraleInvNr;
	long lNeutralePosNr;
} TeBearbCalldata;

typedef struct _TTEWALK {
long			lAction;
long			lFlags;
MyTcontextTek	*ptMcTek;
bool			isCbBefore;
} TTEWALK;

typedef struct _CDTEWALK {
TEK				*ptTekBefore,
				*ptTekNow;
TEP				*ptTepBefore,
				*ptTepNow;
StockbaseTChangeTepInfo	tTepInfo;

long			lAction;
long			lKorrTypRead;
long			lStapelHoehe;

/* returns */
long			lFlags;
char	const	*pkcErrTxt;

/* only for CbWa_TeWalk */
AUSID			tAusId;

/* only for CbWe_TeWalk1 */
GivgTWevBearb	*ptWevBearb;

/* only for CbCeckAndInit_TeWalk */
long			lKorrTyp;
int				iTpa;		
TPA				tTpa;		

/* only for CbCeckBefore_TeWalk */
TEP				*ptTep;

/* For Prot Zap */
KRETK				tKretk;
OpmsgTCbStackPtr	stackPtr;

/* for FachK check */
FTT 			tFtt;
int 			iForceStdTepUpd;
MskDialog 		hMaskRl;

/* for updating wevp information */
ArrayPtr			hArrTepInfo;
} CDTEWALK;

/* ------- Functions ---------------------------------------------------- */

typedef int (*f_CbTeWalk_tebearb)(const void*,			/* tid */
							  char const *,		/* log facility */
							  eReasonTeWalk,	/* reason */
							  CDTEWALK *);		/* all other info */

/*static int CbCheckBt_TeBearb();
static int CbInitKorrTyp_TeBearb();*/
static int TeWalk_tebearb (const void *, UNITABLERL *, char const **, TTEWALK *,
					   f_CbTeWalk_tebearb, OpmsgTCbStackPtr stackPtr);

static int CbStd_TeWalk (const void *, char const *, eReasonTeWalk, CDTEWALK *);
static int CbWa_TeWalk (const void *, char const *, eReasonTeWalk, CDTEWALK *);
static int CbCeckBefore_TeWalk (const void *, char const *,
										eReasonTeWalk, CDTEWALK *);

static f_UESetEfByArtMeTeBearb mxUESetEfByArtMeTeBearb;

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*-------------------------------------------------------------------------*/
static int SortInfo (const void *ptData1, const void *ptData2)
{
return (((StockbaseTChangeTepInfo *)ptData1)->lPosNr - ((StockbaseTChangeTepInfo *)ptData2)->lPosNr);
}
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbArtChange_TeBearb(MskDialog hMaskRl, MskStatic hEf,
						   MskElement hEfRl,
						   int iReason, void *pvCbc,void *xx)

{
	char const *pcFac;
	int					iRv, nNoEle, nI;
	UNITABLERL  		*utr;
	UNICONNECTRL        *ucr;
	LBCBuffer           *lbc;
	LBCElement          *lel;

	ContainerAtom   	hAtom;
	MyTcontextTep   	*ptMcTep;

	TEP                	*ptTepNow,
						*ptTepBefore;
	TUESetEfByArt		tTUESetEfByArtMeTeBearb;

	switch (iReason) {
	case FCB_CH:
		pcFac = lUniGetFacility (hMaskRl);

		/* unimenue */
		utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);

		UniTransferMaskDup(hMaskRl,utr);

		ptTepNow = (TEP *)UniTableGetRecNow(utr);
		if (ptTepNow == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "UniTableGetRec... failed");
			return (-1);
		}

		/* ptTepBefore */
		ucr = UniTableGetParentConnectRl(utr);
		lbc = UniConnectGetLBCByIdx(ucr, 0);
		nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
		
		for (ptTepBefore = NULL, nI = 0;
			 nI < nNoEle;
			 ptTepBefore = NULL, nI++) {
			
			lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
			
			ptTepBefore = (TEP *)UniConnectLBCElemGetRecBeforeByIdx
					  (ucr, 0, lel);
			if (ptTepBefore &&
				ptTepBefore->tepPosNr == ptTepNow->tepPosNr) {
				break;
			}
		}
		
		/* context */
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "MskAtomGet failed: "ATOM_UniAprMyContext);
			return (-1);
		}
		ptMcTep = (MyTcontextTep*)hAtom->client_data;

		if (memcmp (&ptTepNow->tepMId.AId, 
					&ptMcTep->tGdArt.gdartAId, sizeof (AID)) == 0 &&
			memcmp (&ptTepNow->tepMId.AId, 
					&ptMcTep->tGdArte.gdarteAId, sizeof (AID)) == 0) {
			break;
		}
	
		//clear old data
		memset (&ptMcTep->tGdArt, 0, sizeof (V_GDART));
		memset (&ptMcTep->tGdArte, 0, sizeof (V_GDARTE));
		memset (&ptTepNow->tepMngs, 0, sizeof(MNGS));
		memset (ptTepNow->tepMId.SerienNrGrp, 0, sizeof(ptTepNow->tepMId.SerienNrGrp));
		ptMcTep->dAnzMng = 0.0;
		ptMcTep->dRestMng = 0.0;

		if (IsEmptyString (ptTepNow->tepMId.AId.Mand) || 
			IsEmptyString (ptTepNow->tepMId.AId.ArtNr) ||
			IsEmptyString (ptTepNow->tepMId.AId.Var) || 
			ptTepNow->tepMId.AId.AusPr == 0) {
		
			MskVaAssignMatch (hMaskRl,  "*-Einheit-*",
							  MskNupdate,             (Value)1,
							  MskNtransferVar2Dup,    (Value)1,
							  NULL);
			MskVaAssignMatch (hMaskRl,  "*-Calc-*",
							  MskNupdate,             (Value)1,
							  MskNtransferVar2Dup,    (Value)1,
							  NULL);
			
			/* USER EXIT */
			if (mxUESetEfByArtMeTeBearb != NULL) {
				
				memset (&tTUESetEfByArtMeTeBearb, 0,
											sizeof(tTUESetEfByArtMeTeBearb));
				tTUESetEfByArtMeTeBearb.hMaskRl = hMaskRl;
				
				mxUESetEfByArtMeTeBearb(&tTUESetEfByArtMeTeBearb);
			}
				
			MskVaAssign (hMaskRl,  MskGetElement(TEP_Mngs_Gew_t),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrClr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,		(Value) 1,
						 NULL);
			return (-1);
		}
		
		OpmsgSIf_ErrResetMsg();
		
		iRv = ArticleSIf_GetArtArtEByAId_GD (hMaskRl, pcFac, 
					&ptTepNow->tepMId.AId, &ptMcTep->tGdArt, &ptMcTep->tGdArte);
		
		(void )TSqlRollback (hMaskRl);
		
		if (iRv < 0) {
			OPMSG_SFF(ArticleSIf_GetArtArtEByAId_GD);
			
			memset (&ptMcTep->tGdArt, 0, sizeof (V_GDART));
			memset (&ptMcTep->tGdArte, 0, sizeof (V_GDARTE));
			MskVaAssignMatch (hMaskRl,  "*-Einheit-*",
							  MskNupdate,             (Value)1,
							  MskNtransferVar2Dup,    (Value)1,
							  NULL);
			MskVaAssignMatch (hMaskRl,  "*-Calc-*",
							  MskNupdate,             (Value)1,
							  MskNtransferVar2Dup,    (Value)1,
							  NULL);
			
			/* USER EXIT */
			if (mxUESetEfByArtMeTeBearb != NULL) {
				
				memset (&tTUESetEfByArtMeTeBearb, 0,
											sizeof(tTUESetEfByArtMeTeBearb));
				tTUESetEfByArtMeTeBearb.hMaskRl = hMaskRl;
				
				mxUESetEfByArtMeTeBearb (&tTUESetEfByArtMeTeBearb);
			}
			
			MskVaAssign (hMaskRl,  MskGetElement(TEP_Mngs_Gew_t),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrOr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,		(Value) 1,
						 NULL);
			
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE Korrektur"), NULL);
			return (-1);
		}
		
		if (ptTepBefore &&
			memcmp (&ptTepBefore->tepMId.AId,
					&ptMcTep->tGdArt.gdartAId, sizeof (AID)) == 0 &&
			memcmp (&ptTepBefore->tepMId.AId,
					&ptMcTep->tGdArte.gdarteAId, sizeof (AID)) == 0) {
			
			ptTepNow->tepMngs.GewBasis = ptTepBefore->tepMngs.GewBasis;
		} else {
			ptTepNow->tepMngs.GewBasis = ptMcTep->tGdArt.gdartNettoGew;
		}
		
		MskVaAssignMatch (hMaskRl,  "*-Einheit-*",
						  MskNupdate,             (Value)1,
						  MskNtransferVar2Dup,    (Value)1,
						  NULL);
		MskVaAssignMatch (hMaskRl,  "*-Calc-*",
						  MskNupdate,             (Value)1,
						  MskNtransferVar2Dup,    (Value)1,
						  NULL);
		
		/* USER EXIT */
		if (mxUESetEfByArtMeTeBearb != NULL) {
				
			memset (&tTUESetEfByArtMeTeBearb, 0,
					sizeof(tTUESetEfByArtMeTeBearb));
			tTUESetEfByArtMeTeBearb.hMaskRl = hMaskRl;
			tTUESetEfByArtMeTeBearb.tGdArt = ptMcTep->tGdArt;
			
			mxUESetEfByArtMeTeBearb (&tTUESetEfByArtMeTeBearb);
		}
		
		iRv = ArticleSIf_IsWeightToEdit_GD (hMaskRl, pcFac, &ptMcTep->tGdArt);
		
		(void )TSqlRollback (hMaskRl);
		
		if (iRv < 0) {
			OPMSG_SFF(ArticleSIf_IsWeightToEdit_GD);
			
			MskVaAssign (hMaskRl,  MskGetElement(TEP_Mngs_Gew_t),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrOr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,		(Value) 1,
						 NULL);
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE Korrektur"), NULL);
			
			return (-1);
		}
		
		if (iRv == 1) {
			MskVaAssign (hMaskRl,  MskGetElement(TEP_Mngs_Gew_t),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrClr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,		(Value) 1,
						 NULL);
		} else {
			MskVaAssign (hMaskRl,  MskGetElement(TEP_Mngs_Gew_t),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrOr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,		(Value) 1,
						 NULL);
		}
		
		break;
	default:
		break;
	}
	return (1);
}
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		in case that MHD have been changed, copy MHD to Fifo
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbMhdChange_TeBearb(MskDialog hMaskRl, MskStatic hEf,
						   MskElement hEfRl,
						   int iReason, void *pvCbc,void *xx)

{
	UNITABLERL  		*utr = NULL;
	MskTcbcLf           *hCbcLf = NULL;
	TEP                	*ptRecTep = NULL;
	ContainerAtom   	hAtom = NULL;
	MyTcontextTep   	*ptMcTep = NULL;;
	char const *pcFac = NULL;
	int iRv = 0;
	struct tm tmMHD = {};;

	switch (iReason) {
	case FCB_LF:
		/* Log facility */
		pcFac = lUniGetFacility (hMaskRl);

		/* Unitable-Handle  */
		utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);

		UniTransferMaskDup(hMaskRl,utr);

		/* get actual data from edit field */	
		hCbcLf = (MskTcbcLf *)pvCbc;
		if (hCbcLf == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "hCbcLf is NULL !");
			return (-1);
		}

		ptRecTep = (TEP *)UniTableGetRecNow(utr);
		if (ptRecTep == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "UniTableGetRec... failed");
			return -1;
		}

		/* context */
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "MskAtomGet failed: "ATOM_UniAprMyContext);
			return (-1);
		}
		ptMcTep = (MyTcontextTep*)hAtom->client_data;

		/* check set FIFO */
		tmMHD = *(struct tm *)hCbcLf->var.p_tm;
		iRv = ArticleSIf_CheckFifoDatum_GD (hMaskRl, pcFac, &ptMcTep->tGdArt,
				&ptRecTep->tepFifoDatum, &tmMHD, JANEIN_N /* check RHD */);
		if (iRv < 0) {
			OPMSG_SFF(ArticleSIf_CheckFifoDatum_GD);
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("Änderung des MHD"), NULL);
			hCbcLf->result = 0;
			return iRv;
		}

		MskVaAssign (hMaskRl,  MskGetElement(TEP_FifoDatum_t),
			MskNkey,               (Value) KEY_DEF,
			MskNupdate,             (Value)1,
			MskNtransferVar2Dup,    (Value)1,
			NULL);

	default:
		break;
	}

	return (1);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*    $$
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int CbScan_TeBearb (MskDialog hMaskRl, unsigned char *pcData)
{
	SCANDATA		tScanData;
	int				iRv;
	UNITABLERL  	*utr;
	TEK				*ptTek;
	UNITABLE		*ut;
	char const *pcFac;
	char			acCmpData[MAX_SCANDATA_TEBEARB+1];

	if (pcData == NULL) {
		return (RETURN_ACCEPTED);
	}

	utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);
	ut = UniTableGetTable (utr);

	if (strcmp (ut->utName, TN_TEK)) {
		return (RETURN_ACCEPTED);
	}
			

	pcFac = lUniGetFacility (hMaskRl);

	memset (acCmpData, 0, MAX_SCANDATA_TEBEARB+1);
	strncpy(acCmpData, (char *)pcData, MAX_SCANDATA_TEBEARB);

	memset (&tScanData, 0, sizeof (tScanData));

	iRv = ArticleSIf_ConvScanData (hMaskRl, pcFac,
								   acCmpData, &tScanData);

	(void )TSqlRollback (hMaskRl);

	if (iRv < 0) {
		OPMSG_SFF(ArticleSIf_ConvScanData);
		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE Korrektur"), NULL);
		
		return (RETURN_ACCEPTED);
	}

	ptTek = (TEK *)UniTableGetRecNow(utr);

	if (IsEmptyStrg (tScanData.acTeId) == 0) {
		FRMWRK_STRARRNCPY(ptTek->tekTeId, tScanData.acTeId);
		MskVaAssign (hMaskRl,     MskGetElement (TEK_TeId_t),
					 MskNkey,            	(Value) KEY_DEF,
					 MskNtransferVar2Dup,	(Value)TRUE,
					 MskNupdate,            (Value)TRUE,
					 NULL);

		iRv = TExecStdSql (hMaskRl, StdNselect, TN_TEK, ptTek);
		if (iRv > 0) {
			UniRead(hMaskRl, utr, 1);
		} else {
			WamasBox (SHELL_OF(hMaskRl),
			WboxNboxType,   WBOX_INFO,
			WboxNbutton,    WboxbNok,
			WboxNtext,      (char const*)scoped_cstr::form (MlMsg(
				"Transporteinheit '%s' nicht vorhanden."), ptTek->tekTeId),
			NULL);
		}
		UniUpdateMaskVarR(hMaskRl, utr);

	} else if (IsEmptyStrg(acCmpData) == 0) {
		SlobaseSIf_Str2Pos (hMaskRl, acCmpData, &ptTek->tekPos);
		TSqlRollback (hMaskRl);

		MskVaAssign (hMaskRl,     MskGetElement (TEK_Pos_t),
					 MskNkey,            	(Value) KEY_DEF,
					 MskNtransferVar2Dup,	(Value)TRUE,
					 MskNupdate,            (Value)TRUE,
					 NULL);
	}

	return (EFCBM_CONTINUE);
}
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		resets the TE-Korrektur to Standard (inactive)
-*		Cb for changing TE-Id
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbInitKorrTyp_TeBearb (MskDialog hMaskRl, MskStatic hEf,
						MskElement hEfRl, int iReason, void *cbc,
						void *pvCd)

{
	char const *pcFac;
	ContainerAtom   	hAtom;
	UNITABLERL			*utr;
	UNITABLE			*ut;
	TEK					*ptTekNow,
						*ptTekBefore;
	MyTcontextTek   	*ptMcTek;

	switch (iReason) {
	case FCB_CH:
		pcFac = lUniGetFacility (hMaskRl);
		
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "%s",
						  "MskAtomGet failed: "ATOM_UniAprMyContext);
			break;
		}
		ptMcTek = (MyTcontextTek*) hAtom->client_data;
		
		utr = (UNITABLERL *) MskDialogGet (hMaskRl, MskNmaskCalldata);
		ut = UniTableGetTable (utr);
		
		ptTekBefore = (TEK *)UniTableGetRecBefore (utr);
		ptTekNow = (TEK *)UniTableGetRecNow (utr);
		
		if (ptTekBefore == NULL || ptTekNow == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						  "UniTableGetRec... failed");
			break;
		}
		
		UniTransferMaskDup (hMaskRl, utr);
		if (strcmp(ptTekBefore->tekTeId, ptTekNow->tekTeId)) {
			memset (&ptMcTek->tAusk, 0, sizeof(AUSK));
			memset (&ptMcTek->tWevk, 0, sizeof(WEVK));
			memset (&ptMcTek->tWevBearb, 0, sizeof(ptMcTek->tWevBearb));
			ptMcTek->lKorrTypRead = TEBEARB_Std;
			ptMcTek->lKorrTyp = TEBEARB_Std;
			ptMcTek->iModTep = 1;	/* no real effect */
			if (ut && ut->utCalldata) {
				((TeBearbCalldata *)ut->utCalldata)->eOrigCaller =
														OriginalCaller_Standard;
			}
		}
		
		/* causes UNITABLEREASON_UPDATE_MASK_VAR in MaskCallbacks */
		UniUpdateMaskVarR (hMaskRl, utr);
		
		break;
	default:
		break;
	}
	return 1;
}	

/**
* will return the MOT and the article (if exists) for the given TU type
*
* - a TU type must always have an assigned MOT
* - but an MOT does not need to have an article
*
* you could call this function with both \a pktIvThm and \a pktIvArt NULL,
* just to check if an MOT exists for the TU type
*
* @note if \a pktIvThm or \a pktIvArt are NULL, it does not affect the
*       returnvalue
* @note the function will even return the article, if it is not stock relevant
*
* @param[in]   pvTid       Sql transaction id
* @param[in]   pcFac       Logging facility
* @param[in]   pkcTuTypeId ID of the TU-type
* @param[out]  pktIvThm    [optional] the corresponding MOT
* @param[out]  pktIvArt    [optional] the corresponding article (if any)
*
* @retval   2 MOT and article found
* @retval   1 MOT found (no article found)
* @retval  <0 error
*/
static int getMotAndArticleForTuType(const void* pvTid, char const *pcFac,
			char const *pkcTuTypeId, IV_THM *pktIvThm, IV_ART *pktIvArt)
{
	int iRv;
	int iRvFct;
	IV_ART tIvArt;
	IV_THM tIvThm;

	if (IsEmptyString(pkcTuTypeId)) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "Invalid arguments");
		return OpmsgSIf_ErrPush((int)GeneralTerrArgs, NULL);
	}

	memset(&tIvArt, 0, sizeof(tIvArt));
	memset(&tIvThm, 0, sizeof(tIvThm));
	iRv = TExecSql(pvTid,
			   " SELECT %"TN_IV_THM", %"TN_IV_ART
			   " FROM "TN_IV_ART
				   ", "TN_IV_TTS
				   ", "TN_IV_THM
			   " WHERE " TCN_IV_THM_ThmId " = "TCN_IV_TTS_THM_ThmId
			   " AND " TCN_IV_ART_AId_ArtNr " (+) = " TCN_IV_THM_ART_AId_ArtNr
			   " AND " TCN_IV_ART_AId_Mand  " (+) = " TCN_IV_THM_ART_AId_Mand
			   " AND " TCN_IV_ART_AId_Var   " (+) = " TCN_IV_THM_ART_AId_Var
			   " AND " TCN_IV_ART_AId_AusPr " (+) = " TCN_IV_THM_ART_AId_AusPr
			   " AND " TCN_IV_TTS_TetId     " = :TetId "
			   , SELSTRUCT(TN_IV_THM, tIvThm)
			   , SELSTRUCT(TN_IV_ART, tIvArt)
			   , SQLSTRING(pkcTuTypeId)
			   , NULL);
	if (iRv <= 0) {
		if (TSqlError(pvTid) == SqlNotFound) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				"no MOT (and article) found for TU-type: %s", pkcTuTypeId);
			return OpmsgSIf_ErrPush((int)GeneralTerrNoDataFound,
							MLM("Für den TE-Typ '%s' wurde kein "
								"Transporthilfsmittel gefunden."),
									pkcTuTypeId);
		}
		LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid,
							   "failed to read MOT and article for tu-type");
		return OpmsgSIf_ErrPush((int)GeneralTerrDb, "%s", TSqlPrettyErrTxt(pvTid));
	}

	iRvFct = 1;
	if (!IsEmptyString(tIvArt.ivartAId.ArtNr)) {
		iRvFct = 2;
	}

	if (pktIvThm != NULL) {
		*pktIvThm = tIvThm;
	}

	if (pktIvArt != NULL) {
		*pktIvArt = tIvArt;
	}

	return iRvFct;
}

/**
* returns the listbuffer element and the TEP of the primary MOT of the TU
*
* @param[in]   pcFac       Logging facility
* @param       ucr         uniconnect RL
* @param[out]  phLelPrim   pinter to the listbuffer element of the primary MOT
* @param[out]  ptTepPrim   pointer to the TEP of the primary MOT
*
* @retval   1 primary MOT found
* @retval   0 not primary MOT found
* @retval  <0 error
*/
static int getPrimaryMOT(char const *pcFac, UNICONNECTRL *ucr,
		   LBCElement **phLelPrim, TEP **pptTepPrimMot)
{
	long nNoEle;
	long nI;
	LBCBuffer   *lbc;
	LBCElement  *lel;
	TEP         *ptTep;

	*phLelPrim = NULL;
	*pptTepPrimMot = NULL;
	lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);
	nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
	for (nI = 0, ptTep = NULL;
		 nI < nNoEle;
		 nI++, ptTep = NULL) {
		lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
		ptTep = (TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
		if (ptTep->tepPrimThmKz == JANEIN_J) {
			*phLelPrim = lel;
			*pptTepPrimMot = ptTep;
			return 1;
		}
	}

	return 0;
}

/**
* returns the listbuffer element which references to the given TEP
*
* @param[in]   pcFac       Logging facility
* @param       ucr         uniconnect RL
* @param[out]  phLelPrim   pointer to the listbuffer element of the TEP
* @param[in]   ptTepPrim   pointer to the TEP we are looking for
*
* @retval   1 Element found
* @retval   0 No Element found
* @retval  <0 error
*/
static int getListbufferEle4Tep(char const *pcFac, UNICONNECTRL *ucr,
		   LBCElement **phLel, TEP *ptSearchTep)
{
	long nNoEle;
	long nI;
	LBCBuffer   *lbc;
	LBCElement  *lel;
	TEP         *ptTep;

	*phLel = NULL;
	lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);
	nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
	for (nI = 0, ptTep = NULL;
		 nI < nNoEle;
		 nI++, ptTep = NULL) {
		lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
		ptTep = (TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
		if (ptTep->tepPosNr == ptSearchTep->tepPosNr) {
			*phLel = lel;
			return 1;
		}
	}

	return 0;
}

/**
* will prepare a new TEP to be inserted
*
* will set the default MatQ and the history
*
* @param[in]   pvTid       Sql transaction id
* @param[in]   pcFac       Logging facility
* @param[out]  ptTep       TEP to be inserted
*
* @retval >=0 success
* @retval  <0 error
*/
static int prepareTepToInsert(const void* pvTid, char const *pcFac, TEP *ptTep)
{
	int iRv;
	char    acDefMatQ[MATQKID_LEN + 1];

	memset(&acDefMatQ, 0, sizeof(acDefMatQ));
	iRv = ParamSIf_Get1Parameter(pvTid, pcFac, PrmDefMatq,
								  acDefMatQ);
	OPMSG_CFR(ParamSIf_Get1Parameter);

	FRMWRK_STRARRNCPY(ptTep->tepMId.MatQ, acDefMatQ);

	iRv = FrmwrkSIf_SetHist(pcFac, FrmwrkTSetHistTyp_Insert, TN_TEP, ptTep);
	OPMSG_CFR(FrmwrkSIf_SetHist);

	return 1;
}

/**
* will prepare the TEP \a ptNewPrimMot to be inserted as new primary MOT
*
* will use default MatQ
*
* @param[in]   pvTid       Sql transaction id
* @param[in]   pcFac       Logging facility
* @param[in]	ptUtr		Unitable
* @param[in]   pkcTuId     Transport Unit ID
* @param[in]   pktIvArt    Article data
* @param[out]  ptNewPrimMot    this TEP will be initialized
*
* @retval >=0 success
* @retval  <0 error
*/
static int initNewPrimMOT(const void* pvTid, char const *pcFac, UNITABLERL *ptUtr,
	char const *pkcTuId, IV_ART const *pktIvArt, TEP *ptNewPrimMot)
{
	int iRv;
	UNICONNECTRL	*ucr;
	LBCBuffer		*lbc;

	ucr = UniTableGetConnectRlArray(ptUtr);
	lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);

	FRMWRK_STRARRNCPY(ptNewPrimMot->tepTeId, pkcTuId);

	iRv = Frmwrk_UnimenuGetNextPosNr(pcFac, ucr, lbc,
									 offsetof(TEP, tepPosNr),
									 &ptNewPrimMot->tepPosNr, 0);
	OPMSG_CFR(Frmwrk_UnimenuGetNextPosNr);

	iRv = prepareTepToInsert(pvTid, pcFac, ptNewPrimMot);
	OPMSG_CFR(prepareTepToInsert);

	ptNewPrimMot->tepMId.AId = pktIvArt->ivartAId;
	ptNewPrimMot->tepMId.ThmKz = JANEIN_J;
	ptNewPrimMot->tepPrimThmKz = JANEIN_J;

	// amount
	ptNewPrimMot->tepMngs.Mng = 1;
	ptNewPrimMot->tepMngs.GewBasis = pktIvArt->ivartNettoGew;
	ptNewPrimMot->tepMngs.Gew = ptNewPrimMot->tepMngs.Mng *
									ptNewPrimMot->tepMngs.GewBasis;

	return 1;
}

/**
* will insert/update the entry of TEP \a lTepPosNr in \a ptMcTek::hArrTepInfo
* with the current values in \a ptMcTek
*
* @param[in]   pcFac       Logging facility
* @param[in]   lTepPosNr   position number of the TEP
* @param       ptMcTek     [inout]will be read and updated
*
* @retval >=0 success
* @retval  <0 error
*/
static int setTepInfo(char const *pcFac, long lTepPosNr, MyTcontextTek *ptMcTek)
{
	int     iRv = 0;
	StockbaseTChangeTepInfo    tTepInfo, *ptTepInfo;

	memset(&tTepInfo, 0, sizeof(tTepInfo));

	tTepInfo.lPosNr = lTepPosNr;
	ptTepInfo = (StockbaseTChangeTepInfo *)ArrGetFindElem(ptMcTek->hArrTepInfo, &tTepInfo);
	if (ptTepInfo != NULL) {
		tTepInfo = *ptTepInfo;
	}
	FRMWRK_STRARRNCPY (tTepInfo.acBschl, ptMcTek->acBschl);
	FRMWRK_STRARRNCPY (tTepInfo.acKostSt, ptMcTek->acKostSt);
	FRMWRK_STRARRNCPY (tTepInfo.acMand, ptMcTek->acMand);
	FRMWRK_STRARRNCPY (tTepInfo.acLiefNr, ptMcTek->acLiefNr);
	FRMWRK_STRARRNCPY (tTepInfo.acNeutralId, ptMcTek->acNeutralId);
	tTepInfo.lNeutralInvNr = ptMcTek->lNeutralInvNr;
	tTepInfo.lNeutralPosNr = ptMcTek->lNeutralPosNr;
	tTepInfo.iBschlKost = 1;

	if (ptTepInfo != NULL) {
		*ptTepInfo = tTepInfo;
	} else {
		iRv = ArrAddElem(ptMcTek->hArrTepInfo, &tTepInfo);
		if (iRv != 1) {
			LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT, "ArrAddElem",
									  "iRv=%d", iRv);
			return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
		}
	}

	ptMcTek->iModTep = 1;

	return 1;
}

/**
* will insert/update the entry of TEP \a lTepPosNr in \a ptMcTek::hArrTepInfo
* with the current values in \a ptMcTep
*
* @param[in]   pcFac       Logging facility
* @param[in]   lTepPosNr   position number of the TEP
* @param       ptMcTek     [inout]will be read and updated
* @param[in]   ptMcTep     TEP information
*
* @retval >=0 success
* @retval  <0 error
*/
static int setTepInfo(char const *pcFac, long lTepPosNr, MyTcontextTek *ptMcTek, MyTcontextTep *ptMcTep)
{
	int     iRv = 0;
	StockbaseTChangeTepInfo    tTepInfo, *ptTepInfo;

	memset(&tTepInfo, 0, sizeof(tTepInfo));

	tTepInfo.lPosNr = lTepPosNr;
	ptTepInfo = (StockbaseTChangeTepInfo *)ArrGetFindElem(ptMcTek->hArrTepInfo, &tTepInfo);
	if (ptTepInfo != NULL) {
		tTepInfo = *ptTepInfo;
	}
	FRMWRK_STRARRNCPY (tTepInfo.acBschl, ptMcTep->acBschl);
	FRMWRK_STRARRNCPY (tTepInfo.acKostSt, ptMcTep->acKostSt);
	FRMWRK_STRARRNCPY (tTepInfo.acMand, ptMcTep->acMand);
	FRMWRK_STRARRNCPY (tTepInfo.acLiefNr, ptMcTep->acLiefNr);
	FRMWRK_STRARRNCPY (tTepInfo.acNeutralId, ptMcTep->acNeutralId);
	tTepInfo.lNeutralInvNr = ptMcTep->lNeutralInvNr;
	tTepInfo.lNeutralPosNr = ptMcTep->lNeutralPosNr;
	tTepInfo.iBschlKost = 1;

	if (ptTepInfo != NULL) {
		*ptTepInfo = tTepInfo;
	} else {
		iRv = ArrAddElem(ptMcTek->hArrTepInfo, &tTepInfo);
		if (iRv != 1) {
			LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT, "ArrAddElem",
									  "iRv=%d", iRv);
			return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
		}
	}

	ptMcTek->iModTep = 1;

	return 1;
}

/**
* will set the color for TEPs where the MId.ThmKz is set
*
* @param[in]   hMaskRl     Mask handle (also used as SQL-transaction ID)
* @param[in]   pcFac       Logging facility
*
* @retval >=0 success
* @retval  <0 error
*/
static int handleMotDisplay(MskDialog hMaskRl, char const *pcFac)
{
	int nNoEle;
	int nI;
	int iDeleted;
	TEP *ptTep;
	UNITABLERL  	*utr;
	UNICONNECTRL    *ucr;
	LBCBuffer       *lbc;
	LBCElement      *lel;
	ContainerAtom    hAtom;

	utr = (UNITABLERL *)MskDialogGet(hMaskRl, MskNmaskCalldata);
	ucr = UniTableGetConnectRlArray(utr);

	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
	if (hAtom == NULL ||
	hAtom->client_data == VNULL) {
	LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
						 "INTERNAL ERROR: failed to get my context");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }

    lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);

    for (ptTep = NULL, nI = 0;
         nI < nNoEle;
         ptTep = NULL, nI++) {

        lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
        iDeleted = (int )UniConnectLBCElemGetDeleted(ucr, lel);
        if (iDeleted != 0) {
            continue;
        }

        ptTep = (TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
        if (ptTep->tepMId.ThmKz == JANEIN_J) {
            LBCVaSet(lbc,
                     LBCBackColorElement(UniConnectGetLBCConnect(ucr),
                                         LbcSectBody, lel,
                                         STOCKBASE_LB_MOT_BG_COLOR),
                     NULL);
        }
    }
    LBCUpdate(lbc);

    return 1;
}

/**
 * to be called when the user changes the TU type of the TU
 *
 * @note the function may rollback the transaction and open a dialog
 *
 * @param[in]   hMaskRl     Mask handle (also used as SQL-transaction ID)
 * @param[in]   pcFac       Logging facility
 *
 * @retval >=0 success
 * @retval  <0 error
 */
static int changeTuTypeId(MskDialog hMaskRl, char const *pcFac,
                          char const *pkcTuTypeId)
{
    int iRv;
    int iPrimMotExists;
    int iNewMotHasArticle;
    ContainerAtom   	hAtom;
	UNITABLERL			*utr;
	TEK					*ptTekNow;
    UNICONNECTRL        *ucr;
    LBCBuffer           *lbc;
    LBCElement          *lelPrim;
    TEP                 *ptTepPrimMot;
    MyTcontextTek   	*ptMcTek;
    IV_ART              tIvArt;
    TEP                 tNewPrimMot;

    hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
    if (hAtom == NULL ||
        hAtom->client_data == (Value)NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "INTERNAL ERROR: failed to get client data");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }

    ptMcTek = (MyTcontextTek*) hAtom->client_data;
    utr = (UNITABLERL *)MskDialogGet(hMaskRl, MskNmaskCalldata);
    UniTransferMaskDup(hMaskRl, utr);
	ucr = UniTableGetConnectRlArray(utr);
    lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);

    ptTekNow = (TEK *)UniTableGetRecNow (utr);
    if (ptTekNow == NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "INTERNAL ERROR: failed to get TEK data");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }

    if (strcmp(ptTekNow->tekTTS_TetId, pkcTuTypeId) == 0) {
        // no change
        return 0;
    }

    ptTepPrimMot = NULL;
    lelPrim = NULL;
    iPrimMotExists = 0;
    iRv = getPrimaryMOT(pcFac, ucr, &lelPrim, &ptTepPrimMot);
    OPMSG_CFR(getPrimaryMOT);
    if (iRv == 1) {
        iPrimMotExists = 1;
    }

    if (IsEmptyString(pkcTuTypeId)) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "TU-Type must not be empty");
        return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                MLM("TE-Typ muss eingegeben werden"));
    }

    memset(&tIvArt, 0, sizeof(tIvArt));
    iRv = getMotAndArticleForTuType(hMaskRl, pcFac, pkcTuTypeId,
                                    NULL, &tIvArt);
    OPMSG_CFR(getMotAndArticleForTuType);
    iNewMotHasArticle = 0;
    if (iRv == 2) {
        if (tIvArt.ivartBestKz == JANEIN_J) {
            iNewMotHasArticle = 1;
        } else {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
                         "article %s of the new TU-type is not stock relevant",
                         FwrefnrSIf_GetRefNr(TN_IV_ART, &tIvArt));
        }
    } else {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
                     "new TU-type '%s' has no article", pkcTuTypeId);
    }

    if (iNewMotHasArticle == 0 &&
        iPrimMotExists == 0) {
        /* there was no old primary MOT position and the new MOT has
         * no article (or it is not stock relevant) - so there is nothing to do
         */
        return 0;
    }

    if (iPrimMotExists  == 1) {
        if (iNewMotHasArticle == 1) {
            if (memcmp(&ptTepPrimMot->tepMId.AId, &tIvArt.ivartAId,
                       sizeof(ptTepPrimMot->tepMId.AId)) == 0) {
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
                                     "new article matches old article");
                return 0;
            } else {
                // change the AId of the existing TEP
                iRv = setTepInfo(pcFac, ptTepPrimMot->tepPosNr, ptMcTek);
                OPMSG_CFR(setTepInfo);

                ptTepPrimMot->tepMId.AId = tIvArt.ivartAId;

				//correct quantity and weight for new THM (article)
				ptTepPrimMot->tepMngs.Mng = 1.0;
				ptTepPrimMot->tepMngs.GewBasis = tIvArt.ivartNettoGew;
				ptTepPrimMot->tepMngs.Gew = tIvArt.ivartNettoGew;

				iRv = UniConnectLBCElemSetEntered(ucr, lelPrim , 1L);
            	OPMSG_CFR(UniConnectLBCElemSetEntered);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
                                     "changed aid of existing TU");
                return 1;
            }
        } else {
            // primary MOT existed, but new TU-type does not have an article
            // --> delete the old TEP
            iRv = setTepInfo(pcFac, ptTepPrimMot->tepPosNr, ptMcTek);
            OPMSG_CFR(setTepInfo);

            iRv = Frmwrk_UnimenuDeleteElement(pcFac, ucr, lbc, MD_LBC_IDX_TEP,
                                              lelPrim);
            OPMSG_CFR(Frmwrk_UnimenuDeleteElement);
            return 1;
        }
    }

    // primary MOT did not exist
    if (iNewMotHasArticle == 1) {
        // primary MOT did not exist, and the new TU-type hase an article
        // --> insert new TEP
        memset(&tNewPrimMot, 0, sizeof(tNewPrimMot));
        iRv = initNewPrimMOT(hMaskRl, pcFac, utr,
							 ptTekNow->tekTeId, &tIvArt, &tNewPrimMot);
        OPMSG_CFR(initNewPrimMOT);

        iRv = setTepInfo(pcFac, tNewPrimMot.tepPosNr, ptMcTek);
        OPMSG_CFR(setTepInfo);

        iRv = Frmwrk_UnimenuInsertElement(pcFac, ucr, lbc, MD_LBC_IDX_TEP,
                                          MD_TEP_LBC_NAME, &tNewPrimMot);
        OPMSG_CFR(Frmwrk_UnimenuInsertElement);

        iRv = handleMotDisplay(hMaskRl, pcFac);
        OPMSG_CFR(handleMotDisplay);

        return 1;
    }

    LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                         "INTERNAL ERROR: should never come here");
    return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
}

/**
 * callback for optionmenu TEK_TTS_TetId_t
 */
static int CbChangeTetId_TeBearb(MskDialog hMaskRl, MskStatic hEf,
        MskElement hEfRl, int iReason, void *cbc, void *pvCd)

{
    int iRv = 0;
	char const *pcFac = NULL;
    MskTcbcLf   *hCbcLf = NULL;
    UNITABLERL *utr;
	
    switch (iReason) {
    case FCB_LF:
        pcFac = lUniGetFacility (hMaskRl);

        hCbcLf = (MskTcbcLf *)cbc;
        iRv = changeTuTypeId(hMaskRl, pcFac, hCbcLf->var.p_char);
        if (iRv < 0) {
            TSqlRollback(hMaskRl);
            OPMSG_SFF(changeTuTypeId);
            OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Typ Änderung"), NULL);

            hCbcLf->result = 0;
            return EFCBM_ERROR;
        }
        TSqlCommit(hMaskRl);
        utr = (UNITABLERL *)MskDialogGet(hMaskRl, MskNmaskCalldata);
        UniUpdateMaskVar(hMaskRl, utr);
		break;
	default:
		break;
	}

	return EFCBM_CONTINUE;
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 		Gets infos for deleting TE
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbCeckBefore_TeWalk (const void* pvTid, char const *pcFac,
								eReasonTeWalk eReason,
								CDTEWALK *ptCd)
{
	int				iRv;
	
	switch (eReason) {
	case TEP_INSERT:
		/* fall through */
	case TEP_UPDATE:
		/* fall through */
	case TEP_NONE:
		if (ptCd->lAction == ACTION_DELETE) {
			break;
		}
		
		if (ptCd->ptTepNow->tepMId.ThmKz == JANEIN_J) {
			break;
		}
		
		if ((ptCd->lFlags & TEBEARB_NOTEMPTY) == 0) {
			// TE has been empty until now
			// no it is not empty anymore - set the flag
			ptCd->lFlags |= TEBEARB_NOTEMPTY;
			ptCd->ptTep = ptCd->ptTepNow;
		} else {
			// TE has not been empty and now we have another position
			// so this is a mixed TU
			ptCd->lFlags |= TEBEARB_MISCHTE;
		}
		break;
	
	case TEWALK_FINISH:
		if (ptCd->lAction == ACTION_DELETE) {
			break;
		}
		
		if (ptCd->ptTep == NULL ||
			(ptCd->lFlags & TEBEARB_NOTEMPTY) == 0 ||
			(ptCd->lFlags & TEBEARB_MISCHTE) ||
			(ptCd->lKorrTypRead  & TEBEARB_Wa)) {
			break;
		}
		
		iRv = ArticleSIf_CheckTetByAIdAndMng (pvTid, pcFac,
											  ptCd->ptTekNow->tekTTS_TetId,
											  &ptCd->ptTep->tepMngs,
											  &ptCd->ptTep->tepMId.AId);
		
		if (iRv == ARTICLEERRCODE_MNGTOHIGHFORTET) {
			ptCd->lFlags |= TEBEARB_MNGTOHIGH;
			OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed, "%s",
                             FwrefnrSIf_GetRefNr(TN_TEP, ptCd->ptTepNow));
		} else if (iRv == ARTICLEERRCODE_TETIDNOTFOUND) {
			OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed, "%s",
                             FwrefnrSIf_GetRefNr(TN_TEP, ptCd->ptTepNow));
			return (iRv);

		} else if (iRv < 0) {
			ptCd->pkcErrTxt = MlM("Mengenüberprüfung bezüglich des "
								 "TE-Typen fehlgeschlagen");
			OPMSG_CFR(ArticleSIf_CheckTetByAIdAndMng);
		}
		break;
		
	default:
		break;
	}
	
	return (0);
}

static int work4DelAaAndFAus (MskDialog hMaskRl, char const *pcFac, char *pcTeId)
{
	AAK tAak = {};	

	// read Aak
	int iRv = GowobaseSIf_GetUnfinishedAakForTeId (hMaskRl, pcFac, pcTeId, &tAak);
	if (iRv == 0 || iRv == GeneralTerrIntNoImp) {
		// no Aa found
		return 0;
	}
	OPMSG_CFR (GowobaseSIf_GetUnfinishedAakForTeId);

	// cancle Aa
	iRv = GowobaseSIf_CancelAa (hMaskRl, pcFac, &tAak.aakAaId);
	if (iRv == GeneralTerrIntNoImp) {
		return 0;
	}
	OPMSG_CFR (GowobaseSIf_CancelAa);

	// handle possible Faus
	iRv = GofausbaseSIf_ReadAndHandleFaus (hMaskRl, pcFac, 
				&tAak.aakFAUSK_FausId);
	if (iRv < 0 && iRv != GeneralTerrIntNoImp) {
		OPMSG_CFR (GofausbaseSIf_ReadAndHandleFaus);
	}

	return 1;
}

static int checkNoActiveVplkExistsForTargetTek(const void *pvTid, const char *pcFac, const char *pkcTeId)
{
	long lCount = 0;

	int iRv = TExecSql(pvTid,
			" SELECT COUNT("TCN_IV_VPLK_KsNr")"
			" FROM "TN_IV_TOURP
				" JOIN "TN_IV_VPLP" ON "TCN_IV_VPLP_Ist_TourpNr" = "TCN_IV_TOURP_TourpNr
				" JOIN "TN_IV_VPLK" ON "TCN_IV_VPLK_KsNr" = "TCN_IV_VPLP_VPLK_KsNr
			" WHERE "TCN_IV_TOURP_TEK_TeId" = :TeId"
				" AND "TCN_IV_VPLK_Status" = "STR_VPLKSTATUS_AKTIV,
			SELLONG   (lCount),
			SQLSTRING (pkcTeId),
			NULL);
	if (iRv != 1) {
		LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid,
				"%s: Error checking if TU:[%s] is a target TU for a currently active VPLK",
				__FUNCTION__, pkcTeId);
		return OpmsgSIf_ErrPush(GeneralTerrDb, NULL);
	}

	if (lCount > 0) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				"%s: TU:[%s] is a target TU for a currently active VPLK",
				__FUNCTION__, pkcTeId);
		return OpmsgSIf_ErrPush(GeneralTerrGeneral,
				MLM("Die TE kann nicht gelöscht werden, weil sie eine Ziel-TE eines momentan aktiven "
					"Kommissionierauftrages ist.\n"
					"Bitte warten Sie, bis der Kommissionierauftrag abgearbeitet ist und versuchen Sie "
					"dann erneut, die TE zu löschen."));
	}

	return 1;
}

static int beforeDeleteOrOk_TeBearb(MskDialog hMaskRl, char const *pcFac,
                                    MskStatic hEf)
{
	int					iRv;
	char	const		*pkcErrTxt = NULL;
    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek;
	UNITABLERL			*utr;
	TTEWALK				tTeWalk;
	TPA					tTpa;
	TEK					*ptTekNow;
	long				lCnt;
	STOCKBASE_TEBEARB_DEL tTeBearbDel;

    /* get mask context */
    hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
    if (hAtom == NULL ||
        hAtom->client_data == VNULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "MskAtomGet failed: "ATOM_UniAprMyContext);
        return OpmsgSIf_ErrPush((int)GeneralTerrGeneral, NULL);
    }

    ptMcTek = (MyTcontextTek*) hAtom->client_data;

    /* reset for CbAfterDelete_TeBearb */
    memset(ptMcTek->acTeId, 0, sizeof(ptMcTek->acTeId));

    /* get UniMenuData */
    utr = (UNITABLERL *)MskDialogGet (hMaskRl, MskNmaskCalldata);
    UniTransferMaskDup (hMaskRl, utr);
    ptTekNow = (TEK *)UniTableGetRecNow(utr);
    if (ptTekNow == NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "UniTableGetRecNow()");
        return OpmsgSIf_ErrPush((int)GeneralTerrGeneral, NULL);
    }

    /* TeWalk's */
    memset(&tTeWalk, 0, sizeof(tTeWalk));
    tTeWalk.ptMcTek = ptMcTek;
    tTeWalk.lFlags = 0;
    tTeWalk.lAction = (strcmp (hEf->ec.name, "Ok_F") == 0) ?	ACTION_OK : ACTION_DELETE;
    tTeWalk.isCbBefore = true;

    /* walk - check mng/tetid
     * NOTE: tTeWalk.lAction might be changed!
     */
    iRv = TeWalk_tebearb (hMaskRl, utr, &pkcErrTxt,
                          &tTeWalk, CbCeckBefore_TeWalk,
                          NULL /* stack pointer */);
    TSqlRollback(hMaskRl);
    if (iRv != ARTICLEERRCODE_TETIDNOTFOUND) {
        OPMSG_CFR(TeWalk_tebearb);
    }

    if (iRv == ARTICLEERRCODE_TETIDNOTFOUND ||
        tTeWalk.lFlags & TEBEARB_MNGTOHIGH) {

        iRv = WamasBox (SHELL_OF (hMaskRl),
                    WboxNboxType,   		WBOX_WARN,
                    WboxNbuttonText,		MlM("OK"),
                    WboxNbuttonRv,			IS_Ok,
                    WboxNbutton,    		WboxbNcancel,
                    WboxNescButton,    		WboxbNcancel,
                    WboxNmwmTitle,  		MlM ("TE Korrektur"),
                    WboxNtext,
                    (iRv < 0)
                        ? MlMsg("Mengenüberprüfung bezüglich TE-Typ kann "
                                "nicht durchgeführt werden.")
                        : MlMsg ("TE-Typ zu klein für erfasste Menge"),
                    WboxNbuttonText,	MlMsg ("Fehlerliste..."),	
                    WboxNbuttonCallbackName,	"OpmsgGIf_ErrDlg",
                    NULL);

        if (iRv != IS_Ok) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "User abort");
            return OpmsgSIf_ErrPush((int)GeneralTerrUserAbort, NULL);
        }
        LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
                  "override of warning: amount/TE-type check");
		OpmsgSIf_ErrResetMsg();
    }

    /* TPA */
    if (tTeWalk.lAction == ACTION_DELETE) {
        pkcErrTxt = "";
        memset (&tTpa, 0, sizeof(TPA));
        iRv = TpobaseSIf_GetTpaByTeId(hMaskRl, pcFac, ptTekNow->tekTeId, &tTpa);
        OPMSG_CFR(TpobaseSIf_GetTpaByTeId);
        if (iRv == 1) {
            TSqlRollback (hMaskRl);
            std::string msg(scoped_cstr::form("%s\n%s",
                    MlMsg ("TE löschen nicht möglich, Transportauftrag existiert."),
                    MlMsg ("Transportauftrag stornieren?")));
            iRv = WamasBox (SHELL_OF (hMaskRl),
                            WboxNboxType,   		WBOX_ALERT,
                            WboxNbuttonText,		MlM("Stornieren"),
                            WboxNbuttonRv,			IS_Ok,
                            WboxNbutton,    		WboxbNcancel,
                            WboxNescButton,    		WboxbNcancel,
                            WboxNmwmTitle,  		MlM ("TE Korrektur"),
                            WboxNtext,
                            msg.c_str(),
                            NULL);

            if (iRv != IS_Ok) {
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "User abort");
                return OpmsgSIf_ErrPush((int)GeneralTerrUserAbort, NULL);
            }

            iRv = TpobaseSIf_StornoTpa (hMaskRl, pcFac, &tTpa);
            OPMSG_CFR(TpobaseSIf_StornoTpa);

            TSqlCommit (hMaskRl);
            TrgutilSIf_ExecuteTriggers (pcFac);
            WamasBox (SHELL_OF (hMaskRl),
                      WboxNtimer,		5000,
                      WboxNboxType,		WBOX_INFO,
                      WboxNbutton,		WboxbNok,
                      WboxNescButton,	WboxbNok,
                      WboxNmwmTitle,	MlM ("TE Korrektur"),
                      WboxNtext,
                      MlMsg ("Transportauftrag wird storniert"),
                      NULL);
            
            // try again to get tpa
            iRv = TpobaseSIf_GetTpaByTeId(hMaskRl, pcFac, ptTekNow->tekTeId, &tTpa);
            if (iRv == 1) {
            	TSqlRollback (hMaskRl);
            	WamasBox (SHELL_OF (hMaskRl),
            			WboxNboxType,	WBOX_INFO,
            			WboxNbutton, 	WboxbNok,
           				WboxNescButton,	WboxbNok,
            			WboxNmwmTitle, 	MlM ("TE Korrektur"),
            			WboxNtext, 		MlMsg ("Transportauftrag noch vorhanden. Bitte erneut versuchen!"),
            			NULL);
            	return (0);
            }
        }
    }

    /* Target TU for active VPLK */
    if (tTeWalk.lAction == ACTION_DELETE && ptMcTek->lKorrTyp & TEBEARB_Wa) {
    	// This check is necessary to not break the picking workflow - otherwise it would be
    	// possible to cancel a TOURP (by deleting the TEK) while e.g. the MOT registration
    	// or printing-at-end workflow is active.
    	iRv = checkNoActiveVplkExistsForTargetTek(hMaskRl, pcFac, ptTekNow->tekTeId);
    	OPMSG_CFR (checkNoActiveVplkExistsForTargetTek);
    }

    /* TU of an ordinary goodsin-in process
     * - is not protocolled, yet and is not a supplier return TU
     * then we check if an INVBA exists
     */
    if ((ptMcTek->lKorrTyp & TEBEARB_We) &&
        (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure) == 0) {

        tTeWalk.lFlags = 0;
        lCnt = 0;
        iRv = TExecSql (hMaskRl,
                        " SELECT COUNT(INVBA.TEP_TEID) "
                        " FROM INVBA "
                        " WHERE INVBA.TEP_TEID = :teid "
                        " AND STATUS not in ("STR_INVBASTATUS_FERTIG","STR_INVBASTATUS_STORNO")",
                        SQLSTRING (ptTekNow->tekTeId),
                        SELLONG (lCnt),
                        NULL);

        if (iRv < 0) {
            LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, hMaskRl,
                    "failed to read INVBA");
            return OpmsgSIf_ErrPush((int)GeneralTerrDb,
                    MLM("Fehler beim Lesen der Inventurbedarfsposition"));
        }
        if(lCnt > 0) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "INVBA exists");
            return OpmsgSIf_ErrPush((int)GeneralTerrGeneral,
                    MLM("Wareneingangskorrektur ist nicht erlaubt, wenn eine "
                        "aktive Inventurbedarfsposition existiert"));
        }
    }
    
    if (tTeWalk.lAction == ACTION_DELETE) {
    	// AA and FAUS handling
    	iRv = work4DelAaAndFAus (hMaskRl, pcFac, ptTekNow->tekTeId);
    	OPMSG_CFR (work4DelAaAndFAus);
    }

    memset(&ptMcTek->tInfoDelTek, 0, sizeof(ptMcTek->tInfoDelTek));
    ptMcTek->iInfoDelTek = 0;

    /* if the user inserts or updates the TU, she needs to enter additional
     * data: e.g. Buschl, NeutralId, ..
     *
     * if a TU is only updated, we do not need this data
     * INSERT or DELETE only / WE not selected or Retoure only
     */
    if (tTeWalk.lAction != ACTION_UPDATE &&
        ((ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) == 0 ||
         (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure))) {

        memset(&tTeBearbDel, 0, sizeof(tTeBearbDel));
        tTeBearbDel.lCheckLieferant = JANEIN_J;
		FRMWRK_STRARRNCPY (tTeBearbDel.acNeutralId, ptMcTek->acNeutralId);
		tTeBearbDel.lNeutralInvNr = ptMcTek->lNeutralInvNr;
		tTeBearbDel.lNeutralPosNr = ptMcTek->lNeutralPosNr;

        if ((ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) &&
            (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure)) {
            // goodsin supplier return

            iRv = ParamSIf_Get1Parameter(hMaskRl, pcFac,
                                         PrmStockbaseTeKorLRetBuschl,
                                          tTeBearbDel.acBschl);
            if (iRv == ParameterTerrNotFound) {
                OpmsgSIf_ErrResetMsg();
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
                    "Parameter (%s) is not set", PrmStockbaseTeKorLRetBuschl);
            } else {
                OPMSG_CFR(ParamSIf_Get1Parameter);
            }

            tTeBearbDel.lCheckNeutralId = JANEIN_N;

            iRv = _me_tebearb_delete(SHELL_OF(hMaskRl), &tTeBearbDel);
            OPMSG_CFR(_me_tebearb_delete);
        } else if (ptMcTek->lKorrTyp & TEBEARB_Std &&
                   tTeWalk.lAction == ACTION_INSERT) {

            tTeBearbDel.lCriteria |= AccodeSifTCheckBschlNeutral;
            tTeBearbDel.lCheckNeutralId = JANEIN_J;
            tTeBearbDel.iDelete = 1;
            LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
            "Set iDelete is 1, for not check neutral id ");

            iRv = _me_tebearb_delete(SHELL_OF(hMaskRl), &tTeBearbDel);	
            OPMSG_CFR(_me_tebearb_delete);
        } else {
            tTeBearbDel.lCriteria |= AccodeSifTCheckBschlNeutral;
            tTeBearbDel.lCheckNeutralId = JANEIN_J;

            iRv = _me_tebearb_delete(SHELL_OF(hMaskRl), &tTeBearbDel);	
            OPMSG_CFR(_me_tebearb_delete);
        }

        if (iRv != DELETE_YES) {
            memset (&ptMcTek->tInfoDelTek, 0, sizeof(StockbaseTChangeTepInfo));
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "cancel");
            return OpmsgSIf_ErrPush((int)GeneralTerrUserAbort, NULL);
        } else {
            FRMWRK_STRARRNCPY (ptMcTek->tInfoDelTek.acBschl, tTeBearbDel.acBschl);
            FRMWRK_STRARRNCPY (ptMcTek->tInfoDelTek.acKostSt, tTeBearbDel.acKostSt);
            FRMWRK_STRARRNCPY (ptMcTek->tInfoDelTek.acMand, tTeBearbDel.acMand);
            FRMWRK_STRARRNCPY (ptMcTek->tInfoDelTek.acLiefNr, tTeBearbDel.acLiefNr);
            FRMWRK_STRARRNCPY (ptMcTek->tInfoDelTek.acNeutralId, tTeBearbDel.acNeutralId);
            ptMcTek->tInfoDelTek.lNeutralInvNr = tTeBearbDel.lNeutralInvNr;
            ptMcTek->tInfoDelTek.lNeutralPosNr = tTeBearbDel.lNeutralPosNr;
        }
        ptMcTek->iInfoDelTek = 1;
    }

	if (tTeWalk.lAction == ACTION_UPDATE &&
			IsEmptyString(ptTekNow->tekEin_LagRf)){
			iRv = WamasBox (SHELL_OF(hMaskRl),
                            WboxNboxType, WBOX_WARN,
                            WboxNbutton, WboxbNok,
                            WboxNbutton, WboxbNcancel,
                            WboxNtimer, 5000,
                            WboxNmwmTitle, MlMsg("Warnung"),
                            WboxNtext, MlMsg("Einlagerreihenfolge ist nicht gesetzt.\n"
                                            "Wollen Sie trotzdem fortfahren?"),
                            NULL);

            if(iRv != WboxbNok){
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "cancel");
				return OpmsgSIf_ErrPush((int)GeneralTerrUserAbort, NULL);
            }
	}

    return 1;
}
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 		Gets infos for deleting TE
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbBeforeDeleteOrOk_TeBearb(MskDialog hMaskRl, MskStatic hEf,
	MskElement hEfRl, int iReason, void *cbc, void *pvCd)

{
	int			iRv;
	char const *pcFac = NULL;
	
    switch (iReason) {
	case FCB_XF:
        pcFac = lUniGetFacility (hMaskRl);
		OpmsgSIf_ErrResetMsg();
		TrgutilSIf_ResetTriggers(pcFac);

        iRv = beforeDeleteOrOk_TeBearb(hMaskRl, pcFac, hEf);
        if (iRv < 0) {
        	TSqlRollback(hMaskRl);
        	OPMSG_SFF(beforeDeleteOrOk_TeBearb);
        	OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("Transporteinheit löschen"), NULL);
        	return (EFCBM_ERROR);
        }
        if (iRv == 0) {
        	// TPA exists
        	return (EFCBM_ERROR);
        }
        TSqlCommit(hMaskRl);

		break;
	default:
		break;
		
	}
	return (EFCBM_CONTINUE);
}
		
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 		Gets infos for deleting TE
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbAfterDelete_TeBearb(MskDialog hMaskRl, MskStatic hEf,
	MskElement hEfRl, int iReason, void *cbc, void *pvCd)

{
	char const *pcFac;
    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek;
	UNITABLERL			*utr;
	TEK					*ptTekNow;
	
	
    switch (iReason) {
	case FCB_XF:
        pcFac = lUniGetFacility (hMaskRl);
		OpmsgSIf_ErrResetMsg();
    	
		/* get mask context */
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						  "MskAtomGet failed: "ATOM_UniAprMyContext);
			
			return (EFCBM_ERROR);
    	}
		ptMcTek = (MyTcontextTek*) hAtom->client_data;
		
		/* get UniMenueData */
		utr = (UNITABLERL *)MskDialogGet (hMaskRl, MskNmaskCalldata);
		
		if (IsEmptyStrg(ptMcTek->acTeId) == 0) {
			
			ptTekNow = (TEK *)UniTableGetRecNow (utr);
			if (ptTekNow == NULL) {
				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
									  "UniTableGetRecNow failed");
				return (EFCBM_ERROR);
			}
		
			FRMWRK_STRARRNCPY (ptTekNow->tekTeId, ptMcTek->acTeId);
			UniRead(hMaskRl, utr, 1);
			UniUpdateMaskVarR(hMaskRl, utr);
		}
		break;
	default:
		break;
		
	}
	return (EFCBM_CONTINUE);
}
		
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 		Gets infos for deleting TE
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbNewTeId_TeBearb(MskDialog hMaskRl, MskStatic hEf,
	MskElement hEfRl, int iReason, void *cbc, void *pvCd)

{
	int					iRv;
	char const *pcFac;
    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek;
	UNITABLERL			*utr;
	UNITABLE			*ut;
	TEK					*ptTekNow;
	char				acTeId[TEID_LEN+1];
	char				acMand[MANDANT_LEN+1];
	
    switch (iReason) {
	case FCB_XF:
        pcFac = lUniGetFacility (hMaskRl);
		OpmsgSIf_ErrResetMsg();
		
		/* get mask context */
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						  "MskAtomGet failed: "ATOM_UniAprMyContext);
			
			return (EFCBM_ERROR);
    	}
		ptMcTek = (MyTcontextTek*) hAtom->client_data;
		
		/* get UniMenueData */
		utr = (UNITABLERL *)MskDialogGet (hMaskRl, MskNmaskCalldata);
		ut = UniTableGetTable (utr);

		UniTransferMaskDup (hMaskRl, utr);
		
		ptTekNow = (TEK *)UniTableGetRecNow (utr);
		if (ptTekNow == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "UniTableGetRecNow failed");
			return (EFCBM_ERROR);
		}
			
		
		if (IsEmptyStrg(ptTekNow->tekTeId) == 0) {
			
			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,	WBOX_ALERT,
				WboxNbutton,	WboxbNok,
				WboxNmwmTitle,	MlM ("Neue TE-Id"),
				WboxNtext,		MlMsg ("Feld TE-Id ist nicht leer"),
				NULL);
			break;
		
		} else {
            memset(acTeId, 0, sizeof(acTeId));
            memset (acMand, 0, sizeof(acMand));

            iRv = StockbaseGIf_MeGetNewTeId(hMaskRl, pcFac, acMand, UCTYP_WA,
                                            acTeId);

            (void )TSqlRollback (hMaskRl);

            if (iRv < 0) {
				OPMSG_SFF(StockbaseGIf_MeGetNewTeId);
				OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("Neue TE-Id"), NULL);
                break;
            }

            FRMWRK_STRARRNCPY(ptTekNow->tekTeId, acTeId);
            MskVaAssign (hMaskRl, MskGetElement ("TEK_TeId_t"),
                         MskNtransferVar2Dup,       (Value)TRUE,
                         MskNupdate,                (Value)TRUE,
                         NULL);
            memset (&ptMcTek->tAusk, 0, sizeof(AUSK));
            memset (&ptMcTek->tWevk, 0, sizeof(WEVK));
            memset (&ptMcTek->tWevBearb, 0, sizeof(ptMcTek->tWevBearb));
            ptMcTek->lKorrTypRead = TEBEARB_Std;
            ptMcTek->lKorrTyp = TEBEARB_Std;
            ptMcTek->iModTep = 1;  /* no real effect */
            if (ut && ut->utCalldata) {
                ((TeBearbCalldata *)ut->utCalldata)->eOrigCaller =
                                                        OriginalCaller_Standard;
            }

            /* causes UNITABLEREASON_UPDATE_MASK_VAR in MaskCallbacks */
            UniUpdateMaskVarR (hMaskRl, utr);
		}
		
		break;
	default:
		break;
		
	}
	return (EFCBM_CONTINUE);
}
		
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	cb for checkbuttons
-*	sets editfields and buttons by causing update of both masks
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbCheckBt_TeBearb (MskDialog hMaskRl, MskStatic hEf,
								   MskElement hEfRl, int iReason, void *cbc,
								   void *pvCd)
{
	char const *pcFac;
    ContainerAtom   	hAtom;
	UNITABLERL			*utr;
	UNITABLERL			*utrChild;
	UNITABLE			*ut;
	UNICONNECTRL		*ucrChild;
	TEK					*ptTekNow,
			  			*ptTekBefore;
	TEP					*ptTepNow,
			  			*ptTepBefore,
			  			tTep;
	long				lKorrTyp = 0,
			  			lSet;
	MskTcbcXf 			*cbc_xf;
    MyTcontextTek   	*ptMcTek;
	
    switch (iReason) {
	case FCB_XF:
	 	cbc_xf = (MskTcbcXf *)cbc;
        pcFac = lUniGetFacility (hMaskRl);
    	
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		
		utr = (UNITABLERL *)MskDialogGet (hMaskRl, MskNmaskCalldata);
		ut = UniTableGetTable (utr);
		
		ptTekBefore = (TEK *)UniTableGetRecBefore (utr);
		ptTekNow = (TEK *)UniTableGetRecNow (utr);
		
		ucrChild = (UNICONNECTRL *)UniTableGetConnectRlArray (utr);
		utrChild = (UNITABLERL *)UniConnectGetTableRl (ucrChild);
		
        ptTepNow = (TEP *)UniTableGetRecNow(utrChild);
		/* UniTableGetRecBefore is ok here,
		 * because checkbutton becomes inactive, when ptTepBefore changes */
        ptTepBefore = (TEP *)UniTableGetRecBefore(utrChild);
		
		if (hAtom == NULL || hAtom->client_data == (Value)NULL ||
			ptTepBefore == NULL || ptTepNow == NULL ||
			ptTekBefore == NULL || ptTekNow == NULL) {
			
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						  "UniTableGetRec or MskAtomGet failed");
			
			/* set old value */
			MskElementVaSet (hEfRl,
							 MskNtransferVar2Dup, 	(Value) TRUE,
							 MskNupdate,			(Value) TRUE,
							 NULL);
			
			if (cbc_xf) {
				cbc_xf->result = FALSE;
			}
			return 0;
    	}
		
		ptMcTek = (MyTcontextTek*) hAtom->client_data;
        // remember old value of korrTyp
		lKorrTyp = ptMcTek->lKorrTyp;
        // get new value from mask
		UniTransferMaskDup(hMaskRl, utr);
		
		if (ptMcTek->iModTep != 0 ||
			memcmp(ptTekNow, ptTekBefore, sizeof(TEK))) {
            // TEP has been modified
			OpmsgGIf_ShowAlertBox(hMaskRl, GeneralTerrDb, MlM("TE-Korrektur"), 
					MlMsg ("Ändern der TE-Korrektur nach "
							 "erneutem Lesen möglich"));
			
			/* change of korrTyp is not allowed - reset to old value */
			ptMcTek->lKorrTyp = lKorrTyp;
			MskElementVaSet (hEfRl,
							 MskNtransferVar2Dup, 	(Value) TRUE,
							 MskNupdate,			(Value) TRUE,
							 NULL);
			
			if (cbc_xf) {
				cbc_xf->result = FALSE;
			}
        	return 0;
		}
		
        /* lSet will have all bits set, that the user has enabled since the
         * last change
         */
		lSet = ptMcTek->lKorrTyp & (~lKorrTyp);

		/* TEBEARB_Std and TEBEARB_We - must not be set both */
		if (lSet & TEBEARB_Std) {
            // user has selected Std-correction

            // clear the We-Bit
			ptMcTek->lKorrTyp &= (~TEBEARB_We);
            // clear the We-Bit
			ptMcTek->lKorrTyp &= (~TEBEARB_WeNach);
			
			if (ut && ut->utCalldata) {
                // reset original caller
				((TeBearbCalldata *)ut->utCalldata)->eOrigCaller =
                                                        OriginalCaller_Standard;
			}
		} else if (lSet & (TEBEARB_We | TEBEARB_WeNach)) {
            // user has selected We-correction

            // clear standard correction bit
			ptMcTek->lKorrTyp &= (~TEBEARB_Std);
		}
		
		/* reset TEP */
		if (lSet == 0) {
			*ptTepNow = *ptTepBefore;
		
		} else if (lSet & (TEBEARB_We| TEBEARB_WeNach)) {
			tTep = *ptTepNow;
			*ptTepNow = *ptTepBefore;
			ptTepNow->tepMngs = tTep.tepMngs;
		}
		
		/* new flags set/reset */
		MskElementSet (hEfRl, MskNtransferDup2Var, (Value) TRUE);
		
		/* causes UNITABLEREASON_UPDATE_MASK_VAR in MaskCallbacks */
		UniUpdateMaskVarR (hMaskRl, utr);

		break;
	default:
		break;
	}
	return 1;
}	

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbFillInvNr(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                    int iReason, void *pvCbc,void *xx)

{
	
	MskTcbcOl       *cbc_ol;
    int             nI,iDbRvX;
	long			alInvNr[BLOCKSIZE];

    switch (iReason) {

    case FCB_OM:
        cbc_ol      = (MskTcbcOl *)pvCbc;
        switch (cbc_ol->xreason) {
        case XCB_OCLOSED:
            MskNextTabGroup((MskTgenericRl *)hEfRl);
            break;
        case XCB_OCREATE:
			MskTransferMaskDup (hMaskRl);

			iDbRvX = 0;
			do {
				memset (alInvNr, 0, sizeof(alInvNr));
				if (iDbRvX == 0) {
					iDbRvX = TExecSqlX (hMaskRl, NULL,
						"	SELECT INVNR from INVK ",
						BLOCKSIZE, 0,
						SELLONG (alInvNr[0]),
						NULL);
				} else {
					iDbRvX = TExecSqlV (hMaskRl, NULL, NULL, NULL, NULL, NULL);
				}
				if (iDbRvX < 0) {
					if (TSqlError(hMaskRl) != SqlNotFound) {
						TSqlRollback(hMaskRl);
						return (OpmsgSIf_ErrPush (GeneralTerrDb,
												  "%s", TSqlErrTxt(hMaskRl)));
					}
				}

				for (nI = 0; nI < iDbRvX; nI ++) {
					if(alInvNr[nI] != 0){
						MskOptmLoadEf ((MskTtextRl *)(void *) hEfRl,
								   &alInvNr[nI],NULL, cbc_ol);
					}
				}
			} while (BLOCKSIZE == iDbRvX);
			TSqlRollback(hMaskRl);
			break;
		}
		
		break;
		
    default:
        break;
    }

	return 1;
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbFillInvPosNr(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                    int iReason, void *pvCbc,void *xx)
{
	ContainerAtom   hAtom;	
	MskTcbcOl       *cbc_ol;
    int             nI,iDbRvX;
	long			alInvNr[BLOCKSIZE];
    MyTcontextTep  	*ptMcTep;

    switch (iReason) {

    case FCB_OM:
        cbc_ol      = (MskTcbcOl *)pvCbc;
        switch (cbc_ol->xreason) {
        case XCB_OCLOSED:
            MskNextTabGroup((MskTgenericRl *)hEfRl);
            break;
        case XCB_OCREATE:
			MskTransferMaskDup (hMaskRl);
			hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
			if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
				break;
			}
			ptMcTep = (MyTcontextTep*)hAtom->client_data;

			iDbRvX = 0;
			do {
				memset (alInvNr, 0, sizeof(alInvNr));
				if (iDbRvX == 0) {
					iDbRvX = TExecSqlX (hMaskRl, NULL,
						"	SELECT POSNR from INVP WHERE INVNR = :invnr ",
						BLOCKSIZE, 0,
						SELLONG (alInvNr[0]),
						SQLLONG (ptMcTep->lNeutralInvNr ),
						NULL);
				} else {
					iDbRvX = TExecSqlV (hMaskRl, NULL, NULL, NULL, NULL, NULL);
				}
				if (iDbRvX < 0) {
					if (TSqlError(hMaskRl) != SqlNotFound) {
						TSqlRollback(hMaskRl);
						return (OpmsgSIf_ErrPush (GeneralTerrDb,
												  "%s", TSqlErrTxt(hMaskRl)));
					}
				}

				for (nI = 0; nI < iDbRvX; nI ++) {
					if(alInvNr[nI] != 0){
						MskOptmLoadEf ((MskTtextRl *)(void *) hEfRl,
								   &alInvNr[nI],NULL, cbc_ol);
					}
				}
			}while (BLOCKSIZE == iDbRvX);
			TSqlRollback(hMaskRl);
        break;

		}
    default:
        break;
    }

	return 1;
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*-------------------------------------------------------------------------*/
static int CbCalcMng_TeBearb(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                    int iReason, void *pvCbc,void *xx)

{
    UNITABLERL  		*utr;
    MskTcbcLf           *hCbcLf = NULL;
    ContainerAtom   	hAtom;
    MyTcontextTep   	*ptMcTep;

    TEP                	*ptRecTep = NULL, *ptTepBefore = NULL;
	CALCMNGBESTANZ 		eMngBestAnz = (CALCMNGBESTANZ)0;
	double				dAnzMng, dRestMng,
			            dVarSave = 0.0;
    char const *pcFac;
	int					iRv;
	MNGS                tMngs;

    switch (iReason) {
	case FCB_LF:
        /* Log facility */
        pcFac = lUniGetFacility (hMaskRl);

        /* Unitable-Handle holen */
        utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);

        UniTransferMaskDup(hMaskRl,utr);

    	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);

		if (hAtom == NULL && hAtom->client_data) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"MskAtomGet failed: ATOM "ATOM_UniAprMyContext);
        	return (-1);
    	}

		ptMcTep = (MyTcontextTep*)hAtom->client_data;

        hCbcLf = (MskTcbcLf *)pvCbc;

        if (hCbcLf == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "hCbcLf is NULL !");
            break;
        }

        ptRecTep = (TEP *)UniTableGetRecNow(utr);
		ptTepBefore = (TEP *)UniTableGetRecBefore (utr);
		if (ptRecTep == NULL ||
			ptTepBefore == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "UniTableGetRec failed");
			return EFCBM_ERROR;
		}

		tMngs = ptRecTep->tepMngs;

		dAnzMng = ptMcTep->dAnzMng;
		dRestMng = ptMcTep->dRestMng;

        if (strcmp (hEf->ec.name, TEP_Mngs_Mng_t) == 0) {

			switch (hEfRl->ec_rl.key) {
			case KEY_DEF:
				dVarSave = ptRecTep->tepMngs.Mng;
            	ptRecTep->tepMngs.Mng = *(double *)hCbcLf->var.p_double;
				eMngBestAnz = BESTANZ;
				break;
			case KEY_1:
				dVarSave = ptMcTep->dAnzMng;
				dAnzMng = *(double *)hCbcLf->var.p_double;
				eMngBestAnz = ANZBEST;
				break;
			case KEY_2:	
				dVarSave = ptMcTep->dRestMng;
				dRestMng = *(double *)hCbcLf->var.p_double;
				eMngBestAnz = ANZBEST;
				break;
			default:
				break;
			}
        }

        if (strcmp (hEf->ec.name,TEP_Mngs_Gew_t ) == 0) {
			dVarSave = ptRecTep->tepMngs.Gew;
			ptRecTep->tepMngs.Gew = *(double *)hCbcLf->var.p_double;
			eMngBestAnz = CALCGEW;
        }

		if (strcmp (hEf->ec.name, "ME_TEBEARBP_MengeR_t") == 0 &&
			hEfRl->ec_rl.key == KEY_1) {
			ptRecTep->tepMngs.Mng += *(double *)hCbcLf->var.p_double; 	

			*(double *)hCbcLf->var.p_double = 0;

			eMngBestAnz = BESTANZ;
		}

		if (strcmp (hEf->ec.name, "ME_TEBEARBP_MengeR_t") == 0 &&
			hEfRl->ec_rl.key == KEY_2) {
			dAnzMng += *(double *)hCbcLf->var.p_double;

			*(double *)hCbcLf->var.p_double = 0;

			eMngBestAnz = ANZBEST;
		}

		OpmsgSIf_ErrResetMsg();
		
		iRv = ArticleSIf_GetMngGewFromArt_GD (hMaskRl, pcFac,
					&ptRecTep->tepMngs, &ptMcTep->tGdArt, &ptMcTep->tGdArte, &dAnzMng,
					&dRestMng, eMngBestAnz);

        if (iRv < 0) {
			(void )TSqlRollback (hMaskRl);
			OPMSG_SFF(ArticleSIf_GetMngGewFromArt_GD);
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), NULL);
			
			ptRecTep->tepMngs = tMngs;
			*(double *)hCbcLf->var.p_double = dVarSave;
			break;
		}
		
        ptMcTep->dAnzMng = dAnzMng;
        ptMcTep->dRestMng = dRestMng;

		if (strcmp (hEf->ec.name, TEP_Mngs_Mng_t)  == 0) {
			if (hEfRl->ec_rl.key == KEY_2) {
				*(double *)hCbcLf->var.p_double = dRestMng;
			} else if (hEfRl->ec_rl.key == KEY_1) {
				*(double *)hCbcLf->var.p_double = dAnzMng;
			}
		}

        (void )TSqlRollback (hMaskRl);
		/* for negativ quantities/weight set MatQ inactive otherwise  activ */
		if (ptRecTep->tepMngs.Mng < 0 ||
			ptRecTep->tepMngs.Gew < 0 ||
			ptTepBefore->tepMngs.Mng < 0 ||
			ptTepBefore->tepMngs.Gew < 0) {
			MskVaAssign (hMaskRl,  MskGetElement("TEP_MId_MatQ_t"),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrOr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,   (Value)TRUE,
						 NULL);
		} else {
			MskVaAssign (hMaskRl,  MskGetElement("TEP_MId_MatQ_t"),
						 MskNkey,			(Value) KEY_DEF,
						 MskNattrClr,		(Value) EF_ATTR_INACTIVE,
						 MskNupdate,   (Value)TRUE,
						 NULL);

		}

        MskVaAssignMatch (hMaskRl,  "*-Calc-*",
            MskNupdate,             (Value)1,
            MskNtransferVar2Dup,    (Value)1,
            NULL);

		break;
	default:
		break;
	}
	
	return (1);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CheckTpa4We_tebearb (const void* pvTid, char const *pcFac, TPA *ptTpa, POS *ptSollPos)
{
	
	int iRv;
	
	if (ptTpa->tpaStatus != TPASTATUS_NEU &&
		ptTpa->tpaStatus != TPASTATUS_SENDEN) {
				
		return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("WE Korrektur nicht möglich, "
						 "aktiver Transportauftrag existiert")));
	}
			
	iRv = SlobaseSIf_PosCmpFeldBez (pvTid, pcFac, &ptTpa->tpaAktPos, ptSollPos);
	OPMSG_CFR(SlobaseSIf_PosCmpFeldBez);
	
	if (iRv == 1) {
		return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("WE Korrektur nicht möglich, "
						 "TE steht nicht auf aktueller Position des TPAs")));
	}
	
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CheckTpap_tebearb (const void* pvTid, char const *pcFac, TPA *ptTpa, 
							  TEP *ptTepBefore, TEP *ptTepNow)
{
	
	int iRv = 0;
	long lCntTpap = 0;
	std::string	stmt = "select count(*)"
						" from "TN_TPAP
					   " where "TCN_TPAP_TaNr" = :tanr"
					     " and "TCN_TPAP_TEP_TeId" = :teid"
						 " and "TCN_TPAP_TEP_PosNr" = :posnr";

	iRv = TExecSql(pvTid, stmt.c_str(), 
				   SELLONG(lCntTpap),
				   SQLLONG(ptTpa->tpaTaNr),
				   SQLSTRING(ptTepNow->tepTeId),
				   SQLLONG(ptTepNow->tepPosNr),
				   NULL);
	if (iRv <= 0) {
		LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid,
							   "failed to read TPAP. TaNr[%ld] TeId[%s] PosNr[%ld]",
								ptTpa->tpaTaNr, ptTepNow->tepTeId, ptTepNow->tepPosNr);
		return OpmsgSIf_ErrPush((int)GeneralTerrDb, MLM("Fehler beim Lesen von Transportauftragspositionen"));
	}
	
	if (lCntTpap > 0) {
		// TPAP available. In this case don't allow change of quantity
		if (FrmwrkSIf_CmpDouble(ptTepBefore->tepMngs.Mng, ptTepNow->tepMngs.Mng,
				FrmwrkSIf_GetStandardDoublePrecision()) != 0 ||
			FrmwrkSIf_CmpDouble(ptTepBefore->tepMngs.Gew, ptTepNow->tepMngs.Gew,
					FrmwrkSIf_GetStandardDoublePrecision()) != 0) {
			return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("Korrektur der Menge nicht möglich, "
						 "TE-Position ist in Verwendung in einer Transportauftragsposition.")));
			
		}
	}

	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CheckAndWriteTeId_tebearb (const void* pvTid, char const *pcFac,
        eReasonTeWalk eReason, TEK const *pktTekNow)
{
	int iRv;
    long lAllowedTypes =FRMWRK_BIT_ENC(TYP_Normal) | FRMWRK_BIT_ENC(TYP_Extern);
    TEIDCFG tTEIDCFG;

    if (eReason == TEK_UPDATE) {
        // temp ids also allowed
        FRMWRK_BIT_SET(lAllowedTypes, TYP_Temp);
    }

    memset(&tTEIDCFG, 0, sizeof(tTEIDCFG));
	iRv = Stockbase_CheckTuId_INTERNAL(pvTid, pcFac, pktTekNow->tekTeId,
                                           lAllowedTypes, NULL, JANEIN_N,
                                           &tTEIDCFG);
    OPMSG_CFR(Stockbase_CheckTuId_INTERNAL);

	if (eReason == TEK_INSERT &&
        tTEIDCFG.teidcfgBenutzt == JANEIN_J) {
		
		iRv = _StockbaseSIf_WriteUsedTeId(pvTid, pcFac, pktTekNow->tekTeId);
        OPMSG_CFR(_StockbaseSIf_WriteUsedTeId);
	}
		
	return (0);
}

static int handleStammTe(const void* pvTid, char const *pcFac, char const *pcTeId)
{
	TEK tTekParent = {};
	FRMWRK_STRARRNCPY(tTekParent.tekTeId, pcTeId);

	int iRv = TExecStdSql(pvTid, StdNselectUpdNo, TN_TEK, &tTekParent);
	if (iRv != 1) {
		LoggingSIf_PkLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid, TN_TEK, &tTekParent, NULL);
		return (OpmsgSIf_ErrPush(GeneralTerrDb, "'%s' : '%s'",
				LoggingSIf_GetLogPk(pvTid, TN_TEK, &tTekParent), TSqlErrTxt (pvTid)));
	}

	iRv = StockbaseS_HandleTek(pvTid, pcFac, tTekParent.tekTeId, HandleTuCheckTts_No);
	OPMSG_CFR(StockbaseS_HandleTek);

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int WriteTeComplete_tebearb (const void* pvTid, char const *pcFac, TEK *ptTekNow)
{
	int			iRv;

	// Tu can be emtpy because of auto sent stock correction to host. The Tu can be delete if no TEP
	// exists any more.
	TEK tTek = *ptTekNow;
	iRv = FrmwrkSIf_ReadRecord(pvTid, pcFac, TN_TEK, &tTek);
	if (iRv == GeneralTerrNoDataFound) {
		OpmsgSIf_ErrResetMsg();
		return 0;
	} else {
		OPMSG_CFR(FrmwrkSIf_ReadRecord);
	}

	iRv = StockbaseS_HandleTek(pvTid,pcFac,ptTekNow->tekTeId,HandleTuCheckTts_No);
	OPMSG_CFR(StockbaseS_HandleTek);

	if (!IsEmptyStrg(ptTekNow->tekStamm_TeId)) {

		iRv = handleStammTe(pvTid, pcFac, ptTekNow->tekStamm_TeId);
		OPMSG_CFR(handleStammTe);
	}

	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int WriteInv_tebearb (const void* pvTid, char const *pcFac, StockbaseTChangeTepInfo *ptTepInfo,
							 TEK *ptTekNow, TEP *ptTepNow, TEP *ptTepBefore)
{
	if (ptTepNow->tepPrimThmKz == JANEIN_J){
		/*Inv not needed for THMs*/
		return 1;
	}
	
	StdemandTstockMod stockMod = {};
	
	int iRv = FrmwrkSIf_BdCopyTable(pvTid, pcFac,
			TN_IV_TEP, &stockMod.tIvTep, TN_TEP, ptTepBefore, BdCopyTableCopyAll);
	OPMSG_CFR(FrmwrkSIf_BdCopyTable);
	
	stockMod.ptNewMngs= &ptTepNow->tepMngs;
	stockMod.ptNewMId = &ptTepNow->tepMId;
	stockMod.tPos     = ptTekNow->tekPos;
	stockMod.eUcTyp   = UCTYP_KOR;
	FRMWRK_STRARRNCPY (stockMod.acBuSchl,     ptTepInfo->acBschl);
	FRMWRK_STRARRNCPY (stockMod.acKostSte,    ptTepInfo->acKostSt);
	FRMWRK_STRARRNCPY (stockMod.acNeutraleId, ptTepInfo->acNeutralId);
	stockMod.lNeutraleInvNr = ptTepInfo->lNeutralInvNr;
	stockMod.lNeutralePosNr = ptTepInfo->lNeutralPosNr;
	
	iRv = StdemandSIf_StockCorrection(pvTid, pcFac, stockMod, false);
	if (iRv != GeneralTerrIntNoImp) {
		OPMSG_CFR (StdemandSIf_StockCorrection);
	}
	
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int WriteMngs_tebearb (const void* pvTid, char const *pcFac,
		StockbaseTChangeTepInfo *ptTepInfo, TEP *ptTepNow, TEP *ptTepBefore, KRETK *ptKretk)
{
	int				iRv = 0;
	MODTEPMNGDESC	tModTepMng = {};
	MODTEPMNGSTRUCT tModTepMngStruct = {};
	
	tModTepMng.lAnzStruct = 1;
	tModTepMng.ptTepMngStruct = &tModTepMngStruct;
	tModTepMng.eCheckTts = HandleTuCheckTts_No;
	FRMWRK_STRARRNCPY (tModTepMng.acBuSchl, ptTepInfo->acBschl);
	FRMWRK_STRARRNCPY (tModTepMng.acKostSte, ptTepInfo->acKostSt);
	
	FRMWRK_STRARRNCPY (tModTepMngStruct.acTeId, ptTepNow->tepTeId);
	tModTepMngStruct.lPosNr = ptTepNow->tepPosNr;
	tModTepMngStruct.tMngs = ptTepNow->tepMngs;
	tModTepMngStruct.tStockbaseProtZap.tKretId = ptKretk->kretkKretId;
	FRMWRK_STRARRNCPY (tModTepMngStruct.tStockbaseProtZap.acKSTMand, ptKretk->kretkKST_Mand);
	FRMWRK_STRARRNCPY (tModTepMngStruct.tStockbaseProtZap.acKSTKuNr, ptKretk->kretkKST_KuNr);
    if (IsEmptyString(ptKretk->kretkKretId.KretNr)) {
		if(ptTepNow->tepPrimThmKz == JANEIN_J){
			tModTepMngStruct.tStockbaseProtZap.eUcTyp = UCTYP_INTERN;
		}else {
			tModTepMngStruct.tStockbaseProtZap.eUcTyp = UCTYP_KOR;
		}
	} else {
		tModTepMngStruct.tStockbaseProtZap.eUcTyp = UCTYP_KRET;
	}

    if (strcmp(ptTepNow->tepMId.SerienNrGrp, ptTepBefore->tepMId.SerienNrGrp) != 0) {
    	tModTepMngStruct.changeSerials = true;
    	FRMWRK_STRARRNCPY (tModTepMngStruct.acNewSerienNrGrp, ptTepNow->tepMId.SerienNrGrp);
    }
	
	iRv = StockbaseSIf_ModTepMng (pvTid, pcFac, &tModTepMng);
	OPMSG_CFR(StockbaseSIf_ModTepMng);
	
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int MakeKorr4Wev_tebearb (const void* pvTid, char const *pcFac,
								GivgTWevBearb *ptWevBearb,
								StockbaseTChangeTepInfo *ptTepInfo,
								TEP *ptTepNow, TEP *ptTepBefore, UCTYP eUcTyp,
                                eReasonTeWalk eReason,
								OpmsgTCbStackPtr stackPtr)
{
	int				iRv = 0;
	V_GDART			tGdArt = {};
	GivgTKorr4Wev	tKorr4Wev;
	
	int cmp = 0;
	iRv = compareDates(pcFac, ptTepNow->tepMId.MHD, ptTepBefore->tepMId.MHD, cmp);
	OPMSG_CFR(compareDates);

    if (CMPDOUBLE(ptTepNow->tepMngs.Gew,
				  ptTepBefore->tepMngs.Gew) == 0 &&
		CMPDOUBLE(ptTepNow->tepMngs.Mng,
				  ptTepBefore->tepMngs.Mng) == 0 &&
		cmp == 0 &&
		strcmp(ptTepNow->tepMId.Charge,ptTepBefore->tepMId.Charge) == 0 &&
		strcmp(ptTepNow->tepMId.ResNr,ptTepBefore->tepMId.ResNr) == 0 ) {
		return (0);
	}
	
	memset(&tKorr4Wev, 0, sizeof(tKorr4Wev));
	FRMWRK_STRARRNCPY (tKorr4Wev.acTeId, ptTepNow->tepTeId);
	tKorr4Wev.lTepPosNr = ptTepNow->tepPosNr;
	tKorr4Wev.zMhd = ptTepNow->tepMId.MHD;
	FRMWRK_STRARRNCPY (tKorr4Wev.acCharge, ptTepNow->tepMId.Charge);
	FRMWRK_STRARRNCPY (tKorr4Wev.acResNr, ptTepNow->tepMId.ResNr);
	tKorr4Wev.eMode = ptWevBearb->eMode;
	tKorr4Wev.tPrivate = ptTepInfo->tPrivate;
    tKorr4Wev.eQtyReduced = JANEIN_N;
	
	iRv = ArticleSIf_SubMngs (pcFac,
							  &ptTepNow->tepMngs,
							  &ptTepBefore->tepMngs,
							  &tKorr4Wev.tDiffMngs);
	
	/* when quantities are reduced, set the eQtyReduced flag */
	if(eReason == TEP_DELETE) {
	    /* if TEP was deleted, quantities are set 0 => quantities
	    were reduced */
	    tKorr4Wev.eQtyReduced = JANEIN_J;
	} else {
	    /* else the eReason = TEP_UPDATE. If tDiffMngs < 0 => quantities
	    are reduced */
	    if((tKorr4Wev.tDiffMngs.Gew < 0) && (tKorr4Wev.tDiffMngs.Mng < 0)) {
	        tKorr4Wev.eQtyReduced = JANEIN_J;
	    }
	}

	iRv = ArticleSIf_GetArtByAId_GD (pvTid, pcFac, &ptTepNow->tepMId.AId, 
									&tGdArt);
	OPMSG_CFR(ArticleSIf_GetArtByAId_GD);

	if (ptTepNow->tepMId.ThmKz &&
		tGdArt.gdartBestKz == JANEIN_J) {
		tKorr4Wev.eBestThm = JANEIN_J;
	}	

	OPMSG_CFR(ArticleSIf_SubMngs);
	
	iRv = GivgSIf_MakeKorr4Wev (pvTid, pcFac, &tKorr4Wev, eUcTyp,
			stackPtr);
	OPMSG_CFR(GivgSIf_MakeKorr4Wev);

	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int WriteTepAndInv_tebearb (const void* pvTid, char const *pcFac,
								eReasonTeWalk eReason,
								StockbaseTChangeTepInfo *ptTepInfo,
								TEK *ptTekNow, TEK *ptTekBefore,
								TEP *ptTepNow, TEP *ptTepBefore,
								KRETK *ptKretk)
{
	int					iRv;
	TEP					tTepNow;
	TEP					tTepBefore;
	TEP_TRIGGER			tTepTrigger;
	OpmsgTCbStackPtr    stackPtr = NULL;

	/* DELETE */
	if (eReason == TEP_DELETE) {
		
		tTepBefore = *ptTepBefore;
		tTepNow = *ptTepBefore;
		
		// If there is a group of serials on the TEP clear it in tTepNow so that serial removal is sent to the host
		if (_StockbaseSIf_IsInternalSerial(pvTid, pcFac, tTepNow.tepMId.SerienNrGrp)) {
			memset(tTepNow.tepMId.SerienNrGrp, 0, sizeof(tTepNow.tepMId.SerienNrGrp));
		}
		
		memset (&tTepNow.tepMngs, 0, sizeof(MNGS));

		/* Write Mngs */
		iRv = WriteMngs_tebearb (pvTid, pcFac, ptTepInfo,
								 &tTepNow, &tTepBefore, ptKretk);
		if (iRv == StockbaseTerrTeNotExists) {
			/// Because THM is deleted in Stockcorrection from the correction of the std tep
			return 0;
		}
		OPMSG_CFR(WriteMngs_tebearb);
		
		/* Write Inventur */
		iRv = WriteInv_tebearb (pvTid, pcFac, ptTepInfo,
				ptTekNow, &tTepNow, &tTepBefore);
		OPMSG_CFR(WriteInv_tebearb);
		
		if (tTepBefore.tepPrimThmKz == JANEIN_J){
			/*We have to book THM back to KomZone*/
			StockbaseSIfTMoveMot 	tmpData = {};
			ArrayPtr hArrQThm = ArrCreate(sizeof(StockbaseSIfTMoveMot), 
															1, NULL, NULL);
			if (hArrQThm == NULL) {
				LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT,
									"ArrCreate", 
									"Error in ArrCreate()!");
				return OpmsgSIf_ErrPush (GeneralTerrMemory, NULL);
			}
			
			tmpData.tTep = tTepBefore;
			iRv = ArrAddElem(hArrQThm, &tmpData);
			if (iRv < 0) {
				LoggingSIf_LogPrintfTools (pcFac, LOGGING_SIF_ALERT,
									"ArrAddElem",
									"ArrAddElem failed");
				return OpmsgSIf_ErrPush (GeneralTerrMemory, NULL);
			}

			iRv = OpmsgSIf_CbCreate(pcFac, &stackPtr);
			OPMSG_CFR(OpmsgSIf_CbCreate);

			iRv = _StockbaseSIf_MoveMot2Pickingloc(pvTid, pcFac, hArrQThm, stackPtr);
			OPMSG_CFR (_StockbaseSIf_MoveMot2Pickingloc);
		}
		
	/* INSERT */
	} else if (eReason == TEP_INSERT) {
		
		tTepNow = *ptTepNow;
		tTepBefore = *ptTepNow;

		// If there is a group of serials on the TEP clear it in tTepBefore so that serial addition is sent to the host
		if (_StockbaseSIf_IsInternalSerial(pvTid, pcFac, tTepBefore.tepMId.SerienNrGrp)) {
			memset(tTepBefore.tepMId.SerienNrGrp, 0, sizeof(tTepBefore.tepMId.SerienNrGrp));
		}

		memset (&tTepBefore.tepMngs, 0, sizeof(MNGS));
		
		/* insert of TEP with MNGS 0 */
		memset(&tTepTrigger, 0, sizeof(tTepTrigger));
		tTepTrigger.tTep = tTepBefore;
		FRMWRK_STRARRNCPY (tTepTrigger.acFeldId, ptTekNow->tekPos.FeldId);
		
		iRv = CallTepBefInsertTrigger(pvTid, pcFac, &tTepTrigger, 1);
		OPMSG_CFR(CallTepBefInsertTrigger);
		
		iRv = TExecStdSql (pvTid, StdNinsert, TN_TEP, &tTepBefore);
		
		if (iRv != 1) {
			LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEP, &tTepBefore, NULL);
			return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEP, &tTepBefore),
			    	TSqlErrTxt (pvTid)));
		}
		
		/* Write Mngs */
		iRv = WriteMngs_tebearb (pvTid, pcFac, ptTepInfo,
								 &tTepNow, &tTepBefore, ptKretk);
		OPMSG_CFR(WriteMngs_tebearb);
		
		/* Write Inventur */
		iRv = WriteInv_tebearb (pvTid, pcFac, ptTepInfo,
				ptTekNow, &tTepNow, &tTepBefore);
		OPMSG_CFR(WriteInv_tebearb);
		
		if(tTepBefore.tepPrimThmKz == JANEIN_J) {
            /*We have to book new THM to Tek */
            V_GDTEK tempV_GdTek = {};

            iRv = FrmwrkSIf_BdCopyTable(pvTid, pcFac, TN_V_GDTEK, &tempV_GdTek, TN_TEK,
                                ptTekNow, 0);

            OPMSG_CFR(FrmwrkSIf_BdCopyTable);

            int iRv = StockbaseSIf_GetMngFromKomz (pvTid, pcFac,
                                          &tTepNow.tepMId, &tTepNow.tepMngs,
                                          &tempV_GdTek, &tTepNow.tepPosNr,
                                          JANEIN_N, tTepNow.tepMId.AId.Mand,
                                          JANEIN_J, UCTYP_INTERN,
                                          NULL);/*stackptr*/
            OPMSG_CFR(StockbaseSIf_GetMngFromKomz);
        }
	
	/* UPDATE */
	} else if (eReason == TEP_UPDATE &&
			   (memcmp(&ptTepNow->tepMId.AId,
					   &ptTepBefore->tepMId.AId, sizeof(AID)))) {
		
		/* MID changed
		 * delete old Pos and Insert new Pos */
		tTepBefore = *ptTepBefore;
		tTepNow = *ptTepBefore;

		// If there is a group of serials on the TEP clear it in tTepNow so that serial removal is sent to the host
		if (_StockbaseSIf_IsInternalSerial(pvTid, pcFac, tTepNow.tepMId.SerienNrGrp)) {
			memset (tTepNow.tepMId.SerienNrGrp, 0, sizeof(tTepNow.tepMId.SerienNrGrp));
		}

		memset (&tTepNow.tepMngs, 0, sizeof(MNGS));
		
		/* Write Mngs */
		iRv = WriteMngs_tebearb (pvTid, pcFac, ptTepInfo,
								 &tTepNow, &tTepBefore, ptKretk);
		OPMSG_CFR(WriteMngs_tebearb);
		
		/* Write Inventur */
		iRv = WriteInv_tebearb (pvTid, pcFac, ptTepInfo,
				ptTekNow, &tTepNow, &tTepBefore);
		OPMSG_CFR(WriteInv_tebearb);
		
		if (tTepBefore.tepPrimThmKz == JANEIN_J){
			/*We have to book old THM back to KomZone*/
			StockbaseSIfTMoveMot 	tmpData = {};
			ArrayPtr hArrQThm = ArrCreate(sizeof(StockbaseSIfTMoveMot), 
															1, NULL, NULL);
			if (hArrQThm == NULL) {
				LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT,
									"ArrCreate", 
									"Error in ArrCreate()!");
				return OpmsgSIf_ErrPush (GeneralTerrMemory, NULL);
			}
			
			tmpData.tTep = tTepBefore;
			iRv = ArrAddElem(hArrQThm, &tmpData);
			if (iRv < 0) {
				LoggingSIf_LogPrintfTools (pcFac, LOGGING_SIF_ALERT,
									"ArrAddElem",
									"ArrAddElem failed");
				return OpmsgSIf_ErrPush (GeneralTerrMemory, NULL);
			}

			iRv = OpmsgSIf_CbCreate(pcFac, &stackPtr);
			OPMSG_CFR(OpmsgSIf_CbCreate);

			iRv = _StockbaseSIf_MoveMot2Pickingloc(pvTid, pcFac, hArrQThm, stackPtr);
			OPMSG_CFR (_StockbaseSIf_MoveMot2Pickingloc);
		}
		
		tTepNow = *ptTepNow;
		tTepBefore = *ptTepNow;

		// If there is a group of serials on the TEP clear it in tTepBefore so that serial addition is sent to the host
		if (_StockbaseSIf_IsInternalSerial(pvTid, pcFac, tTepBefore.tepMId.SerienNrGrp)) {
			memset (tTepBefore.tepMId.SerienNrGrp, 0, sizeof(tTepBefore.tepMId.SerienNrGrp));
		}

		memset (&tTepBefore.tepMngs, 0, sizeof(MNGS));
		
		/* insert of TEP with MNGS 0 */
		memset(&tTepTrigger, 0, sizeof(tTepTrigger));
		tTepTrigger.tTep = tTepBefore;
		FRMWRK_STRARRNCPY (tTepTrigger.acFeldId, ptTekNow->tekPos.FeldId);
		
		iRv = CallTepBefInsertTrigger(pvTid, pcFac, &tTepTrigger, 1);
		OPMSG_CFR(CallTepBefInsertTrigger);
		
		iRv = TExecStdSql (pvTid, StdNinsert, TN_TEP, &tTepBefore);
		
		if (iRv != 1) {
			LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEP, &tTepBefore, NULL);
			
			if (TSqlError(pvTid) == SqlDuplicate) {
				return (OpmsgSIf_ErrPush (StockbaseTerrChangeArtOnKomz, NULL));
			} else {
				return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEP, &tTepBefore),
			    	TSqlErrTxt (pvTid)));
			}
		}
		
		/* Write Mngs */
		iRv = WriteMngs_tebearb (pvTid, pcFac, ptTepInfo,
								 &tTepNow, &tTepBefore, ptKretk);
		OPMSG_CFR(WriteMngs_tebearb);
		
		/* Write Inventur */
		iRv = WriteInv_tebearb (pvTid, pcFac, ptTepInfo,
				ptTekNow, &tTepNow, &tTepBefore);
		OPMSG_CFR(WriteInv_tebearb);

		if(tTepBefore.tepPrimThmKz == JANEIN_J) {
			/*We have to book new THM to Tek */
			V_GDTEK tempV_GdTek = {};

			iRv = FrmwrkSIf_BdCopyTable(pvTid, pcFac, TN_V_GDTEK, &tempV_GdTek, TN_TEK,
                                ptTekNow, 0);	

		    OPMSG_CFR(FrmwrkSIf_BdCopyTable);

			int iRv = StockbaseSIf_GetMngFromKomz (pvTid, pcFac,
                                          &tTepNow.tepMId, &tTepNow.tepMngs,
                                          &tempV_GdTek, &tTepNow.tepPosNr,
                                          JANEIN_N, tTepNow.tepMId.AId.Mand,
                                          JANEIN_J, UCTYP_INTERN,
                                          NULL);/*stackptr*/
		    OPMSG_CFR(StockbaseSIf_GetMngFromKomz);
		}
	} else if (eReason == TEP_UPDATE) {

		bool bMIdChanged = false;
		bool bMngChanged = false;
		
		tTepBefore = *ptTepBefore;
 		iRv = wamas::wms::_StockbaseSIf_IsMatQGroupEq(pvTid, pcFac, 
 										(char*)ptTepBefore->tepMId.MatQ,
 										(char*)ptTepNow->tepMId.MatQ);
 		if (iRv <= 0){
 			if (iRv == 0) {
 				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
 									"MatQGruppen unterscheiden sich.\n"
 									"MaterialqualifikationsIdtausch nur "
 									"innerhalb selber MatQGruppe möglich.");
 				return OpmsgSIf_ErrPush((int)StockbaseTerrMatqKGroupsDiffer,
							MLM("MatQGruppen unterscheiden sich.\n"
 								"MaterialqualifikationsIdtausch nur "
 								"innerhalb selber MatQGruppe möglich."));
 			} else {
 				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
 									 "Fehler bei MatQGruppenüberprüfung.");
 				return OpmsgSIf_ErrPush((int)GeneralTerrDb,
 									MLM("Fehler bei MatQGruppenüberprüfung."));
 			}
 		}	
 
		/* update Matq, SerienNrGrp, ResNr, Charge, MHD, WeNr */
		iRv = _StockbaseSIf_IsMIdDiff(pvTid, pcFac, ptTepBefore->tepMId, ptTepNow->tepMId);
		OPMSG_CFR (_StockbaseSIf_IsMIdDiff);
		if (iRv == 1) {
			tTepNow = tTepBefore;
			FRMWRK_STRARRNCPY (tTepNow.tepMId.MatQ, ptTepNow->tepMId.MatQ);
			FRMWRK_STRARRNCPY (tTepNow.tepMId.SerienNrGrp, ptTepNow->tepMId.SerienNrGrp);
			FRMWRK_STRARRNCPY (tTepNow.tepMId.ResNr, ptTepNow->tepMId.ResNr);
			FRMWRK_STRARRNCPY (tTepNow.tepMId.WeNr, ptTepNow->tepMId.WeNr);
			FRMWRK_STRARRNCPY (tTepNow.tepMId.Charge, ptTepNow->tepMId.Charge);
			tTepNow.tepMId.MHD = ptTepNow->tepMId.MHD;
			
			IV_TEP tIvTepBefore = {};
			iRv = FrmwrkSIf_BdCopyTable(pvTid, pcFac, TN_IV_TEP, &tIvTepBefore, TN_TEP, &tTepBefore, 0);
			OPMSG_CFR (FrmwrkSIf_BdCopyTable);
			
			iRv = _StockbaseSIf_ChangeTepMId(pvTid, pcFac, tIvTepBefore, tTepNow.tepMId, UCTYP_KOR, ptKretk);
			OPMSG_CFR (_StockbaseSIf_ChangeTepMId);
			
			bMIdChanged = true;
		}
	
		/* update except Mngs, SerienNrGrp */
		tTepBefore = *ptTepNow;
		tTepBefore.tepMngs = ptTepBefore->tepMngs;
		if (!bMIdChanged) {
			FRMWRK_STRARRNCPY (tTepBefore.tepMId.SerienNrGrp, ptTepBefore->tepMId.SerienNrGrp);
		}
		
		/* No trigger needed because no mng is changed */
		iRv = TExecStdSql(pvTid, StdNupdate, TN_TEP, &tTepBefore);
		
		if (iRv != 1) {
			LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEP, &tTepBefore, NULL);
			return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEP, &tTepBefore),
			    	TSqlErrTxt (pvTid)));
		}
		
		/* update Mngs and serial numbers (if not updated with changed MID) */
		/* put to the end, otherwise ZAP is not correct, if MID changes too */
		if (FRMWRK_DOUBLECMP (ptTepNow->tepMngs.Mng, !=, ptTepBefore->tepMngs.Mng)
				|| FRMWRK_DOUBLECMP (ptTepNow->tepMngs.Gew, !=, ptTepBefore->tepMngs.Gew)
				|| strcmp(ptTepNow->tepMId.SerienNrGrp, ptTepBefore->tepMId.SerienNrGrp) != 0) {
			tTepNow = *ptTepNow;
			tTepBefore = *ptTepNow;
			tTepBefore.tepMngs = ptTepBefore->tepMngs;
			if (!bMIdChanged) {
				FRMWRK_STRARRNCPY (tTepBefore.tepMId.SerienNrGrp, ptTepBefore->tepMId.SerienNrGrp);
			}
			
			/* Write Mngs */
			iRv = WriteMngs_tebearb (pvTid, pcFac, ptTepInfo,
									 &tTepNow, &tTepBefore, ptKretk);
			OPMSG_CFR(WriteMngs_tebearb);
			
			bMngChanged = true;
		}
		
		/* Write Inventur */
		if (bMIdChanged || bMngChanged) {
			iRv = WriteInv_tebearb (pvTid, pcFac, ptTepInfo,
									ptTekNow, ptTepNow, ptTepBefore);
			OPMSG_CFR(WriteInv_tebearb);
		}
	}
	return (0);
}

static int FttTtsCheck (const void* pvTid, char const *pcFac, TEK *ptTek, CDTEWALK *ptCd)
{
	if (!IsEmptyStrg (ptTek->tekTTS_TetId) && !IsEmptyStrg (ptTek->tekFTT_FachtId)) {
		// FTT has TTS, FTT is entered and is not valid
		
		std::vector<IV_FTT> vecIvFtt;
		int iRv = StockbaseSIf_GetFtt4TetId (pvTid, pcFac, 
										ptTek->tekTTS_TetId, vecIvFtt);
		OPMSG_CFR (StockbaseSIf_GetFtt4TetId);
		
		bool found = false;
		unsigned int iCnt;
		for (iCnt = 0; iCnt < vecIvFtt.size(); iCnt++) {
			if (strcmp(vecIvFtt[iCnt].ivfttFachtId, ptTek->tekFTT_FachtId) == 0) {
				found = true;	
				break;
			}
		} // for
		if (found == false && vecIvFtt.size() > 0) 
		{
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"FTT '%s' is not valid for TTS '%s' - ask user",
					ptTek->tekFTT_FachtId, ptTek->tekTTS_TetId);
			
			int iTtsChanged = 0;
			if (strcmp (ptCd->ptTekBefore->tekTTS_TetId, ptTek->tekTTS_TetId) != 0) {
				iTtsChanged = 1;
			}

			int iRv;
			iRv = WamasBox (SHELL_OF (ptCd->hMaskRl),
					WboxNboxType,    WBOX_WARN,
					WboxNbuttonText, MlM("Löschen"),
					WboxNbuttonRv,	 IS_Ok,
					WboxNbutton,     WboxbNcancel,
					WboxNescButton,  WboxbNcancel,
					WboxNmwmTitle, 
					(iTtsChanged == 1)
						? MlM ("TE-Typ ändern")
						: MlM ("Fachteilungstyp ändern"),
					WboxNtext, 		 MlMsg("Fachteilungstyp stimmt mit dem TE-Typ nicht überein.\nEingetragene "
							"Fachteilungsdaten der Transporteinheit werden gelöscht."),
					NULL);
			if (iRv != IS_Ok) {
				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "User abort");
				return OpmsgSIf_ErrPush((int)GeneralTerrUserAbort, NULL);
			}
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
					"override of warning: FTT/TTS check");
			// reset tekFTT
			memset (ptTek->tekFTT_FachtId, 0, sizeof(ptTek->tekFTT_FachtId));
			memset (ptCd->ptTekNow->tekFTT_FachtId, 0, sizeof(ptCd->ptTekNow->tekFTT_FachtId));
		}
	}
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Insert TEK	
-* RETURNS
-*--------------------------------------------------------------------------*/
static int InsertTek (const void* pvTid, char const *pcFac,
						CDTEWALK *ptCd)
{
	int			iRv;
	TEK			tTekNow;
	TEK			tTekParent;

	tTekNow = *ptCd->ptTekNow;
	memset (tTekNow.tekStamm_TeId, 0, sizeof(tTekNow.tekStamm_TeId));
	
	iRv = SlobaseSIf_CheckTu4Pos(pvTid, pcFac, ptCd->stackPtr,
			NULL, &tTekNow.tekPos, NULL,
			NULL, &tTekNow.tekTet,
			NULL, tTekNow.tekTTS_TetId,
			tTekNow.tekGesGew, ptCd->lStapelHoehe,
			tTekNow.tekTeId);
	OPMSG_CFR(SlobaseSIf_CheckTu4Pos);

	iRv = FttTtsCheck (pvTid, pcFac, &tTekNow, ptCd);
	OPMSG_CFR (FttTtsCheck);

	iRv = CallTekBefInsertTrigger (pvTid, pcFac, &tTekNow, 1);
	OPMSG_CFR(CallTekBefInsertTrigger);

	iRv = TExecStdSql(pvTid, StdNinsert, TN_TEK, &tTekNow);
	if (iRv != 1) {
		LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
				pvTid, TN_TEK, &tTekNow, NULL);
		return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
				LoggingSIf_GetLogPk (pvTid, TN_TEK, &tTekNow),
				TSqlErrTxt (pvTid)));
	}

	/* StammTeId has to be linked by special function */
	if (!IsEmptyStrg(ptCd->ptTekNow->tekStamm_TeId)) {
		
		memset (&tTekParent, 0, sizeof(tTekParent));
		FRMWRK_STRARRNCPY(tTekParent.tekTeId, ptCd->ptTekNow->tekStamm_TeId);
		
		iRv = TExecStdSql(pvTid, StdNselectUpdNo, TN_TEK, &tTekParent);
		if (iRv != 1) {
			LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEK, &tTekParent, NULL);
			return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEK, &tTekParent),
					TSqlErrTxt (pvTid)));
		}

		Array arrSubTeks(sizeof(TEK), 1, NULL, NULL);

		iRv = ArrAddElem (arrSubTeks, &tTekNow);
		if (iRv != 1) {
            LoggingSIf_LogPrintfTools (pcFac, LOGGING_SIF_ALERT, 
										"ArrAddElem", "failed to add TEK");
            return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
		}

		iRv = StockbaseSIf_LinkWithParentTu (pvTid, pcFac, &tTekParent,
				arrSubTeks, HandleTuCheckTts_Yes);
		OPMSG_CFR(StockbaseSIf_LinkWithParentTu);
	}

	return (1);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Update TEK	
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UpdateTek (const void* pvTid, char const *pcFac,
						CDTEWALK *ptCd)
{
	int		iRv;
	TEK		tTekNow,
			tTekParent;

	tTekNow = *ptCd->ptTekNow;
	FRMWRK_STRARRNCPY(tTekNow.tekStamm_TeId, ptCd->ptTekBefore->tekStamm_TeId);

	iRv = SlobaseSIf_CheckTu4Pos(pvTid, pcFac, ptCd->stackPtr,
				&ptCd->ptTekBefore->tekPos,
				&tTekNow.tekPos, NULL,
				&ptCd->ptTekBefore->tekTet, &tTekNow.tekTet,
				ptCd->ptTekBefore->tekTTS_TetId,
				tTekNow.tekTTS_TetId,
				tTekNow.tekGesGew,  ptCd->lStapelHoehe,
				tTekNow.tekTeId);
	OPMSG_CFR(SlobaseSIf_CheckTu4Pos);

	iRv = FttTtsCheck (pvTid, pcFac, &tTekNow, ptCd);
	OPMSG_CFR (FttTtsCheck);
		
	/* Tek before update trigger */
	iRv = CallTekBefUpdateTrigger (pvTid, pcFac, ptCd->ptTekBefore,
			&tTekNow, 1);
	OPMSG_CFR(CallTekBefUpdateTrigger);
	
		
	iRv = TExecStdSql(pvTid, StdNupdate, TN_TEK, &tTekNow);
	if (iRv != 1) {
		LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
				pvTid, TN_TEK, &tTekNow, NULL);
		return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
				LoggingSIf_GetLogPk (pvTid, TN_TEK, &tTekNow),
				TSqlErrTxt (pvTid)));
	}

	/* Tek after update trigger */
	iRv = CallTekAftUpdateTrigger (pvTid, pcFac,
			ptCd->ptTekBefore, &tTekNow, 1);
	OPMSG_CFR(CallTekAftUpdateTrigger);

	/* Unlink for Stamm_TeId */
	if (IsEmptyStrg(ptCd->ptTekNow->tekStamm_TeId) &&
		!IsEmptyStrg(ptCd->ptTekBefore->tekStamm_TeId)) {

		Array arrTeksUnlink(sizeof(TEK), 1, NULL, NULL);

		iRv = ArrAddElem (arrTeksUnlink, &tTekNow);
		if (iRv != 1) {
            LoggingSIf_LogPrintfTools (pcFac, LOGGING_SIF_ALERT, 
										"ArrAddElem", "failed to add TEK");
            return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
		}

		iRv = StockbaseSIf_UnlinkFromParentTu (pvTid, pcFac, arrTeksUnlink,
			   	HandleTuCheckTts_Yes);
		OPMSG_CFR(StockbaseSIf_UnlinkFromParentTu);
	
	/* Link for Stamm_TeId */	
	} else if (!IsEmptyStrg(ptCd->ptTekNow->tekStamm_TeId) &&
				strncmp(ptCd->ptTekNow->tekStamm_TeId,
						ptCd->ptTekBefore->tekStamm_TeId, TEID_LEN) != 0) {
		
		memset (&tTekParent, 0, sizeof(tTekParent));
		FRMWRK_STRARRNCPY(tTekParent.tekTeId, ptCd->ptTekNow->tekStamm_TeId);
		
		iRv = TExecStdSql(pvTid, StdNselectUpdNo, TN_TEK, &tTekParent);
		if (iRv != 1) {
			LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEK, &tTekParent, NULL);
			return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEK, &tTekParent),
					TSqlErrTxt (pvTid)));
		}

		Array arrTeksLink(sizeof(TEK), 1, NULL, NULL);

		iRv = ArrAddElem (arrTeksLink, &tTekNow);
		if (iRv != 1) {
            LoggingSIf_LogPrintfTools (pcFac, LOGGING_SIF_ALERT, 
										"ArrAddElem", "failed to add TEK");
            return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
		}

		iRv = StockbaseSIf_LinkWithParentTu (pvTid, pcFac, &tTekParent,
				arrTeksLink, HandleTuCheckTts_Yes);
		OPMSG_CFR(StockbaseSIf_LinkWithParentTu);
	}

	return (1);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 	check and write WE-Korr
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbWe1_TeWalk (const void* pvTid, char const *pcFac, eReasonTeWalk eReason,
						CDTEWALK *ptCd)
{
	int				iRv;
	TEP				tTep;
    JANEIN          eDeleteTe;
    StockbaseTChangeTepInfo *ptTepInfo;
	GivgTPrepare    tPrepare = {};
	
	switch (eReason) {
	case TEP_UPDATE:

		iRv = GivgSIf_IsTuProtConsistentency(pvTid, pcFac,
											 ptCd->ptTekNow->tekTeId);
		if (iRv == GivgTErrTuChangedSinceAquisition) {
            LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
                "No WE Correction available! "
				"There was already a Std-Correction");
            return OpmsgSIf_ErrPush (GeneralTerrGeneral,
							  MLM("Keine WE-Korrektur möglich! "
								  "Es wurde bereits eine Standard-Korrektur "
								  "durchgeführt."));
		}
		OPMSG_CFR(GivgSIf_IsTuProtConsistentency);
		
		iRv = MakeKorr4Wev_tebearb (pvTid, pcFac,
								   ptCd->ptWevBearb, &ptCd->tTepInfo,
								   ptCd->ptTepNow, ptCd->ptTepBefore, UCTYP_KOR,
                                   eReason,
								   ptCd->stackPtr);
		OPMSG_CFR(MakeKorr4Wev_tebearb);
		break;
	
	case TEP_DELETE:
		
		iRv = GivgSIf_IsTuProtConsistentency(pvTid, pcFac,
											 ptCd->ptTekNow->tekTeId);
		if (iRv == GivgTErrTuChangedSinceAquisition) {
            LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
                "No WE Correction available! "
				"There was already a Std-Correction");
            return OpmsgSIf_ErrPush (GeneralTerrGeneral,
							  MLM("Keine WE-Korrektur möglich! "
								  "Es wurde bereits eine Standard-Korrektur "
								  "durchgeführt."));
		}
		OPMSG_CFR(GivgSIf_IsTuProtConsistentency);

		tTep = *ptCd->ptTepBefore;
		tTep.tepMngs.Mng = 0.0;
		tTep.tepMngs.Gew = 0.0;
		
		iRv = MakeKorr4Wev_tebearb (pvTid, pcFac,
								   ptCd->ptWevBearb,
								   &ptCd->tTepInfo,
								   &tTep, ptCd->ptTepBefore, UCTYP_KOR,
                                   eReason,
								   ptCd->stackPtr);
		OPMSG_CFR(MakeKorr4Wev_tebearb);

		/* update tep info after changes on wevp happened */
		for (ptTepInfo = (StockbaseTChangeTepInfo*)ArrWalkStart(ptCd->hArrTepInfo);
			 ptTepInfo != NULL;
			 ptTepInfo = (StockbaseTChangeTepInfo*)ArrWalkNext(ptCd->hArrTepInfo)){

            tPrepare.eMode = ptCd->ptWevBearb->eMode;
            tPrepare.ePosTyp = ptCd->ptWevBearb->ePosTyp;
            tPrepare.tWevId = ptCd->ptWevBearb->tWevId;
            FRMWRK_STRARRNCPY(tPrepare.acTeId, ptCd->ptTepBefore->tepTeId);
			/* actual position number */
            tPrepare.lTepPosNr = ptTepInfo->lPosNr;
		
			iRv = GivgSIf_PrepareWevp4Korr (pvTid, pcFac, &tPrepare);	
			OPMSG_CFR(GivgSIf_PrepareWevp4Korr);			

			ptTepInfo->tPrivate = tPrepare.tPrivate;
		}

		break;
	
	case TEWALK_FINISH:

		iRv = GivgSIf_IsTuProtConsistentency(pvTid, pcFac,
											 ptCd->ptTekNow->tekTeId);
		if (iRv == GivgTErrTuChangedSinceAquisition) {
            LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
                "No WE Correction available! "
				"There was already a Std-Correction");
            return OpmsgSIf_ErrPush (GeneralTerrGeneral,
							  MLM("Keine WE-Korrektur möglich! "
								  "Es wurde bereits eine Standard-Korrektur "
								  "durchgeführt."));
		}
		OPMSG_CFR(GivgSIf_IsTuProtConsistentency);

        eDeleteTe = (ptCd->lAction == ACTION_DELETE) ? JANEIN_J:JANEIN_N;
        iRv = GivgSIf_TerminateWev4Korr (pvTid, pcFac,
										 &ptCd->ptWevBearb->tWevId,
										 ptCd->ptTepBefore->tepTeId,eDeleteTe,
										 &ptCd->tTepInfo.tPrivate.tWevp,
										 ptCd->ptWevBearb->eMode);
		OPMSG_CFR(GivgSIf_TerminateWev4Korr);
		break;
		
	case TEK_INSERT:
		ptCd->pkcErrTxt = MlM("Anlegen einer TE bei "
							 "gewählter WE-Korrektur nicht möglich");
		return (OpmsgSIf_ErrPush (GeneralTerrGeneral, NULL));
			
	case TEP_INSERT:
		ptCd->pkcErrTxt = MlM("Anlegen von Positionen bei "
							 "gewählter WE-Korrektur nicht möglich");
		return (OpmsgSIf_ErrPush (GeneralTerrGeneral, NULL));
			
	default:
		break;
	}
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 	write Tep, Tep.Mngs, ZAP
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbWe2_TeWalk (const void* pvTid, char const *pcFac, eReasonTeWalk eReason,
						CDTEWALK *ptCd)
{
	int				iRv;
	TEP				tTepNow;
	TEP				tTepBefore;
	
	switch (eReason) {
	
	case TEK_UPDATE:

		iRv = UpdateTek (pvTid, pcFac, ptCd);
		OPMSG_CFR(UpdateTek);
		break;
	
	case TEK_DELETE:
		break;
	
	case TEP_UPDATE:
		
		/* retoure */
		if (ptCd->lKorrTypRead & TEBEARB_WeRetoure) {
			
			iRv = WriteTepAndInv_tebearb (pvTid, pcFac,
										eReason, &ptCd->tTepInfo,
										ptCd->ptTekNow, ptCd->ptTekBefore,
										ptCd->ptTepNow, ptCd->ptTepBefore,
										&ptCd->tKretk);
            OPMSG_CFR(WriteTepAndInv_tebearb);
		} else {
			
			bool bMIdChanged = false;

			tTepBefore = *ptCd->ptTepBefore;

			/* update SerienNrGrp, ResNr, Charge, MHD */
			iRv = _StockbaseSIf_IsMIdDiff(pvTid, pcFac, ptCd->ptTepBefore->tepMId, ptCd->ptTepNow->tepMId);
			OPMSG_CFR (_StockbaseSIf_IsMIdDiff);
			if (iRv == 1) {
				tTepNow = tTepBefore;
				FRMWRK_STRARRNCPY (tTepNow.tepMId.SerienNrGrp, ptCd->ptTepNow->tepMId.SerienNrGrp);
				FRMWRK_STRARRNCPY (tTepNow.tepMId.ResNr, ptCd->ptTepNow->tepMId.ResNr);
				FRMWRK_STRARRNCPY (tTepNow.tepMId.Charge, ptCd->ptTepNow->tepMId.Charge);
				tTepNow.tepMId.MHD = ptCd->ptTepNow->tepMId.MHD;

				IV_TEP tIvTepBefore = {};
				iRv = FrmwrkSIf_BdCopyTable(pvTid, pcFac, TN_IV_TEP, &tIvTepBefore, TN_TEP, &tTepBefore, 0);
				OPMSG_CFR (FrmwrkSIf_BdCopyTable);

				iRv = _StockbaseSIf_ChangeTepMId(pvTid, pcFac, tIvTepBefore, tTepNow.tepMId, UCTYP_KOR, &ptCd->tKretk);
				OPMSG_CFR (_StockbaseSIf_ChangeTepMId);

				bMIdChanged = true;
			}

			/* update except Mngs, SerienNrGrp */
			tTepBefore = *ptCd->ptTepNow;
			tTepBefore.tepMngs = ptCd->ptTepBefore->tepMngs;
			if (!bMIdChanged) {
				FRMWRK_STRARRNCPY (tTepBefore.tepMId.SerienNrGrp, ptCd->ptTepBefore->tepMId.SerienNrGrp);
			}
		
			/* No trigger needed because mng dose not change */
			iRv = TExecStdSql(pvTid, StdNupdate, TN_TEP, &tTepBefore);
			
			if (iRv != 1) {
				LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
					pvTid, TN_TEP, &tTepBefore, NULL);
				return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
					LoggingSIf_GetLogPk (pvTid, TN_TEP, &tTepBefore),
			    	TSqlErrTxt (pvTid)));
			}

			/* update Mngs and serial numbers (if not updated with changed MID) */
			/* put to the end, otherwise ZAP is not correct, if MID changes too */
			if (FRMWRK_DOUBLECMP (ptCd->ptTepNow->tepMngs.Mng, !=, ptCd->ptTepBefore->tepMngs.Mng)
					|| FRMWRK_DOUBLECMP (ptCd->ptTepNow->tepMngs.Gew, !=, ptCd->ptTepBefore->tepMngs.Gew)
					|| strcmp(ptCd->ptTepNow->tepMId.SerienNrGrp, ptCd->ptTepBefore->tepMId.SerienNrGrp) != 0) {
				tTepNow = *ptCd->ptTepNow;
				tTepBefore = *ptCd->ptTepNow;
				tTepBefore.tepMngs = ptCd->ptTepBefore->tepMngs;
				if (!bMIdChanged) {
					FRMWRK_STRARRNCPY (tTepBefore.tepMId.SerienNrGrp, ptCd->ptTepBefore->tepMId.SerienNrGrp);
				}

				iRv = WriteMngs_tebearb (pvTid, pcFac,
										 &ptCd->tTepInfo,
										 &tTepNow, &tTepBefore, &ptCd->tKretk);
				OPMSG_CFR(WriteMngs_tebearb);
			}
		}
		break;
	
	case TEP_DELETE:
		
		/* retoure */
		if (ptCd->lKorrTypRead & TEBEARB_WeRetoure) {
			
			iRv = WriteTepAndInv_tebearb (pvTid, pcFac,
										eReason, &ptCd->tTepInfo,
										ptCd->ptTekNow, ptCd->ptTekBefore,
										ptCd->ptTepNow, ptCd->ptTepBefore,
										&ptCd->tKretk);
            OPMSG_CFR(WriteTepAndInv_tebearb);
		} else {
			
			tTepNow = *ptCd->ptTepBefore;

			// If there is a group of serials on the TEP clear it in tTepNow so that serial removal is sent to the host
			if (_StockbaseSIf_IsInternalSerial(pvTid, pcFac, tTepNow.tepMId.SerienNrGrp)) {
				memset(tTepNow.tepMId.SerienNrGrp, 0, sizeof(tTepNow.tepMId.SerienNrGrp));
			}

			memset(&tTepNow.tepMngs, 0, sizeof(MNGS));

			iRv = WriteMngs_tebearb (pvTid, pcFac,
									 &ptCd->tTepInfo,
									 &tTepNow, ptCd->ptTepBefore, &ptCd->tKretk);
			OPMSG_CFR(WriteMngs_tebearb);
		}
			
		break;
	
	case TE_COMPLETE:
		iRv = WriteTeComplete_tebearb (pvTid, pcFac, ptCd->ptTekNow);
		OPMSG_CFR(WriteTeComplete_tebearb);

		break;
		
	default:
		break;
	}
	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 	write WE protocol
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbWe3_TeWalk (const void* pvTid, char const *pcFac, eReasonTeWalk eReason,
						CDTEWALK *ptCd)
{
	int				iRv;
	GivgTWeProtInfo	tGiWeProtInfo;
	ArrayPtr		ptArrTeData;
	
	switch (eReason) {
	case TEP_UPDATE:
		break;
	
	case TEP_DELETE:
		break;
	
    case TE_COMPLETE:

        // for supplier returns no gi-protocol exists
        if ((ptCd->lKorrTypRead & TEBEARB_WeRetoure) == 0) {
            /**
             * the goodsin protocol is written after all updates of the
             * TEP / TEK records
             * the function makes a copy of the actual state of the db and must
             * be called after all other db transactions */
            memset(&tGiWeProtInfo, 0, sizeof(tGiWeProtInfo));
            strncpy(tGiWeProtInfo.acTuId, ptCd->ptTekNow->tekTeId, TEID_LEN);
            tGiWeProtInfo.acTuId[TEID_LEN] = '\0';
            tGiWeProtInfo.ptWevp = &ptCd->tTepInfo.tPrivate.tWevp;
            tGiWeProtInfo.eClearPos = JANEIN_N;

            iRv = GivgSIf_UpdateWeProt4Korr(pvTid, pcFac, &tGiWeProtInfo,
                                            JANEIN_N);
            OPMSG_CFR(GivgSIf_UpdateWeProt4Korr);
        }

		break;
		
    case TEK_DELETE:
        if ((ptCd->lKorrTypRead & TEBEARB_WeRetoure) == 0) {
            /**
             * the goodsin protocol is written after all updates of the
             * TEP / TEK records
             * as the tek record is only marked for deletion, call the function
             * with force flag */
            memset(&tGiWeProtInfo, 0, sizeof(tGiWeProtInfo));
            strncpy(tGiWeProtInfo.acTuId, ptCd->ptTekNow->tekTeId, TEID_LEN);
            tGiWeProtInfo.acTuId[TEID_LEN] = '\0';
            tGiWeProtInfo.ptWevp = &ptCd->tTepInfo.tPrivate.tWevp;
            tGiWeProtInfo.eClearPos = JANEIN_N;

            iRv = GivgSIf_UpdateWeProt4Korr(pvTid, pcFac, &tGiWeProtInfo,
                                            JANEIN_J);
            OPMSG_CFR(GivgSIf_UpdateWeProt4Korr);
        }

		break;
			
	case TEP_INSERT:
		break;
	case TEWALK_INIT:
		ptArrTeData = ArrCreate(sizeof(GihostmuxTGiTeData), 0,
								(ArrTcbSort) NULL, (void *) NULL);
		if (ptArrTeData == NULL) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
								"Error in ArrCreate()!");
			return OpmsgSIf_ErrPush (GeneralTerrMemory, NULL);
		}
			
		/* read data for sending differences between TEK/TEP and P_WETEK/TEP
		   to host */
		iRv = GihostmuxSIf_GiTuReadData(pvTid, pcFac, ptCd->ptTekNow->tekTeId,
										&ptCd->ptWevBearb->tWevId, NULL,
										ptArrTeData);
		if (iRv < 0) {
			ptArrTeData = ArrDestroy(ptArrTeData);
			OPMSG_CFR(GihostmuxSIf_GiTuReadData);
		}

		if (iRv == 0) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
					"no data to send to host");
		}
		
		/* send differences to host */
		if (iRv > 0) {
			iRv = GihostmuxSIf_GiTuMux(pvTid, pcFac, ptArrTeData);
			if (iRv < 0) {
				ptArrTeData = ArrDestroy(ptArrTeData);
				OPMSG_CFR(GihostmuxSIf_GiTuMux);
			}
		}

		/* destroy array */
		ptArrTeData = ArrDestroy(ptArrTeData);
		break;
			
	default:
		break;
	}
	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbStd_TeWalk (const void* pvTid, char const *pcFac, eReasonTeWalk eReason,
						CDTEWALK *ptCd)
{
	int		iRv;
	
	switch (eReason) {
	
	case TEK_INSERT:

		iRv = InsertTek (pvTid, pcFac, ptCd);
		OPMSG_CFR(InsertTek);

		break;
		
	case TEK_UPDATE:

		iRv = UpdateTek (pvTid, pcFac, ptCd);
		OPMSG_CFR(UpdateTek);
		// force TEP update if FachtId is deleted from TEK
		if (IsEmptyStrg (ptCd->ptTekNow->tekFTT_FachtId) && 
				!IsEmptyStrg (ptCd->ptTekBefore->tekFTT_FachtId))
		{
			ptCd->iForceStdTepUpd = 1;
		}
		break;
		
	case TEK_DELETE:
		break;
	
	case TEP_UPDATE:
		/* fall through */
	
	case TEP_INSERT:
		/* fall through */
		
	case TEP_NONE:
		/* fall through */
	
	case TEP_DELETE:
		
		if (eReason == TEP_INSERT ||
			eReason == TEP_DELETE ||
			memcmp (&ptCd->ptTepNow->tepMId.AId,
				    &ptCd->ptTepBefore->tepMId.AId, sizeof(AID)) != 0 ||
			memcmp (&ptCd->ptTepNow->tepMngs,
					&ptCd->ptTepBefore->tepMngs, sizeof(MNGS)) != 0) {
			
			if (ptCd->ptTepNow->tepPrimThmKz == JANEIN_N) {
				iRv = AccodeSIf_CheckIfInv(pvTid, pcFac, ptCd->tTepInfo.acBschl);
				OPMSG_CFR(AccodeSIf_CheckIfInv);
				if (iRv == 1) {
					ptCd->ptTepNow->tepInvZeit = time ((time_t *)0);
				}
			}
		}

		if (eReason != TEP_DELETE) {
			// reset tepFachK if it's a THM or FachtId is empty 
			if ((ptCd->ptTepNow->tepMId.ThmKz == JANEIN_J && 
					!IsEmptyStrg (ptCd->ptTepNow->tepFachK)) ||
						IsEmptyStrg (ptCd->ptTekNow->tekFTT_FachtId)) 
			{
				memset (ptCd->ptTepNow->tepFachK, 0, sizeof (ptCd->ptTepNow->tepFachK));
			}
			// some checks if tekFachtId is not empty and has changed
			if (!IsEmptyStrg (ptCd->ptTekNow->tekFTT_FachtId) && 
					ptCd->ptTepNow->tepMId.ThmKz == JANEIN_N &&
					strcmp (ptCd->ptTekNow->tekFTT_FachtId, 
							ptCd->ptTekBefore->tekFTT_FachtId) != 0)
			{
				// is FachK entered
				if (IsEmptyStrg (ptCd->ptTepNow->tepFachK)) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							"no tepFachK maintained for FTT: "
							"'%s', TEP PosNr: '%ld'", 
							ptCd->ptTekNow->tekFTT_FachtId, ptCd->ptTepNow->tepPosNr);
					return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
							MLM("Keine Fachkennung bei Position '%ld' für verwendeten "
									"Fachtyp '%s' eingetragen."), 
									ptCd->ptTepNow->tepPosNr, ptCd->ptTekNow->tekFTT_FachtId);
				}
				// for secure
				int nI;
				for (nI = 0; nI < (int)strlen (ptCd->ptTepNow->tepFachK); nI++) {
					if (!isdigit ((int)ptCd->ptTepNow->tepFachK[nI])) {
						LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								"tepFachK not numeric");
						return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
								MLM("Fachkennung bei Position '%ld' ist nicht numerisch"),
								ptCd->ptTepNow->tepPosNr);		
					}
				} // for
				// check if values in TEP are allowed because of MaxAnzFach
				if (ptCd->tFtt.fttAnzFach < atol(ptCd->ptTepNow->tepFachK)) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							"tepFachK is bigger than maintained value for FTT: "
							"'%s' -> allowed: '%ld' -> entered: '%ld', TEP PosNr: '%ld'", 
							ptCd->ptTekNow->tekFTT_FachtId, ptCd->tFtt.fttAnzFach, 
							atol(ptCd->ptTepNow->tepFachK), ptCd->ptTepNow->tepPosNr);
					return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
							MLM("Eingetragene Fachkennung '%ld' bei Position '%ld' ist "
									"für Fachtyp '%s' zu groß.\nMaximal erlaubt: '%ld'"), 
									atol(ptCd->ptTepNow->tepFachK), ptCd->ptTepNow->tepPosNr, 
									ptCd->ptTekNow->tekFTT_FachtId, ptCd->tFtt.fttAnzFach);
				}
			}
		}

		if (eReason != TEP_NONE) {
			iRv = WriteTepAndInv_tebearb (pvTid, pcFac,
										eReason, &ptCd->tTepInfo,
										ptCd->ptTekNow, ptCd->ptTekBefore,
										ptCd->ptTepNow, ptCd->ptTepBefore,
										&ptCd->tKretk);
			OPMSG_CFR(WriteTepAndInv_tebearb);
		}
		break;
	
	case TE_COMPLETE:
		iRv = WriteTeComplete_tebearb (pvTid, pcFac, ptCd->ptTekNow);
		OPMSG_CFR(WriteTeComplete_tebearb);
		break;
	
	default:
		break;
	}
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbWa_TeWalk (const void* pvTid, char const *pcFac, eReasonTeWalk eReason,
						CDTEWALK *ptCd)
{
	MNGS	tMngs;
	int		iRv;
	
	switch (eReason) {
	
	case TEP_UPDATE:
		
		iRv = GovplbaseSIf_ChangeVplMngByTEP (pvTid, pcFac,
											  ptCd->ptTepNow->tepTeId,
											  ptCd->ptTepNow->tepPosNr,
											  &ptCd->ptTepNow->tepMngs,
											  &ptCd->ptTepBefore->tepMngs,
											  NULL);
		if (iRv < 0 && iRv != GeneralTerrIntNoImp) {	
			OPMSG_CFR(GovplbaseSIf_ChangeVplMngByTEP);
		}
		
		break;
	
	case TEP_DELETE:
		memset (&tMngs, 0, sizeof(tMngs));
		iRv = GovplbaseSIf_ChangeVplMngByTEP (pvTid, pcFac,
											  ptCd->ptTepNow->tepTeId,
											  ptCd->ptTepNow->tepPosNr,
											  &tMngs,
											  &ptCd->ptTepBefore->tepMngs,
											  NULL);
		if (iRv < 0 && iRv != GeneralTerrIntNoImp) {
			OPMSG_CFR(GovplbaseSIf_ChangeVplMngByTEP);
		}
		break;
	
	case TEK_INSERT:
		ptCd->pkcErrTxt = MlM("Anlegen einer TE bei "
						 "Existenz eines Auslagerauftrags nicht möglich");
		return (OpmsgSIf_ErrPush (GeneralTerrGeneral, NULL));
			
	case TEP_INSERT:
		ptCd->pkcErrTxt = MlM("Anlegen von Positionen bei "
						 "Existenz eines Auslagerauftrags nicht möglich");
		return (OpmsgSIf_ErrPush (GeneralTerrGeneral, NULL));
	default:
		break;
	}
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		check if at least one tep on tek for update or insert TE
-*		check TPA for delete TEK
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbCeckAndInit_TeWalk (const void* pvTid, char const *pcFac,
								  eReasonTeWalk eReason,
								  CDTEWALK *ptCd)
{	
	int				iRv;
	TPA				tTpa;
	AUSID			tAusId;
	GivgTWevBearb 	tWevBearb;
	
	switch (eReason) {
	case TEWALK_INIT:

		memset (&tAusId, 0, sizeof (tAusId));
		
		/* check WA */
		if (ptCd->lAction != ACTION_INSERT) {
			iRv = GovplbaseSIf_IsAuslTe (pvTid, pcFac,
										 ptCd->ptTekNow->tekTeId, &tAusId);
	
			if (iRv == GeneralTerrIntNoImp) {
				return 0;
			}
			OPMSG_CFR(GovplbaseSIf_IsAuslTe);
		
			if ((IsEmptyStrg(ptCd->tAusId.AusNr) == 0 &&
				 IsEmptyStrg(tAusId.AusNr) == 0 &&
				 memcmp(&ptCd->tAusId, &tAusId, sizeof(tAusId))) ||
				(iRv == 0 && (ptCd->lKorrTypRead & TEBEARB_Wa) != 0) ||
				(iRv == 1 && (ptCd->lKorrTypRead & TEBEARB_Wa) == 0)) {
				
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral, 
							MLM("WA wurde verändert, Daten neu lesen")));
			}
		}
		
		/* check TEBEARB_WeRetoure/TEBEARB_Std */
		if (ptCd->lKorrTyp & TEBEARB_Std) {
			
			memset (&tWevBearb, 0, sizeof(tWevBearb));
			FRMWRK_STRARRNCPY (tWevBearb.acTeId, ptCd->ptTekNow->tekTeId);
		
			iRv = GivgSIf_CheckWeModebyTeId (pvTid, pcFac, &tWevBearb);
            OPMSG_CFR(GivgSIf_CheckWeModebyTeId);
			if (iRv == 1 &&
				tWevBearb.eMode == JANEIN_J &&
				tWevBearb.ePosTyp == GI_WEVPTYP_RETOURE) {
			
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("TE-Korrektur Standard nicht zulässig, "
									 "TE ist Lieferantenretoure")));
			}
		}
		
		/* get TPA */
		memset (&tTpa, 0, sizeof(TPA));
		iRv = TpobaseSIf_GetTpaByTeId (pvTid, pcFac,
									   ptCd->ptTekNow->tekTeId, &tTpa);
        OPMSG_CFR(TpobaseSIf_GetTpaByTeId);
		if (iRv == 1) {
			ptCd->tTpa = tTpa;
			ptCd->iTpa = 1;
		}
		
		break;
	
	case TEK_DELETE:
		
		/* check TPA */
		if (ptCd->iTpa == 1) {
			return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("TE löschen nicht möglich, "
								 "Transportauftrag existiert")));
		}
		break;
	
	case TEK_INSERT:
		iRv = FrmwrkSIf_SetHist(pcFac, FrmwrkTSetHistTyp_Insert, TN_TEK, ptCd->ptTekNow);
		OPMSG_CFR(FrmwrkSIf_SetHist);
		
		ptCd->ptTekNow->tekEinZeit = time ((time_t *)0);
		
		ptCd->lFlags |= TEBEARB_TEPFOUND;
		
		/* check TeId */
		iRv = CheckAndWriteTeId_tebearb (pvTid, pcFac, eReason,
										 ptCd->ptTekNow);
		OPMSG_CFR(CheckAndWriteTeId_tebearb);
		break;
	
	case TEK_UPDATE:
		
		ptCd->lFlags |= TEBEARB_TEPFOUND;
		
		/* check TeId */
		iRv = CheckAndWriteTeId_tebearb (pvTid, pcFac, eReason,
										 ptCd->ptTekNow);
		OPMSG_CFR(CheckAndWriteTeId_tebearb);
		
		/* check TPA */
		if (ptCd->iTpa == 1) {
			
			if (ptCd->lKorrTyp & TEBEARB_Std) {
				iRv = IfcnvyblSIf_IsCheckingStation(pvTid, pcFac, ptCd->ptTekNow->tekPos);
				OPMSG_CFR(IfcnvyblSIf_IsCheckingStation);
				if (iRv == 0) {
					// We would allow changing the TU only at a Convey checking station when it has got a TPO!
					return (OpmsgSIf_ErrPush(GeneralTerrGeneral,
							MLM("Korrektur nicht möglich, Transportauftrag existiert")));
				}
			}
			
			if (ptCd->tTpa.tpaStatus == TPASTATUS_AKTIV) {
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral,
						MLM("Korrektur nicht möglich, Transportauftrag im Status Aktiv existiert")));
			}

			iRv = SlobaseSIf_PosCmp (pcFac, &ptCd->ptTekBefore->tekPos,
									 &ptCd->ptTekNow->tekPos);
			OPMSG_CFR(SlobaseSIf_PosCmp);
		
			if (iRv == 1 ||
				strcmp (ptCd->ptTekBefore->tekTTS_TetId,
						ptCd->ptTekNow->tekTTS_TetId)) {
	
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("Position oder TE-Typ ändern nicht "
									 "möglich, Transportauftrag existiert")));
			}

			if(strcmp(ptCd->ptTekBefore->tekEin_LagRf,
					  ptCd->ptTekNow->tekEin_LagRf) ||
			   strcmp(ptCd->ptTekBefore->tekCombStoreId,
			          ptCd->ptTekNow->tekCombStoreId)) {
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral, MLM("Einlagerreihenfolge oder "
																 "Zusammenlagerungskennung ändern nicht "
									 	 	 	 	 	 	 	 "möglich, Transportauftrag existiert")));
			}
		}
		
		iRv = FrmwrkSIf_SetHist(pcFac, FrmwrkTSetHistTyp_Update, TN_TEK, ptCd->ptTekNow);
		OPMSG_CFR(FrmwrkSIf_SetHist);	
		
		break;
	
	case TEP_UPDATE:
		/* WE 'normal' */
		if (ptCd->iTpa == 1 &&
			(ptCd->lAction == ACTION_UPDATE) &&
			(ptCd->lKorrTyp & TEBEARB_We) &&
			(ptCd->lKorrTypRead & TEBEARB_WeRetoure)) {
			
			iRv = CheckTpa4We_tebearb (pvTid, pcFac, &ptCd->tTpa, 
									   &ptCd->ptWevBearb->tSollPos);
            OPMSG_CFR(CheckTpa4We_tebearb);
		}

		/* Standard Korrektur */
		if (ptCd->iTpa == 1 &&
			ptCd->lAction == ACTION_UPDATE &&
			ptCd->lKorrTyp & TEBEARB_Std) {
			// if TPA exists, check if TPAP exists for TEP which should be updated
			// if TPAP exists quantity change is not allowed
			iRv = CheckTpap_tebearb(pvTid, pcFac, &ptCd->tTpa, ptCd->ptTepBefore, ptCd->ptTepNow);
            OPMSG_CFR(CheckTpap_tebearb);
		}
		
		iRv = FrmwrkSIf_SetHist(pcFac, FrmwrkTSetHistTyp_Update, TN_TEP, ptCd->ptTepNow);
		OPMSG_CFR(FrmwrkSIf_SetHist);	
		
		ptCd->lFlags &= (~TEBEARB_TEPFOUND);
		break;
	
	case TEP_DELETE:
		/* WE 'normal' */
		if (ptCd->iTpa == 1 &&
			(ptCd->lAction == ACTION_UPDATE) &&
			(ptCd->lKorrTyp & TEBEARB_We) &&
			(ptCd->lKorrTypRead & TEBEARB_WeRetoure)) {
			
			iRv = CheckTpa4We_tebearb (pvTid, pcFac, &ptCd->tTpa, 
									   &ptCd->ptWevBearb->tSollPos);
            OPMSG_CFR(CheckTpa4We_tebearb);
		}
		break;
		
	case TEP_INSERT:
		iRv = FrmwrkSIf_SetHist(pcFac, FrmwrkTSetHistTyp_Insert, TN_TEP, ptCd->ptTepNow);
		OPMSG_CFR(FrmwrkSIf_SetHist);
		
		ptCd->ptTepNow->tepErstErfZeit = time ((time_t *)0);
		
		ptCd->lFlags &= (~TEBEARB_TEPFOUND);
		break;
	
	case TEP_NONE:
		ptCd->lFlags &= (~TEBEARB_TEPFOUND);
		break;
	
	default:
		break;
	}
	
	return (0);
}

static int TepWork (const void *pvTid, const char *pcFac,
		UNICONNECTRL *ucr, CDTEWALK &tCd, f_CbTeWalk_tebearb cbTeWalk,
		TTEWALK *ptTeWalk, char const **ppkcErrTxt,
		LBCElement *lel)
{
	int iRv = 0;
	TEP tTep = {};

	/* get TEP */
	long *plEntered = (long *)UniConnectLBCElemGetEntered(ucr, lel);
	long *plDeleted = (long *)UniConnectLBCElemGetDeleted(ucr, lel);

	tCd.ptTepNow = (TEP *)UniConnectLBCElemGetRecNowByIdx (ucr, 0, lel);
	tCd.ptTepBefore = (TEP *)UniConnectLBCElemGetRecBeforeByIdx (ucr, 0, lel);

	if (tCd.ptTepNow == NULL || tCd.ptTepBefore == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				"UniConnectLBCElemGetRec...ByIdx failed");
		return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
	}

	if (tCd.lAction == ACTION_UPDATE) {
		if (plDeleted && plEntered &&
				memcmp (tCd.ptTepBefore, &tTep, sizeof(TEP)) == 0) {
			return 1;
		}
	}

	if (tCd.lAction == ACTION_INSERT) {
		if (plDeleted) {
			return 1;
		}
	}

	if (tCd.lAction == ACTION_DELETE) {
		if (plEntered &&
				memcmp (tCd.ptTepBefore, &tTep, sizeof(TEP)) == 0) {
			return 1;
		}
	}

	/* TEP_NONE */
	if (tCd.lAction == ACTION_UPDATE &&
			plEntered == NULL && plDeleted == NULL && tCd.iForceStdTepUpd == 0) {

		iRv = cbTeWalk (pvTid, pcFac, TEP_NONE, &tCd);
		if (iRv < 0) {
			*ppkcErrTxt = tCd.pkcErrTxt;
			ptTeWalk->lFlags = tCd.lFlags;
			OPMSG_CFR(cbTeWalk);
		}
		return 1;
	}

	/* get TepInfo */
	memset (&tCd.tTepInfo, 0, sizeof (StockbaseTChangeTepInfo));
	tCd.tTepInfo.lPosNr = tCd.ptTepNow->tepPosNr;

	StockbaseTChangeTepInfo	*ptTepInfo =
			(StockbaseTChangeTepInfo*)ArrGetFindElem (ptTeWalk->ptMcTek->hArrTepInfo, &tCd.tTepInfo);

	if (ptTeWalk->ptMcTek->iInfoDelTek) {
		tCd.tTepInfo = ptTeWalk->ptMcTek->tInfoDelTek;
		tCd.tTepInfo.lPosNr = tCd.ptTepNow->tepPosNr;
		if (ptTepInfo) {
			tCd.tTepInfo.tPrivate = ptTepInfo->tPrivate;
		}
	} else if (ptTepInfo) {
		tCd.tTepInfo = *ptTepInfo;
	}


	/* TEP_DELETE */
	if (tCd.lAction == ACTION_DELETE ||
			plDeleted) {

		iRv = cbTeWalk (pvTid, pcFac, TEP_DELETE, &tCd);

		/* TEP_INSERT */
	} else if (tCd.lAction == ACTION_INSERT ||
			(plEntered &&
					memcmp (tCd.ptTepBefore, &tTep, sizeof(TEP)) == 0)) {

		iRv = cbTeWalk (pvTid, pcFac, TEP_INSERT, &tCd);

		/* TEP_UPDATE */
	} else {

		iRv = cbTeWalk (pvTid, pcFac, TEP_UPDATE, &tCd);
	}

	if (iRv < 0) {
		*ppkcErrTxt = tCd.pkcErrTxt;
		ptTeWalk->lFlags = tCd.lFlags;
		OPMSG_CFR(cbTeWalk);
	}

	if (ptTepInfo) {
		ptTepInfo->tPrivate = tCd.tTepInfo.tPrivate;
	}
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int TeWalk_tebearb (const void* pvTid, UNITABLERL *ptUtr,
						   char const **ppkcErrTxt, TTEWALK *ptTeWalk,
						   f_CbTeWalk_tebearb cbTeWalk,
						   OpmsgTCbStackPtr stackInPtr)
{
	int             	iRv, nI, nNoEle;
	TEP					*ptTepNow;
	UNICONNECTRL        *ucr;
	LBCBuffer           *lbc;
    LBCElement          *lel;
	CDTEWALK			tCd;
    char const *pcFac;
	MskDialog      		hMaskRl;
	V_GDART				tGdArt;

	/* Unimenu */
    hMaskRl  = UniTableGetMaskRl(ptUtr);
    pcFac = lUniGetFacility (hMaskRl);

	/* Walk */
	memset(&tCd, 0, sizeof(tCd));
	tCd.tAusId = ptTeWalk->ptMcTek->tAusk.auskAusId;
	tCd.ptWevBearb = &ptTeWalk->ptMcTek->tWevBearb;
	tCd.lKorrTypRead = ptTeWalk->ptMcTek->lKorrTypRead;
	tCd.lKorrTyp = ptTeWalk->ptMcTek->lKorrTyp;
	tCd.lFlags = ptTeWalk->lFlags;
	tCd.tKretk =  ptTeWalk->ptMcTek->tKretk;
	tCd.lStapelHoehe =  INT_MAX;
	tCd.stackPtr =  stackInPtr;
	tCd.hMaskRl = hMaskRl;
	tCd.hArrTepInfo = ptTeWalk->ptMcTek->hArrTepInfo;

	/* loop TEP */
	ucr = UniTableGetConnectRlArray(ptUtr);
    lbc = UniConnectGetLBCByIdx(ucr, 0);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);

	for (nI = 0; nI < nNoEle; nI++) {

		lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);

		/* get TEP */
		long *plDeleted = (long *)UniConnectLBCElemGetDeleted(ucr, lel);
		
		if (plDeleted) {
			continue;
		}
		
		ptTepNow = (TEP *)UniConnectLBCElemGetRecNowByIdx (ucr, 0, lel);
		
		if (ptTepNow == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					  "UniConnectLBCElemGetRec...ByIdx failed");
			return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
		}
		
		memset (&tGdArt, 0, sizeof (V_GDART));

		iRv = ArticleSIf_GetArtByAId_GD (hMaskRl, pcFac,
											 &ptTepNow->tepMId.AId,
											 &tGdArt);
		OPMSG_CFR(ArticleSIf_GetArtByAId_GD);

		if (ptTepNow->tepMId.ThmKz == JANEIN_N) {
			if (tCd.lStapelHoehe > tGdArt.gdartStapelHoehe) {
				tCd.lStapelHoehe = tGdArt.gdartStapelHoehe;
			}
		}

		if (!ptTeWalk->isCbBefore) {

			std::string newSerienNrGrp;
			iRv = _StockbaseSIf_HandleSerials(pvTid, pcFac, ptTepNow->tepMId.SerienNrGrp, newSerienNrGrp,
					(*(ptTeWalk->ptMcTek->tepSerials))[ptTepNow->tepPosNr]);
			OPMSG_CFR(_StockbaseSIf_HandleSerials);

			if (strncmp(ptTepNow->tepMId.SerienNrGrp, newSerienNrGrp.c_str(), SERIENNR_NR_LEN) != 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
						"SerienNrGrpIp changed from '%s' to '%s'",
						ptTepNow->tepMId.SerienNrGrp,
						newSerienNrGrp.c_str());

				FRMWRK_STRARRNCPY(ptTepNow->tepMId.SerienNrGrp, newSerienNrGrp.c_str());
			}
		}
	}

	if (tCd.lStapelHoehe >=  INT_MAX) {
		tCd.lStapelHoehe = 0;
	}

	/* get TEK */
	tCd.ptTekNow = (TEK *)UniTableGetRecNow (ptUtr);
	tCd.ptTekBefore = (TEK *)UniTableGetRecBefore (ptUtr);

	if (tCd.ptTekNow == NULL || tCd.ptTekBefore == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"UniTableGetRecNow or UniTableGetRecBeforefailed");
		return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
	}

	/* get action */
	if (ptTeWalk->lAction == ACTION_OK) {
		ptTeWalk->lAction = (strcmp(tCd.ptTekNow->tekTeId,
									tCd.ptTekBefore->tekTeId) == 0) ?
									ACTION_UPDATE :
									ACTION_INSERT;
	}
	tCd.lAction = ptTeWalk->lAction;

	// get max allowed AnzFach
	if (!IsEmptyStrg (tCd.ptTekNow->tekFTT_FachtId)) {
		iRv = getFtt (pvTid, pcFac, tCd.ptTekNow->tekFTT_FachtId, &tCd.tFtt);
		OPMSG_CFR (getFtt);
	}

	/* TEWALK_INIT */
	iRv = cbTeWalk (pvTid, pcFac, TEWALK_INIT, &tCd);
	if (iRv < 0) {
		*ppkcErrTxt = tCd.pkcErrTxt;
		ptTeWalk->lFlags = tCd.lFlags;
		OPMSG_CFR(cbTeWalk);
	}

	/* TEK_INSERT, TEK_UPDATE */
	switch (tCd.lAction) {

	case ACTION_INSERT:
		iRv = cbTeWalk (pvTid, pcFac, TEK_INSERT, &tCd);
		break;

	case ACTION_UPDATE:
		iRv = cbTeWalk (pvTid, pcFac, TEK_UPDATE, &tCd);
		break;

	default:
		iRv = 0;
		break;
	}
	if (iRv < 0) {
		*ppkcErrTxt = tCd.pkcErrTxt;
		ptTeWalk->lFlags = tCd.lFlags;
		OPMSG_CFR(cbTeWalk);
	}

	/* update position for hierachical tus */
	iRv = SlobaseSIf_PosCmp(pcFac, &tCd.ptTekNow->tekPos, &tCd.ptTekBefore->tekPos);
	OPMSG_CFR (SlobaseSIf_PosCmp);
	if (iRv != 0) {
		iRv = StockbaseSIf_SyncPosWithParentTu (pvTid, pcFac, tCd.ptTekNow->tekTeId);
		OPMSG_CFR(StockbaseSIf_SyncPosWithParentTu);
		
		/* if the location of a goodsout TU was changed TOUR, TOURP, AUSP and AUSK must be handled */
		if (tCd.lAction == ACTION_UPDATE && tCd.lKorrTypRead & TEBEARB_Wa) {
			IV_TEK parentTek = {};
			iRv = StockbaseSIf_GetTuOrParentTu(pvTid, pcFac, tCd.ptTekNow->tekTeId, parentTek);
			OPMSG_CFR (StockbaseSIf_GetTuOrParentTu);
			
			iRv = GotourbaseSIf_ReadAndHandleTourTourpByTeId(pvTid, pcFac, parentTek.ivtekTeId);
			OPMSG_CFR (GotourbaseSIf_ReadAndHandleTourTourpByTeId);
			
			iRv = GowabaseSIf_ReadAndHandleAusForVplp(pvTid, pcFac, parentTek.ivtekTeId);
			OPMSG_CFR (GowabaseSIf_ReadAndHandleAusForVplp);
			
			iRv = GowabaseSIf_ReadAndHandleAusForGPal(pvTid, pcFac, parentTek.ivtekTeId);
			OPMSG_CFR (GowabaseSIf_ReadAndHandleAusForGPal);
		}
	}

	/* loop TEP */
	ucr = UniTableGetConnectRlArray(ptUtr);
    lbc = UniConnectGetLBCByIdx(ucr, 0);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);

    // work for all TEPs != DELETE
	for (nI = 0; nI < nNoEle; nI++) {
		lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);

		long *plDeleted = (long *)UniConnectLBCElemGetDeleted(ucr, lel);
		if (plDeleted) {
			continue;
		}
		iRv = TepWork(pvTid, pcFac, ucr, tCd, cbTeWalk, ptTeWalk, ppkcErrTxt, lel);
		OPMSG_CFR(TepWork);
	}
	
    // work for all TEPs = DELETE
	for (nI = 0; nI < nNoEle; nI++) {
		lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);

		long *plDeleted = (long *)UniConnectLBCElemGetDeleted(ucr, lel);
		if (!plDeleted) {
			continue;
		}
		iRv = TepWork(pvTid, pcFac, ucr, tCd, cbTeWalk, ptTeWalk, ppkcErrTxt, lel);
		OPMSG_CFR(TepWork);
	}

	/* TEK_DELETE */
	if (tCd.lAction == ACTION_DELETE) {
		iRv = cbTeWalk (pvTid, pcFac, TEK_DELETE, &tCd);
		
		if (iRv < 0) {
			*ppkcErrTxt = tCd.pkcErrTxt;
			ptTeWalk->lFlags = tCd.lFlags;
			OPMSG_CFR(cbTeWalk);
		}
	}
	
	/* TE_COMPLETE */
	if (tCd.lAction != ACTION_DELETE) {
		iRv = cbTeWalk (pvTid, pcFac, TE_COMPLETE, &tCd);
		if (iRv < 0) {
			*ppkcErrTxt = tCd.pkcErrTxt;
			ptTeWalk->lFlags = tCd.lFlags;
			OPMSG_CFR(cbTeWalk);
		}
	}
	
	/* TEWALK_FINISH */
	iRv = cbTeWalk (pvTid, pcFac, TEWALK_FINISH, &tCd);
	if (iRv < 0) {
		*ppkcErrTxt = tCd.pkcErrTxt;
		ptTeWalk->lFlags = tCd.lFlags;
		OPMSG_CFR(cbTeWalk);
	}
	
	*ppkcErrTxt = tCd.pkcErrTxt;
	ptTeWalk->lFlags = tCd.lFlags;
	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CallTeWalks_tebearb (const void* pvTid, UNIMENUINFO *ptUmi,
								char const **ppkcErrTxt,
								OpmsgTCbStackPtr stackPtr)
{
	int					iRv;
	char const *pcFac;
	TTEWALK 			tTeWalk;
	
	long				lAction = ACTION_NONE;
    ContainerAtom   	hAtom;
	MskDialog      		hMaskRl;
	UNITABLERL			*ptUtr;
	
	
	memset (&tTeWalk, 0, sizeof(tTeWalk));

	/* Unimenu */
    ptUtr = UniMenuGetRootTableRl(ptUmi->umiMenuRl);
    hMaskRl  = UniTableGetMaskRl(ptUtr);
    pcFac = lUniGetFacility (hMaskRl);
	
	
	/* atom Action */
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprAction);
	
	if (hAtom) {
		lAction = (long)hAtom->client_data;
	}
	if (lAction == ACTION_NONE) {
		return (0);
	}
	
	tTeWalk.lAction = lAction;
	
	/* atom MaskContext */
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);

    if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
		
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							 "MskAtomGet failed: ATOM "ATOM_UniAprMyContext);
		return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
    }
	
    tTeWalk.ptMcTek = (MyTcontextTek*)hAtom->client_data;
	
	/* check and complete data for all modes */
	iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
						  (f_CbTeWalk_tebearb) CbCeckAndInit_TeWalk, stackPtr);
	OPMSG_CFR(TeWalk_tebearb);
	
 	/* check and write Wa */
	if (tTeWalk.ptMcTek->lKorrTypRead & TEBEARB_Wa) {
		
		iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
							  (f_CbTeWalk_tebearb) CbWa_TeWalk, stackPtr);
		OPMSG_CFR(TeWalk_tebearb);
	}
	
	/* check and write Std */
	if (tTeWalk.ptMcTek->lKorrTyp & TEBEARB_Std) {
			
		iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
							  (f_CbTeWalk_tebearb) CbStd_TeWalk, stackPtr);
		OPMSG_CFR(TeWalk_tebearb);
		
	/* check and write We */
	} else if (tTeWalk.ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
		/* 	check and write WE-Korr */
		iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
							  (f_CbTeWalk_tebearb) CbWe1_TeWalk, stackPtr);
		OPMSG_CFR(TeWalk_tebearb);
		/* 	write Tep, ZAP */
		iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
							  (f_CbTeWalk_tebearb) CbWe2_TeWalk, stackPtr);
		OPMSG_CFR(TeWalk_tebearb);
		/* update we protocol P_WETEK, P_WETEP */
		iRv = TeWalk_tebearb (pvTid, ptUtr, ppkcErrTxt, &tTeWalk,
							  (f_CbTeWalk_tebearb) CbWe3_TeWalk, stackPtr);
		OPMSG_CFR(TeWalk_tebearb);
	}
	
	/* comlete GesGew, GesVol, OrigTeKz
	   is done in walk CbWe2_TeWalk, CbStd_TeWalk */
	
	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int MsgKomz_tebearb (const void* pvTid, UNIMENUINFO *ptUmi)
{
	int					iRv;
    char const *pcFac;
	MskDialog      		hMaskRl;
	UNITABLERL			*ptUtr;
	
	TEK					*ptTekNow;
	TEP					tTep;
	MyTcontextTek		*ptMcTek;
	
	long				lAction = ACTION_NONE;
    ContainerAtom   	hAtom;
	
	/* Unimenu */
    ptUtr = UniMenuGetRootTableRl(ptUmi->umiMenuRl);
    hMaskRl  = UniTableGetMaskRl(ptUtr);
    pcFac = lUniGetFacility (hMaskRl);
	
	/* atom MaskContext */
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);

    if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
		
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							 "MskAtomGet failed: ATOM "ATOM_UniAprMyContext);
		return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
    }
	
    ptMcTek = (MyTcontextTek*)hAtom->client_data;
	
	/* get TEK */
	ptTekNow = (TEK *)UniTableGetRecNow (ptUtr);
	
	if (ptTekNow == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"UniTableGetRecNow or UniTableGetRecBeforefailed");
		return (OpmsgSIf_ErrPush(GeneralTerrInternal, NULL));
	}
	
	/* atom Action */
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprAction);
	
	if (hAtom) {
		lAction = (long)hAtom->client_data;
	}
	
	if (lAction != ACTION_DELETE) {
		return (0);
	}
	
	iRv = TExecSql (pvTid,
					" SELECT %TEP "
					" FROM TEP "
                    " WHERE TEP.TEID = :teid ",
					SQLSTRING (ptTekNow->tekTeId),
					SELSTRUCT (TN_TEP, tTep),
					NULL);
	
	TSqlRollback(pvTid);
	
    if (iRv <= 0) {
		if (TSqlError (pvTid) != SqlNotFound) {
			LoggingSIf_SqlErrLogPrintf (pcFac, LOGGING_SIF_ALERT, pvTid,
                                     "_StockbaseSIf_CheckTuOnKomPos");
			return (OpmsgSIf_ErrPush(GeneralTerrDb, NULL));
		
		} else {
			return (0);
		}
	}
	
	FRMWRK_STRARRNCPY (ptMcTek->acTeId, ptTekNow->tekTeId);
	
	WamasBox (SHELL_OF (hMaskRl),
			  WboxNboxType,   	WBOX_INFO,
			  WboxNbutton,    	WboxbNok,
			  WboxNmwmTitle,  	MlM ("Korrektur"),
			  WboxNtext,      	
			  MlMsg ("TE-Position wurde nicht gelöscht."
                    "\nMenge wurde auf 0 gesetzt."),
			  NULL);
	
	return (1);
}

static int _CreateTpaAndWriteMovementLog(const void *pvTid, UNIMENUINFO *ptUmi)
{
	int iRv = 0;

	UNITABLERL *ptUtr = UniMenuGetRootTableRl(ptUmi->umiMenuRl);
	MskDialog hMaskRl  = UniTableGetMaskRl(ptUtr);
	const char *pcFac = lUniGetFacility (hMaskRl);

	/* atom Action */
	long lAction = ACTION_NONE;
	ContainerAtom hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprAction);
	if (hAtom) {
		lAction = (long)hAtom->client_data;
	}

	if (lAction != ACTION_UPDATE) {
		return 0;
	}

	/* get old and new TEK */
	TEK *ptTekBefore = (TEK *)UniTableGetRecBefore (ptUtr);
	if (ptTekBefore == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "UniTableGetRecBefore failed");
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
	}

	TEK *ptTekNow = (TEK *)UniTableGetRecNow (ptUtr);
	if (ptTekNow == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "UniTableGetRecNow failed");
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
	}

	iRv = _StockbaseSIf_CreateTpaAndWriteMovementLog(pvTid, pcFac, ptTekNow->tekTeId,
			ptTekBefore->tekPos, ptTekNow->tekPos);
	OPMSG_CFR (_StockbaseSIf_CreateTpaAndWriteMovementLog);

	return 1;
}

static int _CancelTourpIfUnused_tebearb (void *pvTid, UNIMENUINFO *ptUmi)
{
	int iRv = 0;
	
	/* Unimenu */
    UNITABLERL *ptUtr = UniMenuGetRootTableRl(ptUmi->umiMenuRl);
    MskDialog hMaskRl  = UniTableGetMaskRl(ptUtr);
    const char *pcFac = lUniGetFacility (hMaskRl);
	
	/* atom MaskContext */
	ContainerAtom hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
    if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "MskAtomGet failed: ATOM "ATOM_UniAprMyContext);
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
    }
	
	/* get TEK */
	TEK *ptTekNow = (TEK *)UniTableGetRecNow (ptUtr);
	if (ptTekNow == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "UniTableGetRecNow or UniTableGetRecBeforefailed");
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
	}
	
	/* atom Action */
	long lAction = ACTION_NONE;
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprAction);
	if (hAtom) {
		lAction = (long)hAtom->client_data;
	}
	
	if (lAction != ACTION_DELETE) {
		return 0;
	}
	
	/* Check if there is a TOURP for this TEK */
	TOURP tTourp = {};
	iRv = GotourbaseSIf_GetTourpByTeId (pvTid, pcFac, ptTekNow->tekTeId, &tTourp);
	if (iRv == GeneralTerrIntNoImp) {
		OpmsgSIf_ErrResetMsg();
		iRv = 0;
	} else {
		OPMSG_CFR(GotourbaseSIf_GetTourpByTeId);
	}
	if (iRv == 0) {
		// No TOURP exists for this TEK
		return 0;
	}

	/* Cancel the TOURP if it's not used anymore */
	iRv = GotourbaseSIf_CheckForUnusedTourp (pvTid, pcFac, tTourp.tourpTourpNr);
	if (iRv == GeneralTerrIntNoImp) {
		OpmsgSIf_ErrResetMsg();
		iRv = 0;
	} else {
		OPMSG_CFR(GotourbaseSIf_CheckForUnusedTourp);
	}
	if (iRv == 1) {
		iRv = GovplbaseSIf_GebindeStorno (pvTid, pcFac, tTourp.tourpTEK_TeId, FrmwrkSIf_GetUser(),
				JANEIN_J/*eStornoTpa*/,
				JANEIN_J/*eStornoAa*/,
				JANEIN_N/*eChangeAuspMngs*/,
				JANEIN_N/*eForce*/);
		if (iRv == GeneralTerrIntNoImp) {
			OpmsgSIf_ErrResetMsg();
		} else {
			OPMSG_CFR(GovplbaseSIf_GebindeStorno);
		}

		// remove THM TEPs -> Empty TEK will be deleted by GdDelSto
		iRv = _StockbaseSIf_DissolveTu (pvTid, pcFac, tTourp.tourpTEK_TeId, NULL);
		OPMSG_CFR(_StockbaseSIf_DissolveTu);
	}
	
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UniTableTSqlCommit_tebearb (void* pvTid, UNIMENUINFO *ptUmi)
{
	int					iRv = 0, iRvHnd = 0;
	char	const		*pkcErrTxt = NULL;
	MskDialog			hMaskRl;
	char const			*pcFac;
	UNITABLERL			*ptUtr;
    OpmsgTCbStackPtr	stackPtr = NULL;

	/* Unimenu */
    ptUtr = UniMenuGetRootTableRl(ptUmi->umiMenuRl);
    hMaskRl  = UniTableGetMaskRl(ptUtr);
    pcFac = lUniGetFacility (hMaskRl);
    iRv = OpmsgSIf_CbCreate(pcFac, &stackPtr);

    if (iRv < 0) {
		OPMSG_SFF(OpmsgSIf_CbCreate);
		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), NULL);
		SetError (SUB_IGNORE);
		return iRv;
    }

	while (true) {
		iRv = CallTeWalks_tebearb (pvTid, ptUmi, &pkcErrTxt, stackPtr);
			
		if (iRv < 0) {
			TSqlRollback(hMaskRl);

			iRvHnd = OpmsgGIf_HandleError(hMaskRl, pcFac, 
							stackPtr, iRv,
							MlM("Transporteinheit bilden"));

			if (iRvHnd > 0) {
				/* error has been handled - try again */
				continue;
			} else if (iRvHnd == 0) {
				iRv = GeneralTerrHasAlreadyBeenHandled;
				break;
			} else {
				OPMSG_SFF(CallTeWalks_tebearb);
				OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), pkcErrTxt);
				SetError (SUB_IGNORE);
				break;
			}
		}
		break;
	}
	
    // cleanup
    OpmsgSIf_CbDestroy(&stackPtr);

	if (iRv < 0) {
		if (iRv == GeneralTerrUserAbort) {
			// SetError to suppress uni error box
			SetError (SUB_IGNORE);
		}
		return iRv;
	}
	
	iRv = _CreateTpaAndWriteMovementLog(pvTid, ptUmi);
	if (iRv < 0) {
		OPMSG_SFF(_CreateTpaAndWriteMovementLog);
		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), NULL);
		SetError (SUB_IGNORE);
		return iRv;
	}

	iRv = _CancelTourpIfUnused_tebearb (pvTid, ptUmi);
    if (iRv < 0) {
		OPMSG_SFF(_CancelTourpIfUnused_tebearb);
		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), NULL);
		SetError (SUB_IGNORE);
		return iRv;
    }
	
	TSqlCommit(pvTid);
	TrgutilSIf_ExecuteTriggers(pcFac);
	
	iRv = MsgKomz_tebearb (pvTid, ptUmi);
    if (iRv < 0) {
		OPMSG_SFF(MsgKomz_tebearb);
		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE-Korrektur"), NULL);
		SetError (SUB_IGNORE);
		return iRv;
    }

	return (iRv);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int ReadWe_TEBearb (const void* pvTid, char const *pcFac,
						   TEK *ptTek, MyTcontextTek *ptMcTek,
						   UNITABLEINFO *ptUti,
						   char const **ppkcErrTxt)
{
	int				iRv;
	long			lSkipPositionCheck;
	GivgTWevBearb 	tWevBearb;
	UNITABLE		*ut = UniTableGetTable (ptUti->utiTableRl);
		
	*ppkcErrTxt = NULL;
	memset (&tWevBearb, 0, sizeof(tWevBearb));
	FRMWRK_STRARRNCPY (tWevBearb.acTeId, ptTek->tekTeId);
		
	iRv = GivgSIf_CheckWeModebyTeId (pvTid, pcFac, &tWevBearb);
	
	if (iRv == GivgTerrWeVorgangStatus) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"GI Correction in not possible (protocol)");
		*ppkcErrTxt=MlM("WE Korrektur nicht möglich (Protokollierung)");
		return (OpmsgSIf_ErrPush(GeneralTerrGeneral, NULL));
	
	} else if (iRv < 0) {
		OPMSG_CFR(GivgSIf_CheckWeModebyTeId);
	}
	
	if (iRv == 0) {
		if (ut->utCalldata && ((TeBearbCalldata *)ut->utCalldata)->eOrigCaller
                                                        == OriginalCaller_GI) {
			return (OpmsgSIf_ErrPush(GeneralTerrGeneral, NULL));
		}
		return (0);
	}
	
	/* WE exists */
	ptMcTek->tWevBearb = tWevBearb;
	ptMcTek->tWevk.wevkWevId = tWevBearb.tWevId;
	
	/* WE finished */
	if (tWevBearb.eMode == JANEIN_N) {
		ptMcTek->lKorrTypRead |= TEBEARB_WeNach;
		ptMcTek->tWevk.wevkStartZeit = tWevBearb.zAnliefzeit;
		
	/* WE retoure */
	} else if (tWevBearb.ePosTyp == GI_WEVPTYP_RETOURE) {
		ptMcTek->lKorrTypRead |= TEBEARB_WeRetoure;
	}
	
	/* check WE retoure or WE "normal" */	
	if ((ptMcTek->lKorrTypRead & TEBEARB_WeNach) == 0) {
        lSkipPositionCheck = JANEIN_N;
        // no position check for supplier returns
		if ((ptMcTek->lKorrTypRead & TEBEARB_WeRetoure) == 0) {
			iRv = ParamSIf_Get1Parameter(pvTid, pcFac,
										 PrmStockbaseTeKorCheckGiPos,
										  &lSkipPositionCheck);
            if (iRv == ParameterTerrNotFound) {
                OpmsgSIf_ErrResetMsg();
            } else {
                OPMSG_CFR(ParamSIf_Get1Parameter);
            }
		}
		
		if (lSkipPositionCheck != JANEIN_J) {
	
			iRv = SlobaseSIf_PosCmpFeldBez (pvTid, pcFac, &ptTek->tekPos,
									 &tWevBearb.tSollPos);
            OPMSG_CFR(SlobaseSIf_PosCmpFeldBez);
			if (iRv == 1) {
				*ppkcErrTxt=MlM("WE Korrektur nicht möglich, "
							   "TE steht nicht auf Bereitstellfläche");
				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"GI Correction in not possible (TeKorrCheckGiPos)");
				return (OpmsgSIf_ErrPush(GeneralTerrGeneral, NULL));
			}
		}
	}
	iRv=GivgSIf_IsTuProtConsistentency(pvTid, pcFac, ptTek->tekTeId);
	if (iRv == GivgTErrTuChangedSinceAquisition) {
        LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
            "No WE Correction available! ");
        OpmsgSIf_ErrResetMsg();
        return 1;
	}
	OPMSG_CFR(GivgSIf_IsTuProtConsistentency);
	ptMcTek->lKorrTypRead |= TEBEARB_We;
	return (1);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int ReadWa_TEBearb (const void* pvTid, char const *pcFac,
						   TEK *ptTek, MyTcontextTek *ptMcTek,
						   char const **ppkcErrTxt)
{
	int				iRv;
	AUSID			tAusId;
	
	
	*ppkcErrTxt = NULL;
	memset(&tAusId, 0, sizeof(tAusId));
		
	iRv = GovplbaseSIf_IsAuslTe (pvTid, pcFac, ptTek->tekTeId, &tAusId);
	
	if (iRv == 0 || iRv == GeneralTerrIntNoImp) {
		return (0);
	}
	if (iRv < 0) {
		*ppkcErrTxt = MlM("WA lesen fehlgeschlagen");
		OPMSG_CFR(GovplbaseSIf_IsAuslTe);
	}
	
	/* WA exists */
	ptMcTek->tAusk.auskAusId = tAusId;
	ptMcTek->lKorrTypRead |= TEBEARB_Wa;
	
	return (1);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int lUniTExecStdSql_tebearb(void* pvTid,
                            sqlNstmtRes iResource, char const *pacDbName,
                            void *pvDat, UNITABLEINFO *ptUti)
{
    int         	iRv = 0, iRvDb = 1;
	char const *pcFac = NULL;
	char const 		*pkcErrTxt;
    ContainerAtom  	hAtom;
    MyTcontextTek  	*ptMcTek;
    MskDialog   	hMaskRl = UniTableGetMaskRl(ptUti->utiTableRl);
	TEK				*ptTek = NULL, *ptOldTek = NULL;
	UNITABLE		*ut = UniTableGetTable (ptUti->utiTableRl);
	std::string		oldCombStoreId, newCombStoreId;
	
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
	if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
		return (-1);
	}
	ptMcTek = (MyTcontextTek*)hAtom->client_data;
	
	switch (iResource) {
    case StdNinsert:
		MskAtomAdd (hMaskRl, NULL, ATOM_UniAprAction, NULL,
					(Value)ACTION_INSERT);
        break;
		
    case StdNupdate:
		MskAtomAdd (hMaskRl, NULL, ATOM_UniAprAction, NULL,
					(Value)ACTION_UPDATE);
		pcFac = lUniGetFacility(hMaskRl);
		
		/*
		 * The TEK's CombStoreId can optionally be an MlM()'d string in the database,
		 * e.g. "@@MD5_1f7889dbb75cb89a900ee12c719e9e14:Misch-TE"
		 * The dialog translates it and displays only "Misch-TE" in the edit field.
		 * On update we have to check if the CombStoreId was changed by the user and
		 * only then we must update the CombStoreId.
		 */
		ptTek = (TEK*)pvDat;
		ptOldTek = (TEK*)UniTableGetRecBefore(ptUti->utiTableRl);
		
		oldCombStoreId = MlUnpack(ptOldTek->tekCombStoreId);
		newCombStoreId = MlUnpack(ptTek->tekCombStoreId);
		if (oldCombStoreId == newCombStoreId) {
			FRMWRK_STRARRNCPY (ptTek->tekCombStoreId, ptOldTek->tekCombStoreId);
		}
        break;

    case StdNdelete:
		MskAtomAdd (hMaskRl, NULL, ATOM_UniAprAction, NULL,
					(Value)ACTION_DELETE);
		break;
	
    case StdNselect:		/* FALLTHROUGH */
    case StdNselectNext:	/* FALLTHROUGH */
    case StdNselectPrev:	/* FALLTHROUGH */
    case StdNselectFirst:	/* FALLTHROUGH */
    case StdNselectLast:
		MskAtomAdd (hMaskRl, NULL, ATOM_UniAprAction, NULL,
					(Value)ACTION_NONE);
		
		ptMcTek->iRead = 0;
		ptMcTek->lKorrTypRead = 0;
		memset(&ptMcTek->tWevk, 0, sizeof(WEVK));
		memset(&ptMcTek->tAusk, 0, sizeof(AUSK));
		memset(&ptMcTek->tWevBearb, 0, sizeof(ptMcTek->tWevBearb));
		memset (ptMcTek->acBschl, 0, sizeof(ptMcTek->acBschl));
		memset (ptMcTek->acKostSt, 0, sizeof(ptMcTek->acKostSt));
		memset (ptMcTek->acMand, 0, sizeof(ptMcTek->acMand));
		memset (ptMcTek->acLiefNr, 0, sizeof(ptMcTek->acLiefNr));
		memset (ptMcTek->acNeutralId, 0, sizeof(ptMcTek->acNeutralId));
		ptMcTek->lNeutralInvNr = 0;
		ptMcTek->lNeutralPosNr = 0;
		ArrUnload(ptMcTek->hArrTepInfo);
		
		iRvDb = TExecStdSql(pvTid, iResource, pacDbName, pvDat);
		
		if (iRvDb != 1) {
			ptMcTek->lKorrTypRead = TEBEARB_Std;
			break;
		}
		
		ptMcTek->iModTep = 0;
		ptMcTek->iRead = 1;
		
		ptTek = (TEK *) pvDat;
		pcFac = lUniGetFacility(hMaskRl);
		OpmsgSIf_ErrResetMsg();
		
		/* read WE */
		pkcErrTxt = NULL;
		
		iRv = ReadWe_TEBearb (pvTid, pcFac, ptTek, ptMcTek, ptUti, &pkcErrTxt);
		
		if (iRv < 0 &&
			ut->utCalldata && ((TeBearbCalldata *)ut->utCalldata)->eOrigCaller
                                                        == OriginalCaller_GI) {
			TSqlRollback(pvTid);
			OPMSG_SFF(ReadWe_TEBearb);
			
			((TeBearbCalldata *)ut->utCalldata)->eOrigCaller =
                                                        OriginalCaller_Standard;
			ptMcTek->lKorrTypRead &= ~(TEBEARB_We |
									TEBEARB_WeNach |
									TEBEARB_WeRetoure);
			switch (iRv) {
			case GeneralTerrDb:
				pkcErrTxt = TSqlErrTxt(pvTid);
				break;
			case GeneralTerrGeneral:
				if (pkcErrTxt == NULL) {
					pkcErrTxt = MlM("WE lesen fehlgeschlagen");
				}
				break;
			default:				
				pkcErrTxt = OpmsgSIf_ErrGetErrMsg (iRv);
				break;
			}
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM ("Lesen"), pkcErrTxt);
			
		}
		
		/* read WA */
		pkcErrTxt = NULL;
		
		iRv = ReadWa_TEBearb (pvTid, pcFac, ptTek, ptMcTek, &pkcErrTxt);
		
		if (iRv < 0) {
			TSqlRollback(pvTid);
			OPMSG_SFF(ReadWa_TEBearb);
			
			switch (iRv) {
			case GeneralTerrDb:
				pkcErrTxt = TSqlErrTxt(pvTid);
				break;
			case GeneralTerrGeneral:
				if (pkcErrTxt == NULL) {
					pkcErrTxt = MlM("WA lesen fehlgeschlagen");
				}
				break;
			default:				
				pkcErrTxt = OpmsgSIf_ErrGetErrMsg (iRv);
				break;
			}
			
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM ("Lesen"), pkcErrTxt);
		}
		
		iRv = GetAnzSubTe_tebearb(pvTid, pcFac, ptTek, ptMcTek);
		if (iRv < 0) {
			TSqlRollback(pvTid);
			OPMSG_SFF (GetAnzSubTe_tebearb);
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM ("Lesen"), NULL);
		}

		break;
	
	default:
		MskAtomAdd (hMaskRl, NULL, ATOM_UniAprAction, NULL,
					(Value)ACTION_NONE);
		iRvDb = TExecStdSql(pvTid, iResource, pacDbName, pvDat);
		break;
	}
		
    return (iRvDb);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UniConnectTExecStdSqlX_tebearb(void* pvTid,
        SqlContext* hSqlContext, sqlNstmtRes eRes ,char const * pcTableName,
        void* pvData, int nBlocksize, char const * pcFilter, char const * pcOrderBy,
        UNICONNECTINFO* phUci)
{
    int					iRvDb = 0, iRv = 0, iI;
    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek;
	TEP					*ptTep;
	MskDialog			hMaskRl;
	UNITABLERL			*ptUtr;
	GivgTPrepare		tPrepare;
	char const *pcFac	= NULL;
	char 	const		*pkcErrTxt = NULL;
	StockbaseTChangeTepInfo	tTepInfo;

	switch(eRes) {
	case StdNinsert:
		/* FALLTHROUH */
	case StdNinsertCont:
		/* FALLTHROUH */
	case StdNupdate:
		/* FALLTHROUH */
	case StdNupdateCont:
		/* FALLTHROUH */
	case StdNdelete:
		/* FALLTHROUH */
	case StdNdeleteCont:
		iRvDb = nBlocksize;
		break;
	case StdNselect:
	case StdNselectCont:
		
		iRvDb = TExecStdSqlX(pvTid, hSqlContext, eRes, pcTableName,
						   pvData, nBlocksize, pcFilter, pcOrderBy);
		
		if (iRvDb < 0 && TSqlError(pvTid) != SqlNotFound) {
			return (iRvDb);
		}
		
		ptUtr = (UNITABLERL *)
				  UniConnectGetParentTableRl(phUci->uciConnectRl);
		hMaskRl = UniTableGetMaskRl(ptUtr);
		pcFac = lUniGetFacility (hMaskRl);
		
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
        if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTek = (MyTcontextTek*)hAtom->client_data;
        
        ptMcTek->tepSerials->clear();
        
        for (iI=0, ptTep = (TEP*)pvData; iI < iRvDb; iI++, ptTep++) {
        	iRv = _StockbaseSIf_GetSerialsByGroup(pvTid, pcFac, ptTep->tepMId.SerienNrGrp,
        			(*(ptMcTek->tepSerials))[ptTep->tepPosNr]);
        	if (iRv < 0) {
        		TSqlRollback(pvTid);
        		OPMSG_SFF (_StockbaseSIf_GetSerialsByGroup);
        		OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("TE Korrektur"), NULL);
        		ArrUnload(ptMcTek->hArrTepInfo);
        		ptMcTek->lKorrTypRead &= (~TEBEARB_We);
        		ptMcTek->lKorrTyp &= (~TEBEARB_We);
        		ptMcTek->lKorrTypRead &= (~TEBEARB_WeNach);
        		ptMcTek->lKorrTyp &= (~TEBEARB_WeNach);
        		return iRv;
        	}
        }
		
		if ((ptMcTek->lKorrTypRead & (TEBEARB_We | TEBEARB_WeNach)) == 0) {
			break;
		}

		for (iI=0, ptTep = (TEP *)pvData;
			 iI < iRvDb;
			 iI++, ptTep++) {
		
			memset(&tPrepare, 0, sizeof(tPrepare));
			tPrepare.eMode = ptMcTek->tWevBearb.eMode;
			tPrepare.ePosTyp = ptMcTek->tWevBearb.ePosTyp;
			tPrepare.tWevId = ptMcTek->tWevBearb.tWevId;
			FRMWRK_STRARRNCPY(tPrepare.acTeId, ptTep->tepTeId);
			tPrepare.lTepPosNr = ptTep->tepPosNr;
			tPrepare.tMId = ptTep->tepMId;
			
			iRv = GivgSIf_PrepareWevp4Korr (pvTid, pcFac, &tPrepare);
			
			if (iRv < 0) {
				TSqlRollback(pvTid);
				OPMSG_SFF(GivgSIf_PrepareWevp4Korr);

				switch (iRv) {
				case GivgTerrWeVorgangStatus:
					pkcErrTxt=MlM("WE Korrektur nicht möglich, "
						 "WE-Vorgangstyp ungleich Normal oder Retoure");
					break;
				case GeneralTerrDb:
				case GeneralTerrGeneral:
					pkcErrTxt = MlM("WE lesen fehlgeschlagen, "
								   "WE-Korrektur nicht möglich");
					break;
				default:
					pkcErrTxt = OpmsgSIf_ErrGetErrMsg (iRv);
					break;
				}
			
				OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM ("TE Korrektur"), pkcErrTxt);
				
				ArrUnload(ptMcTek->hArrTepInfo);
				ptMcTek->lKorrTypRead &= (~TEBEARB_We);
				ptMcTek->lKorrTyp &= (~TEBEARB_We);
				ptMcTek->lKorrTypRead &= (~TEBEARB_WeNach);
				ptMcTek->lKorrTyp &= (~TEBEARB_WeNach);
				return iRv;
			}
			
			memset (&tTepInfo, 0, sizeof (StockbaseTChangeTepInfo));
			tTepInfo.lPosNr = ptTep->tepPosNr;
			tTepInfo.tPrivate = tPrepare.tPrivate;
			
			iRv = ArrAddElem(ptMcTek->hArrTepInfo, &tTepInfo);
			
			if (iRv < 0) {
				TSqlRollback(pvTid);
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
									  "ArrAddElemArray failed");
				OpmsgGIf_ShowAlertBox(hMaskRl, GeneralTerrMemory, MlM ("TE Korrektur"), NULL);
				
				ArrUnload(ptMcTek->hArrTepInfo);
				ptMcTek->lKorrTypRead &= (~TEBEARB_We);
				ptMcTek->lKorrTyp &= (~TEBEARB_We);
				ptMcTek->lKorrTypRead &= (~TEBEARB_WeNach);
				ptMcTek->lKorrTyp &= (~TEBEARB_WeNach);
				return (-1);
			}
		}
		
		break;
			
    default:
        iRvDb = TExecStdSqlX(pvTid, hSqlContext, eRes, pcTableName,
                pvData, nBlocksize, pcFilter, pcOrderBy);
        break;
    }
    return (iRvDb);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 	Reads the KST for the AUSK.AusId
-* RETURNS
-*	0	OK
-*	<=	not OK
-*--------------------------------------------------------------------------*/
static int getCustomerByAusId (const void *pvTid, const char *pcFac, const AUSID& tAusId, KST& tKst)
{ 
	int iRv = TExecSql(pvTid,
				" SELECT "	"%"TN_KST
				" FROM "	TN_AUSK
				" JOIN "	TN_KST	" ON ( "	
								TCN_KST_Mand	" = "	TCN_AUSK_KST_Mand	" AND "
								TCN_KST_KuNr	" = "	TCN_AUSK_KST_KuNr	
							" ) "
				" WHERE "	TCN_AUSK_AusId_Mand		" = :Mand"
				" AND "		TCN_AUSK_AusId_AusNr		" = :AusNr"
				" AND "		TCN_AUSK_AusId_HostAusKz	" = :HostAusKz"
				" ",
				SELSTRUCT(TN_KST, tKst),
				SQLSTRING(tAusId.Mand),
				SQLSTRING(tAusId.AusNr),
				SQLSTRING(tAusId.HostAusKz),
				NULL);
	if (iRv != 1) {
		AUSK tAusk = {};
		tAusk.auskAusId = tAusId;
		LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid, "%s: Error getting KST for %s",
				__FUNCTION__, FwrefnrSIf_GetRefNr(TN_AUSK, &tAusk));
		return OpmsgSIf_ErrPush(GeneralTerrDb, NULL);
	}
	return 0;
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* 	Check if KST.UeberliefErl is 'J'
-* RETURNS
-*	IS_Ok	OK
-*	IS_No	not OK
-*	<0		Error
-*--------------------------------------------------------------------------*/
static int checkUeberliefOk (MskTmaskRlPtr hMaskRl, const char *pcFac, MyTcontextTek *ptMcTek)
{
	int iRv = 0;
	KST tKst = {};

	iRv = getCustomerByAusId(hMaskRl, pcFac, ptMcTek->tAusk.auskAusId, tKst);
	OPMSG_CFR(getCustomerByAusId)
	if (tKst.kstLokKstTyp.UeberliefErl == JANEIN_J) {
		iRv = IS_Ok;
	} else {
		iRv = IS_No;
	}	
	return iRv;
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CheckTEP_tebearb (MskDialog hMaskRl, char const *pcFac,
							 OpmsgTCbStackPtr stackPtr,
							 MyTcontextTek *ptMcTek,
							 MyTcontextTep *ptMcTep,
							 TEK *ptTekNow, TEK *ptTekBefore,
							 TEP *ptTepNow, TEP *ptTepBefore,
							 long lAction,
							 UNICONNECTRL *ucr,
    						 LBCBuffer *lbc)
{
	int 					iRv = 0;
	IV_ART					tIvArt = {};
	IV_ARTE					tIvArte = {};
	AccodeSifTCheckBschl 	tCheckBschl = {};

	/*TEP PosNr <= 0 is not allowed */

	if(ptTepNow->tepPosNr <= 0) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "INTERNAL ERROR: posnr == 0");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
	}
	/* WE exists */
	if (ptMcTek->lKorrTypRead & (TEBEARB_We | TEBEARB_WeNach) ||
			ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
			
		if (lAction == ACTION_INSERT) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                    "insertion of new position not allowed for gi-correction");
            return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                    MLM("Anlegen einer Position bei "
                                        "WE-Korrektur nicht möglich"));
		}
	}
	
	/* WE */
	if (ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
		
		if (lAction == ACTION_UPDATE) {
		
			/* ArtNr changed */
			if (memcmp(&ptTepNow->tepMId.AId,
					   &ptTepBefore->tepMId.AId, sizeof(AID))) {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_AId_ArtNr_t, 0);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                        "change of article not allowed for gi-correction");
                return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                        MLM("Ändern des Artikels bei "
                                            "WE-Korrektur nicht möglich"));
			}
			
			/* MatQ changed */
			if (strcmp(ptTepNow->tepMId.MatQ,
					   ptTepBefore->tepMId.MatQ)) {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_MatQ_t, 0);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                                     "change of material qualification not "
                                     "allowed for gi-correction");
                return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                        MLM("Ändern der Materialqualifikation "
                                            "bei WE-Korrektur nicht möglich"));
			}
		}
	}
	
	/* WA exists */
	if (ptMcTek->lKorrTypRead & TEBEARB_Wa) {
			
		if (lAction == ACTION_INSERT) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                    "insertion of new position not allowed for go-correction");
            return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                    MLM("Anlegen einer Position bei "
                                        "WA-Korrektur nicht möglich"));
		}
		
		if (lAction == ACTION_UPDATE) {
			/* if Now > Before */
			if (CMPDOUBLE(ptTepNow->tepMngs.Mng, ptTepBefore->tepMngs.Mng) > 0 &&
					CMPDOUBLE(ptTepNow->tepMngs.Gew, ptTepBefore->tepMngs.Gew) > 0) {
				iRv = checkUeberliefOk(hMaskRl, pcFac, ptMcTek);
				OPMSG_CFR(checkUeberliefOk)
				if (iRv == IS_No) {
					FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							"increasing the quantity and weight is not allowed for "
							"go-correction");
					return OpmsgSIf_ErrPush(StockbaseTerrGoAmountWeightIncreased,
							MLM("Erhöhen der Menge und des Gewichtes ist bei WA-Korrektur nicht möglich"));
				} else {

					int iRvQuestion = OpmsgSIf_CbQuestion (pcFac, stackPtr,
							StockbaseTerrGoAmountWeightIncreased,
							MlM("Übernehmen"),
							MLM("Überliefern der Menge und des Gewichtes Ok?"));
					if (iRvQuestion == 1) {
						LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
								"%s: TEP [%s/%ld] - user changed qty and weight for go",
								__FUNCTION__, ptTepNow->tepTeId, ptTepNow->tepPosNr);
						OpmsgSIf_ErrResetMsg();
					} else {
						FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
						return OpmsgSIf_ErrPush(StockbaseTerrGoAmountWeightIncreased,
								MLM("Erhöhen der Menge und des Gewichtes ist bei WA-Korrektur nicht möglich"));
					}
				}
			}

			if (CMPDOUBLE(ptTepNow->tepMngs.Mng, ptTepBefore->tepMngs.Mng) > 0 &&
					CMPDOUBLE(ptTepNow->tepMngs.Gew, ptTepBefore->tepMngs.Gew) == 0) {
				iRv = checkUeberliefOk(hMaskRl, pcFac, ptMcTek);	
				OPMSG_CFR(checkUeberliefOk)
				if (iRv == IS_No) { 
					FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							"increasing the quantity is not allowed for "
							"go-correction");
					return OpmsgSIf_ErrPush(StockbaseTerrGoAmountIncreased,
							MLM("Erhöhen der Menge ist bei WA-Korrektur nicht möglich"));
				} else {

					int iRvQuestion = OpmsgSIf_CbQuestion (pcFac, stackPtr,
							StockbaseTerrGoAmountIncreased,
							MlM("Übernehmen"),
							MLM("Überliefern der Menge Ok?"));
					if (iRvQuestion == 1) {
						LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
								"%s: TEP [%s/%ld] - user changed qty for go",
								__FUNCTION__, ptTepNow->tepTeId, ptTepNow->tepPosNr);
						OpmsgSIf_ErrResetMsg();
					} else {
						FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
						return OpmsgSIf_ErrPush(StockbaseTerrGoAmountIncreased,
								MLM("Erhöhen der Menge ist bei WA-Korrektur nicht möglich"));
					}
				}
			}
			
			if (CMPDOUBLE(ptTepNow->tepMngs.Mng, ptTepBefore->tepMngs.Mng) == 0 &&
					CMPDOUBLE(ptTepNow->tepMngs.Gew, ptTepBefore->tepMngs.Gew) > 0) {
				iRv = checkUeberliefOk(hMaskRl, pcFac, ptMcTek);	
				OPMSG_CFR(checkUeberliefOk)
				if (iRv == IS_No) {
					FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Gew_t, 0);
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							"increasing the weight is not allowed for "
							"go-correction");
					return OpmsgSIf_ErrPush(StockbaseTerrGoWeightIncreased,
							MLM("Erhöhen des Gewichtes ist bei WA-Korrektur nicht möglich"));
				} else {

					int iRvQuestion = OpmsgSIf_CbQuestion (pcFac, stackPtr,
							StockbaseTerrGoWeightIncreased,
							MlM("Übernehmen"),
							MLM("Überliefern des Gewichtes Ok?"));
					if (iRvQuestion == 1) {
						LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
								"%s: TEP [%s/%ld] - user changed weight for go",
								__FUNCTION__, ptTepNow->tepTeId, ptTepNow->tepPosNr);
						OpmsgSIf_ErrResetMsg();
					} else {
						FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Gew_t, 0);
						return OpmsgSIf_ErrPush(StockbaseTerrGoWeightIncreased,
								MLM("Erhöhen des Gewichtes ist bei WA-Korrektur nicht möglich"));
					}
				}
			}
			
			int cmp1 = 0;
			iRv = compareDates(pcFac, ptTepNow->tepMId.MHD, ptTepBefore->tepMId.MHD, cmp1);
			OPMSG_CFR(compareDates);
			int cmp2 = 0;
			iRv = compareDates(pcFac, ptTepNow->tepFifoDatum, ptTepBefore->tepFifoDatum, cmp2);
			OPMSG_CFR(compareDates);

			/* others changed */
			if (memcmp(&ptTepNow->tepMId.AId, &ptTepBefore->tepMId.AId, sizeof(ptTepNow->tepMId.AId)) != 0
					|| strcmp(ptTepNow->tepMId.Charge, ptTepBefore->tepMId.Charge) != 0
					|| strcmp(ptTepNow->tepMId.MatQ, ptTepBefore->tepMId.MatQ) != 0
					|| strcmp(ptTepNow->tepMId.WeNr, ptTepBefore->tepMId.WeNr) != 0
					|| strcmp(ptTepNow->tepMId.ResNr, ptTepBefore->tepMId.ResNr) != 0
					|| strcmp(ptTepNow->tepMId.MatZId, ptTepBefore->tepMId.MatZId) != 0
					|| cmp1 != 0
					|| ptTepNow->tepMId.ThmKz != ptTepBefore->tepMId.ThmKz
					|| cmp2 != 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
						"go-correction: only change of amount or serial numbers allowed ");
				return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
						MLM("Bei der WA-Korrektur dürfen nur die Menge bzw. das Gewicht und "
							"die Seriennummern geändert werden."));
			}
		}
	}
	
	/* Standard */
	if (ptMcTek->lKorrTyp & TEBEARB_Std) {
		
		if (lAction != ACTION_DELETE) {
			
			/* Matq empty */
			if (IsEmptyStrg(ptTepNow->tepMId.MatQ)) {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_MatQ_t, 0);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                        "material qualification is empty");
                return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                MLM("Materialqualifikation nicht eingegeben"));
			}
			
			// reset tepFachK if it's a THM or FachtId is empty
			if ((ptTepNow->tepMId.ThmKz == JANEIN_J && 
					!IsEmptyStrg (ptTepNow->tepFachK)) ||
						IsEmptyStrg (ptTekNow->tekFTT_FachtId)) 
			{
				memset (ptTepNow->tepFachK, 0, sizeof (ptTepNow->tepFachK));
			}

			// tekFachtId, FachK is entered and tepFachK or tekFachtId has changed
			if (!IsEmptyStrg (ptTekNow->tekFTT_FachtId) && ptTepNow->tepMId.ThmKz == JANEIN_N && 
				(strcmp (ptTepNow->tepFachK, ptTepBefore->tepFachK) != 0 ||
				strcmp (ptTekNow->tekFTT_FachtId, ptTekBefore->tekFTT_FachtId) != 0)) 
			{
				// is FachK entered
				if (IsEmptyStrg (ptTepNow->tepFachK)) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							"no tepFachK maintained for FTT: '%s'", 
							ptTekNow->tekFTT_FachtId);
					return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
							MLM("Keine Fachkennung für verwendeten "
									"Fachtyp '%s' eingetragen."), 
									ptTekNow->tekFTT_FachtId);
				}
				// check characters for numeric values
				int nI;
				for (nI = 0; nI < (int)strlen (ptTepNow->tepFachK); nI++) {
					if (!isdigit ((int)ptTepNow->tepFachK[nI])) {
						LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								"tepFachK not numeric");
						return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
								MLM("Fachkennung ist nicht numerisch"));		
					}
				} // for

				FTT tFtt = {};
				iRv = getFtt (hMaskRl, pcFac, ptTekNow->tekFTT_FachtId, &tFtt);
				if (iRv < 0) {
					FrmwrkGIf_SetFocus2Field (hMaskRl, TEK_FTT_FachtId_t, 0);
					OPMSG_CFR (getFtt);
				}

				long lAnzFach = atol (ptTepNow->tepFachK);
				if (lAnzFach == 0) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							"tepFachK is '0' --> not allowed!");
					return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
							MLM("Eingetragene Fachkennung '0' ist nicht "
								"erlaubt!"));
				}

				if (lAnzFach > tFtt.fttAnzFach) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							"tepFachK is bigger than maintained value for FTT: "
							"'%s' -> allowed: '%d' -> entered: '%ld'", 
							ptTekNow->tekFTT_FachtId, iRv, lAnzFach);
					return OpmsgSIf_ErrPush ((int)GeneralTerrInvalidUserInput,
							MLM("Eingetragene Fachkennung '%ld' ist für Fachtyp '%s' "
									"zu groß.\nMaximal erlaubt '%ld'"), 
									lAnzFach, ptTekNow->tekFTT_FachtId, tFtt.fttAnzFach);
				}
			}
		}
	}

	/* get article */
	iRv = ArticleSIf_GetArtArtEByAId_IV(hMaskRl, pcFac, &ptTepNow->tepMId.AId, &tIvArt, &tIvArte);
	if (iRv < 0) {
		FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_AId_ArtNr_t, 0);
        OPMSG_CFR(ArticleSIf_GetArtArtEByAId_GD);
	}

	if (lAction != ACTION_DELETE) {
		// get type of correction and use it for all required fields
		StockbaseTSnrCaller	eCaller = StockbaseTSnrCaller_WH;
		if (ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach | TEBEARB_WeRetoure)) {
			eCaller = StockbaseTSnrCaller_GI;
		} else if (ptMcTek->lKorrTyp & TEBEARB_Wa) {
			eCaller = StockbaseTSnrCaller_GO;
		} else if (ptMcTek->lKorrTyp & TEBEARB_Std) {
			eCaller = StockbaseTSnrCaller_WH;
		}

		// check number of serials if amount of article or article changed
		if ((memcmp(&ptTepNow->tepMId.AId, &ptTepBefore->tepMId.AId, sizeof(AID))) ||
		 	(FRMWRK_DOUBLECMP(ptTepNow->tepMngs.Mng, !=, ptTepBefore->tepMngs.Mng))) {

			OpmsgTCbStackPtr	stackPtr = NULL;
			iRv = OpmsgSIf_CbCreate(pcFac, &stackPtr);
			OPMSG_CFR(OpmsgSIf_CbCreate);

			iRv = _StockbaseSIf_CheckNumberOfSerials(hMaskRl, pcFac, stackPtr,
						NULL, &tIvArt, &ptTepNow->tepMngs, eCaller, ptMcTep->serials->size());
			if (iRv < 0) {
				TSqlRollback(hMaskRl);

				int iRvHnd = OpmsgGIf_HandleError(hMaskRl, pcFac, stackPtr, iRv, MlM("Seriennummern"));
				OpmsgSIf_CbDestroy(&stackPtr);
				/* error has to be handled */ 
				if (iRvHnd == 0) {
					return GeneralTerrHasAlreadyBeenHandled;
				} else if (iRvHnd < 0) {
					FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 0);
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "Amount of SNr aren't correct");
					return iRv;
				}
				/* error has been handled - try again */
			}
			OpmsgSIf_CbDestroy(&stackPtr);
		}

		/* Charge */
		if ((eCaller == StockbaseTSnrCaller_GI && tIvArt.ivartChargePflWe == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_GO && tIvArt.ivartChargePflWa == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_WH && tIvArt.ivartChargePflWh == JANEIN_J)) {

			if (IsEmptyStrg(ptTepNow->tepMId.Charge)) {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_Charge_t, 0);
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "batch required, but empty");
				return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
								MLM("Charge muss eingegeben werden"));
			}
		}
		
		/* MHD */
		if ((eCaller == StockbaseTSnrCaller_GI && tIvArt.ivartMHDPflWe == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_GO && tIvArt.ivartMHDPflWa == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_WH && tIvArt.ivartMHDPflWh == JANEIN_J)) {

			if (isDateEmpty(pcFac, ptTepNow->tepMId.MHD)) {
                FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_MId_MHD_t, 0);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "expiry date required, but empty");
                return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                MLM("MHD muss eingegeben werden"));
			}
		}

		/* FIFO */
		if ((eCaller == StockbaseTSnrCaller_GI && tIvArt.ivartMHDPflWe == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_GO && tIvArt.ivartMHDPflWa == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_WH && tIvArt.ivartMHDPflWh == JANEIN_J)) {

			if (isDateEmpty(pcFac, ptTepNow->tepFifoDatum)) {
                FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_FifoDatum_t, 0);
                LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "FIFO date required, but empty");
                return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                                MLM("Das FIFO-Datum muss eingegeben werden."));
			}
		}

		if (lAction == ACTION_INSERT &&
			((eCaller == StockbaseTSnrCaller_GI && tIvArt.ivartMHDPflWe == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_GO && tIvArt.ivartMHDPflWa == JANEIN_J) ||
		 	(eCaller == StockbaseTSnrCaller_WH && tIvArt.ivartMHDPflWh == JANEIN_J)) &&
			isDateEmpty(pcFac, ptTepNow->tepFifoDatum)) {

			today(ptTepNow->tepFifoDatum);
			int iRv = addDays(pcFac, ptTepNow->tepFifoDatum, tIvArt.ivartRestHaltTaWe);
			OPMSG_CFR(addDays);

			
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
				"[%s] [%s]: FIFO is empty --> set FIFO [%s]",
				FwrefnrSIf_GetRefNr(TN_TEP, ptTepNow),
				FwrefnrSIf_GetRefNr(ARTICLE_REFID_AID, &tIvArt.ivartAId),
				formatDate("%Y.%m.%d %H:%M:%S", &ptTepNow->tepFifoDatum).c_str());
		}
		
		/* Mngs < 0 */
		if (CMPDOUBLE(ptTepNow->tepMngs.Gew, 0.0) < 0 ||
			CMPDOUBLE(ptTepNow->tepMngs.Mng, 0.0) < 0) {
            FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                    "negative amount is invalid");
            return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                            MLM("Menge/Gewicht dürfen nicht kleiner 0 sein"));
		}
	
		/* Mngs == 0 */
		if (CMPDOUBLE(ptTepNow->tepMngs.Gew, 0.0) == 0 ||
			CMPDOUBLE(ptTepNow->tepMngs.Mng, 0.0) == 0) {
            FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                    "zero amount is invalid");
			
			int iTepCnt = 0;	
			for (int nI = 0; nI < LBCNumberOfElements(lbc, LbcSectBody);nI++) {
				LBCElement          *lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);

				int iDeleted = (int )UniConnectLBCElemGetDeleted(ucr, lel);
				if (iDeleted == 0) {
					iTepCnt++;
				}
			}

			if (iTepCnt > 1) {
				return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                            MLM("Korrektur von Menge/Gewicht auf 0 nur durch "
                                "Löschen der TE-Position möglich"));
			} else {
				return OpmsgSIf_ErrPush((int)GeneralTerrInvalidUserInput,
                            MLM("Korrektur von Menge/Gewicht auf 0 nur durch "
                                "Löschen der Transporteinheit möglich"));
			}
		}
	}
	
	/* Std-Korrektur or WE-Korrektur Retoure */
	if (! IsEmptyString(ptTekBefore->tekTeId)) {
		memset(&tCheckBschl, 0, sizeof(tCheckBschl));
		FRMWRK_STRARRNCPY (tCheckBschl.acBuSchl, ptMcTep->acBschl);
		
		if ((ptMcTek->lKorrTyp & TEBEARB_Std) ||
			((ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) &&
			 (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure))) {
		
			if (lAction == ACTION_DELETE ||			/* tep delete */
				lAction == ACTION_INSERT ||			/* tep insert */
				memcmp(&ptTepNow->tepMngs,
					   &ptTepBefore->tepMngs, sizeof(MNGS)) ||
				memcmp(&ptTepNow->tepMId.Charge,
					   &ptTepBefore->tepMId.Charge, 
					   sizeof(ptTepBefore->tepMId.Charge)) ||
				memcmp(&ptTepNow->tepMId.MHD,
					   &ptTepBefore->tepMId.MHD, 
					   sizeof(ptTepBefore->tepMId.MHD)) ||
				memcmp(&ptTepNow->tepMId.ResNr,
					   &ptTepBefore->tepMId.ResNr,
					   sizeof(ptTepBefore->tepMId.ResNr)) ||
				memcmp(&ptTepNow->tepMId.WeNr,
					   &ptTepBefore->tepMId.WeNr,
					   sizeof(ptTepBefore->tepMId.WeNr)) ||
				memcmp(&ptTepNow->tepMId.MatQ,
					   &ptTepBefore->tepMId.MatQ,
					   sizeof(ptTepBefore->tepMId.MatQ)) ||
				memcmp(&ptTepNow->tepMId.AId,
					   &ptTepBefore->tepMId.AId, sizeof(AID)) ||
				(*(ptMcTek->tepSerials))[ptTepNow->tepPosNr] != *(ptMcTep->serials)) {

				FRMWRK_STRARRNCPY (tCheckBschl.acKostSt, ptMcTep->acKostSt);
				tCheckBschl.lCriteria |= AccodeSifTCheckBschlKostSt;	
			}	
		}
		
		if (ptMcTek->lKorrTyp & TEBEARB_Std &&
	   		lAction == ACTION_INSERT) {
			
			FRMWRK_STRARRNCPY (tCheckBschl.acNeutralId, ptMcTep->acNeutralId);
			tCheckBschl.lNeutralInvNr = ptMcTep->lNeutralInvNr;
			tCheckBschl.lNeutralPosNr = ptMcTep->lNeutralPosNr;
		
			tCheckBschl.lCriteria |= AccodeSifTCheckBschlNeutral;
		}		
			
		if (tCheckBschl.lCriteria > 0 ) {	
			iRv = AccodeSIf_CheckBschlData(hMaskRl, pcFac,
										   &tCheckBschl);
			if (iRv < 0) {
				switch (iRv) {
				case AccodeTerrKostStPflJa:
					FrmwrkGIf_SetFocus2Field (hMaskRl,
											  "KOST_KostSte_t", 0);
					break;
				case AccodeTerrNeutralIdErf:
					FrmwrkGIf_SetFocus2Field (hMaskRl,
											  "INVBA_NeutralId_t", 0);
					break;
				case AccodeTerrInvNeutralInvNr:
					FrmwrkGIf_SetFocus2Field (hMaskRl,
											  "INVBA_Neutrale_InvNr_t", 0);
					break;
				case AccodeTerrInvNeutralPosNr:
					FrmwrkGIf_SetFocus2Field (hMaskRl,
											  "INVBA_Neutrale_PosNr_t", 0);
					break;
				case AccodeTerrBschlNoFound:
					/* FALLTHROUGH */
				case AccodeTerrBschlEmpty:	
					/* FALLTHROUGH */
				case AccodeTerrInvalidInv:
					/* FALLTHROUGH */
				case GeneralTerrDb:
					/* FALLTHROUGH */
				case GeneralTerrArgs:	
					/* FALLTHROUGH */
				default:
					FrmwrkGIf_SetFocus2Field(hMaskRl, BSCHL_BuSchl_t, 0);
					break;
				}

                OPMSG_CFR(AccodeSIf_CheckBschlData);
			}
		}	
	}
	
	if (lAction == ACTION_UPDATE) {
		/* amount changed but not weight */
		if (CMPDOUBLE(ptTepNow->tepMngs.Gew,
					  ptTepBefore->tepMngs.Gew) == 0 &&
			CMPDOUBLE(ptTepNow->tepMngs.Mng,
					  ptTepBefore->tepMngs.Mng) != 0) {

			int iRvQuestion = OpmsgSIf_CbQuestion (pcFac, stackPtr, 
								StockbaseTerrQtyChangedWithoutWeight,
								MlM("Übernehmen"),
								MLM("Menge wurde bei "
									"gleichbleibendem Gewicht geändert"));
			if (iRvQuestion == 1) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
					"%s: TEP [%s/%ld] - user changed qty, without weight",
					__FUNCTION__, ptTepNow->tepTeId, ptTepNow->tepPosNr); 

				OpmsgSIf_ErrResetMsg();
			} else {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Gew_t, 0);
				return OpmsgSIf_ErrPush(StockbaseTerrQtyChangedWithoutWeight,
							MLM("Menge wurde bei "
								"gleichbleibendem Gewicht geändert"));
			}
		}
		
		/* weight changed but not amount */
		if (CMPDOUBLE(ptTepNow->tepMngs.Gew,
					  ptTepBefore->tepMngs.Gew) != 0 &&
			CMPDOUBLE(ptTepNow->tepMngs.Mng,
					  ptTepBefore->tepMngs.Mng) == 0) {

			int iRvQuestion = OpmsgSIf_CbQuestion (pcFac, stackPtr, 
								StockbaseTerrWeightChangedWithoutQty,
								MlM("Übernehmen"),
								MLM("Gewicht wurde bei "
									"gleichbleibender Menge geändert"));
			if (iRvQuestion == 1) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
					"%s: TEP [%s/%ld] - user changed weight, without qty",
					__FUNCTION__, ptTepNow->tepTeId, ptTepNow->tepPosNr); 

				OpmsgSIf_ErrResetMsg();
			} else {
				FrmwrkGIf_SetFocus2Field (hMaskRl, TEP_Mngs_Mng_t, 1);
				return OpmsgSIf_ErrPush(StockbaseTerrWeightChangedWithoutQty,
							MLM("Gewicht wurde bei "
								"gleichbleibender Menge geändert"));
			}
		}
	}

	return 1;
}

int GetAnzSubTe_tebearb(const void *pvTid, const char *pcFac, const TEK *ptTek, MyTcontextTek *ptMcTek)
{
	ptMcTek->lAnzSubTe = 0;
	int rv = TExecSql(pvTid,
			" select count("TCN_TEK_TeId") from "TN_TEK
			" where "TCN_TEK_Stamm_TeId" = :TeId",
			SELLONG   (ptMcTek->lAnzSubTe),
			SQLSTRING (ptTek->tekTeId),
			NULL);
	if (rv != 1) {
		LoggingSIf_SqlErrLogPrintf(pcFac, LOGGING_SIF_ALERT, pvTid, "Error counting sub TUs");
		return OpmsgSIf_ErrPush(GeneralTerrDb, NULL);
	}
	
	return 1;
}

static int CbSetMixedTu_TeBearb(MskDialog hMaskRl, MskStatic hEf,
        MskElement hEfRl, int iReason, void *cbc, void *pvCd)
{
	UNITABLERL	*utr = NULL;
	UNITABLE	*ut = NULL;
	TEK			*ptTek = NULL;
	
	switch (iReason) {
	case FCB_XF:
		utr = (UNITABLERL*)MskDialogGet(hMaskRl, MskNmaskCalldata);
		ut = UniTableGetTable(utr);
		if (strcmp(ut->utName, TN_TEK) != 0) {
			break;
		}
		UniTransferMaskDup(hMaskRl, utr);
		ptTek = (TEK*)UniTableGetRecNow(utr);
		FRMWRK_STRARRNCPY (ptTek->tekCombStoreId, MlM(STOCKBASE_COMBSTOREID_MIXED_TU));
		UniUpdateMaskVarR(hMaskRl, utr);
		break;
	default:
		break;
	}
	
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UniTableCbTEK (UNITABLEINFO *ptUti)
{
    MskDialog       	hMaskRl;
	char const *pcFac = NULL;

    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek;
	UNITABLE			*ut;
    UNITABLERL  	*utr;

    hMaskRl = UniTableGetMaskRl(ptUti->utiTableRl);
	ut = UniTableGetTable(ptUti->utiTableRl);

	JANEIN  eNach = JANEIN_N;
	int iRv = ParamSIf_Get1Parameter(hMaskRl, pcFac, PrmTEKorrNachtraeg, &eNach);
	if (iRv < 0 && iRv != ParameterTerrNotFound) {
		OPMSG_CFR(ParamSIf_Get1Parameter);
	}

    switch(ptUti->utiReason) {
	case UNITABLEREASON_INITFIELDS:
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
        if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTek = (MyTcontextTek*)hAtom->client_data;
		
		ArrUnload(ptMcTek->hArrTepInfo);
		
		/* this reset is for 'clear fields' and other inits -
		 * iRead == 1 only right after successfull reading of TEK */
		
		if (ptMcTek->iRead == 0) {
			
			memset(&ptMcTek->tWevBearb, 0, sizeof(ptMcTek->tWevBearb));
			memset (&ptMcTek->tWevk, 0, sizeof(WEVK));
			memset (&ptMcTek->tAusk, 0, sizeof(AUSK));
			ptMcTek->lKorrTypRead = TEBEARB_Std;
			ptMcTek->lKorrTyp = TEBEARB_Std;
			ptMcTek->iModTep = 1;	/* no real effect */
			ptMcTek->lNeutralInvNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutraleInvNr;
			ptMcTek->lNeutralPosNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutralePosNr;
			
		} else {

            // TEK has just been read
			ptMcTek->lKorrTyp = 0;
			if (ptMcTek->lKorrTypRead & TEBEARB_Wa) {
				ptMcTek->lKorrTyp |= TEBEARB_Wa;
			}
			if ((ptMcTek->lKorrTypRead & TEBEARB_We) &&
				ut->utCalldata &&
					((TeBearbCalldata *)ut->utCalldata)->eOrigCaller ==
                                                    OriginalCaller_GI) {
				ptMcTek->lKorrTyp |= TEBEARB_We;
			}
				
			if ((ptMcTek->lKorrTypRead & TEBEARB_WeNach) &&
				ut->utCalldata &&
					((TeBearbCalldata *)ut->utCalldata)->eOrigCaller ==
                                                    OriginalCaller_GI) {
				ptMcTek->lKorrTyp |= TEBEARB_WeNach;
			}
			/* '=' ok here */
			if (ptMcTek->lKorrTypRead & TEBEARB_Std) {
				ptMcTek->lKorrTyp = TEBEARB_Std;
			}
			
			ptMcTek->lNeutralInvNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutraleInvNr;
			ptMcTek->lNeutralPosNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutralePosNr;
		}
		
		ptMcTek->iRead = 0;
		
		break;

	case UNITABLEREASON_UPDATE_MASK_VAR:
		pcFac = lUniGetFacility (hMaskRl);

		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
		ptMcTek = (MyTcontextTek*)hAtom->client_data;

		/* set inactve all CheckButtons */
		MskVaAssignMatch (hMaskRl,  "*-Check-*",
						  MskNattrOr,   (Value)EF_ATTR_INACTIVE,
						  NULL);
		
		/* set active the CheckButtons that may be used */
 		if (ptMcTek->iModTep == 0 &&
			(ptMcTek->lKorrTypRead & TEBEARB_Std) == 0) {
			
			/* Std : let inactive with WeRetoure */
			if ((ptMcTek->lKorrTypRead & (TEBEARB_We | TEBEARB_WeNach)) == 0 ||
				(ptMcTek->lKorrTypRead & TEBEARB_WeRetoure) == 0) {
				MskVaAssign (hMaskRl, MskGetElement ("CheckStd_TeBearb"),
							 MskNattrClr,   (Value)EF_ATTR_INACTIVE,
							 NULL);
			}
			
			/* We */
			if ((ptMcTek->lKorrTypRead & TEBEARB_We )) {
				MskVaAssign (hMaskRl, MskGetElement ("CheckWe_TeBearb"),
							 MskNattrClr,   (Value)EF_ATTR_INACTIVE,
							 NULL);
			}

			/* We (nachträglich) */
			if ((ptMcTek->lKorrTypRead & TEBEARB_WeNach )) {
				MskVaAssign (hMaskRl, MskGetElement ("CheckWeNach_TeBearb"),
							 MskNattrClr,   (Value)EF_ATTR_INACTIVE,
							 NULL);
			}
		}
		
		/* set buttons && efs active/inactive 							 */
		/* "-BtExec-" is Match for all possibly active Efs (TEBEARB_Std) */
		if (ptMcTek->lKorrTyp & (TEBEARB_Std | TEBEARB_We | TEBEARB_WeNach)) {
			MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
							  MskNattrClr,   (Value)EF_ATTR_INACTIVE,
							  NULL);
			MskVaAssignMatch (hMaskRl,  "*-EfExec-*",
							  MskNattrClr,   (Value)EF_ATTR_INACTIVE,
							  NULL);
			
			
			/* set inactive 'lock-fields' for We/Wa */
			if (ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
				MskVaAssignMatch (hMaskRl,  "*-NoWe-*",
								  MskNattrOr,  (Value)EF_ATTR_INACTIVE,
								  NULL);
			}
			
			if (ptMcTek->lKorrTypRead & TEBEARB_Wa) {
				MskVaAssignMatch (hMaskRl,  "*-NoWa-*",
								  MskNattrOr,  (Value)EF_ATTR_INACTIVE,
								  NULL);
			}
			
			MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
							  MskNupdate,   (Value)TRUE,
							  NULL);
			
			MskVaAssign(hMaskRl, MskGetElement("Btn_SetMixedTu_TeBearb"),
					MskNkey,		KEY_DEF,
					MskNattrClr,   	(Value)EF_ATTR_INACTIVE,
					MskNupdate,		(Value)TRUE,
					NULL);
			
		} else {
			/* set inactive everything */
			MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
							  MskNattrOr,   (Value)EF_ATTR_INACTIVE,
							  MskNupdate,   (Value)TRUE,
							  NULL);
			MskVaAssignMatch (hMaskRl,  "*-EfExec-*",
							  MskNattrOr,   (Value)EF_ATTR_INACTIVE,
							  NULL);
			
			MskVaAssign(hMaskRl, MskGetElement("Btn_SetMixedTu_TeBearb"),
					MskNkey,		KEY_DEF,
					MskNattrOr,   	(Value)EF_ATTR_INACTIVE,
					MskNupdate,		(Value)TRUE,
					NULL);
		}
		
		/* set additional efs visible/invisible 						 */
		/* "-Inv-...-" is Match 										 */
		if (ptMcTek->lKorrTypRead & TEBEARB_We) {
		
			MskVaAssign (hMaskRl, MskGetElement ("CheckWe_TeBearb"),
                             MskNattrClr,   (Value)EF_ATTR_INACTIVE,
                             NULL);
			
			MskVaAssign (hMaskRl, MskGetElement ("CheckWeNach_TeBearb"),
                             MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                             NULL);
		} 
		if (ptMcTek->lKorrTypRead & TEBEARB_WeNach) {
			
			MskVaAssign (hMaskRl, MskGetElement ("CheckWe_TeBearb"),
                             MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                             NULL);
			
			MskVaAssign (hMaskRl, MskGetElement ("CheckWeNach_TeBearb"),
                             MskNattrClr,   (Value)EF_ATTR_INACTIVE,
                             NULL);
		}
		MskVaAssign (hMaskRl,  MskGetElement("NeuPos_F"),
					 MskNkey,			(Value) 1,
					 MskNattrOr,		(Value) EF_ATTR_INACTIVE,
					 MskNupdate,   (Value)TRUE,
					 NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("MeTek_AnzSubTe_t"),
				MskNtransferVar2Dup,	(Value)TRUE,
				MskNupdate,				(Value)TRUE,
				NULL);

        handleMotDisplay(hMaskRl, pcFac);

		break;
		
	
	case UNITABLEREASON_CREATE_MASK:
		pcFac = lUniGetFacility (hMaskRl);
		
		MskVaAssign (hMaskRl, MskGetElement (TEK_OrigTeKz_sr),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		if( eNach == JANEIN_N ) {
			MskVaAssign (hMaskRl, MskGetElement ("CheckWeNach_TeBearb"),
				MskNattr, EF_ATTR_INVISIBLE,
				NULL);

			MskVaAssign (hMaskRl, MskGetElement ("LabelWeNach_TeBearb"),
				MskNattr, EF_ATTR_INVISIBLE,
				NULL);
		}

		MskVaAssign (hMaskRl, MskGetElement (TEK_StammTeKz_sr),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEK_GesGew_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEK_GesVol_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEK_EinZeit_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEK_SumGefGutPunkte_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);

		MskVaAssignMatch (hMaskRl, "*-NSRM-TEK-INACTIVE-*",
				MskNattr, EF_ATTR_INACTIVE,
				NULL);

		ptMcTek = (MyTcontextTek*) calloc (1, sizeof(MyTcontextTek));
		if (ptMcTek == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "calloc failed");
			return -1;
		}
		ptMcTek->tepSerials = new std::map<long, StockbaseTSNrList>;
		if (ptMcTek->tepSerials == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "calloc failed");
			return -1;
		}
		
		ptMcTek->lKorrTypRead = TEBEARB_Std;
		ptMcTek->lKorrTyp = TEBEARB_Std;
		ptMcTek->tKretk = ((TeBearbCalldata *)ut->utCalldata)->tKretk;
		ptMcTek->lNeutralInvNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutraleInvNr;
		ptMcTek->lNeutralPosNr = ((TeBearbCalldata *)ut->utCalldata)->lNeutralePosNr;
		
		ptMcTek->hArrTepInfo = ArrCreate (sizeof(StockbaseTChangeTepInfo), 10, (ArrTcbSort) SortInfo, NULL);
		if (ptMcTek->hArrTepInfo == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
								  "ArrCreate failed");
			return (0);
		}
		
		MskAtomAdd(hMaskRl, NULL,
				   ATOM_UniAprMyContext, NULL, (Value)ptMcTek);
		
		
		MskVaAssignMatch (hMaskRl, "*-WEVK-*",
						  MskNvariableStruct,     (Value)&ptMcTek->tWevk,
						  NULL);
		
		MskVaAssignMatch (hMaskRl, "*-AUSK-*",
						  MskNvariableStruct,     (Value)&ptMcTek->tAusk,
						  NULL);
		
        MskVaAssign (hMaskRl, MskGetElement ("Button_NewTeId"),
					 MskNattrClr,   	(Value)EF_ATTR_INACTIVE,
					 NULL);

        MskVaAssign(hMaskRl, MskGetElement("MeTek_AnzSubTe_t"),
        		MskNvariable,		(Value)&ptMcTek->lAnzSubTe,
        		NULL);
		
		EfcbmInstFirstByName(hMaskRl, "Button_NewTeId", KEY_DEF,
							 CbNewTeId_TeBearb, NULL);

        MskVaAssignMatch (hMaskRl,     "*-MaskTek-Check-*",
            			  MskNvariableStruct, (Value) &ptMcTek->lKorrTyp,
						  NULL);

        EfcbmInstFirstByName(hMaskRl, "Delete_F", KEY_DEF,
							 CbBeforeDeleteOrOk_TeBearb, NULL);
		
		EfcbmInstFirstByName(hMaskRl, "Ok_F", KEY_DEF,
							 CbBeforeDeleteOrOk_TeBearb, NULL);
		
		EfcbmInstAfterByName(hMaskRl, "Delete_F", KEY_DEF, (void *)NULL,
							 CbAfterDelete_TeBearb, NULL);

		EfcbmInstFirstByName(hMaskRl, "CheckStd_TeBearb", KEY_DEF,
							 CbCheckBt_TeBearb, NULL);

		EfcbmInstFirstByName(hMaskRl, "CheckWe_TeBearb", KEY_DEF,
							 CbCheckBt_TeBearb, NULL);
		
		EfcbmInstFirstByName(hMaskRl, "CheckWeNach_TeBearb", KEY_DEF,
							 CbCheckBt_TeBearb, NULL);

		EfcbmInstFirstByName(hMaskRl, TEK_TeId_t, KEY_DEF,
							 CbInitKorrTyp_TeBearb, NULL);

		EfcbmInstFirstByName(hMaskRl, TEK_TTS_TetId_t, KEY_DEF,
							 CbChangeTetId_TeBearb, NULL);
		
		EfcbmInstFirstByName(hMaskRl, "Btn_SetMixedTu_TeBearb", KEY_DEF,
							 CbSetMixedTu_TeBearb, NULL);
		
		FrmwrkGIf_AddScannerCallback (hMaskRl, CbScan_TeBearb);
        break;

	case UNITABLEREASON_END_MASK:

		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
        if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTek = (MyTcontextTek*)hAtom->client_data;
		if (ptMcTek != NULL) {
			if (ptMcTek->tepSerials != NULL) {
				delete ptMcTek->tepSerials;
				ptMcTek->tepSerials = NULL;
			}
			ptMcTek->hArrTepInfo = ArrDestroy(ptMcTek->hArrTepInfo);
			free(ptMcTek);
			ptMcTek = NULL;
		}
		MskAtomRemove(hMaskRl, NULL, ATOM_UniAprMyContext, 0);

		FrmwrkGIf_RemoveScannerCallback(hMaskRl);
        break;
	case UNITABLEREASON_POP_FOREGROUND_MASK:
	{
		UNIMENU* ptUnimenu = UniMenuGetMenu(UniTableGetMenuRl(ptUti->utiTableRl));
		void *pvData=ptUnimenu->umCalldata;
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
        if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTek = (MyTcontextTek*)hAtom->client_data;
		int rv=0;
		if(pvData!=NULL) {
			utr = (UNITABLERL *)MskDialogGet (hMaskRl, MskNmaskCalldata);
			TEK *ptRec=(TEK *) UniTableGetRecNow(utr);
			OWidget current = (OWidget) MskElementGet(pvData, MskNwMain);

			char * buffer=(char *) WdgGet(current, WdgNtext);
			if(!IsEmptyStrg(buffer)){
				strncpy(ptRec->tekTeId, buffer, TEID_LEN);
				rv = TExecStdSql (hMaskRl, StdNselect, TN_TEK, ptRec);
				strncpy(ptMcTek->acTeId, buffer, TEID_LEN);
				if(rv>0) {
					UniRead(hMaskRl, utr, 1);
				} else {
					WamasBox (SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_INFO,
					WboxNbutton,    WboxbNok,
					WboxNtext,      (char const*)scoped_cstr::form (MlMsg(
						"Transporteinheit '%s' nicht vorhanden."), buffer),
					NULL);
				}
				UniUpdateMaskVarR(hMaskRl, utr);
			}
		}
		break;
	}
	default:
        break;
    }
    return lUniGeneralMaskCb(ptUti);
}

/**
 * will be called for the UNITABLEREASON_INITFIELDS reason
 *
 * @param[in]   hMaskRl     Mask handle (also used as SQL-transaction ID)
 * @param[in]   pcFac       Logging facility
 * @param       ptMc        [inout] context pointer
 *
 * @retval >=0 success
 * @retval  <0 error
 */
static int uniTableCbTEP_InitFields(MskDialog hMaskRl, char const *pcFac, MyTcontextTep *ptMcTep)
{
	ptMcTep->dAnzMng = 0;
	ptMcTep->dRestMng = 0;
	ptMcTep->dMngRelativ = 0;
	ptMcTep->dAnzMngRelativ = 0;
    memset (ptMcTep->acBschl, 0, sizeof(ptMcTep->acBschl));
    memset (ptMcTep->acKostSt, 0, sizeof(ptMcTep->acKostSt));
    memset (ptMcTep->acMand, 0, sizeof(ptMcTep->acMand));
    memset (ptMcTep->acLiefNr, 0, sizeof(ptMcTep->acLiefNr));
    memset (ptMcTep->acNeutralId, 0, sizeof(ptMcTep->acNeutralId));
    memset (&ptMcTep->tGdArt, 0, sizeof(ptMcTep->tGdArt));
    memset (&ptMcTep->tGdArte, 0, sizeof(ptMcTep->tGdArte));

    return 1;
}

/**
 * will be called for the UPDATE_MASK_VAR reason
 *
 * @param[in]   hMaskRl     Mask handle (also used as SQL-transaction ID)
 * @param[in]   pcFac       Logging facility
 * @param       ptMc        [inout] context pointer
 *
 * @retval >=0 success
 * @retval  <0 error
 */
static int uniTableCbTEP_UpMskVar(MskDialog hMaskRl, char const *pcFac,
                                  MyTcontextTep *ptMcTep, MyTcontextTek *ptMcTek)
{
    int              iRv;
	int				 nNoEle;
    int              nI;
    int              iDeleted;
    int              iTepCnt;
    UNITABLERL  	*utr;
	TEP				*ptTepNow;
    UNICONNECTRL    *ucr;
	LBCBuffer       *lbc;
    LBCElement      *lel;
    StockbaseTChangeTepInfo *ptTepInfo;
    StockbaseTChangeTepInfo tTepInfo;
    MNGS             tMngs;	
    TUESetEfByArt    tTUESetEfByArtMeTeBearb;

    utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);
    ptTepNow = (TEP *)UniTableGetRecNow(utr);
    
    if (ptMcTep->bDoNotReloadSerials) {
    	ptMcTep->bDoNotReloadSerials = false;
    } else {
    	ptMcTep->serials->clear();
    	ptMcTep->serials->assign(
    			(*(ptMcTek->tepSerials))[ptTepNow->tepPosNr].begin(),
    			(*(ptMcTek->tepSerials))[ptTepNow->tepPosNr].end());
    }

    memset(&ptMcTep->tGdArt, 0, sizeof(ptMcTep->tGdArt));
    memset(&ptMcTep->tGdArte, 0, sizeof(ptMcTep->tGdArte));
    if (! IsEmptyString(ptTepNow->tepMId.AId.ArtNr)) {
        iRv = ArticleSIf_GetArtArtEByAId_GD(hMaskRl, pcFac,
                        &ptTepNow->tepMId.AId,&ptMcTep->tGdArt, &ptMcTep->tGdArte);
        OPMSG_CFR(ArticleSIf_GetArtArtEByAId_GD);
    }

    /* count Tep, that are not deleted */
    ucr = UniTableGetParentConnectRl(utr);
    lbc = UniConnectGetLBCByIdx(ucr, 0);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
    for (nI = 0, iTepCnt = 0;
         nI < nNoEle;
         nI++) {

        lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);

        iDeleted = (int )UniConnectLBCElemGetDeleted(ucr, lel);
        if (iDeleted == 0) {
            iTepCnt++;
        }
    }

    // deactivate all editfields
    MskVaAssignMatch(hMaskRl,  "*-BtExec-*",
                     MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                     NULL);
    MskVaAssignMatch(hMaskRl,  "*-EfExec-*",
                     MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                     NULL);

    /* set buttons && efs active/inactive 							 */
    /* "-BtExec-" is Match for all possibly active Efs (TEBEARB_Std) */
    if (ptMcTek->lKorrTyp & (TEBEARB_Std | TEBEARB_We | TEBEARB_WeNach) &&
        ptTepNow->tepPrimThmKz == JANEIN_N) {

        MskVaAssignMatch(hMaskRl,  "*-BtExec-*",
                         MskNattrClr,   (Value)EF_ATTR_INACTIVE,
                         NULL);
        MskVaAssignMatch(hMaskRl,  "*-EfExec-*",
                         MskNattrClr,   (Value)EF_ATTR_INACTIVE,
                         NULL);

        /* USER EXIT */
        if (mxUESetEfByArtMeTeBearb != NULL) {

            memset(&tTUESetEfByArtMeTeBearb, 0,
                   sizeof(tTUESetEfByArtMeTeBearb));
            tTUESetEfByArtMeTeBearb.hMaskRl = hMaskRl;

            if (! IsEmptyString(ptMcTep->tGdArt.gdartAId.ArtNr)) {
                tTUESetEfByArtMeTeBearb.tGdArt = ptMcTep->tGdArt;
            }

            mxUESetEfByArtMeTeBearb(&tTUESetEfByArtMeTeBearb);
        }

        if (iTepCnt <= 1) {
            MskVaAssign(hMaskRl,    MskGetElement("DelEl_F"),
                        MskNkey,	(Value)KEY_DEF,
                        MskNattrOr,	(Value)EF_ATTR_INACTIVE,
                        NULL);
        }

        /* set inactive weight */
        iRv = 0;
        if (! IsEmptyStrg(ptMcTep->tGdArt.gdartAId.ArtNr)) {
            iRv = ArticleSIf_IsWeightToEdit_GD(hMaskRl, pcFac,
                                               &ptMcTep->tGdArt);
            OPMSG_CFR(ArticleSIf_IsWeightToEdit_GD);
        }

        if (iRv != 1) {
            MskVaAssign(hMaskRl,    MskGetElement(TEP_Mngs_Gew_t),
                        MskNkey,   	(Value)KEY_DEF,
                        MskNattrOr,	(Value)EF_ATTR_INACTIVE,
                        NULL);
        }

        /* set inactive 'lock-fields' for We/Wa */
        if (ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
            MskVaAssignMatch(hMaskRl,      "*-NoWe-*",
                             MskNattrOr,  (Value)EF_ATTR_INACTIVE,
                             NULL);
            /* WE retoure */
            if (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure) {
                MskVaAssignMatch(hMaskRl,  "*-InvData-*",
                                 MskNattrClr,  (Value)EF_ATTR_INACTIVE,
                                 NULL);
            }
        }

        if (ptMcTek->lKorrTypRead & TEBEARB_Wa) {
            MskVaAssignMatch (hMaskRl,  "*-NoWa-*",
                              MskNattrOr,  (Value)EF_ATTR_INACTIVE,
                              NULL);
        }
    }
    
    MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
                      MskNupdate,   (Value)TRUE,
                      NULL);

	iRv = ArticleSIf_IsSingleVar(hMaskRl, pcFac);
	OPMSG_CFR(ArticleSIf_IsSingleVar);
	if (iRv > 0) {
		MskVaAssign (hMaskRl,  MskGetElement (TEP_MId_AId_Var_t),
					 MskNattrOr, (Value)EF_ATTR_INACTIVE,
					 NULL);
	}
	iRv = ArticleSIf_IsSingleAusPr(hMaskRl, pcFac);
	OPMSG_CFR(ArticleSIf_IsSingleAusPr);
	if (iRv > 0) {
		MskVaAssign (hMaskRl,  MskGetElement (TEP_MId_AId_AusPr_t),
					 MskNattrOr, (Value)EF_ATTR_INACTIVE,
					 NULL);
	}
	iRv = ClientsSIf_IsSingleClient(hMaskRl, pcFac);
	OPMSG_CFR(ClientsSIf_IsSingleClient);
	if (iRv > 0) {
		MskVaAssign (hMaskRl,  MskGetElement (TEP_MId_AId_Mand_t),
					 MskNattrOr, (Value)EF_ATTR_INACTIVE,
					 NULL);
	}

    /* for negativ quantities/weight set MatQ inactive otherwise  activ */
    if (FRMWRK_DOUBLECMP(ptTepNow->tepMngs.Mng, <, 0) ||
        FRMWRK_DOUBLECMP(ptTepNow->tepMngs.Gew, <, 0)) {
        MskVaAssign (hMaskRl,       MskGetElement("TEP_MId_MatQ_t"),
                     MskNkey,		(Value) KEY_DEF,
                     MskNattrOr,	(Value) EF_ATTR_INACTIVE,
                     MskNupdate,   	(Value)TRUE,
                     NULL);
    } 		
    /* reset */
    ptMcTep->dAnzMng = 0.0;
    ptMcTep->dRestMng = 0.0;

    memset(&tTepInfo, 0, sizeof(tTepInfo));
    tTepInfo.lPosNr = ptTepNow->tepPosNr;
    ptTepInfo = (StockbaseTChangeTepInfo*)ArrGetFindElem (ptMcTek->hArrTepInfo, &tTepInfo);
    if (ptTepInfo && ptTepInfo->iBschlKost) {

        FRMWRK_STRARRNCPY (ptMcTep->acBschl, ptTepInfo->acBschl);
        /* otherwise override of acKostS,
           because of CbSqlDf on BSCHL_BuSchl_t */
        MskVaAssign(hMaskRl,		        MskGetElement(BSCHL_BuSchl_t),
                    MskNkey,				(Value)KEY_DEF,
                    MskNtransferVar2Dup,	(Value)TRUE,
                    MskNupdate,		        (Value)TRUE,
                    NULL);

        FRMWRK_STRARRNCPY (ptMcTep->acKostSt,      ptTepInfo->acKostSt);
        FRMWRK_STRARRNCPY (ptMcTep->acMand,        ptTepInfo->acMand);
        FRMWRK_STRARRNCPY (ptMcTep->acLiefNr,      ptTepInfo->acLiefNr);
        FRMWRK_STRARRNCPY (ptMcTep->acNeutralId,   ptTepInfo->acNeutralId);
        ptMcTep->lNeutralInvNr = ptTepInfo->lNeutralInvNr;
        ptMcTep->lNeutralPosNr = ptTepInfo->lNeutralPosNr;
    } else {
        /* also after CbInitKorrTyp and CbCheckBt - PosNr not changed! */
        memset (ptMcTep->acBschl, 0, sizeof(ptMcTep->acBschl));
        memset (ptMcTep->acKostSt, 0, sizeof(ptMcTep->acKostSt));
        memset (ptMcTep->acMand, 0, sizeof(ptMcTep->acMand));
        memset (ptMcTep->acLiefNr, 0, sizeof(ptMcTep->acLiefNr));
        memset (ptMcTep->acNeutralId, 0, sizeof(ptMcTep->acNeutralId));
        
        // init INVP data with the data saved in the context of the parent dialog (GD071)
		ptMcTep->lNeutralInvNr = ptMcTek->lNeutralInvNr;
		ptMcTep->lNeutralPosNr = ptMcTek->lNeutralPosNr;

        if ((ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) &&
            (ptMcTek->lKorrTypRead & TEBEARB_WeRetoure)) {

            iRv = ParamSIf_Get1Parameter(hMaskRl, pcFac,
                                         PrmStockbaseTeKorLRetBuschl,
                                          ptMcTep->acBschl);
            if (iRv < 0 &&
                iRv != ParameterTerrNotFound) {
                OPMSG_CFR(ParamSIf_Get1Parameter);
            }

            MskVaAssign (hMaskRl,		        MskGetElement(BSCHL_BuSchl_t),
                         MskNkey,				(Value) KEY_DEF,
                         MskNtransferVar2Dup,	(Value) TRUE,
                         NULL);
            MskVaAssign (hMaskRl,		        MskGetElement(KOST_KostSte_t),
                         MskNkey,				(Value) KEY_DEF,
                         MskNtransferDup2Var,	(Value) TRUE,
                         NULL);
        }

    }

    if (!IsEmptyString(ptMcTep->tGdArt.gdartAId.ArtNr)) {
        tMngs = ptTepNow->tepMngs;
        iRv = ArticleSIf_GetMngGewFromArt_GD(hMaskRl, pcFac, &tMngs,
                                              &ptMcTep->tGdArt, &ptMcTep->tGdArte,
                                              &ptMcTep->dAnzMng, &ptMcTep->dRestMng,
                                              BESTANZ);
        OPMSG_CFR(ArticleSIf_GetMngGewFromArt_GD);
    }

    if (ptTepNow->tepPrimThmKz == JANEIN_J) {
        MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
                          MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                          MskNupdate,   (Value)TRUE,
                          NULL);
    }

    if (ptTepNow->tepMId.ThmKz == JANEIN_J) {
    	MskVaAssign (hMaskRl,  	MskGetElement(TEP_FachK_t),
    			MskNattrOr,   (Value)EF_ATTR_INACTIVE,
    			MskNupdate,   (Value)TRUE,
    			NULL);
    }

    return 1;
}

/**
 * will initialize the \a ptTep for a new MOT position
 *
 * caller must enter the primary key (Tu-Id and PosNr)
 *
 * @param[in]   pvTid       Sql transaction id
 * @param[in]   pcFac       Logging facility
 * @param[in]   pktLERec    information about the new LE-position
 * @param[out]  ptTep       TU-position to initialize
 *
 * @retval >=0 success
 * @retval  <0 error
 */
static int initTep4LinkedEmpties(const void* pvTid, char const *pcFac,
    ArticleTArrLinkedEmpties const *pktLERec, TEP *ptTep)
{
    int     iRv;

    iRv = prepareTepToInsert(pvTid, pcFac, ptTep);
    OPMSG_CFR(prepareTepToInsert);

    ptTep->tepMId.AId = pktLERec->tIvArt.ivartAId;
    ptTep->tepMId.ThmKz = JANEIN_J;
    ptTep->tepMngs = pktLERec->tAmount;
    ptTep->tepPrimThmKz = JANEIN_N;

    return 1;
}

/**
 * will calculate the linked empties for the article-change and adopt those
 * positions in the listbuffer of the parent dialog
 *
 * @param[in]   hMaskRl         Mask handle (also used as SQL-transaction ID)
 * @param[in]   pcFac           Logging facility
 * @param[in]   pktTepNow       Tep before change: may be null for ACTION_INSERT
 * @param[in]   pktTepNow       Tep after change: ignored for ACTION_DELETE
 * @param[in]   lAction         see ACTION_XXX defines
 * @param       hArrLinkedEmpties   [inout] array of ArticleTArrLinkedEmpties
 *                                  if the linked empties positions exist in
 *                                  the array, they will be changed, else
 *                                  they will be added
 *
 * @retval  >0 success
 * @retval   0 no listbuffer entries in the main dialog have been changed
 * @retval  <0 error
 */
static int handleLinkedEmptiesWork(MskDialog hMaskRl, char const *pcFac,
    TEP const *pktTepNow, TEP const *pktTepBefore, long lAction,
    MyTcontextTek *ptMcTek, MyTcontextTep *ptMcTep, ArrayPtr hArrLinkedEmpties)
{
    int iRv;
    int              nNoEle;
    int              nI;
    UNITABLERL  	*utr;
    UNICONNECTRL    *ucr;
	LBCBuffer       *lbc;
    LBCElement      *lel;
    TEP             *ptTep;
    TEP             tTepLE;
    ArticleTLinkedEmptiesIn tDataInBefore, tDataInNow;
    ArticleTLinkedEmptiesIn *ptDataInBefore, *ptDataInNow;

    ptDataInBefore = NULL;
    ptDataInNow = NULL;

    if (lAction != ACTION_INSERT) {
        memset(&tDataInBefore, 0, sizeof(tDataInBefore));
        tDataInBefore.tAId = pktTepBefore->tepMId.AId;
        tDataInBefore.dAmount = pktTepBefore->tepMngs.Mng;
        tDataInBefore.eQuantityHandledOnly = JANEIN_J;
        ptDataInBefore = &tDataInBefore;
    }
    if (lAction != ACTION_DELETE) {
        memset(&tDataInNow, 0, sizeof(tDataInNow));
        tDataInNow.tAId = pktTepNow->tepMId.AId;
        tDataInNow.dAmount = pktTepNow->tepMngs.Mng;
        tDataInNow.eQuantityHandledOnly = JANEIN_J;
        ptDataInNow = &tDataInNow;
    }

    iRv = ArticleSIf_GetLinkedEmpties(hMaskRl, pcFac, ptDataInBefore,
                                      ptDataInNow, hArrLinkedEmpties);
    OPMSG_CFR(ArticleSIf_GetLinkedEmpties);

    iRv = ArrGetNoEle(hArrLinkedEmpties);
    if (iRv < 0) {
        LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT, "ArrGetNoEle",
                                  "failed to get no of elements");
        return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
    } else if (iRv == 0) {
        return 0;
    }

    /* UniMenue */
    utr = (UNITABLERL *)MskDialogGet(hMaskRl, MskNmaskCalldata);
    ucr = UniTableGetParentConnectRl(utr);

    ArticleTArrLinkedEmpties tLERec;
    ArticleTArrLinkedEmpties const *pktLERec;

    // adjust amount of linked empties
    lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
    for (nI = 0; nI < nNoEle; nI++) {
        lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
        ptTep = (TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
        if (ptTep->tepPosNr != pktTepNow->tepPosNr &&
            ptTep->tepMId.ThmKz == JANEIN_J) {

            memset(&tLERec, 0, sizeof(tLERec));
            tLERec.tIvArt.ivartAId = ptTep->tepMId.AId;
            pktLERec = (const ArticleTArrLinkedEmpties*)
				ArrSearchElem(hArrLinkedEmpties, &tLERec);
            if (pktLERec == NULL) {
                // this linked empties position has not been changed
                continue;
            }

            iRv = setTepInfo(pcFac, ptTep->tepPosNr, ptMcTek, ptMcTep);
            OPMSG_CFR(setTepInfo);

            // in/decrease amount
            iRv = ArticleSIf_AddMngs(pcFac, &ptTep->tepMngs, &pktLERec->tAmount,
                                     &ptTep->tepMngs);
            OPMSG_CFR(ArticleSIf_AddMngs);
            if (FRMWRK_DOUBLECMP(ptTep->tepMngs.Mng, <=, 0)) {
                ptTep->tepMngs.Mng = 0;
                ptTep->tepMngs.Gew = 0;

                iRv = Frmwrk_UnimenuDeleteElement(pcFac, ucr, lbc,
                                                  MD_LBC_IDX_TEP, lel);
                OPMSG_CFR(Frmwrk_UnimenuDeleteElement);

            } else {
                // if the position has been deleted - reactivate it
                iRv = Frmwrk_UnimenuReactivateElement(pcFac, ucr, lbc,
                                                      MD_LBC_IDX_TEP, lel);
                OPMSG_CFR(Frmwrk_UnimenuReactivateElement);
            }

            iRv = UniConnectLBCElemSetEnteredByIdx(ucr, MD_LBC_IDX_TEP,
                                                   lel, 1L);
            if (iRv < 0) {
                LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT,
                      "UniConnectLBCElemSetEnteredByIdx", "iRv=%d", iRv);
                return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed,
                                        NULL);
            }

            iRv = ArrUnloadSearchElem(hArrLinkedEmpties, pktLERec);
            if (iRv < 0) {
                LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT,
                                      "ArrUnloadSearchElem()", "iRv=%d", iRv);
                return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed,
                                        NULL);
            }
        }
    }

    // insert new records
    pktLERec = (ArticleTArrLinkedEmpties const *)
                                ArrWalkStart(hArrLinkedEmpties);
    while (pktLERec != NULL) {

        if (FRMWRK_DOUBLECMP(pktLERec->tAmount.Mng,  <=, 0)) {
            /* e.g. a user has deleted the beer-bottles, but has kept the beer
             * then we will come here and want to subtract beer-bottles
             * since there are no beer bottles at all on this TU there is
             * nothing to do
             */
        } else {
            memset(&tTepLE, 0, sizeof(tTepLE));
            FRMWRK_STRARRNCPY(tTepLE.tepTeId, pktTepNow->tepTeId);

			iRv = Frmwrk_UnimenuGetNextPosNr(pcFac, ucr, lbc,
											 offsetof(TEP, tepPosNr),
											 &tTepLE.tepPosNr,
											 pktTepNow->tepPosNr);
            OPMSG_CFR(Frmwrk_UnimenuGetNextPosNr);

            iRv = initTep4LinkedEmpties(hMaskRl, pcFac, pktLERec, &tTepLE);
            OPMSG_CFR(initTep4LinkedEmpties);

            iRv = Frmwrk_UnimenuInsertElement(pcFac, ucr, lbc, MD_LBC_IDX_TEP,
                                              MD_TEP_LBC_NAME, &tTepLE);
            OPMSG_CFR(Frmwrk_UnimenuInsertElement);

            iRv = setTepInfo(pcFac, tTepLE.tepPosNr, ptMcTek, ptMcTep);
            OPMSG_CFR(setTepInfo);
        }

        iRv = ArrUnloadSearchElem(hArrLinkedEmpties, pktLERec);
        if (iRv < 0) {
            LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT,
                                      "ArrUnloadSearchElem()", "iRv=%d", iRv);
            return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
        }
        pktLERec = (ArticleTArrLinkedEmpties const *)
                                    ArrWalkStart(hArrLinkedEmpties);
    }

    LBCUpdate(lbc);

    return 1;
}

/**
 * will allocate memory and call handleLinkedEmptiesWork()
 */
static int handleLinkedEmpties(MskDialog hMaskRl, char const *pcFac,
    TEP const *pktTepNow, TEP const *pktTepBefore, long lAction,
    MyTcontextTek *ptMcTek, MyTcontextTep *ptMcTep)
{
    int iRv;
    ArrayPtr         hArrLinkedEmpties;

    if (pktTepNow->tepMId.ThmKz == JANEIN_J) {
        return 0;
    }

    hArrLinkedEmpties = ArrCreate(sizeof(ArticleTArrLinkedEmpties), 1,
                                  NULL, NULL);
    if (hArrLinkedEmpties == NULL) {
        LoggingSIf_LogPrintfTools(pcFac, LOGGING_SIF_ALERT, "ArrCreate",
                                  "failed to allocate memory");
        return OpmsgSIf_ErrPush((int)GeneralTerrSubFunctionFailed, NULL);
    }

    iRv = handleLinkedEmptiesWork(hMaskRl, pcFac, pktTepNow, pktTepBefore,
                                  lAction, ptMcTek, ptMcTep, hArrLinkedEmpties);
    hArrLinkedEmpties = ArrDestroy(hArrLinkedEmpties);
    OPMSG_CFR(handleLinkedEmptiesWork);
    if (iRv == 0) {
        return 0;
    }

    return 1;
}

/**
 * returns the current action (see ACTION_XXX defines) when the user clicks
 * 'Apply' or 'Delete' in the position dialog
 *
 * @param[in]   pcFac       Logging facility
 * @param[in]   ptUti       Unitable info (to get the reason)
 * @param[in]   ucr         Uniconnect (to get the head listbuffer)
 * @param[in]   pktTepNow   current TEP info - PosNr needed to search listbuffer
 * @param[out]  plAction    the current action (see ACTION_XXX defines)
 *
 * @retval  =0 success
 * @retval   0 NO ACTION at all
 *             - user has clicken 'New Position' in head dialog
 *               and then 'Delete' in the position dialog
 * @retval  <0 error
 */
static int getAction(char const *pcFac, UNITABLEINFO *ptUti,
                     UNICONNECTRL *ucr, TEP const *pktTepNow, long *plAction)
{
    TEP                 *ptTepNowLb, *ptTepBeforeLb;
	LBCBuffer           *lbc;
    LBCElement          *lel;
    int					nNoEle, nI;

    /* loop TEP */
    lbc = UniConnectGetLBCByIdx(ucr, MD_LBC_IDX_TEP);
    nNoEle = LBCNumberOfElements(lbc, LbcSectBody);
    ptTepBeforeLb = NULL;
    for (ptTepNowLb = NULL, nI = 0;
         nI < nNoEle;
         ptTepNowLb = NULL, nI++) {

        lel = LBCGetElementByIdx(lbc, LbcSectBody, nI);
        ptTepNowLb = (TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
        if (ptTepNowLb &&
            ptTepNowLb->tepPosNr == pktTepNow->tepPosNr) {
			// UPDATE, INSERT
            ptTepBeforeLb =
					(TEP *)UniConnectLBCElemGetRecBeforeByIdx(ucr, 0, lel);
            break;
        } else {
			ptTepBeforeLb =
					(TEP *)UniConnectLBCElemGetRecBeforeByIdx(ucr, 0, lel);
			if (ptTepBeforeLb &&
				ptTepBeforeLb->tepPosNr == pktTepNow->tepPosNr) {
				// DELETE
				ptTepNowLb =
					(TEP *)UniConnectLBCElemGetRecNowByIdx(ucr, 0, lel);
			}
		}
    }

    if (ptTepNowLb == NULL) {
        if (ptUti->utiReason == UNITABLEREASON_DELELEM) {
            // user has clicken 'New Position' in head dialog
            // and then 'Delete' in the position dialog
            return 0;
        } else {
            // position does not exist in head dialogs listbuffer and
            // user clicked 'Apply' in the position dialog
            *plAction = ACTION_INSERT;
        }
    } else {
        if (ptUti->utiReason == UNITABLEREASON_DELELEM) {
            // record exists in head dialogs listbuffer and user
            // clicked 'Delete'
            *plAction = ACTION_DELETE;
        } else {
            // record exists in head dialogs listbuffer and user
            // clicked 'Apply'
            *plAction = ACTION_UPDATE;
        }
    }

    return 1;
}

/**
 * will be called for the reasons: UNITABLEREASON_ENTER, UNITABLEREASON_DELELEM
 *
 * @param[in]   hMaskRl     Mask handle (also used as SQL-transaction ID)
 * @param[in]   pcFac       Logging facility
 * @param       ptTepNow    [inout] Tep data
 *
 * @retval >=0 success
 * @retval  <0 error
 */
static int uniTableCbTEP_Change(MskDialog hMaskRl, char const *pcFac,
                                UNITABLEINFO *ptUti)
{
    int iRv;
    MskDialog   		hMaskRlParent;
    ContainerAtom   	hAtom;
    MyTcontextTek   	*ptMcTek = NULL;
    MyTcontextTep		*ptMcTep = NULL;
    UNITABLERL  		*utr;
    UNITABLERL  		*utrParent;
    UNICONNECTRL        *ucr;
	LBCBuffer			*lbc;

	TEK					*ptTekNow;
	TEK					*ptTekBefore;
	TEP					*ptTepNow, *ptTepBefore;

	long				lAction;

	/* MaskContext */
	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
	if (hAtom == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "ptMcTek is NULL !");
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
	}
	ptMcTep = (MyTcontextTep*)(hAtom->client_data);
	if (ptMcTep == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "ptMcTep is NULL !");
		return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
	}
	
	/* Parent MaskContext */
	utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);
	ucr = UniTableGetParentConnectRl(utr);
	utrParent = (UNITABLERL *) UniConnectGetParentTableRl(ucr);
	hMaskRlParent = UniTableGetMaskRl (utrParent);
    hAtom = MskAtomGet(hMaskRlParent, NULL, ATOM_UniAprMyContext);
    if (hAtom == NULL ||
        hAtom->client_data == (Value)NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "INTERNAL ERROR: MskAtomGet() failed");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }
    ptMcTek = (MyTcontextTek*)hAtom->client_data;
    if (ptMcTek == NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "INTERNAL ERROR: ptMcTek == NULL");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }

    /* UniMenue */
    UniTransferMaskDup(hMaskRl, utr);
    UniTransferMaskDup(hMaskRlParent, utrParent);

	lbc = UniConnectGetLBCByIdx(ucr, 0);

    ptTepNow = (TEP *)UniTableGetRecNow (utr);
    ptTepBefore = (TEP *)UniTableGetRecBefore(utr);
    ptTekNow = (TEK *)UniTableGetRecNow (utrParent);
    ptTekBefore = (TEK *)UniTableGetRecBefore (utrParent);

    if (ptTepNow == NULL ||
        ptTekNow == NULL ||
        ptTekBefore == NULL) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
                             "INTERNAL ERROR: failed to get data");
        return OpmsgSIf_ErrPush((int)GeneralTerrInternal, NULL);
    }

    iRv = getAction(pcFac, ptUti, ucr, ptTepNow, &lAction);
    OPMSG_CFR(getAction);
    if (iRv == 0) {
        return 0;
    }

    if (lAction == ACTION_INSERT &&
        ptTepNow->tepPosNr == 0) {
		
		iRv = Frmwrk_UnimenuGetNextPosNr(pcFac, ucr, lbc,
										 offsetof(TEP, tepPosNr),
										 &ptTepNow->tepPosNr, 0);
        OPMSG_CFR(Frmwrk_UnimenuGetNextPosNr);
    }

	//create stack pointer
	OpmsgTCbStackPtr	stackPtr = NULL;

	iRv = OpmsgSIf_CbCreate(pcFac, &stackPtr);
	OPMSG_CFR(OpmsgSIf_CbCreate);

	while (true) {
		OpmsgSIf_ErrResetMsg();

    	iRv = CheckTEP_tebearb(hMaskRl, pcFac, stackPtr, ptMcTek, ptMcTep, ptTekNow, 
					ptTekBefore, ptTepNow, ptTepBefore, lAction, ucr, lbc);
		if (iRv < 0) {
			TSqlRollback(hMaskRl);

			int iRvHnd = OpmsgGIf_HandleError (hMaskRl, pcFac, stackPtr, iRv,
									MlM("TE Korrektur"));
			if (iRvHnd > 0) {
				//error has been handled - try again
				continue;
			} else if (iRvHnd == 0) {
				iRv = GeneralTerrHasAlreadyBeenHandled;
				break;
			} else {
				OPMSG_SFF(CheckTEP_tebearb);
				break;
			}
		}
		break;
	}

	//destroy stack pointer
	OpmsgSIf_CbDestroy(&stackPtr);

	if (iRv < 0) {
		return (iRv);
	}

	/**
	 * create new tu position if article has been changed
	 */
	if (lAction == ACTION_UPDATE &&
		memcmp (&ptTepNow->tepMId.AId,
				&ptTepBefore->tepMId.AId, sizeof(AID)) != 0){

		//create new tu position (get new PosNr)
		iRv = Frmwrk_UnimenuGetNextPosNr (pcFac, ucr, lbc,
						offsetof(TEP, tepPosNr), 
						&ptTepNow->tepPosNr, ptTepNow->tepPosNr);
		OPMSG_CFR(Frmwrk_UnimenuGetNextPosNr);	

		iRv = setTepInfo(pcFac, ptTepNow->tepPosNr, ptMcTek, ptMcTep);
		OPMSG_CFR(setTepInfo);

		//delete old tu position (mark tu position for delete)
		LBCElement	*lel = NULL;

		iRv = getListbufferEle4Tep(pcFac, ucr, &lel, ptTepBefore);
		OPMSG_CFR(getListbufferEle4Tep);

		iRv = Frmwrk_UnimenuDeleteElement_NoResetEntered(pcFac, ucr, lbc,
                                                  MD_LBC_IDX_TEP, lel);
		OPMSG_CFR(Frmwrk_UnimenuDeleteElement);

		iRv = setTepInfo(pcFac, ptTepBefore->tepPosNr, ptMcTek, ptMcTep);
		OPMSG_CFR(setTepInfo);
	} else {
    	iRv = setTepInfo(pcFac, ptTepNow->tepPosNr, ptMcTek, ptMcTep);
    	OPMSG_CFR(setTepInfo);
	}

    /* now that the user has changed a TEP, we deactivate the correction type
     * check buttons in the HEAD dialog, because the user must not mix different
     * correction types  */
    MskVaAssignMatch (hMaskRlParent,  "*-Check-*",
                      MskNattrOr,   (Value)EF_ATTR_INACTIVE,
                      MskNupdate,   (Value)TRUE,
                      NULL);

    iRv = handleLinkedEmpties(hMaskRl, pcFac, ptTepNow, ptTepBefore, lAction,
                              ptMcTek, ptMcTep);
    OPMSG_CFR(handleLinkedEmpties);

    // update parent masks listbuffer
    handleMotDisplay(hMaskRlParent, pcFac);
    
    (*(ptMcTek->tepSerials))[ptTepNow->tepPosNr].clear();
    (*(ptMcTek->tepSerials))[ptTepNow->tepPosNr].assign(
    		ptMcTep->serials->begin(),
    		ptMcTep->serials->end());

    return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UniTableCbTEP(UNITABLEINFO *ptUti)
{
    MskDialog       	hMaskRl;
	MskDialog   		hMaskRlParent;

    ContainerAtom   	hAtom;
    MyTcontextTep   	*ptMcTep = NULL;
    MyTcontextTek		*ptMcTek = NULL;
    UNITABLERL  		*utr;
    UNITABLERL  		*utrParent;
    UNICONNECTRL        *ucr;

	char const			*pcFac;
	int					iRv;

    hMaskRl = UniTableGetMaskRl(ptUti->utiTableRl);
	pcFac = lUniGetFacility (hMaskRl);
	
	if (hMaskRl == NULL) {
		return lUniGeneralMaskCb(ptUti);
	}

    switch(ptUti->utiReason) {
	case UNITABLEREASON_INITFIELDS:
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);

		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTep = (MyTcontextTep*)hAtom->client_data;
        OpmsgSIf_ErrResetMsg();
        iRv = uniTableCbTEP_InitFields(hMaskRl, pcFac, ptMcTep);
        if (iRv < 0) {
            OPMSG_SFF(uniTableCbTEP_InitFields);
            OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("Felder Initialisierung"),
                                  NULL);
            return -1;
        }
		break;
	
	case UNITABLEREASON_UPDATE_MASK_VAR:
		
		/* MaskContext */
		hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
            break;
        }
        ptMcTep = (MyTcontextTep*)hAtom->client_data;
		pcFac = lUniGetFacility(hMaskRl);
		
		/* Parent MaskContext */
		utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);
		ucr = UniTableGetParentConnectRl(utr);
		utrParent = (UNITABLERL *) UniConnectGetParentTableRl(ucr);
		hMaskRlParent = UniTableGetMaskRl (utrParent);
		hAtom = MskAtomGet(hMaskRlParent, NULL, ATOM_UniAprMyContext);
		if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
			break;
		}
		ptMcTek = (MyTcontextTek*)hAtom->client_data;
				
        OpmsgSIf_ErrResetMsg();
        iRv = uniTableCbTEP_UpMskVar(hMaskRl, pcFac, ptMcTep, ptMcTek);
		if (iRv < 0) {
            TSqlRollback(hMaskRl);
            OPMSG_SFF(uniTableCbTEP_UpMskVar);
            OpmsgGIf_ShowAlertBox(hMaskRl, iRv,
                                  MlM("Transporteinheit bearbeiten"), NULL);
            break;
		}
        TSqlCommit(hMaskRl);
		break;
		
	case UNITABLEREASON_ENTER:
    case UNITABLEREASON_DELELEM:
    	
    	/* MaskContext */
    	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
    	if (hAtom == NULL || hAtom->client_data == (Value)NULL) {
    		break;
    	}
    	ptMcTep = (MyTcontextTep*)hAtom->client_data;
        pcFac = lUniGetFacility (hMaskRl);

        OpmsgSIf_ErrResetMsg();
        iRv = uniTableCbTEP_Change(hMaskRl, pcFac, ptUti);
        if (iRv < 0) {
        	ptMcTep->bDoNotReloadSerials = true;
            TSqlRollback(hMaskRl);
            OPMSG_SFF(uniTableCbTEP_Change);
            OpmsgGIf_ShowAlertBox(hMaskRl, iRv,
                                  MlM("Transporteinheit Position ändern"),
                                  NULL);
            return -1;
        }
        TSqlCommit(hMaskRl);

		break;

	case UNITABLEREASON_CREATE_MASK:
		pcFac = lUniGetFacility (hMaskRl);
		
		MskVaAssign (hMaskRl, MskGetElement (TEP_MId_WeNr_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEP_GesVol_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEP_GesGew_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEP_ErstErfZeit_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssign (hMaskRl, MskGetElement (TEP_InvZeit_t),
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		MskVaAssignMatch (hMaskRl, "*-PtTCycleCount-*",
				MskNattr, EF_ATTR_INACTIVE,
				NULL);
		
		ptMcTep = (MyTcontextTep*)calloc(1, sizeof(MyTcontextTep));
		if (ptMcTep == NULL) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "Error allocating ptMcTep");
			return -1;
		}
		ptMcTep->serials = new StockbaseTSNrList;
		if (ptMcTep->serials == NULL) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "Error allocating ptMcTep->serials");
			return -1;
		}
		MskAtomAdd(hMaskRl, NULL, ATOM_UniAprMyContext, NULL, (Value)ptMcTep);
		
		MskVaAssign (hMaskRl,     MskGetElement ("INVBA_NeutralId_t"),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) &ptMcTep->acNeutralId,
			NULL);

		MskVaAssign (hMaskRl, MskGetElement ("INVBA_Neutrale_InvNr_t"),
			MskNkey,			(Value) KEY_DEF,
			MskNvariable,		(Value) &ptMcTep->lNeutralInvNr,
			NULL);

		MskVaAssign (hMaskRl, MskGetElement ("INVBA_Neutrale_PosNr_t"),
			MskNkey,			(Value) KEY_DEF,
			MskNvariable,		(Value) &ptMcTep->lNeutralPosNr,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (BSCHL_BuSchl_t),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) &ptMcTep->acBschl,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (KOST_KostSte_t),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) &ptMcTep->acKostSt,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement ("LST_Mand_t"),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) &ptMcTep->acMand,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement ("LST_LiefNr_t"),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) &ptMcTep->acLiefNr,
			NULL);
		
		MskVaAssign (hMaskRl,     MskGetElement (TEP_Mngs_Mng_t),
			MskNkey,            (Value) KEY_1,
			MskNvariable,       (Value) &ptMcTep->dAnzMng,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (TEP_Mngs_Mng_t),
			MskNkey,            (Value) KEY_2,
			MskNvariable,       (Value) &ptMcTep->dRestMng,
			NULL);
			
		MskVaAssign (hMaskRl,   MskGetElement ("ME_TEBEARBP_MengeR_t"),
			MskNkey,            (Value) KEY_1,
			MskNvariable,       (Value) &ptMcTep->dMngRelativ,
			NULL);
		
		MskVaAssign (hMaskRl,   MskGetElement ("ME_TEBEARBP_MengeR_t"),
			MskNkey,            (Value) KEY_2,
			MskNvariable,       (Value) &ptMcTep->dAnzMngRelativ,
			NULL);
		
		MskVaAssign (hMaskRl,   MskGetElement (V_GDART_Anzeige_Einheit_t),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) ptMcTep->tGdArt.gdartAnzeige_Einheit,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (V_GDART_Anzeige_Einheit_t),
			MskNkey,            (Value) KEY_1,
			MskNvariable,       (Value) ptMcTep->tGdArt.gdartAnzeige_Einheit,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (V_GDART_Bestand_Einheit_t),
			MskNkey,            (Value) KEY_DEF,
			MskNvariable,       (Value) ptMcTep->tGdArt.gdartBestand_Einheit,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (V_GDART_Bestand_Einheit_t),
			MskNkey,            (Value) KEY_1,
			MskNvariable,       (Value) ptMcTep->tGdArt.gdartBestand_Einheit,
			NULL);

		MskVaAssign (hMaskRl,     MskGetElement (V_GDART_Bestand_Einheit_t),
			MskNkey,            (Value) KEY_2,
			MskNvariable,       (Value) ptMcTep->tGdArt.gdartBestand_Einheit,
			NULL);
			
		EfcbmInstByName(hMaskRl,    "TEP_Mngs_Mng_t",     KEY_DEF,
						CbCalcMng_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    "INVBA_Neutrale_InvNr_t",     KEY_DEF,
						CbFillInvNr,      NULL);
		EfcbmInstByName(hMaskRl,    "INVBA_Neutrale_PosNr_t",     KEY_DEF,
						CbFillInvPosNr,      NULL);
		EfcbmInstByName(hMaskRl,    "TEP_Mngs_Mng_t",     KEY_1,
						CbCalcMng_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    "TEP_Mngs_Mng_t",     KEY_2,
						CbCalcMng_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    "TEP_Mngs_Gew_t",     KEY_DEF,
						CbCalcMng_TeBearb,      NULL);
		
		EfcbmInstByName(hMaskRl,    "ME_TEBEARBP_MengeR_t", KEY_1,
						CbCalcMng_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    "ME_TEBEARBP_MengeR_t", KEY_2,
						CbCalcMng_TeBearb,      NULL);
		
		EfcbmInstByName(hMaskRl,    TEP_MId_AId_Mand_t, KEY_DEF,
						CbArtChange_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    TEP_MId_AId_ArtNr_t, KEY_DEF,
						CbArtChange_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    TEP_MId_AId_Var_t, KEY_DEF,
						CbArtChange_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    TEP_MId_AId_AusPr_t, KEY_DEF,
						CbArtChange_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    TEP_MId_MHD_t, KEY_DEF,
						CbMhdChange_TeBearb,      NULL);
		EfcbmInstByName(hMaskRl,    "ME_TEBEARBP_SerienNr", KEY_DEF,
				        CbSerienNr_Tep,      NULL);
        break;

    case UNITABLEREASON_END_MASK:
        pcFac = lUniGetFacility(hMaskRl);
        hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
        if (hAtom == NULL) {
        	break;
        }
        ptMcTep = (MyTcontextTep*)(hAtom->client_data);
        if (ptMcTep != NULL) {
        	if (ptMcTep->serials != NULL) {
        		delete ptMcTep->serials;
        		ptMcTep->serials = NULL;
        	}
        	free(ptMcTep);
        	ptMcTep = NULL;
        }
        MskAtomRemove(hMaskRl, NULL, ATOM_UniAprMyContext, 0);
        
        utr = (UNITABLERL *)MskDialogGet(hMaskRl, MskNmaskCalldata);
        ucr = UniTableGetParentConnectRl(utr);
		utrParent = (UNITABLERL *) UniConnectGetParentTableRl(ucr);
		hMaskRlParent = UniTableGetMaskRl(utrParent);
        handleMotDisplay(hMaskRlParent, pcFac);
        break;

	default:
        break;
    }
    return lUniGeneralMaskCb(ptUti);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int UniTableCbTEKPRN (UNITABLEINFO *ptUti)
{
    MskDialog  hMaskRl;

    hMaskRl = UniTableGetMaskRl(ptUti->utiTableRl);
	
	if (hMaskRl == NULL) {
		return lUniGeneralMaskCb(ptUti);
	}

    switch(ptUti->utiReason) {
	case UNITABLEREASON_UPDATE_MASK_VAR:
		
		/* set buttons && efs active/inactive 							 */
		/* "-BtExec-" is Match for all possibly active Efs (TEBEARB_Std) */
			
		MskVaAssignMatch (hMaskRl,  "*-BtExec-*",
						  MskNattrOr,   (Value)EF_ATTR_INACTIVE,
						  MskNupdate,   (Value)TRUE,
						  NULL);
		MskVaAssignMatch (hMaskRl,  "*-EfExec-*",
						  MskNattrOr,   (Value)EF_ATTR_INACTIVE,
						  NULL);
		
		break;
	
	default:
        break;
    }
    return lUniGeneralMaskCb(ptUti);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Listbuttoncallback
-* RETURNS
-*      0	OK
-*--------------------------------------------------------------------------*/
static int search_tebearb (UNITABLEINFO *ptUti)
{
    switch (ptUti->utiReason) {
    case UNITABLEREASON_SEARCH:
        _li_te (GetRootShell (), (Value)LI_TE_CALLSOURCE_BEARBEITEN, NULL);
        break;
    default:
        break;
    }
    return (0);
}

/**
 *	Callback for Button "Seriennummer erfassen").
 *
 *	@param[in]	hMaskRl		mask handle (also used as SQL-transaction ID)
 *	@param[in]	hEf			editfield handle
 *	@param[in]	hEfRl		mask element
 *	@param[in]	iReason		callback reason
 *	@param[in]	hCbc		data structure
 *	@param[in]	xx			mask data
 *
 *	@retval	1	OK
 *	@retval	<o	Error
 */
int CbSerienNr_Tep (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
						   int iReason, void *hCbc, void *xx)
{
	int					iRv = 0;
	char const			*pcFac;
	MskDialog			hMaskRlParent;
    UNITABLERL			*utr;
    UNITABLERL  		*utrParent;
    UNICONNECTRL        *ucr;
    ContainerAtom   	hAtom;
	MyTcontextTep       *ptMcTep;
	MyTcontextTek		*ptMcTek;
	StockbaseTEnterSerials tEnterSerials;
	TEP                 *ptRecTep;
	

	switch (iReason) {
	case FCB_XF:
		pcFac = lUniGetFacility (hMaskRl);
        utr = (UNITABLERL *) MskDialogGet(hMaskRl, MskNmaskCalldata);
        UniTransferMaskDup(hMaskRl,utr);

        /* MaskContext */
    	hAtom = MskAtomGet(hMaskRl, NULL, ATOM_UniAprMyContext);
    	if (hAtom == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "MskAtomGet hAtom is NULL !");
        	return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL); 
    	}
		ptMcTep = (MyTcontextTep*)(hAtom->client_data);
    	if (ptMcTep == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "ptMcTep is NULL !");
        	return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL);
    	}
    	
    	/* Parent MaskContext */
    	ucr = UniTableGetParentConnectRl(utr);
    	utrParent = (UNITABLERL *) UniConnectGetParentTableRl(ucr);
    	hMaskRlParent = UniTableGetMaskRl (utrParent);
    	hAtom = MskAtomGet(hMaskRlParent, NULL, ATOM_UniAprMyContext);
    	if (hAtom == NULL) {
    		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "MskAtomGet hAtom is NULL !");
    		return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL); 
    	}
    	ptMcTek = (MyTcontextTek*)(hAtom->client_data);
    	if (ptMcTek == NULL) {
    		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "ptMcTek is NULL !");
    		return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL);
    	}

		ptRecTep = (TEP *)UniTableGetRecNow(utr);
		
		tEnterSerials.serials = ptMcTep->serials;

		// (defensive) default
		tEnterSerials.eChangeable = JANEIN_N;

		// handle standard correction like Wa correction
		if (ptMcTek->lKorrTyp & TEBEARB_Std) {
			tEnterSerials.eCaller = StockbaseTSnrCaller_WH;
			tEnterSerials.eChangeable = JANEIN_J;
		} else if(ptMcTek->lKorrTyp & TEBEARB_Wa) {
			tEnterSerials.eCaller = StockbaseTSnrCaller_GO;
			tEnterSerials.eChangeable = JANEIN_J;
		} else if (ptMcTek->lKorrTyp & (TEBEARB_We | TEBEARB_WeNach)) {
			tEnterSerials.eCaller = StockbaseTSnrCaller_GI;
			tEnterSerials.eChangeable = JANEIN_J;
		} else {
			// no correction flag set at all
			tEnterSerials.eChangeable = JANEIN_N;
		}

		if (ptMcTep->eDlgMode == DLG_MODE_MAINTAIN) {
			// in maintain mode, you can always change the serial numbers
			tEnterSerials.eChangeable = JANEIN_J;
			// any default - better use WH 
			tEnterSerials.eCaller = StockbaseTSnrCaller_WH;
		}

		tEnterSerials.tIv_Art.ivartAId = ptMcTep->tGdArt.gdartAId;
		tEnterSerials.tMngs = ptRecTep->tepMngs;

		iRv = _me_enterserials(pcFac, SHELL_OF(hMaskRl), &tEnterSerials);
		if (iRv < 0) {
			OPMSG_SFF(_me_enterserials);
			OpmsgGIf_ShowAlertBox(hMaskRl, iRv, MlM("Seriennummern-Eingabe"), NULL);
			return iRv;
		}
		
		MskVaAssign(hMaskRl,	        MskGetElement(TEP_MId_SerienNrGrp_t),
					MskNkey,			(Value) KEY_DEF,
					MskNupdate,         (Value) TRUE,
					MskNtransferVar2Dup,(Value) TRUE,
					NULL);
		break;
	default:
		break;
	}
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		
-* RETURNS
-*      0   Ok
-*--------------------------------------------------------------------------*/
int entry_tebearb (OWidget parent,void  *ptRec, TOriginalCaller eOrigCaller,
                   KRETK *ptKretk, long lNeutraleInvNr, long lNeutralePosNr)
{
	return entry_tebearb(parent, ptRec, eOrigCaller, ptKretk, lNeutraleInvNr, lNeutralePosNr, NULL);
}

int entry_tebearb (OWidget parent,void  *ptRec, TOriginalCaller eOrigCaller,
                   KRETK *ptKretk, long lNeutraleInvNr, long lNeutralePosNr, MskElement hParentEle)
{
	UNITABLE				ut_TEP;
    UNICONNECT				uc_TEK_TEP[2];
    UNITABLE				ut_TEK;
    UNIMENU					um_TEK;
	UNITABLE				ut_TEKPRN;
	static TeBearbCalldata	stTeBearbCalldata;
	memset(&stTeBearbCalldata, 0, sizeof(stTeBearbCalldata));
	stTeBearbCalldata.eOrigCaller = eOrigCaller;
	if (ptKretk != NULL) {
		stTeBearbCalldata.tKretk = *ptKretk;
	}
	if (lNeutraleInvNr > 0 && lNeutralePosNr > 0) {
		stTeBearbCalldata.lNeutraleInvNr = lNeutraleInvNr;
		stTeBearbCalldata.lNeutralePosNr = lNeutralePosNr;
	}

	OwrcLoadObject (ApNconfigFile, "WEVK.rc");
	OwrcLoadObject (ApNconfigFile, "AUSK.rc");
	OwrcLoadObject (ApNconfigFile, "V_GDART.rc");
    OwrcLoadObject (ApNconfigFile, "TEK.rc");
    OwrcLoadObject (ApNconfigFile, "TEP.rc");
    OwrcLoadObject (ApNconfigFile, "TEKPRN.rc");
    OwrcLoadObject (ApNconfigFile, "MATQK.rc");
    OwrcLoadObject (ApNconfigFile, "BSCHL.rc");
    OwrcLoadObject (ApNconfigFile, "KOST.rc");
    OwrcLoadObject (ApNconfigFile, "LST.rc");
    OwrcLoadObject (ApNconfigFile, "INVBA.rc");
    OwrcLoadObject (ApNconfigFile, "global_goods.rc");
    OwrcLoadObject (ApNconfigFile, "me_tekprn.rc");
    OwrcLoadObject (ApNconfigFile, "me_tek.rc");
    OwrcLoadObject (ApNconfigFile, "me_tep.rc");

    memset (&ut_TEP, 0, sizeof (UNITABLE));
    ut_TEP.utName               = TN_TEP;
    ut_TEP.utLabel              =
		FrmwrkGIf_GetMenuTitle 
					(MlMsg ("GD081 Transporteinheit Position bearbeiten"));
    ut_TEP.utMaskName           = "ME_TEP";
    ut_TEP.utAlterMaskName      = NULL;
    ut_TEP.utSelectTableName    = TN_TEP;
    ut_TEP.utInsertTableName    = TN_TEP;
    ut_TEP.utUpdateTableName    = TN_TEP;
    ut_TEP.utDeleteTableName    = TN_TEP;
    ut_TEP.utStructName         = SN_TEP;
    ut_TEP.utStructSize         = sizeof (TEP);
    ut_TEP.utMaskMatch          = "*-TEP-*";
    ut_TEP.utFocusField         = TEP_MId_AId_Mand_t;
    ut_TEP.utFocusKey           = 0;
    ut_TEP.utTExecStdSql        = NULL;
    ut_TEP.utTExecStdSqlX       = NULL;
    ut_TEP.utTableSearchCb      = search_tebearb;
	ut_TEP.utTableCb            = UniTableCbTEP;
    ut_TEP.utCalldata           = NULL;
    ut_TEP.utConnectNoEle       = 0;
    ut_TEP.utConnectArray       = NULL;
    ut_TEP.utMagic              = UNIMAGIC;

    memset (&ut_TEKPRN, 0, sizeof (UNITABLE));

    ut_TEKPRN.utName               = TN_TEKPRN;
    ut_TEKPRN.utLabel              =
        FrmwrkGIf_GetMenuTitle (MlMsg ("GD138 TE-Druckaufträge bearbeiten"));
    ut_TEKPRN.utMaskName           = "ME_TEKPRN";
    ut_TEKPRN.utAlterMaskName      = NULL;
    ut_TEKPRN.utSelectTableName    = TN_TEKPRN;
    ut_TEKPRN.utInsertTableName    = TN_TEKPRN;
    ut_TEKPRN.utUpdateTableName    = TN_TEKPRN;
    ut_TEKPRN.utDeleteTableName    = TN_TEKPRN;
    ut_TEKPRN.utStructName         = SN_TEKPRN;
    ut_TEKPRN.utStructSize         = sizeof (TEKPRN);
    ut_TEKPRN.utMaskMatch          = "*-TEKPRN-*";
    ut_TEKPRN.utFocusField         = TEKPRN_TeId_t;
    ut_TEKPRN.utFocusKey           = 0;
    ut_TEKPRN.utTExecStdSql        = NULL;
    ut_TEKPRN.utTExecStdSqlX       = NULL;
    ut_TEKPRN.utTableSearchCb      = search_tebearb;
    ut_TEKPRN.utTableCb            = UniTableCbTEKPRN;
    ut_TEKPRN.utCalldata           = NULL;
    ut_TEKPRN.utConnectNoEle       = 0;
    ut_TEKPRN.utConnectArray       = NULL;
    ut_TEKPRN.utMagic              = UNIMAGIC;

	memset (uc_TEK_TEP, 0, sizeof (uc_TEK_TEP));
    uc_TEK_TEP[0].ucName                   = "TEK_TEP";
    uc_TEK_TEP[0].ucLabel                  =
		FrmwrkGIf_GetMenuTitle 
				(MlMsg ("GD081 Transporteinheit Position bearbeiten"));
    uc_TEK_TEP[0].ucLBCName                = MD_TEP_LBC_NAME;
    uc_TEK_TEP[0].ucLBCHeaderMaskName      = "LB_TEP";
    uc_TEK_TEP[0].ucLBCBodyMaskName        = "LB_TEP";
    uc_TEK_TEP[0].ucLBCFooterMaskName      = NULL;
    uc_TEK_TEP[0].ucLBCHeaderAlterMaskName = NULL;
    uc_TEK_TEP[0].ucLBCBodyAlterMaskName   = NULL;
    uc_TEK_TEP[0].ucLBCFooterAlterMaskName = NULL;
    uc_TEK_TEP[0].ucSelectTableName        = TN_TEP;
    uc_TEK_TEP[0].ucInsertTableName        = TN_TEP;
    uc_TEK_TEP[0].ucDeleteTableName        = TN_TEP;
    uc_TEK_TEP[0].ucUpdateTableName        = TN_TEP;
    uc_TEK_TEP[0].ucStructName             = SN_TEP;
    uc_TEK_TEP[0].ucStructSize             = sizeof (TEP);
    uc_TEK_TEP[0].ucLBCMaskMatch           = "*-TEP-*";
    uc_TEK_TEP[0].ucParentJoinFields       = PK_TEK;
    uc_TEK_TEP[0].ucJoinFields             = PK_TEK;
    uc_TEK_TEP[0].ucOrderByFields          = PK_TEP;
    uc_TEK_TEP[0].ucTExecStdSqlX           = UniConnectTExecStdSqlX_tebearb;
    uc_TEK_TEP[0].ucConnectCb              = NULL;
    uc_TEK_TEP[0].ucCalldata               = NULL;
    uc_TEK_TEP[0].ucTable                  = &ut_TEP;
    uc_TEK_TEP[0].ucMagic                  = UNIMAGIC;

    uc_TEK_TEP[1].ucName                   = "TEKPRN";
    uc_TEK_TEP[1].ucLabel                  =
		FrmwrkGIf_GetMenuTitle (MlMsg ("GD138 TE-Druckaufträge bearbeiten"));
    uc_TEK_TEP[1].ucLBCName                = "TEKPRN";
    uc_TEK_TEP[1].ucLBCHeaderMaskName      = "LB_TEKPRN";
    uc_TEK_TEP[1].ucLBCBodyMaskName        = "LB_TEKPRN";
    uc_TEK_TEP[1].ucLBCFooterMaskName      = NULL;
    uc_TEK_TEP[1].ucLBCHeaderAlterMaskName = NULL;
    uc_TEK_TEP[1].ucLBCBodyAlterMaskName   = NULL;
    uc_TEK_TEP[1].ucLBCFooterAlterMaskName = NULL;
    uc_TEK_TEP[1].ucSelectTableName        = TN_TEKPRN;
    uc_TEK_TEP[1].ucInsertTableName        = TN_TEKPRN;
    uc_TEK_TEP[1].ucDeleteTableName        = TN_TEKPRN;
    uc_TEK_TEP[1].ucUpdateTableName        = TN_TEKPRN;
    uc_TEK_TEP[1].ucStructName             = SN_TEKPRN;
    uc_TEK_TEP[1].ucStructSize             = sizeof (TEKPRN);
    uc_TEK_TEP[1].ucLBCMaskMatch           = "*-TEKPRN-*";
    uc_TEK_TEP[1].ucParentJoinFields       = PK_TEK;
    uc_TEK_TEP[1].ucJoinFields             = PK_TEK;
    uc_TEK_TEP[1].ucOrderByFields          = PK_TEKPRN;
    uc_TEK_TEP[1].ucTExecStdSqlX           = NULL;
    uc_TEK_TEP[1].ucConnectCb              = NULL;
    uc_TEK_TEP[1].ucCalldata               = NULL;
    uc_TEK_TEP[1].ucTable                  = &ut_TEKPRN;
    uc_TEK_TEP[1].ucMagic                  = UNIMAGIC;

	memset (&ut_TEK, 0, sizeof (UNITABLE));
    ut_TEK.utName               = TN_TEK;
    ut_TEK.utLabel              =
		FrmwrkGIf_GetMenuTitle (MlMsg (ME_TEBEARB_TITLE));
    ut_TEK.utMaskName           = "ME_TEK_TEP";
    ut_TEK.utAlterMaskName      = NULL;
    ut_TEK.utSelectTableName    = TN_TEK;
    ut_TEK.utInsertTableName    = TN_TEK;
    ut_TEK.utUpdateTableName    = TN_TEK;
    ut_TEK.utDeleteTableName    = TN_TEK;
    ut_TEK.utStructName         = SN_TEK;
    ut_TEK.utStructSize         = sizeof (TEK);
    ut_TEK.utMaskMatch          = "*-TEK-*";
    ut_TEK.utFocusField         = "TEK_TeId_t";
    ut_TEK.utFocusKey           = 0;
    ut_TEK.utTExecStdSql        = lUniTExecStdSql_tebearb;
    ut_TEK.utTExecStdSqlX       = NULL;
    ut_TEK.utTableSearchCb      = search_tebearb;
	ut_TEK.utTableCb            = UniTableCbTEK ;
    ut_TEK.utCalldata           = &stTeBearbCalldata;
    ut_TEK.utConnectNoEle       = 2;
    ut_TEK.utConnectArray       = uc_TEK_TEP;
    ut_TEK.utMagic              = UNIMAGIC;

	memset (&um_TEK, 0, sizeof (UNIMENU));
    um_TEK.umName               = "TE BEARBEITEN";
    um_TEK.umLabel              =
		FrmwrkGIf_GetMenuTitle (MlMsg (ME_TEBEARB_TITLE));
    um_TEK.umRootTable          = &ut_TEK;
    um_TEK.umTSqlCommit         = UniTableTSqlCommit_tebearb;
    um_TEK.umTSqlRollback       = NULL;
    um_TEK.umMenuCb             = NULL;
    um_TEK.umCalldata           = hParentEle;
    um_TEK.umMagic              = UNIMAGIC;

    UniMenu (parent, &um_TEK, TN_TEK, ptRec);
	
    return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Entrypoint for Tebearb
-* RETURNS
-*--------------------------------------------------------------------------*/
void me_tebearb (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                int iReason, void *pvCbc)
{
    switch (iReason) {
    case FCB_XF:
        entry_tebearb (GetRootShell (), NULL, OriginalCaller_Standard, NULL, 0, 0);
        break;

    default:
        break;
    }

    return;
}
/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*    interface
-* RETURNS
-*-------------------------------------------------------------------------*/
int _StockbaseGIf_Korrektur(OWidget parent, char *pcTeId, KRETK *ptKretk,
		long lNeutraleInvNr, long lNeutralePosNr)
{
    if (pcTeId == NULL) {
        return GeneralTerrArgs;
    }
	TEK		tTek = {};

	if (IsEmptyString(pcTeId)) {
		/* open an empty GD071 if no TeId has been specified */
		entry_tebearb (parent, NULL, OriginalCaller_Standard, ptKretk,
				lNeutraleInvNr, lNeutralePosNr);
	} else {
		FRMWRK_STRNCPY (tTek.tekTeId, pcTeId, TEID_LEN);
		entry_tebearb (parent, &tTek, OriginalCaller_Standard, ptKretk,
				lNeutraleInvNr, lNeutralePosNr);
	}

    return(0);
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*    interface
-* RETURNS
-*-------------------------------------------------------------------------*/
int _StockbaseGIf_Korrektur_GI (OWidget parent, char *pcTeId)
{
    if (pcTeId == NULL) {
        return GeneralTerrArgs;
    }

	TEK		tTek = {};
    FRMWRK_STRNCPY (tTek.tekTeId, pcTeId, TEID_LEN);

	entry_tebearb (parent, &tTek, OriginalCaller_GI, NULL, 0, 0);

    return(0);
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*    interface
-* RETURNS
-*-------------------------------------------------------------------------*/
int _StockbaseGIf_Korrektur_GO (OWidget parent, char *pcTeId)
{
    if (pcTeId == NULL) {
        return GeneralTerrArgs;
    }

	TEK		tTek = {};
    FRMWRK_STRNCPY (tTek.tekTeId, pcTeId, TEID_LEN);

	entry_tebearb (parent, &tTek, OriginalCaller_GO, NULL, 0, 0);

    return(0);
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*    see .h file in interface
-* RETURNS
-*-------------------------------------------------------------------------*/
int _StockbaseGIf_RegUESetEfByArtMeTeBearb
						(f_UESetEfByArtMeTeBearb xUESetEfByArtMeTeBearb)
{
	mxUESetEfByArtMeTeBearb = xUESetEfByArtMeTeBearb;
	return (0);
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*    reading FTT for given FachtId
-* RETURNS
-*-------------------------------------------------------------------------*/
int getFtt (const void* pvTid, char const *pcFac, char *pcFachtId, FTT *ptFtt)
{
	int iRv;
	FTT tFtt = {};


	if (pcFachtId == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, "pcFachtId is NULL");
		return OpmsgSIf_ErrPush (GeneralTerrArgs, NULL);
	}

	FRMWRK_STRARRNCPY (tFtt.fttFachtId, pcFachtId);

	iRv = TExecStdSql(pvTid, StdNselect, TN_FTT, &tFtt);
	if (iRv != 1) {
		LoggingSIf_PkLogPrintf (pcFac, LOGGING_SIF_ALERT,
				pvTid, TN_FTT, &tFtt, NULL);
		return (OpmsgSIf_ErrPush (GeneralTerrDb, "%s : %s",
				LoggingSIf_GetLogPk (pvTid, TN_FTT, &tFtt),
				TSqlErrTxt (pvTid)));
	}
	*ptFtt = tFtt;
	return 1;
}

