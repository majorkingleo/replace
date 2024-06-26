/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dbsqlstd.h>
#include <time.h>
#include <wamasbox.h>
#include <unistd.h>

/* ------- Owil-Headers --------------------------------------------------- */
#include <owil.h>

/* ------- Tools-Headers -------------------------------------------------- */
#include <dbsqlstd.h>
#include <ml.h>
#include <logtool.h>
#include <owil.h>
#include <errmsg.h>
#include <hist_util.h>
#include <stime.h>

#include <mumalloc.h>
#include <sql_util.h>
#include <disperr.h>
#include <errmsg.h>
#include <reallocx.h>
#include <parameter.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "facility.h"
#include "init_table.h"
#include "hist_util.h"
#include "aus.h"
#include "tep.h"
#include "te_aus.h"
#include "tpa.h"
#include "te_tpa.h"
#include "te_util.h"
#include "vpl_util.h"
#include "aus_util.h"
#include "wamas.h"
#include "touren_util.h"
#include "stock_util.h"
#include "facility.h"

#define _VOLLSTAENDIG_UTIL_C
#include "vollstaendig_util.h"
#include "ausueb_util.h"
#include "me_tourueb.h"
#undef _VOLLSTAENDIG_UTIL_C


/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/
#define BLOCKSIZE 				(100)
/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

/* ===========================================================================
 * LOCAL (STATIC) Variables and Function-Prototypes
 * =========================================================================*/

 /* ------- Variables ----------------------------------------------------- */

 /* ------- Function-Prototypes ------------------------------------------- */

/* ===========================================================================
 * GLOBAL VARIABLES
 * =========================================================================*/

/* ===========================================================================
 * LOCAL (STATIC) Functions
 * =========================================================================*/
int CbSortVollstaendigkeitAusp (void *ptB1, void *ptB2, void *pvCd)
{
	VOLLAUSPDATA 	*ptPtr1 = (VOLLAUSPDATA *)ptB1;
	VOLLAUSPDATA 	*ptPtr2 = (VOLLAUSPDATA *)ptB2;
    int             iCmp=0;

	iCmp = ptPtr1->zBestellZeit - ptPtr2->zBestellZeit;
	if (iCmp == 0 ) {
		iCmp = strncmp (ptPtr1->tAusId.Mand,
						ptPtr2->tAusId.Mand, MAND_LEN);
		if (iCmp == 0 ) {
			iCmp = strncmp (ptPtr1->tAusId.AusNr,
							ptPtr2->tAusId.AusNr, AUSNR_LEN);
			if (iCmp == 0 ) {
				iCmp = strncmp (ptPtr1->tAusId.AusKz,
								ptPtr2->tAusId.AusKz, EINAUSKZ_LEN);
				if (iCmp == 0 ) {
					iCmp = ptPtr1->tAusId.TlNr - ptPtr2->tAusId.TlNr;
					if (iCmp == 0 ) {
						iCmp = ptPtr1->lPosNr - ptPtr2->lPosNr;
						if (iCmp == 0 ) {
							return 0;
						}
					}
				}
			}
		}
	}

	return iCmp > 0 ? (1) : (-1);
}

