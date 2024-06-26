/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include <stdio.h>
#include <string.h>
#include <time.h>
#ifndef WIN32
#include <sys/param.h>
#endif

/* ------- Owil-Headers --------------------------------------------------- */
#include <owil.h>
#include <module/owss.h>
#include <efcbm.h>
#include <owrcloader.h>
#include <module/owgr.h>

/* ------- Tools-Headers -------------------------------------------------- */
#include <dbsqlstd.h>
#include <hist_util.h>
#include <t_util.h>
#include <ml.h>
#include <mlpack.h>
#include <makefilter.h>
#include <logtool.h>
#include <sqldef.h>
#include <logtool.h>
#include <sqlkey.h>
#include <efcbm.h>
#include <cycle.h>
#include <disperr.h>
#include <wamaswdg.h>
/* ------- Local-Headers -------------------------------------------------- */
#include "primanme.h"
#include "primandb.h"
#include "primanut.h"
#include "reallocx.h"

#include "menufct.h"
#include "defs.h"
#include "facility.h"
#include "callback.h"
#include "term_util.h"
#include "init_table.h"
#include "parameter.h"
#include "prm_util.h"
#include "mlm_util.h"
#include "vpl.h"
#include "te_vpl.h"
#include "tek.h"
#include "te_tek.h"
#include "tpa.h"
#include "vorlaz.h"
#include "te_tpa.h"
#include "pos_util.h"
#include "prn_util.h"
#include "vpl_util.h"
#include "te_util.h"
#include "list_util.h"
#include "etik_util.h"
#include "ket_util.h"
#include "status_handling.h"
#include "me_nachdruck.h"
#include "me_buchkom.h"
#include "me_listtelabel.h"
#include "me_vplueb.h"
#include "dbwrite.h"
#include "barcint.h"
#include "me_vplueb_util.h"
#include "callback_util.h"
#include "radparam.h"
#include "me_printer.h"
#include "me_vplueb_print.h"
#include "me_komanab.h"
#include "hotkey_util.h"
#include "komm_util.h"


#define MD_BLOCKSIZE	300
#define RC_NAME         "ME_KOMANAB"
#define	STANDORT_LEN	100
#define STMT_LEN        2548


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
    Funktion zum Drucken eines HE/TE-Labels.
