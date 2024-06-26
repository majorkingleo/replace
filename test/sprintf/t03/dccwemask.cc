/*****************************************************************************
+* PROJECT:   SCHLAU
+* PACKAGE:   package name
+* FILE:	  dccwemask.c
+* CONTENTS:  overview description, list of functions, ...
+* COPYRIGHT NOTICE:
+*		 (c) Copyright 2001 by
+*				 Salomon Automationstechnik Ges.m.b.H
+*				 Friesachstrasse 15
+*				 A-8114 Friesach bei Graz
+*				 Tel.: ++43 3127 200-0
+*				 Fax.: ++43 3127 200-22
+*
+****************************************************************************/

/* ===========================================================================
 * INCLUDES
 * =========================================================================*/

/* ------- System-Headers ------------------------------------------------- */
#include <math.h>
/* ------- Owil-Headers --------------------------------------------------- */

/* ------- Tools-Headers -------------------------------------------------- */
#include <dbsqlstd.h>
#include <ml.h>
#include <mlmsg.h>
#include <mlpack.h>
#include <errmsg.h>
#include <logtool2.h>
#include <hist_util.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "ean13.h"
#include "art.h"
#include "tep.h"
#include "lbd.h"

#include "pos_util.h"
#include "vpl_util.h"
#include "mkz_util.h"
#include "facility.h"
#include "parameter.h"
#include "te_util.h"
#include "te_util2.h"
#include "init_table.h"
#include "funkglob.h"
#include "radfac.h"
#include "dcckutil.h"
#include "dcclic.h"
#include "dcclogin.h"
#include "dccutil.h"
#include "dccvkeys.h"
#include "dccapputil.h"
#include "dccwe.h"
#include "dcc_util.h"
#include "dccwemask2.h"
#include "mng_util.h"
#include "wea_util.h"
#include "dccweutil.h"
#include "dccsumgew.h"
#include "dummy_util.h"
#include "te_typ_from_mng.h"
#include "dcclst.h"
#include "dcclstutil.h"
#include "lb_bew.h"
#include "dcccrossdck.h"
#include "dcccrossdck_lekk.h"	
#include "wea_util2.h"
#include "voice_util.h"
#include <crossdock.h>
#include "dccretoure.h"
#include <cpp_util.h>

#define _DCCWEMASK_C
#include "dccwemask.h"
#undef _DCCWEMASK_C

using namespace Tools;


/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/
#define SCANCODE 	100
#define BLOCKSIZE	10
#define KPL_BLOCKSIZE_WE	2
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
static int _doLiefBewJob(dccw_context *dccw, dccclient_stm *scl);
static int _checkSerNr(dccw_context *dccw, dccclient_stm *scl);
static void SmaBuildTEBildenSerNr(dccw_context *dccw,dccclient_stm *scl,
						int iEanArt, int iScanTE);		
static int _CheckInfo(FS_Job_Context  *fs, std::string & pcMsg);

void SmaBuildScdAvisAus(dccw_context *dccw,dccclient_stm *scl);
int SmaBuildScdErfass(dccw_context *dccw,dccclient_stm *scl);
int SmaBuildScdErfass2(dccw_context *dccw,dccclient_stm *scl, 
		AUSK *ptAusk, AUSP *ptAusp);
															
/* ===========================================================================
 * GLOBAL VARIABLES
 * =========================================================================*/

/* ===========================================================================
 * LOCAL (STATIC) Functions
 * =========================================================================*/
int KleinMngArtDirektAufKort(void *pvTid) 
{
	int	iWert=0; 

	if (PrmGet1Parameter (NULL, P_WeKleinMngArt,
					PRM_CACHE, &iWert) != PRM_OK) {
		/* Falls Parameter nicht gesetzt ist, wird
		-* er auf 1 (dierkt auf KORT buchen) gesetzt
		-*/
		iWert = 1; 
	}
	if (iWert == 1) {
		return 1; /* Direkt auf KORT buchen */
	} else {
		return 0; /* Mit TPA einlagern */
	}
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* DESCRIPTION 
-* There can be more than one message displayed one by one. 
-* This means that after first message displayed (and proccessed by 
-* SmaErrorTreatment), next one sets the value of fs->PreviousMask to ID_err.
-* In that case, we use global fs->iShowInfo which signalize that when previous
-* mask is ID_err and this flag is set, call ID_tebild mask, and otherwise
-* process as usually
-***********************************
-* TIP - for other Info messages, just expand aiDisplayedMessage and repeat
-* procedure as it is done with existing ones
-***********************************
-* RETURNS 
-* 0, there is nothing to display, 1 display message (pcMsg)
-*--------------------------------------------------------------------------*/ 
static int _CheckInfo(FS_Job_Context  *fs, std::string & pcMsg)
{
	long lValue = 0;
	static int aiDisplayedMessage[2] = {0, 0};
	static WEAID	tWeaIdTmp;
 	static int 		iPosNr = 0;
	
	/* Info should be displayed only on first opening of tebild mask */
	if (fs->iAllInfoDisplayed == 1) {
		return 0;
	}
	
	/* Check if WEAP is changed (ShowInfo is once pro WEAP) */
 	if (memcmp(&tWeaIdTmp, &fs->tWeap.weapWeaId, sizeof(WEAID)) == 0 &&
 		iPosNr == fs->tWeap.weapPosNr && 
		(IsEmptyStrg(fs->tWeap.weapArtBesch1) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch2) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch3) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch4) == 0)) {
 		if (fs->tWeap.weapShowInfo == 0 && 
 			IsEmptyStrg(fs->tWeap.weapInfo) == 0) {	
 			/* Set flag, but don't show it */
 			fs->tWeap.weapShowInfo = 1;
 		}
 	}
	
	fs->iShowInfo = 1;
	tWeaIdTmp = fs->tWeap.weapWeaId;
 	iPosNr = fs->tWeap.weapPosNr;
	
	if (PrmGet1Parameter(NULL, P_WeShowInfo,
		  			PRM_CACHE, &lValue) != PRM_OK) {
		lValue = JANEIN_N;
	}	
	if (fs->tWeap.weapShowInfo == 0 && 
		lValue == JANEIN_J &&
		(IsEmptyStrg(fs->tWeap.weapArtBesch1) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch2) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch3) == 0 ||
		 IsEmptyStrg(fs->tWeap.weapArtBesch4) == 0)) {
		/* P_SPEZ_SCHLAU */
		/* Statt der Info wird die Artikelbeschreibung angezeigt */
		pcMsg = format("%s%s%s%s",
			fs->tWeap.weapArtBesch1,
			fs->tWeap.weapArtBesch2,
			fs->tWeap.weapArtBesch3,
			fs->tWeap.weapArtBesch4);
		fs->tWeap.weapShowInfo = 1;
		return 1;
	}
	if (fs->tWeap.weapPruefKz == 1 && aiDisplayedMessage[0] == 0) {
	    pcMsg = MlMsg("Ware muss gepr�ft werden!");
		aiDisplayedMessage[0] = 1;
		return 1;
	}
	if (PrmGet1Parameter(NULL, P_WeEveHeArt,
		  			PRM_CACHE, &lValue) != PRM_OK) {
		lValue = JANEIN_N;
	}
	if(lValue == 1 && 
		(fs->tArt.artMngs.VeHeFa != fs->tWeap.weapAnmMngs.VeHeFa) &&
		aiDisplayedMessage[1] == 0){
	    pcMsg = MlMsg("Artikelstamm EVE/HE weicht von Bestellung ab!");
		aiDisplayedMessage[1] = 1;
		return 1;
	}

	aiDisplayedMessage[0] = aiDisplayedMessage[1] = 0;
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 1;
	return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* DESCRIPTION
-*  get LieferantenBewertung codes and builds a list
-*--------------------------------------------------------------------------*/
static int _doLiefBewJob(dccw_context *dccw, dccclient_stm *scl)
{
	/* FS_Job_Context  *fs = (FS_Job_Context *)get_job_from_scl (scl); */
	char            acMsg[256+1];
	LBST			atLbst[BLOCKSIZE];
	int				iDbRv = 0;
	ArrDesc         tArrDesc;
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	
	
	memset (&tArrDesc, 0, sizeof (tArrDesc));
	
	do {
		memset (atLbst, 0, sizeof(atLbst));
		atLbst[0].lbstEbene = LBEBENE_POSITION;
		/* Read all LiefBewCodes with field Ebene = LBEBENE_POSITION */
		iDbRv = ExecStdSqlX (NULL, StdNselect, TN_LBST, atLbst, 
								BLOCKSIZE, FN_LBST_Ebene, NULL);

		/* Error check */
		if (iDbRv <= 0) {
			if (SqlError == SqlNotFound) {
			    StrCpyDestLen(acMsg,
					MlMsg("Kein BEWERTUNG CODE gefunden!"));
            	SendError(dccw, scl, acMsg, DCC_ERROR,
                			DCC_WRITE_LOG, GetFunkFac(fs));
				return 0;
			}
			StrCpyDestLen(acMsg,
				MlMsg("Datenbank Fehler!"));
        	SendError(dccw, scl, acMsg, DCC_ERROR,
            			DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}
		if (AppendEle (&tArrDesc, atLbst, iDbRv, sizeof(LBST)) < 0) {
            LogPrintf (GetFunkFac(fs), LT_ALERT, "Error AppendEle LBST");
            return (-1);
        }
	} while (iDbRv == BLOCKSIZE);
	
	/* Send next mask inside MakeInfoList */
	if (MakeInfoList (dccw, scl, GetFunkFac(fs), 
				&tArrDesc, InfoListeLiefBew) < 0) {
		LogPrintf (GetFunkFac(fs), LT_ALERT, "ERROR: MakeInfoList failed!");
    	return (-1);			
	}
	
	return 1;
} 
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* DESCRIPTION
-*  Function checks if Serial numbers are entered, chack their validity and 
-*	perform number conversions and calculations. At the end, for range of 
-*	serial numbers, sets the Menge for each new TEP to 1
-* RETURNS
-*  -1  error, 1 ok
-*--------------------------------------------------------------------------*/ 
static int _checkSerNr(dccw_context *dccw, dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acMsg[129];
	char			acSerNr[2][SERNR_LEN + 1];
	long    		lFlags = 0;
	long 			lS1 = 0, lS2 = 0, lDiff = 0, lSerLen=0;
	int				nI = 0, nJ = 0, iRv = 0;
	double			dMng = 0;
	int				iSerNrLen1 = 0, iSerNrLen2 = 0;
	char 			*pcTemp;
	
	fs->iL2Erf = 0;
	if (fs->iScanTE == JANEIN_N) {
		/* First time -> check seriennummer */
		memset (acMsg, 0, sizeof(acMsg));
		memset (acSerNr, 0, sizeof(acSerNr));
		
		strcpy(acSerNr[0], dccmask_get_field(dccw, scl,
	                    efname(ef_SerNr1), &lFlags, NULL));
	
		strcpy(acSerNr[1], dccmask_get_field(dccw, scl,
	                    efname(ef_SerNr2), &lFlags, NULL));
	             
		
		dMng = GetDoubleFromRadioInput(dccw, scl,
	                efname(ef_AnzStueck), &lFlags, NULL);
		
		dMng += GetDoubleFromRadioInput(dccw, scl,
	                efname(ef_AnzHE), &lFlags, NULL) * 
	          	GetDoubleFromRadioInput(dccw, scl,
	                efname(ef_VeHeFa), &lFlags, NULL);    
	                
	    /* Check if S1 is entered */
		if (IsEmptyStrg(acSerNr[0]) == 1) {
		    StrCpyDestLen(acMsg,
				"Bitte Seriennummer (S1) eingeben!");
			SendError(dccw,scl, acMsg, DCC_ERROR,
		   		DCC_WRITE_LOG, GetFunkFac(fs));                	
		   	return (-1);	
		}
		/* Check if S2 is entered */
		if (IsEmptyStrg(acSerNr[1]) != 1) {
			/* In that case, both S1 and S2 must be numeric and S2 > S1 */	
			for (nI = 0; nI < 2; nI ++) {
				lSerLen = strlen(acSerNr[nI]);
				for (nJ = 0; nJ < lSerLen; nJ ++) {
					if (acSerNr[nI][nJ] < 48 || acSerNr[nI][nJ] > 57){
					    StrCpyDestLen(acMsg,format(
							"Seriennummer (%s) muss nummerisch sein!", 
							nI == 0 ? "S1" : "S2"));
						SendError(dccw,scl, acMsg, DCC_ERROR,
					   		DCC_WRITE_LOG, GetFunkFac(fs));
					   	return (-1);	
		        	}
		    	}
			}
			iSerNrLen1 = strlen(acSerNr[0]);
			iSerNrLen2 = strlen(acSerNr[1]);
			
			if (iSerNrLen1 > iSerNrLen2) {
			    StrCpyDestLen(acMsg,
					"Seriennummer S2 muss gr�sser als S1 sein!");
				SendError(dccw,scl, acMsg, DCC_ERROR,
			   		DCC_WRITE_LOG, GetFunkFac(fs));
			   	return (-1);	
			}
			
			if (iSerNrLen1 <= 9){
				lS1 = atol(acSerNr[0]);
			} else {
				pcTemp = &acSerNr[0][iSerNrLen1 - 9];
				lS1 = atol(pcTemp);
				acSerNr[0][iSerNrLen1 - 9] = '\0';
			}	
			if (iSerNrLen2 <= 9){
				lS2 = atol(acSerNr[1]);
			} else {
				pcTemp = &acSerNr[1][iSerNrLen2 - 9];
				lS2 = atol(pcTemp);
				acSerNr[1][iSerNrLen2 - 9] = '\0';
			}
			if (iSerNrLen1 > 9) {
				if (strncmp(acSerNr[0], acSerNr[1], iSerNrLen1 - 9) != 0){
				    StrCpyDestLen(acMsg,
						"Seriennummern sind falsch!");
					SendError(dccw,scl, acMsg, DCC_ERROR,
			   			DCC_WRITE_LOG, GetFunkFac(fs));
			   		return (-1);
				}
			}
			
			lDiff = lS2 - lS1 + 1;
			/* If user has entered s1 and s2 and not Menge, then calculate,
			-*	otherwise, make 'diff check' 
			*/
			if (dMng!= (double) lDiff) {
			    StrCpyDestLen(acMsg,format(
					"EVE falsch! Muss %ld sein!", lDiff));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				return (-1);	
			}
		} else {
			/* Check Menge => EVE must be 1 */
			if (dMng != (double) 1) {
			    StrCpyDestLen(acMsg,
					"Menge ungleich 1 EVE!");
				SendError(dccw,scl, acMsg, DCC_ERROR,
			   		DCC_WRITE_LOG, GetFunkFac(fs));
			   	return (-1);	
			}
		}
	    strcpy(fs->tTep.tepMId.SerNr, dccmask_get_field(dccw, scl,
	                    		efname(ef_SerNr1), &lFlags, NULL));
	    strcpy(fs->acSerNr[0], dccmask_get_field(dccw, scl,
	                    		efname(ef_SerNr1), &lFlags, NULL));
	    strcpy(fs->acSerNr[1], dccmask_get_field(dccw, scl,
	                    		efname(ef_SerNr2), &lFlags, NULL));
	    
	} else { /* TE is scanned -> seriennummer art buchung */
		if (scl->endkey != DCCAPP_VK_MISCHTE) { /* Not Misch-TE-Key */

		    const char *pcTetId = dccmask_get_field(dccw, scl, efname(ef_TeTyp), &lFlags, NULL);
		    char acTetId[TETID_LEN+1] = {0};

		    if( pcTetId != NULL ) {
		        StrCpyDestLen( acTetId, pcTetId );
		    }


			if (_getTetId(dccw, scl, acTetId, fs->iMischCount) < 0){
				return 0;
			}                  		
			if (fs->eMode == DccGeplWeTeBilden) { /*Geplanter WE */
				iRv = GeplWeBuchen (NULL, GetFunkFac(fs), fs, 
									NULL, JANEIN_N, JANEIN_N);
				if (iRv >= 0 ) {
					if (iRv == 2) { /* Avis ist abgeschlossen */
						fs->iLastOk = JANEIN_N;
			        	fs->eMode = DccGeplWe;
			        	SmaBuildIndentifWeGepl(dccw,scl);
					} else {
						fs->iLastOk = JANEIN_J;
						fs->eMode = DccGeplWeEan;
						fs->iAnzTE ++;
						fs->iGebucht = JANEIN_J;
						SmaBuildScanEanWeGepl(dccw,scl);
					}
				} else if (iRv == ERROR_OLDERGOODS) {
				    StrCpyDestLen (acMsg,
							MlMsg ("Es gibt in der Reserve Ware mit �lterem "
								   "Fifo-Datum!"));
					SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
										GetFunkFac(fs));
				} else if (iRv == ERROR_UEBERLIEF) {
				    StrCpyDestLen (acMsg,
			            	MlMsg ("�berliefern nicht erlaubt!"));
			    	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
			                        	GetFunkFac(fs));
				} else if (iRv == ERROR_WRONGTEID) {
				    StrCpyDestLen (acMsg,
			            	MlMsg ("TE-Id steht auf keiner WE-Position!"));
			    	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
			                        	GetFunkFac(fs));
				} else {
					ErrorMsg(dccw, scl, iRv);
					StrCpyDestLen (acMsg,
			            	MlMsg ("Fehler beim Buchen des WE-Avises!"));
			    	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
			                        	GetFunkFac(fs));
				}
			} else if (fs->eMode == DccUngeplWeTeBilden) { /* Ungepl WE */
				iRv = UngeplWeBuchen (NULL, GetFunkFac(fs), fs, 
								NULL, JANEIN_N);
				if (iRv >= 0 ) {
					fs->iLastOk = JANEIN_J;
					fs->eMode = DccUngeplWeEan;
					fs->iAnzTE ++;
					fs->iGebucht = JANEIN_J;
					SmaBuildScanEanWeUngepl(dccw,scl);
				} else {
					if (iRv == ERROR_OLDERGOODS) {
					    StrCpyDestLen (acMsg,
								MlMsg ("Es gibt in der Reserve Ware mit �lterem "
									   "Fifo-Datum!"));
						SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
											GetFunkFac(fs));
						return (-1);
					}
					if (iRv != -1){
						ErrorMsg(dccw, scl, iRv);
					} else {
					    StrCpyDestLen (acMsg,
	                        MlMsg ("Fehler beim Buchen des WE-Avises!"));
						SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
	                                    GetFunkFac(fs));
					}
				}
			}
			if (iRv >= 0 ) {
				/* Reset MischFlag
				-*/
				iRv = ResetMischFlag(dccw, scl, GetFunkFac(fs));
			}
		}
	}
	return (iRv);                    		
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* DESCRIPTION
-*  Function checks or gets TeTyp
-* RETURNS
-*  -1  error, 1 ok
-*--------------------------------------------------------------------------*/
int _getTetId(dccw_context *dccw, dccclient_stm *scl,
												char *pcTetId, int iMischTe)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lCalcTeTypMng = 0, lAnzHe = 0;
	char			acMsg[129];
	int				iRv = 0;
	TTS				tTts;
	
	if(DCCAPP_VK_MISCHTE == scl->endkey){
		return 0;
	}

	memset(acMsg, 0, sizeof(acMsg));
	memset(&tTts, 0, sizeof(tTts));
	
	/* Get CalcTeTypMng parameter */
	if (PrmGet1Parameter(NULL, P_CalcTeTypMng,
		  			PRM_CACHE, &lCalcTeTypMng) != PRM_OK) {
		lCalcTeTypMng = JANEIN_N;
	}		
	
	/* Check if we must get TeTyp from Menge or from Artikelstamm */
	for (;;) {
		if(	IsEmptyStrg(pcTetId) == 1) {
			if (iMischTe == 0 && lCalcTeTypMng == JANEIN_J){
				/* Get TeTyp from Menge */	
				if (fs->tTep.tepMngs.VeHeFa == 0) {
					fs->tTep.tepMngs.VeHeFa = 1;
				}
				lAnzHe = (long) (fs->tTep.tepMngs.Mng / 
								fs->tTep.tepMngs.VeHeFa);
				if (lAnzHe < 0) {
				    StrCpyDestLen(acMsg,format(
					"Error getting Anzahl He in Te %s", fs->tTep.tepTeId));
					LogPrintf(GetFunkFac(fs), LT_ALERT, "%s", acMsg);
				/* No need to return, in next funct lAnzHe is set to 1 */
				}

				iRv = getTeTypFromMenge (NULL, &fs->tArt, 
											lAnzHe, &tTts);
				if (iRv < 0) {
					if (iRv == -2) {
					    StrCpyDestLen(acMsg, GetTeTypErr());
						SendError(dccw,scl, acMsg, DCC_ERROR,
					   		DCC_WRITE_LOG, GetFunkFac(fs));
					   	return -1;	
					}
					StrCpyDestLen(acMsg, GetTeTypErr());
							/* "Fehler bei TE-Typ from Menge ermitteln!"); */
					SendError(dccw,scl, acMsg, DCC_ERROR,
				   		DCC_WRITE_LOG, GetFunkFac(fs));
				   	return -1;
				}
				StrCpy(fs->TetId, tTts.ttsTet.TetId);
				break;
			} else {
				 /* Get Misch Te Typ parameter */
				if (iMischTe != 0) {
					if (PrmGet1Parameter(NULL, P_MischTeTyp,
					 PRM_CACHE, pcTetId) != PRM_OK) {
					    StrCpyDestLen(acMsg,
							"Fehler beim lesen Misch Te Typ parameter!");
						SendError(dccw,scl, acMsg, DCC_ERROR,
							DCC_WRITE_LOG, GetFunkFac(fs));
						return -1;
					}
				} else {
				    StrCpyDestLen(acMsg, "Bitte TE-Typ eingeben!");
					SendError(dccw,scl, acMsg, DCC_ERROR,
							DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}	

				if (IsEmptyStrg(pcTetId) == 1) {
				    StrCpyDestLen(acMsg, "Bitte TE-Typ eingeben!");
					SendError(dccw,scl, acMsg, DCC_ERROR,
				   		DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}
			}
		} 
		/* Check if TeTyp entered exists in table TTS */
		if (checkTeTyp_TTS(NULL, pcTetId, GetFunkFac(fs)) < 1) {
			LogPrintf (GetFunkFac(fs), LT_ALERT, "TE: %s TE-Typ (%s) ung�ltig!", 
						fs->tTep.tepTeId, pcTetId);
			StrCpyDestLen(acMsg, "TE-Typ ung�ltig!");
				SendError(dccw,scl, acMsg, DCC_ERROR,
				   		DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}
		LogPrintf (GetFunkFac(fs), LT_ALERT, "TE: %s TE-Typ: %s", 
						fs->tTep.tepTeId, pcTetId);
		StrCpyDestLen(fs->TetId, pcTetId);
		break;	
	}
	/* Check if TeTyp is undefined */
	if (strncmp(fs->TetId, "UNDEF", 5) == 0) {
		LogPrintf (GetFunkFac(fs), LT_ALERT, "TE: %s TE-Typ is UNDEF!", 
					fs->tTep.tepTeId);
		StrCpyDestLen(acMsg, "Fehler bei Stammdaten\n"
						"(TE-Typ = UNDEF)!");
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
	return 1;
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildTEBildenSerNr(dccw_context *dccw,dccclient_stm *scl,
-*						int iEanArt, int iScanTE)
-* DESCRIPTION
-*  Function builds the mask for WE with Serial numbers
-* RETURNS
-*  void
-*--------------------------------------------------------------------------*/
static void SmaBuildTEBildenSerNr(dccw_context *dccw,dccclient_stm *scl,
						int iEanArt, int iScanTE)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineLabelBuf[8][40];
	char			acLineValueBuf[11][40];
	int				iWert, iRv, iArtOfLp=0;
	app_msk_par  	*MskParData = NULL;
	POS				tPos;
	long			lCalcTeTypMng = 0;
	double			dHe = 0, dEve = 0;
	
	memset(acLineLabelBuf, 0, sizeof(acLineLabelBuf));
	memset(acLineValueBuf, 0, sizeof(acLineValueBuf));
	memset(&tPos, 0, sizeof(POS));

	/* Labels */
	if (fs->iMischCount == 0) {
	    StrCpy(acLineLabelBuf[0],	format("%20s",	MlMsg("   TE-Pos. bilden   ")));
	} else {
	    StrCpy(acLineLabelBuf[0],	format("%20s",	MlMsg("   Misch-TE bilden  ")));
	}
	
	StrCpy(acLineLabelBuf[1],	format("%s:", 	MlMsg("ArtNr")));
	StrCpy(acLineLabelBuf[2],	format("%s:", 	MlMsg("S1")));
	StrCpy(acLineLabelBuf[3],	format("%s:", 	MlMsg("S2")));
	StrCpy(acLineLabelBuf[4],	format("%s:", 	MlMsg("Mng")));
	StrCpy(acLineLabelBuf[5],	format("%s:", 	MlMsg("E/H")));
	StrCpy(acLineLabelBuf[6],	format("%s:", 	MlMsg("T")));
	StrCpy(acLineLabelBuf[7],	format("%s",	MlMsg("TE")));
	
	/* data */
	StrCpy(acLineValueBuf[0], fs->tArt.artAId.ArtNr);
	StrCpy(acLineValueBuf[1], fs->tArt.artArtBez);
	StrCpy(acLineValueBuf[4], "0");
	StrCpy(acLineValueBuf[5], fs->tArt.artHeEinheit);
	StrCpy(acLineValueBuf[6], "0");
	StrCpy(acLineValueBuf[7], fs->tArt.artVeEinheit);
    
	StrCpy(acLineValueBuf[8], format("%.0f", fs->tTep.tepMngs.VeHeFa));
    
    if (PrmGet1Parameter (NULL, P_GIFillTUId,
                        PRM_CACHE, &iWert) != PRM_OK) {
        iWert = 0;
    }
	if (iWert == JANEIN_J) {
	    StrCpy(acLineValueBuf[10], fs->tTep.tepTeId);
	} else {
		acLineValueBuf[10][0]='\0';
	}

	if (iEanArt == EANART128 && fs->iMaskeJN == 0) {

		if (PrmGet1Parameter (NULL, P_EditEAN128Data,
                        PRM_CACHE, &iWert) != PRM_OK) {
			iWert = 0;
		}

		/* Alle Daten zum bilden der TE vorhanden ? */
		iRv = AllDataForTep(&fs->tTep, &fs->tArt);

		if ((iWert == 1 || iRv <= 0) && iScanTE == 0) {
			
			/* Edit data of Ean128 */

			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTetId),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);             
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_TeTyp),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);       
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTe),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_ScTeNoMu),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
        	if (glLicenses[WKMT03] == 1 ) { 
				MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_GEWERF:FinishMask[NoCheck]");
			}
			fs->iScanTE = JANEIN_N;
		} else {
			/* all data of TEP exists */

			/* Wenn es ein Kleinmengenartikel ist -> die Kort-Abgabe-Maske
			-* aufschalten */
			if (fs->tArt.artKleinMngArt == JANEIN_J && 
				IsEmptyStrg(fs->tWeap.weapResNr) == 1 &&
				KleinMngArtDirektAufKort(NULL) == 1 &&
						scl->cur_maskid != ID_kortbuch &&
						fs->iMischCount == 0 ) {
				if (FindKortOrLp(dccw,scl, &iArtOfLp, &tPos, JANEIN_N) > 0 ) {
                   SmaBuildKortBuchWe(dccw, scl, &tPos, iArtOfLp);
                   return;
				} 
			}
		    
		    MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_AnzHE),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_AnzStueck),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_SerNr1),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_SerNr2),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_VeHeFa),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_KORTBUCH:FinishMask[NoCheck]");
			fs->iScanTE = JANEIN_J;
		}

	} else {
		fs->iMaskeJN = 0;
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTetId),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);             
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_TeTyp),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);       
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                       efname (ef_LabTe),
                                       (long )DCCEF_ATTR_INVISIBLE,
                                       NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                       efname (ef_ScTeNoMu),
                                       (long )DCCEF_ATTR_INVISIBLE,
                                       NULL);
		if (glLicenses[WKMT03] == 1 ) { 
			MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_GEWERF:FinishMask[NoCheck]");
		}
		fs->iScanTE = JANEIN_N;
	}
	StrCpy(acLineValueBuf[2], fs->acSerNr[0]);
	StrCpy(acLineValueBuf[3], fs->acSerNr[1]);
	
	CALCCOLLI(&fs->tTep.tepMngs.Mng,
	              &fs->tTep.tepMngs.VeHeFa,
	              &dHe, &dEve);

	StrCpy(acLineValueBuf[4], format("%.0lf", dHe));
	StrCpy(acLineValueBuf[6], format("%.0lf", dEve));
	
	/* TeTyp fill from Artikelstamm only wenn CalcTeTypFromMng 
	-* param is set to JANEIN_N and not MischTE */
	if (PrmGet1Parameter(NULL, P_CalcTeTypMng,
			  			PRM_CACHE, &lCalcTeTypMng) < 0) {
		lCalcTeTypMng = JANEIN_N;		  				
	}
	if (fs->iMischCount == 0 && lCalcTeTypMng == JANEIN_N) {
		if (IsEmptyStrg(fs->tArt.artTet.TetId) == 0) {
		    StrCpy(acLineValueBuf[9], fs->tArt.artTet.TetId);
		}
	} else {
		if (IsEmptyStrg(fs->tElsp.elspTet.TetId) == 0) {
		    StrCpy(acLineValueBuf[9], fs->tElsp.elspTet.TetId);
		}
	}
	
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_tebildsernr,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,	efname(ef_Title),		acLineLabelBuf[0],	-1,
		DparEfContents,	efname(ef_LabArt),		acLineLabelBuf[1],	-1,
		DparEfContents,	efname(ef_LabSerNr1),	acLineLabelBuf[2],	-1,
		DparEfContents,	efname(ef_LabSerNr2),	acLineLabelBuf[3],	-1,
		DparEfContents,	efname(ef_LabMng),		acLineLabelBuf[4],	-1,
		DparEfContents,	efname(ef_LabVeHeFa),	acLineLabelBuf[5],	-1,
		DparEfContents,	efname(ef_LabTetId),	acLineLabelBuf[6],	-1,
		DparEfContents,	efname(ef_LabTe),		acLineLabelBuf[7],	-1,
			
		DparEfContents,	efname(ef_ArtNr),		acLineValueBuf[0],	-1,
		DparEfContents,	efname(ef_ArtBez),		acLineValueBuf[1],	-1,		
		DparEfContents,	efname(ef_SerNr1),		acLineValueBuf[2],	-1,		
		DparEfContents,	efname(ef_SerNr2),		acLineValueBuf[3],	-1,		
		DparEfContents,	efname(ef_AnzHE),		acLineValueBuf[4],	-1,
		DparEfContents,	efname(ef_Info1),		acLineValueBuf[5],	-1,
        DparEfContents,	efname(ef_AnzStueck),	acLineValueBuf[6],	-1,
        DparEfContents,	efname(ef_Info2),		acLineValueBuf[7],	-1,
        DparEfContents,	efname(ef_VeHeFa),		acLineValueBuf[8],	-1,	
		DparEfContents,	efname(ef_TeTyp),		acLineValueBuf[9],	-1,		
        DparEfContents,	efname(ef_ScTeNoMu),	acLineValueBuf[10],	-1,
		
	0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}
