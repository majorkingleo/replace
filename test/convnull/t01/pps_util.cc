/**
 * @file
 * @author Copyright (c) 2008 Salomon Automation GmbH
 */

#include <math.h>

#include <array.h>
#include <dbsqlstd.h>
#include <mlpack.h>
#include <hist_util.h>
#include <sqlstring.h>
#include <sstring.h>

#include <if_logging_s.h>
#include <if_opmsg_s.h>
#include <if_frmwrk_s.h>
#include <if_routing_s.h>
#include <if_fwrefnr_s.h>
#include <if_slobase_s.h>
#include <if_wpgobase_s.h>

#include <ARTE.h>
#include <P_VPLK.h>
#include <P_VPLP.h>
#include <GEWKS.h>
#include <HANDKS.h>
#include <LBPKOM.h>
#include <LBPKOMPRM.h>
#include <RBNODE.h>
#include <IV_KETP.h>
#include <TMSP.h>

#include <TOURP.h>
#include <P_TOURP.h>

#include "wpvplutil.h"
#include "wpgobase_routeutil.h"

#include "pps_util.h"

#define DATASOURCE_NORMAL 1
#define DATASOURCE_PROTOCOL 2

#define BLOCKSIZE 50

typedef struct _SollZeitStructKs {
    VPLK		tVplk;
	LBPKOMPRM	tLbpKomPrm;
	ArrayPtr	hArtE;
	ArrayPtr	hArt;
	ArrayPtr	hVplp;
	ArrayPtr	hFesp;
	TMSK		tTmsk; 
    int			bDefaultTmskFound;
	int			iAnzVplpMitThm;
	int			iAnzVplp;  
	int			iDataSource; 
	char		*pcUser;
	long		lKsNrOrProtNr;	  
	long		lPathLenght;	  
	double		dSumThmLength;
} SollZeitStructKs;

static ArrayPtr hTmsK	= NULL;	
static ArrayPtr hGewKs	= NULL;
static ArrayPtr hHandKs	= NULL;

/**
 * Gets the given stacker's GabelLaenge.
 *
 * @see if_wpgobase_s.h
 */
int _WpgobaseSIf_GetStpGabelLaengeByTpmId(const void *pvTid, const char *pcFac,
							const char *pcTpmId, long *plGabelLaenge)
{
	long	lGabelLaenge = 0;
	int		iRv = 0;

	if (IsEmptyStrg(pcTpmId) || plGabelLaenge == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, "Invalid arguments");
		return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL);
	}

	/* Statement returns the sum length of all gabeln of the given stacker */
	iRv = TExecSql (pvTid,
		"SELECT sum("TCN_TMSP_GabelLaenge")"
		 " FROM "TN_TMSP
		" WHERE "TCN_TMSP_TpmId" = :TpmId",
		SELLONG   (lGabelLaenge),
		SQLSTRING (pcTpmId),
		NULL);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,	
					"Error reading from TMSP. TpmId[%s] %s",
					pcTpmId, TSqlErrTxt (pvTid));
		*plGabelLaenge = 0;
		return OpmsgSIf_ErrPush(GeneralTerrDb, NULL);
	}

	*plGabelLaenge = lGabelLaenge;

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*   int GetVplkByKsNr
-* DESCRIPTION
-*          function get VPLK by KsNr
-*		
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		ptSoll.........The soll struct
-*		
-* RETURNS 
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int GetVplkByKsNr	(void *pvTid, const char *pcFac,
							SollZeitStructKs *ptSoll)
{
	/* Definition */
	P_VPLK	tP_Vplk;
	int		iRv		=	0;		

	/* Precondition */
	if (ptSoll == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, 
			"GetVplkByKsNr:Precondition failed.");
	}

	/* Initialisation */
	memset (&tP_Vplk, 0, sizeof (tP_Vplk));

	/* Algorithm */
	if (ptSoll->iDataSource == DATASOURCE_NORMAL) {
		ptSoll->tVplk.vplkKsNr = ptSoll->lKsNrOrProtNr;
		tP_Vplk.p_vplkProtNr = ptSoll->lKsNrOrProtNr;
		iRv = TExecStdSql (pvTid, StdNselect, 
								TN_VPLK, &ptSoll->tVplk);
		if (iRv < 1) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"GetVplkByKsNr: TExecStdSql failed. %s",
						TSqlErrTxt(pvTid));
			return -1;
		}
	} else {
		/* Get data from protokoll */
		tP_Vplk.p_vplkProtNr = ptSoll->lKsNrOrProtNr;
		iRv = TExecStdSql (pvTid, StdNselect, TN_P_VPLK, &tP_Vplk);
		if (iRv < 1) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"GetVplkByKsNr: TExecStdSql failed. %s",
						TSqlErrTxt(pvTid));
			return -1;
		}
		ptSoll->tVplk = tP_Vplk.p_vplkVplk;
	}

    return 1;
} /* GetVplkByKsNr */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		ptSoll.........The soll struct
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int GetVplpAndArtByKsNr	(void *pvTid, const char *pcFac, 
								SollZeitStructKs *ptSoll)
{
	/* Declaration */
	int			iRv			=	0;
	int			iRvFunc		=	0;
	int			iDbRvX		=	0;
	int			nI			=	0;
	ART			aArt		[BLOCKSIZE];
	VPLP		aVplp		[BLOCKSIZE];
	P_VPLP		aP_Vplp		[BLOCKSIZE];
	FESP		aFesp		[BLOCKSIZE];

	/* Initialisation */
	ptSoll->hArt = ArrCreate (sizeof (ART), BLOCKSIZE, NULL, NULL);
	if (ptSoll->hArt == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "GetVplpAndArtByKsNr: ArrCreate failed");
		return -1;
	}

	ptSoll->hVplp = ArrCreate (sizeof (VPLP), BLOCKSIZE, NULL, NULL);
	if (ptSoll->hVplp == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "GetVplpAndArtByKsNr: ArrCreate failed");
		return -1;
	}

	ptSoll->hFesp = ArrCreate (sizeof (FESP), BLOCKSIZE, NULL, NULL);
	if (ptSoll->hFesp == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "GetVplpAndArtByKsNr: ArrCreate failed");
		return -1;
	}

	/* Algorithm */
	/* Load ART */
	do {
		memset (aArt, 0, sizeof (aArt)); 
		memset (aFesp, 0, sizeof (aFesp)); 
		if (ptSoll->iDataSource == DATASOURCE_NORMAL) {
			memset (aVplp, 0, sizeof (aVplp)); 
		} else {
			memset (aP_Vplp, 0, sizeof (aP_Vplp)); 
		}
		if (nI == 0) {
			iDbRvX = (ptSoll->iDataSource == DATASOURCE_NORMAL) 
				? TExecSqlX (pvTid, NULL,
					"select %"TN_VPLP", %"TN_ART", %"TN_FESP
					" from "TN_VPLP", "TN_ART", "TN_FESP
					" where "TCN_VPLP_VPLK_KsNr" = :ksnr "
					" AND " TCN_ART_AId_Mand  "(+) = " TCN_VPLP_MId_AId_Mand 
					" AND " TCN_ART_AId_ArtNr "(+) = " TCN_VPLP_MId_AId_ArtNr 
					" AND " TCN_ART_AId_Var   "(+) = " TCN_VPLP_MId_AId_Var 
					" AND "	TCN_ART_AId_AusPr "(+) = " TCN_VPLP_MId_AId_AusPr
					" and " TCN_FESP_FeldBez" = "
						" (CASE WHEN SubStr(NVL("TCN_VPLP_KOMZ_FeldKomGrpId",' '),0,1)<>' ' THEN "
							" (SELECT "TCN_FESP_FeldBez" FROM "TN_FESP
							" WHERE "TCN_VPLP_KOMZ_FeldKomGrpId" = "TCN_FESP_FELDKOMGRP_FeldKomGrpId
							" AND rownum = 1) "
						" ELSE "
							" (SELECT "TCN_FESP_FeldBez" FROM "TN_FESP
							" WHERE "TCN_VPLP_KomPos_FeldId" = "TCN_FESP_FeldId
							" AND NVL("TCN_VPLP_KomPos_Offs_L_X",0) >= "TCN_FESP_Offs_L_X
							" AND NVL("TCN_VPLP_KomPos_Offs_L_X",0) < "TCN_FESP_Offs_L_X"+"TCN_FESP_Vol_L_X
							" AND NVL("TCN_VPLP_KomPos_Offs_L_Y",0) >= "TCN_FESP_Offs_L_Y
							" AND NVL("TCN_VPLP_KomPos_Offs_L_Y",0) < "TCN_FESP_Offs_L_Y"+"TCN_FESP_Vol_L_Y
							" AND NVL("TCN_VPLP_KomPos_Offs_L_Z",0) >= "TCN_FESP_Offs_L_Z
							" AND NVL("TCN_VPLP_KomPos_Offs_L_Z",0) < "TCN_FESP_Offs_L_Z"+"TCN_FESP_Vol_L_Z
							" AND rownum = 1) "
						" END) "
					" order by "TCN_VPLP_KomRfNr,
					BLOCKSIZE,0,
					SELSTRUCT (TN_VPLP, aVplp [0]),
					SELSTRUCT (TN_ART, aArt [0]),
					SELSTRUCT (TN_FESP, aFesp[0]),
					SQLLONG (ptSoll->lKsNrOrProtNr),
					NULL)
				: TExecSqlX (pvTid, NULL,
					"SELECT %"TN_P_VPLP", %"TN_ART", %"TN_FESP
					" FROM "TN_P_VPLP", "TN_ART", "TN_FESP
					" WHERE " TCN_P_VPLP_ProtNr " = :ksnr "
					" AND " TCN_ART_AId_Mand  " = "TCN_P_VPLP_Vplp_MId_AId_Mand
					" AND " TCN_ART_AId_ArtNr " = "TCN_P_VPLP_Vplp_MId_AId_ArtNr
					" AND " TCN_ART_AId_Var   " = "TCN_P_VPLP_Vplp_MId_AId_Var
					" AND "	TCN_ART_AId_AusPr " = "TCN_P_VPLP_Vplp_MId_AId_AusPr
					" and " TCN_FESP_FeldBez" = "
						" (CASE WHEN SubStr(NVL("TCN_P_VPLP_Vplp_KOMZ_FeldKomGrpId",' '),0,1)<>' ' THEN "
							" (SELECT "TCN_FESP_FeldBez" FROM "TN_FESP
							" WHERE "TCN_P_VPLP_Vplp_KOMZ_FeldKomGrpId" = "TCN_FESP_FELDKOMGRP_FeldKomGrpId
							" AND rownum = 1) "
						" ELSE "
							" (SELECT "TCN_FESP_FeldBez" FROM "TN_FESP
							" WHERE "TCN_P_VPLP_Vplp_KomPos_FeldId" = "TCN_FESP_FeldId
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_X",0) >= "TCN_FESP_Offs_L_X
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_X",0) < "TCN_FESP_Offs_L_X"+"TCN_FESP_Vol_L_X
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_Y",0) >= "TCN_FESP_Offs_L_Y
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_Y",0) < "TCN_FESP_Offs_L_Y"+"TCN_FESP_Vol_L_Y
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_Z",0) >= "TCN_FESP_Offs_L_Z
							" AND NVL("TCN_P_VPLP_Vplp_KomPos_Offs_L_Z",0) < "TCN_FESP_Offs_L_Z"+"TCN_FESP_Vol_L_Z
							" AND rownum = 1) "
						" END) "
					" order by "TCN_P_VPLP_Vplp_KomRfNr,
					BLOCKSIZE,0,
					SELSTRUCT (TN_P_VPLP, aP_Vplp [0]),
					SELSTRUCT (TN_ART, aArt[0]),
					SELSTRUCT (TN_FESP, aFesp[0]),
					SQLLONG (ptSoll->lKsNrOrProtNr),
					NULL);
		} else {
			iDbRvX = TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);
		}

		/* Check result */
		if (iDbRvX <= 0 && (nI == 0 || TSqlError(pvTid) != SqlNotFound)) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				 "GetVplpAndArtByKsNr: TSqlErrTxt:%s", 
				 TSqlErrTxt (pvTid));
			iRv = -1; 
			break;
		}

		/* Copy ART */
		for (nI = 0; nI < iDbRvX; nI++) {
			iRvFunc = ArrAddElem(ptSoll->hArt, &aArt [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"GetVplpAndArtByKsNr: ArrAddElem failed");
				iRv = -1; 
				break;
			}
			iRvFunc = ArrAddElem(ptSoll->hFesp, &aFesp [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"GetVplpAndArtByKsNr: ArrAddElem failed");
				iRv = -1; 
				break;
			}
		}

		if (ptSoll->iDataSource == DATASOURCE_NORMAL) {
			/* Copy VPLP */
			for (nI = 0; nI < iDbRvX; nI++) {
				/* This is a nice place to count the ART of THM-type */
				if (aVplp[nI].vplpMId.ThmKz == JANEIN_J) {
					ptSoll->iAnzVplpMitThm++;
				}

				iRvFunc = ArrAddElem(ptSoll->hVplp, &aVplp [nI]);
				if (iRvFunc < 0) {
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
						"GetVplpAndArtByKsNr: ArrAddElem failed");
					iRv = -1; 
					break;
				}
			}
		} else {
			/* Copy P_VPLP */
			for (nI = 0; nI < iDbRvX; nI++) {
				/* This is a nice place to count the ART of THM-type */
				if (aP_Vplp[nI].p_vplpVplp.vplpMId.ThmKz == JANEIN_J) {
					ptSoll->iAnzVplpMitThm++;
				}

				iRvFunc = ArrAddElem(ptSoll->hVplp, &aP_Vplp [nI].p_vplpVplp);
				if (iRvFunc < 0) {
					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
						"GetVplpAndArtByKsNr: ArrAddElem failed");
					iRv = -1; 
					break;
				}
			}
		}
	} while (iDbRvX == BLOCKSIZE && iRv >= 0);

	if (iRv < 0) {
		/* Shit happend */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							  "GetVplpAndArtByKsNr: failed");
		iRv = -1;
	} else {
		ptSoll->iAnzVplp = (int) ArrGet (ptSoll->hVplp, ArrNnoEle);
		iRv = 1;
	}

	return iRv;
} /* GetVplpAndArtByKsNr */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of ARTE
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareARTE (void *pv1, void *pv2, void *pvCb)
{
    ARTE 	*pt1    = (ARTE *)pv1;
    ARTE 	*pt2    = (ARTE *)pv2;
	int		iRv        = 0;

	if (pt1->arteAId.AusPr > pt2->arteAId.AusPr) {
		return (1);
    }
	if (pt1->arteAId.AusPr < pt2->arteAId.AusPr) {
		return (-1);
    }

	iRv = strcmp (pt1->arteAId.ArtNr, pt2->arteAId.ArtNr);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

	iRv = strcmp (pt1->arteAId.Var, pt2->arteAId.Var);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

	iRv = strcmp (pt1->arteAId.Mand, pt2->arteAId.Mand);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

	/* Don't move this comparison. Compare Einheit at last.
		So all Items of a ART are one after another */
	iRv = strcmp (pt1->arteEinheit, pt2->arteEinheit);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }
    return 0;
} /* CompareARTE */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		ptSoll.........The soll struct
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int GetArtEByKsNr	(void *pvTid, const char *pcFac, 
							SollZeitStructKs *ptSoll)
{
	int			iRv			=	0;
	int			iRvFunc		=	0;
	int			iDbRvX		=	0;
	int			nI			=	0;
	ARTE		aArtE [BLOCKSIZE];

	/* Initialisation */
	ptSoll->hArtE = ArrCreate (sizeof (ARTE), BLOCKSIZE, CompareARTE, NULL);
	if (ptSoll->hArtE == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "GetArtEByKsNr: ArrCreate failed");
		return -1;
	}

	/* Algorithm */
	/* Load ARTE */
	do {
		memset (aArtE, 0, sizeof (aArtE)); 
		if (nI == 0) {
			iDbRvX = (ptSoll->iDataSource == DATASOURCE_NORMAL) 
				? TExecSqlX (pvTid, NULL,
					" SELECT distinct %" TN_ARTE 
					" FROM   " TN_ARTE " , " TN_VPLP
					" WHERE " TCN_VPLP_VPLK_KsNr " = :ksnr "
					" AND " TCN_ARTE_AId_Mand  " = " TCN_VPLP_MId_AId_Mand 
					" AND " TCN_ARTE_AId_ArtNr " = " TCN_VPLP_MId_AId_ArtNr 
					" AND " TCN_ARTE_AId_Var   " = " TCN_VPLP_MId_AId_Var 
					" AND "	TCN_ARTE_AId_AusPr " = " TCN_VPLP_MId_AId_AusPr,
					BLOCKSIZE,0,
					SELSTRUCT (TN_ARTE, aArtE[0]),
					SQLLONG (ptSoll->lKsNrOrProtNr),
					 NULL)
				: TExecSqlX (pvTid, NULL,
					" SELECT distinct %" TN_ARTE 
					" FROM   " TN_ARTE " , " TN_P_VPLP
					" WHERE " TCN_P_VPLP_ProtNr " = :ksnr "
					" AND " TCN_ARTE_AId_Mand  " = " 
							TCN_P_VPLP_Vplp_MId_AId_Mand 
					" AND " TCN_ARTE_AId_ArtNr " = " 
							TCN_P_VPLP_Vplp_MId_AId_ArtNr 
					" AND " TCN_ARTE_AId_Var   " = " 
							TCN_P_VPLP_Vplp_MId_AId_Var 
					" AND "	TCN_ARTE_AId_AusPr " = " 
							TCN_P_VPLP_Vplp_MId_AId_AusPr,
					BLOCKSIZE,0,
					SELSTRUCT (TN_ARTE, aArtE[0]),
					SQLLONG (ptSoll->lKsNrOrProtNr),
					NULL);
		} else {
			iDbRvX = TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);
		}

		if (iDbRvX <= 0 && TSqlError(pvTid) != SqlNotFound) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				 "GetArtEByKsNr: TSqlErrTxt:%s", 
				 TSqlErrTxt (pvTid));
			iRv = -1; 
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			iRvFunc = ArrAddElem(ptSoll->hArtE, &aArtE [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"GetArtEByKsNr: ArrAddElem failed");
				iRv = -1; 
				break;
			}
		}
	} while (iDbRvX == BLOCKSIZE);

	/* Sort Array */
	if (iRv >= 0) {
		iRvFunc = ArrSort (ptSoll->hArtE, CompareARTE, NULL); 
		if (iRvFunc < 0) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
				"GetArtEByKsNr: ArrSort failed");
			iRv = -1;  
		}
	}

	if (iRv < 0) {
		/* Shit happend */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							  "GetArtEByKsNr: failed");
		iRv = -1;
	} else {
		iRv = 1;
	}

	return iRv;
} /* GetArtEByKsNr */