-* RETURNS
-*--------------------------------------------------------------------------*/
static int ListKomPrintTELabel (MskDialog hMaskRl, 
								char 			*pacFac, 
								VPLK 			*ptVplk,
								PRIMANDOPRINT 	*ptPrnPrn,
								char 			*pacUser,
								long			lCount)
{
    NACHDRUCK_CTX     tNachDruckCtx;
    long            iRv = 0;
    char            acFilename[L_tmpnam + 1];
    char            acTeId[TEID_LEN+1], acTetId[TETID_LEN+1];

    memset (acFilename, 	0, sizeof (acFilename));
    memset (acTetId, 		0, sizeof (acTetId));
	memset (acTeId, 		0, sizeof (acTeId));
    memset (&tNachDruckCtx, 0, sizeof (NACHDRUCK_CTX));

    tmpnam(acFilename);

    tNachDruckCtx.lLabelType = 3; /* TE-Begleitschein */
    tNachDruckCtx.lBegEnd = LABEL_END;


	if ((iRv = GetTeId (hMaskRl, acTeId, pacFac)) < 0) {

		WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,  WBOX_ALERT,
					WboxNbutton,   WboxbNok,
					WboxNmwmTitle, MlM("Listenkommission"),
					WboxNtext,
					 MlM("Fehler beim Anlegen der TE!"),
					NULL);

		LogPrintf (pacFac, LT_ALERT,
				"ListKomPrintTELabel, KsNr: %ld, GetTeId failed !!!",
				ptVplk->vplkKsNr);

		TSqlRollback (hMaskRl);

		return (-1);
	}

	if (IsEmptyStrg (ptVplk->vplkTetId) == JANEIN_N) {

		strcpy (acTetId, ptVplk->vplkTetId);

	} else {

  		PrmGet1Parameter (hMaskRl, P_DefaultKommThm,
            PRM_CACHE, acTetId);

			if (IsEmptyStrg (acTetId) == JANEIN_J){
				strcpy (acTetId, TET_UNDEF);
		}
	}

	if ((iRv = InsertTekIfNotInDb (hMaskRl, 
									pacFac, 
									acTeId, 
									acTetId, 
									ptVplk,
									pacUser)) < 0) {

		LogPrintf (pacFac, LT_ALERT,
				"ListKomPrintTELabel, KsNr: %ld,"
				" InsertTekIfNotInDb failed !!!",
				ptVplk->vplkKsNr);

		TSqlRollback (hMaskRl);

        return (-1);
    }
	
    strncpy (tNachDruckCtx.tVplp.vplpZielTeId, acTeId, TEID_LEN);
    tNachDruckCtx.tVplp.vplpZielTeId[TEID_LEN] = '\0';
    tNachDruckCtx.tVplk.vplkKsNr = ptVplk->vplkKsNr;
    tNachDruckCtx.lAnzColliLab = 1;

    if ((iRv = _GetLabelSrc ((MskTmaskRlPtr) hMaskRl, 
										&tNachDruckCtx,
										ptPrnPrn, 
										acFilename,
										lCount)) < 0) {
		LogPrintf (pacFac, LT_ALERT,
				"ListKomPrintTELabel, KsNr: %ld, TeId: %s,"
				" _GetLabelSrc failed !!!",
				ptVplk->vplkKsNr, 
				tNachDruckCtx.tVplp.vplpZielTeId);

        return (-1);
    }

    return (0);

} /* ListKomPrintTELabel () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int KommAbMelden (MskDialog hMaskRl, 
					char 		*pacFac, 
					VPLK 		*ptVplk,
					KOMANABCTX 	*ptKomAnAbCtx,
					char		*pacUser)
{
	VPLK			tVplk, tVplkTmp;
    PRIMANDOPRINT   *ptPrnPrn;
	char            acStandOrt[STANDORT_LEN+1];
	time_t			zDiff=0;
	long			lCntTek = 0;
	int				iRv = 0, nI = 0, iWert = 0, iSetLastPos = JANEIN_N;
	int				iWaitTime = 0;
	ArrDesc         tArrDescKetVplk;
    char            acGrpKsNr[256];
	long            lCount = 0;

	if (ptVplk == NULL) {

		LogPrintf (pacFac, LT_ALERT,
				"KommAbMelden, ptVplk == NULL !!!");

		return (-1);

	}
	memset (&tVplk, 0, sizeof (VPLK));

	tVplk = *ptVplk;

	if (ReadLockMyVplk (hMaskRl, GetTermFac(), &tVplk) <= 0) {

        LogPrintf (pacFac, LT_ALERT, 
					"KommAbMelden, KsNr: %ld, ReadLockMyVplk failed !!!",
					tVplk.vplkKsNr);

        return(-1);
    }

	/* Check wait time */
    if (PrmGet1Parameter (NULL, P_WaitBookList,
                            PRM_CACHE, &iWaitTime) != PRM_OK) {
        LogPrintf (pacFac, LT_ALERT, 
					"<KommAbMelden> Error getting parameter "
					"P_WaitBookList -> set value to 120");
        iWaitTime = 120;
    }

	zDiff = time(0) - tVplk.vplkKsStartZeit;

	if (zDiff < iWaitTime) {

		WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Listenkommission"),
					WboxNtext,
						MlM("Sie haben den Auftrag gerade gestartet.\n"
							"Abmelden noch nicht m�glich!"),
					NULL);

		return(-1);
	}

	lCntTek = 0;

	if ((iRv = TExecSql( hMaskRl,
					"SELECT COUNT(TEK.TEID) FROM TEK "
					" WHERE TEK.TEGRP = :tegrp",
					SELLONG (lCntTek),
					SQLLONG(tVplk.vplkKsNr),
					NULL)) < 0) {

        WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        StrForm(GetMlmErrTxt(MlM_DbError), TN_TEK),
                    NULL);

        LogPrintf (pacFac, LT_ALERT, 
					"KommAbMelden, KsNr: %ld,  Error: %s ",
				   tVplk.vplkKsNr, TSqlErrTxt(hMaskRl));
        return(-1);

	}

	if (lCntTek < 1) {

		lCntTek = 0;
		/*
		 *	Anzahl neuen TEK holen 
		 */

		me_listtelabel(SHELL_OF(hMaskRl), &lCntTek);
	
        if (lCntTek >= 1) {

			strncpy (acStandOrt, tTermCtx.acXtStandort, STANDORT_LEN);
			acStandOrt[STANDORT_LEN] = '\0';

			if ((PrmGet2Parameter (hMaskRl, 
									P_SelectPrinter,
									"Listenkommission", 
									PRM_CACHE, 
									&iWert) == PRM_OK) && iWert == 1) {
				ptPrnPrn = 
					(PRIMANDOPRINT *)PrimanSelectDoPrintCount(SHELL_OF(hMaskRl),
															  PURPOSE_ETIKETTEN,
															  &lCount);
			} else {
				ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(hMaskRl,
												acStandOrt,
												PURPOSE_ETIKETTEN);
			}

			if(ptPrnPrn == NULL) {
				LogPrintf (pacFac, LT_ALERT,
							"KommAbMelden, ptPrnPrn == NULL");
				return -1;
			}

			if ((iRv = _GetLabelPathByPrnId (hMaskRl, 
											ptPrnPrn, 
											pacFac)) < 0) {
				LogPrintf (pacFac, LT_ALERT,
							"KommAbMelden, _GetLabelPathByPrnId failed !!!");
				return (-1);
			}
			
			for (nI = 0; nI < lCntTek; nI++) {

				if ((iRv = ListKomPrintTELabel (hMaskRl, 
											GetTermFac(), 
											&tVplk, 
											ptPrnPrn,
											pacUser,
										    lCount)) < 0) {

					WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_ALERT,
							WboxNbutton,    WboxbNok,
							WboxNmwmTitle,  MlM("Listenkommission"),
							WboxNtext,
								GetMlmErrTxt(MlM_PrintErr),
							NULL);

					return(-1);
				}
			}
		}
	}

    /* Check Kette Order: if the Order is Kette Komm all the KetGrup is set */
    if (tVplk.vplkKetGrp > 0) {

        memset (&tArrDescKetVplk, 0, sizeof (tArrDescKetVplk));
        memset (&acGrpKsNr[0],0,sizeof(acGrpKsNr));

        if (GetKetteKsNr (hMaskRl, pacFac, &tVplk,
                                        &tArrDescKetVplk) < 0){
            WamasBox(SHELL_OF(hMaskRl),
                        WboxNboxType,   WBOX_ALERT,
                        WboxNbutton,    WboxbNok,
                        WboxNmwmTitle,  MlM("Listenkommission"),
                        WboxNtext,
                            MlM("Fehler beim Anmelden des Auftrags!"),
                        NULL);

            LogPrintf (pacFac, LT_ALERT,
                        "KommAbMelden, GetKetteKsNr failed !!!");
            return(-1);
        }

        for (nI = 0; nI < tArrDescKetVplk.nEle; nI ++){

            memset (&tVplkTmp, 0, sizeof (tVplkTmp));

            tVplkTmp = ((VPLK *)(void *)tArrDescKetVplk.DataArr)[nI];

            /* Lock data */

            if (ReadLockMyVplk (hMaskRl, GetTermFac(), &tVplkTmp) <= 0) {

                LogPrintf (pacFac, LT_ALERT,
                        "KommAbMelden, KsNr: %ld, ReadLockMyVplk failed !!!",
                        tVplk.vplkKsNr);

                if (tArrDescKetVplk.DataArr != NULL) {
                    free (tArrDescKetVplk.DataArr);
                }
                return(-1);
            }

            /* Checking Status and Aktuser from same KetGrp */
            if (tVplkTmp.vplkStatus != VPLKSTATUS_AKTIV){
                continue;
            }
            if (strcmp (tVplkTmp.vplkAktUser, tVplk.vplkAktUser) == 1){
                continue;
            }

			/* iSetLastPos: in SetVplKOMM all Positions (Vplps) have a
			-* KommZeit of 1 second except last position where the rest
			-* ist Set. In Kette Orders only the Last Position of Last VPLK
			-* must have the rest time, all the others must have 1 second */
			if (nI == (tArrDescKetVplk.nEle - 1)){
				iSetLastPos = JANEIN_J;
			} else {
				iSetLastPos = JANEIN_N;
			}

            strcat (acGrpKsNr, StrForm ("\t%ld\n", tVplkTmp.vplkKsNr));

            if (SetVplKOMM (hMaskRl, pacFac, &tVplkTmp, iSetLastPos) < 0) {

                WamasBox(SHELL_OF(hMaskRl),
                        WboxNboxType,   WBOX_ALERT,
                        WboxNbutton,    WboxbNok,
                        WboxNmwmTitle,  MlM("Listenkommission"),
                        WboxNtext,
                            MlM("Fehler beim Anmelden des Auftrags!"),
                        NULL);

                LogPrintf (pacFac, LT_ALERT,
                        "KommAbMelden, SetVplKOMM failed !!!");

                if (tArrDescKetVplk.DataArr != NULL) {
                    free (tArrDescKetVplk.DataArr);
                }
                return(-1);
            }
        }

        if (tArrDescKetVplk.DataArr != NULL) {
            free (tArrDescKetVplk.DataArr);
        }

        WamasBox (SHELL_OF(hMaskRl),
                WboxNboxType,   WBOX_INFO,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext, 
                    StrForm (MlM ("Der Benutzer %s wurde von verketteten "
								"Auftr�gen mit KsNr:\n%s"
                                "abgemeldet!"),
                                ptKomAnAbCtx->acPersNr, acGrpKsNr),
                NULL);

    } else {
        if (SetVplKOMM (hMaskRl, pacFac, &tVplk, JANEIN_J) < 0) {

            WamasBox(SHELL_OF(hMaskRl),
                        WboxNboxType,   WBOX_ALERT,
                        WboxNbutton,    WboxbNok,
                        WboxNmwmTitle,  MlM("Listenkommission"),
                        WboxNtext,
                            MlM("Fehler beim Anmelden des Auftrags!"),
                        NULL);

            LogPrintf (pacFac, LT_ALERT,
                        "KommAbMelden, SetVplKOMM failed !!!");

            return(-1);
        }

        WamasBox(SHELL_OF(hMaskRl),
                WboxNboxType,   WBOX_INFO,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    StrForm(MlM("Der Benutzer %s wurde vom Auftrag\n\n"
                                "  KsNr      : %d\n"
                                "  AusNr     : %s\n"
                                "  Kunde     : %s\n"
                                "  Lagerzone : %s\n\n"
                                "abgemeldet!"),
                                ptKomAnAbCtx->acPersNr, tVplk.vplkKsNr,
                                tVplk.vplkAusId.AusNr,
                                tVplk.vplkKuName, tVplk.vplkLazNr),
                NULL);
    }

	return (1);

} /* KommAbMelden () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int KommAnMelden (MskDialog hMaskRl, char *pacFac, KOMANABCTX *ptKomAnAbCtx)
{
	VPLK			tVplk, tVplkTmp;
	PRIMANDOPRINT   *ptPrnPrn = NULL;
	long			lVplkStartStatus = GetVplkStartStatus (hMaskRl, pacFac);
	long 			lParm1=0;
	int				iRv = 0, iCheckZu = 0, nI, nJ, iKetteOrder = JANEIN_N;
	char			acStandOrt[STANDORT_LEN + 1], acData[128];
	char			acSql[STMT_LEN + 1];
	char            acGrpKsNr[256];
	int             iFunkNotSystem = JANEIN_N, iAnswer = 0, iLazFound;
	long            lCount = 0;
	ArrDesc         tArrDescVplksToStart, tArrDescTmpVplk, tArrLazK;
	StartType		eStartType;

    if (PrmGet1Parameter (NULL, P_FunkNotSystem,
                            PRM_CACHE, &iFunkNotSystem) != PRM_OK) {
        iFunkNotSystem = JANEIN_N;
    }

	memset (&tVplk, 0, sizeof(VPLK));

	/* Check for zugeteilten Auftrag */

	iRv = TExecSql( hMaskRl,
					"SELECT %VPLK FROM VPLK"
					" WHERE VPLK.Status =:a "
					" AND VPLK.AktUser like ' %%' "
					" AND VPLK.ZugUser =:b "
					" AND VPLK.KomArt in ("STR_KOMARTLISTE","STR_KOMARTUNDEF") "
					" AND VPLK.VplArt not like "STR_VPLKART_TPA,
					SELSTRUCT (TN_VPLK, tVplk),
					SQLVPLKSTATUS (lVplkStartStatus),
					SQLSTRING (ptKomAnAbCtx->acPersNr),
					NULL);

	if (iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {

		WamasBox (SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Listenkommission"),
				WboxNtext,
					StrForm(GetMlmErrTxt(MlM_DbError), TN_VPLK),
				NULL);

		LogPrintf (pacFac, LT_ALERT, 
				"KommAnMelden, Error: %s ",
				 TSqlErrTxt(hMaskRl));

		return(-1);
	}

	if(iRv > 0 ) {

		if ((ptKomAnAbCtx->lKsNr > 0) && 
						(ptKomAnAbCtx->lKsNr != tVplk.vplkKsNr)) {

			WamasBox(SHELL_OF(hMaskRl),
                WboxNboxType,   WBOX_INFO,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    StrForm("Sie haben einen zugeteilten Auftrag den\n"
							"Sie zuerst abarbeiten m�ssen! KsNr : %d",
							 tVplk.vplkKsNr),
                NULL);
			return(-1);
		}	

		if (strcmp(ptKomAnAbCtx->acLazNr, tVplk.vplkLazNr) != 0) {

			WamasBox(SHELL_OF(hMaskRl),
                WboxNboxType,   WBOX_INFO,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    StrForm("Sie haben einen zugeteilten Auftrag in\n"
							" der Lagerzone %s den\n"
                            "Sie zuerst abarbeiten m�ssen! KsNr : %d",
                             tVplk.vplkLazNr, tVplk.vplkKsNr),
                NULL);

            return(-1);
		}
	
		iCheckZu = JANEIN_J;
	}

	if (iCheckZu == JANEIN_J) {

		/* zugeteilter Auftrag wird aktiviert */

	} else {

		if (ptKomAnAbCtx->lKsNr > 0) {

            if (ptKomAnAbCtx->AnAbMode == KOMMANAB_AN_NACH) {
    			iRv = TExecSql( hMaskRl,
    							"SELECT %VPLK FROM VPLK"
    							" WHERE VPLK.Status = "STR_VPLKSTATUS_NACH
    							" AND VPLK.AktUser like ' %%' "
    							" AND VPLK.ZugUser like ' %%' "
    							" AND VPLK.VplArt not like "STR_VPLKART_TPA
    							" AND KsNr =:b",
    							SELSTRUCT(TN_VPLK, tVplk),
    							SQLLONG(ptKomAnAbCtx->lKsNr),
    							NULL);

			} else {
			
    			memset(acSql, 0, sizeof(acSql));
    
    			if(	iFunkNotSystem == JANEIN_J &&
    				ptKomAnAbCtx->vplkKomArt == KOMARTFUNK &&
    				ptKomAnAbCtx->vplkStatus == VPLKSTATUS_AKTIV) {
    
    				lVplkStartStatus = VPLKSTATUS_AKTIV;
    				sprintf(acSql, "SELECT %%VPLK FROM VPLK"
    							   " WHERE VPLK.Status =:a "
    							   " AND VPLK.VplArt not like "STR_VPLKART_TPA
    							   " AND KsNr =:b");
    
    			} else {
    	
    				sprintf(acSql, "SELECT %%VPLK FROM VPLK"
    							   " WHERE VPLK.Status =:a "
    							   " AND VPLK.AktUser like ' %%%%' "
    							   " AND VPLK.ZugUser like ' %%%%' "
    							   " AND VPLK.VplArt not like "STR_VPLKART_TPA
    							   " AND KsNr =:b");
    
    			}
    
    			iRv = TExecSql( hMaskRl,
    							acSql,
    							SELSTRUCT(TN_VPLK, tVplk),
    							SQLVPLKSTATUS(lVplkStartStatus),
    							SQLLONG(ptKomAnAbCtx->lKsNr),
    							NULL);
            }
			if (iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {

				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Listenkommission"),
						WboxNtext,
							StrForm(GetMlmErrTxt(MlM_DbError), TN_VPLK),
						NULL);

				LogPrintf (pacFac, LT_ALERT, 
							"KommAnMelden, Error: %s ",
						   	TSqlErrTxt(hMaskRl));

				return(-1);
			}

			if (iRv <= 0) {

				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Listenkommission"),
						WboxNtext,
							GetMlmErrTxt(MlM_NoData),
						NULL);

				return(-1);
			}
		} else {
			iRv = TExecSql( hMaskRl,
							"SELECT %VPLK FROM VPLK"
							" WHERE VPLK.Status =:a "
							" AND VPLK.LazNr =:b"
							" AND VPLK.AktUser like ' %%' "
							" AND VPLK.ZugUser like ' %%' "
							" AND VPLK.KomArt = "STR_KOMARTLISTE
							" AND VPLK.VplArt not like "STR_VPLKART_TPA
							" ORDER BY VPLK.Prio DESC, VPLK.SollStartZeit, "
							" VPLK.LadeRf, VPLK.KsNr",
							SELSTRUCT(TN_VPLK, tVplk),
							SQLVPLKSTATUS(lVplkStartStatus),
							SQLSTRING(ptKomAnAbCtx->acLazNr),
							NULL);

			if (iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {

				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Listenkommission"),
						WboxNtext,
							StrForm(GetMlmErrTxt(MlM_DbError), TN_VPLK),
						NULL);

				LogPrintf (pacFac, LT_ALERT, 
							"KommAnMelden, Error: %s ",
						   	TSqlErrTxt(hMaskRl));
				return(-1);
			}

			if (iRv <= 0) {

				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Listenkommission"),
						WboxNtext,
							GetMlmErrTxt(MlM_NoData),
						NULL);

				return(-1);
			}
		}
	}

    /* Check KetteKommission */

    memset (&tArrDescVplksToStart, 0, sizeof (tArrDescVplksToStart));

    if (IsEmptyStrg (tVplk.vplkKetId) == JANEIN_J){
        /* Not a Kette Komm Order */
        iKetteOrder = JANEIN_N;
        if (ReadLockMyVplk (hMaskRl, GetTermFac(), &tVplk) <= 0) {

            LogPrintf (pacFac, LT_ALERT,
                    "KommAnMelden, KsNr: %ld ReadLockMyVplk failed !!!",
                    tVplk.vplkKsNr);

            return(-1);
        }
        if (AppendEle (&tArrDescVplksToStart, &tVplk, 1,
                                sizeof (VPLK)) < 0) {
            LogPrintf (pacFac, LT_ALERT,
                       "KommAnMelden: Error in Append Element.");
            /* free ArrDesc */
            if (tArrDescVplksToStart.DataArr != NULL) {
                free (tArrDescVplksToStart.DataArr);
            }
            return (-1);
        }
    } else {
        /* Kette Order
        -* Check if Kette Group is defined */
        iKetteOrder = JANEIN_J;
        if (tVplk.vplkKetGrp > 0){

            /* Order was allready printed. Getting All Vplks from Same Group */

            if (GetKetteBereitVplks (hMaskRl, pacFac, &tVplk,
                                        &tArrDescVplksToStart, JANEIN_N) < 0) {

                LogPrintf (pacFac, LT_ALERT,
                       "KommAnMelden: Error in GetKetteBereitVplks %ld.",
                        tVplk.vplkKsNr);
                /* free ArrDesc */
                if (tArrDescVplksToStart.DataArr != NULL) {                                         free (tArrDescVplksToStart.DataArr);
                }
                return (-1);
            }

            /* Locking Vplks */
            for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){

                memset (&tVplkTmp, 0, sizeof(tVplkTmp));
                tVplkTmp = ((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI];

                if (ReadLockMyVplk (hMaskRl, pacFac, &tVplkTmp) <= 0) {

                    LogPrintf (pacFac, LT_ALERT,
                        "KommAnMelden, KsNr: %ld ReadLockMyVplk failed !!!",
                        tVplkTmp.vplkKsNr);

                    return(-1);
                }
            }
        } else {
            /* Getting All Vplks from same Kette */

            memset (&tArrDescTmpVplk, 0, sizeof (tArrDescTmpVplk));

			/* Get all KetteBereitVplks without Kette Group */
            if (GetKetteBereitVplks (hMaskRl, pacFac, &tVplk,
                                        &tArrDescTmpVplk, JANEIN_J) < 0) {

                LogPrintf (pacFac, LT_ALERT,
                       "KommAnMelden: Error in GetKetteBereitVplks %ld.",
                        tVplk.vplkKsNr);
                /* free ArrDesc */
                if (tArrDescTmpVplk.DataArr != NULL) {
                    free (tArrDescTmpVplk.DataArr);
                }
                if (tArrDescVplksToStart.DataArr != NULL){
                    free (tArrDescVplksToStart.DataArr);
                }
                return (-1);
            }

            /* Making Kette Groups and Getting First one.
            -* (Vplks Locked inside Function) */

            if (MakeKetteGroups (hMaskRl, pacFac, &tArrDescTmpVplk,
                            &tArrDescVplksToStart, JANEIN_J, JANEIN_N) < 0){
                LogPrintf (pacFac, LT_ALERT,
                       "CheckSetKetteKomm: Error in MakeKetteGroups");
                /* free ArrDesc */
                if (tArrDescTmpVplk.DataArr != NULL){
                    free (tArrDescTmpVplk.DataArr);
                }
                if (tArrDescVplksToStart.DataArr != NULL){
                    free (tArrDescVplksToStart.DataArr);
                }
                return (-1);
            }
            /* free ArrDesc */
            if (tArrDescTmpVplk.DataArr != NULL){
                free (tArrDescTmpVplk.DataArr);
            }
        }

        /* Showing Kette warning */
        if (tArrDescVplksToStart.nEle > 1){

            memset(&acGrpKsNr[0],0,sizeof(acGrpKsNr));

            for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){
              strcat (acGrpKsNr, StrForm ("\t%ld\n",
                (((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI]).vplkKsNr));
            }
            if (WamasBox(SHELL_OF(hMaskRl),
                            WboxNboxType,   WBOX_WARN,
                            WboxNbuttonText,MlM("OK"),
                            WboxNbuttonRv,  IS_Ok,
                            WboxNbutton,    WboxbNcancel,
                            WboxNescButton, WboxbNcancel,
                            WboxNtimer,     6000,
                            WboxNmwmTitle,  MlM("Listenkommission"),
                            WboxNtext, 
                            StrForm (MlM ("Auftr�ge mit KsNr:\n%s"
                                "sind verkettet und werden gemeinsam gestartet."
                                "\nStarten?"), acGrpKsNr),
                            NULL) != IS_Ok) {
                LogPrintf (pacFac, LT_ALERT,
                       "User canceled start.");
                /* free ArrDesc */
                if (tArrDescVplksToStart.DataArr != NULL){
                    free (tArrDescVplksToStart.DataArr);
                }
                return -1;
            }
        }
    }

	/* Check that User can work in All Kom Zones from Orders.*/

	memset (&tArrLazK, 0, sizeof (tArrLazK));
	if (ReadLazkByPersNr (NULL, pacFac, ptKomAnAbCtx->acPersNr,
                            STR_LAZTYP_KOM, &tArrLazK) < 0) {
        LogPrintf (pacFac, LT_ALERT,
                       "Error in ReadLazkByPersNr. User %s",
						ptKomAnAbCtx->acPersNr);
		if (tArrDescVplksToStart.DataArr != NULL){
        	free (tArrDescVplksToStart.DataArr);
        }
		if (tArrLazK.DataArr != NULL){
			free (tArrLazK.DataArr);
		}
        return (-1);
    }

	for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){

		iLazFound = JANEIN_N;
        memset (&tVplkTmp, 0, sizeof(tVplkTmp));
        tVplkTmp = ((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI];

		for (nJ = 0; nJ < tArrLazK.nEle; nJ++){
        	if (strcmp (tVplkTmp.vplkLazNr, 
				((LAZK *)(void *)tArrLazK.DataArr)[nJ].lazkLazId.LazNr) == 0){
				iLazFound = JANEIN_J;
				break;
			}
		}
		if (iLazFound == JANEIN_N){
			/* User cannot work in this Zone Komm */
			WamasBox (SHELL_OF (hMaskRl),
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    StrForm(MlM("Benutzer %s darf nicht"
							" im Lagerzone %s kommissionieren."),
							ptKomAnAbCtx->acPersNr,
                            tVplkTmp.vplkLazNr),
                WboxNtimer,     6000,
                NULL);

			LogPrintf (pacFac, LT_ALERT,
						   "User %s cannot work in Laznr %s",
							ptKomAnAbCtx->acPersNr,
							tVplkTmp.vplkLazNr);
			if (tArrDescVplksToStart.DataArr != NULL){
				free (tArrDescVplksToStart.DataArr);
			}
			if (tArrLazK.DataArr != NULL){
				free (tArrLazK.DataArr);
			}
			return (-1);
		}
	}

	if (tArrLazK.DataArr != NULL){
    	free (tArrLazK.DataArr);
    }

    /* Check Print */
    iAnswer = WamasBox(SHELL_OF(hMaskRl),
                        WboxNboxType,   WBOX_WARN,
						WboxNbuttonText,MlM("Nein"),
                        WboxNbuttonRv,  0,
                        WboxNbuttonText,MlM("Listen"),
                        WboxNbuttonRv,  1,
                        WboxNbuttonText,MlM("Etiketten"),
                        WboxNbuttonRv,  2,
                        WboxNescButton, WboxbNcancel,
                        WboxNtimer,     6000,
                        WboxNmwmTitle,  MlM("Listenkommission"),
                        WboxNtext,
                            MLM("Wollen Sie eine Kommissionierliste drucken?"),
                        NULL);
    if ((iAnswer!=1) && (iAnswer!=2)){
        /* print not need */

    } else {
        /* iAnswer 1-> Listen, iAnswer 2-> Etiketten */

        if ((PrmGet2Parameter (hMaskRl,
                                P_SelectPrinter,
                                "Listenkommission",
                                PRM_CACHE,
                                &lParm1) == PRM_OK) &&
                                lParm1 == JANEIN_J) {
            ptPrnPrn = (PRIMANDOPRINT *)PrimanSelectDoPrintCount (
                                                    SHELL_OF(hMaskRl),
                                                    (iAnswer == 1)?
                                                        PURPOSE_LISTEN:
                                                        PURPOSE_ETIKETTEN,
													&lCount);
        } else {
            strncpy(acStandOrt, tTermCtx.acXtStandort, STANDORT_LEN);
            acStandOrt[STANDORT_LEN] = '\0';

            ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(hMaskRl,
                                            acStandOrt,
                                            (iAnswer == 1)?
                                                PURPOSE_LISTEN:
                                                PURPOSE_ETIKETTEN);
        }

        if (iAnswer == 1){
            /* Print a List */
            for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){

                memset (acData,     0, sizeof (acData));
                sprintf(acData, "%ld",
                   ((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI].vplkKsNr);
                if (Print_PCL_List (hMaskRl,
                                        pacFac,
                                        ptPrnPrn,
                                        acData,
                                        ModeKomList,
                                        JANEIN_J,
										lCount) < 0) {

					LogPrintf (pacFac, LT_ALERT,
                               "Error in Print_PCL_List. KsNr:%s", acData);
                    /* free ArrDesc */
                    if (tArrDescVplksToStart.DataArr != NULL){
                        free (tArrDescVplksToStart.DataArr);
                    }
                    TSqlRollback(hMaskRl);
                    WamasBox(SHELL_OF(hMaskRl),
                            WboxNboxType,   WBOX_ALERT,
                            WboxNbutton,    WboxbNok,
                            WboxNmwmTitle,  MlM("Listenkommission"),
                            WboxNtext,
                                GetMlmErrTxt(MlM_PrintErr),
                            NULL);
                    return(-1);
                }
                if (((VPLK *)(void *)
                        tArrDescVplksToStart.DataArr)[nI].vplkFLAG.Druck
                                                                != SETFLAGOK){
                    ((VPLK *)(void *)
                        tArrDescVplksToStart.DataArr)[nI].vplkFLAG.Druck
                                                                = SETFLAGOK;
                }
            }
        } else {
            /* Printing Etiketten */

            if (Print_KomEtik (hMaskRl, pacFac, ptPrnPrn,
                                &tArrDescVplksToStart, JANEIN_J, lCount) < 0){
                LogPrintf (pacFac, LT_ALERT,
                               "Error in Print_KomEtik.");
                /* free ArrDesc */
                if (tArrDescVplksToStart.DataArr != NULL){
                    free (tArrDescVplksToStart.DataArr);
                }
                TSqlRollback (hMaskRl);
                WamasBox(SHELL_OF(hMaskRl),
                            WboxNboxType,   WBOX_ALERT,
                            WboxNbutton,    WboxbNok,
                            WboxNmwmTitle,  MlM("Listenkommission"),
                            WboxNtext,
                                GetMlmErrTxt(MlM_PrintErr),
                            NULL);
                return (-1);
            }
            for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){
                if (((VPLK *)(void *)
                        tArrDescVplksToStart.DataArr)[nI].vplkFLAG.Druck
                                                                != SETFLAGOK){
                    ((VPLK *)(void *)
                        tArrDescVplksToStart.DataArr)[nI].vplkFLAG.Druck
                                                                = SETFLAGOK;
                }
            }
        }
    }

    /* Aktivating Order/s */

    memset(&acGrpKsNr[0],0,sizeof(acGrpKsNr));

    for (nI = 0; nI < tArrDescVplksToStart.nEle; nI++){

        memset (&tVplkTmp, 0, sizeof(tVplkTmp));
        tVplkTmp = ((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI];

        strcpy (tVplkTmp.vplkAktUser, ptKomAnAbCtx->acPersNr);

		if (ptKomAnAbCtx->AnAbMode == KOMMANAB_AN_NACH){
			eStartType = LIST_NACH;
		} else {
			eStartType = LIST_STARTEN;
		}
        if (SetKomAktiv (hMaskRl, pacFac, GetUserOrTaskName(), 
												&tVplkTmp, eStartType) < 0) {

            TSqlRollback (hMaskRl);
            WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        MlM("Fehler beim Anmelden des Auftrags!"),
                    NULL);

            LogPrintf (pacFac, LT_ALERT,
                    "KomAnMelden, SetKomAktiv failed !!!KsNr %ld",
                    tVplkTmp.vplkKsNr);
            /* free ArrDesc */
            if (tArrDescVplksToStart.DataArr != NULL){
                free (tArrDescVplksToStart.DataArr);
            }
            return(-1);
        }

        strcat (acGrpKsNr, StrForm ("\t%ld\n",
                (((VPLK *)(void *)tArrDescVplksToStart.DataArr)[nI]).vplkKsNr));
    }

    if (iKetteOrder == JANEIN_N){
        WamasBox(SHELL_OF(hMaskRl),
            WboxNboxType,   WBOX_INFO,
            WboxNbutton,    WboxbNok,
            WboxNmwmTitle,  MlM("Listenkommission"),
            WboxNtext,
                StrForm(MlM("Der Auftrag %d"), tVplk.vplkKsNr);
                StrForm(MlM("Der Auftrag\n\n"
                        "  KsNr      : %d\n"
                        "  AusNr     : %s\n"
                        "  Kunde     : %s\n"
                        "  Lagerzone : %s\n\n"
                        "wurde f�r den Benutzer %s aktiviert!"),
                        tVplk.vplkKsNr,
                        tVplk.vplkAusId.AusNr,
                        tVplk.vplkKuName,
                        tVplk.vplkLazNr,
                        ptKomAnAbCtx->acPersNr),
            NULL);
    } else {
        WamasBox(SHELL_OF(hMaskRl),
            WboxNboxType,   WBOX_INFO,
            WboxNbutton,    WboxbNok,
            WboxNmwmTitle,  MlM("Listenkommission"),
            WboxNtimer,     6000,
            WboxNtext, 
                StrForm (
                        MlM ("Die Auftr�ge mit KsNr:\n%s"
                        "wurden f�r den Benutzer %s aktiviert!"),
                        acGrpKsNr,
                        ptKomAnAbCtx->acPersNr),
            NULL);
    }

    if (tArrDescVplksToStart.DataArr != NULL){
        free (tArrDescVplksToStart.DataArr);
    }

	return(0);

} /* KommAnMelden () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
    Callback f�r den OK-Button. Funktion schreibt die �nderungen in die DB.
-* RETURNS
-*--------------------------------------------------------------------------*/