/* ===========================================================================
 * GLOBAL (PUBLIC) Functions
 * =========================================================================*/

/* ===========================================================================
 * ALLGEIMEIN 
 * =========================================================================*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* void SmaBuildLiefBew (dccw_context *dccw, dccclient_stm *scl, char *pcBewCode)
-* DESCRIPTION Builds funk maske for Lieferanten Bewertung
-* RETURNS
-*  return void;
-*--------------------------------------------------------------------------*/
void SmaBuildLiefBew (dccw_context *dccw, dccclient_stm *scl, const char *pcBewCode)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLabelLine[4][40];
	char			acDataLine[7][40];
	double			dHe = 0, dEve = 0, dMng = 0;

	memset(acLabelLine, 0, sizeof(acLabelLine));
	memset(acDataLine, 0, sizeof(acDataLine));
	
	/***** Labels *****
	*******************/
	StrCpy(acLabelLine[0], format( "%20s",	MlMsg("   Lief-Bewertung   ")) );
	StrCpyDestLen(acLabelLine[1],	MlMsg("ArtNr"));
	StrCpyDestLen(acLabelLine[2],	MlMsg("BewCode"));
	StrCpyDestLen(acLabelLine[3],	MlMsg("Mng"));
	
	/***** Data *****
	*****************/
	StrCpy(acDataLine[0], 	fs->tArt.artAId.ArtNr);
	StrCpy(acDataLine[1],	fs->tArt.artArtBez);
	StrCpyDestLen(acDataLine[2],	pcBewCode);
	
	dMng = fs->tWeap.weapAnmMngs.Mng - fs->tWeap.weapLiMngsLls.Mng;
	
	CALCCOLLI(&dMng, &fs->tTep.tepMngs.VeHeFa,
              &dHe, &dEve);

	StrCpy(acDataLine[3], 	format("%.0lf", dHe));
	StrCpy(acDataLine[4], 	format("%.0lf", dEve));
	StrCpy(acDataLine[5],	fs->tArt.artHeEinheit);
	StrCpy(acDataLine[6],	fs->tArt.artVeEinheit);

	FUNKLOG(0);dccacVA_start_mask(dccw, scl, ID_liefbew,
		DparEfContents,	efname(ef_Title),			acLabelLine[0],	-1,
		/* Article */	
		DparEfContents,	efname(ef_LabArt),			acLabelLine[1],	-1,
		DparEfContents,	efname(ef_ArtNr),			acDataLine[0],	-1,
		DparEfContents,	efname(ef_ArtBez),			acDataLine[1],	-1,
		/* Lieferanten Bewertung */
		DparEfContents,	efname(ef_LabLiefBewCode),	acLabelLine[2],	-1,	
		DparEfContents,	efname(ef_LiefBewCode), 	acDataLine[2],	-1,	
		/* Mengen */
		DparEfContents,	efname(ef_LabMng),			acLabelLine[3],	-1,
		DparEfContents,	efname(ef_AnzHE),			acDataLine[3],	-1,
		DparEfContents,	efname(ef_Info1),			acDataLine[5],	-1,
        DparEfContents,	efname(ef_AnzStueck),		acDataLine[4],	-1,
	    DparEfContents,	efname(ef_Info2),			acDataLine[6],	-1,
		0);

	return;
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-* int SmaGetLiefBew (dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION Process Lieferanten Bewertung (writes in LBDK table) 
-*	with data from LiefBew mask
-* RETURNS
-*  return -1 error, 1 ok, 0 ok, but no database action performed;
-*--------------------------------------------------------------------------*/
int SmaGetLiefBew (dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lFlags = 0;
	LBDK			tLbdk;
	LBST			tLbst;
	char            acMsg[128+1];
	int				iRv = 0;

	switch(scl->endkey){
		case DCCAPP_VK_ENTER:
			if (GetDoubleFromRadioInput(dccw, scl,
                    efname(ef_AnzStueck), &lFlags, NULL) == 0 && 
              	GetDoubleFromRadioInput(dccw, scl,
                    efname(ef_AnzHE), &lFlags, NULL) == 0) {
			    StrCpy(acMsg,"Keine Menge eingegeben!");
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				return -1;        	

            }
			break;
		case DCCAPP_VK_ESC:
			SmaBuildScanEanWeGepl(dccw, scl);
			return (0);
		case DCCAPP_VK_LIEFBEW:
			iRv = _doLiefBewJob(dccw, scl);
			if (iRv < 0) {
			    StrCpy(acMsg,"ERROR: _doLiefBewJob failed!");
	            LogPrintf (GetFunkFac(fs), LT_ALERT, "%s", acMsg);
	            
	            StrCpy(acMsg,"Intern Fehler!");
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				return -1;
			}
			return 0;
		default:
		    StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}

	/* Write LBDK */
	memset (&tLbdk, 0, sizeof(tLbdk));
	memset (&tLbst, 0, sizeof(tLbst));
	
	tLbdk.lbdkWeaId = fs->tWeak.weakWeaId;
	tLbdk.lbdkIdx = GetNewLbdkIdx(NULL, GetFunkFac(fs), &tLbdk);
	if (tLbdk.lbdkIdx < 0) {
	    StrCpy(acMsg,"Datenbank Fehler!");
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
	tLbdk.lbdkAId = fs->tWeap.weapMId.AId;
	strncpy(tLbdk.lbdkLiefNr, fs->tWeak.weakLiefNr, LIEFNR_LEN);
	tLbdk.lbdkLiefNr[LIEFNR_LEN] = '\0';
	tLbdk.lbdkTlNr = fs->tWeak.weakTlNr;
	tLbdk.lbdkWeaPosNr = fs->tWeap.weapPosNr;
	tLbdk.lbdkTepPosNr = 0;
	strcpy (tLbdk.lbdkLbCode, dccmask_get_field (dccw, scl, 
								efname(ef_LiefBewCode), &lFlags, NULL));
								
	strcpy (tLbst.lbstLbCode, tLbdk.lbdkLbCode);							
	/* Check if code entered exists in table LBST */
	if (TExecStdSql(NULL, StdNselect, TN_LBST, &tLbst) < 0) {
		if (TSqlError(NULL) == SqlNotFound) {
		    StrCpy(acMsg,
				"Bewertungscode nicht in der Tabelle LBST vorhanden!");
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}
	}

	tLbdk.lbdkWert = (fs->tTep.tepMngs.VeHeFa * 
					GetDoubleFromRadioInput(dccw, scl,
                    			efname(ef_AnzHE), &lFlags, NULL))  + 
                 	GetDoubleFromRadioInput(dccw, scl, efname(ef_AnzStueck),
                    			&lFlags, NULL);
                    			
	tLbdk.lbdkLbStat = LBSTAT_OK;
	tLbdk.lbdkHostStatus = LBDKHOSTSTATUS_SENDEN;
	SetHist (TN_LBDK, (void *)&tLbdk, HIST_INSERT, fs->acPersNr);
	
	if (TExecStdSql (NULL, StdNinsert, TN_LBDK, &tLbdk) < 0) {
		LogPrintf (GetFunkFac(fs), LT_ALERT, "DBError: %s", TSqlErrTxt(NULL));
		StrCpy(acMsg,"Datenbank Fehler!");
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
	LogPrintf (GetFunkFac(fs), LT_DEBUG, 
		"LieferantenBew (LiefNr: %s) .. OK!", tLbdk.lbdkLiefNr);
	SmaBuildScanEanWeGepl(dccw, scl);
	return(0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/

int ErrorMsg(dccw_context *dccw,dccclient_stm *scl,int iRv)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);

	    switch (iRv) {
			    case ERROR_NONE:
		break;
    case ERROR_KPL:
        SendError(dccw,scl, "Kein Lagerplatz f�r Artikel ohne KPL im Feldstamm "
                            "angegeben!", DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
        break;
    case ERROR_STDSCHECK:
        SendError(dccw,scl, "Kein Umpackplatz f�r TE mit negativen StdCheck "
                            "im Feldstamm angegeben!", DCC_ERROR, DCC_WRITE_LOG,
                             GetFunkFac(fs));
        break;
    case ERROR_POSUN:
        SendError(dccw,scl, "WE - Position unbekannt!", DCC_ERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
        break;
    case ERROR_FES:
        SendError(dccw,scl, "SqlFehler bei Select FES!", DCC_ERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
        break;
    case ERROR_ART:
        SendError(dccw,scl, "SqlFehler bei Select ART!", DCC_ERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
		break;
    case ERROR_KORT:
        SendError(dccw,scl, "SqlFehler bei Select KORT!", DCC_ERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
		break;
    case ERROR_DBEVEHE:
		SendError(dccw,scl, "Direktbuchung nicht m�glich (EVE/HE-Faktor)!",
							DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
		break;
	case ERROR_EVEHECHECK:
		SendError(dccw,scl, "Kein Umpackplatz f�r TE mit negativen EveHeCheck!",
							DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
	}

    return (-1);
}
/* ===========================================================================
 * ALLGEIMEIN 
 * =========================================================================*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildWeMainMenu(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Function builds the mask for WE
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildWeMainMenu(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	app_msk_par		*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg("   Wareneingang    ")));

	if (fs->lUserRights2 & FS_USER_WEGEPL) {
	    StrCpyDestLen(acLineBuf[1],MlMsg(FS_USER_SWEGEPL));
	}
	if (fs->lUserRights2 & FS_USER_WEUNGEPL) {
	    StrCpyDestLen(acLineBuf[2],MlMsg(FS_USER_SWEUNGEPL));
	}
	if (fs->lUserRights2 & FS_USER_WECROSSDCK) {
	    StrCpyDestLen(acLineBuf[3],MlMsg(FS_USER_SWECROSSDCK));
	}
	if (fs->lUserRights2 & FS_USER_WERETOURE) {
	    StrCpyDestLen(acLineBuf[4],MlMsg(FS_USER_SWERETOURE));
	}
	
	/* 
	 * hide unused buttons
	 */
	if (strlen (acLineBuf[1]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key1p),
						  			   (long)DCCEF_ATTR_INVISIBLE,
					   	  			   NULL);
	}
	if (strlen (acLineBuf[2]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key2p),
						 			   (long)DCCEF_ATTR_INVISIBLE,
						  			   NULL);
	}
	if (strlen (acLineBuf[3]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key3p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[4]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key4p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[5]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key5p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[6]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key6p),
									   (long)DCCEF_ATTR_INVISIBLE,
						 			   NULL);
	}
	if (strlen (acLineBuf[7]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
				efname (ef_key7p),
				(long)DCCEF_ATTR_INVISIBLE,
				NULL);
	}	
	
	fs->eMode = DccWe;
	fs->iLastOk = JANEIN_N;
	fs->iMaskeJN = 0;
	fs->iAllInfoDisplayed = 0;

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_vmenu,
		XparLinkedDataList, MskParData,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Menu1),acLineBuf[1],-1,
		DparEfContents,efname(ef_Menu2),acLineBuf[2],-1,
		DparEfContents,efname(ef_Menu3),acLineBuf[3],-1,	
		DparEfContents,efname(ef_Menu4),acLineBuf[4],-1,	
		0);

	if (MskParData != NULL) {
		fs_release_Maskparam (MskParData);
	}

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int SmaGetWeMainInput(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acMsg[128+1];
#ifdef ORIGINAL_CROSSDOCKING
	int				iParamVal = 0;
#endif
	
	acMsg[0]='\0';

	switch(scl->endkey){
		case '1':

			if (!(fs->lUserRights2 & FS_USER_WEGEPL)) {
			    StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
				SendError(dccw,scl, acMsg, DCC_ERROR,
				   DCC_WRITE_LOG, GetFunkFac(fs));
				break;
			}
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw,scl); 
			break;

		case '2':

			if (!(fs->lUserRights2 & FS_USER_WEUNGEPL)) {
			    StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				break;
			}
			fs->eMode = DccUngeplWe;
  		  	SmaBuildAnmeldWeUngepl(dccw,scl); 
			break;
		case '3':
			SmaBuildScdAvisAus(dccw, scl);
#ifdef ORIGINAL_CROSSDOCKING
			if (PrmGet1Parameter (NULL, P_LekkSpez,
                        PRM_CACHE, &iParamVal) != PRM_OK) {
        		iParamVal = JANEIN_N;
    		}
			if (JANEIN_N == iParamVal) {
				SmaBuildCrossdck (dccw, scl);
			} else {
				/* Crossdocking specific for Lekkerland */
				SmaBuildCrossdck_lekk (dccw, scl);
			}
#endif /*ORIGINAL_CROSSDOCKING*/
			break;

		case '4':

			if (!(fs->lUserRights2 & FS_USER_WERETOURE)) {
			    StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
				SendError(dccw,scl, acMsg, DCC_ERROR,
				   DCC_WRITE_LOG, GetFunkFac(fs));
				break;
			}
			fs->eMode = DccRetoure;
			SmaBuildRetoureMainMenu(dccw,scl); 

			break;
		case DCCAPP_VK_ESC:
			fs->eMode = DccAppNone;
			BuildMainMenu(dccw,scl);
			break;
		default:
		  StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			break;
	}

	/* Es wird hier beinhart angenommen, das aller Speicher zu 
	-* diesem Zeitpunkt freigegeben wurde. Somit werden die 
	-* Pointer auf NULL gesetzt. Falls zu diesem Zeitpunkt auf eine 
	-* Speicherbereich zeigen, der nicht freigegeben wurde, ist 
	-* eine Freigabe des Speichers vergessen worden.
	-*/

	return(0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$
-*  void SmaBuildRetoureMainMenu(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Function builds the mask for Retoure 
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildRetoureMainMenu(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	app_msk_par		*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg("      Retoure       ")));

	StrCpyDestLen(acLineBuf[1],MlMsg(FS_USER_SRETOUREERSTERF));
	StrCpyDestLen(acLineBuf[2],MlMsg(FS_USER_SRETOUREVERRAEUMEN));
	
	/* 
	 * hide unused buttons

	if (strlen (acLineBuf[1]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key1p),
						  			   (long)DCCEF_ATTR_INVISIBLE,
					   	  			   NULL);
	}
	if (strlen (acLineBuf[2]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key2p),
						 			   (long)DCCEF_ATTR_INVISIBLE,
						  			   NULL);
	}
	*/

	if (strlen (acLineBuf[3]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key3p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[4]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key4p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[5]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key5p),
									   (long)DCCEF_ATTR_INVISIBLE,
									   NULL);
	}
	if (strlen (acLineBuf[6]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
									   efname (ef_key6p),
									   (long)DCCEF_ATTR_INVISIBLE,
						 			   NULL);
	}
	if (strlen (acLineBuf[7]) == 0) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
				efname (ef_key7p),
				(long)DCCEF_ATTR_INVISIBLE,
				NULL);
	}	
	
	fs->eMode = DccGeplWe;
	fs->iLastOk = JANEIN_N;
	fs->iMaskeJN = 0;
	fs->iAllInfoDisplayed = 0;

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_rmenu,
		XparLinkedDataList, MskParData,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Menu1),acLineBuf[1],-1,
		DparEfContents,efname(ef_Menu2),acLineBuf[2],-1,
		0);

	if (MskParData != NULL) {
		fs_release_Maskparam (MskParData);
	}

	return;
}

/*----------------------------------------------------------------------------
 * -* SYNOPSIS
 *  -*  $$
 *  -*  void SmaBuildRetoureMainMenu(dccw_context *dccw,dccclient_stm *scl)
 *  -* DESCRIPTION
 *  -* Look what was inputed from the Retoure Main-Menu 
 *  -* RETURNS
 *  -*  return;
 *  -*--------------------------------------------------------------------------*/