static int GetSizeOfThmsByKsNrOrProtNr(
	const void 			*pvTid,
	const char			*pcFac,
	SollZeitStructKs	*ptSoll)
{
	const char			*pcFctName = "GetSizeOfThmsByksNrOrProtNr";
	int					iRv = 0;
	char				*pcStmt = NULL;

	if (ptSoll->iDataSource == DATASOURCE_NORMAL) {
		if (PStringCreateF(&pcStmt, 
				"SELECT sum("TCN_TTS_MaxLadeBreite")"
				 " FROM "TN_TOURP", "TN_TTS
				" WHERE "TCN_TOURP_TTS_TetId" = "TCN_TTS_TetId
				  " AND "TCN_TOURP_TourpNr" IN"
					" (SELECT DISTINCT "TCN_VPLP_Soll_TourpNr" FROM "TN_VPLP 
							" WHERE "TCN_VPLP_VPLK_KsNr" = :KsNr)") == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"%s: Error creating Statement",
						pcFctName);
			return -1;
		}
	} else {
		if (PStringCreateF(&pcStmt, 
				"SELECT sum("TCN_TTS_MaxLadeBreite")"
				 " FROM "TN_P_TOURP", "TN_TTS
				" WHERE "TCN_P_TOURP_Tourp_TTS_TetId" = "TCN_TTS_TetId
				  " AND "TCN_P_TOURP_Tourp_TourpNr" IN"
					" (SELECT DISTINCT "TCN_P_VPLP_Vplp_Soll_TourpNr
					  " FROM "TN_P_VPLP" WHERE "TCN_P_VPLP_ProtNr" = :ProtNr)"
				  ) == NULL) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"%s: Error creating Statement",
						pcFctName);
			return -1;
		}
	}

	iRv = TExecSql (pvTid, 
					pcStmt, 
					SELDOUBLE(ptSoll->dSumThmLength),
					SQLLONG(ptSoll->lKsNrOrProtNr),
					NULL);
	if (iRv < 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"GetVplkByKsNr: TExecStdSql failed. %s",
					TSqlErrTxt(pvTid));
		return -1;
	}

	if (pcStmt != NULL) {
		PStringDestroy(&pcStmt);
		pcStmt = NULL;
	}

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		ptSoll.........The soll struct
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int CalculateLadezeit (void *pvTid, const char *pcFac,
						double *pdResult,
						SollZeitStructKs *ptSoll)
{
	/* Declaration */
	int		iRv		=	0;
	int		lSum	=	0; 

	LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
			 "<CalculateLadezeit>. DataSource: %d", 
			ptSoll->iDataSource); 

	/* Algorithm */	
	if (ptSoll->iDataSource == DATASOURCE_NORMAL) {
		iRv =  TExecSql (pvTid,
			" SELECT SUM (" TCN_LBPKOM_LadeZeit " * IV.CNT_TTS) "
			" FROM   " TN_TTS " , " TN_LBPKOM " , "
			" ( "
			"	SELECT TETID, COUNT (TETID) CNT_TTS "
			"	FROM "
			"   ( "
			"		SELECT DISTINCT "	TOURP_TourpNr   " , " 
										TOURP_TTS_TetId " as TETID "
			"		FROM  " TN_VPLP " , " TN_TOURP
			"       WHERE " TCN_VPLP_Soll_TourpNr " = " TCN_TOURP_TourpNr
			"		AND   " TCN_VPLP_VPLK_KsNr     " = :ksnr "
			"	)  "
			"	GROUP BY TETID "
			") IV	"
			" WHERE " TCN_TTS_THM_ThmId " = " TCN_LBPKOM_ThmId
			" AND " TCN_LBPKOM_LagIdKom " = :lagid "
			" AND " TCN_TTS_TetId " = IV.TETID ",
			SELLONG (lSum),
			SQLLONG (ptSoll->lKsNrOrProtNr),
			SQLSTRING (ptSoll->tVplk.vplkLBKOM_LagIdKom),
			NULL);
	 } else {
		iRv =  TExecSql (pvTid,
			" SELECT SUM (" TCN_LBPKOM_LadeZeit " * IV.CNT_TTS) "
			" FROM   " TN_TTS " , " TN_LBPKOM " , "
			" ( "
			"	SELECT TETID, COUNT (TETID) CNT_TTS "
			"	FROM "
			"   ( "
			"		SELECT DISTINCT "	P_TOURP_Tourp_TourpNr	" , " 
										P_TOURP_Tourp_TTS_TetId " as TETID "
			"		FROM " TN_P_VPLP " , " TN_P_TOURP
			"		WHERE " TCN_P_VPLP_Vplp_Soll_TourpNr     " = " 
							TCN_P_TOURP_Tourp_TourpNr            
			"		AND   " TCN_P_VPLP_ProtNr " = :protnr "
			"	)  "
			"	GROUP BY TETID "
			") IV	"
			" WHERE " TCN_TTS_THM_ThmId " = " TCN_LBPKOM_ThmId
			" AND " TCN_LBPKOM_LagIdKom " = :lagid "
			" AND " TCN_TTS_TetId " = IV.TETID ",
			SELLONG (lSum),
			SQLLONG (ptSoll->lKsNrOrProtNr),
			SQLSTRING (ptSoll->tVplk.vplkLBKOM_LagIdKom), 
			NULL);
	}
	if (iRv > 0 && lSum <= 0) {
		iRv =  TExecSql (pvTid,
			" SELECT SUM (" TCN_LBPKOM_LadeZeit " * IV.CNT_TTS) "
			" FROM   " TN_TTS " , " TN_LBPKOM " , "
			" ( "
			"	SELECT TETID, COUNT (TETID) CNT_TTS "
			"	FROM "
			"   ( "
			"		SELECT DISTINCT "	TOURP_TourpNr	" , " 
										TOURP_TTS_TetId " as TETID "
			"		FROM " TN_P_VPLP " , " TN_TOURP
			"		WHERE " TCN_P_VPLP_Vplp_Soll_TourpNr     " = " 
							TCN_TOURP_TourpNr            
			"		AND   " TCN_P_VPLP_Vplp_VPLK_KsNr " = :ksnr "
			"	)  "
			"	GROUP BY TETID "
			") IV	"
			" WHERE " TCN_TTS_THM_ThmId " = " TCN_LBPKOM_ThmId
			" AND " TCN_LBPKOM_LagIdKom " = :lagid "
			" AND " TCN_TTS_TetId " = IV.TETID ",
			SELLONG (lSum),
			SQLLONG (ptSoll->lKsNrOrProtNr),
			SQLSTRING (ptSoll->tVplk.vplkLBKOM_LagIdKom), 
			NULL);
	}

	/* Check result */
	if (iRv <= 0) {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
			 "CalculateLadezeit: TSqlErrTxt:%s", 
			 TSqlErrTxt (pvTid));
		return -1; 
	} else {
        LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
			 "<CalculateLadezeit>. KsNr: %ld, LagIdKom: %s, SumZeit: %d", 
			ptSoll->lKsNrOrProtNr, ptSoll->tVplk.vplkLBKOM_LagIdKom, lSum); 
	}

	*pdResult = lSum;

	return (1);

} /* CalculateLadezeit */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*         Fill the struct SollZeitStructKs
-*--------------------------------------------------------------------------*/
static void FreeSollStruct (SollZeitStructKs *ptSoll)
{
	if (ptSoll->hVplp != NULL) {
		ArrDestroy (ptSoll->hVplp);
	}

	if (ptSoll->hFesp != NULL) {
		ArrDestroy (ptSoll->hFesp);
	}

	if (ptSoll->hArt != NULL) {
		ArrDestroy (ptSoll->hArt);
	}

	if (ptSoll->hArtE != NULL) {
		ArrDestroy (ptSoll->hArtE);
	}

	memset (ptSoll, 0, sizeof (*ptSoll));
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*         Fill the struct SollZeitStructKs
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int FillSollStructKs	(void *pvTid, const char *pcFac,
							long lKsNrOrProtNr, char *pcUser,
							SollZeitStructKs *ptSoll)
{
	/* Declaration */
	int		iRv		=	0;
	PPSPRM	*pPrm	=	&ptSoll->tLbpKomPrm.lbpkomprmpps_PRM; 

	ptSoll->lKsNrOrProtNr 	= lKsNrOrProtNr;
	ptSoll->pcUser 			= pcUser;

	/* Read VPLK */
	iRv = GetVplkByKsNr (pvTid, pcFac, ptSoll);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "<FillSollStructKs>. lKsNrOrProtNr: %ld, \n"
				   "GetVplkByKsNr failed !!!",
				   lKsNrOrProtNr);
		return -1;
	}

	 /* Read LbpKompPrm */
	strcpy (ptSoll->tLbpKomPrm.lbpkomprmLagIdKom,
			ptSoll->tVplk.vplkLBKOM_LagIdKom);
	ptSoll->tLbpKomPrm.lbpkomprmVplTyp = ptSoll->tVplk.vplkVplTyp;
	ptSoll->tLbpKomPrm.lbpkomprmKomArt = ptSoll->tVplk.vplkKomArt;

	iRv = TExecStdSql (pvTid, StdNselect, 
							TN_LBPKOMPRM, &ptSoll->tLbpKomPrm);
	if (iRv < 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"<FillSollStructKs>.Lagerbereichkommissionierparameter\n"
					"not found %s/%s/%s (Table: LBPKOMPRM) \n"
					"FillSollStructKs:%s",
					ptSoll->tVplk.vplkLBKOM_LagIdKom,
					(char *)l2sGetNameByValue ((LO2STR *)&l2s_VPLKTYP,
							ptSoll->tVplk.vplkVplTyp),
					(char *)l2sGetNameByValue ((LO2STR *)&l2s_KOMART,
							ptSoll->tVplk.vplkKomArt),
					TSqlErrTxt(pvTid));
		return -1;
	}

	 /* Read default TMSK */
	strcpy (ptSoll->tTmsk.tmskTpmId, pPrm->TpmId);
	iRv = TExecStdSql (pvTid, StdNselect, TN_TMSK, &ptSoll->tTmsk);
	if (iRv <= 0 ) {
		if (TSqlError(pvTid) != SqlNotFound) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
						"<FillSollStructKs>.GetDefaultTmsk. KsNr: %ld\n"
						"Error: %s ",
						ptSoll->tVplk.vplkKsNr,
						TSqlErrTxt(pvTid));

			return -1;
		} else {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY, 
						"<FillSollStructKs> No Default TPMId for Laz of "
						"KsNr: %ld found", ptSoll->tVplk.vplkKsNr);

			ptSoll->bDefaultTmskFound = 0;
		}
		/* No problem, if no TPM found */
	} else {
		ptSoll->bDefaultTmskFound = 1;
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY, 
				"<FillSollStructKs> Default TPMId for KsNr %ld is %s",
				 ptSoll->tVplk.vplkKsNr,
				 ptSoll->tTmsk.tmskTpmId);
	}

	/* Read VPLP and ART*/
	iRv = GetVplpAndArtByKsNr (pvTid, pcFac, ptSoll); 
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "FillSollStructKs. KsNr: %ld. "
				   "GetVplpAndArtByKsNr failed",
				   ptSoll->tVplk.vplkKsNr);
		return -1;
	}

	/* Read ARTE */
	iRv = GetArtEByKsNr (pvTid, pcFac, ptSoll);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "FillSollStructKs. KsNr: %ld, "
				   "GetArtEByKsNr failed",
				   ptSoll->tVplk.vplkKsNr);
		return -1;
	}

	/* Calculate the needed space of all THMs for this Picking Order */
	iRv = GetSizeOfThmsByKsNrOrProtNr(pvTid, pcFac, ptSoll);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "FillSollStructKs. KsNr: %ld, "
				   "GetSizeOfThmsByKsNrOrProtNr failed",
				   ptSoll->tVplk.vplkKsNr);
		return -1;
	}

	return 1;
} /* FillSollStructKs */