static int CbAnAbmelden (MskTmaskRlPtr hMaskRl, MskStatic ef,
					MskElement ef_rl, int reason, void *cbc, void *calldata)
{
	KOMANABCTX   		*ptKommAnAbCtx = NULL;
    VPLK         		tVplk;
	VPLUEB_CTX          *ptVpluebCtx = NULL;
	int          		iRv = 0, iRet = 0;
	long		 		lCntPersNr = 0;

    switch (reason) {
    case FCB_XF:

		ptKommAnAbCtx = (KOMANABCTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);

        MskTransferMaskDup(hMaskRl);

		if (IsEmptyStrg (ptKommAnAbCtx->acPersNr) == JANEIN_J) {

			WamasBox (SHELL_OF (hMaskRl),
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
					MlM("Bitte geben Sie die Personalnummer ein!"),
                NULL);
			break;
		}

		/* Personalnummer checken */
		lCntPersNr = 0;
		iRv = TExecSql( hMaskRl,
						"SELECT COUNT(*) from UM_PERS WHERE"
						" UM_PERS.PersNr =:a",
						SELLONG(lCntPersNr),
						SQLSTRING(ptKommAnAbCtx->acPersNr),
						NULL);

		if(lCntPersNr < 1) {

			WamasBox (SHELL_OF (hMaskRl),
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    MlM("Personalnummer im System nicht vorhanden!"),
                NULL);

			LogPrintf (GetTermFac(), LT_ALERT, 
						"CbAnAbmelden. PersNr: %s nicht bekannt.Error: %s",
						ptKommAnAbCtx->acPersNr,
					   TSqlErrTxt(hMaskRl));

			TSqlRollback(hMaskRl);

            break;
		}

		memset (&tVplk, 0, sizeof(VPLK));
		/*
		 *	existiert f�r den User ein aktiver VPLK ???
		 */

		iRv = TExecSql( hMaskRl,
						"SELECT %VPLK FROM VPLK"
						" WHERE VPLK.STATUS = "STR_VPLKSTATUS_AKTIV
						" AND VPLK.KomArt = "STR_KOMARTLISTE
						" AND VPLK.AktUser = :a ",
						SELSTRUCT (TN_VPLK, tVplk),
						SQLSTRING (ptKommAnAbCtx->acPersNr),
						NULL);		

		if (iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {

			WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        StrForm(GetMlmErrTxt(MlM_DbError), TN_VPLK),
                    NULL);

			LogPrintf (GetTermFac(), LT_ALERT, 
						"CbAnAbmelden. PersNr: %s, Error: %s ",
						ptKommAnAbCtx->acPersNr,
					   	TSqlErrTxt(hMaskRl));

			TSqlRollback(hMaskRl);

			break;
		}

		if (iRv > 0) {
			/* 
			 *		ein aktiver VPLK mit der Listenkommission gefunden !!!
			 */

			if (ptKommAnAbCtx->AnAbMode == KOMMANAB_AN) {

			/* AnAbMode == anmelden , abmelden nicht m�glich !!!*/
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Listenkommission"),
					WboxNtext,
						StrForm (MlM ("F�r PersNr: %s existiert "
								"schon ein aktiver Auftrag"), 
								ptKommAnAbCtx->acPersNr),
					NULL);

				break;

			} else {
				/* Kommission abmelden */
				
				if (ptKommAnAbCtx->AnAbMode == KOMMANAB_AB) {

					if ((iRet = KommAbMelden (hMaskRl, 
											GetTermFac(), 
											&tVplk, 
											ptKommAnAbCtx,
											GetUserOrTaskName())) < 0) {	

						TSqlRollback(hMaskRl);

            			break;
					}

				} else {
					/* ptKommAnAbCtx->AnAbMode == KOMMANAB_UNDEF */

			  		if (WamasBox(SHELL_OF(hMaskRl),
							WboxNboxType,   WBOX_WARN,
							WboxNbuttonText,MlM("OK"),
							WboxNbuttonRv,  IS_Ok,
							WboxNbutton,    WboxbNcancel,
							WboxNescButton, WboxbNcancel,
							WboxNmwmTitle,  
								MlM("Listenkommission"),
							WboxNtext,
								StrForm (MlM 
									("Aktiven Auftrag KsNr:%ld abmelden?"),
									tVplk.vplkKsNr),
							NULL) != IS_Ok){

            			break;
					}
					if ((iRet = KommAbMelden (hMaskRl, 
											GetTermFac(), 
											&tVplk, 
											ptKommAnAbCtx,
											GetUserOrTaskName())) < 0) {	

						TSqlRollback(hMaskRl);
            			break;
					}
        		}
			}

		} else {
			/* keinen aktiven Auftrag gefunden */

			if (ptKommAnAbCtx->AnAbMode == KOMMANAB_AB) {

				/* Abmelden nicht m�glich */
				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Listenkommission"),
					WboxNtext,
						StrForm (MlM 
							("Keinen aktiven Auftrag f�r PersNr: %s"),
								ptKommAnAbCtx->acPersNr),
					NULL);

				break;

	
			}

			/* Kommission anmelden */

			if (IsEmptyStrg (ptKommAnAbCtx->acLazNr) == JANEIN_J) {

				WamasBox (SHELL_OF (hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Listenkommission"),
					WboxNtext,
						MlM("Bitte geben Sie eine Lagerzone ein!"),
					NULL);

				set_focus (hMaskRl, "VPLK_LazNr_t", KEY_DEF);

				TSqlRollback(hMaskRl);

				break;
			}

			if ((iRet = KommAnMelden (hMaskRl, 
								GetTermFac(), ptKommAnAbCtx)) < 0) {	

                TSqlRollback(hMaskRl);

                break;
            }
		}

		TSqlCommit(hMaskRl);

	  	if (((ptVpluebCtx = 	
				(VPLUEB_CTX *)MskRlMaskGet (ptKommAnAbCtx->hMaskRlVplueb,
										MskNmaskCalldata)) != NULL) &&
            (ptKommAnAbCtx->AnAbMode != KOMMANAB_AN_NACH)) {

        	ListUpdateVplueb (ptKommAnAbCtx->hMaskRlVplueb, 
									ptVpluebCtx, 
									MANUELL_LEVEL);

        	MskUpdateMaskVar (ptKommAnAbCtx->hMaskRlVplueb);
    	}

		MskCbClose (hMaskRl, ef, ef_rl, reason, cbc);

        break;

    default:

        break;
    }

    return (EFCBM_CONTINUE);

} /* CbAnAbmelden () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      TE Begleitschein drucken.
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbBegdruck (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                         int iReason, void *pvCbc, void *pvCalldata)
{
	KOMANABCTX    	*ptKomAnAbCtx = NULL;
	VPLK			tVplk;
	PRIMANDOPRINT 	*ptPrnPrn;
    int         	iRv = 0, nI = 0;
	int             iWert=0;
	long			lAnzahl=0;
	long            lCount = 0;
	char            acStandOrt[100+1];

    switch (iReason) {
    case FCB_XF:

		ptKomAnAbCtx = (KOMANABCTX *)MskRlMaskGet(hMaskRl,
                                                MskNmaskCalldata);
        MskTransferMaskDup(hMaskRl);
		
		memset (&tVplk, 0, sizeof(VPLK));

		/* Statement looks for aktive Order for User 
		-* There is no need to give the KsNr, if there is
		-* one active it will be printed.*/

		if (ptKomAnAbCtx->lKsNr == 0){
        	iRv = TExecSql( hMaskRl,
                        "SELECT %VPLK FROM VPLK"
                        " WHERE VPLK.STATUS = "STR_VPLKSTATUS_AKTIV
                        " AND VPLK.KomArt = "STR_KOMARTLISTE
                        " AND VPLK.AktUser = :a ",
                        SELSTRUCT(TN_VPLK, tVplk),
                        SQLSTRING(ptKomAnAbCtx->acPersNr),
                        NULL);
		} else {
			iRv = TExecSql( hMaskRl,
                        "SELECT %VPLK FROM VPLK"
                        " WHERE VPLK.STATUS = "STR_VPLKSTATUS_AKTIV
                        " AND VPLK.KsNr = :a ",
                        SELSTRUCT(TN_VPLK, tVplk),
                        SQLLONG (ptKomAnAbCtx->lKsNr),
                        NULL);
		}

        if(iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {

            WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        StrForm(GetMlmErrTxt(MlM_DbError), TN_VPLK),
                    NULL);

            LogPrintf (GetTermFac(), LT_ALERT, 
					"CbBegdruck, Error: %s ",
				   TSqlErrTxt(hMaskRl));

            TSqlRollback(hMaskRl);

            break;
        }

		if(iRv <= 0) {

			if (ptKomAnAbCtx->lKsNr == 0){
				WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        GetMlmErrTxt(MlM_NoData),
                    NULL);
			} else {
				WamasBox(SHELL_OF(hMaskRl),
                    WboxNboxType,   WBOX_ALERT,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
						MlM("Der Begleitschein kann nur f�r gestartete "
							"Auftr�ge gedruckt werden!"),
                    NULL);
			}

			TSqlRollback(hMaskRl);

            break;
		}	

		lAnzahl = 0;

		me_listtelabel(SHELL_OF(hMaskRl), &lAnzahl);
	
		if(lAnzahl < 1) {
			break;
		}	

		strncpy(acStandOrt, tTermCtx.acXtStandort, 100);
		acStandOrt[100] = '\0';

		if (PrmGet2Parameter (hMaskRl, 
								P_SelectPrinter,
								"Listenkommission", 
								PRM_CACHE, 
								&iWert) == PRM_OK && iWert == 1) {

			ptPrnPrn =
				(PRIMANDOPRINT *)PrimanSelectDoPrintCount(SHELL_OF(hMaskRl),
														  PURPOSE_ETIKETTEN,
														  &lCount);
		} else {
			ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(hMaskRl,
											acStandOrt,
											PURPOSE_ETIKETTEN);
		}

		if(ptPrnPrn == NULL) {

			LogPrintf (GetTermFac(), LT_ALERT,
				"CbBegdruck, ptPrnPrn == NULL");

			return (-1);
		}

		if (_GetLabelPathByPrnId(hMaskRl, ptPrnPrn, GetTermFac()) < 0) {
		
			LogPrintf (GetTermFac(), LT_ALERT,
						"CbBegdruck, _GetLabelPathByPrnId failed !!!");
			return (-1);
		}

		for (nI = 0; nI < lAnzahl; nI++) {

			if ((iRv = ListKomPrintTELabel (hMaskRl, 
											GetTermFac(), 
											&tVplk, 
											ptPrnPrn,
											GetUserOrTaskName(),
											lCount)) < 0) {

				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Listenkommission"),
						WboxNtext,
							GetMlmErrTxt(MlM_PrintErr),
						NULL);

				LogPrintf (GetTermFac(), LT_ALERT,
							"CbBegdruck, KsNr: %ld, "
							"ListKomPrintTELabel failed !!!",
							tVplk.vplkKsNr);

				TSqlRollback(hMaskRl);

				break;
			}
		}

		if (nI == lAnzahl) {

			WamasBox (SHELL_OF (hMaskRl),
                    WboxNboxType,   WBOX_INFO,
                    WboxNbutton,    WboxbNok,
                    WboxNmwmTitle,  MlM("Listenkommission"),
                    WboxNtext,
                        StrForm(
							MlM("Es wurden %d TE-Begleitscheine gedruckt!"),
							lAnzahl),
                    NULL);
		}

        TSqlCommit (hMaskRl);

        break;

    default:
        break;
    }

    return (1);
} /* CbBegdruck() */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* Callback for Barcode-Interface
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbScan (MskTmaskRlPtr hMaskRl, char *pacScanData)