int SmaGetRetoureMainInput(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acMsg[128+1];
	// int				iParamVal = 0;
	
	acMsg[0]='\0';

	switch(scl->endkey){
		case '1':
/*
			if (!(fs->lUserRights2 & FS_USER_RETOUREERSTERF)) {
				sprintf(acMsg,"%s",DCCMSG_FALSCHETASTE);
				SendError(dccw,scl, acMsg, DCC_ERROR,
				   DCC_WRITE_LOG, GetFunkFac(fs));
				break;
			}
*/
			SmaBuildIndentifRetoure(dccw,scl); 
			break;

		case '2':
/*
			if (!(fs->lUserRights2 & FS_USER_RETOUREVERRAEUMEN)) {
				sprintf(acMsg,"%s",DCCMSG_FALSCHETASTE);
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				break;
			}
*/
  		  	SmaBuildRetoureVerraeumenAuf(NULL,GetFunkFac(fs),dccw,scl); 
			break;

		case DCCAPP_VK_ESC:
			fs->eMode = DccAppNone;
			SmaBuildWeMainMenu(dccw,scl);
			break;
		default:
		  StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			break;
	}

	/* Es wird hier beinhart angenommen, das aller Speicher zu 
	-* diesem Zeitpunkt freigegeben wurde. Somit werden die 
	-* Pointer auf NULL gesetzt. Falls zu diesem Zeitpunkt auf eine 
	-* Speicherbereich zeigen, der nicht freigegeben wurde, ist 
	-* eine Freigabe des Speichers vergessen worden.
	-*/

	return(0);
}/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildTEBilden(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildTEBilden(dccw_context *dccw,dccclient_stm *scl,
						int iEanArt, int iScanTE)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[20][40];
	char			acLineBuf2[20][40];
	int				iWert, iRv, iArtOfLp=0;
	app_msk_par  	*MskParData = NULL;
	double          dHe, dEve, dLE2;
	POS				tPos;
	long			lCalcTeTypMng = 0, lWeErfLe2 = 0;
	RetWareType		eRetWareType;
	char            acZielPosStr[POSSTRG_LEN+1];
	FES             atFes[KPL_BLOCKSIZE_WE];
	KORT            tKort, atKort[KPL_BLOCKSIZE_WE];
	int				nI = 0, iFound = 0;
	long			lOnlyMngOnFunk;
	int				iRvDb;
	
	memset(acZielPosStr, 0, sizeof(acZielPosStr));
	
	if (fs->eMode != DccUngeplWeTeBilden){

	    std::string msg;

		if(_CheckInfo(fs, msg ) == 1) {

			SendErrorNoBack(dccw,scl, msg.c_str(), DCC_ERROR,
       		DCC_NO_LOG, GetFunkFac(fs));
       		return;	
		}
	}

	fs->iEanArtWE = iEanArt;
	fs->iScanTEWE = iScanTE;

	/* Gard Art */
	if (fs->tArt.artGardKz == JANEIN_J) {

        if (IsEmptyStrg (fs->tArt.artLagO.FeldId) == JANEIN_J) {
            /*  Stammlagerort anlegen */
            iRvDb = TExecSql (NULL,
                                "UPDATE ART SET LAGO_FELDID = 'GRHR' "
                                "WHERE AID_MAND = :Mand "
                                "AND AID_ARTNR = :ArtNr "
                                "AND AID_VAR = :Var ",
                                SQLSTRING(fs->tArt.artAId.Mand),
                                SQLSTRING(fs->tArt.artAId.ArtNr),
                                SQLSTRING(fs->tArt.artAId.Var),
                                NULL);

            if (iRvDb <= 0) {
                LogPrintf(GetFunkFac(fs), LT_ALERT,
                            "Fehler beim Update Art :%s",
                            TSqlErrTxt(NULL));
            }
            /* Set Stammlagerort in Art */
            strcpy(fs->tArt.artLagO.FeldId, "GRHR");
        }

		iRv = GetTeId(NULL, fs->tTep.tepTeId, GetFunkFac(fs));

		if (iRv < 0) {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "Fehler beim TE-Id generieren !");
		} else {	 	
			/* set charge for Art */
		    StrCpy(fs->tTep.tepMId.Charge, format("%c%c%c%c%c%c",
						fs->tTep.tepTeId[12],
						fs->tTep.tepTeId[13],
						fs->tTep.tepTeId[14],
						fs->tTep.tepTeId[15],
						fs->tTep.tepTeId[16],
						fs->tTep.tepTeId[17]));
		}
	}		
	
	/* If Seriennummerplicht Artikel is set and Weap.PruefKz is not, 
	-* build mask for Seriennummer */
	if (fs->tArt.artSerNrPflWE == JANEIN_J && fs->tWeap.weapPruefKz == 0) {
		SmaBuildTEBildenSerNr(dccw, scl, iEanArt, iScanTE);
		return;
	}
	
	/* Get type of retour ware */
	if (fs->tWeak.weakRetWare == 1) {
		if (fs->tArt.artLgKz == 1) {
			eRetWareType = EMPTIES;
		} else if (fs->tWeap.weapPruefKz == 1) {
			eRetWareType = NOT_SELLABLE_OR_THM;
		} else if (fs->iMischCount == 0 &&
			(fs->tTep.tepMngs.Mng == 
				 fs->tTep.tepMngs.VeHeFa * 
				 fs->tTep.tepVps.HeTeFa)){
			eRetWareType = SELLABLE_GANZ;
		} else {
			eRetWareType = SELLABLE;
		}	
	} else {
		eRetWareType = NORMAL;
	}

	memset(acLineBuf, 0, sizeof(acLineBuf));
	memset(acLineBuf2, 0, sizeof(acLineBuf2));
	memset(&tPos, 0, sizeof(POS));

	/* Lables */
	if (fs->iMischCount == 0) {
	    StrCpy(acLineBuf[0],format("%20s",MlMsg("      TE bilden     ")));
	} else {
	    StrCpy(acLineBuf[0],format("%20s",MlMsg("      Misch-TE      ")));
	}
	sprintf(acLineBuf[1],"%s:", MlMsg("ArtNr"));
	sprintf(acLineBuf[2],"%s", fs->tArt.artArtBez);
	sprintf(acLineBuf[3],"%s:", MlMsg("Charge"));
	sprintf(acLineBuf[4],"%s:", MlMsg("VarBez"));
	sprintf(acLineBuf[5],"%s:", MlMsg("Mng"));
	sprintf(acLineBuf[6],"%s:", MlMsg("Gew"));
	sprintf(acLineBuf[7],"%s", MlMsg("TE"));
	sprintf(acLineBuf[10],"%s:", MlMsg("T"));
	sprintf(acLineBuf[12], "%s:", MlMsg("EVE/HE"));
    sprintf(acLineBuf[15], "%s", MlMsg("Charge:"));
	
	/* data */
	sprintf(acLineBuf2[1], "%s", fs->tArt.artAId.ArtNr);
	sprintf(acLineBuf2[4], "%s", fs->tArt.artVarBez);
    sprintf(acLineBuf[14], "%s", fs->tArt.artAId.Var);
	sprintf(acLineBuf2[3], "%s", fs->tTep.tepMId.Charge);
	sprintf(acLineBuf[13], "%.0f", fs->tTep.tepMngs.VeHeFa);

	if (NOT_SELLABLE_OR_THM == eRetWareType ||
		(EMPTIES == eRetWareType && 1 == fs->tArt.artLgMngVerw )) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                efname (ef_LabCharge),
                                (long )DCCEF_ATTR_INVISIBLE,
                                NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                efname (ef_Charge),
                                (long )DCCEF_ATTR_INVISIBLE,
                                NULL);   
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                efname (ef_Gew),
                                (long )DCCEF_ATTR_INVISIBLE,
                                NULL);   
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                efname (ef_LabGew),
                                (long )DCCEF_ATTR_INVISIBLE,
                                NULL);
	}

	sprintf(acLineBuf2[7], "%.3f", fs->tTep.tepMngs.Gew);

	if (PrmGet1Parameter (NULL, P_GIFillTUId,
                        PRM_CACHE, &iWert) != PRM_OK) {
        iWert = 0;
    }

	if (iWert == JANEIN_J) {
		sprintf(acLineBuf2[8], "%s", fs->tTep.tepTeId);
	} else {
		acLineBuf2[8][0]='\0';
	}

	if (iEanArt == EANART128 && fs->iMaskeJN == 0) {

		if (PrmGet1Parameter (NULL, P_EditEAN128Data,
                        PRM_CACHE, &iWert) != PRM_OK) {
			iWert = 0;
		}

		/* Alle Daten zum bilden der TE vorhanden ? */
		iRv = AllDataForTep(&fs->tTep, &fs->tArt);

		if ((iWert == 1 || iRv <= 0) && iScanTE == 0) {
			/* Edit data of Ean128 */
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTetId),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);             
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_TeTyp),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);       
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTe),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_ScTeNoMu),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
                                        
			if (glLicenses[WKMT03] == 1 ) { 
				MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_GEWERF:FinishMask[NoCheck]");
			}
			fs->iScanTE = JANEIN_N;
		} else {
			/* all data of TEP exists */

			/* Wenn es ein Kleinmengenartikel ist -> die Kort-Abgabe-Maske
			-* aufschalten */
			if (fs->tArt.artKleinMngArt == JANEIN_J && 
				IsEmptyStrg(fs->tWeap.weapResNr) == 1 &&
				KleinMngArtDirektAufKort(NULL) == 1 &&
						scl->cur_maskid != ID_kortbuch &&
						fs->iMischCount == 0 ) {
				if (FindKortOrLp(dccw,scl, &iArtOfLp, &tPos, JANEIN_N) > 0 ) {
                   SmaBuildKortBuchWe(dccw, scl, &tPos, iArtOfLp);
                   return;
				} 
			}

			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_Charge),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_VarBez),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_AnzHE),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_AnzStueck),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);

			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                            efname (ef_MHD),
                            (long )DCCEF_ATTR_INACTIVE,
                            NULL);

			MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_Gew),
                                        (long )DCCEF_ATTR_INACTIVE,
                                        NULL);
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
					efname (ef_key_onkort),
					(long )DCCEF_ATTR_INVISIBLE,
					NULL);
			
			MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_KORTBUCH:FinishMask[NoCheck]");
			fs->iScanTE = JANEIN_J;
		}

	} else {
		fs->iMaskeJN = 0;
		   MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabTetId),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);             
		   MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_TeTyp),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);       
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                       efname (ef_LabTe),
                                       (long )DCCEF_ATTR_INVISIBLE,
                                       NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                       efname (ef_ScTeNoMu),
                                       (long )DCCEF_ATTR_INVISIBLE,
                                       NULL);
		
		MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
										NULL, 0,
										"@VK_GEWERF:FinishMask[NoCheck]");
		fs->iScanTE = JANEIN_N;
	}

	if (JANEIN_N == fs->iScanTE) {
		if (NORMAL != eRetWareType || fs->tWeap.weapVerkRaum == JANEIN_J) {
			fs->tTep.tepMngs.Mng = 	fs->tWeap.weapAnmMngs.Mng - 
									fs->tWeap.weapLiMngsLls.Mng;
		}
	}
	
	CALCCOLLI(&fs->tTep.tepMngs.Mng,
              &fs->tTep.tepMngs.VeHeFa,
              &dHe, &dEve);
              
	/* Get Prm for Schlau no HE on Mask */
	if (PrmGet1Parameter (NULL, P_OnlyMngOnFunk,
    	PRM_CACHE, &lOnlyMngOnFunk) != PRM_OK) {
    	/* Error get Prm */
    	lOnlyMngOnFunk = 0;
	}

	if (lOnlyMngOnFunk == JANEIN_J) {
		dEve = fs->tTep.tepMngs.Mng;
		dHe = 0;
	}

    if (0 == dEve) {
		/* Check if it is LE erfassung */
		if (PrmGet1Parameter(NULL, P_WeErfLe2,
				  			PRM_CACHE, &lWeErfLe2) < 0) {
			lWeErfLe2 = JANEIN_N;		  				
		}
		if (lWeErfLe2 == JANEIN_J && fs->tTep.tepVps.HeL2Fa > 0) {
				fs->iL2Erf = 1; /* LE2 erfassung */
				/* Call CALCCOLLI with calculated HE and HeL2Fa */
				dLE2 = dHe;
				CALCCOLLI(&dLE2,
              		&fs->tTep.tepVps.HeL2Fa,
              		&dHe, &dEve);
		} else {
			fs->iL2Erf = 0;
		}
    }

	if (fs->eMode == DccGeplWeTeBilden) {
		if (fs->iScanTE != JANEIN_J) {
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
				efname (ef_key_charge),
				(long )DCCEF_ATTR_INVISIBLE,
				NULL);
		}		
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
				efname (ef_LabCharge),
				(long )DCCEF_ATTR_INVISIBLE,
				NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
				efname (ef_Charge),
				(long )DCCEF_ATTR_INVISIBLE,
				NULL);   

		if (fs->dSaveMngWE > 0.0 &&
			dEve <= 0.0) {
			dEve = fs->dSaveMngWE; 		
		}
	} else {
		if (fs->tArt.artChargePfl == JANEIN_N) {
			MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
										efname (ef_Charge),
										(long )DCCEF_ATTR_MUST,
										NULL);
		}								
	}

	
	sprintf(acLineBuf2[5], "%.0lf", dHe);
	if ((RNDDOUBLE(&dEve) - (int ) dEve) > 0) { 
		/* Komma zahl */
    	sprintf(acLineBuf2[6], "%.1lf", dEve);
	} else {
		/* Not Komma zahl */
		sprintf(acLineBuf2[6], "%.0lf", dEve);
	}

    if (1 == fs->iL2Erf) {
		sprintf(acLineBuf[8],"%s", fs->tArt.artLe2Einheit);
    	sprintf(acLineBuf[9],"%s", fs->tArt.artHeEinheit);
	} else {
		sprintf(acLineBuf[8],"%s", fs->tArt.artHeEinheit);
    	sprintf(acLineBuf[9],"%s", fs->tArt.artVeEinheit);
	}

	
	memset(&atKort[0], 0, sizeof(atKort));
	memset(&tKort, 0, sizeof(tKort));
	memset(&atFes[0], 0, sizeof(atFes));

	iRv = ReadKortForAId(NULL,
					 &fs->tArt.artAId,
					 "NORMAL",
					 &atKort[0],
					 &atFes[0],
					 GetFunkFac(fs),
					 KPL_BLOCKSIZE_WE,1);

	if (iRv < 0) { /* Datenbank Error */
		LogPrintf(GetFunkFac(fs), LT_ALERT,
		"Fehler beim Lesen des KORT %s/%s/%s/%s: %s ",
			fs->tTep.tepMId.AId.Mand,
			fs->tTep.tepMId.AId.ArtNr,
			fs->tTep.tepMId.AId.Var,
			fs->tTep.tepMId.MatKz, TSqlErrTxt(NULL));
	}

	if(iRv == 1) {
			/* One Kort */
			tKort = atKort[0];
	} else if (iRv > 1) {
		/* More at one Kort */
		for(nI = 0; nI < iRv; nI++) {
			if(atKort[nI].kortVerpackTyp == VERPACKTYP_HE) {
				tKort = atKort[nI];
				iFound = 1;
			}
		}
		if(iFound == 0) {
			tKort = atKort[0];
		}
	}

	if (iRv > 0) {
		/* Kort Found */
		Pos2Str(NULL, &tKort.kortKomPos, acZielPosStr);

		sprintf(acLineBuf[3], "%s", acZielPosStr);	
	
	} else {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
							efname (ef_LabSerNr2),
							(long )DCCEF_ATTR_INVISIBLE,
							NULL);
	}

	
	    /* Keine Gewichtsartikel -> keine Felder */
    if (fs->tArt.artGewKz == JANEIN_N ||
			glLicenses[WKMT03] == 0) { 
        MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_Gew),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
        MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabGew),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
    }
	
	/* TeTyp fill from Artikelstamm only wenn CalcTeTypFromMng 
	-* param is set to JANEIN_N and not MischTE */
	if (PrmGet1Parameter(NULL, P_CalcTeTypMng,
			  			PRM_CACHE, &lCalcTeTypMng) < 0) {
		lCalcTeTypMng = JANEIN_N;		  				
	}
	if (fs->iMischCount == 0 && lCalcTeTypMng == JANEIN_N) {
		if (IsEmptyStrg(fs->tArt.artTet.TetId) == 0) {
			sprintf(acLineBuf[11], "%s", fs->tArt.artTet.TetId);
		}
	} else {
		if (IsEmptyStrg(fs->tElsp.elspTet.TetId) == 0) {
			sprintf(acLineBuf[11], "%s", fs->tElsp.elspTet.TetId);
		}
	}


    /* Get Prm for Schlau no HE on Mask */
    if (lOnlyMngOnFunk == JANEIN_J) {
#ifdef _POSWALD_AMELDEMENGE
        MskParData = fs_put_Maskparam(MskParData, LparEfAttrSet,
                                            efname (ef_Info1),
                                            (long )DCCEF_ATTR_INVISIBLE,
                                            NULL);
#endif /* _POSWALD_AMELDEMENGE */
        MskParData = fs_put_Maskparam(MskParData, LparEfAttrSet,
                                            efname (ef_AnzHE),
                                            (long )DCCEF_ATTR_INVISIBLE,
                                            NULL);

		memset(acLineBuf[8], 0, sizeof(acLineBuf[8]));
	
		if ((fs->tWeap.weapAnmMngs.Mng - fs->tWeap.weapLiMngsLls.Mng) >= 0) {
	
			if ((RNDDOUBLE(&fs->tWeap.weapAnmMngs.Mng) - 
							(int) fs->tWeap.weapAnmMngs.Mng ) > 0) {
    		/* Komma zahl */
 	   			sprintf(acLineBuf[8], "%.1f", fs->tWeap.weapAnmMngs.Mng -
											  fs->tWeap.weapLiMngsLls.Mng);
			} else {
    			/* Not Komma zahl */
    			sprintf(acLineBuf[8], "%.0f", fs->tWeap.weapAnmMngs.Mng -
											  fs->tWeap.weapLiMngsLls.Mng);
			}
		} else {
			/* Negativ Mng ??? */
			sprintf(acLineBuf[8], "0");
		}

    }

    /* Keine FIFO-Pflicht -> keine Felder */
    if (fs->tArt.artFifoPfl == JANEIN_N) {
        MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_MHD),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
        MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabMHD),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);

		MskParData = fs_put_Maskparam (MskParData, DparMskKeySet,
                                NULL, 0,
                                "@VK_ENTER:FinishMask");
    } else {
		sprintf(acLineBuf2[7], "%s", "MHD");	
		if (fs->tTep.tepMHD > 100000) {
			strftime(acLineBuf2[10], 11, "%d%m%Y",
                localtime(&fs->tTep.tepMHD));


#ifdef _POSWALD			
        	MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
                                        efname (ef_AnzStueck),
                                        (long )DCCEF_ATTR_CURSOR_AT_BEGIN,
                                        NULL);

        	MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_TeTyp),
                                        (long )DCCEF_ATTR_CURSOR_AT_BEGIN,
                                        NULL);
#endif /* _POSWALD */

		} else {
			acLineBuf2[10][0]='\0';
		}
	}

	
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_tebild,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,	efname(ef_Title),	acLineBuf[0],	-1,
		DparEfContents,	efname(ef_LabArt),	acLineBuf[1],	-1,
		DparEfContents,	efname(ef_ArtBez),	acLineBuf[2],	-1,
		DparEfContents,	efname(ef_LabSerNr2),acLineBuf[3],	-1,
		DparEfContents,	efname(ef_LabVarBez),acLineBuf[4],	-1,
		DparEfContents,	efname(ef_LabMng),	acLineBuf[5],	-1,
		DparEfContents,	efname(ef_LabGew),	acLineBuf[6],	-1,
		DparEfContents,	efname(ef_LabTe),	acLineBuf[7],	-1,
		DparEfContents,	efname(ef_ArtNr),	acLineBuf2[1],	-1,
		DparEfContents, efname(ef_VarNr),	acLineBuf[14],  -1,
		DparEfContents,	efname(ef_Charge),	acLineBuf2[3],	-1,
		DparEfContents,	efname(ef_VarBez),	acLineBuf2[4],	-1,
		DparEfContents,	efname(ef_Info1),	acLineBuf[8],	-1,
        DparEfContents,	efname(ef_Info2),	acLineBuf[9],	-1,
		DparEfContents,	efname(ef_AnzHE),	acLineBuf2[5],	-1,
        DparEfContents,	efname(ef_AnzStueck),acLineBuf2[6],	-1,
		DparEfContents,	efname(ef_LabMHD),	acLineBuf2[7],	-1,
		DparEfContents,	efname(ef_MHD),		acLineBuf2[10],	-1,
		DparEfContents,	efname(ef_ScTeNoMu),acLineBuf2[8],	-1,
		DparEfContents,	efname(ef_LabTetId),acLineBuf[10],	-1,
		DparEfContents,	efname(ef_TeTyp),	acLineBuf[11],	-1,		
		DparEfContents,	efname(ef_LabVeHeFa),acLineBuf[12],	-1,		
		DparEfContents,	efname(ef_VeHeFa),	acLineBuf[13],	-1,		
		DparEfContents,	efname(ef_LabCharge),	acLineBuf[15],	-1,		
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}