/**
 * Gets the first and the last LBKOM in the chain of the given VPLK.
 * First and last LBKOM may be the same.
 * 
 * @param[in]	pvTid		SQL's transaction ID
 * @param[in]	pkcFac		log facility
 * @param[in]	ktVplk		the VPLK
 * @param[out]	strFirst	LagIdKom of the first LBKOM
 * @param[out]	strLast		LagIdKom of the last LBKOM
 * 
 * @retval 1	Ok
 * @retval <0	Error
 * @retval	GeneralTerrNoDataFound
 * @retval	GeneralTerrDb
 ****************************************************************************/
static int getFirstAndLastLbkomOfVpl(void *pvTid, const char *pkcFac, const VPLK &ktVplk,
									 std::string &strFirst, std::string &strLast)
{
	int		iDbRv = 0;
	char	acFirst[LAGID_LEN+1] = {};
	char	acLast[LAGID_LEN+1] = {};

	iDbRv = TExecSql(pvTid,
				" SELECT ("
					// Get first LBKOM
					" SELECT "TCN_IV_KETP_LBKOM_LagIdKom
					" FROM "TN_VPLK
						", "TN_IV_KETP
					" WHERE "TCN_VPLK_KETK_KetId" = "TCN_IV_KETP_KetId
					  " AND "TCN_VPLK_KsNr" = :KsNr"
					  " AND "TCN_IV_KETP_Ketreihe" = ("
							" SELECT MIN("TCN_IV_KETP_Ketreihe")"
							" FROM "TN_VPLK
								", "TN_IV_KETP
							" WHERE "TCN_VPLK_KETK_KetId" = "TCN_IV_KETP_KetId
							  " AND "TCN_VPLK_KsNr" = :KsNr"
					  ")"
				" ), ("
					// Get last LBKOM
					" SELECT "TCN_IV_KETP_LBKOM_LagIdKom
					" FROM "TN_VPLK
						", "TN_IV_KETP
					" WHERE "TCN_VPLK_KETK_KetId" = "TCN_IV_KETP_KetId
					  " AND "TCN_VPLK_KsNr" = :KsNr"
					  " AND "TCN_IV_KETP_Ketreihe" = ("
							" SELECT MAX("TCN_IV_KETP_Ketreihe")"
							" FROM "TN_VPLK
								", "TN_IV_KETP
							" WHERE "TCN_VPLK_KETK_KetId" = "TCN_IV_KETP_KetId
							  " AND "TCN_VPLK_KsNr" = :KsNr"
					  ")"
				" ) FROM dual",
				SELSTR(acFirst, LAGID_LEN+1),
				SELSTR(acLast, LAGID_LEN+1),
				SQLLONG(ktVplk.vplkKsNr),
				SQLLONG(ktVplk.vplkKsNr),
				SQLLONG(ktVplk.vplkKsNr),
				SQLLONG(ktVplk.vplkKsNr),
				NULL);
	if (iDbRv <= 0) {
		LoggingSIf_SqlErrLogPrintf(pkcFac, LOGGING_SIF_ALERT, pvTid, 
				"Error reading LagIdKom of first and last LBKOM in chain");
		if (TSqlError(pvTid) != SqlNotFound) {
			return OpmsgSIf_ErrPush (GeneralTerrDb, "%s",
					TSqlPrettyErrTxt(pvTid));
		} else {
			return OpmsgSIf_ErrPush (GeneralTerrNoDataFound, "%s",
					TSqlPrettyErrTxt(pvTid));
		}
	} 

	strFirst = acFirst;
	strLast = acLast;

	return 1;
} /* getFirstAndLastLbkomOfVpl */