{
    VPLK        tVplk;
    KOMANABCTX  *ptKommAnAbCtx;
	VPLUEB_CTX  *ptVpluebCtx = NULL;
	char		acPersNr[PERSNR_LEN];
    long        lKsNr = 0, lPersNr = 0;
	int			iCntPersNr = 0, iScanDataLen = 0;

    if (WdgGuiGet(GuiNactiveShell) != (Value)SHELL_OF(hMaskRl)) {
        return (0);
    }

    if (pacScanData == NULL) {
	        return (0);
    }
	ptKommAnAbCtx = (KOMANABCTX *)MskRlMaskGet(hMaskRl,
                                                MskNmaskCalldata);
	iScanDataLen = strlen (pacScanData);
	if (iScanDataLen == PERSNR_LEN) {
    	lPersNr = atol (pacScanData);
		memset (acPersNr, 0, sizeof (acPersNr));
		sprintf (acPersNr, "%ld", lPersNr);
		        /* Personalnummer checken */
        iCntPersNr = 0;

       	if(TExecSql( hMaskRl,
                        "SELECT COUNT(*) from UM_PERS WHERE"
                        " UM_PERS.PersNr =:a",
                        SELINT (iCntPersNr),
                        SQLSTRING (acPersNr),
                        NULL) < 0) {

			WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Transportauftrag"),
				 WboxNtext,
					StrForm (GetMlmErrTxt (MlM_DbError), TN_UM_PERS),
				 NULL);

			LogPrintf (GetTermFac(), LT_ALERT,
				"Error reading from UM_PERS. Error: %s",
				TSqlErrTxt(hMaskRl));

			TSqlRollback(hMaskRl);

			return (-1);
		}


        if (iCntPersNr < 1) {

            WamasBox (SHELL_OF (hMaskRl),
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  MlM("Listenkommission"),
                WboxNtext,
                    MlM("Personalnummer im System nicht vorhanden!"),
                NULL);

            LogPrintf (GetTermFac(), LT_ALERT,
                        "CbScan. PersNr: %s nicht bekannt.Error: %s",
                        acPersNr,
                       TSqlErrTxt(hMaskRl));

            TSqlRollback(hMaskRl);
			return (-1);

        }
		strncpy (ptKommAnAbCtx->acPersNr, acPersNr, PERSNR_LEN);
		TSqlRollback(hMaskRl);
		MskUpdateMaskVar (hMaskRl);
		return (1);
	}


    memset (&tVplk, 0, sizeof(VPLK));
    lKsNr = atol(pacScanData);
    tVplk.vplkKsNr = lKsNr;

      /* VPLK lesen und locken */
    if (ReadMyVplk (hMaskRl, GetTermFac(), &tVplk) <= 0) {

        TSqlRollback (hMaskRl);

        return (-1);

    }
  	if (tVplk.vplkVplArt == VPLKART_TPA) {

		  WamasBox (SHELL_OF (hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Kommissionsschein"),
				WboxNtext,
				StrForm (MlM
					("Kommissionsschein %ld ist"
					" Ganzpalettenauftrag."),
					tVplk.vplkKsNr),
				NULL);

			TSqlRollback (hMaskRl);
			return (-1);
	}

    if (tVplk.vplkStatus == VPLKSTATUS_BEREIT) {

        strncpy (ptKommAnAbCtx->acLazNr, tVplk.vplkLazNr, LAZNR_LEN);
        ptKommAnAbCtx->lKsNr = tVplk.vplkKsNr;
        ptKommAnAbCtx->AnAbMode = KOMMANAB_AN;

    }

    if (tVplk.vplkStatus == VPLKSTATUS_AKTIV) {

        if (tVplk.vplkKomArt == KOMARTLISTE) {

            strncpy (ptKommAnAbCtx->acLazNr, tVplk.vplkLazNr, LAZNR_LEN);
            strncpy (ptKommAnAbCtx->acPersNr,
										tVplk.vplkAktUser, USR_LEN);
            ptKommAnAbCtx->lKsNr = tVplk.vplkKsNr;
            ptKommAnAbCtx->AnAbMode = KOMMANAB_AB;

        }
    }

  	TSqlRollback(hMaskRl);

	MskUpdateMaskVar (hMaskRl);

	if ((ptVpluebCtx = 
			(VPLUEB_CTX *)MskRlMaskGet (ptKommAnAbCtx->hMaskRlVplueb,
										MskNmaskCalldata)) != NULL) {

		ListUpdateVplueb (ptKommAnAbCtx->hMaskRlVplueb, 
								ptVpluebCtx, 
								MANUELL_LEVEL);

		MskUpdateMaskVar (ptKommAnAbCtx->hMaskRlVplueb);
	}

    return (1);

} /* CbScan () */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
    Masken-Callback