/* ===========================================================================
 * UNGEPLANTER WARENEINGANG 
 * =========================================================================*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-*  SmaGetWeMainInput(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Look what was inputed from the WE Main-Menu
-* RETURNS
-*  Radio returns 0
-*--------------------------------------------------------------------------*/
void SmaBuildAnmeldWeUngepl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char            acLineBuf[8][40];
	char			acPosStrg[POSSTRG_LEN+1];
	int				iWert;
	app_msk_par  	*MskParData = NULL;

	memset(&fs->tArt, 0, sizeof(ART));
	memset(&fs->tTep, 0, sizeof(TEP));
	memset(&fs->tElsp, 0, sizeof(ELSP));
       
    memset(acLineBuf, 0, sizeof(acLineBuf));
 	sprintf(acLineBuf[0],"%20s",MlMsg("   ungeplanter WE   "));

	sprintf(acLineBuf[1],"%s:", MlMsg("Man"));
	sprintf(acLineBuf[2],"%s:", MlMsg("MatKz"));
	sprintf(acLineBuf[3],"%s:", MlMsg("BuSchl"));
	sprintf(acLineBuf[4],"%s", MlMsg("Pos"));

	Pos2Str(NULL, &fs->tAktposPOS,  acPosStrg);

	if (PrmGet1Parameter (NULL, P_SingleClient,
                       PRM_CACHE, &iWert) != PRM_OK) {
		iWert = 0;
	}

	if (iWert == 1) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_Mand),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabMand),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
                                        efname (ef_Mand),
                                        (long )DCCEF_ATTR_MUST,
                                        NULL);
	}

	if (PrmGet1Parameter (NULL, P_SingleMatKz,
					PRM_CACHE, &iWert) != PRM_OK) {
        iWert = 0;
    }

	if (iWert == 1) {
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_MatKz),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrSet,
                                        efname (ef_LabMatKz),
                                        (long )DCCEF_ATTR_INVISIBLE,
                                        NULL);
		MskParData = fs_put_Maskparam (MskParData, LparEfAttrClr,
                                        efname (ef_MatKz),
                                        (long )DCCEF_ATTR_MUST,
                                        NULL);
	}
	fs->eMode = DccUngeplWe;
	fs->iAnzTE = 0;

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_ungewean,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
        DparEfContents,efname(ef_Title),acLineBuf[0],-1,
        DparEfContents,efname(ef_LabMand),acLineBuf[1],-1,
        DparEfContents,efname(ef_Mand),fs->acMand,-1,
        DparEfContents,efname(ef_LabMatKz),acLineBuf[2],-1,
        DparEfContents,efname(ef_MatKz),fs->acMatKz,-1,
        DparEfContents,efname(ef_LabBuSchl),acLineBuf[3],-1,
        DparEfContents,efname(ef_BuSchl),fs->acBuSchl,-1,
        DparEfContents,efname(ef_LabPos),acLineBuf[4],-1,
        DparEfContents,efname(ef_Scanpos),acPosStrg,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int SmaGetUngeplWeAnm(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lFlags = 0;
	int				isPos=0, iRv=0, iWert=0;
	char			acMsg[128+1];
	char			acPosStr[POSSTRG_LEN+1];
	POS				tPos;

	acMsg[0]='\0';

	switch(scl->endkey){

		case DCCAPP_VK_ENTER:
			/* Fallthrough */
		case DCCKEY_SCAN_N:
		case DCCAPP_VK_SCAN:
			break;
 
		case DCCAPP_VK_ESC:
#ifdef UHU
			if (DeleteAllOfDummyNr(NULL, GetFunkFac(fs), fs->lDummyNr) < 0) {
    			sprintf(acMsg, "%s", DCCMSG_ERRORDBREAD);
    			SendError(dccw,scl, acMsg, DCC_ERROR,
            		DCC_WRITE_LOG, GetFunkFac(fs));
    			return(-1);
			}
#endif /* UHU */
			fs->iMischCount = 0;
			SmaBuildWeMainMenu(dccw,scl);
			return 0;
		default:
			sprintf(acMsg,"%s",DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}

	strcpy(fs->acMand, dccmask_get_field(dccw, scl,
                   efname(ef_Mand), &lFlags, NULL));

	strcpy(fs->acMatKz, dccmask_get_field(dccw, scl,
					efname(ef_MatKz), &lFlags, NULL));

	strcpy(fs->acBuSchl, dccmask_get_field(dccw, scl,
					efname(ef_BuSchl), &lFlags, NULL));

	strcpy(acPosStr, dccmask_get_field(dccw, scl,
					efname(ef_Scanpos), &lFlags, NULL));
	 if (IsEmptyStrg(acPosStr) != 0) {
    	 SendError(dccw, scl, MlMsg("Position angeben!"), DCC_ERROR,
        	 DCC_NO_LOG, GetFunkFac(fs));
    	 return(0);
	 }

	Pos_ext2int(NULL, acPosStr, acPosStr);

	isPos = CheckIfPos(NULL, acPosStr, NULL, &tPos);

	/* Gescannte Daten sind eine Position */
	if (isPos > 0 ) {
		DccSetAktpos(fs, &tPos);
	} else {
		/* vielleich noch verbessern */
		sprintf(acMsg,"%s",
                (MlMsg("Kein Lagerplatz gefunden!")));
        SendErrorNoBack(dccw, scl, acMsg,
                        DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
        return (0);
	}

	if (PrmGet1Parameter (NULL, P_SingleClient,
                       PRM_CACHE, &iWert) != PRM_OK) {
		iWert = 0;
	}

	if (iWert == 1) {
		memset(fs->acMand, '0', sizeof(fs->acMand));
		fs->acMand[MAND_LEN]='\0';
	} else {
	 	if (IsEmptyStrg(fs->acMand) != 0) {
    	     SendError(dccw, scl, MlMsg("Mandant angeben!"), DCC_ERROR,
        	     DCC_NO_LOG, GetFunkFac(fs));
       	  return(0);
   	  	}
	}

	if (PrmGet1Parameter (NULL, P_SingleMatKz,
					PRM_CACHE, &iWert) != PRM_OK) {
        iWert = 0;
    }

	if (iWert == 1) {
		strcpy(fs->acMatKz, DEF_MATKZ);
	}

	/* �berprr�ung ob g�ltiges MatKz ??? */
	if (IsEmptyStrg(fs->acMatKz) == 0) {
		iRv = ReadMKz(NULL, GetFunkFac(fs), fs->acMand, fs->acMatKz, NULL);
		if (iRv <= 0) {
			sprintf(acMsg,"%s", MlMsg("Unbekanntes Materialkennzeichen!"));
			SendErrorNoBack(dccw,scl, acMsg, DCC_ERROR,
					DCC_NO_LOG, GetFunkFac(fs));
			return (0);
		}
	} else {
		sprintf(acMsg,"%s", MlMsg("Materialkennzeichen eingeben!"));
		SendErrorNoBack(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
		return (0);
	}

	/* Buchungsschl�ssel-Pflicht ??? */

	/* Ueberpr�fung ob g�ltige BuSchl */
	if (IsEmptyStrg(fs->acBuSchl) == 0) {
		iRv = CheckBuSchlExists(NULL, GetFunkFac(fs), fs->acBuSchl);
		if (iRv <= 0) {
			/* ung�ltiger Buchungsschl�ssel */
			sprintf(acMsg,"%s", MlMsg("Unbekannter Buchungsschl�ssel!"));
			SendErrorNoBack(dccw,scl, acMsg, DCC_ERROR,
					DCC_NO_LOG, GetFunkFac(fs));
			return (0);
		}
	}


	fs->eMode = DccUngeplWeEan;

	/* Schnellerfassung noch nicht moeglich */
	fs->iLastOk = JANEIN_N;
	fs->iMischCount = 0;

	SmaBuildScanEanWeUngepl(dccw, scl);

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildScanEanWeUngepl(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Function builds the mask for scanning EAN
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildScanEanWeUngepl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	char            acPosStr[POSSTRG_LEN+1];
	char			acMulti[256];
	app_msk_par  	*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));
	memset(acMulti, 0, sizeof(acMulti));
	memset(fs->acSerNr, 0, sizeof(fs->acSerNr));
	
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 0;

	if (fs->iMischCount == 0 || fs->iGebucht == JANEIN_J) { 
		/* Keine Misch-TE erfasst */
		Pos2Str(NULL,&fs->tAktposPOS,acPosStr);

		sprintf(acLineBuf[0],"%20s",MlMsg("   ungeplanter WE   "));
	
		sprintf(acMulti, "%s: %02d\n%s: %s", MlMsg("Anzahl TEs"), fs->iAnzTE,
											 MlMsg("Position"), acPosStr);
	} else {
		sprintf(acLineBuf[0],"%20s",MlMsg("    Misch-TE     "));
        sprintf(acMulti,"%s: %d",MlMsg("Anzahl Pos"), fs->iMischCount);
	}

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scanean,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Msgline),acMulti,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int SmaGetEanWeUngpl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lFlags = 0, lAktCount=0, lGesCount=0;
	int				iRv=0, iEanArt;
	TEP				tTep;
	ART				tArt;
	char			acMsg[128+1];
	char			acScanCode1[SCANCODE+1], acScanCode2[SCANCODE+1];
	
	acMsg[0]='\0';
	acScanCode1[0]='\0';
	acScanCode2[0]='\0';

	switch(scl->endkey){
		case DCCAPP_VK_LASTTE:
			if (fs->iLastOk == JANEIN_J) {
				fs->eMode = DccUngeplWeTeBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildTEBilden(dccw,scl, EANART128, JANEIN_J);
			} else {
				sprintf(acMsg,MlMsg("Schnell-Erfassung nicht m�glich"));
                SendError(dccw,scl, acMsg, DCC_ERROR,
                    DCC_NO_LOG, GetFunkFac(fs));
			}
			return (0);
		case DCCAPP_VK_ENTER:
			/* Fallthrough */
		case DCCKEY_SCAN_N:
		case DCCAPP_VK_SCAN:
			break;
 
		case DCCAPP_VK_ESC:
			if (fs->iMischCount != 0 && fs->iGebucht == JANEIN_N) {
				if (GetTepFromDummy(NULL, GetFunkFac(fs),
                    				fs->lDummyNr, fs->iMischCount,
                    				&fs->tTep) <= 0) {
				
    				sprintf(acMsg, "%s", DCCMSG_ERRORDBREAD);
    				SendError(dccw,scl, acMsg, DCC_SQLERROR,
        				DCC_WRITE_LOG, GetFunkFac(fs));
    				return -1;
				}
				memcpy(&fs->tArt.artAId, &fs->tTep.tepMId.AId, sizeof(AID));
				if (TExecStdSql(NULL, StdNselect, TN_ART, &fs->tArt) <= 0)  {

    				sprintf(acMsg, "%s", DCCMSG_ERRORDBREAD);
    				SendError(dccw,scl, acMsg, DCC_SQLERROR,
        				DCC_WRITE_LOG, GetFunkFac(fs));
    				return -1;

				}

				fs->iMischCount --;

                if (DeleteLastOfDummyNr(NULL, GetFunkFac(fs),
                                fs->lDummyNr) < 0) {

                    sprintf(acMsg, "%s", DCCMSG_ERRORDBREAD);
                    SendError(dccw,scl, acMsg, DCC_SQLERROR,
                        DCC_WRITE_LOG, GetFunkFac(fs));
                    return -1;
                }

		 		fs->eMode = DccUngeplWeTeBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildTEBilden(dccw, scl, EANART128, 1);	
				return 0;
			} else {

				if (DeleteAllOfDummyNr(NULL, GetFunkFac(fs), fs->lDummyNr) < 0) {
    				sprintf(acMsg, "%s", DCCMSG_ERRORDBREAD);
    				SendError(dccw,scl, acMsg, DCC_ERROR,
            			DCC_WRITE_LOG, GetFunkFac(fs));
    				return(-1);
				}
				fs->iMischCount = 0;

				fs->eMode = DccUngeplWe;
				SmaBuildAnmeldWeUngepl(dccw,scl);
				return 0;
			}
		default:
			sprintf(acMsg,"%s",DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}

	if (fs->iGebucht == JANEIN_J) {
		fs->iGebucht = JANEIN_N;
		fs->iMischCount = 0;
	}

	/* Lesen der Eingegebenen Felder */
	strcpy(acScanCode1, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode1), &lFlags, NULL));

	strcpy(acScanCode2, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode2), &lFlags, NULL));

	/* check if Barcode is scanned */
	if (IsEmptyStrg(acScanCode1)!= 0 && IsEmptyStrg(acScanCode2)!= 0) {
		sprintf(acMsg, "%s", MlMsg("Bitte EAN-Code eingegeben!"));
        SendError(dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
        return (-1);
	}

	/* Decode EAN */
	fs->acEan13Code[0] = '\0';
	memset(&fs->tTep, 0, sizeof(TEP));
	iRv = DecodeEan(NULL, GetFunkFac(fs), acScanCode1, acScanCode2, 
					&tTep, &tArt, fs->acEan13Code, &iEanArt);

	LogPrintf(GetFunkFac(fs), LT_ALERT, "TeId nach DecodeEan: %s", tTep.tepTeId);	

	if (iRv == 1) {
		memcpy(&fs->tTep, &tTep, sizeof(TEP));
		memcpy(&fs->tArt, &tArt, sizeof(ART));
		memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
        memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        fs->tTep.tepMngs.VeHeFa = fs->tArt.artMngs.VeHeFa;
		fs->tTep.tepMngs.GewVe = fs->tArt.artMngs.GewVe;
		fs->eMode = DccUngeplWeTeBilden;
		memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
		memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
		fs->dSaveMngWE = 0.0;
		SmaBuildTEBilden(dccw,scl,iEanArt, 0);
	} else if (iRv < 0) {
		/* Da ist was schief gegangen */
		sprintf(acMsg, "%s", MlMsg("Fehler beim Auswerten des EAN-Codes!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
		return (-1);
	} else if (iRv == 0) {
		/* Keinen Artikel gefunden  */
		sprintf(acMsg, "%s", MlMsg("Keine Artikel f�r EAN-Code gefunden!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
		return -1;

		/* 
		 * 	fs->eMode = DccUngeplWeEan;
		 *  SmaBuildScanEanWeUngepl(dccw, scl);
		 *  */
	} else {
		/* Mehrere gefunden */
		memcpy(&fs->tTep, &tTep, sizeof(TEP));
		memcpy(&fs->tArt, &tArt, sizeof(ART));
		memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
        memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        fs->tTep.tepMngs.VeHeFa = fs->tArt.artMngs.VeHeFa;
		fs->tTep.tepMngs.GewVe = fs->tArt.artMngs.GewVe;
		if (SearchArtforEan(GetFunkFac(fs), fs, &lAktCount, 
							&lGesCount, fs->acEan13Code, 
							NULL, PAGE_NONE,JANEIN_N)	<= 0) {
			sprintf(acMsg, "%s", 
				MlMsg("Fehler beim Lesen des Artikels!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
		fs->eMode = DccUngeplWeEan;
		SmaBuildListArtWeUngepl(dccw,scl, lAktCount, lGesCount);
	}

	return(0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-* DESCRIPTION
-* RETURNS
-*  Radio returns 0
-*--------------------------------------------------------------------------*/
int SmaGetTeBildenWeUngepl(dccw_context *dccw,dccclient_stm *scl)
{
    FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
    long            lFlags = 0;
	char            acMsg[128+1];
	int 			iRv, iArtOfLp = 0, iSendError=JANEIN_N;
	POS				tPos;
	double          dGewTolProz, dGewTolSoll, dGewTolIst, dGewTolMax;
	double          dMaxArtWeightFa, dSollGew;
	char            acWeDefSpKz[SPKZ_LEN+1];
	char			acRestlaufMatKzWE[MATKZ_LEN+1] = {0};
	
	memset(&tPos, 0, sizeof(POS));

	strcpy(fs->tTep.tepMId.MatKz, fs->acMatKz);

	switch(scl->endkey){
		case DCCAPP_VK_KORTBUCH:
			if (fs->iMischCount == 1) {
            	sprintf(acMsg,"%s",
					MlMsg("Misch-TE kann nicht auf KPL gebucht werden"));
            	SendError(dccw,scl, acMsg, DCC_ERROR,
                			DCC_WRITE_LOG, GetFunkFac(fs));
           		return -1;
			} else {
				memset(&tPos, 0, sizeof(POS));
            	iRv = FindKortOrLp(dccw,scl, &iArtOfLp, &tPos, JANEIN_J);
            	if (iRv <= 0) { /* Datenbank Fehler */
                	/* Meldung wird in FindKortOrLp ausgespuckt */
                	return (iRv);
				}
				SmaBuildKortBuchWe(dccw, scl, &tPos, iArtOfLp);
				return 1;
			} 

		case DCCAPP_VK_ENTER:
            /* Fallthrough */
        case DCCKEY_SCAN_N:
        case DCCAPP_VK_SCAN:
            break;

		case DCCAPP_VK_ESC:
			if (fs->iScanTE == JANEIN_J) {
				fs->eMode = DccUngeplWeTeBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildTEBilden(dccw, scl, EANART13, 0);	
			} else {
				fs->eMode = DccUngeplWeEan;
				fs->iGebucht = JANEIN_N;
				SmaBuildScanEanWeUngepl(dccw,scl);
			}
			return 0;
		case DCCAPP_VK_LIEFBEW:
			sprintf(acMsg,"%s",
				MlMsg("Lieferantenbewertung nur f�r geplanter WE erlaubt!"));
        	SendError(dccw,scl, acMsg, DCC_ERROR,
            			DCC_WRITE_LOG, GetFunkFac(fs));
       		return -1;	
		case DCCAPP_VK_GEWERF:
			/* fallthrough */
			/* We read all edit fields and build the next mask */
		case DCCAPP_VK_MISCHTE:
			/* fallthrough */
            /* We generate TEP as normal, but do not write it,
                instead we put it in the buffer and increment
				the buffer-count */
			break;
        default:
            sprintf(acMsg,"%s",DCCMSG_FALSCHETASTE);
            SendError(dccw,scl, acMsg, DCC_ERROR,
                DCC_WRITE_LOG, GetFunkFac(fs));
           return -1;
    }

	if (fs->iScanTE == JANEIN_N) {
		if (scl->cur_maskid != ID_tebildsernr) {
			strcpy(fs->tTep.tepMId.Charge, dccmask_get_field(dccw, scl,
						efname(ef_Charge), &lFlags, NULL));
			if (fs->tTep.tepFifoDatum < 0) {
				fs->tTep.tepFifoDatum = 0;
			}
		} else {
			/* Seriennummer-Artikel kann nicht Chargen/FIFO-pflichtig sein
			-*/
			memset(fs->tTep.tepMId.Charge, 0, sizeof(fs->tTep.tepMId.Charge));
			fs->tTep.tepFifoDatum = 0;
		}
        
        if (fs->iL2Erf == 1) { /* Umverpackung */
        	fs->tTep.tepMngs.Mng = GetDoubleFromRadioInput(dccw, scl,
             						efname(ef_AnzStueck), &lFlags, NULL) *
             						fs->tTep.tepMngs.VeHeFa;
        	
        	fs->tTep.tepMngs.Mng += GetDoubleFromRadioInput(dccw, scl,
          							efname(ef_AnzHE), &lFlags, NULL) * 
          							fs->tTep.tepVps.HeL2Fa *
          							fs->tTep.tepMngs.VeHeFa;
        	
        } else {
			fs->tTep.tepMngs.Mng = GetDoubleFromRadioInput(dccw, scl,
	                    efname(ef_AnzStueck), &lFlags, NULL);
	        fs->tTep.tepMngs.Mng += GetDoubleFromRadioInput(dccw, scl,
	                    efname(ef_AnzHE), &lFlags, NULL) * 
	                    fs->tTep.tepMngs.VeHeFa;
        }
        if (scl->cur_maskid != ID_tebildsernr) {
			fs->tTep.tepMngs.Gew = GetDoubleFromRadioInput(dccw, scl,
                    efname(ef_Gew), &lFlags, NULL);
		} else {
			fs->tTep.tepMngs.Gew = 0;
		}
	} else {
		strcpy(fs->tTep.tepTeId, dccmask_get_field(dccw, scl,
                    efname(ef_ScTeNoMu), &lFlags, NULL));

		if (scl->endkey != DCCAPP_VK_MISCHTE) {
			if (IsEmptyStrg(fs->tTep.tepTeId) != 0) {
               	sprintf(acMsg,"%s",MlMsg("Bitte TE-Id eingeben"));
               	SendError(dccw,scl, acMsg, DCC_ERROR,
                   	DCC_NO_LOG, GetFunkFac(fs));
               	return(0);
           	}
            if (CheckTeIdLenght(NULL, GetFunkFac(fs), fs->tTep.tepTeId) < 0) {
                sprintf (acMsg, "%s",
                        MlMsg ("TE-Id-L�nge nicht korrekt!"));
                SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
                return (0);
            }
		}
	}
	/* Process seriennummer artikel */
	if (fs->tArt.artSerNrPflWE == JANEIN_J && fs->tWeap.weapPruefKz == 0) {
		iRv = _checkSerNr(dccw, scl);
		if (iRv < 0) {
			return -1;
		}
		if (fs->iScanTE == JANEIN_J && scl->endkey != DCCAPP_VK_MISCHTE) {
			return 0;
		}
	}
	/* Wenn eine Gewichtserfassung erw�nscht ist, so ist
	-* wird hier die neue Maske gebaut und abgebrochen
	-*/
	if (scl->endkey == DCCAPP_VK_GEWERF) {
		fs->tTep.tepMngs.Gew = 0;
		fs->iAnzGew = 0;
		SmaBuildSumGew(dccw,scl);
		return 0;
	}


	iRv = AllDataForTep(&fs->tTep, &fs->tArt);
	if (iRv <= 0) {	
		sprintf (acMsg, "%s",
				MlMsg ("Nicht alle notwendigen Daten angegeben!"));
		SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
		return (0);
	}

	memset(acMsg, 0, sizeof(acMsg));

	/* Toleranzen-Check */
	if (fs->iScanTE == JANEIN_N) {
        if (RNDDOUBLE(&fs->tTep.tepMngs.Gew) == 0) {
            fs->lGewGanzTeKz = 0;
            CalcTEPWeight(&fs->tArt, &fs->tTep.tepMngs);
        } else {
            fs->lGewGanzTeKz = 1;
        }
		if (fs->iMischCount > 0) {
            fs->lGewGanzTeKz = 0;
        }

		iRv = CheckTEPWeight(NULL, GetFunkFac(fs), &fs->tTep,
                    	&fs->tTep.tepMngs, &fs->tArt,
                    	&dGewTolIst, &dGewTolSoll,
                    	&dGewTolMax, &dGewTolProz,
                    	&dMaxArtWeightFa, &dSollGew);
		switch(iRv) {
		case 0:  /* Alles Ok */
    		/* FALLTHROUGH */
		case 1:
    		break;
		case 2: /* Warnung */
			StrCpy (acMsg, format( "%s%s", acMsg,
					format( MlMsg ( "Das eingebene Gewicht �berschreitet "
							"die Toleranzgrenze (%g %%) dieses "
							"Artikels!"), 
					dGewTolProz)) );
            iSendError = JANEIN_J;
            break;
		case 3: /* Abbruch */
			StrCpy( acMsg, format( "%s%s", acMsg, format(
					MlMsg ( "Das eingebene Gewicht �berschreitet "
							"die maximale Toleranzgrenze (%g %%) dieses "
							"Artikels!"), 
					dGewTolProz * dMaxArtWeightFa)));
            fs->iMaskeJN = 1;
            iSendError =  JANEIN_J;
            break;
		default: /* Error */
			sprintf(acMsg, "%s", GetErrmsg1());	
			SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
			return (-1);
		}

		/* Restlaufzeit-Check */
		memset(acWeDefSpKz, 0, sizeof(acWeDefSpKz));
		iRv = IsFifoOk(NULL, GetFunkFac(fs), &fs->tTep, &fs->tArt, 
						UNGEPLANT, acWeDefSpKz, acRestlaufMatKzWE);

		switch(iRv) {
		case 0:  /* Warnung RLZ ueberschritten */
			StrCpy(acMsg, format("%s%s", acMsg,
						MlMsg ( "TE-Sperre, weil Resthaltbarkeit "
							"nicht erf�llt!")));
			iSendError = JANEIN_J;
			break;
		case 1: /* RLZ ist OK */
    		break;
		default: /* Error */
			StrCpyDestLen(acMsg, GetErrmsg1());
			SendError(dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
			return (-1);
		}
        if (iSendError == JANEIN_J) {
            SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
            return (0);
        }
	}
	
	if (fs->iScanTE == JANEIN_J) {

	    const char *pcTetId = dccmask_get_field(dccw, scl, efname(ef_TeTyp), &lFlags, NULL);
	    char acTetId[TETID_LEN+1] = {0};

	    if( pcTetId != NULL ) {
	        StrCpyDestLen( acTetId, pcTetId );
	    }

		if (_getTetId(dccw, scl, acTetId, fs->iMischCount) < 0){
			return 0;
		}
		if (scl->endkey != DCCAPP_VK_MISCHTE) {
			/* TE anlegen */
			if (IsEmptyStrg(fs->tTep.tepTeId) != 0) {
				StrCpyDestLen (acMsg, MlMsg ("Bitte TE-Id scannen!"));
            		SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
				return -1;
			}
            if (CheckTeIdLenght(NULL, GetFunkFac(fs), fs->tTep.tepTeId) < 0) {
                StrCpyDestLen (acMsg, MlMsg ("TE-Id-L�nge nicht korrekt!"));
                SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
                return -1;
            }
			
			iRv = UngeplWeBuchen (NULL, GetFunkFac(fs), fs, 
							NULL, JANEIN_N);
			if (iRv >= 0 ) {
				fs->iLastOk = JANEIN_J;
				fs->eMode = DccUngeplWeEan;
				fs->iAnzTE ++;
				fs->iGebucht = JANEIN_J;
				SmaBuildScanEanWeUngepl(dccw,scl);
			} else {
				if (iRv == ERROR_OLDERGOODS) {
					StrCpyDestLen (acMsg, MlMsg ("Es gibt in der Reserve Ware mit �lterem "
								   "Fifo-Datum!"));
					SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
										GetFunkFac(fs));
					return (-1);
				}
				if (iRv != -1){
					ErrorMsg(dccw, scl, iRv);
				} else {
					StrCpyDestLen (acMsg, MlMsg ("Fehler beim Buchen des WE-Avises!"));
					SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                    GetFunkFac(fs));
				}
			}
			if (iRv >= 0 ) {
				/* Reset MischFlag
				-*/
				iRv = ResetMischFlag(dccw, scl, GetFunkFac(fs));
			}
			return (iRv);	
		} else {
			/* Misch-TE anlegen */

			if (fs->iMischCount == 0) {
    			if ((fs->lDummyNr = GetDummyNr(NULL, GetFunkFac(fs))) < 0) {
        			StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
        			SendError(dccw,scl, acMsg, DCC_SQLERROR,
                			DCC_WRITE_LOG, GetFunkFac(fs));
        			return -1;
    			}
			}
			if (InsertNewTepIntoDummy(NULL, GetFunkFac(fs),
                          			&fs->tTep, fs->lDummyNr) < 0) {

			    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
    			SendError(dccw,scl, acMsg, DCC_SQLERROR,
            			DCC_WRITE_LOG, GetFunkFac(fs));
    			return -1;
			}
	
			fs->iMischCount++;
			fs->iGebucht = JANEIN_N;
			fs->eMode = DccUngeplWeEan;
			SmaBuildScanEanWeUngepl(dccw,scl);
			return 0;	
		}
	} else {
		 fs->eMode = DccUngeplWeTeBilden;
		memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
		memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
		fs->dSaveMngWE = 0.0;
		SmaBuildTEBilden(dccw, scl, EANART128, 1);	
	}
	return 0;
}

/* ===========================================================================
 * GEPLANTER WARENEINGANG 
 * =========================================================================*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildWeMainMenu(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Function builds the mask for 'indentifizieren' of WE
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildIndentifWeGepl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	char			acMulti[256];
	app_msk_par  	*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg("    geplanter WE    ")));

	StrCpyDestLen(acMulti, MlMsg("Position, EAN\n"
								 "oder EAN128\n"
								 "einscannen:"));

#ifdef _POSWALD_NUR_ZWEI_ZEILEN			
	if (fs->eMode == DccGeplWe) {
		MskParData = fs_put_Maskparam(MskParData, LparEfAttrClr,
				efname (ef_ScanCode3),
				(long )DCCEF_ATTR_INVISIBLE,
				NULL);
		MskParData = fs_put_Maskparam(MskParData, LparEfAttrClr,
				efname (ef_ScanCode3),
				(long )DCCEF_ATTR_INACTIVE,
				NULL);
		MskParData = fs_put_Maskparam(MskParData, LparEfAttrClr,
				efname (ef_ScanCode2),
				(long )DCCEF_FINISH_MASK,
				NULL);
		MskParData = fs_put_Maskparam(MskParData, LparEfAttrSet,
				efname (ef_ScanCode3),
				(long )DCCEF_FINISH_MASK,
				NULL);
	}
#endif /* _POSWALD_NUR_ZWEI_ZEILEN */			
								 
	fs->iAnzTE = 0;
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 0;
	fs->iLastOk = JANEIN_N;
	
	if (fs->eMode != DccGeplWeEan) {
		memset(&fs->tWeak, 0, sizeof(WEAK));
	}

    memset(&fs->tWeap, 0, sizeof(WEAP));
    memset(&fs->tTep, 0, sizeof(TEP));
    memset(&fs->tArt, 0, sizeof(ART));
    memset(&fs->tElsp, 0, sizeof(ELSP));
    
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scanean,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Msgline),acMulti,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}
/*----------------------------------------------------------------------------
* -* SYNOPSIS
* -*  $$
* -*  void SmaBuildWeMainMenu(dccw_context *dccw,dccclient_stm *scl)
* -* DESCRIPTION
* -*  Function builds the mask for 'indentifizieren' of WE
* -* RETURNS
* -*  return;
* -*--------------------------------------------------------------------------*/

void SmaBuildIndentifRetoure(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	char			acMulti[256];
	app_msk_par  	*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg("      Retoure       ")));

	StrCpyDestLen(acMulti, MlMsg("Position, EAN\n"
								 "oder EAN128\n"
								 "einscannen:"));

								 
	fs->iAnzTE = 0;
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 0;
	fs->iLastOk = JANEIN_N;
	
	if (fs->eMode != DccRetoure) {
		memset(&fs->tWeak, 0, sizeof(WEAK));
	}

	fs->eMode = DccRetoure;

    memset(&fs->tWeap, 0, sizeof(WEAP));
    memset(&fs->tTep, 0, sizeof(TEP));
    memset(&fs->tArt, 0, sizeof(ART));
    memset(&fs->tElsp, 0, sizeof(ELSP));
    
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scanean,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Msgline),acMulti,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	$$
-*  void SmaBuildScanEanWeGepl(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Function builds the mask for scanning EAN
-* RETURNS
-*  return;
-*--------------------------------------------------------------------------*/
void SmaBuildScanEanWeGepl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	char            acPosStr[POSSTRG_LEN+1];
	char			acMulti[256];
	app_msk_par  	*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));
	memset(acMulti, 0, sizeof(acMulti));
	memset(fs->acSerNr, 0, sizeof(fs->acSerNr));
	memset(&fs->tElsp, 0, sizeof(ELSP));
	
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 0;

	fs->eMode = DccGeplWeEan;

	if (fs->iMischCount == 0 || fs->iGebucht == JANEIN_J) {
        /* Keine Misch-TE erfasst */
		Pos2Str(NULL,&fs->tAktposPOS,acPosStr);

		StrCpy(acLineBuf[0],format("%20s",MlMsg("    geplanter WE    ")));

		StrCpy(acMulti, format("%s: %02d\n%s: %s", MlMsg("Anzahl TEs"), fs->iAnzTE,
											 MlMsg("Position"), acPosStr));
	} else {
		StrCpy(acLineBuf[0],format("%20s",MlMsg("    Misch-TE     ")));
		StrCpy(acMulti, format("%s: %d", MlMsg("Anzahl Pos"), fs->iMischCount));
	}

	MskParData = fs_put_Maskparam(MskParData, LparEfAttrClr,
			efname (ef_key_lastte),
			(long )DCCEF_ATTR_INVISIBLE,
			NULL);
	
	StrCpy(acLineBuf[1],format("%s: %s", MlMsg("WEA-Nr"), fs->tWeak.weakWeaId.WeaNr));
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scanean,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Info1),acLineBuf[1],-1,	
		DparEfContents,efname(ef_Msgline),acMulti,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}
 /*----------------------------------------------------------------------------
  *  -* SYNOPSIS
  *   -*  $$
  *    -*  void SmaBuildScanEanWeGepl(dccw_context *dccw,dccclient_stm *scl)
  *     -* DESCRIPTION
  *      -*  Function builds the mask for scanning EAN
  *       -* RETURNS
  *        -*  return;
  *         -*--------------------------------------------------------------------------*/
void SmaBuildScanEanRetoure(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	char            acPosStr[POSSTRG_LEN+1];
	char			acMulti[256];
	app_msk_par  	*MskParData = NULL;

	memset(acLineBuf, 0, sizeof(acLineBuf));
	memset(acMulti, 0, sizeof(acMulti));
	memset(fs->acSerNr, 0, sizeof(fs->acSerNr));
	memset(&fs->tElsp, 0, sizeof(ELSP));
	
	fs->iShowInfo = 0;
	fs->iAllInfoDisplayed = 0;

	fs->eMode = DccRetoureEan;

	if (fs->iMischCount == 0 || fs->iGebucht == JANEIN_J) {
        /* Keine Misch-TE erfasst */
		Pos2Str(NULL,&fs->tAktposPOS,acPosStr);

		StrCpy(acLineBuf[0],format("%20s",MlMsg("      Retoure       ")));

		StrCpy(acMulti, format( "%s: %02d\n%s: %s", MlMsg("Anzahl TEs"), fs->iAnzTE,
											 MlMsg("Position"), acPosStr));
	} else {
	    StrCpy(acLineBuf[0],format("%20s",MlMsg("    Misch-TE     ")));
	    StrCpy(acMulti, format("%s: %d", MlMsg("Anzahl Pos"), fs->iMischCount));
	}

	MskParData = fs_put_Maskparam(MskParData, LparEfAttrClr,
			efname (ef_key_lastte),
			(long )DCCEF_ATTR_INVISIBLE,
			NULL);
	
	StrCpy(acLineBuf[1],format("%s: %s", MlMsg("WEA-Nr"), fs->tWeak.weakWeaId.WeaNr));
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scanean,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_Info1),acLineBuf[1],-1,	
		DparEfContents,efname(ef_Msgline),acMulti,-1,
		0);

	/* Free allocated memory */
	if (MskParData != NULL) {
			    fs_release_Maskparam (MskParData);
	}

	return;
}