int CbSortVollstaendigkeitArt (void *ptB1, void *ptB2, void *pvCd)
{
	VOLLAUSPDATA 	*ptPtr1 = (VOLLAUSPDATA *)ptB1;
	VOLLAUSPDATA 	*ptPtr2 = (VOLLAUSPDATA *)ptB2;
    int             iCmp=0;

	iCmp = strncmp (ptPtr1->tMatId.AId.Mand,
					ptPtr2->tMatId.AId.Mand, MAND_LEN);
	if (iCmp == 0 ) {
		iCmp = strncmp (ptPtr1->tMatId.AId.ArtNr,
						ptPtr2->tMatId.AId.ArtNr, ARTNR_LEN);
		if (iCmp == 0 ) {
			iCmp = strncmp (ptPtr1->tMatId.AId.Var,
							ptPtr2->tMatId.AId.Var, VAR_LEN);
			if (iCmp == 0 ) {
				iCmp = strncmp (ptPtr1->tMatId.MatKz,
								ptPtr2->tMatId.MatKz, MATKZ_LEN);
				if (iCmp == 0 ) {
					return 0;
				}
			}
		}
	}

	return iCmp > 0 ? (1) : (-1);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      int CbSortMindAusp (void *ptB1, void *ptB2, void *pvCd)
-* DESCRIPTION
-*      Sort entries by AusID and PosNr
-* RETURNS
-*      value of iCmp
-*--------------------------------------------------------------------------*/
static int FillAuspDataVoll (void 			*pvTid,
							 char 			*pcFac,
							 AUSK			*ptAusk,
							 VOLLAUSPDATA 	*ptAuspData,
							 int			iAnzAuspData,
							 ArrayPtr 		ptArrPtr)
{
	int		iAnz = 0, nCnt = 0;

	for (nCnt = 0; nCnt < iAnzAuspData; nCnt ++) {

		ptAuspData[nCnt].dOffenMng = ptAuspData[nCnt].tBestMngs.Mng - 
										ptAuspData[nCnt].dAuspVollMng;

		ptAuspData[nCnt].zBestellZeit = ptAusk->auskBestZeit;
		ptAuspData[nCnt].tAusId = ptAusk->auskAusId;
		ptAuspData[nCnt].tTourId = ptAusk->auskTourId;

		iAnz = ArrAddElem (ptArrPtr, &ptAuspData[nCnt]);
		if(iAnz < 0) {
			LogPrintf (pcFac, LT_ALERT,
					   "<FillAuspData>: allocate VOLLAUSPDATA memory failed");
			return (-1);
		}
	}

	return (0);
}


static int FromAusp2ArtVoll (void 			*pvTid,
							 char 			*pcFac,
							 AUSK			*ptAusk,
							 VOLLAUSPDATA 	*ptAuspData,
							 int			iAnzAuspData,
							 ArrayPtr 		ptArrPtr)
{
	VOLLAUSPDATA		*ptEqData = NULL;
	int					iAnz = 0, nCnt = 0;

	/* --- handle each AUSP entry --- */
	for (nCnt = 0; nCnt < iAnzAuspData; nCnt ++) {

		/* is there an equal record in the array ? */

		ptEqData = (VOLLAUSPDATA *)ArrGetFindElem(ptArrPtr, &ptAuspData[nCnt]);

		if (ptEqData != NULL) {

			/* Modifying an existant record */

			ptEqData->lAnzPos ++;
			ptEqData->dSollMngGe += ptAuspData[nCnt].tBestMngs.Mng;
			ptEqData->dVollMngGe += ptAuspData[nCnt].dAuspVollMng;

		} else {

			/* Adding a new record */

			ptAuspData[nCnt].lAnzPos = 1;
			ptAuspData[nCnt].dSollMngGe = ptAuspData[nCnt].tBestMngs.Mng;
			ptAuspData[nCnt].dVollMngGe = ptAuspData[nCnt].dAuspVollMng;

			iAnz = ArrAddElem(ptArrPtr, &ptAuspData[nCnt]);
			if (iAnz < 0) {
				LogPrintf (pcFac, LT_ALERT,
						   "<FromAusp2Art>: allocate VOLLAUSPDATA memory "
						   "failed");
				return (-1);
			}
		}
	}

	return (0);
}

/*----------------------------------------------------------------------------
-*  RETURN
-*  -1...Fehler 
-*   0...keine Daten gefunden 
-*   1...Daten gefunden 
-*--------------------------------------------------------------------------*/
int _CheckVollstaendigkeit(	void 		*pvTid,
							char 		*pcFac,
							AUSK 		*ptAusk,
							int 		iAnzAusk,
							int			*piAnzAuskUpd,
							ArrayPtr 	ptArrPtr,
							ArrayPtr	ptAuspPtr)
{
	VOLLAUSPDATA		atAuspData[BLOCKSIZE], 
						*ptArtData = NULL, *ptAuspData = NULL;
	STOCK_FILTER		tStockFilter;
	STOCK_DATA			tStockData;
	time_t				zEvening = 0, zMorning = 0, zNow=0;
	int					nCnt, nCnt2, iRv, iRvDb, iFirst;
	long				lAnzAuskUpd = 0, lAnzArt = 0, lAnzAusp = 0;


	/*
	 * 1. Lesen der AUSP, welche noch nicht (oder nicht vollstaendig) reserviert
	 *    	sind.
	 * 2. Aufbau eines Arrays pro MID
	 * 3. Aufbau eines Arrays pro AUSP (Sortierung BestZeit, AUSP.AusId, 
	 *		AUSP.PosNr
	 * 4. Ermittlung Bestand pro MID inkl. bereits reservierten Bestand
	 * 5. Zuteilung der verfuegbaren Menge
	 * 6. Schreiben der AUSP
	 * 7. Schreiben der AUSK
	 */


	/* --- read data for each AUSK --- */

	for (nCnt = 0; nCnt < iAnzAusk; nCnt ++) {

		iRvDb = iFirst = 0;

		do {
			memset (atAuspData, 0, sizeof (atAuspData));

			iRvDb = iRvDb == 0
			? TExecSqlX (pvTid, NULL,
					"SELECT "
					"AUSP.MId_AId_Mand, AUSP.MId_AId_ArtNr, "
					"AUSP.MId_AId_Var, AUSP.MId_MatKz, "
					"AUSP.PosNr, AUSP.BestMngs_Mng, "
					"AUSP.UrBestMng, AUSP.VollMng "
					"FROM "
					" AUSK, AUSP "
					"WHERE "
					"AUSK.AusId_Mand = :mand AND "
					"AUSK.AusId_AusNr = :ausnr AND "
					"AUSK.AusId_AusKz = :auskz AND "
					"AUSK.AusId_TlNr = :tlnr AND "
					"AUSK.Status = "STR_AUSKSTATUS_FREIG" AND "
					"AUSK.AusId_Mand = AUSP.AusId_Mand AND "
					"AUSK.AusId_AusNr = AUSP.AusId_AusNr AND "
					"AUSK.AusId_AusKz = AUSP.AusId_AusKz AND "
					"AUSK.AusId_TlNr = AUSP.AusId_TlNr AND "
					"AUSP.ThmKz = 0 AND "
					"AUSP.ResNr like ' %%' AND "
					"AUSP.TeId like ' %%' AND "
					"AUSP.SpKz like ' %%' AND "
					"AUSP.Status = "STR_AUSPSTATUS_NEU" AND "
					"AUSP.CMD = "STR_AUSPCOMMAND_OK" AND "
					"AUSP.OrigPosNr = 0 AND "
					"AUSP.VollMng < AUSP.BestMngs_Mng "
					"ORDER BY "
					" AUSK.BestZeit,"
					" AUSK.AusId_Mand,"
					" AUSK.AusId_AusNr,"
					" AUSK.AusId_AusKz,"
					" AUSK.AusId_TlNr,"
					" AUSP.PosNr",
					BLOCKSIZE, 0,
					SELOFSSTR (atAuspData[0].tMatId.AId.Mand,
							   MAND_LEN + 1, sizeof(VOLLAUSPDATA)),
					SELOFSSTR (atAuspData[0].tMatId.AId.ArtNr,
							   ARTNR_LEN + 1, sizeof(VOLLAUSPDATA)),
					SELOFSSTR (atAuspData[0].tMatId.AId.Var,
							   VAR_LEN + 1, sizeof(VOLLAUSPDATA)),
					SELOFSSTR (atAuspData[0].tMatId.MatKz,
							   MATKZ_LEN + 1, sizeof(VOLLAUSPDATA)),
					SELOFSLONG (atAuspData[0].lPosNr,
								sizeof(VOLLAUSPDATA)),
					SELOFSDOUBLE (atAuspData[0].tBestMngs.Mng,
								  sizeof(VOLLAUSPDATA)),
					SELOFSDOUBLE (atAuspData[0].dAuspUrBestMng,
								  sizeof(VOLLAUSPDATA)),
					SELOFSDOUBLE (atAuspData[0].dAuspVollMng,
								  sizeof(VOLLAUSPDATA)),
					SQLSTRING (ptAusk[nCnt].auskAusId.Mand),
					SQLSTRING (ptAusk[nCnt].auskAusId.AusNr),
					SQLSTRING (ptAusk[nCnt].auskAusId.AusKz),
					SQLLONG (ptAusk[nCnt].auskAusId.TlNr),
					NULL)
			: TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);

			if (((iRvDb <= 0) && (iFirst == 0)) ||
				((TSqlError (pvTid) != 0) &&
									(TSqlError (pvTid) != SqlNotFound))) {

				if (TSqlError (pvTid) == SqlNotFound){
					LogPrintf (pcFac, LT_ALERT,
						"<_CheckVollstaendigkeit>: no AUSP for "
						"AUSK [%s/%s/%s/%ld] found",
						ptAusk[nCnt].auskAusId.Mand,
						ptAusk[nCnt].auskAusId.AusNr,
						ptAusk[nCnt].auskAusId.AusKz,
						ptAusk[nCnt].auskAusId.TlNr);
					continue;
				}

				LogPrintf (pcFac, LT_ALERT,
					"<_CheckVollstaendigkeit> read data for AUSK [%s/%s/%s/%ld]"
					" failed - SQL: %s",
					ptAusk[nCnt].auskAusId.Mand,
					ptAusk[nCnt].auskAusId.AusNr,
					ptAusk[nCnt].auskAusId.AusKz,
					ptAusk[nCnt].auskAusId.TlNr,
					TSqlErrTxt (pvTid));
				SetErrmsg1(TSqlErrTxt(pvTid));
				return (-1);
			}

			/* Add the data to the arrdesc */
			if (FromAusp2ArtVoll(pvTid, pcFac, &ptAusk[nCnt],
									atAuspData, iRvDb, ptArrPtr) < 0) {
				return (-1);
			}

			if (ptAuspPtr != NULL) {
				if (FillAuspDataVoll (pvTid, pcFac, &ptAusk[nCnt],
								  atAuspData, iRvDb, ptAuspPtr) < 0) {
					return (-1);
				}
			}

			iFirst ++;

		} while (iRvDb == BLOCKSIZE);

	} /* end for */

	/* Sort of the Arrays */
	ArrSort (ptArrPtr, NULL, NULL);
	ArrSort (ptAuspPtr, NULL, NULL);

	/* --- article info --- */
	lAnzArt = (long)ArrGet(ptArrPtr, ArrNnoEle);

	LogPrintf (pcFac, LT_DEBUG,
			   "<_CheckVollstaendigkeit>: count Art: [%ld]", lAnzArt);

	if (lAnzArt == 0) {
		return (0);
	}

	zNow = time(0);
	zMorning = morning (zNow);
	zEvening = evening (zNow);

	StrForm(MlM("Der Auftrag %d"), tVplk.vplkKsNr);

	if (iAnzAusk > 1) {
		iRv = WamasBox(SHELL_OF(pvTid),
				WboxNboxType,           WBOX_INFO,
				WboxNmwmTitle,  		MlM("Vollst�ndigkeitspr�fung"),
				WboxNtext,				StrForm(
										MlMsg("Vollst�ndigkeitspr�fung\n f�r %d "
										"Auftr�ge durchf�hren?"), iAnzAusk),
				WboxNinputEf,           "WEAK_LiefTerm_t",
				WboxNinputEfLabel,      "Liefertermin von:",
				WboxNinputEfVStruct,    (void *)&zMorning,
				WboxNinputEfKey,        KEY_DEF,
				WboxNinputEf,           "WEAK_LiefTerm_t",
				WboxNinputEfLabel,      "Liefertermin bis:",
				WboxNinputEfVStruct,    (void *)&zEvening,
				WboxNinputEfKey,        KEY_1,
				WboxNbuttonText,        MlMsg("OK"),
				WboxNbuttonRv,          JANEIN_J,
				WboxNbutton,            WboxbNcancel,
				WboxNbuttonRv,          JANEIN_N,
				WboxNescButton,         WboxbNcancel,
				WboxNfocusEf,			WboxbNcancel,
				NULL);
	} else {
		iRv = WamasBox(SHELL_OF(pvTid),
				WboxNboxType,           WBOX_INFO,
				WboxNmwmTitle,  		MlM("Vollst�ndigkeitspr�fung"),
				WboxNtext,				StrForm(
										MlMsg("Vollst�ndigkeitspr�fung\n f�r %d "
										"Auftrag durchf�hren?"), iAnzAusk),
				WboxNinputEf,           "WEAK_LiefTerm_t",
				WboxNinputEfLabel,      "Liefertermin von:",
				WboxNinputEfVStruct,    (void *)&zMorning,
				WboxNinputEfKey,        KEY_DEF,
				WboxNinputEf,           "WEAK_LiefTerm_t",
				WboxNinputEfLabel,      "Liefertermin bis:",
				WboxNinputEfVStruct,    (void *)&zEvening,
				WboxNinputEfKey,        KEY_1,
				WboxNbuttonText,        MlMsg("OK"),
				WboxNbuttonRv,          JANEIN_J,
				WboxNbutton,            WboxbNcancel,
				WboxNbuttonRv,          JANEIN_N,
				WboxNescButton,         WboxbNcancel,
				WboxNfocusEf,			WboxbNcancel,
				NULL);
	}

	if(iRv == JANEIN_N) {
		LogPrintf (pcFac, LT_ALERT, 
				"<_CheckVollstaendigkeit>: Eingabe abgebrochen");
		return (0);
	}

	for (nCnt = 0; nCnt < lAnzArt; nCnt ++){
		ptArtData = (VOLLAUSPDATA *)ArrGetElem(ptArrPtr, nCnt);
		if (ptArtData == NULL) {
			LogPrintf (pcFac, LT_ALERT,
				"<_CheckVollstaendigkeit>: get ART-element [%d] of "
				"VOLLAUSPDATA array failed", nCnt);
			continue;
		}

		/* --- get stock for ART --- */
		memset (&tStockFilter, 0, sizeof (tStockFilter));
		memset (&tStockData, 0, sizeof (tStockData));

		tStockFilter.tMId = ptArtData->tMatId;
		tStockFilter.lSimpleVarKz = 0;
		tStockFilter.lCalcHe = 0;
		tStockFilter.lDetAvbQty = 0;
		/* --- Use Anmeldemenge --- */
		tStockFilter.lInclWeAQty = 1;
		tStockFilter.zLowTime = zMorning;
		tStockFilter.zHighTime = zEvening;

		iRv = Stock_GetStock4Art (pvTid, pcFac, &tStockFilter, &tStockData);

		if (iRv < 0) {
			LogPrintf (pcFac, LT_ALERT,
						"Error STOCK for ART [%s/%s/%s] MatKz [%s] failed",
						ptArtData->tMatId.AId.Mand, 
						ptArtData->tMatId.AId.ArtNr,
						ptArtData->tMatId.AId.Var,
						ptArtData->tMatId.MatKz);
			return (-1);
		}

		iRv = TExecSql (pvTid, 	
				"SELECT "
					"SUM(NVL(AUSP.VollMng, 0) / "
					"DECODE(AUSP.BestMngs_VeHeFa, 0, 1, AUSP.BestMngs_VeHeFa)) "
				"FROM "
					"AUSK, AUSP "
				"WHERE "
					"AUSK.KzWeinWelt = 1 "
					"AND AUSP.AusId_Mand = AUSK.AusId_Mand "
					"AND AUSP.AusId_AusNr = AUSK.AusId_AusNr "
					"AND AUSP.AusId_AusKz = AUSK.AusId_AusKz "
					"AND AUSP.AusId_TlNr = AUSK.AusId_TlNr "
					"AND AUSP.MId_AId_Mand = :a "
					"AND AUSP.MId_AId_ArtNr = :b "
					"AND AUSP.MId_AId_Var = :c "
					"AND AUSP.MId_MatKz = :d "
					"AND AUSP.ResNr like ' %%' "
					"AND AUSP.TeId like ' %%' "
					"AND AUSP.SpKz like ' %%' "
					"AND AUSP.ThmKz = 0 "
					"AND NOT EXISTS ( "
					"	SELECT 1 FROM VPLP WHERE "
					"    AUSP.AusId_Mand = VPLP.AusId_Mand AND "
					"    AUSP.AusId_AusNr = VPLP.AusId_AusNr AND"
					"    AUSP.AusId_AusKz = VPLP.AusId_AusKz AND"
					"    AUSP.AusId_TlNr = VPLP.AusId_TlNr  AND"
					"    AUSP.PosNr = VPLP.PosNr "
					" ) ",
				SELDOUBLE (ptArtData->dSumVollMngGe),
				SQLSTRING (ptArtData->tMatId.AId.Mand),
				SQLSTRING (ptArtData->tMatId.AId.ArtNr),
				SQLSTRING (ptArtData->tMatId.AId.Var),
				SQLSTRING (ptArtData->tMatId.MatKz),
				NULL);	

		if (iRv < 0) {
			LogPrintf (pcFac, LT_ALERT,
						"Error Select AUSP ART [%s/%s/%s] "
						"MatKz [%s] failed [%s]",
						ptArtData->tMatId.AId.Mand, 
						ptArtData->tMatId.AId.ArtNr,
						ptArtData->tMatId.AId.Var,
						ptArtData->tMatId.MatKz,
						TSqlErrTxt(pvTid));
			return -1;
		}

		ptArtData->dIstMng = tStockData.atSTD[STOCK_TYPE_FRE].dQtyEve;
		ptArtData->dLiMng = tStockData.atSTD[STOCK_TYPE_WEA].dQtyEve;

		ptArtData->dMngVer = 	
						ptArtData->dIstMng +		/* Freie Menge */
						ptArtData->dLiMng -			/* We-Menge */
						ptArtData->dSumVollMngGe;	/* bereits reservierte 															 * Vollstaendigkeitsmenge 
													 */

		LogPrintf (pcFac, LT_DEBUG,
			"<_CheckVollstaendigkeit>: AId [%s/%s/%s] MatKz [%s] - "
			"BestMngGesamt [%.3lf] VollMngGesamtOffen [%.3lf] "
			"VollMngGesamtReserviert [%.3lf] IstMng [%.3lf] "
			"WeMng [%.3lf] verfuegbare Menge [%.3lf]",
			ptArtData->tMatId.AId.Mand, ptArtData->tMatId.AId.ArtNr,
			ptArtData->tMatId.AId.Var, ptArtData->tMatId.MatKz,
			ptArtData->dSollMngGe, ptArtData->dVollMngGe,
			ptArtData->dSumVollMngGe, ptArtData->dIstMng, 
			ptArtData->dLiMng, ptArtData->dMngVer);
	}

	/* --- AUSP info --- */
	if (ptAuspPtr != NULL) {
		lAnzAusp = (long)ArrGet(ptAuspPtr, ArrNnoEle);
		LogPrintf (pcFac, LT_DEBUG,
				   "<_CheckVollstaendigkeit>: count Ausp: [%ld]", lAnzAusp);

		for (nCnt = 0; nCnt < lAnzAusp; nCnt ++) {
			ptAuspData = (VOLLAUSPDATA *)ArrGetElem(ptAuspPtr, nCnt);
			if (ptAuspData == NULL) {
				LogPrintf (pcFac, LT_ALERT,
					"<_CheckVollstaendigkeit>: get AUSP-element [%d] of "
					"VOLLAUSPDATA array failed", nCnt);
				continue;
			}

			LogPrintf (pcFac, LT_DEBUG,
				"<_CheckVollstaendigkeit>: AUSP [%s/%s/%s/%ld/%ld] "
				"AId [%s/%s/%s] MatKz [%s] - "
				"BestMng [%.3lf] VollMng [%.3lf] offen [%.3lf]",
				ptAuspData->tAusId.Mand, ptAuspData->tAusId.AusNr,
				ptAuspData->tAusId.AusKz, ptAuspData->tAusId.TlNr,
				ptAuspData->lPosNr,
				ptAuspData->tMatId.AId.Mand, ptAuspData->tMatId.AId.ArtNr,
				ptAuspData->tMatId.AId.Var, ptAuspData->tMatId.MatKz,
				ptAuspData->tBestMngs.Mng,
				ptAuspData->dAuspVollMng, ptAuspData->dOffenMng);
		}
	}

	/* --- walk through art array --- */
	for (nCnt = 0; nCnt < lAnzArt; nCnt ++){
		ptArtData = (VOLLAUSPDATA *)ArrGetElem(ptArrPtr, nCnt);
		if (ptArtData == NULL) {
			LogPrintf (pcFac, LT_ALERT,
				"<_CheckVollstaendigkeit>: get ART-element [%d] of "
				"VOLLAUSPDATA array failed", nCnt);
			continue;
		}
		/* --- walk through ausp array --- */
		for (nCnt2 = 0; nCnt2 < lAnzAusp; nCnt2 ++) {
			ptAuspData = (VOLLAUSPDATA *)ArrGetElem(ptAuspPtr, nCnt2);
			if (ptAuspData == NULL) {
				LogPrintf (pcFac, LT_ALERT,
					"<_CheckVollstaendigkeit>: get AUSP-element [%d] of "
					"VOLLAUSPDATA array failed", nCnt2);
				continue;
			}

			if (memcmp(&ptArtData->tMatId, 
				&ptAuspData->tMatId, 
				sizeof(MID)) == 0)
			{
				if (ptArtData->dMngVer > 0) {
					/* --- genug fuer ganze AUSP --- */
					if (ptAuspData->dOffenMng < ptArtData->dMngVer) {
						ptAuspData->dAuspVollMng += ptAuspData->dOffenMng;
						ptArtData->dMngVer -= ptAuspData->dOffenMng;
					} else
					/* --- Teil der AUSP kann reserviert werden --- */
					if (ptArtData->dMngVer < ptAuspData->dOffenMng ) {
						ptAuspData->dAuspVollMng += ptArtData->dMngVer;
						ptArtData->dMngVer = 0;
						ptAuspData->iUpdate = 1;
					} else {
						ptAuspData->dAuspVollMng += ptArtData->dMngVer;
						ptArtData->dMngVer = 0;
					}

					if (TExecSql(pvTid, "UPDATE "
											"AUSP "
										"SET "
											"AUSP.VollMng=:a, "
											"AUSP.Hist_AeZeit =:b, "
											"AUSP.Hist_AeUser =:c "
										"WHERE "
											"AUSP.AusId_Mand=:d AND "
											"AUSP.AusId_AusNr=:e AND "
											"AUSP.AusId_AusKz=:f AND "
											"AUSP.AusId_TlNr=:g AND "
											"AUSP.PosNr=:h",
									 SQLDOUBLE(ptAuspData->dAuspVollMng),
									 SQLTIMET(zNow),
									 SQLSTRING(GetUserOrTaskName()),
									 SQLSTRING(ptAuspData->tAusId.Mand),
									 SQLSTRING(ptAuspData->tAusId.AusNr),
									 SQLSTRING(ptAuspData->tAusId.AusKz),
									 SQLLONG(ptAuspData->tAusId.TlNr),
									 SQLLONG(ptAuspData->lPosNr),
									 NULL) != 1) {
						LogPrintf (pcFac, LT_ALERT,
									"Error Update AUSP [%s/%s/%s/%ld/%ld] "
									"failed [%s]",
									ptAuspData->tAusId.Mand, 
									ptAuspData->tAusId.AusNr,
									ptAuspData->tAusId.AusKz, 
									ptAuspData->tAusId.TlNr,
									ptAuspData->lPosNr,
									TSqlErrTxt(pvTid));
						return -1;
					}

					LogPrintf (pcFac, LT_DEBUG,
						"<Setting>: AUSP [%s/%s/%s/%ld/%ld] - "
						"ART [%s/%s/%s] MatKz [%s] "
						"BestMng [%.3lf] VollMng [%.3lf] "
						"weiter verfuegbar [%.3lf]",
						ptAuspData->tAusId.Mand, 
						ptAuspData->tAusId.AusNr,
						ptAuspData->tAusId.AusKz, 
						ptAuspData->tAusId.TlNr,
						ptAuspData->lPosNr,
						ptAuspData->tMatId.AId.Mand, 
						ptAuspData->tMatId.AId.ArtNr,
						ptAuspData->tMatId.AId.Var, 
						ptAuspData->tMatId.MatKz,
						ptAuspData->tBestMngs.Mng, 
						ptAuspData->dAuspVollMng, 
						ptArtData->dMngVer);

					if (ptAuspData->iUpdate == 1) {
						LogPrintf (FAC_LSM_AUSCHECKER, LT_ALERT,
							"AUSP [%s/%s/%s/%ld/%ld] ART [%s/%s/%s] MatKz [%s] "
							"BestMng [%.3lf] VollMng [%.3lf] "
							"Weinwelt: Bestand nicht ausreichend",
							ptAuspData->tAusId.Mand, 
							ptAuspData->tAusId.AusNr,
							ptAuspData->tAusId.AusKz, 
							ptAuspData->tAusId.TlNr,
							ptAuspData->lPosNr,
							ptAuspData->tMatId.AId.Mand, 
							ptAuspData->tMatId.AId.ArtNr,
							ptAuspData->tMatId.AId.Var, 
							ptAuspData->tMatId.MatKz,
							ptAuspData->tBestMngs.Mng, 
							ptAuspData->dAuspVollMng);
					}
				} else {
					LogPrintf (FAC_LSM_AUSCHECKER, LT_ALERT,
						"AUSP [%s/%s/%s/%ld/%ld] ART [%s/%s/%s] MatKz [%s] "
						"BestMng [%.3lf] VollMng [%.3lf] "
						"Weinwelt: Bestand nicht ausreichend",
						ptAuspData->tAusId.Mand, 
						ptAuspData->tAusId.AusNr,
						ptAuspData->tAusId.AusKz, 
						ptAuspData->tAusId.TlNr,
						ptAuspData->lPosNr,
						ptAuspData->tMatId.AId.Mand, 
						ptAuspData->tMatId.AId.ArtNr,
						ptAuspData->tMatId.AId.Var, 
						ptAuspData->tMatId.MatKz,
						ptAuspData->tBestMngs.Mng, 
						ptAuspData->dAuspVollMng);
				}
			}
		}
	}

	/* --- Check AUSK --- */
	nCnt2 = 0;
	for (nCnt = 0; nCnt < iAnzAusk; nCnt ++) {

		iRv = TExecSql (pvTid, 	
				"SELECT "
					"COUNT(AUSP.PosNr) "
				"FROM "
					"AUSP "
				"WHERE "
					"AUSP.AusId_Mand = :a "
					"AND AUSP.AusId_AusNr = :b "
					"AND AUSP.AusId_AusKz = :c "
					"AND AUSP.AusId_TlNr = :d "
					"AND AUSP.ResNr like ' %%' "
					"AND AUSP.TeId like ' %%' "
					"AND AUSP.SpKz like ' %%' "
					"AND AUSP.ThmKz = 0 "
					"AND AUSP.VollMng < AUSP.BestMngs_Mng ",
				SELLONG (lAnzAuskUpd),
				SQLSTRING(ptAusk[nCnt].auskAusId.Mand),
				SQLSTRING(ptAusk[nCnt].auskAusId.AusNr),
				SQLSTRING(ptAusk[nCnt].auskAusId.AusKz),
				SQLLONG(ptAusk[nCnt].auskAusId.TlNr),
				NULL);	

		if (iRv <= 0) {
			LogPrintf (pcFac, LT_ALERT,
						"Error Select AUSK [%s/%s/%s/%ld "
						"failed [%s]",
						ptAusk[nCnt].auskAusId.Mand,
						ptAusk[nCnt].auskAusId.AusNr,
						ptAusk[nCnt].auskAusId.AusKz,
						ptAusk[nCnt].auskAusId.TlNr,
						TSqlErrTxt(pvTid));

			return -1;
		}

		/* --- Es ist nichts mehr offen --- */
		if (lAnzAuskUpd == 0) {

			long lStatus = AUSKSTATUS_BEREIT;

			if (TExecSql(pvTid, "UPDATE "
									"AUSK "
								"SET "
									"AUSK.Status=:a, "
									"AUSK.KzVollStaendig = 1, "
									"AUSK.Hist_AeZeit =:b, "
									"AUSK.Hist_AeUser =:c "
								"WHERE "
									"AUSK.AusId_Mand=:d AND "
									"AUSK.AusId_AusNr=:e AND "
									"AUSK.AusId_AusKz=:f AND "
									"AUSK.AusId_TlNr=:g",
								SQLAUSKSTATUS(lStatus),
								SQLTIMET(zNow),
								SQLSTRING(GetUserOrTaskName()),
								SQLSTRING(ptAusk[nCnt].auskAusId.Mand),
								SQLSTRING(ptAusk[nCnt].auskAusId.AusNr),
								SQLSTRING(ptAusk[nCnt].auskAusId.AusKz),
								SQLLONG(ptAusk[nCnt].auskAusId.TlNr),
								 NULL) != 1) {

				LogPrintf (pcFac, LT_ALERT,
							"Error Update AUSK [%s/%s/%s/%ld] failed [%s]",
							ptAusk[nCnt].auskAusId.Mand,
							ptAusk[nCnt].auskAusId.AusNr,
							ptAusk[nCnt].auskAusId.AusKz,
							ptAusk[nCnt].auskAusId.TlNr,
							TSqlErrTxt(pvTid));
				return -1;
			}

			nCnt2++;

			LogPrintf (pcFac, LT_DEBUG,
							"Status von  AUSK [%s/%s/%s/%ld] auf [%s] gesetzt",
							ptAusk[nCnt].auskAusId.Mand,
							ptAusk[nCnt].auskAusId.AusNr,
							ptAusk[nCnt].auskAusId.AusKz,
							ptAusk[nCnt].auskAusId.TlNr,
							l2sGetNameByValue(&l2s_AUSKSTATUS, lStatus));
		}
	}

	(*piAnzAuskUpd) = nCnt2;

	return (1);
}