-* RETURNS
-*--------------------------------------------------------------------------*/

static int CbMask (MskTmaskRlPtr hMaskRl, int reason)
{
	KOMANABCTX    	*ptKommAnAbCtx = NULL;

    switch (reason) {

    case MSK_CM:

		ptKommAnAbCtx = (KOMANABCTX*)MskRlMaskGet(hMaskRl,
                                                MskNmaskCalldata);

        EfcbmInstByName(hMaskRl, "Ok_F", KEY_DEF, CbAnAbmelden, NULL);
        EfcbmInstByName(hMaskRl, "Begleit_F", KEY_DEF, CbBegdruck, NULL);
	
		MskVaAssign(hMaskRl,        MskGetElement("VPLK_KsNr_t"),
                        MskNvariable,   (Value)&ptKommAnAbCtx->lKsNr,
                        NULL);
		MskVaAssign(hMaskRl,        MskGetElement("VPLK_AktUser_t"),
                        MskNvariable,   (Value)ptKommAnAbCtx->acPersNr,
                        NULL);
		MskVaAssign(hMaskRl,        MskGetElement("VPLK_LazNr_t"),
                        MskNvariable,   (Value)ptKommAnAbCtx->acLazNr,
                        NULL);

		MskVaAssign(hMaskRl,        MskGetElement("LAZK_DefTpmId_t"),
                        MskNvariable,   (Value)ptKommAnAbCtx->acTpmId,
                        NULL);
	 	/* fuer Scannerunterstuezung */
		addMskDesc(hMaskRl, "00", CbScan, 0);
		MskUpdateMaskVar (hMaskRl);

		/* Install Hotkeys */
        InstallHotkeys4Mask (hMaskRl, GetTermFac());
        (void )TSqlRollback (hMaskRl);
	
        break;

	case MSK_RA:

		ptKommAnAbCtx = (KOMANABCTX*)MskRlMaskGet(hMaskRl,
                                                MskNmaskCalldata);

		set_focus (hMaskRl, "VPLK_AktUser_t", KEY_DEF);

		break;

    case MSK_DM:

		ptKommAnAbCtx = (KOMANABCTX*)MskRlMaskGet(hMaskRl,
                                                MskNmaskCalldata);
        TSqlRollback (hMaskRl);

		delMskDesc(hMaskRl);

        if (ptKommAnAbCtx != NULL) {
            free (ptKommAnAbCtx);
            ptKommAnAbCtx = NULL;
        }
        break;

    default:
        break;
    }

    return (MSK_OK_TRANSFER);
}