static int LookingForArt(dccw_context *dccw,dccclient_stm *scl,
						char *pcLine1, char *pcLine2, FS_Job_Context  *fs,
						TEP *ptTep, ART *ptArt, WEAP *ptWeap,
						char *pcFac)
{
	ART				atArt[2];
	WEAP			atWeap[2];
	WEAK			atWeak[2];
	char			acArtNr[ARTNR_LEN+1];
	char			acVar[VAR_LEN+1];
	char			acMsg[255+1];
	int				iRv, iWeMultiAvisErf;

	memset(acMsg, 0, sizeof(acMsg));
	memset(&atWeak[0], 0, sizeof(atWeak));

	    /* Read WeMultiAvisErf */
    if (PrmGet1Parameter (NULL, P_WeMultiAvisErf,
                        PRM_CACHE, &iWeMultiAvisErf) != PRM_OK) {
        iWeMultiAvisErf = 0;
    }


	if ((IsEmptyStrg(pcLine1) == 0 && IsEmptyStrg(pcLine2) != 0) ||
		(IsEmptyStrg(pcLine1) != 0 && IsEmptyStrg(pcLine2) ==0)) {

		if (IsEmptyStrg(pcLine1) == 0 && strlen(pcLine1) <= ARTNR_LEN) {
			strncpy(acArtNr, pcLine1, ARTNR_LEN);
			acArtNr[ARTNR_LEN] = '\0';
		} else if (IsEmptyStrg(pcLine2) == 0 && strlen(pcLine2) <= ARTNR_LEN) {
			strncpy(acArtNr, pcLine2, ARTNR_LEN);
			acArtNr[ARTNR_LEN] = '\0';
		}  else {
			StrCpyDestLen(acMsg,MlMsg("Kein Artikel gefunden."));
			SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}

		memset(&atArt[0], 0, sizeof(atArt));
		memset(&atWeap[0], 0, sizeof(atWeap));

		iRv = TExecSqlX(NULL, NULL,
				"SELECT %ART,%WEAP FROM ART,WEAP "
				"WHERE ART.AId_Mand = :mand "
				"  AND ART.AId_ArtNr = :artnr "
				"  AND ART.AId_Var = '.' "
				"  AND WEAP.MId_Aid_Mand=ART.AId_Mand"
				"  AND WEAP.MId_Aid_ArtNr=ART.AId_ArtNr"
				"  AND WEAP.MId_Aid_Var=ART.AId_Var"
				"  AND WEAP.WeaId_Mand=:mand1"
				"  AND WEAP.WeaId_WeaNr=:weanr"
				"  AND WEAP.WeaId_EinKz=:einkz",
				2, 0,
				SELSTRUCT(TN_ART, atArt[0]),
				SELSTRUCT(TN_WEAP, atWeap[0]),
				SQLSTRING(fs->tWeak.weakWeaId.Mand),
				SQLSTRING(acArtNr),
				SQLSTRING(fs->tWeak.weakWeaId.Mand),
				SQLSTRING(fs->tWeak.weakWeaId.WeaNr),
				SQLEINKZ(fs->tWeak.weakWeaId.EinKz),
				NULL);

		if ((iRv <=0)
		 || (TSqlError (NULL) !=0 && TSqlError (NULL) != SqlNotFound)) {
			LogPrintf(pcFac, LT_ALERT,
				"Artikel:%s nicht gefunden", acArtNr);
			StrCpyDestLen(acMsg,MlMsg("Kein Artikel gefunden."));
			SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		} else if (iRv > 0) {
			*ptArt = atArt[0];
			*ptWeap = atWeap[0];
			fs->tTep = *ptTep;
			return(1);
		}
	
	} else if (IsEmptyStrg(pcLine1) == 0 && IsEmptyStrg(pcLine2) == 0) {
	

		if ( (strlen(pcLine1) <= ARTNR_LEN) &&
			(strlen(pcLine2) <= VAR_LEN) ) {
		        strncpy(acArtNr, pcLine1, ARTNR_LEN);
		        acArtNr[ARTNR_LEN] = '\0';
		        strncpy(acVar, pcLine2, VAR_LEN);
		        acVar[VAR_LEN] = '\0';
		}  else {
		        StrCpyDestLen(acMsg,MlMsg("Kein Artikel gefunden."));
		        SendError(dccw,scl, acMsg, DCC_ERROR,
				                        DCC_WRITE_LOG, GetFunkFac(fs));
		        return -1;
		}	

		memset(&atArt[0], 0, sizeof(atArt));
		memset(&atWeap[0], 0, sizeof(atWeap));

		if (iWeMultiAvisErf == JANEIN_J) {		

			iRv = TExecSqlX (NULL, NULL,
						"SELECT %WEAK, %WEAP, %ART FROM WEAK, WEAP, ART WHERE "
						"WEAK.Status IN ('ANMELD', 'AKTIV') "
						"AND WEAP.MId_AId_Mand = ART.AId_Mand "
						"AND WEAP.MId_AId_ArtNr = ART.AId_ArtNr "
						"AND WEAP.MId_AId_Var = ART.AId_Var "
						"AND WEAK.WeaId_Mand = WEAP.WeaId_Mand "
						"AND WEAK.WeaId_WeaNr = WEAP.WeaId_WeaNr "
						"AND WEAK.WeaId_EinKz = WEAK.WeaId_EinKz "
						"AND (WEAP.Status IN ('NEU', 'AKTIV') "
						"	OR (WEAP.Status = 'FERTIG' "
						"	AND WEAK.Ueberlief = 1 ) "
						"	OR (WEAP.Status = 'FERTIG' "
						"	AND WEAP.LiMngsLls_Mng < WEAP.MaxErfMngs_Mng "
						"   AND WEAK.Ueberlief = 0 )) "
						"AND ART.AId_ArtNr = :ArtNr "
						"AND ART.AId_Var = :Var "
						"AND WEAK.Position_FeldId = :FeldId "
						"ORDER BY WEAP.WEAID_MAND, "
						"		  WEAP.WEAID_WEANR, "
						"		  WEAP.WEAID_EINKZ, "
						"		  WEAP.POSNR ",
						2, 0,
						SQLSTRING(acArtNr),			
						SQLSTRING(acVar),
						SQLSTRING(fs->tRampePos.FeldId),
						SELSTRUCT(TN_WEAK, atWeak[0]),
						SELSTRUCT(TN_WEAP, atWeap[0]),
						SELSTRUCT(TN_ART, atArt[0]),
						NULL);

		} else {

			iRv = TExecSqlX(NULL, NULL,
		                "SELECT %ART,%WEAP FROM ART,WEAP "
		                "WHERE ART.AId_Mand = :mand "
		                "  AND ART.AId_ArtNr = :artnr "
						"  AND ART.AId_Var = :Var "
 		                "  AND WEAP.MId_Aid_Mand=ART.AId_Mand"
		                "  AND WEAP.MId_Aid_ArtNr=ART.AId_ArtNr"
		                "  AND WEAP.MId_Aid_Var=ART.AId_Var"
		                "  AND WEAP.WeaId_Mand=:mand1"
		                "  AND WEAP.WeaId_WeaNr=:weanr"
		                "  AND WEAP.WeaId_EinKz=:einkz",
		                2, 0,
		                SELSTRUCT(TN_ART, atArt[0]),
		                SELSTRUCT(TN_WEAP, atWeap[0]),
		                SQLSTRING(fs->tWeak.weakWeaId.Mand),
		                SQLSTRING(acArtNr),
						SQLSTRING(acVar),
		                SQLSTRING(fs->tWeak.weakWeaId.Mand),
		                SQLSTRING(fs->tWeak.weakWeaId.WeaNr),
		                SQLEINKZ(fs->tWeak.weakWeaId.EinKz),
		                NULL);	
		}

		if ((iRv <=0)
			 || (TSqlError (NULL) !=0 && TSqlError (NULL) != SqlNotFound)) {
		        LogPrintf(pcFac, LT_ALERT,
				"Artikel:%s/%s fuer %s/%s/%d nicht gefunden Sql:%s", 
				acArtNr, 
				acVar,
				fs->tWeak.weakWeaId.Mand,
				fs->tWeak.weakWeaId.WeaNr,
				fs->tWeak.weakWeaId.EinKz,
				TSqlErrTxt(NULL));
		        StrCpyDestLen(acMsg,MlMsg("Kein Artikel gefunden."));
		        SendError(dccw,scl, acMsg, DCC_ERROR,
		                      DCC_WRITE_LOG, GetFunkFac(fs));
		        return -1;
		} else if (iRv > 0) {
	
			if (iWeMultiAvisErf == JANEIN_N) {		

		       *ptArt = atArt[0];
		       *ptWeap = atWeap[0];
		        fs->tTep = *ptTep;
				return(1);
			} else {

				fs->tTep = *ptTep;
				*ptArt = atArt[0];
				*ptWeap = atWeap[0];
				fs->tWeak = atWeak[0];
				fs->tArt = atArt[0];
		        return(1);
			}
		}
	
	} else {
	    StrCpyDestLen(acMsg,MlMsg("Kein Artikel gefunden."));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
	return 0;
}

int GetDiffErfLiMng (char *pcFac, FS_Job_Context *fs,
							double *pdDiffErfLiMng)
{
	int		iRvDb;

	*pdDiffErfLiMng = 0;

	iRvDb = TExecSql (NULL, "SELECT "
							   "SUM(WEAP.MaxErfMngs_Mng - WEAP.LiMngsLls_Mng) "
							"FROM WEAK, WEAP "
                            "WHERE WEAK.Status IN "
                                "("STR_WEAKSTATUS_ANMELD", "
                                STR_WEAKSTATUS_AKTIV") "
                            "AND WEAK.WeaId_Mand = WEAP.WeaId_Mand "
                            "AND WEAK.WeaId_WeaNr = WEAP.WeaId_WeaNr "
                            "AND WEAK.WeaId_EinKz = WEAK.WeaId_EinKz "
							"AND WEAK.Position_FeldId = :RampePos "
							"AND WEAP.LiMngsLls_Mng < WEAP.MaxErfMngs_Mng "
							"AND WEAP.MId_AId_Mand = :AId_Mand "
							"AND WEAP.MId_AId_ArtNr = :AId_ArtNr "
							"AND WEAP.MId_AId_Var = :AId_Var "
							"AND (WEAP.WeaId_Mand != :Mand OR "
							" WEAP.WeaId_WeaNr != :WeaNr OR "
							" WEAP.WeaId_EinKz != :EinKz OR "
							" WEAP.PosNr != :PosNr) ",
							SELDOUBLE (*pdDiffErfLiMng),
							SQLSTRING(fs->tRampePos.FeldId),
							SQLSTRING(fs->tWeap.weapMId.AId.Mand),
							SQLSTRING(fs->tWeap.weapMId.AId.ArtNr),
							SQLSTRING(fs->tWeap.weapMId.AId.Var),
							SQLSTRING(fs->tWeap.weapWeaId.Mand),
							SQLSTRING(fs->tWeap.weapWeaId.WeaNr),
							SQLEINKZ(fs->tWeap.weapWeaId.EinKz),
							SQLLONG(fs->tWeap.weapPosNr),
							NULL);

	if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
		LogPrintf (pcFac, LT_ALERT, "DBError: %s", TSqlErrTxt(NULL));
		return -1;
	}

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-* DESCRIPTION
-*  Look what was inputed from the WE Main-Menu
-* RETURNS
-*  Radio returns 0
-*--------------------------------------------------------------------------*/
int SmaGetIndentifWeGepl(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lFlags = 0, lAktCount=0, lGesCount=0, lDiffArt = 0;
	int				isPos=0, iRv=0, iRv2=0, iRvEls = 0, iWEANullInit,
	                isWeAvisTmp = 0, isWeAvis = 0;
	int				iEanArt, iValue = 0, iArtNrFound=0, iRv3=0;
	int				iWeMultiAvisErf = 0;
	int             iWeAvisPos = 0;
	POS				tPos;
	TEP				tTep;
	ART				tArt;
	int				isWeSapBestTmp = 0;
	int				iWeSapBestPos = 0;
	int				isSapBest = 0;
	int				iRvDb = 0;
	char			acMsg[128+1];
	char			acScanCode1[SCANCODE+1];
	char			acScanCode2[SCANCODE+1];
	char			acScanCode3[SCANCODE+1];
	char			*pcTeId = NULL;
	char			acSapBest[LIEFBESTNR_LEN+1];
	
	acMsg[0]='\0';
	acScanCode1[0]='\0';
	acScanCode2[0]='\0';
	acScanCode3[0]='\0';

	switch(scl->endkey){
		case DCCAPP_VK_ENTER:
			/* Fallthrough */
		case DCCKEY_SCAN_N:
		case DCCAPP_VK_SCAN:
			break;
		case DCCAPP_VK_ESC:

#ifdef UHU
			if (DeleteAllOfDummyNr(NULL, GetFunkFac(fs), fs->lDummyNr) < 0) {
			    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
    			SendError(dccw,scl, acMsg, DCC_ERROR,
           			DCC_WRITE_LOG, GetFunkFac(fs));
    			return(-1);
			}
			fs->iMischCount = 0;
#endif /* UHU */

			if (fs->eMode == DccGeplWe) {
				SmaBuildWeMainMenu(dccw,scl);
			} else {
                fs->eMode = DccGeplWe;
                SmaBuildIndentifWeGepl(dccw,scl);
            }
			return 0;
        case DCCAPP_VK_FULLMESS:
        	/* Misch-TE zuruecksetzen
        	-*/
			if (ResetMischFlag(dccw, scl, GetFunkFac(fs)) < 0) {
				return -1;
			}
			if (fs->eMode == DccGeplWe) {
				SmaBuildWeMainMenu(dccw,scl);
			} else {
                fs->eMode = DccGeplWe;
                SmaBuildIndentifWeGepl(dccw,scl);
            }
			return 0;
	case DCCAPP_VK_LASTTE:
	    /* Schnellerfassen */
		       if (fs->iLastOk == JANEIN_J) {
		           if (fs->tArt.artCrossDocking == JANEIN_N ||
			            fs->tWeap.weapBestMngs.Mng != 0) {
						if (fs->tWeak.weakUeberlief == JANEIN_N &&
								 (fs->tWeap.weapLiMngsLls.Mng + fs->tTep.tepMngs.Mng) >
							 fs->tWeap.weapMaxErfMngs.Mng) {

						    StrCpyDestLen (acMsg, MlMsg ("�berliefern nicht erlaubt!"));
							SendError(dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
												GetFunkFac(fs));
							return (0);
						}
					}
					fs->eMode = DccGeplWeTeBilden;
					memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
					memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
					fs->dSaveMngWE = 0.0;
    				SmaBuildTEBilden(dccw,scl, EANART128, JANEIN_J);
		       } else {
		           StrCpyDestLen(acMsg,MlMsg("Schnell-Erfassung nicht m�glich"));
			       SendError(dccw,scl, acMsg, DCC_ERROR,
					           DCC_NO_LOG, GetFunkFac(fs));
		       }
		   return (0);    
			
        case DCCAPP_VK_ABORT:
			/* Misch TE erfassung Fertig */

            if (fs->iMischCount > 0) {

				fs->iMischCount --;
				if (DeleteLastOfDummyNr(NULL, GetFunkFac(fs),
								fs->lDummyNr) < 0) {
				    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
					SendError(dccw,scl, acMsg, DCC_SQLERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}
                fs->eMode = DccGeplWeTeBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
                SmaBuildTEBilden(dccw,scl, EANART128, JANEIN_J);
                return (0);
            } /* else Falsche taste (Fallthrough) */
		default:
		  StrCpyDestLen(acMsg, DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}
	/* VeHeFa from art or weap? 0 = weap, 1 = art */
	if (PrmGet1Parameter (NULL, P_WeEveHeArt,
                        PRM_CACHE, &iValue) != PRM_OK) {
        iValue = 0;
    }
	/* Read WeMultiAvisErf */
	if (PrmGet1Parameter (NULL, P_WeMultiAvisErf,
                        PRM_CACHE, &iWeMultiAvisErf) != PRM_OK) {
        iWeMultiAvisErf = 0;
    }
    
    fs->iL2Erf = 0;
    memset(fs->acEan13Code, 0, sizeof(fs->acEan13Code));

	/* Lesen der Eingegebenen Felder */
	strcpy(acScanCode1, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode1), &lFlags, NULL));

	strcpy(acScanCode2, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode2), &lFlags, NULL));
	
#ifdef _POSWALD_NUR_ZWEI_ZEILEN	
	strcpy(acScanCode3, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode3), &lFlags, NULL));