/**
 * see if_wpgobase_s.h
 ****************************************************************************/
int _WpgobaseSIf_GetAllNodesOfLbkom(void *pvTid, const char *pkcFac,
									const std::string &strLagIdKom,
									ArrayPtr hArrNodes,
									const WpgobaseTnodeType eNodeType)
{
	int		iRv, iDbRvX, nI;
	char	aacNodeName[BLOCKSIZE][RBNAME_LEN+1];

	iRv = 0;
	nI = 0;

	// input check
	if (hArrNodes == NULL) {
		LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT, "invalid arguments");
		return OpmsgSIf_ErrPush(GeneralTerrArgs, NULL);
	}

	std::string strStmt(
					" SELECT DISTINCT "TCN_RBNODE_NName
					" FROM "TN_FESP
						", "TN_RBNODE
					" WHERE "TCN_FESP_LBKOM_LagIdKom" = :lagid "
					" AND "TCN_RBNODE_GName" = "TCN_FESP_RBVPL_GName
					" AND "TCN_RBNODE_NName" = "TCN_FESP_RBVPL_NName
					" AND ("
			);
	if (eNodeType == WpgobaseNnodeTypeStart) {
		strStmt += TCN_RBNODE_KomAttr_START" = 1)";
	} else {
		strStmt += TCN_RBNODE_KomAttr_ENDE" = 1)";
	}

	do {
		memset(aacNodeName, 0, sizeof(aacNodeName));

		iDbRvX = (nI == 0)
			?  TExecSqlX(pvTid, NULL,
					strStmt.c_str(),
					BLOCKSIZE, 0, 
					SELSTR (aacNodeName[0], RBNAME_LEN+1),
					SQLSTRING (strLagIdKom.c_str()),
					NULL)
			:  TExecSqlV(pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX < BLOCKSIZE && TSqlError(pvTid) != SqlNotFound) {
			LoggingSIf_SqlErrLogPrintf(pkcFac, LOGGING_SIF_ALERT, pvTid,
								"Error reading start/end nodes in %s",
								strLagIdKom.c_str());
			return OpmsgSIf_ErrPush(GeneralTerrDb, "%s", TSqlErrTxt(pvTid));
		}

		if (iDbRvX <= 0) {
			if (nI == 0) {
				/* not any data found
				   */
				LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT, "No nodes found");
				return OpmsgSIf_ErrPush(GeneralTerrNoDataFound, NULL);
			}
			/* no additional data
			   */
			return 1;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			iRv = ArrAddElem(hArrNodes, aacNodeName[nI]);
			if (iRv < 0) {
				LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT,
						"ArrAddElem() failed %d", iRv);
				return OpmsgSIf_ErrPush(GeneralTerrInternal, NULL);
			}
		}

	} while(iDbRvX == BLOCKSIZE);

	return 1;
} /* _WpgobaseSIf_GetAllNodesOfLbkom */

/**
 * This funcion calculates the start node and the target position. 
 * It also calculates the lenght of the found route. 
 * If the routing was OK, ptSoll->tVplk.vplkEndPos and ptSoll->tVplk.vplkStart_NName will be filled.
 * @return 1 -> OK
 * @return <0 -> Error
 */