/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
    Menu with (filled) structure as parameter

	pacLazNr -> lager zone ID -> input parameter
	pacTpmId -> Tpm ID -> output parameter

-* RETURNS
-*--------------------------------------------------------------------------*/
int GetLazTpmId(MskTmaskRlPtr hMaskRl,/*in*/ char *pacLazNr,/*out*/ char *pacTpmId) { 
	
	int iRv;

	if(pacLazNr != NULL && 
	  (IsEmptyStrg(pacLazNr) == JANEIN_N)) {

		memset (pacTpmId, 0, sizeof (TPMID_LEN + 1));
		iRv = TExecSql( hMaskRl,
					"SELECT DEFTPMID FROM LAZK"	
					" WHERE LAZK.LAZID_LAZTYP = " STR_LAZTYP_KOM
					" AND LAZK.LAZID_LAZNR = :a ",
					SELSTR (pacTpmId, TPMID_LEN+1),
					SQLSTRING (pacLazNr),
					NULL);
	
		TSqlRollback (hMaskRl);

		if (iRv <= 0 && TSqlError(hMaskRl) != SqlNotFound ) {


			LogPrintf (GetTermFac(), LT_ALERT, 
					"GetLazTpmId, LazNr: %s\n"
					"Error: %s ",
					pacLazNr,
					 TSqlErrTxt(hMaskRl));

			return(-1);
		}
	}

	return 0;
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
    Menu with (filled) structure as parameter
-* RETURNS
-*--------------------------------------------------------------------------*/
int _me_komanab (OWidget hWidget, KOMANABCTX *ptSourceCtx)
{
    MskDialog           hMaskRl = NULL;
    OWidget             hWidgetSh = NULL;
    KOMANABCTX       	*ptKommAnAbCtx;

	OwrcLoadObject (ApNconfigFile, "me_komanab.rc");

    if((hMaskRl = MskOpenMask (NULL, RC_NAME)) == NULL) {

        fprintf(stderr,"RC ERROR (%s)\n", RC_NAME);

        return(RETURN_ERROR);
    }

    if ((hWidgetSh = SHELL_OF(hMaskRl)) == NULL) {

        if ((ptKommAnAbCtx = malloc (sizeof (KOMANABCTX))) == NULL) {

            MskDestroy(hMaskRl);
            fprintf(stderr, "MALLOC ERROR (ptKommAnAbCtx)\n");
            return(RETURN_ERROR);
        }
		memset (ptKommAnAbCtx, 0, sizeof (KOMANABCTX));
        MskRlMaskSet (hMaskRl, MskNmaskCalldata,   (Value)ptKommAnAbCtx);
        MskRlMaskSet (hMaskRl, MskNmaskCallback,   (Value)CbMask);

    } else {

        ptKommAnAbCtx = (KOMANABCTX*)MskDialogGet(hMaskRl, MskNmaskCalldata);
    }

	if (ptSourceCtx != NULL) {
	
		memset(ptKommAnAbCtx->acTpmId, 0, sizeof(ptKommAnAbCtx->acTpmId));
		GetLazTpmId(hMaskRl, ptSourceCtx->acLazNr ,ptKommAnAbCtx->acTpmId);  

		strncpy (ptKommAnAbCtx->acPersNr,	ptSourceCtx->acPersNr, PERSNR_LEN);
		strncpy (ptKommAnAbCtx->acLazNr,	ptSourceCtx->acLazNr, LAZNR_LEN);
		ptKommAnAbCtx->lKsNr 			= ptSourceCtx->lKsNr;
		ptKommAnAbCtx->vplkKomArt 		= ptSourceCtx->vplkKomArt;
		ptKommAnAbCtx->vplkStatus 		= ptSourceCtx->vplkStatus;
		ptKommAnAbCtx->AnAbMode 		= ptSourceCtx->AnAbMode;
		ptKommAnAbCtx->hMaskRlVplueb 	= ptSourceCtx->hMaskRlVplueb;
	}

    /* Copy handles from first mask */
	if (ptSourceCtx == NULL) {
    	hWidgetSh = ApShellModelessCreate (hWidget, AP_CENTER, AP_CENTER);
	} else {
    	hWidgetSh = ApShellModalCreate (hWidget, AP_CENTER, AP_CENTER);
	}

    WamasWdgAssignMenu (hWidgetSh, RC_NAME);

    MskCreateDialog (hWidgetSh, hMaskRl, 
					MlM("Kommission an-/abmelden"), NULL, HSL_NI, SMB_All);

    MskUpdateMaskVar (hMaskRl);

	if (ptSourceCtx != NULL) {
		WdgMainLoop();
	}

    return (RETURN_ACCEPTED);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Entrypoint vom Mneutree
-* RETURNS
-*--------------------------------------------------------------------------*/
int me_komanab(MskDialog hMaskRl, MskStatic ef, MskElement el,
                    int reason, void *cbc)
{
    OWidget    hOWidget = GetRootShell();

    switch (reason) {
    case FCB_EF:
         _me_komanab(hOWidget, NULL);
        break;

    default:
        break;
    }

    return (0);

}  /*  me_komanab()  */