#endif /* _POSWALD_NUR_ZWEI_ZEILEN */	

	memset(acScanCode3,0,sizeof(acScanCode3));

	if (fs->eMode == DccGeplWe) {
		/* Check if any of three fields is a valid We-Avis Nr */
		if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
			isWeAvisTmp = IsWeAvis (NULL, 
									GetFunkFac(fs),
									acScanCode1,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 1;
			}
			isWeAvis += isWeAvisTmp;
		}
		
		/* Check field2 */	
		if (IsEmptyStrg(acScanCode2) == JANEIN_N) {
			isWeAvisTmp = IsWeAvis (NULL, 
									GetFunkFac(fs),
									acScanCode2,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 2;
			}	
			isWeAvis += isWeAvisTmp;
		}
		
		/* Check field3 */	
		if (IsEmptyStrg(acScanCode3) == JANEIN_N) {
			isWeAvisTmp = IsWeAvis (NULL, 
									GetFunkFac(fs),
									acScanCode3,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 3;
			}
			isWeAvis += isWeAvisTmp;
		}

		memset(acSapBest, 0, sizeof(acSapBest));	
		/* Sap Bestellnummer feld 1*/	
		if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
			isWeSapBestTmp = IsSapBestAvis (NULL, 
									GetFunkFac(fs),
									acScanCode1,
									acSapBest);
			if (isWeSapBestTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeSapBestTmp > 0) {
				iWeSapBestPos = 1;
			}
			isSapBest += isWeSapBestTmp;
		}
		
		/* Sap Bestellnummer feld 2*/	
		if (IsEmptyStrg(acScanCode2) == JANEIN_N) {
			isWeSapBestTmp = IsSapBestAvis (NULL, 
									GetFunkFac(fs),
									acScanCode2,
									acSapBest);
			if (isWeSapBestTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeSapBestTmp > 0) {
				iWeSapBestPos = 2;
			}	
			isSapBest += isWeSapBestTmp;
		}


		if (isWeAvis == 1 && isSapBest == 1) {	

			char 	acWeaIdNr[WEANR_LEN+1];

			memset(acWeaIdNr, 0, sizeof(acWeaIdNr));
					
			iRvDb = TExecSql (NULL,"SELECT DISTINCT WEAK.WEAID_WEANR FROM WEAK, WEAP WHERE "
										"WEAK.WEAID_WEANR = WEAP.WEAID_WEANR AND "
										"WEAK.WEAID_WEANR = :WeaNr AND "
										"WEAP.LIEFBESTNR = :LiefBest ",
										SELSTR(acWeaIdNr,WEANR_LEN+1),
										SQLSTRING(fs->tWeak.weakWeaId.WeaNr),
										SQLSTR(acSapBest,LIEFBESTNR_LEN+1),
										NULL);	

			if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
				/* DB - Error */
				sprintf(acMsg, "%s", 
						MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return -1;	
			}		

			if (TSqlError(NULL) == SqlNotFound) {	
				/* Check SapBestNr and WeavisNr */
				scl->cur_maskid = ID_scanean;
				sprintf(acMsg, "%s", MlMsg("WE-Avis stimmt mit SapBestNr nicht �berein!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_WRITE_LOG, GetFunkFac(fs));
				return (-1);
			}

			if (iWeSapBestPos == 1) {
				memset(acScanCode1,0,sizeof(acScanCode1));
			} else {
				memset(acScanCode2,0,sizeof(acScanCode2));
			}		


		} else if (isWeAvis == 0 && isSapBest == 1) {	


			WEAK 	tWeak;	

			memset(&tWeak, 0, sizeof(tWeak));
					
			iRvDb = TExecSql (NULL,"SELECT %WEAK FROM WEAK, WEAP WHERE "
										"WEAK.WEAID_WEANR = WEAP.WEAID_WEANR AND "
										"WEAP.LIEFBESTNR = :LiefBestNr ",
										SELSTRUCT(TN_WEAK, tWeak),
										SQLSTRING(acSapBest),
										NULL);	

			if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
				/* DB - Error */
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return -1;	
			}		

			if (TSqlError(NULL) == SqlNotFound) {	
				/* Check SapBestNr and WeavisNr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("WE-Avis stimmt mit SapBestNr nicht �berein!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			}

			isWeAvis = 1;	
			iWeAvisPos = 3;
			fs->tWeak = tWeak;
			strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);

			if (iWeSapBestPos == 1) {
				memset(acScanCode1,0,sizeof(acScanCode1));
			} else {
				memset(acScanCode2,0,sizeof(acScanCode2));
			}		
		}	

		/* Check that max. one field is a valid We-Avis Nr */
		if (isWeAvis > 1 || isSapBest > 1) {
			/* More then one Avis found */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Bitte WE-Avis/SapBestNr nur einmal eingeben!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_NO_LOG, GetFunkFac(fs));
			return (-1);

		} else if (isWeAvis == 1) {
			/* 
			 * Valid WE_Avis
			 * switch WeAvisNr into acScanCode3 
			 */
			if (iWeAvisPos == 1) {
				strcpy(acScanCode1,acScanCode3);
				strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);
			} else if (iWeAvisPos == 2) {
				strcpy(acScanCode2,acScanCode3);
				strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);
			}
			fs->acEan13Code[0] = '\0';
			fs->acArtNrWe[0] = '\0';
			fs->acVar[0] = '\0';
			memset(&fs->tTep, 0, sizeof(TEP));
			
			iRv = DecodeEan(NULL, GetFunkFac(fs), acScanCode1, acScanCode2, 
							&tTep, &tArt, fs->acEan13Code, &iEanArt);
			
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
			    IsEmptyStrg(acScanCode3) == JANEIN_N && 
				IsEmptyStrg(fs->acEan13Code) == JANEIN_J) {
				/* All fields are not empty but no one is a valid EAN Nr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("Keine EAN gefunden "));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  	DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			}
		
			/* When no Artikel was decode, use the Artikel from Field 1 or 2*/
			if (IsEmptyStrg(acScanCode1) == JANEIN_N ||
			    IsEmptyStrg(acScanCode2) == JANEIN_N) {
				if (IsEmptyStrg(tArt.artAId.ArtNr) == JANEIN_J) { 
					if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
						strcpy(tArt.artAId.ArtNr,acScanCode1);
					} else {
						strcpy(tArt.artAId.ArtNr,acScanCode2);
					}
				}	
			}

			/*Decoded Artikel must equal one field wenn all fields are not empty*/
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
			    IsEmptyStrg(acScanCode3) == JANEIN_N) { 
				if (strcmp(acScanCode1,tArt.artAId.ArtNr) != 0 &&
				    strcmp(acScanCode2,tArt.artAId.ArtNr) != 0 &&
					strcmp(acScanCode3,tArt.artAId.ArtNr) != 0) {

					scl->cur_maskid = ID_scanean;
					StrCpyDestLen(acMsg, MlMsg("Keinen Artikel gefunden"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						  	DCC_NO_LOG, GetFunkFac(fs));
					return (-1);
				}
			}
			
			if (iRv > 0 || iRv == -2) { 
				/* Artikel Anhand des EAN-Codes gefunden ( iRv > 0) oder
				-* Kein Ean-Code Eingegeben 
				-*/
				/* Check the Artikel and WeaNr */
				iRv2 = CheckWeAvisForEan(NULL,
										GetFunkFac(fs),
										fs,
										fs->acEan13Code,
										tArt.artAId.ArtNr,
										JANEIN_N);

				if (iRv2 > 0) {						
					/* Ein Avis gefunden -> Maske zum Scannen
					-* der EAN wird aufgeschalten*/
					DccSetAktpos(fs, &fs->tWeak.weakPosition);
					fs->eMode = DccGeplWeEan;
					SmaBuildScanEanWeGepl(dccw,scl);
					return (0);
				} else if (iRv2 == 0) {
					/* Nichts gefunden */
				    StrCpyDestLen(acMsg, MlMsg("Der Artikel ist nicht gueltig fuer Avis !"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));
					return -1;
				} else if (iRv2 < 0) {
					/* Da ist was schief gegangen */
				    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));
					return -1;
				} 
			} else if (iRv < 0) {
				/* Da ist was schief gegangen */
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Auswerten des EAN-Codes!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
				return (-1);
			} else if (iRv == 0) {
				/* Keinen Artikel gefunden  */
			    StrCpyDestLen(acMsg, MlMsg("Kein Positionen gefunden!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));

				scl->cur_maskid = ID_scanean;
				return -1;
			}
		} else if (isWeAvis == 0) {
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
				IsEmptyStrg(acScanCode3) == JANEIN_N) {
				/* All fields are not empty but no one is a valid WE-Avis Nr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("Keine WE-Avis/SapBestNr gefunden "));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (IsEmptyStrg(acScanCode1) == JANEIN_J &&
					   IsEmptyStrg(acScanCode3) == JANEIN_N) {
			   /* Copy ScanCode3 to ScanCode1 */
			   strcpy(acScanCode1,acScanCode3);
		    } else if (IsEmptyStrg(acScanCode2) == JANEIN_J &&
			           IsEmptyStrg(acScanCode3) == JANEIN_N) {
				/* Copy ScanCode3 to ScanCode2 */
				strcpy(acScanCode2,acScanCode3);
			}
	   }
	}

	if (IsEmptyStrg(acScanCode2) == JANEIN_J &&
		IsEmptyStrg(acScanCode1) == JANEIN_N &&
		strlen(acScanCode1) < 12) {

		strcpy(acScanCode2, ".");

	}

	if (fs->iGebucht == JANEIN_J) {
		fs->iGebucht = JANEIN_N;
		fs->iMischCount = 0;
	}

	/* check if Barcode is scanned */
	if (fs->iMischCount == 0 && fs->eMode == DccGeplWe) {
		/* is one of the 2 fields empty -> check if data is a position */
		if ((IsEmptyStrg(acScanCode1) == 0 && IsEmptyStrg(acScanCode2)!= 0) ||
			(IsEmptyStrg(acScanCode2) == 0 && IsEmptyStrg(acScanCode1)!= 0)) {
	
			isPos = CheckIfPos(NULL, acScanCode1, acScanCode2, &tPos);
		}
	} else {
		isPos = 0;
	}
	if (iWeMultiAvisErf == 1 && fs->eMode == DccGeplWe && isPos <= 0 &&
		fs->iMischCount == 0) {
	    StrCpyDestLen(acMsg, MlMsg("Bitte Position (Rampe) scannen!"
		" (Parameter WeMultiAvisErf = 1)"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_NO_LOG, GetFunkFac(fs));

		fs->eMode = DccGeplWe;
		SmaBuildIndentifWeGepl(dccw, scl);
		return (iRv);
	}
	
	if (IsEmptyStrg(acScanCode1) != 0 && IsEmptyStrg(acScanCode2)!= 0 &&
		fs->eMode == DccGeplWe) { 
			/* Damit man alle m�glichen WE-Avise zur Auswahl bekommt,
			 -* werden folgende Variablen mit Hand gesetzt.
			 -*/
		isPos = 1;
		memset(&tPos, 0, sizeof(POS));	
	}

	/* Gescannte Daten sind eine Position */
	if (isPos > 0 ) {
		
		DccSetAktpos(fs, &tPos);

		iRv = SearchWeAvisForPos (	GetFunkFac(fs), 
									fs, 
									&lAktCount, 
									&lGesCount,
									&tPos,
									PAGE_NONE,
									JANEIN_N);

		if (iRv == 0) {
			/* Kein Avis gefunden */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Kein Avis gefunden"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));

#ifdef _POSWALD
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw, scl);
#endif /* POSWALD */
			return -1;

		} else if (iRv == 1 || (iRv > 1 && iWeMultiAvisErf == 1)) {
			/* Ein Avis gefunden -> Maske zum Scannen 
			-* der EAN wird aufgeschalten*/
			fs->tRampePos = fs->tWeak.weakPosition;
			DccSetAktpos(fs, &fs->tWeak.weakPosition);
			fs->eMode = DccGeplWeEan;
			SmaBuildScanEanWeGepl(dccw,scl);

		} else if (iRv > 1) {
			/* Dieser Fall -> Blaettern von Avis  */
			fs->eMode = DccGeplWePos;
			SmaBuildListAvisPosWeGepl(	dccw, 
										scl,
										lAktCount,
										lGesCount);
		} else {
			/* Es gab einen Datenbankfehler */
		    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
		return (iRv);
	} else {
		if (fs->eMode == DccGeplWe) {
			/* 	Search if scanned code is TeId which exists in ELSP table */
			/* Check if user has scanned TeId */
			pcTeId = NULL;
			if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
									acScanCode1) == 1) {
				pcTeId = acScanCode1;
			} else if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
									acScanCode2) == 1) {
				pcTeId = acScanCode2;
			}
			if (pcTeId != NULL) {
	
				iRv = SearchWeAvisForTeId (	GetFunkFac(fs),
											fs,
											&lAktCount,
											&lGesCount,
											pcTeId,
											PAGE_NONE,
											JANEIN_N);

				if (iRv == 1) {
					/* One Avis found -> Open Mask for EAN scan */
					DccSetAktpos(fs, &fs->tWeak.weakPosition);
					fs->eMode = DccGeplWeEan;
					SmaBuildScanEanWeGepl(dccw,scl);
					return (1);
				} 	else if (iRv > 1) {
					/* Error by HOST */
				    StrCpyDestLen(acMsg, MlMsg( 	"Fehler: der HOST hat falsche "
									"WE-Avis-Daten geschickt!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}  	else if (iRv == 0) {
					/* Fallthrough */
				}	else if (iRv < 0){
					/* Database error */
				    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}
			}
		}
	}

	fs->acEan13Code[0] = '\0';
	fs->acArtNrWe[0] = '\0';
	fs->acVar[0] = '\0';
	memset(&fs->tTep, 0, sizeof(TEP));
	iRv = DecodeEan(NULL, GetFunkFac(fs), acScanCode1, acScanCode2, 
					&tTep, &tArt, fs->acEan13Code, &iEanArt);
	
	if (IsEmptyStrg(acScanCode1) == 0 &&
		IsEmptyStrg(acScanCode2) == 0 &&
		iWeMultiAvisErf == 0) {

		strncpy(fs->acArtNrWe, acScanCode1, ARTNR_LEN);	
		fs->acArtNrWe[ARTNR_LEN] = '\0';

		strncpy(fs->acVar, acScanCode2, VAR_LEN);
		fs->acVar[VAR_LEN] = '\0';
	}

	/* Search for WE-Avis-Positions through TE-Id */				
	if(fs->eMode != DccGeplWe) {
		/* Check if user has scanned TeId */
		pcTeId = NULL;
		if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
		       					acScanCode1) == 1) {
		 	pcTeId = acScanCode1;
		} else if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
		       					acScanCode2) == 1) {
			pcTeId = acScanCode2;
		}
		if (pcTeId != NULL) {

			iRvEls = SearchWeAvisPosForEls(GetFunkFac(fs), 
										fs, 
										&lAktCount,
										&lGesCount,
										&fs->tWeak.weakWeaId,
										fs->acEan13Code, 
										pcTeId,
										PAGE_NONE,
										JANEIN_N);
			if (iRvEls == 1) {
				fs->tTep.tepMId = fs->tElsp.elspMId;
				fs->tTep.tepMngs = fs->tElsp.elspMngs;
				fs->tTep.tepFifoDatum = fs->tElsp.elspMHD; 
				strcpy(fs->acSerNr[0], fs->tElsp.elspMId.SerNr);
				
				fs->tTep.tepVps = fs->tArt.artVps;
				fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;
				
				fs->tTep.tepMngs.VeHeFa = iValue == 1 ? 
					fs->tArt.artMngs.VeHeFa : 
					fs->tWeap.weapBestMngs.VeHeFa;
				
				DccSetAktpos(fs, &fs->tWeak.weakPosition);
				fs->eMode = DccGeplWeTeBilden;
				
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildTEBilden(dccw,scl,iEanArt, 0);
				
				return 0;
			} else if (iRvEls == -2) {
			    StrCpyDestLen(acMsg, MlMsg(	"Es gibt mehrere Artikel f�r die TE-Id.\n"
								"Bitte TE-Id und den EAN-Code scannen!!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				SmaBuildScanEanWeGepl(dccw,scl);	
				return (-1);	
			} else if (iRvEls == 0) {
				/* Fallthrough */
			} else if (iRvEls < 0) {
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				return -1;	
			}
		}
	}
	/* Gescannte Daten sind ein EAN-Code */

	if (fs->eMode != DccGeplWe &&
		(iRv == 0 ||
        ((iRv == 1) && (IsEmptyStrg(tArt.artAId.ArtNr) == 1)))) { 
		iRv3 = LookingForArt(dccw, scl, acScanCode1, acScanCode2, fs,
											&tTep, 
											&tArt, 
											&fs->tWeap, 
											GetFunkFac(fs));

		if (iRv3 < 0) {
			return -1;
		} else if (iRv3 == 1) {
			iArtNrFound = 1;
		} 
	}

	if (iRv > 0 || iRv == -2) { 
			/* Artikel Anhand des EAN-Codes gefunden ( iRv > 0) oder
			-* Kein Ean-Code Eingegeben 
			-*/

		if (fs->eMode == DccGeplWe &&
			IsEmptyStrg(acScanCode1) == 0 &&
			IsEmptyStrg(acScanCode2) == 0 &&
			IsEmptyStrg(fs->acEan13Code) == 1 &&
			iWeMultiAvisErf == 0) {

			iRv2 = SearchWeAvisforEan(	GetFunkFac(fs), 
									fs, 
									&lAktCount,
									&lGesCount,
									&lDiffArt,
									&fs->tWeak.weakWeaId,
									NULL, 
									acScanCode1,
									acScanCode2,
									PAGE_NONE,
									JANEIN_N);
		}		

		if(fs->eMode == DccGeplWe &&
		   ((IsEmptyStrg(acScanCode1) == 0) || 
		    (IsEmptyStrg(acScanCode2) == 0)) &&
		    IsEmptyStrg(fs->acEan13Code) == 1 && 
			iRv2 == 0) {

			/* Kein Avis gefunden */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Kein Avis gefunden"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			

#ifdef _POSWALD
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw, scl);
#endif /* _POSWALD */
			return -1;
		}


		if (iRv != -2 || iArtNrFound == 1) {
			memcpy(&fs->tTep, &tTep, sizeof(TEP));
			memcpy(&fs->tArt, &tArt, sizeof(ART));
			memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
        	memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        	fs->tTep.tepMngs.VeHeFa = fs->tArt.artMngs.VeHeFa;
        	fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;
		}

		if (iArtNrFound == 0 && iRv2 == 0) {
			iRv2 = SearchWeAvisforEan(	GetFunkFac(fs), 
									fs, 
									&lAktCount,
									&lGesCount,
									&lDiffArt,
									&fs->tWeak.weakWeaId,
									fs->acEan13Code, 
									NULL,
									NULL,
									PAGE_NONE,
									JANEIN_N);
		}
		if ((iRv2 == 1 ||iArtNrFound == 1 || 
			(iWeMultiAvisErf == 1 && iRv2 > 1 && lDiffArt == JANEIN_N)) &&
			fs->eMode != DccGeplWe) {
			memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
            memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        	fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;

            /* Menge mit 0 vorinitialieren */
            if (PrmGet1Parameter (NULL, P_WEANullInit,
                        PRM_CACHE, &iWEANullInit) != PRM_OK) {
                iWEANullInit = JANEIN_N;
            }
            if (iWEANullInit == JANEIN_J) {
                fs->tTep.tepMngs.Mng = 0;
            }
        	
        	fs->tTep.tepMngs.VeHeFa = iValue == 1 ? 
		    	fs->tArt.artMngs.VeHeFa : 
		    	fs->tWeap.weapBestMngs.VeHeFa;
        	
			DccSetAktpos(fs, &fs->tWeak.weakPosition);
			fs->eMode = DccGeplWeTeBilden;
			
			memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
			memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
			fs->dSaveMngWE = 0.0;
			SmaBuildTEBilden(dccw,scl,iEanArt, 0);

		} else if (iRv2 == 1 && fs->eMode == DccGeplWe) {

            /* Ein Avis gefunden -> Maske zum Scannen
            -* der EAN wird aufgeschalten*/
            DccSetAktpos(fs, &fs->tWeak.weakPosition);
            fs->eMode = DccGeplWeEan;
            SmaBuildScanEanWeGepl(dccw,scl);

		} else if (iRv2 == 0) {
			/* Nichts gefunden */
		    StrCpyDestLen(acMsg, MlMsg("Kein Artikel f�r EAN-Code gefunden!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		} else if (iRv2 < 0) {
			/* Da ist was schief gegangen */
		    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		} else {
			/* mehrere Gefunden */
			SmaBuildListAvisEanWeGepl(	dccw,
										scl,
										lAktCount,
										lGesCount);
			return 0;
		}
			
	} else if (iRv < 0) {
		/* Da ist was schief gegangen */
	    StrCpyDestLen(acMsg, MlMsg("Fehler beim Auswerten des EAN-Codes!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
		return (-1);
	} else if (iRv == 0) {
		/* Keinen Artikel gefunden  */
	    StrCpyDestLen(acMsg, MlMsg("Kein Positionen gefunden!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));

		scl->cur_maskid = ID_scanean;
#ifdef _POSWALD
		SmaBuildIndentifWeGepl(dccw, scl);
#endif
		return -1;
	}

	return(0);
}
/*----------------------------------------------------------------------------
 * -* SYNOPSIS
 *  -*    $$
 *  -* DESCRIPTION
 *  -*  Look what was inputed from the WE Main-Menu
 *  -* RETURNS
 *  -*  Radio returns 0
 *  -*--------------------------------------------------------------------------*/
int SmaGetRetoureScanEan(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	long			lFlags = 0, lAktCount=0, lGesCount=0, lDiffArt = 0;
	int				isPos=0, iRv=0, iRv2=0, iRvEls = 0, iWEANullInit,
	                isWeAvisTmp = 0, isWeAvis = 0;
	int				iEanArt, iValue = 0, iArtNrFound=0, iRv3=0;
	int				iWeMultiAvisErf = 0;
	int             iWeAvisPos = 0;
	POS				tPos;
	TEP				tTep;
	ART				tArt;
	int				isWeSapBestTmp = 0;
	int				iWeSapBestPos = 0;
	int				isSapBest = 0;
	int				iRvDb = 0;
	char			acMsg[128+1];
	char			acScanCode1[SCANCODE+1];
	char			acScanCode2[SCANCODE+1];
	char			acScanCode3[SCANCODE+1];
	char			*pcTeId = NULL;
	char			acSapBest[LIEFBESTNR_LEN+1];
	
	acMsg[0]='\0';
	acScanCode1[0]='\0';
	acScanCode2[0]='\0';
	acScanCode3[0]='\0';


	switch(scl->endkey){
		case DCCAPP_VK_ENTER:
			/* Fallthrough */
		case DCCKEY_SCAN_N:
		case DCCAPP_VK_SCAN:
			break;
		case DCCAPP_VK_ESC:

#ifdef UHU
			if (DeleteAllOfDummyNr(NULL, GetFunkFac(fs), fs->lDummyNr) < 0) {
			    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
    			SendError(dccw,scl, acMsg, DCC_ERROR,
           			DCC_WRITE_LOG, GetFunkFac(fs));
    			return(-1);
			}
			fs->iMischCount = 0;
#endif /* UHU */
			if (fs->eMode == DccRetoure) {
				SmaBuildRetoureMainMenu(dccw,scl);
			} else {
                fs->eMode = DccRetoure;
                SmaBuildIndentifRetoure(dccw,scl);
            }
			return 0;
			
        case DCCAPP_VK_ABORT:
			/* Misch TE erfassung Fertig */

            if (fs->iMischCount > 0) {

				fs->iMischCount --;
				if (DeleteLastOfDummyNr(NULL, GetFunkFac(fs),
								fs->lDummyNr) < 0) {
				    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
					SendError(dccw,scl, acMsg, DCC_SQLERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}
                fs->eMode = DccRetoureBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
                SmaBuildRetoureMask(NULL,GetFunkFac(fs),dccw,scl);
                return (0);
            } /* else Falsche taste (Fallthrough) */
		default:
		  StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}
	/* VeHeFa from art or weap? 0 = weap, 1 = art */
	if (PrmGet1Parameter (NULL, P_WeEveHeArt,
                        PRM_CACHE, &iValue) != PRM_OK) {
        iValue = 0;
    }
	/* Read WeMultiAvisErf */
	if (PrmGet1Parameter (NULL, P_WeMultiAvisErf,
                        PRM_CACHE, &iWeMultiAvisErf) != PRM_OK) {
        iWeMultiAvisErf = 0;
    }
    
    fs->iL2Erf = 0;
    memset(fs->acEan13Code, 0, sizeof(fs->acEan13Code));

	/* Lesen der Eingegebenen Felder */
	strcpy(acScanCode1, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode1), &lFlags, NULL));

	strcpy(acScanCode2, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode2), &lFlags, NULL));
#ifdef _POSWALD_NUR_ZWEI_ZEILEN	
	strcpy(acScanCode3, dccmask_get_field(dccw, scl, 
					efname(ef_ScanCode3), &lFlags, NULL));
#endif /* _POSWALD_NUR_ZWEI_ZEILEN */	

	memset(acScanCode3,0,sizeof(acScanCode3));

	if (fs->eMode == DccRetoure) {
		/* Check if any of three fields is a valid We-Avis Nr */
		if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
			isWeAvisTmp = IsRetoure (NULL, 
									GetFunkFac(fs),
									acScanCode1,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 1;
			}
			isWeAvis += isWeAvisTmp;
		}
		
		/* Check field2 */	
		if (IsEmptyStrg(acScanCode2) == JANEIN_N) {
			isWeAvisTmp = IsRetoure (NULL, 
									GetFunkFac(fs),
									acScanCode2,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 2;
			}	
			isWeAvis += isWeAvisTmp;
		}
		
		/* Check field3 */	
		if (IsEmptyStrg(acScanCode3) == JANEIN_N) {
			isWeAvisTmp = IsRetoure (NULL, 
									GetFunkFac(fs),
									acScanCode3,
									&fs->tWeak);
			if (isWeAvisTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeAvisTmp > 0) {
				iWeAvisPos = 3;
			}
			isWeAvis += isWeAvisTmp;
		}

		memset(acSapBest, 0, sizeof(acSapBest));	
		/* Sap Bestellnummer feld 1*/	
		if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
			isWeSapBestTmp = IsRetoureLiefBestNr (NULL, 
									GetFunkFac(fs),
									acScanCode1,
									acSapBest);
			if (isWeSapBestTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeSapBestTmp > 0) {
				iWeSapBestPos = 1;
			}
			isSapBest += isWeSapBestTmp;
		}
		
		/* Sap Bestellnummer feld 2*/	
		if (IsEmptyStrg(acScanCode2) == JANEIN_N) {
			isWeSapBestTmp = IsRetoureLiefBestNr (NULL, 
									GetFunkFac(fs),
									acScanCode2,
									acSapBest);
			if (isWeSapBestTmp < 0) {
				/* Database error*/
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (isWeSapBestTmp > 0) {
				iWeSapBestPos = 2;
			}	
			isSapBest += isWeSapBestTmp;
		}
/* Wird erst spaeter abgefragt */
		isSapBest = 0;

		if (isWeAvis == 1 && isSapBest == 1) {	

			char 	acWeaIdNr[WEANR_LEN+1];

			memset(acWeaIdNr, 0, sizeof(acWeaIdNr));
					
			iRvDb = TExecSql (NULL,"SELECT DISTINCT WEAK.WEAID_WEANR FROM WEAK, WEAP WHERE "
										"WEAK.WEAID_WEANR = WEAP.WEAID_WEANR AND "
										"WEAK.WEAID_WEANR = :WeaNr AND "
										"WEAP.LIEFBESTNR = :LiefBest ",
										SELSTR(acWeaIdNr,WEANR_LEN+1),
										SQLSTRING(fs->tWeak.weakWeaId.WeaNr),
										SQLSTR(acSapBest,LIEFBESTNR_LEN+1),
										NULL);	

			if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
				/* DB - Error */
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return -1;	
			}		

			if (TSqlError(NULL) == SqlNotFound) {	
				/* Check SapBestNr and WeavisNr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("WE-Avis stimmt mit SapBestNr nicht �berein!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_WRITE_LOG, GetFunkFac(fs));
				return (-1);
			}

			if (iWeSapBestPos == 1) {
				memset(acScanCode1,0,sizeof(acScanCode1));
			} else {
				memset(acScanCode2,0,sizeof(acScanCode2));
			}		


		} else if (isWeAvis == 0 && isSapBest == 1) {	

			WEAK 	tWeak;	

			memset(&tWeak, 0, sizeof(tWeak));
					
			iRvDb = TExecSql (NULL,"SELECT %WEAK FROM WEAK, WEAP WHERE "
										"WEAK.WEAID_WEANR = WEAP.WEAID_WEANR AND "
										"WEAP.LIEFBESTNR = :LiefBestNr ",
										SELSTRUCT(TN_WEAK, tWeak),
										SQLSTRING(acSapBest),
										NULL);	

			if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
				/* DB - Error */
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return -1;	
			}		

			if (TSqlError(NULL) == SqlNotFound) {	
				/* Check SapBestNr and WeavisNr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("WE-Avis stimmt mit SapBestNr nicht �berein!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			}

			isWeAvis = 1;	
			iWeAvisPos = 3;
			fs->tWeak = tWeak;
			strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);

			if (iWeSapBestPos == 1) {
				memset(acScanCode1,0,sizeof(acScanCode1));
			} else {
				memset(acScanCode2,0,sizeof(acScanCode2));
			}		
		}	

		/* Check that max. one field is a valid We-Avis Nr */
		if (isWeAvis > 1 || isSapBest > 1) {
			/* More then one Avis found */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Bitte WE-Avis/SapBestNr nur einmal eingeben!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
					  DCC_NO_LOG, GetFunkFac(fs));
			return (-1);

		} else if (isWeAvis == 1) {
			/* 
			 * Valid WE_Avis
			 * switch WeAvisNr into acScanCode3 
			 */
			if (iWeAvisPos == 1) {
				strcpy(acScanCode1,acScanCode3);
				strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);
			} else if (iWeAvisPos == 2) {
				strcpy(acScanCode2,acScanCode3);
				strcpy(acScanCode3,fs->tWeak.weakWeaId.WeaNr);
			}
			fs->acEan13Code[0] = '\0';
			fs->acArtNrWe[0] = '\0';
			fs->acVar[0] = '\0';
			memset(&fs->tTep, 0, sizeof(TEP));
			
			iRv = DecodeEan(NULL, GetFunkFac(fs), acScanCode1, acScanCode2, 
							&tTep, &tArt, fs->acEan13Code, &iEanArt);
			
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
			    IsEmptyStrg(acScanCode3) == JANEIN_N && 
				IsEmptyStrg(fs->acEan13Code) == JANEIN_J) {
				/* All fields are not empty but no one is a valid EAN Nr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("Keine EAN gefunden "));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					  	DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			}
		
			/* When no Artikel was decode, use the Artikel from Field 1 or 2*/
			if (IsEmptyStrg(acScanCode1) == JANEIN_N ||
			    IsEmptyStrg(acScanCode2) == JANEIN_N) {
				if (IsEmptyStrg(tArt.artAId.ArtNr) == JANEIN_J) { 
					if (IsEmptyStrg(acScanCode1) == JANEIN_N) {
						strcpy(tArt.artAId.ArtNr,acScanCode1);
					} else {
						strcpy(tArt.artAId.ArtNr,acScanCode2);
					}
				}	
			}

			/*Decoded Artikel must equal one field wenn all fields are not empty*/
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
			    IsEmptyStrg(acScanCode3) == JANEIN_N) { 
				if (strcmp(acScanCode1,tArt.artAId.ArtNr) != 0 &&
				    strcmp(acScanCode2,tArt.artAId.ArtNr) != 0 &&
					strcmp(acScanCode3,tArt.artAId.ArtNr) != 0) {

					scl->cur_maskid = ID_scanean;
					StrCpyDestLen(acMsg, MlMsg("Keinen Artikel gefunden"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						  	DCC_NO_LOG, GetFunkFac(fs));
					return (-1);
				}
			}
			
			if (iRv > 0 || iRv == -2) { 
				/* Artikel Anhand des EAN-Codes gefunden ( iRv > 0) oder
				-* Kein Ean-Code Eingegeben 
				-*/
				/* Check the Artikel and WeaNr */
				iRv2 = CheckWeAvisForEan(NULL,
										GetFunkFac(fs),
										fs,
										fs->acEan13Code,
										tArt.artAId.ArtNr,
										JANEIN_J);

				if (iRv2 > 0) {						
					/* Ein Avis gefunden -> Maske zum Scannen
					-* der EAN wird aufgeschalten*/
					DccSetAktpos(fs, &fs->tWeak.weakPosition);
					fs->eMode = DccGeplWeEan;
					SmaBuildScanEanRetoure(dccw,scl);
					return (0);
				} else if (iRv2 == 0) {
					/* Nichts gefunden */
				    StrCpyDestLen(acMsg, MlMsg("Der Artikel ist nicht gueltig fuer Avis !"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));
					return -1;
				} else if (iRv2 < 0) {
					/* Da ist was schief gegangen */
				    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));
					return -1;
				} 
			} else if (iRv < 0) {
				/* Da ist was schief gegangen */
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Auswerten des EAN-Codes!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
				return (-1);
			} else if (iRv == 0) {
				/* Keinen Artikel gefunden  */
			    StrCpyDestLen(acMsg, MlMsg("Kein Positionen gefunden!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_NO_LOG, GetFunkFac(fs));

				scl->cur_maskid = ID_scanean;
				return -1;
			}
		} else if (isWeAvis == 0) {
			if (IsEmptyStrg(acScanCode1) == JANEIN_N &&
				IsEmptyStrg(acScanCode2) == JANEIN_N &&
				IsEmptyStrg(acScanCode3) == JANEIN_N) {
				/* All fields are not empty but no one is a valid WE-Avis Nr */
				scl->cur_maskid = ID_scanean;
				StrCpyDestLen(acMsg, MlMsg("Keine WE-Avis/SapBestNr gefunden "));
				SendError(dccw,scl, acMsg, DCC_ERROR,
						  DCC_NO_LOG, GetFunkFac(fs));
				return (-1);
			} else if (IsEmptyStrg(acScanCode1) == JANEIN_J &&
					   IsEmptyStrg(acScanCode3) == JANEIN_N) {
			   /* Copy ScanCode3 to ScanCode1 */
			   strcpy(acScanCode1,acScanCode3);
		    } else if (IsEmptyStrg(acScanCode2) == JANEIN_J &&
			           IsEmptyStrg(acScanCode3) == JANEIN_N) {
				/* Copy ScanCode3 to ScanCode2 */
				strcpy(acScanCode2,acScanCode3);
			}
	   }
	}


	if (IsEmptyStrg(acScanCode2) == JANEIN_J &&
		IsEmptyStrg(acScanCode1) == JANEIN_N &&
		strlen(acScanCode1) < 12) {

		strcpy(acScanCode2, ".");

	}

	/* Check LiefBestnr */

	/* check if Barcode is scanned */
	if (fs->iMischCount == 0 && fs->eMode == DccRetoure) {
		/* is one of the 2 fields empty -> check if data is a position */
		if ((IsEmptyStrg(acScanCode1) == 0 && IsEmptyStrg(acScanCode2)!= 0) ||
			(IsEmptyStrg(acScanCode2) == 0 && IsEmptyStrg(acScanCode1)!= 0)) {

			isWeSapBestTmp = IsRetoureLiefBestNr (NULL,
                        GetFunkFac(fs), acScanCode1, acSapBest);
		}
	} else {
		isWeSapBestTmp = 0;
	}


	if (isWeSapBestTmp > 0 ) {
		

		iRv = SearchWeAvisForLiefBestNr (	GetFunkFac(fs), 
									fs, 
									&lAktCount, 
									&lGesCount,
									acScanCode1,
									PAGE_NONE,
									JANEIN_J);

		if (iRv == 0) {
			/* Kein Avis gefunden */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Keine Retoure gefunden"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));

#ifdef _POSWALD
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw, scl);
#endif /* POSWALD */
			return -1;

		} else if (iRv == 1 || (iRv > 1 && iWeMultiAvisErf == 1)) {
			/* Ein Avis gefunden -> Maske zum Scannen 
			-* der EAN wird aufgeschalten*/
			fs->tRampePos = fs->tWeak.weakPosition;
			DccSetAktpos(fs, &fs->tWeak.weakPosition);
			fs->eMode = DccRetoureEan;
			SmaBuildScanEanRetoure(dccw,scl);

		} else if (iRv > 1) {
			/* Dieser Fall -> Blaettern von Avis  */
			fs->eMode = DccRetoureLiefBestNr;
			SmaBuildListLiefBestNrRetoure(	dccw, 
										scl,
										lAktCount,
										lGesCount);
		} else {
			/* Es gab einen Datenbankfehler */
		    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
		return (iRv);
	}



	if (fs->iGebucht == JANEIN_J) {
		fs->iGebucht = JANEIN_N;
		fs->iMischCount = 0;
	}

	/* check if Barcode is scanned */
	if (fs->iMischCount == 0 && fs->eMode == DccRetoure) {
		/* is one of the 2 fields empty -> check if data is a position */
		if ((IsEmptyStrg(acScanCode1) == 0 && IsEmptyStrg(acScanCode2)!= 0) ||
			(IsEmptyStrg(acScanCode2) == 0 && IsEmptyStrg(acScanCode1)!= 0)) {
	
			isPos = CheckIfPos(NULL, acScanCode1, acScanCode2, &tPos);
		}
	} else {
		isPos = 0;
	}

	if (iWeMultiAvisErf == 1 && fs->eMode == DccRetoure && isPos <= 0 &&
		fs->iMischCount == 0) {
	    StrCpyDestLen(acMsg, MlMsg("Bitte Position (Rampe) scannen!"
		" (Parameter WeMultiAvisErf = 1)"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_NO_LOG, GetFunkFac(fs));

		fs->eMode = DccRetoure;
		SmaBuildIndentifRetoure(dccw, scl);
		return (iRv);
	}
	
	if (IsEmptyStrg(acScanCode1) != 0 && IsEmptyStrg(acScanCode2)!= 0 &&
		fs->eMode == DccRetoure) { 
			/* Damit man alle m�glichen WE-Avise zur Auswahl bekommt,
			 -* werden folgende Variablen mit Hand gesetzt.
			 -*/
		isPos = 1;
		memset(&tPos, 0, sizeof(POS));	
	}

	/* Gescannte Daten sind eine Position */
	if (isPos > 0 ) {
		
		DccSetAktpos(fs, &tPos);

		iRv = SearchWeAvisForPos (	GetFunkFac(fs), 
									fs, 
									&lAktCount, 
									&lGesCount,
									&tPos,
									PAGE_NONE,
									JANEIN_J);

		if (iRv == 0) {
			/* Kein Avis gefunden */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Kein Avis gefunden"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));

#ifdef _POSWALD
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw, scl);
#endif /* POSWALD */
			return -1;

		} else if (iRv == 1 || (iRv > 1 && iWeMultiAvisErf == 1)) {
			/* Ein Avis gefunden -> Maske zum Scannen 
			-* der EAN wird aufgeschalten*/
			fs->tRampePos = fs->tWeak.weakPosition;
			DccSetAktpos(fs, &fs->tWeak.weakPosition);
			fs->eMode = DccRetoureEan;
			SmaBuildScanEanRetoure(dccw,scl);

		} else if (iRv > 1) {
			/* Dieser Fall -> Blaettern von Avis  */
			fs->eMode = DccRetourePos;
			SmaBuildListAvisPosRetoure(	dccw, 
										scl,
										lAktCount,
										lGesCount);
		} else {
			/* Es gab einen Datenbankfehler */
		    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
		return (iRv);
	} else {
		if (fs->eMode == DccRetoure) {
			/* 	Search if scanned code is TeId which exists in ELSP table */
			/* Check if user has scanned TeId */
			pcTeId = NULL;
			if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
									acScanCode1) == 1) {
				pcTeId = acScanCode1;
			} else if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
									acScanCode2) == 1) {
				pcTeId = acScanCode2;
			}
			if (pcTeId != NULL) {
	
				iRv = SearchWeAvisForTeId (	GetFunkFac(fs),
											fs,
											&lAktCount,
											&lGesCount,
											pcTeId,
											PAGE_NONE,
											JANEIN_J);

				if (iRv == 1) {
					/* One Avis found -> Open Mask for EAN scan */
					DccSetAktpos(fs, &fs->tWeak.weakPosition);
					fs->eMode = DccRetoureEan;
					SmaBuildScanEanRetoure(dccw,scl);
					return (1);
				} 	else if (iRv > 1) {
					/* Error by HOST */
				    StrCpyDestLen(acMsg, MlMsg( 	"Fehler: der HOST hat falsche "
									"WE-Avis-Daten geschickt!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}  	else if (iRv == 0) {
					/* Fallthrough */
				}	else if (iRv < 0){
					/* Database error */
				    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
					SendError(dccw,scl, acMsg, DCC_ERROR,
						DCC_WRITE_LOG, GetFunkFac(fs));
					return -1;
				}
			}
		}
	}

	fs->acEan13Code[0] = '\0';
	fs->acArtNrWe[0] = '\0';
	fs->acVar[0] = '\0';
	memset(&fs->tTep, 0, sizeof(TEP));
	iRv = DecodeEan(NULL, GetFunkFac(fs), acScanCode1, acScanCode2, 
					&tTep, &tArt, fs->acEan13Code, &iEanArt);
	
	if (IsEmptyStrg(acScanCode1) == 0 &&
		IsEmptyStrg(acScanCode2) == 0 &&
		iWeMultiAvisErf == 0) {

		strncpy(fs->acArtNrWe, acScanCode1, ARTNR_LEN);	
		fs->acArtNrWe[ARTNR_LEN] = '\0';

		strncpy(fs->acVar, acScanCode2, VAR_LEN);
		fs->acVar[VAR_LEN] = '\0';
	}

	/* Search for WE-Avis-Positions through TE-Id */				
	if(fs->eMode != DccRetoure) {
		/* Check if user has scanned TeId */
		pcTeId = NULL;
		if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
		       					acScanCode1) == 1) {
		 	pcTeId = acScanCode1;
		} else if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
		       					acScanCode2) == 1) {
			pcTeId = acScanCode2;
		}
		if (pcTeId != NULL) {

			iRvEls = SearchWeAvisPosForEls(GetFunkFac(fs), 
										fs, 
										&lAktCount,
										&lGesCount,
										&fs->tWeak.weakWeaId,
										fs->acEan13Code, 
										pcTeId,
										PAGE_NONE,
										JANEIN_J);
			if (iRvEls == 1) {
				fs->tTep.tepMId = fs->tElsp.elspMId;
				fs->tTep.tepMngs = fs->tElsp.elspMngs;
				fs->tTep.tepFifoDatum = fs->tElsp.elspMHD; 
				strcpy(fs->acSerNr[0], fs->tElsp.elspMId.SerNr);
				
				fs->tTep.tepVps = fs->tArt.artVps;
				fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;
				
				fs->tTep.tepMngs.VeHeFa = iValue == 1 ? 
					fs->tArt.artMngs.VeHeFa : 
					fs->tWeap.weapBestMngs.VeHeFa;
				
				DccSetAktpos(fs, &fs->tWeak.weakPosition);
				fs->eMode = DccGeplWeTeBilden;
				
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildRetoureMask(NULL,GetFunkFac(fs),dccw,scl);
				
				return 0;
			} else if (iRvEls == -2) {
			    StrCpyDestLen(acMsg, MlMsg(	"Es gibt mehrere Artikel f�r die TE-Id.\n"
								"Bitte TE-Id und den EAN-Code scannen!!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				SmaBuildScanEanRetoure(dccw,scl);	
				return (-1);	
			} else if (iRvEls == 0) {
				/* Fallthrough */
			} else if (iRvEls < 0) {
			    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
				return -1;	
			}
		}
	}
	/* Gescannte Daten sind ein EAN-Code */

	if (fs->eMode != DccRetoure &&
		(iRv == 0 ||
        ((iRv == 1) && (IsEmptyStrg(tArt.artAId.ArtNr) == 1)))) { 
		iRv3 = LookingForArt(dccw, scl, acScanCode1, acScanCode2, fs,
											&tTep, 
											&tArt, 
											&fs->tWeap, 
											GetFunkFac(fs));

		if (iRv3 < 0) {
			return -1;
		} else if (iRv3 == 1) {
			iArtNrFound = 1;
		} 
	}

	if (iRv > 0 || iRv == -2) { 
			/* Artikel Anhand des EAN-Codes gefunden ( iRv > 0) oder
			-* Kein Ean-Code Eingegeben 
			-*/

		if (fs->eMode == DccRetoure &&
			IsEmptyStrg(acScanCode1) == 0 &&
			IsEmptyStrg(acScanCode2) == 0 &&
			IsEmptyStrg(fs->acEan13Code) == 1 &&
			iWeMultiAvisErf == 0) {

			iRv2 = SearchWeAvisforEan(	GetFunkFac(fs), 
									fs, 
									&lAktCount,
									&lGesCount,
									&lDiffArt,
									&fs->tWeak.weakWeaId,
									NULL, 
									acScanCode1,
									acScanCode2,
									PAGE_NONE,
									JANEIN_J);
		}		

		if(fs->eMode == DccRetoure &&
		   ((IsEmptyStrg(acScanCode1) == 0) || 
		    (IsEmptyStrg(acScanCode2) == 0)) &&
		    IsEmptyStrg(fs->acEan13Code) == 1 && 
			iRv2 == 0) {

			/* Kein Avis gefunden */
			scl->cur_maskid = ID_scanean;
			StrCpyDestLen(acMsg, MlMsg("Kein Avis gefunden"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			

#ifdef _POSWALD
			fs->eMode = DccGeplWe;
			SmaBuildIndentifWeGepl(dccw, scl);
#endif /* _POSWALD */
			return -1;
		}


		if (iRv != -2 || iArtNrFound == 1) {
			memcpy(&fs->tTep, &tTep, sizeof(TEP));
			memcpy(&fs->tArt, &tArt, sizeof(ART));
			memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
        	memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        	fs->tTep.tepMngs.VeHeFa = fs->tArt.artMngs.VeHeFa;
        	fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;
		}

		if (iArtNrFound == 0 && iRv2 == 0) {
			iRv2 = SearchWeAvisforEan(	GetFunkFac(fs), 
									fs, 
									&lAktCount,
									&lGesCount,
									&lDiffArt,
									&fs->tWeak.weakWeaId,
									fs->acEan13Code, 
									NULL,
									NULL,
									PAGE_NONE,
									JANEIN_J);
		}
		if ((iRv2 == 1 ||iArtNrFound == 1 || 
			(iWeMultiAvisErf == 1 && iRv2 > 1 && lDiffArt == JANEIN_N)) &&
			fs->eMode != DccRetoure) {
			memcpy(&fs->tTep.tepMId.AId, &fs->tArt.artAId, sizeof(AID));
            memcpy(&fs->tTep.tepVps, &fs->tArt.artVps, sizeof(VPS));
        	fs->tTep.tepMngs.GewVe = fs->tWeap.weapAnmMngs.GewVe;

            /* Menge mit 0 vorinitialieren */
            if (PrmGet1Parameter (NULL, P_WEANullInit,
                        PRM_CACHE, &iWEANullInit) != PRM_OK) {
                iWEANullInit = JANEIN_N;
            }
            if (iWEANullInit == JANEIN_J) {
                fs->tTep.tepMngs.Mng = 0;
            }
        	
        	fs->tTep.tepMngs.VeHeFa = iValue == 1 ? 
		    	fs->tArt.artMngs.VeHeFa : 
		    	fs->tWeap.weapBestMngs.VeHeFa;
        	
			DccSetAktpos(fs, &fs->tWeak.weakPosition);
			fs->eMode = DccRetoure;
			
			memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
			memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
			fs->dSaveMngWE = 0.0;
			SmaBuildRetoureMask(NULL,GetFunkFac(fs),dccw,scl);

		} else if (iRv2 == 1 && fs->eMode == DccGeplWe) {

            /* Ein Avis gefunden -> Maske zum Scannen
            -* der EAN wird aufgeschalten*/
            DccSetAktpos(fs, &fs->tWeak.weakPosition);
            fs->eMode = DccRetoureEan;
            SmaBuildScanEanRetoure(dccw,scl);

		} else if (iRv2 == 0) {
			/* Nichts gefunden */
		    StrCpyDestLen(acMsg, MlMsg("Kein Artikel f�r EAN-Code gefunden!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		} else if (iRv2 < 0) {
			/* Da ist was schief gegangen */
		    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
			SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		} else {
			/* mehrere Gefunden */
			SmaBuildListAvisEanRetoure(	dccw,
										scl,
										lAktCount,
										lGesCount);
			return 0;
		}
			
	} else if (iRv < 0) {
		/* Da ist was schief gegangen */
	    StrCpyDestLen(acMsg, MlMsg("Fehler beim Auswerten des EAN-Codes!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
		return (-1);
	} else if (iRv == 0) {
		/* Keinen Artikel gefunden  */
	    StrCpyDestLen(acMsg, MlMsg("Kein Positionen gefunden!"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
				DCC_NO_LOG, GetFunkFac(fs));

		scl->cur_maskid = ID_scanean;
#ifdef _POSWALD
		SmaBuildIndentifWeGepl(dccw, scl);
#endif
		return -1;
	}

	return(0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*	  $$
-*  SmaGetWeMainInput(dccw_context *dccw,dccclient_stm *scl)
-* DESCRIPTION
-*  Look what was inputed from the WE Main-Menu
-* RETURNS
-*  Radio returns 0
-*--------------------------------------------------------------------------*/
int SmaGetTeBildenWeGepl(dccw_context *dccw,dccclient_stm *scl)
{
    FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
    long            lFlags = 0, lIsMischTe = 0, lRetWECheck, lCountTep = 0;
	char            acMsg[128+1];
	int 			iRv, iArtOfLp=0, iSendError=JANEIN_N, iRvDb;
	int				iWeAnmWeightTol, iLastTe=JANEIN_N;
	int				iWeMultiAvisErf, lLpOhneKpl=JANEIN_N;
    POS             tPos;
    double          dGewTolProz, dGewTolSoll, dGewTolIst, dGewTolMax;
    double          dMaxArtWeightFa, dSollGew;
	double          lfTmpGew, lfGewTol, lfTmpMinAnmGew, lfAnmMinTmpGew;
	double 			dDiffErfLiMng;
    char            acWeDefSpKz[SPKZ_LEN+1];
    char			acRestlaufMatKzWE[MATKZ_LEN+1] = {0};
    RetWareType		eRetWareType;
    TEP				tTep;
	time_t          zMHD;
	// double			dMng = 0;



	
	strcpy(fs->tTep.tepMId.MatKz, fs->tWeap.weapMId.MatKz);
	
	switch(scl->endkey){
		case DCCAPP_VK_CHARGE:

	        fs->tTep.tepMngs.Mng = GetDoubleFromRadioInput(dccw, scl,
             	efname(ef_AnzStueck), &lFlags, NULL);

			fs->dSaveMngWE = fs->tTep.tepMngs.Mng;	

   			LogPrintf ("TEST_MNG", LT_ALERT, "Mng:%f", fs->dSaveMngWE);
        	
			zMHD = GetWeDateFromRadioInput (dccw, scl, efname(ef_MHD),
										&lFlags, NULL, 0);
			if (zMHD < 100000) {
				zMHD = GetWeDateFromRadioInput (dccw, scl, efname(ef_MHD),
												&lFlags, NULL, 1);
			} 

			fs->tTep.tepMHD = zMHD;
			fs->tTep.tepFifoDatum = fs->tTep.tepMHD;

			if (MakeInfoList (dccw, scl, GetFunkFac(fs),
            	NULL, InfoListeCharge) < 0) {
   				 LogPrintf (GetFunkFac(fs), LT_ALERT, "ERROR: MakeInfoList failed!");
    			return (-1);
			}

			break;

		case DCCAPP_VK_KORTBUCH:
			if (fs->iMischCount == 1) {
			    StrCpyDestLen(acMsg,MlMsg("Misch-TE kann nicht auf KPL gebucht werden"));
            	SendError(dccw,scl, acMsg, DCC_ERROR,
                			DCC_WRITE_LOG, GetFunkFac(fs));
           		return -1;
			} else if (IsEmptyStrg(fs->tWeap.weapResNr) == 0) {
			    StrCpyDestLen(acMsg,MlMsg("Reservierte Ware darf "
					"nicht auf KPL gebucht werden!"));
            	SendError(dccw,scl, acMsg, DCC_ERROR,
                			DCC_WRITE_LOG, GetFunkFac(fs));
           		return -1;
			} else {
				memset(&tPos, 0, sizeof(POS));
            	iRv = FindKortOrLp(dccw,scl, &iArtOfLp, &tPos, JANEIN_J);
            	if (iRv < 0) { /* Datenbank Fehler */
                	/* Meldung wird in FindKortOrLp ausgespuckt */
                	return (-1);
				}
				SmaBuildKortBuchWe(dccw, scl, &tPos, iArtOfLp);
				return 1;
			}
		case DCCAPP_VK_ENTER:
			/* Fallthrough */
        case DCCKEY_SCAN_N:
        case DCCAPP_VK_SCAN:
            break;

		case DCCAPP_VK_ESC:
			if (fs->iScanTE == JANEIN_J) {
				fs->eMode = DccGeplWeTeBilden;
				memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
				memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
				fs->dSaveMngWE = 0.0;
				SmaBuildTEBilden(dccw, scl, EANART13, 0);	
			} else {
				fs->eMode = DccGeplWeEan;
            	SmaBuildScanEanWeGepl(dccw,scl);
			}
			return 0;
		case DCCAPP_VK_LIEFBEW:
			iRv = _doLiefBewJob(dccw, scl);
			if (iRv < 0) {
			    StrCpyDestLen(acMsg,"ERROR: _doLiefBewJob failed!");
	            LogPrintf (GetFunkFac(fs), LT_ALERT, "%s", acMsg);
	            return -1;
			}
			return 0;
		case DCCAPP_VK_GEWERF:
			/* fallthrough */
			/* We read all edit-fields and build the next mask */
		case DCCAPP_VK_MISCHTE:
			/* fallthrough */
            /* We generate TEP as normal, but do not write it,
                instead we put it in the buffer and increment
				the buffer-count */
			break;
        default:
            StrCpyDestLen(acMsg,DCCMSG_FALSCHETASTE);
            SendError(dccw,scl, acMsg, DCC_ERROR,
                DCC_WRITE_LOG, GetFunkFac(fs));
            return -1;
    }

#ifdef _POSWALD_TEST
   dMng = GetDoubleFromRadioInput(dccw, scl,
             		efname(ef_AnzStueck), &lFlags, NULL);

	if (dMng > 999999999) {
	    StrCpyDestLen(acMsg,MlMsg("Eingegebene Menge zu Gr�� !"));
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_NO_LOG, GetFunkFac(fs));
		return -1;
	}
#endif /* _POSWALD_TEST */

   /* Get type of retour ware */
	if (fs->tWeak.weakRetWare == 1) {
		if (fs->tArt.artLgKz == 1) {
			eRetWareType = EMPTIES;
		} else if (fs->tWeap.weapPruefKz == 1) {
			eRetWareType = NOT_SELLABLE_OR_THM;
		} else if (fs->iMischCount == 0 &&
			(fs->tTep.tepMngs.Mng == 
				 fs->tTep.tepMngs.VeHeFa * 
				 fs->tTep.tepVps.HeTeFa)){
			eRetWareType = SELLABLE_GANZ;
		} else {
			eRetWareType = SELLABLE;
		}	
	} else {
		eRetWareType = NORMAL;
	}

	if (fs->iScanTE == JANEIN_N) {

		if (scl->cur_maskid != ID_tebildsernr) {
			//strcpy(fs->tTep.tepMId.Charge, dccmask_get_field(dccw, scl,
			//			efname(ef_Charge), &lFlags, NULL));
			strcpy(fs->tTep.tepMId.Charge, fs->acSaveChargeWE);			
			if (fs->tTep.tepFifoDatum < 0) {
				fs->tTep.tepFifoDatum = 0;
			}
		} else {
			/* Seriennummer-Artikel kann nicht Chargen/FIFO-pflichtig sein
			-*/
			memset(fs->tTep.tepMId.Charge, 0, sizeof(fs->tTep.tepMId.Charge));
			fs->tTep.tepFifoDatum = 0;
		}
        
        if (fs->iL2Erf == 1) { /* Umverpackung  LE2 */
        	fs->tTep.tepMngs.Mng = GetDoubleFromRadioInput(dccw, scl,
             						efname(ef_AnzStueck), &lFlags, NULL) *
             						fs->tTep.tepMngs.VeHeFa;
        	
        	fs->tTep.tepMngs.Mng += GetDoubleFromRadioInput(dccw, scl,
          							efname(ef_AnzHE), &lFlags, NULL) * 
          							fs->tTep.tepVps.HeL2Fa *
          							fs->tTep.tepMngs.VeHeFa;
        } else {
	        fs->tTep.tepMngs.Mng = GetDoubleFromRadioInput(dccw, scl,
             	efname(ef_AnzStueck), &lFlags, NULL);
        	
        	fs->tTep.tepMngs.Mng += GetDoubleFromRadioInput(dccw, scl,
          		efname(ef_AnzHE), &lFlags, NULL) * fs->tTep.tepMngs.VeHeFa;
        }
		if (scl->cur_maskid != ID_tebildsernr) {
			fs->tTep.tepMngs.Gew = GetDoubleFromRadioInput(dccw, scl,
					efname(ef_Gew), &lFlags, NULL);
		} else {
			fs->tTep.tepMngs.Gew = 0;
		}
	} else {
		strcpy(fs->tTep.tepTeId, dccmask_get_field(dccw, scl,
                    efname(ef_ScTeNoMu), &lFlags, NULL));

		if (scl->endkey != DCCAPP_VK_MISCHTE) {
			if (IsEmptyStrg(fs->tTep.tepTeId) != 0) {
			    StrCpyDestLen(acMsg,MlMsg("Bitte TE-Id eingeben"));
				SendError(dccw,scl, acMsg, DCC_ERROR,
					DCC_NO_LOG, GetFunkFac(fs));
				return(0);
			}
		
			if (CheckTeIdLenght(NULL, GetFunkFac(fs), 
		       					fs->tTep.tepTeId) < 0) {
			    StrCpyDestLen (acMsg, MlMsg ("TE-Id-L�nge nicht korrekt!"));
		        SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
		                        GetFunkFac(fs));
		        return 0;
			}
		}
	}
	/* Empties (leergut) article proccessing */
	if (EMPTIES == eRetWareType && 1 == fs->tArt.artLgMngVerw ) {
	    StrCpy(fs->TetId, fs->tArt.artTet.TetId);
		iRv = _searchTeForArt (NULL, &fs->tArt, &tTep, GetFunkFac(fs));
		if (iRv < 0) {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "ERROR: Empties process failed !");
						ErrorMsg(dccw, scl, iRv);
						StrCpyDestLen (acMsg, MlMsg ("Fehler beim Buchen des WE-Avises!"));
        	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                            	GetFunkFac(fs));			
		} else if (iRv == 0) {
			iRv = GeplWeBuchen(NULL, GetFunkFac(fs), fs, 
					&fs->tArt.artLagO, JANEIN_J, JANEIN_N);
		} else {
			iRv = DirektGeplWeBuchen (NULL, GetFunkFac(fs), fs, 
						&fs->tArt.artLagO, &tTep);
		}
		if (iRv >= 0 ) {
			if (iRv == 2) { /* Avis ist abgeschlossen */
				fs->iLastOk = JANEIN_N;
            	fs->eMode = DccGeplWe;
            	SmaBuildIndentifWeGepl(dccw,scl);
			} else {
				fs->iLastOk = JANEIN_J;
				fs->eMode = DccGeplWeEan;
				fs->iAnzTE ++;
				fs->iGebucht = JANEIN_J;
				SmaBuildScanEanWeGepl(dccw,scl);
			}
		} else if (iRv == ERROR_OLDERGOODS) {
		    StrCpyDestLen (acMsg, MlMsg ("Es gibt in der Reserve Ware mit �lterem "
						   "Fifo-Datum!"));
			SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
								GetFunkFac(fs));
		} else if (iRv == ERROR_UEBERLIEF) {
		    StrCpyDestLen (acMsg, MlMsg ("�berliefern nicht erlaubt!"));
        	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                            	GetFunkFac(fs));
		} else if (iRv == ERROR_WRONGTEID) {
		    StrCpyDestLen (acMsg, MlMsg ("TE-Id steht auf keiner WE-Position!"));
			SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
								GetFunkFac(fs));
		}else {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "ERROR: GeplWeBuchen failed !");
			ErrorMsg(dccw, scl, iRv);
			StrCpyDestLen (acMsg, MlMsg ("Fehler beim Buchen des WE-Avises!"));
        	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                            	GetFunkFac(fs));
    	}
		if (iRv >= 0 ) {
			/* Reset MischFlag
			-*/
			iRv = ResetMischFlag(dccw, scl, GetFunkFac(fs));
		}
    	return (iRv);
	}
	/* Proccess not sellable or thm retour ware */
	if (NOT_SELLABLE_OR_THM == eRetWareType) {
	    StrCpy(fs->TetId, fs->tArt.artTet.TetId);
		iRv = GeplWeBuchen(NULL, GetFunkFac(fs), fs, 
					NULL, JANEIN_N, JANEIN_N);
		if (iRv >= 0 ) {
			if (iRv == 2) { /* Avis ist abgeschlossen */
				fs->iLastOk = JANEIN_N;
            	fs->eMode = DccGeplWe;
            	SmaBuildIndentifWeGepl(dccw,scl);
			} else {
				fs->eMode = DccGeplWeEan;
				fs->iAnzTE ++;
				fs->iGebucht = JANEIN_J;
				fs->iLastOk = JANEIN_J;
				SmaBuildScanEanWeGepl(dccw,scl);
			}
		} else if (iRv == ERROR_OLDERGOODS) {
		    StrCpyDestLen (acMsg, MlMsg ("Es gibt in der Reserve Ware mit �lterem "
						   "Fifo-Datum!"));
			SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
								GetFunkFac(fs));
		} else if (iRv == ERROR_UEBERLIEF) {
		    StrCpyDestLen (acMsg, MlMsg ("�berliefern nicht erlaubt!"));
        	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                            	GetFunkFac(fs));
		} else if (iRv == ERROR_WRONGTEID) {
		    StrCpyDestLen (acMsg, MlMsg ("TE-Id steht auf keiner WE-Position!"));
			SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
								GetFunkFac(fs));
		}else {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "ERROR: GeplWeBuchen failed !");
			ErrorMsg(dccw, scl, iRv);
			StrCpyDestLen (acMsg, MlMsg ("Fehler beim Buchen des WE-Avises!"));
        	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                            	GetFunkFac(fs));
    	}
		if (iRv >= 0 ) {
			/* Reset MischFlag
			-*/
			iRv = ResetMischFlag(dccw, scl, GetFunkFac(fs));
		}
		return (iRv);
	}
	/* If artikel is Seriennummer Artikel */
	if (fs->tArt.artSerNrPflWE == JANEIN_J && fs->tWeap.weapPruefKz == 0) {
		iRv = _checkSerNr(dccw, scl);
		if (iRv < 0) {
			return -1;
		}
		if (fs->iScanTE == JANEIN_J && scl->endkey != DCCAPP_VK_MISCHTE) {
			return 0;
		}
	}
	/* Wenn eine Gewichtserfassung erw�nscht ist, so ist
	-* wird hier die neue Maske gebaut und abgebrochen
	-*/

	if (scl->endkey == DCCAPP_VK_GEWERF) {
		fs->tTep.tepMngs.Gew = 0;
		fs->iAnzGew = 0;
		SmaBuildSumGew(dccw,scl);
		return 0;
	}

	if (fs->tArt.artChargePfl == JANEIN_J &&
		IsEmptyStrg(fs->tTep.tepMId.Charge) == JANEIN_J) {

	    StrCpyDestLen (acMsg, MlMsg ("Bitte Charge angeben!"));
		SendErrorNoBack (dccw, scl,
						acMsg,
						DCC_ERROR,
						DCC_NO_LOG,
						GetFunkFac(fs));
		return (0);
	}

	if (fs->tArt.artFifoPfl == JANEIN_J) {

    	zMHD = GetWeDateFromRadioInput (dccw, scl, efname(ef_MHD),
										&lFlags, NULL, 0);
		if (zMHD < 100000) {

	        LogPrintf (GetFunkFac(fs), LT_ALERT, "no MHD from mask - try with format");

			zMHD = GetWeDateFromRadioInput (dccw, scl, efname(ef_MHD),
											&lFlags, NULL, 1);
			if (zMHD < 100000) {
			    StrCpyDestLen (acMsg, MlMsg ("Bitte MHD angeben!"));
				SendErrorNoBack (dccw, scl,
								acMsg,
								DCC_ERROR,
								DCC_NO_LOG,
                                GetFunkFac(fs));
				return (0);
			}
		} 

		fs->tTep.tepMHD = zMHD;
		fs->tTep.tepFifoDatum = fs->tTep.tepMHD;
			
	}

	iRv = AllDataForTep(&fs->tTep, &fs->tArt);
	if (iRv <= 0) {	
	    StrCpyDestLen (acMsg, MlMsg ("Nicht alle notwendigen Daten angegeben!"));
		SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
		return (0);
	}

	memset(acMsg, 0, sizeof(acMsg));

	/* Toleranzen-Check */
	if (fs->iScanTE == JANEIN_N) {
        if (RNDDOUBLE(&fs->tTep.tepMngs.Gew) == 0) {
            fs->lGewGanzTeKz = 0;
            CalcTEPWeight(&fs->tArt, &fs->tTep.tepMngs);
        } else {
            fs->lGewGanzTeKz = 1;
        }
		if (fs->iMischCount > 0) {
            fs->lGewGanzTeKz = 0;
        }

		/* Read WeMultiAvisErf */
		if (PrmGet1Parameter (NULL, P_WeMultiAvisErf,
                        PRM_CACHE, &iWeMultiAvisErf) != PRM_OK) {
        	iWeMultiAvisErf = 0;
    	}

		/* Ueberlieferungs-Check
		-*/
		if (fs->tArt.artCrossDocking == JANEIN_N || 
			fs->tWeap.weapBestMngs.Mng != 0) {
			if (fs->tWeak.weakUeberlief == JANEIN_N &&
				(fs->tWeap.weapLiMngsLls.Mng + fs->tTep.tepMngs.Mng) >
				 fs->tWeap.weapMaxErfMngs.Mng) {
			
				if (iWeMultiAvisErf == 1) {
					iRv = GetDiffErfLiMng (GetFunkFac(fs), fs, &dDiffErfLiMng);
					if (iRv < 0) {
					    StrCpyDestLen(acMsg, MlMsg("Fehler beim Lesen der Datenbank!"));
						SendError(dccw,scl, acMsg, DCC_ERROR,
							DCC_NO_LOG, GetFunkFac(fs));
						return -1;
					}
					if ((fs->tWeap.weapMaxErfMngs.Mng + dDiffErfLiMng) <
						(fs->tWeap.weapLiMngsLls.Mng + fs->tTep.tepMngs.Mng)) {

						fs->iMaskeJN = 1;
						StrCpyDestLen (acMsg, MlMsg ("�berliefern nicht erlaubt!"));
						SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR,
									DCC_NO_LOG, GetFunkFac(fs));
						return (0);
					}
				} else {

					fs->iMaskeJN = 1;
					StrCpyDestLen (acMsg, MlMsg ("�berliefern nicht erlaubt!"));
					SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
									GetFunkFac(fs));
					return (0);
				}
			}
		}
	
		iRv = CheckTEPWeight(NULL, GetFunkFac(fs), &fs->tTep,
                    	&fs->tTep.tepMngs, &fs->tArt,
                    	&dGewTolIst, &dGewTolSoll,
                    	&dGewTolMax, &dGewTolProz,
                    	&dMaxArtWeightFa, &dSollGew);
		switch(iRv) {
		case 0:  /* Alles Ok */
    		/* FALLTHROUGH */
		case 1:
    		break;
		case 2: /* Warnung */
		  StrCpy (acMsg, format("%s%s", acMsg, format(
						MlMsg ( "Das eingebene Gewicht �berschreitet "
							"die Toleranzgrenze (%g %%) dieses "
							"Artikels!"), 
						dGewTolProz)));
            iSendError = JANEIN_J;
            break;
		case 3: /* Abbruch */
			StrCpy (acMsg, format("%s%s", format(
						MlMsg ( "Das eingebene Gewicht �berschreitet "
						"die maximale Toleranzgrenze (%g %%) dieses "
						"Artikels!"), 
				dGewTolProz * dMaxArtWeightFa)));
            fs->iMaskeJN = 1;
            iSendError =  JANEIN_J;
            break;
		default: /* Error */
		    StrCpyDestLen(acMsg, GetErrmsg1());
			SendError(dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
			return (-1);
		}

		if (NOT_SELLABLE_OR_THM != eRetWareType &&
			EMPTIES != eRetWareType) {

			if (PrmGet1Parameter(NULL, P_RetWECheck,
								 PRM_CACHE, &lRetWECheck) != PRM_OK) {
				lRetWECheck = JANEIN_J;
			}
			if (eRetWareType == NORMAL_TPA_AUF_KORT ||
				eRetWareType == NORMAL ||
				lRetWECheck == JANEIN_J) {

				/* Restlaufzeit-Check */
				memset(acWeDefSpKz, 0, sizeof(acWeDefSpKz));
				iRv = IsFifoOk(NULL, GetFunkFac(fs), &fs->tTep, &fs->tArt, 
								GEPLANT, acWeDefSpKz, acRestlaufMatKzWE);

				switch(iRv) {
				case 0:  /* Warnung RLZ ueberschritten */
					StrCpy (acMsg, format( "%s%s", acMsg,
								MlMsg ( "TE-Sperre, weil Resthaltbarkeit "
									"nicht erf�llt!")));
					iSendError = JANEIN_J;
					break;
				case 1: /* RLZ ist OK */
					break;
				default: /* Error */
				    StrCpyDestLen(acMsg, GetErrmsg1());
					SendError(dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
											GetFunkFac(fs));
					return (-1);
				}
			}
		}

		/* Anmeldegewicht ungleich Liefergewicht ? */
        /* Menge abfragen */
        if ((fs->tWeap.weapLiMngsLls.Mng + fs->tTep.tepMngs.Mng) >=
                        fs->tWeap.weapAnmMngs.Mng) {
            /* Letzte TE */
            iLastTe = JANEIN_J;
        }

        if (PrmGet1Parameter (NULL, P_WeAnmWeightTol,
                    PRM_CACHE, &iWeAnmWeightTol) != PRM_OK) {
            iWeAnmWeightTol = 0;
        }

        /* If WeapStatus_fertig, warning message */
        if (fs->tArt.artGewKz == JANEIN_J &&
        	fs->tWeap.weapAnmMngs.Gew > 0) {

            lfTmpGew = fs->tWeap.weapLiMngsLls.Gew +
                        fs->tTep.tepMngs.Gew;
            RNDDOUBLE(&lfTmpGew);

            lfGewTol = fs->tWeap.weapAnmMngs.Gew*((double)iWeAnmWeightTol/100);
            RNDDOUBLE(&lfGewTol);

            lfTmpMinAnmGew = lfTmpGew - fs->tWeap.weapAnmMngs.Gew;
            RNDDOUBLE(&lfTmpMinAnmGew);

            lfAnmMinTmpGew = fs->tWeap.weapAnmMngs.Gew - lfTmpGew;
            RNDDOUBLE(&lfAnmMinTmpGew);

			if ( lfTmpMinAnmGew > lfGewTol ||
                    (iLastTe == JANEIN_J &&
                    lfAnmMinTmpGew > lfGewTol)) {
                /* ABBRUCH */

                StrCpy (acMsg, format("%s%s ",acMsg,
                        MlMsg ( "Es wurde die maximale Abweichung vom"
                                " Anmeldegewicht erreicht!")));
                fs->iMaskeJN = 1;
                iSendError = JANEIN_J;
            }

        }

        if (iSendError == JANEIN_J) {
            SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
            return (0);
        }
	}

	if (fs->iScanTE == JANEIN_J) {
		/* MischTe nicht erlaubt */

		iRvDb = TExecSql (NULL,
					"SELECT COUNT(*) FROM TEP "
					"WHERE TEP.TEID = :TeId ",
					SELLONG (lIsMischTe),
					SQLSTRING (fs->tTep.tepTeId),
					NULL);

		if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
			/* DB - Error */
		    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
			SendError(dccw,scl, acMsg, DCC_SQLERROR,
					DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}	
			
		if (lIsMischTe > 0) {
			/* check Tep */
			iRv = CheckTepPos (NULL, fs->tTep.tepTeId, &fs->tTep,
			                   &fs->tArt, &fs->tWeak.weakPosition,
			                   GetFunkFac(fs));		
			if (iRv <= 0) {
				/* error */
				switch (iRv)
				{		
					case TEP_ERROR_TEGA:
					    StrCpy (acMsg, "TeBo oder Gard Artikel");
						break;
					case TEP_ERROR_DB:
					    StrCpy (acMsg, "Fehler bei TE Check");
						break;

					case TEP_ERROR_TPA:
					    StrCpy (acMsg, "TPA hat den falschen Status");
						break;

					case TEP_ERROR_POS:	
					    StrCpy (acMsg,
								"TE-Id steht auf der falschen Position");	
						break;

					case TEP_ERROR_CHARGE:	
					    StrCpy (acMsg, "Charge stimmt nicht �berein");
						break;

					case TEP_ERROR_MHD:
					    StrCpy (acMsg, "MHD stimmt nicht �berein");
						break;

					case TEP_ERROR_ART:
					    StrCpy (acMsg, "Artikel stimmt nicht �berein");
						break;

					default:
					    StrCpy (acMsg, "Fehler");
				}
				
				StrCpy (acMsg, format( "%s TE-Id ist schon verplant!", acMsg ));
				SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
						GetFunkFac(fs));
				return 0;
			}	
		}	
		
		const char *pcTetId = dccmask_get_field(dccw, scl, efname(ef_TeTyp), &lFlags, NULL);
		char acTetId[TETID_LEN+1] = {0};

		if( pcTetId != NULL ) {
		    StrCpyDestLen(acTetId, pcTetId);
		}

		if (_getTetId(dccw, scl, acTetId, fs->iMischCount) < 0){
				return 0;
		}
		if (scl->endkey != DCCAPP_VK_MISCHTE) { /* Not Misch-TE-Key */
			
			/* MischTe max 100 Positionen */
            iRvDb = TExecSql (NULL, "SELECT COUNT(*) FROM TEP "
                                    "WHERE TEP.TEID = :TeId",
                                    SELLONG(lCountTep),
                                    SQLSTRING(fs->tTep.tepTeId),
                                    NULL);

            if (iRvDb <= 0 && TSqlError(NULL) != SqlNotFound) {
                /* Db Error */
                StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
                SendError(dccw,scl, acMsg, DCC_SQLERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
                return -1;
            }

            if (lCountTep >= 100) {
                StrCpyDestLen (acMsg,
                        MlMsg ("Auf einer MischTe d�rfen maximal\n"
                               "100 Positionen erfast werden!"));
                SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                    GetFunkFac(fs));
                return 0;
            }	
			
			/* Check if this is Misch-TE and there
            -* is no Kort for LAST Article
            */
            if (fs->iMischCount > 0) {
				if (PrmGet1Parameter (NULL, P_LpOhneKpl, PRM_CACHE,
						&lLpOhneKpl) == PRM_OK && lLpOhneKpl == JANEIN_J) {

					if(1 != IsThereAKort(NULL,
							&fs->tTep.tepMId.AId,
							fs->tTep.tepMId.MatKz,
							"DCCWEMASK")) {

						/* Set flag for use in error treatment
						-*  (reset is done there)
						*/
						fs->iArtOhneKort = 1;

						fs->iMaskeJN = 1;
						StrCpyDestLen (acMsg,
							MlMsg ("Artikel ohne Kommissionierplatz darf\n"
									"nicht als Misch-Palette erfasst werden!"));
						SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
										GetFunkFac(fs));
						return 0;
					}
				}
            }

			/* TE anlegen (TEId check allready done) */
			iRv = GeplWeBuchen (NULL, GetFunkFac(fs), fs, 
							NULL, JANEIN_N, JANEIN_N);
			if (iRv >= 0 ) {
				if (iRv == 2) { /* Avis ist abgeschlossen */
					fs->iLastOk = JANEIN_N;
                	fs->eMode = DccGeplWe;
                	SmaBuildIndentifWeGepl(dccw,scl);
				} else {
					fs->eMode = DccGeplWeEan;
					fs->iAnzTE ++;
					fs->iGebucht = JANEIN_J;
					fs->iLastOk = JANEIN_J;
					SmaBuildScanEanWeGepl(dccw,scl);
				}
			} else if (iRv == ERROR_OLDERGOODS) {
			    StrCpyDestLen (acMsg,
						MlMsg ("Es gibt in der Reserve Ware mit �lterem "
							   "Fifo-Datum!"));
				SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
									GetFunkFac(fs));
			} else if (iRv == ERROR_UEBERLIEF) {
			    StrCpyDestLen (acMsg,
                    	MlMsg ("�berliefern nicht erlaubt!"));
            	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
			} else if (iRv == ERROR_WRONGTEID) {
			    StrCpyDestLen (acMsg,
						MlMsg ("TE-Id steht auf keiner WE-Position!"));
				SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
									GetFunkFac(fs));
			} else {
				ErrorMsg(dccw, scl, iRv);
				StrCpyDestLen (acMsg,
                    	MlMsg ("Fehler beim Buchen des WE-Avises!"));
            	SendError (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                	GetFunkFac(fs));
        	}
			if (iRv >= 0 ) {
				/* Reset MischFlag
				-*/
				if (ResetMischFlag(dccw, scl, GetFunkFac(fs)) < 0) {
					return -1;
				}
			}
        	return (iRv);	
		} else {
			/* F�r Schlau keine Mischpaletten */
		    StrCpyDestLen (acMsg,
				       MlMsg ("Es d�rfen keine MischTE angelegt werden!"));
			SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
					                    GetFunkFac(fs));
			return 0;
			/* MischTe max 100 Positionen */
            if (fs->iMischCount > 99) {
                StrCpyDestLen (acMsg,
                        MlMsg ("Auf einer MischTe d�rfen maximal\n"
                               "100 Positionen erfast werden!"));
                SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
                                GetFunkFac(fs));
                return 0;
            }

            /* Misch-TE anlegen */
            if (fs->iMischCount == 0) {
                if ((fs->lDummyNr = GetDummyNr(NULL, GetFunkFac(fs))) < 0) {
                    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
                    SendError(dccw,scl, acMsg, DCC_SQLERROR,
                            DCC_WRITE_LOG, GetFunkFac(fs));
                    return -1;
                }
            }

			/* Misch-TE anlegen */
            if (fs->iMischCount == 0) {
            	if ((fs->lDummyNr = GetDummyNr(NULL, GetFunkFac(fs))) < 0) {
            	    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
                	SendError(dccw,scl, acMsg, DCC_SQLERROR,
                        	DCC_WRITE_LOG, GetFunkFac(fs));
                	return -1;
            	}
        	}
        	if (InsertNewTepPosNrIntoDummy(NULL, GetFunkFac(fs),
                                  		&fs->tTep, &fs->tWeap.weapPosNr, 
                                  		&fs->tWeap.weapWeaId,
										fs->lDummyNr) < 0) {

        	    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
            	SendError(dccw,scl, acMsg, DCC_SQLERROR,
                    	DCC_WRITE_LOG, GetFunkFac(fs));
            	return -1;
        	}

			fs->eMode = DccGeplWeEan;
			fs->iGebucht = JANEIN_N;

			if (PrmGet1Parameter (NULL, P_LpOhneKpl, PRM_CACHE,
					&lLpOhneKpl) == PRM_OK && lLpOhneKpl == JANEIN_J) {
				/* When is Misch-TE, check if there is a Kort
				*/
				if(1 != IsThereAKort(NULL,
								&fs->tTep.tepMId.AId,
								fs->tTep.tepMId.MatKz,
								"DCCWEMASK")) {

					/* Set flag for use in error treatment 
					-*	(reset is done there)
					*/
					fs->iArtOhneKort = 1;
					
					fs->iMaskeJN = 1;
					StrCpyDestLen (acMsg,
						MlMsg ("Artikel ohne Kommissionierplatz darf\n"
								"nicht als Misch-Palette erfasst werden!"));
					SendErrorNoBack (dccw, scl, acMsg, DCC_ERROR, DCC_NO_LOG,
									GetFunkFac(fs));
					return 0;
				}
			}

			fs->iMischCount++;
            SmaBuildScanEanWeGepl(dccw,scl);
			return 0;
        }
	} else {
		fs->eMode = DccGeplWeTeBilden;
		memset(fs->acSaveChargeWE, 0, sizeof(fs->acSaveChargeWE));		
		memset(fs->acSaveTeIdWE, 0, sizeof(fs->acSaveTeIdWE));
		fs->dSaveMngWE = 0.0;
		SmaBuildTEBilden(dccw, scl, EANART128, 1);	
	}
	return 0;
}