static int _FindStartNodeEndPosAndLength(void* pvTid, const char* pkcFac, 
								  SollZeitStructKs* ptSoll, long* plLength)
{
	int  iRv = 0;
	ArrayPtr hArrNodesStart = NULL;
	ArrayPtr hArrNodesEnd = NULL;

	hArrNodesStart = ArrCreate (sizeof(char) * (RBNAME_LEN+1), 0, NULL, NULL);
	if (hArrNodesStart == NULL) {
		LoggingSIf_LogPrintf (pkcFac, LOGGING_SIF_ALERT, "ArrCreate failed");
		return OpmsgSIf_ErrPush(GeneralTerrMemory, NULL);
	}
	hArrNodesEnd = ArrCreate (sizeof(char) * (RBNAME_LEN+1), 0, NULL, NULL);
	if (hArrNodesEnd == NULL) {
		LoggingSIf_LogPrintf (pkcFac, LOGGING_SIF_ALERT, "ArrCreate failed");
		return OpmsgSIf_ErrPush(GeneralTerrMemory, NULL);
	}

	/* First we need to find all start nodes of the first LBKOM and
	 * all end nodes of the last LBKOM in the chain.
	 */
	std::string strLagIdKomFirst;
	std::string strLagIdKomLast;

	iRv = getFirstAndLastLbkomOfVpl(pvTid, pkcFac, ptSoll->tVplk,
									strLagIdKomFirst, strLagIdKomLast);
	if (iRv < 0) {
		LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT,
				"getFirstAndLastLbkomOfVpl() failed %d", iRv);
		ArrDestroy(hArrNodesStart);
		ArrDestroy(hArrNodesEnd);
		return OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
								"getFirstAndLastLbkomOfVpl");
	}

	iRv = _WpgobaseSIf_GetAllNodesOfLbkom(pvTid, pkcFac, strLagIdKomFirst,
								hArrNodesStart, WpgobaseNnodeTypeStart);
	if (iRv < 0) {
		if (iRv == GeneralTerrNoDataFound) {
			// It's not absolutely necessary to have a start node
			LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_TRACE,
					"No start nodes found for LBKOM '%s'", strLagIdKomFirst.c_str());
		} else {
			LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT,
					"_WpgobaseSIf_GetAllNodesOfLbkom() failed %d", iRv);
			ArrDestroy(hArrNodesStart);
			ArrDestroy(hArrNodesEnd);
			return OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
									"_WpgobaseSIf_GetAllNodesOfLbkom");
		}
	}

	iRv = _WpgobaseSIf_GetAllNodesOfLbkom(pvTid, pkcFac, strLagIdKomLast,
									hArrNodesEnd, WpgobaseNnodeTypeEnd);
	if (iRv < 0) {
		LoggingSIf_LogPrintf(pkcFac, LOGGING_SIF_ALERT,
				"_WpgobaseSIf_GetAllNodesOfLbkom() failed %d", iRv);
		ArrDestroy(hArrNodesStart);
		ArrDestroy(hArrNodesEnd);
		return OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
								"_WpgobaseSIf_GetAllNodesOfLbkom");
	}

	char* pcStartNode = NULL;
	char* pcEndNode = NULL;

	iRv = _WpgobaseSIf_FindRoute(pvTid, pkcFac,
								 hArrNodesStart,
								 ptSoll->hFesp,
								 hArrNodesEnd, "VPL",
								 &pcStartNode, &pcEndNode, plLength);
	if (iRv < 0) {
		OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
						 "_WpgobaseSIf_FindRoute");
		ArrDestroy(hArrNodesStart);
		ArrDestroy(hArrNodesEnd);
		return iRv;
	}

	// Fill path lenght [m]
	ptSoll->lPathLenght = *plLength;

	// Fill StartNode and EndPos before we return
	strcpy(ptSoll->tVplk.vplkStart_NName, pcStartNode);

	char* apcNode[1];
	char* apcFeldBez[1];
	char acFeldBez[FELDBEZ_LEN+1];

	apcNode[0] = pcEndNode;
	apcFeldBez[0] = acFeldBez;

	iRv = RoutingSIf_Node2Fesp(apcNode, 1, "VPL", apcFeldBez);
	if (iRv < 0) {
		OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
						 "RoutingSIf_Node2Fesp");
		ArrDestroy(hArrNodesStart);
		ArrDestroy(hArrNodesEnd);
		return iRv;
	}

	iRv = SlobaseSIf_GetPos4FeldBez(pvTid, pkcFac, &ptSoll->tVplk.vplkEndPos, acFeldBez);
	if (iRv < 0) {
		OpmsgSIf_ErrPush(GeneralTerrSubFunctionFailed,
						 "SlobaseSIf_GetPos4FeldBez");
		ArrDestroy(hArrNodesStart);
		ArrDestroy(hArrNodesEnd);
		return iRv;
	}

	LoggingSIf_LogPrintf (pkcFac, LOGGING_SIF_NOTIFY, 
			"--- %s PathLen [%ld], EndPosFeldBez [%s]",
			FwrefnrSIf_GetRefNr(TN_VPLK, &ptSoll->tVplk),
			ptSoll->lPathLenght,
			acFeldBez);

	ArrDestroy(hArrNodesStart);
	ArrDestroy(hArrNodesEnd);

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          Calculates time required to travel the order
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int CalculatePathTimeVpl(void *pvTid, const char *pcFac,
									SollZeitStructKs *ptSoll,
									double *pdResult)
{
	/* Declaration */
	int			iRv			=	0;
	char		*pcTpmId	=	NULL;
	long		lLength		=	1;
	double		dSpeed		=	5;

	TMSK    tTmsk;
	TMSK    *ptTmsk			= NULL;

	LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
						 " ''''' CalculatePathTimeVpl: Try '''''");

	if (pcTpmId == NULL) {
		pcTpmId = ptSoll->tTmsk.tmskTpmId;
	}

	if (pcTpmId == NULL) {
	  pcTpmId = "Hand01";
	}

	/* Call routing and fill ptSoll->tVplk.vplkStart_NName and ptSoll->tVplk.vplkEndPos
	 * We also get the length of the routing graph written to the lLength variable
	 */
	iRv = _FindStartNodeEndPosAndLength(pvTid, pcFac, ptSoll, &lLength);
	if( iRv < 0) {
		LoggingSIf_LogPrintf ( pcFac, LOGGING_SIF_ALERT,
				"_FindStartNodeEndPosAndLength failed");
		return iRv;
	}

	/* Is a Tpm given? */
	if (hTmsK != NULL &&
		pcTpmId != NULL) {
		strcpy(tTmsk.tmskTpmId,	pcTpmId);
		ptTmsk = (TMSK*) ArrGetFindElem (hTmsK, &tTmsk);
	}

	/* If no TPM found, use Default TPM for LAZ. If no default
	 * found, use Faktor of 0
	 */
	if (ptTmsk == NULL) {
		if(ptSoll->bDefaultTmskFound) {
			dSpeed = ptSoll->tTmsk.tmskGeschw;
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
				"No specific TPM, using default of LAZ. KsNr=%ld",
				ptSoll->tVplk.vplkKsNr);
		} else {
			dSpeed = 1;
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
				"No default TPM for LAZ existing. KsNr=%ld ==> set Speed to 1",
				ptSoll->tVplk.vplkKsNr);
		}
	} else {
		dSpeed = ptTmsk->tmskGeschw;
	}

	if (dSpeed == 0) {
		*pdResult = 0;

		OpmsgSIf_WriteOpmsg (pvTid, pcFac, WpgobaseTerrNoSpeedForTPM,
			FwrefnrSIf_GetRefNr (TN_TMSK, &tTmsk), FrmwrkSIf_GetUser(),
			MLM("Dem Transportmittel %s is keine Geschwindigkeit zugeordnet.\n"
				"Um die Wegematrix f�r die Sollzeitberechnung ber�cksichtigen "
				"zu k�nnen ist die Geschwindigkeit des Transportmittels "
				"unbedingt erforderlich"), FwrefnrSIf_GetRefNr (TN_TMSK, &tTmsk));

		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				"Warning: Speed of %s is zero; Continueing anyway...",
				FwrefnrSIf_GetRefNr (TN_TMSK, &tTmsk));
	} else {
		*pdResult = (double)lLength / dSpeed;
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
			" ''''' CalculatePathTimeVpl: success. KsNr=%ld. Weight=%ld '''''",
			ptSoll->tVplk.vplkKsNr, lLength);
	}

	return 1;  
}  /* CalculatePathTimeVpl */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          Calculates time required to travel the order
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int CalculateNumberOfLoadActivities(
	const void				*pvTid,
	const char				*pcFac,
	const SollZeitStructKs	*ptSoll,
	long 					*plNumberOfLoadActivities)
{
	/* Declaration */
	const char	*pcFctName	= "CalculateNumberOfLoadActivities";
	int			iRv			=	0;
	VPLP		*pVplp		=	NULL;
	int			nI			=	0;
	char		*pcTpmId	=	NULL;
	long		lSizeOnStacker	=	0;

	LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
						 " ~~~~ %s: Try ~~~~", pcFctName);

	/* Initialisation */

	/* Algorithm */
	for (pVplp = (VPLP*)ArrWalkStart (ptSoll->hVplp), nI = 0;
		 pVplp != NULL;
		 pVplp = (VPLP*)ArrWalkNext (ptSoll->hVplp), nI++) {

		if (pcTpmId == NULL) {
			pcTpmId = pVplp->vplpTpmId;
		}
	}

	if (IsEmptyStrg(pcTpmId)) {
		if (IsEmptyStrg(ptSoll->tTmsk.tmskTpmId)) {
			pcTpmId = "Hand01";
		} else {
			pcTpmId = (char *)ptSoll->tTmsk.tmskTpmId;
		}
	}

	iRv = _WpgobaseSIf_GetStpGabelLaengeByTpmId(pvTid, pcFac, pcTpmId, &lSizeOnStacker);
   	if (iRv < 0) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
			"%s: failed to call '_WpgobaseSIf_GetStpGabelLaengeByTpmId'. TpmId[%s]",
			pcFctName, pcTpmId);
		return -1;
	}
	if (lSizeOnStacker <= 0) {
		*plNumberOfLoadActivities = 0;
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY,
				"%s: received StaplerLaenge is not allowed ( <= 0). "
				"TpmId[%s] Laenge[%ld]",
				pcFctName, pcTpmId, lSizeOnStacker);
	} else {
		*plNumberOfLoadActivities = (long)
						ceil(ptSoll->dSumThmLength / lSizeOnStacker) -1;
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE,
				  "%s: success. KsNr=%ld. Number Of Activities=%ld "
				  "(StaplerGabelLaenge[%ld] SumPallettenLaenge[%f])",
				  pcFctName, ptSoll->tVplk.vplkKsNr, *plNumberOfLoadActivities,
				  lSizeOnStacker, ptSoll->dSumThmLength);
	}

	LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_TRACE, " ~~~~ %s: finished ~~~~",
						 pcFctName);

	return 1;  
}  /* CalculateNumberOfLoadActivities */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of AID
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareAID (void *pv1, void *pv2, void *pvCb)
{
    AID 	*pt1    = (AID *)pv1;
    AID 	*pt2    = (AID *)pv2;
	int		iRv        = 0;

	if (pt1->AusPr > pt2->AusPr) {
		return (1);
    }
	if (pt1->AusPr < pt2->AusPr) {
		return (-1);
    }

	iRv = strcmp (pt1->ArtNr, pt2->ArtNr);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

	iRv = strcmp (pt1->Var, pt2->Var);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

	iRv = strcmp (pt1->Mand, pt2->Mand);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

    return 0;
} /* CompareAID */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of AID
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareARTEByFaktor (void *pv1, void *pv2, void *pvCb)
{
    ARTE 	*pt1    = (ARTE *) pv1;
    ARTE 	*pt2    = (ARTE *) pv2;

	if (pt1->arteNenner / pt1->arteZaehler >
		pt2->arteNenner / pt2->arteZaehler) {
		return 1;
    }
	if (pt1->arteNenner / pt1->arteZaehler <
		pt2->arteNenner / pt2->arteZaehler) {
		return -1;
    }

    return 0;
} /* CompareARTEByFaktor */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Search all ArtE matching to Atr. Stored them in phAllArtE.
-*		The pointer pptAnzeigeE, pptPaletteE are linked to the item
-*		in phAllArtE.
-*
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else.
-*--------------------------------------------------------------------------*/
static int SearchArte	(void *pvTid, 
						const char *pcFac,
						SollZeitStructKs *ptSoll, 
						ART 	* ptArt,
						ARTE	**pptAnzeigeE,
						ArrayPtr *phAllArtE)
{
	/* Declaration */
	ARTE	tCompareStruct;	
	ARTE	*ptArtE;	
	long	lIndex		=	0;
	long	lArteCount	=	0;
	int		iRv			=	0;

	/* Initialisation */
	memset (&tCompareStruct, 0, sizeof (tCompareStruct));
	strcpy  (tCompareStruct.arteEinheit, ptArt->artAnzeige_Einheit);
	tCompareStruct.arteAId = ptArt->artAId;

	/* Search AnzeigeEinheit */
	lIndex = ArrFindElem (ptSoll->hArtE, &tCompareStruct);
	if (lIndex < 0) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "SearchArte: ArrFindElem failed."
							 "SearchArte:  No Anzeigeeinheit found ");
		return -1;
	}
	*pptAnzeigeE = (ARTE*) ArrGetElem (ptSoll->hArtE, lIndex);

	/* Create result array */
	*phAllArtE = ArrCreate (sizeof (ARTE), 10, CompareARTEByFaktor, NULL);
	if (*phAllArtE == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
		"SearchArte: ArrCreate failed");
		return -1;
	}
	
	/* Walk backward while ART and ARTE matched */
	/*while (lIndex > 0 && 
			CompareAID (&ptArt->artAId, 
				&((ARTE*) ArrGetElem (ptSoll->hArtE, lIndex -1))->arteAId,
				NULL)) {
		lIndex--;
	}*/

	/* Walk forward while ART and ARTE matched 
		Copy Items to result Array. */
	lArteCount = (long) ArrGet (ptSoll->hArtE, ArrNnoEle);
	do {
		ptArtE = (ARTE*) ArrGetElem (ptSoll->hArtE, lIndex);
		ArrAddElem (*phAllArtE, ptArtE);
		lIndex++;
	} while (lIndex < lArteCount && 
			CompareAID (&ptArt->artAId, 
				&((ARTE*) ArrGetElem (ptSoll->hArtE, lIndex -1))->arteAId,
				NULL));

	/* Sort Array */
	iRv = ArrSort (*phAllArtE, CompareARTEByFaktor, NULL); 
	if (iRv < 0) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "SearchArte: ArrSort failed");
		return -1;  
	}
	return 1;
} /* SearchArte */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This function gets the head an all positions of "Verplanung"
-*			indicated by lKsnr.
-*          Then the time of this job will be returned.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		ptSoll.........The soll struct
-*		pdResult.......The result of calculation
-*				  
-* PRECONDITION failed if:
-*		lKsNr.................==...0				
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int CalculateEstimatedTimeVpl (void *pvTid, const char *pcFac,
									  SollZeitStructKs *ptSoll,
									  double *pdResult)
{
	/* Declaration */
	long	lCountAllArT	= 0;
	int		iRv				= 0;
	PPSPRM	*pPrm			= &ptSoll->tLbpKomPrm.lbpkomprmpps_PRM;
	VPLP	*ptVplp			= NULL;
	ART		*ptArt			= NULL;
	ARTE	*ptAnzeigeE		= NULL;
	ARTE	*ptPaletteE		= NULL;
	ARTE	*ptAktArtE		= NULL;
	ArrayPtr hAllArtE		= NULL;
	VPLP	 tPrevVplp;
	long    lAnzahl			= 0;
	long	lAnzahlEinheit	= 0;
	long	lGanzTeFaktor	= 0;
	long	lAnzeigeFaktor	= 0;
	long	lMenge			= 0;
	long	iI				= 0;
	long	lFaktor			= 0;

	HANDKS	*ptHandKs		= NULL;
	HANDKS	tHandKsCompare;

	GEWKS	*ptGewKs		= NULL;
	GEWKS	tGewKsCompare;
	long	lAnzVpl = 0, lLoadActivities = 0;
	double	dTimeVpl = 0, dTmpResult = 0;

	/* User information */
	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
						  " ++++ CalculateEstimatedTimeVpl: start ++++"); 

	/* Initialisation */
	memset (&tHandKsCompare, 0, sizeof (tHandKsCompare));
	memset (&tGewKsCompare, 0, sizeof (tGewKsCompare));

	/* Auftragsfaktor */
	if (pPrm->AuftrFaktKz == JANEIN_J) {
		*pdResult += pPrm->AuftrFakt;
	}

	/* Fahrzeit */
	if (pPrm->WgmKz == JANEIN_J) {
 		iRv = CalculatePathTimeVpl (pvTid, pcFac, ptSoll, &dTimeVpl);
		if (iRv != 1) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					   "<CalculateEstimatedTimeVpl> "
					   "CalculatePathTimeVpl failed"); 
			return iRv;
		}
		*pdResult += dTimeVpl;
	}
	
	if (pPrm->TeLadeFaktKz == JANEIN_J) {

		iRv = CalculateNumberOfLoadActivities(pvTid, pcFac, ptSoll, 
											  &lLoadActivities);
		if (iRv < 0) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					   "<CalculateEstimatedTimeVpl>."
					   "CalculatePathTimeVpl failed"); 
			return (-1);
		}
		*pdResult += pPrm->TeLadeFakt * lLoadActivities;
	}
	/* Ladezeit TE */
	if (pPrm->TeTypKz ==  JANEIN_J) {
		iRv = CalculateLadezeit (pvTid, pcFac, &dTmpResult, ptSoll);
		if (iRv != 1) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					   "<CalculateEstimatedTimeVpl>. "
					   "CalculateLadezeit failed"); 
			return -1;
		}
		*pdResult += dTmpResult;
	}

	memset(&tPrevVplp, 0, sizeof(tPrevVplp));

	/* Now we loop over the single VPLP */
	for (ptVplp = (VPLP*)ArrWalkStart (ptSoll->hVplp),
		 ptArt  = (ART*)ArrWalkStart (ptSoll->hArt);
		 ptVplp != NULL && ptArt != NULL;
		 ptVplp = (VPLP*)ArrWalkNext (ptSoll->hVplp),
		 ptArt  = (ART*)ArrWalkNext (ptSoll->hArt)) {

		/* Init for loop */
		ptAnzeigeE		= NULL;
		ptPaletteE		= NULL;
		hAllArtE		= NULL;
		lCountAllArT	= 0;
		lAnzahl			= 0; 
		lAnzahlEinheit	= 0;
		lGanzTeFaktor	= 0;
		lAnzeigeFaktor	= 0;
		
		if (ptVplp->vplpStatus == VPLPSTATUS_FERTIG ||
			ptVplp->vplpStatus == VPLPSTATUS_BUCHEN) {

			lMenge = (long)(ptVplp->vplpIstMngs.Mng);
		} else {
			lMenge = (long)(ptVplp->vplpSollMngs.Mng);
		}

		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			" -----------START------------------\n"
			"KsNr: %ld, VplNr: %ld, Status: %s,\n"
			"AID: %s/%s/%s/%ld \n"
			"SollMng: %.0lf, SollGew: %.3lf, IstMng: %.0lf, IstGew: %.0lf",
			ptVplp->vplpVPLK_KsNr,
			ptVplp->vplpVplNr,
			(char *)l2sGetNameByValue((LO2STR *)&l2s_VPLPSTATUS,
								ptVplp->vplpStatus),
			ptVplp->vplpMId.AId.Mand,
			ptVplp->vplpMId.AId.ArtNr,
			ptVplp->vplpMId.AId.Var,
			ptVplp->vplpMId.AId.AusPr,
			ptVplp->vplpSollMngs.Mng,
			ptVplp->vplpSollMngs.Gew,
			ptVplp->vplpIstMngs.Mng,
			ptVplp->vplpIstMngs.Gew);

		if (lMenge <= 0) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
			   	"Menge <= 0 --> continue.\n"
				"--------------END-----------------------");
			continue;
			/*break;*/
		}
	
		if (ptVplp->vplpFlag.MitnahmeKz == JANEIN_J) {
			if (pPrm->PosFaktKz == JANEIN_J) {
				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
					"MitnamheTE: %.3lf +=  PosFakt: %.3lf\n"
					"--------------END-----------------------",
					*pdResult, pPrm->PosFakt);
				*pdResult += pPrm->PosFakt;
				continue;
			} else {
				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
					"MitnahmeTE --> continue.\n"
					"--------------END-----------------------");
				continue;
			}
		}

		/* If AusId, AId and KomPos the same as before
		 * don't count this Vplp
		 */	
		if (ptSoll->tVplk.vplkKomArt == KOMART_ETIKETT) {
			if (!(strcmp(ptVplp->vplpAUSP_AusId.AusNr,
						tPrevVplp.vplpAUSP_AusId.AusNr) 		== 0 &&
				strcmp(ptVplp->vplpAUSP_AusId.Mand,
						tPrevVplp.vplpAUSP_AusId.Mand) 			== 0 &&
				strcmp(ptVplp->vplpAUSP_AusId.HostAusKz,
						tPrevVplp.vplpAUSP_AusId.HostAusKz)		== 0 &&
				ptVplp->vplpAUSP_PosNr == tPrevVplp.vplpAUSP_PosNr 	 &&
				ptVplp->vplpMId.AId.AusPr ==
					tPrevVplp.vplpMId.AId.AusPr &&
				strcmp(ptVplp->vplpMId.AId.Mand,
						tPrevVplp.vplpMId.AId.Mand) 			== 0 &&
				strcmp(ptVplp->vplpMId.AId.ArtNr,
						tPrevVplp.vplpMId.AId.ArtNr) 			== 0 &&
				strcmp(ptVplp->vplpMId.AId.Var,
						tPrevVplp.vplpMId.AId.Var) 				== 0 &&
				strcmp(ptVplp->vplpKomPos.FeldId,
						tPrevVplp.vplpKomPos.FeldId)			== 0)) {

				/* Auftragpositionsfaktor */
				if (ptArt->artArtTyp != ARTTYPE_THM) {
					if (pPrm->PosFaktKz == JANEIN_J) {
						LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
							"SumSollZeit: %.3lf +=  PosFakt: %.3lf)",
							*pdResult, pPrm->PosFakt);
						*pdResult += pPrm->PosFakt;
					}
				}
				/* Gewichtspositionskonstante */
				if (ptArt->artBestErfArtWa != BESTERFART_STK) {
					if (pPrm->GewPosFaktKz == JANEIN_J) {
						LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
							"SumSollZeit: %.3lf +=  GewPosFakt: %.3lf)",
							*pdResult, pPrm->GewPosFakt);
						*pdResult += pPrm->GewPosFakt;
					}
				}
				tPrevVplp = *ptVplp;
			}

		} else { /* ptVplk->vplkKomArt != KOMART_ETIKETT */
			/* Auftragpositionsfaktor */
			if (ptArt->artArtTyp != ARTTYPE_THM) {
				if (pPrm->PosFaktKz == JANEIN_J) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
						"SumSollZeit: %.3lf +=  PosFakt: %.3lf)",
						*pdResult, pPrm->PosFakt);
					*pdResult += pPrm->PosFakt;
				}
			}
			/* Gewichtspositionskonstante */
			if (ptArt->artBestErfArtWa != BESTERFART_STK) {
				if (pPrm->GewPosFaktKz == JANEIN_J) {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
						"SumSollZeit: %.3lf +=  GewPosFakt: %.3lf)",
						*pdResult, pPrm->GewPosFakt);
					*pdResult += pPrm->GewPosFakt;
				}
			}
			tPrevVplp = *ptVplp;
		}

		/* Search ARTE */
		 iRv = SearchArte (pvTid, pcFac, ptSoll, 
							ptArt, &ptAnzeigeE, &hAllArtE);
		if (iRv != 1) {
			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateEstimatedTimeVpl: SearchArte failed !!!!"); 
			return (-1);
		}

		lCountAllArT = (long) ArrGet (hAllArtE, ArrNnoEle);
		lAnzeigeFaktor = (long)(ptAnzeigeE->arteZaehler / ptAnzeigeE->arteNenner);

		if (lCountAllArT > 0 && ptArt->artGanzTeMng > 0) {

			for (ptAktArtE = (ARTE*)ArrWalkStart (hAllArtE);
				ptAktArtE != NULL;
				ptAktArtE = (ARTE*)ArrWalkNext (hAllArtE)) {

				if (((double)(ptAktArtE->arteZaehler / ptAktArtE->arteNenner))
					 >=	ptArt->artGanzTeMng) {

					ptPaletteE = ptAktArtE;
					break;
				}
			}
			ptAktArtE = NULL;
		}

		/* Ganzpalettenfaktor */
		if (ptPaletteE != NULL && pPrm->GanzTeFaktKz == JANEIN_J) {

			lGanzTeFaktor = (long)(ptPaletteE->arteZaehler / ptPaletteE->arteNenner);
			lAnzahl = lMenge / lGanzTeFaktor;

			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			   "SummSollZeit: %.3lf += (lAnzahl: %ld * GanzTeFakt: %.3lf)",
				*pdResult, lAnzahl, pPrm->GanzTeFakt);

			*pdResult += lAnzahl * pPrm->GanzTeFakt;
		}

		/* Pickfaktor */
		if (pPrm->PickFaktKz == JANEIN_J) {
			lAnzahl = (lGanzTeFaktor == 0)
						?lMenge
						:lMenge % lGanzTeFaktor; 
			lAnzahl = (lAnzahl / lAnzeigeFaktor) + 
					  (lAnzahl % lAnzeigeFaktor);

			LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			   "SummSollZeit:%.3lf += (lAnzahl: %ld * PickFakt: %.3lf)",
				*pdResult, lAnzahl, pPrm->PickFakt);

			*pdResult += lAnzahl * pPrm->PickFakt;
		}

		/* Handlingsfaktor */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			   "HandKlasseKz = %ld ", pPrm->HandKlasseKz);

		if (pPrm->HandKlasseKz == JANEIN_J) {

			lAnzahlEinheit = 0;
			lAnzahl = lMenge; 				

			for (iI = lCountAllArT - 1; iI >= 0; iI--) {

				ptAktArtE = (ARTE*)ArrGetElem (hAllArtE, iI);
				/* Search HandKs */
				strcpy (tHandKsCompare.handksHandKlasse,
						ptAktArtE->arteHANDKS_HandKlasse);

				ptHandKs = (HANDKS*) ArrGetFindElem (hHandKs, &tHandKsCompare);

				if (ptHandKs == NULL) {

					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT, 
						"<CalculateEstimatedTimeVpl>"
						" ArrFindElem failed. HandKs [%s] not found." 
						" AId [%s][%s][%s][%ld]", 
						ptAktArtE->arteHANDKS_HandKlasse,
						ptAktArtE->arteAId.Mand,
						ptAktArtE->arteAId.ArtNr,
						ptAktArtE->arteAId.Var,
						ptAktArtE->arteAId.AusPr);
					continue;
				} else {
					LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE, 
						"<CalculateEstimatedTimeVpl>"
						" AId [%s][%s][%s][%ld] -> HandKs [%s]", 
						ptAktArtE->arteAId.Mand,
						ptAktArtE->arteAId.ArtNr,
						ptAktArtE->arteAId.Var,
						ptAktArtE->arteAId.AusPr,
						ptAktArtE->arteHANDKS_HandKlasse);
				}	   

				lFaktor = (long)(ptAktArtE->arteZaehler / ptAktArtE->arteNenner);
				lAnzahlEinheit = lAnzahl / lFaktor;
				lAnzahl = lAnzahl % lFaktor;

				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
					"SumSollZeit: %.3lf += (lAnzahlEinheit: %ld * "
					"HandKsZeit: %ld)",
					*pdResult, lAnzahlEinheit, ptHandKs->handksZeit);

				*pdResult += lAnzahlEinheit * ptHandKs->handksZeit;
			}
		} 
		/* Gewichtsfaktor */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			   "GewKlasseKz = %ld", pPrm->GewKlasseKz);
		if (pPrm->GewKlasseKz == JANEIN_J) {

			lAnzahlEinheit = 0;
			lAnzahl = lMenge; 

			for (iI = lCountAllArT - 1; iI >= 0; iI--) {

				ptAktArtE = (ARTE*)ArrGetElem (hAllArtE, iI);
				/* Search GewKs */

				if (IsEmptyString (ptAktArtE->arteGEWKS_GewKlasse)
					== JANEIN_J) {

					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_NOTIFY, 
						"<CalculateEstimatedTimeVpl> "
						"ptGewKs is Empty. AId [%s][%s][%s][%s][%ld]", 
						tGewKsCompare.gewksGewKlasse,
						ptAktArtE->arteAId.Mand,
						ptAktArtE->arteAId.ArtNr,
						ptAktArtE->arteAId.Var,
						ptAktArtE->arteAId.AusPr);

					continue;
				} 
				strcpy (tGewKsCompare.gewksGewKlasse,
						ptAktArtE->arteGEWKS_GewKlasse);

				ptGewKs = (GEWKS*) ArrGetFindElem (hGewKs, &tGewKsCompare);

				if (ptGewKs == NULL) {

					LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
						"<CalculateEstimatedTimeVpl>. ArrFindElem failed."
						"ptGewKs [%s] not found. AId [%s][%s][%s][%ld]", 
						tGewKsCompare.gewksGewKlasse,
						ptAktArtE->arteAId.Mand,
						ptAktArtE->arteAId.ArtNr,
						ptAktArtE->arteAId.Var,
						ptAktArtE->arteAId.AusPr);

					return -1;
				}

				lFaktor = (long)(ptAktArtE->arteZaehler / ptAktArtE->arteNenner);
				lAnzahlEinheit = lAnzahl / lFaktor;
				lAnzahl = lAnzahl % lFaktor;

				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
					"SumSollZeit: %.3lf += (lAnzahlEinheit: %ld * "
					"GewKsZeit: %ld)",
					*pdResult, lAnzahlEinheit, ptGewKs->gewksZeit);

				*pdResult += lAnzahlEinheit * ptGewKs->gewksZeit;
			}
		}
		/* Gewichtsartikelfaktoren */
		if (ptArt->artBestErfArtWa != BESTERFART_STK) {
			/* Gewichtskonstante */
			if (pPrm->GewFaktKz == JANEIN_J) {
				lAnzahl = (lGanzTeFaktor == 0)
							?lMenge
							:lMenge % lGanzTeFaktor; 
				lAnzahl = (lAnzahl / lAnzeigeFaktor) + 
						  (lAnzahl % lAnzeigeFaktor);

				LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
					"SumSollZeit: %.3lf += (lAnzahl: %ld * GewFakt: %.3lf)",
					*pdResult, lAnzahl, pPrm->GewFakt);

				*pdResult += lAnzahl * pPrm->GewFakt;
			}
		}

		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE, 
				"-----------------END----------------------------------"); 

	}
	/* Pers�nliche Verteilzeit */
	if (pPrm->PersFaktKz == JANEIN_J && pPrm->PersFakt > 0) {
		*pdResult = *pdResult * (double ) pPrm->PersFakt / 100;
	}
	/* Sachliche Verteilzeit */
	if (pPrm->SachFaktKz == JANEIN_J && pPrm->SachFakt > 0) {
		*pdResult = *pdResult * (double) (pPrm->SachFakt / 100);
	}
	/* Leistungsgrad */
	if (pPrm->LeistGradKz == JANEIN_J && pPrm->LeistGrad > 0) {
		*pdResult = *pdResult * (double) (pPrm->LeistGrad / 100);
	}

	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE,
			"<CalculateEstimatedTimeVpl>.\n"
			"SummSollZeit:    %.3lf\n"
			"AuftrFaktKz:     %ld (AuftrFakt: 	%.3lf)\n"
			"WgmKz:           %ld (dTimeVpl: 	%.3lf)\n"
			"PosFaktKz:       %ld (AnzVpl: 		%ld * PosFakt: %.3lf)\n"
			"TeLadeFaktKz:    %ld (LoadAktivities: %ld * TeLadeFakt: %.3lf)\n"
			"TeTypKz:         %ld (dTmpResult: 	%.3lf)\n"
			"PersFaktKz:      %ld (PersFakt: 	%.3lf)\n"
			"SachFaktKz:      %ld (SachFakt: 	%.3lf)\n"
			"LeistGrad:       %ld (LeistGrad: 	%.3lf)\n",
			*pdResult,
			pPrm->AuftrFaktKz, pPrm->AuftrFakt,
			pPrm->WgmKz, dTimeVpl,
			pPrm->PosFaktKz, lAnzVpl, pPrm->PosFakt,
			pPrm->TeLadeFaktKz, lLoadActivities, pPrm->TeLadeFakt,
			pPrm->TeTypKz, dTmpResult,
			pPrm->PersFaktKz, pPrm->PersFakt ,
			pPrm->SachFaktKz, pPrm->SachFakt,
			pPrm->LeistGradKz, pPrm->LeistGrad);

	RNDDOUBLE(pdResult);

	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_TRACE, 
			" ++++ CalculateEstimatedTimeVpl: end (Result = %.3lf) ++++",
			*pdResult); 

	return (1);

}  /* CalculateEstimatedTimeVpl */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of TMSK
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareTMSK (void *pv1, void *pv2, void *pvCb)
{
    TMSK 	*pt1    = (TMSK *)pv1;
    TMSK 	*pt2    = (TMSK *)pv2;
	int		iRv        = 0;

	iRv = strcmp (pt1->tmskTpmId, pt2->tmskTpmId);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

    return 0;
} /* CompareTMSK */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Load the static array hTmsK. 
-*		The array will die with the task.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int InitStaticArrayTmsK	(void *pvTid, const char *pcFac)
{
	int			iRv			=	0;
	int			iRvFunc		=	0;
	int			iDbRvX		=	0;
	int			nI			=	0;
	TMSK		aTmsK [BLOCKSIZE];

	if (hTmsK	!= NULL) {
		return 1;
	}

	/* Userinformation */
	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
						  "InitStaticArrayTmsK: try");

	/* Initialisation */
	hTmsK = ArrCreate (sizeof (TMSK), BLOCKSIZE, CompareTMSK, NULL);
	if (hTmsK == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "InitStaticArrayTmsK: ArrCreate failed");
		return -1;
	}

	/* Algorithm */
	/* Load TMSK */
	do {
		memset (aTmsK, 0, sizeof (aTmsK)); 
		iDbRvX = (nI == 0)
			? TExecSqlX (pvTid, NULL,
					 " select %" TN_TMSK 
					 " from   " TN_TMSK,
					 BLOCKSIZE,0,
					 SELSTRUCT (TN_TMSK, aTmsK[0]),
					 NULL)
			:  TExecSqlV(pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError(pvTid) != SqlNotFound) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				 "InitStaticArrayTmsK: TSqlErrTxt:%s", 
				 TSqlErrTxt (pvTid));
			iRv = -1; 
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			iRvFunc = ArrAddElem(hTmsK, &aTmsK [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"InitStaticArrayTmsK: ArrAddElem failed");
				iRv = -1; 
				break;
			}
		}
	} while (iDbRvX == BLOCKSIZE && iRv != -1);

	/* Sort Array */
	if (iRv >= 0) {
		iRvFunc = ArrSort (hTmsK, CompareTMSK, NULL); 
		if (iRvFunc < 0) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
				"InitStaticArrayTmsK: ArrSort failed");
			iRv = -1;  
		}
	}

	if (iRv < 0) {
		/* Shit happend */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							  "InitStaticArrayTmsK: failed");
		iRv = -1;
	} else {
		/* Userinformation */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
							  "InitStaticArrayTmsK: success");
		iRv = 1;
	}

	return iRv;
} /* InitStaticArrayTmsK */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of GEWKS
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareGEWKS (void *pv1, void *pv2, void *pvCb)
{
    GEWKS 	*pt1    = (GEWKS *)pv1;
    GEWKS 	*pt2    = (GEWKS *)pv2;
	int		iRv        = 0;

	iRv = strcmp (pt1->gewksGewKlasse, pt2->gewksGewKlasse);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

    return 0;
} /* CompareGEWKS */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Load the static array hGewKs. 
-*		The array will die with the task.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int InitStaticArrayGewKs	(void *pvTid, const char *pcFac)
{
	int			iRv			=	0;
	int			iRvFunc		=	0;
	int			iDbRvX		=	0;
	int			nI			=	0;
	GEWKS		aGewKs [BLOCKSIZE];

	if (hGewKs	!= NULL) {
		return 1;
	}

	/* Userinformation */
	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
						  "InitStaticArrayGewKs: try");

	/* Initialisation */
	hGewKs = ArrCreate (sizeof (GEWKS), BLOCKSIZE, CompareGEWKS, NULL);
	if (hGewKs == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "InitStaticArrayGewKs: ArrCreate failed");
		return -1;
	}

	/* Algorithm */
	/* Load GEWKS */
	do {
		memset (aGewKs, 0, sizeof (aGewKs)); 
		iDbRvX = (nI == 0)
			? TExecSqlX (pvTid, NULL,
					 " select %" TN_GEWKS 
					 " from   " TN_GEWKS,
					 BLOCKSIZE,0,
					 SELSTRUCT (TN_GEWKS, aGewKs[0]),
					 NULL)
			:  TExecSqlV(pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError(pvTid) != SqlNotFound) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				 "InitStaticArrayGewKs: TSqlErrTxt:%s", 
				 TSqlErrTxt (pvTid));
			iRv = -1; 
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			iRvFunc = ArrAddElem(hGewKs, &aGewKs [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"InitStaticArrayGewKs: ArrAddElem failed");
				iRv = -1; 
				break;
			}
		}
	} while (iDbRvX == BLOCKSIZE && iRv != -1);

	/* Sort Array */
	if (iRv >= 0) {
		iRvFunc = ArrSort (hGewKs, CompareGEWKS, NULL); 
		if (iRvFunc < 0) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
				"InitStaticArrayGewKs: ArrSort failed");
			iRv = -1;  
		}
	}

	if (iRv < 0) {
		/* Shit happend */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							  "InitStaticArrayGewKs: failed");
		iRv = -1;
	} else {
		/* Userinformation */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
							  "InitStaticArrayGewKs: success");
		iRv = 1;
	}

	return iRv;
} /* InitStaticArrayGewKs */ 

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This functions compares two items of HANDKS
-* RETURNS   0 .. equal
-*           1 .. pv1 is higher
-*          -1 .. pv2 is higher
-*--------------------------------------------------------------------------*/
static int CompareHANDKS (void *pv1, void *pv2, void *pvCb)
{
    HANDKS 	*pt1    = (HANDKS *)pv1;
    HANDKS 	*pt2    = (HANDKS *)pv2;
	int		iRv        = 0;

	iRv = strcmp (pt1->handksHandKlasse, pt2->handksHandKlasse);
	if (iRv > 0) {
		return 1;
    }
	if (iRv < 0) {
		return -1;
    }

    return 0;
} /* CompareHANDKS */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Load the static array hHandKs. 
-*		The array will die with the task.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int InitStaticArrayHandKs	(void *pvTid, const char *pcFac)
{
	int			iRv			=	0;
	int			iRvFunc		=	0;
	int			iDbRvX		=	0;
	int			nI			=	0;
	HANDKS		aHandKs [BLOCKSIZE];

	if (hHandKs	!= NULL) {
		return 1;
	}

	/* Userinformation */
	LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
						  "InitStaticArrayHandKs: try");

	/* Initialisation */
	hHandKs = ArrCreate (sizeof (HANDKS), BLOCKSIZE, CompareHANDKS, NULL);
	if (hHandKs == NULL) {
		LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
							 "InitStaticArrayHandKs: ArrCreate failed");
		return -1;
	}

	/* Algorithm */
	/* Load HANDKS */
	do {
		memset (aHandKs, 0, sizeof (aHandKs)); 
		iDbRvX = (nI == 0)
			? TExecSqlX (pvTid, NULL,
					 " select %" TN_HANDKS 
					 " from   " TN_HANDKS,
					 BLOCKSIZE,0,
					 SELSTRUCT (TN_HANDKS, aHandKs[0]),
					 NULL)
			:  TExecSqlV(pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError(pvTid) != SqlNotFound) {
            LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT,
				 "InitStaticArrayHandKs: TSqlErrTxt:%s", 
				 TSqlErrTxt (pvTid));
			iRv = -1; 
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			iRvFunc = ArrAddElem(hHandKs, &aHandKs [nI]);
			if (iRvFunc < 0) {
				LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
					"InitStaticArrayHandKs: ArrAddElem failed");
				iRv = -1; 
				break;
			}
		}
	} while (iDbRvX == BLOCKSIZE && iRv != -1);

	/* Sort Array */
	if (iRv >= 0) {
		iRvFunc = ArrSort (hHandKs, CompareHANDKS, NULL); 
		if (iRvFunc < 0) {
			LoggingSIf_LogPrintf(pcFac, LOGGING_SIF_ALERT, 
				"InitStaticArrayHandKs: ArrSort failed");
			iRv = -1;  
		}
	}

	if (iRv < 0) {
		/* Shit happend */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
							  "InitStaticArrayHandKs: failed");
		iRv = -1;
	} else {
		/* Userinformation */
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_NOTIFY,
							  "InitStaticArrayHandKs: success");
		iRv = 1;
	}

	return iRv;
} /* InitStaticArrayHandKs */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Load the static array hHandKs. 
-*		The array will die with the task.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. 
-*--------------------------------------------------------------------------*/
static int InitStaticArrays	(void *pvTid, const char *pcFac)
{
	int	iRv	= 0;

	iRv = InitStaticArrayTmsK (pvTid, pcFac);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
			"InitStaticArrays: InitStaticArrayTmsK: failed");
		return -1;
	}

	iRv = InitStaticArrayGewKs (pvTid, pcFac);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
			"InitStaticArrays: InitStaticArrayGewKs: failed");
		return -1;
	}

	iRv = InitStaticArrayHandKs (pvTid, pcFac);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
			"InitStaticArrays:InitStaticArrayHandKs: failed");
		return -1;
	}

	return 1;
} /* InitStaticArrays */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*          This function gets the head an all positions of "Verplanung"
-*			indicated by lKsnr.
-*          Then the time of this job will be returned.
-*
-*		pvTid..........SQL's transaction ID (or NULL)
-*		pcFac..........facility name
-*		lKsNr..........number of Kommissionierung
-*		pdResult.......The result of calculation
-*				  
-* PRECONDITION failed if:
-*		lKsNr.................==...0				
-*		
-* RETURNS
-*		-1      :   any error. See Log-file for more information
-*		 1		:	else. Need commit.
-*--------------------------------------------------------------------------*/
static int CalculateSollZeit(void *pvTid, const char *pcFac, 
								long lKsNrOrProtNr, char *pcUser,
								double *pdResult,
								long *plPathLenght,
								int	iDataSource)
{	 
	/* Declaration */
	SollZeitStructKs	tSollStruct;
	int		iRv	=	0;

	/* Precondition */
	if (lKsNrOrProtNr == 0 ||
		pcUser == NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeit: "
				   " Precondition failed. lKsNrOrProtNr: %ld, User '%s'", 
				   lKsNrOrProtNr, (pcUser != NULL) ? pcUser : "NULL");
		return -1;
	}	

	/* Initialisation */
	memset (&tSollStruct, 0, sizeof(tSollStruct));
	tSollStruct.iDataSource		= iDataSource;
	tSollStruct.lKsNrOrProtNr	= lKsNrOrProtNr;
	tSollStruct.pcUser			= pcUser;

	/* Init the static arrays. Happens only on time in each task*/
	iRv = InitStaticArrays (pvTid, pcFac); 
 	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeit: InitStaticArrays failed.");
		FreeSollStruct(&tSollStruct);
		return -1;
	}	

	/* Init the dynamic struct. Happens in each call */
	iRv = FillSollStructKs (pvTid, pcFac, lKsNrOrProtNr, pcUser, &tSollStruct);
	if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeit: FillSollStructKs failed."
				   "lKsNrOrProtNr: %ld, User '%s'", 
				   lKsNrOrProtNr, (pcUser != NULL) ? pcUser : "NULL");
		FreeSollStruct(&tSollStruct);
		return -1;
	}	

	/* Algorithm */
	iRv = CalculateEstimatedTimeVpl	(pvTid, pcFac, &tSollStruct, pdResult);
	if (iRv < 0) 
	{
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeit: CalculateEstimatedTimeVpl failed."
				   "lKsNrOrProtNr: %ld, User '%s'", 
				   lKsNrOrProtNr, (pcUser != NULL) ? pcUser : "NULL");
	}	
	else
	{
		// set path lenght function parameter
		*plPathLenght = tSollStruct.lPathLenght;
	}

	/* Clean up */
	FreeSollStruct(&tSollStruct);

	return iRv;  
} /* CalculateSollZeit */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int SetSollZeit	(void *pvTid, const char *pcFac, long lKsNr, char *pcHistUser)
{
	/* Declaration */
	VPLK	tVplk;
	int		iDbRv		=	0;
	int		iRv			=	0;
	double	dSollZeit 	= 	0;
	long	lPathLen	= 	0;
			    
	/* Precondition */
	if (lKsNr				<=	0		||		
		pcHistUser			==	NULL) {

		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "SetSollZeit: Precondition failed. KsNr: %ld, User '%s'", 
				   lKsNr, (pcHistUser != NULL) ? pcHistUser : "NULL");
		return -1;
	}	

	/* Algorithm */

	/* Calculate time */
    iRv = CalculateSollZeitByKsNr (pvTid, pcFac, lKsNr, &dSollZeit, &lPathLen); 
    if (iRv != 1) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "SetSollZeit for KsNr failed: %ld",	lKsNr);

        return -1;
    }

	memset (&tVplk, 0, sizeof (tVplk));
    tVplk.vplkKsNr = lKsNr;

	if ((iDbRv = TExecStdSql (pvTid, 
							  StdNselectUpdNo, 
							  TN_VPLK, 
							  &tVplk)) <= 0) {

        LoggingSIf_LogPrintf ( pcFac, LOGGING_SIF_ALERT, 
                    "SetSollZeitByKsNr for KsNr: %ld failed. Error %s",
                    lKsNr, TSqlErrTxt(pvTid));

        return -1;
	}

	tVplk.vplkSollZeit = (long)dSollZeit;
	tVplk.vplkFlag.SzCalc  = SZCALC_BERECHNET;

	if (_WpgobaseSIf_UpdateVplk (pvTid, pcFac, &tVplk) < 0) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "SetSollZeitByKsNr for KsNr: %ld failed.  Error %s",
				   lKsNr, TSqlErrTxt(pvTid));
        return -1;
    }

    return 1;
} /* SetSollZeit */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int CalculateSollZeitByKsNr	(void *pvTid, const char *pcFac, 
							long lKsNr, double *pdResult, long *plPathLenght)
{
	char		acUser[USR_LEN+1];
	/* Precondition */

	memset (acUser, 0, sizeof (acUser));
	strncpy (acUser , "UNDEF", USR_LEN);
	acUser[USR_LEN] = '\0';

	if (lKsNr <= 0) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeitByKsNr: "
				   "Precondition failed. KsNr: %ld", lKsNr);
		return -1;
	}	

	return CalculateSollZeit	(pvTid, pcFac, 
								lKsNr, acUser, pdResult, plPathLenght,
								DATASOURCE_NORMAL);
} /* CalculateSollZeitByKsNr */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int CalculateSollZeitByKsNrAndUserReady	(void *pvTid, const char *pcFac, 
									long lKsNr, char *pcUser,
									double *pdResult, long *plPathLenght)
{
	/* Precondition */
	if (lKsNr			<=	0		||		
		pcUser			==	NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeitByKsNrAndUser: "
				   "Precondition failed. KsNr: %ld, User '%s'", 
				   lKsNr, (pcUser != NULL) ? pcUser : "NULL");
		return -1;
	}	

	return CalculateSollZeit	(pvTid, pcFac, 
								lKsNr, pcUser, pdResult, plPathLenght,
								DATASOURCE_NORMAL);
} /* CalculateSollZeitByKsNrAndUserReady */
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int CalculateSollZeitByKsNrAndUser	(void *pvTid, const char *pcFac, 
									long lKsNr, char *pcUser,
									double *pdResult, long *plPathLenght)
{
	/* Precondition */
	if (lKsNr			<=	0		||		
		pcUser			==	NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeitByKsNrAndUser: "
				   "Precondition failed. KsNr: %ld, User '%s'", 
				   lKsNr, (pcUser != NULL) ? pcUser : "NULL");
		return -1;
	}	

	return CalculateSollZeit	(pvTid, pcFac, 
								lKsNr, pcUser, pdResult, plPathLenght,
								DATASOURCE_NORMAL);
} /* CalculateSollZeitByKsNrAndUser */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int CalculateSollZeitByProtNr	(void *pvTid, const char *pcFac, 
								long lProtNr, double *pdResult, 
								long *plPathLenght)
{
	/* Precondition */
	if (lProtNr <= 0) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeitByProtNr: "
				   "Precondition failed. lProtNr: %ld", lProtNr);
		return -1;
	}	

	return CalculateSollZeit	(pvTid, pcFac, 
								lProtNr, NULL, pdResult, plPathLenght,
								DATASOURCE_PROTOCOL);
} /* CalculateSollZeitByProtNr */

 
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		See h-file
-*--------------------------------------------------------------------------*/
int CalculateSollZeitByProtNrAndUser	(void *pvTid, const char *pcFac, 
										long lProtNr, char *pcUser,
										double *pdResult, long *plPathLenght)
{
	/* Precondition */
	if (lProtNr			<=	0		||		
		pcUser			==	NULL) {
		LoggingSIf_LogPrintf (pcFac, LOGGING_SIF_ALERT,
				   "CalculateSollZeitByProtNrAndUser: "
				   "Precondition failed. lProtNr: %ld, User '%s'", 
				   lProtNr, (pcUser != NULL) ? pcUser : "NULL");
		return -1;
	}	

	return CalculateSollZeit	(pvTid, pcFac, 
								lProtNr, pcUser, pdResult, plPathLenght,
								DATASOURCE_PROTOCOL);
} /* CalculateSollZeitByProtNrAndUser */