int ResetMischFlag(dccw_context *dccw,dccclient_stm *scl, char *pcFac)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char            acMsg[128+1];

	memset(acMsg, 0, sizeof(acMsg));

	if (DeleteAllOfDummyNr(NULL, pcFac, fs->lDummyNr) < 0) {
	    StrCpyDestLen(acMsg, DCCMSG_ERRORDBREAD);
		SendError(dccw,scl, acMsg, DCC_ERROR,
			DCC_WRITE_LOG, pcFac);
		return(-1);
	}
	fs->iMischCount = 0;
	fs->iAllInfoDisplayed = 0;

	return 0;

}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function builds the mask for Schlau Crossdocking - WEA Selection
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
void SmaBuildScdAvisAus(dccw_context *dccw,dccclient_stm *scl)
{
	/*FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);*/
	char acLineBuf[2][40];

	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg("    Avis-Auswahl    ")));
	StrCpyDestLen(acLineBuf[1],MlMsg("TE"));
	
	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scdAvisAus,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_LabTe),acLineBuf[1],-1,
		0);

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function executes the mask for Schlau Crossdocking - WEA Selection
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
int SmaScdAvisAus(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	int iRv = 0;
	long lFlags = 0;
	char acTeId[TEID_LEN+1];
	char acTeId2[TEID_LEN+1];

	switch(scl->endkey){
		case DCCAPP_VK_ENTER: /* Fallthrough */
		case DCCKEY_SCAN_N: /* Fallthrough */
		case DCCAPP_VK_SCAN:
			memset(acTeId, 0, sizeof(acTeId));
			strcpy(acTeId, dccmask_get_field(dccw, scl,
				efname(ef_ScanTeId), &lFlags, NULL));
			if (IsEmptyStrg(acTeId) == 1) {
				/*error, TeId is empty string*/
				SendError (dccw, scl, 
					MlMsg ("Fehler beim einlesen des TeId!"),
					DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
				return -1;
			}
	
			memset(acTeId2, 0, sizeof(acTeId2));
	
			/* Try to find VerdTeId */
			iRv = TExecSql(NULL, 
						"SELECT ZielTeId "
						"FROM VPLP "
						"WHERE VerdTeId = :TeId ",
						SELSTR(acTeId2, TEID_LEN+1),
						SQLSTRING(acTeId),
						NULL);
			
			if (iRv <= 0 && TSqlError(NULL) != SqlNotFound) {
				SendError (dccw, scl, DCCMSG_ERRORDBREAD, DCC_SQLERROR,
								DCC_WRITE_LOG, GetFunkFac(fs));
				return -1;
			}

			if (iRv > 0) {
			    /* VerdTeId */
				strcpy(acTeId, acTeId2);
			}
		
			iRv = ScdFindWeak(NULL, acTeId, &fs->tWeak);
			if (iRv < 1) {
				SendError (dccw, scl,
					MlMsg ("Fehler, TE ist nicht im Crossdocking!"),
					DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
				return -1;
			}
		
			return SmaBuildScdErfass(dccw, scl);
			
		case DCCAPP_VK_ESC:
			SmaBuildWeMainMenu(dccw, scl);
			break;
		default:
			SendError(dccw,scl, DCCMSG_FALSCHETASTE, DCC_ERROR,
				DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
	}
	
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function builds the mask for Schlau Crossdocking - Stage 1
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
int SmaBuildScdErfass(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char			acLineBuf[8][40];
	int				iRv = 0;

	iRv = ScdCountOpenWeap(NULL, &fs->tWeak.weakWeaId);
	if (iRv < 0) {
		SendError (dccw, scl, 
			MlMsg ("Fehler beim ermitteln der offenen postitionen!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
	
	memset(acLineBuf, 0, sizeof(acLineBuf));

	StrCpy(acLineBuf[0],format("%20s",MlMsg(" Crossdocking erf. ")));
	StrCpyDestLen(acLineBuf[1],MlMsg("WeaNr"));
	StrCpy(acLineBuf[2],fs->tWeak.weakWeaId.WeaNr);
	StrCpyDestLen(acLineBuf[3],MlMsg("CTour"));
	StrCpy(acLineBuf[4],fs->tWeak.weakCrossdockTourId.Tour);
	StrCpyDestLen(acLineBuf[5],MlMsg("Offene Pos"));
	StrCpyDestLen(acLineBuf[6],format("%d",iRv));
	StrCpyDestLen(acLineBuf[7],MlMsg("TE"));

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scdErfass,
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_LabMand),acLineBuf[1],-1,
		DparEfContents,efname(ef_Mand),acLineBuf[2],-1,
		DparEfContents,efname(ef_LabTour),acLineBuf[3],-1,
		DparEfContents,efname(ef_Tour),acLineBuf[4],-1,
		DparEfContents,efname(ef_LabPos),acLineBuf[5],-1,
		DparEfContents,efname(ef_KuNr),acLineBuf[6],-1,
		DparEfContents,efname(ef_LabTe),acLineBuf[7],-1,
		0);

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function executes Schlau Crossdocking - Stage 1
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
static int ScdErfass(dccw_context *dccw, dccclient_stm *scl, 
				char *pcTeId, AUSK *ptAusk, AUSP *ptAusp) {
				
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	int iRv = 0;
	POS tPos;
			
	memset(&tPos, 0, sizeof(tPos));

	iRv = ScdFindWeap(NULL, &fs->tWeak.weakWeaId, 
		pcTeId, &fs->tWeap);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen der WEAP!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
	
	iRv = ScdFindAus(NULL, pcTeId, ptAusk, ptAusp);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen des AUS!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
	
	iRv = ScdGetTourRamp(NULL, &ptAusk->auskTourId, &tPos);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen der Rampe!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
	
	if (IsEmptyStrg(tPos.FeldId) == 1) {
		SendError (dccw, scl,
			MlMsg ("Fehler, keine Rampe eingegeben!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}

	return 0;
}	


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function executes the mask for Schlau Crossdocking - Stage 1
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
int SmaScdErfass(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	int 		iRv = 0;
	long 		lFlags = 0;
	char 		acTeId[TEID_LEN+1];
	SqlContext  *ptSqlCtx = NULL;
	int 		iDbRv = 0;
	char 		aacTeId[BLOCKSIZE][TEID_LEN+1];
	int			iFirst = 0;
	int			nI;
	AUSK		tAusk;
	AUSP 		tAusp;

	switch(scl->endkey){
	case DCCAPP_VK_ENTER: /* Fallthrough */
	case DCCKEY_SCAN_N: /* Fallthrough */
	case DCCAPP_VK_SCAN:
		memset(&tAusk, 0, sizeof(tAusk));
		memset(&tAusp, 0, sizeof(tAusp));
		memset(acTeId, 0, sizeof(acTeId));
		
		strcpy(acTeId, dccmask_get_field(dccw, scl,
			efname(ef_ScanTeId), &lFlags, NULL));
		if (IsEmptyStrg(acTeId) == 1) {
			/*error, TeId is empty string*/
			SendError (dccw, scl, 
				MlMsg ("Fehler beim einlesen des TeId!"),
				DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
				return -1;
		}
		
		/* Reset TE-Id, a Value in this Var signs a 
		   verdTeId */
		memset(fs->acVerKontrTeId, 0, 
				sizeof(fs->acVerKontrTeId));


		/* Try to Find VerdTeId */
		/* Get SqlContext */
		ptSqlCtx = TSqlNewContext(NULL, NULL);
		if (ptSqlCtx == NULL) {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "TSqlNewContext: %s", 
				TSqlErrTxt(NULL));
			SendError (dccw, scl,
				MlMsg ("Datenbank-Fehler!"),
				DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}

		/* Read from DB */
		do {
			memset(aacTeId, 0, sizeof(aacTeId));

			iDbRv = (iFirst == 0)
                ? TExecSqlX (NULL, ptSqlCtx,
                                "SELECT DISTINCT ZielTeId "
								"FROM VPLP "
                                "WHERE VerdTeId = :TeId ",
                                BLOCKSIZE, 0,
                                SELSTR(aacTeId, TEID_LEN+1),
                                SQLSTRING(acTeId),
                                NULL)
                : TExecSqlV (NULL, ptSqlCtx, NULL, NULL, NULL, NULL);
			
			if (iDbRv <= 0 && TSqlError(NULL) != SqlNotFound) {
				SendError (dccw, scl, DCCMSG_ERRORDBREAD, DCC_SQLERROR,
								DCC_WRITE_LOG, GetFunkFac(fs));
				TSqlDestroyContext (NULL,ptSqlCtx);
				return -1;
			}

			if (iDbRv > 0) {
			    /* VerdTeId */
				strcpy(fs->acVerKontrTeId, acTeId);
			}

			if (iDbRv < 0 && iFirst == 0) {
				/* Not a VerdTeId */
				iDbRv = 1;
				strcpy(aacTeId[0], acTeId);
			}

			for (nI = 0; nI < iDbRv; nI++) {
				iRv = ScdErfass(dccw, scl, aacTeId[nI],
				      &tAusk, &tAusp);
				if (iRv < 0) {
					TSqlDestroyContext (NULL,ptSqlCtx);
				    return iRv;
				}	
			}

			iFirst = 1;
		
		} while (iDbRv == BLOCKSIZE);

		TSqlDestroyContext (NULL,ptSqlCtx);
	
		break;
	case DCCAPP_VK_ESC:
		SmaBuildScdAvisAus(dccw, scl);
		return 1;
	default:
		SendError(dccw,scl, DCCMSG_FALSCHETASTE, DCC_ERROR,
			DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
			
	SmaBuildScdErfass2(dccw, scl, &tAusk, &tAusp);
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function builds the mask for Schlau Crossdocking - Stage 2
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
int SmaBuildScdErfass2(dccw_context *dccw,dccclient_stm *scl, 
	AUSK *ptAusk, AUSP *ptAusp)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	char	acLineBuf[15][40];
	char	acArtBez[ARTBEZ_LEN];
	char	acPos[256];
	int     iBereichJN = 1;
	app_msk_par     *MskParData = NULL;
	int 	iVerd = (!IsEmptyStrg(fs->acVerKontrTeId));

	if (iVerd) {
	    strcpy(acArtBez, MlMsg("Verdichtete TE"));
	} else {	
		if (ScdGetArtBez(NULL, &ptAusp->auspMId.AId, acArtBez) < 0) {
			SendError (dccw, scl, 
				MlMsg ("Fehler beim lesen der ArtBez!"),
				DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
	}

	memset(acLineBuf, 0, sizeof(acLineBuf));
	Pos2Str(NULL, &ptAusk->auskZiel, acPos);
	StrCpy(acLineBuf[0],format("%20s\0",MlMsg(" Crossdocking erf. ")));
	StrCpy(acLineBuf[1],acArtBez);
	if (iVerd == 0) {
		StrCpyDestLen(acLineBuf[2],MlMsg("Menge"));
		StrCpy(acLineBuf[3],format("%12.2f",fs->tWeap.weapBestMngs.Mng));
	}
	
	StrCpyDestLen(acLineBuf[4],MlMsg("TE"));
	
	if (iVerd) {
	    StrCpy(acLineBuf[5],fs->acVerKontrTeId);
	} else {	
	    StrCpy(acLineBuf[5],fs->tWeap.weapTeId);
	}

	StrCpyDestLen(acLineBuf[6],MlMsg("Tour/L"));
	StrCpy(acLineBuf[7],format("%s/%d",ptAusk->auskTourId.Tour, ptAusk->auskLadeRf));
	StrCpyDestLen(acLineBuf[8],MlMsg("POS"));
	StrCpyDestLen(acLineBuf[9],acPos);
	StrCpyDestLen(acLineBuf[10],MlMsg("POS"));
	StrCpyDestLen(acLineBuf[11],MlMsg("B"));


	if (PrmGet1Parameter (NULL, P_BereichJN,
		PRM_CACHE, &iBereichJN) != PRM_OK) {
		iBereichJN = 1;
    }

	/* SCHLAU: Bereich field in Abgabe */
    if (iBereichJN == 1){

        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrClr,
                                    efname (ef_Bereich),
                                    (long )DCCEF_ATTR_INACTIVE,
                                    NULL);
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrClr,
                                    efname (ef_Bereich),
                                    (long )DCCEF_ATTR_INVISIBLE,
                                    NULL);
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrClr,
                                    efname (ef_Info4),
                                    (long )DCCEF_ATTR_INVISIBLE,
                                    NULL);

    } else {
        /* Default Bereich value is 0 */
        strcpy (acLineBuf[12], "0");
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrSet,
                                    efname (ef_Bereich),
                                    (long )DCCEF_ATTR_INACTIVE,
                                    NULL);
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrSet,
                                    efname (ef_Bereich),
                                    (long )DCCEF_ATTR_INVISIBLE,
                                    NULL);
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrSet,
                                    efname (ef_Info4),
                                    (long )DCCEF_ATTR_INVISIBLE,
                                    NULL);
        MskParData = fs_put_Maskparam  (MskParData, LparEfAttrClr,
                                    efname (ef_Bereich),
                                    (long )DCCEF_ATTR_MUST,
                                    NULL);
    }

	FUNKLOG(0);dccacVA_start_mask(dccw,scl,ID_scdErfass2,
		XparLinkedDataList, MskParData,  /* Parameter setzen */
		DparEfContents,efname(ef_Title),acLineBuf[0],-1,
		DparEfContents,efname(ef_ArtBez),acLineBuf[1],-1,
		DparEfContents,efname(ef_Info1),acLineBuf[2],-1,
		DparEfContents,efname(ef_Info2),acLineBuf[3],-1,
		DparEfContents,efname(ef_LabTe),acLineBuf[4],-1,
		DparEfContents,efname(ef_ScanTeId),acLineBuf[5],-1,
		DparEfContents,efname(ef_LabTour),acLineBuf[6],-1,
		DparEfContents,efname(ef_Tour),acLineBuf[7],-1,
		DparEfContents,efname(ef_LabPos),acLineBuf[8],-1,
		DparEfContents,efname(ef_Info3),acLineBuf[9],-1,
		DparEfContents,efname(ef_Info5),acLineBuf[10],-1,
		DparEfContents,efname(ef_Info4),acLineBuf[11],-1,
		DparEfContents,efname(ef_Bereich),acLineBuf[12],-1,
		0);

	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function executes Schlau Crossdocking - Stage 2
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
static int ScdErfass2(dccw_context *dccw,dccclient_stm *scl,
					char *pcTeId, int iBereich,
					char *pcNewPos) {

	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	AUSK 			tAusk;
	AUSP 			tAusp;
	int 			iRv = 0;
	POS 			tPos;
	char 			acOldPos[255];
	char    		acTeIdTmp[TEID_LEN+1];

	memset(&tAusk, 0, sizeof(tAusk));
	memset(&tAusp, 0, sizeof(tAusp));
	memset(&tPos, 0, sizeof(tPos));
	memset(&acOldPos, 0, sizeof(acOldPos));
	
	memset(acTeIdTmp, 0, sizeof(acTeIdTmp));
		
	/* Required, because ScdFindWeap would overwrite TeId */
	strcpy(acTeIdTmp, pcTeId);
	/* Find Weap */
	iRv = ScdFindWeap(NULL, &fs->tWeak.weakWeaId, 
		acTeIdTmp, &fs->tWeap);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen der WEAP!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}

	/* Find Aus */
	iRv = ScdFindAus(NULL, fs->tWeap.weapTeId, 
		&tAusk, &tAusp);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen des AUS!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
	
	/* Get Ramp */
	iRv = ScdGetTourRamp(NULL, &tAusk.auskTourId, &tPos);
	if (iRv < 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler beim lesen der Rampe!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
	}
			
	if (IsEmptyStrg(tPos.FeldId) == 1) {
		SendError (dccw, scl,
			MlMsg ("Fehler, keine Rampe eingegeben!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
		return -1;
	}

	/* Compare Positions */
	Pos2Str(NULL, &tAusk.auskZiel, acOldPos);
			
	if (strcmp(acOldPos, pcNewPos) != 0) {
		SendError (dccw, scl,
			MlMsg ("Fehler, falsches feld!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
		return -1;
	}
		
	/* Confirm Wea */ 	
	iRv = ScdConfirmWea(NULL, &fs->tWeak, &fs->tWeap, 
			&tAusk, iBereich, fs->acPersNr);
	if (iRv<0) {
		SendError (dccw, scl,
			MlMsg ("WEA Fehler!"),
			DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
		return -1;
	
	} 
	
	if (iRv == 1) {
	    /* This was the Last WEA-Position */
		return 1;
	}

	return 0;
}					

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*  Function executes the mask for Schlau Crossdocking - Stage 2
-* RETURNS
-*  -
-*--------------------------------------------------------------------------*/
int SmaScdErfass2(dccw_context *dccw,dccclient_stm *scl)
{
	FS_Job_Context  *fs = (FS_Job_Context*) get_job_from_scl(scl);
	int 		iRv = 0;
	long 		lFlags = 0;
	int 		iBereich;
	int			iVerd = (!IsEmptyStrg(fs->acVerKontrTeId));
	char 		acNewPos[255];
	SqlContext  *ptSqlCtx = NULL;
	int 		iDbRv = 0;
	char 		aacTeId[BLOCKSIZE][TEID_LEN+1];
	int			iFirst = 0;
	int			nI;
	
	
	switch(scl->endkey){
	case DCCAPP_VK_ENTER: /* Fallthrough */
	case DCCKEY_SCAN_N: /* Fallthrough */
	case DCCAPP_VK_SCAN:
		iBereich = atoi (dccmask_get_field (dccw, scl,
											   efname (ef_Bereich),
											   &lFlags, NULL));

		if ((iBereich < 0) || (iBereich > 99)){
			SendError (dccw, scl, MlMsg ("Falsche Bereich eingegeben!"),
						DCC_ERROR, DCC_NO_LOG, GetFunkFac(fs));
			return -1;
		}
		
		memset(&acNewPos, 0, sizeof(acNewPos));

		strcpy(acNewPos, dccmask_get_field(dccw, scl,
			efname(ef_Scanpos), &lFlags, NULL));


		if (!iVerd) {
			/* No VerdTeId, normal TE */
			iRv = ScdErfass2(dccw, scl, fs->tWeap.weapTeId, iBereich,
				  acNewPos); 

			if (iRv < 0) {
			    return -1;
			}	

			if (iRv == 1) {
				/* Was last position */
				SmaBuildScdAvisAus(dccw, scl);
				return 1;
			}

			break;
		}

		/*  VerdTeId */
		
		/* Get SqlContext */
		ptSqlCtx = TSqlNewContext(NULL, NULL);
		if (ptSqlCtx == NULL) {
			LogPrintf(GetFunkFac(fs), LT_ALERT, "TSqlNewContext: %s", 
				TSqlErrTxt(NULL));
			SendError (dccw, scl,
				MlMsg ("Datenbank-Fehler!"),
				DCC_ERROR, DCC_WRITE_LOG, GetFunkFac(fs));
			return -1;
		}

		/* Read from DB */
		do {
			memset(aacTeId, 0, sizeof(aacTeId));

			iDbRv = (iFirst == 0)
                ? TExecSqlX (NULL, ptSqlCtx,
                                "SELECT DISTINCT ZielTeId "
								"FROM VPLP "
                                "WHERE VerdTeId = :TeId ",
                                BLOCKSIZE, 0,
                                SELSTR(aacTeId, TEID_LEN+1),
                                SQLSTRING(fs->acVerKontrTeId),
                                NULL)
                : TExecSqlV (NULL, ptSqlCtx, NULL, NULL, NULL, NULL);
			
			if (iDbRv <= 0 && 
					(TSqlError(NULL) != SqlNotFound ||
					iFirst == 0)) {
				SendError (dccw, scl, DCCMSG_ERRORDBREAD, DCC_SQLERROR,
								DCC_WRITE_LOG, GetFunkFac(fs));
				TSqlDestroyContext (NULL,ptSqlCtx);
				return -1;
			}

			for (nI = 0; nI < iDbRv; nI++) {
				iRv = ScdErfass2(dccw, scl, aacTeId[nI],
				      iBereich, acNewPos);
				if (iRv < 0) {
					TSqlDestroyContext (NULL,ptSqlCtx);
				    return iRv;
				}
			}

			iFirst = 1;
		
		} while (iDbRv == BLOCKSIZE);

		TSqlDestroyContext (NULL,ptSqlCtx);

		if (iRv == 1) {
			/* WEA Completed */
			SmaBuildScdAvisAus(dccw, scl);
			return 1;
		}

		break;
	case DCCAPP_VK_ESC:
		return SmaBuildScdErfass(dccw, scl);
	default:
		SendError(dccw,scl, DCCMSG_FALSCHETASTE, DCC_ERROR,
			DCC_WRITE_LOG, GetFunkFac(fs));
		return -1;
	}
			
	return SmaBuildScdErfass(dccw, scl);
}
