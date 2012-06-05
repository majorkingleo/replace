/*****************************************************************************
+* PROJECT:   SCHLAU
+* PACKAGE:   package name
+* FILE:      me_taetst.c
+* CONTENTS:  overview description, list of functions, ...
+* COPYRIGHT NOTICE:
+*         (c) Copyright 2001 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+*
+****************************************************************************/

/* ===========================================================================
 * INCLUDES
 * =========================================================================*/

/* ------- System-Headers ------------------------------------------------- */

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

/* ------- Owil-Headers --------------------------------------------------- */

#include <owil.h>
#include <dbsqlstd.h>
#include <wamasbox.h>
#include <wamaswdg.h>
#include <module/owss.h>
#include <module/owgr.h>
#include <module/owrd.h>

/* ------- Tools-Headers -------------------------------------------------- */

#include <efcbm.h>
#include <logtool.h>
#include <owrcloader.h>
#include <ml.h>
#include <t_util.h>
#include <makefilter.h>
#include <errmsg.h>
#include <ml_util.h>

#include <wamas.h>


/* ------- Local-Headers -------------------------------------------------- */

#include "hist_util.h"
#include "sqllist.h"
#include "term_util.h"
#include "proc.h"
#include "menufct.h"
#include <logtool.h>

#include "prm_util.h"
#include "parameter.h"
#include "wampers.h"
#include <facility.h>
#include "me_taetst.h"
#include <mrad.h>
#include <prexec.h>
#include <me_printer.h>
#include "init_table.h"



/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define RC_NAME				"ME_TAETST"
#define BLOCKSIZE_TAET		400
#define FAC_ME_TAETST		"ME_TAETST"

#define TAG					1
#define WOCHE				2
#define MONAT				3
#define LINE				1
#define BARLINE				2
#define PIE					3
#define BAR					4

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

typedef struct _TTaetStCtx {
	int		iSumme;
	int		iZeitraum;
	char	cPersNr[10+1];
	int		iDiagramtyp;
	long	ttDatumVon, ttDatumBis;
	int		iKurventyp;
    ListBuffer  Lb;
} TTaetStCtx;

typedef struct _LbfRecTaetst {				/* Listbuffer data */
	TAEST	tTaest;
}LBFREC_TAETST;

static DdTableRec data_taet[] = {
    {"taet_sizeofLbEntry", (Value )sizeof(LBFREC_TAETST)},
    {"offs_taetBez",  (Value)MemberOffset(LBFREC_TAETST,tTaest.taestTaetBez)},
    {"offs_taetId",   (Value)MemberOffset(LBFREC_TAETST,tTaest.taestTaetId)},
   {NULL}
};
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

/*----------------------------------------------------------------------------
-* Set Focus 
-*--------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$
-* static int CbCancel_taetst(MskDialog hMaskRl,  MskStatic hEf, 
-*  			MskElement hEfRl, int iReason, void *pvCbc, void *pvCalldata)
-* DESCRIPTION
-*      Callback for Cancel-Button
-*      Parameter:
-*          Std. Owil-Parameter for Button-Callbacks
-* RETURNS
-*      ever EFCBM_CONTINUE
-*--------------------------------------------------------------------------*/



static int CbCancel_taetst(MskDialog hMaskRl,  MskStatic hEf, MskElement hEfRl,
                int iReason, void *pvCbc, void *pvCalldata)
{

    switch (iReason) {
    case FCB_XF:
        MskCbClose(hMaskRl, hEf, hEfRl, iReason, pvCbc);

    default:
        break;
    }
    return(EFCBM_CONTINUE);
}



static int CbDiagTyp (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
						int iReason, void *pvCbc,void *pvCalldata) 
{
	TTaetStCtx		*ptMc;
	OwObject		cglob;
    OwObject		chart;
	OWidget     	OWd;	
	MskElement      canvas, spread;
	SsHandle        ss;
	
    switch(iReason) {
        case FCB_XF:
			MskDialogSet(hMaskRl,MskNmaskTransferDup2Var,(Value )TRUE);

			canvas = MskQueryRl(hMaskRl,
								MskGetElement("CanvasChart"), 
								KEY_DEF);

			cglob = (OwObject )
					(canvas?MskElementGet(canvas,MskNclientData):VNULL);
			
			if (cglob) {

				if ((ptMc = (TTaetStCtx *) 
						MskRlMaskGet(hMaskRl, MskNmaskCalldata)) == NULL) {
					return EFCBM_CONTINUE;
				}

				LB_DeSellectAll (ptMc->Lb);
				chart   = (OwObject )GrGet(cglob,GrNchartActiveChart);

				spread = MskQueryRl(hMaskRl,
							MskGetElement("ListBuffer_me_taetst"),
							KEY_DEF);

				ss = (SsHandle) MskElementGet (spread, MskNclientData);

				switch (ptMc->iDiagramtyp) {
					case LINE:
						GrSet(chart,GrNchartKind, OWCHART_Line);
						if (ss) {
							SsSet(ss,SsNselectionPolicy,
									(Value)SS_SelPolicySingleSelect);
						}
						ptMc->iSumme = 0;
					break;
					case BARLINE:
						GrSet(chart,GrNchartKind, OWCHART_BarLine);
						ptMc->iSumme = 1;
						if (ss) {
							SsSet(ss,SsNselectionPolicy,
									(Value)SS_SelPolicySingleSelect);
						}
					break;
					case PIE:
						GrSet(chart,GrNchartKind, OWCHART_Pie);
						ptMc->iSumme = 1;
						if (ss) {
							SsSet(ss,SsNselectionPolicy,
									(Value)SS_SelPolicyToggleSelect);
						}
					break;
					case BAR:
						GrSet(chart,GrNchartKind, OWCHART_Bar);
						if (ss) {
							SsSet(ss,SsNselectionPolicy,
									(Value)SS_SelPolicyToggleSelect);
						}
						ptMc->iSumme = 1;
					break;
					default:
						if (ss) {
							SsSet(ss,SsNselectionPolicy,
									(Value)SS_SelPolicySingleSelect);
						}
						GrSet(chart,GrNchartKind, OWCHART_Line);
						ptMc->iSumme = 0;
					break;
				}
			}

			GrSet(chart,GrNchartRedrawHint, OWCHART_RHINT_ALL);

			/* Chart updaten ... */
            GrVaSet(chart,
                GrNchartUpdateLayout,       (Value )TRUE,
                GrNchartUpdateData,         (Value )TRUE,
                GrNchartUpdateScale,        (Value )TRUE,
			NULL);

			/* Canvas neu zeichnen */
			OWd = (OWidget )MskElementGet (canvas, MskNwMain);
			WdgSingleUserRedrawEvent (OWd);
		break;
	default:
		break;
	}

	return (EFCBM_CONTINUE);
} /* CbDiagTyp() */

static int CbKurvTyp (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
						int iReason, void *pvCbc,void *pvCalldata) 
{
	TTaetStCtx		*ptMc;
	OwObject		cglob;
    OwObject		chart;
	OWidget     	OWd;	
	MskElement      canvas;
	
    switch(iReason) {
        case FCB_XF:
			MskDialogSet(hMaskRl,MskNmaskTransferDup2Var,(Value )TRUE);

			canvas = MskQueryRl(hMaskRl,
								MskGetElement("CanvasChart"), 
								KEY_DEF);

			cglob = (OwObject )
					(canvas?MskElementGet(canvas,MskNclientData):VNULL);
			
			if (cglob) {

				if ((ptMc = (TTaetStCtx *) 
						MskRlMaskGet(hMaskRl, MskNmaskCalldata)) == NULL) {
					return EFCBM_CONTINUE;
				}

				chart   = (OwObject )GrGet(cglob,GrNchartActiveChart);

				switch (ptMc->iKurventyp) {
					case 1:
						GrSet(chart,GrNchartCurveType, OWCHART_CTYPE_Line);
					break;
					case 2:
						GrSet(chart,GrNchartCurveType, OWCHART_CTYPE_Spline);
					break;
					case 3:
						GrSet(chart,GrNchartCurveType, OWCHART_CTYPE_Points);
					break;
					default:
						GrSet(chart,GrNchartCurveType, OWCHART_CTYPE_Line);
					break;
				}
			}

			GrSet(chart,GrNchartRedrawHint, OWCHART_RHINT_ALL);

			/* Chart updaten ... */
            GrVaSet(chart,
                GrNchartUpdateLayout,       (Value )TRUE,
                GrNchartUpdateData,         (Value )TRUE,
                GrNchartUpdateScale,        (Value )TRUE,
			NULL);

			/* Canvas neu zeichnen */
			OWd = (OWidget )MskElementGet (canvas, MskNwMain);
			WdgSingleUserRedrawEvent (OWd);
	break;
	default:
	break;
	}

	return (EFCBM_CONTINUE);
} /* CbKurvTyp() */

static void GetWhereAnd (char *pcStmt)
{
	if (strstr (pcStmt, "WHERE") == NULL) {
		strcat (pcStmt, " WHERE");
	} else {
		strcat (pcStmt, " AND");
	}
	return;
}

static int GetTaet (MskDialog hMaskRl, char *pcTaet)
{
	TTaetStCtx      *ptMc;
	LBFREC_TAETST   *ptLbfTaetRec;
	ListElement     hLiEle;
	int				iAnzTaet, nI;
	char			acTaet[60];

	ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);

	iAnzTaet = LBNumberOfSelected(ptMc->Lb);
	if (iAnzTaet == 0 && (ptMc->iDiagramtyp == PIE ||
						  ptMc->iDiagramtyp == BAR)) {
		LB_SellectAll (ptMc->Lb);
		MskUpdateMaskVar (hMaskRl);
	} 
	if (iAnzTaet == 0 &&(ptMc->iDiagramtyp == LINE ||
						ptMc->iDiagramtyp == BARLINE)) {
		WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
				WboxNtext,
				MlM("Bitte eine Tätigkeit selektieren!\n"),
				NULL);
		return (-1);
	}
	for (nI = 0; nI <= ListBufferLastElement(ptMc->Lb);nI++) {
		memset (acTaet, 0, sizeof (acTaet));
		hLiEle = ListBufferGetElement (ptMc->Lb,nI);
		ptLbfTaetRec = (LBFREC_TAETST *)hLiEle->data;
		if ((hLiEle->hint & LIST_HINT_SELECTED)) {
			if (strlen (pcTaet) == 0) {
				sprintf (acTaet,"'%s'", ptLbfTaetRec->tTaest.taestTaetId);
			} else {
				sprintf (acTaet,",'%s'", ptLbfTaetRec->tTaest.taestTaetId);
			}
			strcat (pcTaet,acTaet);
		}
	}
	return iAnzTaet;
}

static int FillChartTaet (MskDialog hMaskRl, OwObject objChart)
{
	TTaetStCtx		*ptMc;
	int				iRv = 0, iAnzTaet = 0, nI = 0;
	double			*data;
	long			lData[BLOCKSIZE_TAET], lDatatmp[BLOCKSIZE_TAET];
	char			acTaetId[BLOCKSIZE_TAET][TAETID_LEN+1];
	char			acStmt[800], acStmtSel[150];
	char			acFilter[160], acTaetStmt[80];
	char			acStmtSelCount [100], acStmtCount[400];
	char            **c_labels;
	long			lAnz=0;
	long			lMaxWert = 0;


	ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
	MskTransferMaskDup (hMaskRl);
	memset (lDatatmp, 0, sizeof (lDatatmp));
	memset (acTaetId, 0, sizeof (acTaetId));
	memset (lData, 0, sizeof (lData));
	memset (acFilter, 0, sizeof (acFilter));

	fprintf (stderr, "Typ = %d, Zeit = %d",ptMc->iDiagramtyp, ptMc->iZeitraum);
	switch (ptMc->iDiagramtyp) {
	case BAR:	/* Summe aus FERTZEIT - STARTZEIT */
	case PIE:

		switch (ptMc->iZeitraum) {
		case 1: /* Tag */

			strcpy (acStmtSel, "SELECT SUM (FERTZEIT-STARTZEIT),"
							" TAETID FROM TAEPT");

			acStmt[0] = '\0';
			strcat (acStmt, acStmtSel);

			acFilter[0] = '\0';
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPT, FN_StartZeit,
				ptMc->ttDatumVon,
				ptMc->ttDatumBis),
				FILTER_END) ) {
					return (-1);
			}
			strcat (acStmt, acFilter);
			GetWhereAnd (acStmt);

			acFilter[0] = '\0';
			acTaetStmt[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
					
			if (ptMc->cPersNr[0] != '\0') {
				GetWhereAnd (acStmt);
				sprintf (acFilter, " PERSNR = '%s'", ptMc->cPersNr);
				strcat(acStmt, acFilter);
			}
					
			strcat (acStmt, " GROUP BY TAETID");


			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lData[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
			}

			GrSet (objChart, GrNchartArrayCount, iRv);
			GrSet (objChart, GrNchartArraySize, 1);
			c_labels = (char **) GrGet (objChart, GrNchartArrayLXaxisString);

			for (nI = 0; nI < iRv; nI++) {
				GrSet(objChart,GrNchartArrayActive,nI);
				GrSet(objChart, GrNchartLabel,
						 (Value )StrForm(" %s ", acTaetId[nI]));
				data    = (double *) GrGet (objChart,GrNchartArray);
				if (data) {
					if (lData[nI] < 0) {
						lData[nI] = 0;
					}
					data[nI] = lData[nI];
					if (c_labels) {
						 c_labels[nI] = StrUpdate(c_labels[nI],acTaetId[nI]);
					}
				}
			}
		break;

		case 2: /* Woche */
			memset (acTaetId, 0, sizeof (acTaetId));

			strcpy (acStmtSel, "SELECT SUM (TAEPWP.DAUER),"
							" TAEPWP.TAETID FROM TAEPWK, TAEPWP");

			acStmt[0] = '\0';
			strcat (acStmt, acStmtSel);

			acFilter[0] = '\0';
			memset (acFilter, 0, sizeof (acFilter));
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPWP, FN_Datum,
					ptMc->ttDatumVon,
					ptMc->ttDatumBis),
				FilterString (TN_TAEPWP, FN_PersNr,
					ptMc->cPersNr,0),
				FILTER_END )) {
					return (-1);
			}
			strcat (acStmt, acFilter);

			GetWhereAnd (acStmt);

			acFilter[0] = '\0';
			acTaetStmt[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
					
			strcat (acStmt,
				" GROUP BY TAEPWP.TAETID");

			memset (lData, 0, sizeof (lData));

			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lData[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}

			GrSet (objChart, GrNchartArrayCount, iRv);
			GrSet (objChart, GrNchartArraySize, 1);
			for (nI = 1; nI < iRv; nI++) {
				GrSet(objChart,GrNchartArrayActive,nI);
				GrSet(objChart, GrNchartLabel,
						 (Value )StrForm(" %s ", acTaetId[nI]));
				data    = (double *) GrGet (objChart,GrNchartArray);
				if (data) {
					if (lData[nI] < 0) {
						lData[nI] = 0;
					}
					*data = lData[nI];
				}
			}
		break;

		case 3: /* Monat */
			memset (acTaetId, 0, sizeof (acTaetId));

			strcpy (acStmtSel, "SELECT SUM (TAEPMP.DAUER),"
							" TAEPMP.TAETID FROM TAEPMK, TAEPMP");

			acStmt[0] = '\0';
			strcat (acStmt, acStmtSel);

			acFilter[0] = '\0';
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPWP, FN_Datum,
					ptMc->ttDatumVon,
					ptMc->ttDatumBis),
				FilterString (TN_TAEPWP, FN_PersNr,
					ptMc->cPersNr,0),
				FILTER_END )) {
					return (-1);
			}
			GetWhereAnd (acStmt);

			acFilter[0] = '\0';
			acTaetStmt[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
					
			if (ptMc->cPersNr[0] != '\0') {
				GetWhereAnd (acStmt);
				sprintf (acFilter, " TAEPMP.PERSNR = '%s'", ptMc->cPersNr);
				strcat(acStmt, acFilter);
			}
					
			strcat (acStmt,
				" GROUP BY TAEPMP.TAETID");

			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lData[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}

			GrSet (objChart, GrNchartArrayCount, iRv);
			GrSet (objChart, GrNchartArraySize, 1);
			for (nI = 0; nI < iRv; nI++) {
				GrSet(objChart,GrNchartArrayActive,nI);
				GrSet(objChart, GrNchartLabel,
						 (Value )StrForm(" %s ", acTaetId[nI]));
				data    = (double *) GrGet (objChart,GrNchartArray);
				if (data) {
					if (lData[nI] < 0) {
						lData[nI] = 0;
					}
					data[nI] = lData[nI];
				}
			}
		break;
		}
	break;
	case LINE:
	case BARLINE:
		MskTransferMaskDup (hMaskRl);
		switch (ptMc->iZeitraum) {
		case 1: /* Tag */
			strcpy (acStmtSel,
				"select trunc(trunc(((startzeit-trunc(startzeit))*24*60))/5),"
				"TAETID, ((FERTZEIT-STARTZEIT)*24) FROM TAEPT");

			strcpy (acStmtSelCount, "SELECT COUNT(TAETID) FROM TAEPT");

			acStmt[0] = '\0';
			acStmtCount[0] = '\0';
			strcat (acStmt, acStmtSel);
			strcat (acStmtCount, acStmtSelCount);

			acFilter[0] = '\0';
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPT, FN_StartZeit,
					ptMc->ttDatumVon,
					ptMc->ttDatumBis),
				FILTER_END )) {
					return (-1);
			}

			strcat (acStmt, acFilter);
			strcat (acStmtCount, acFilter);

			if (ptMc->cPersNr[0] != '\0') {
				GetWhereAnd (acStmt);
				GetWhereAnd (acStmtCount);
				sprintf (acFilter, " TAEPT.PERSNR = '%s'", ptMc->cPersNr);
				strcat (acStmt, acFilter);
				strcat (acStmtCount, acFilter);
			}


			GetWhereAnd (acStmt);
			GetWhereAnd (acStmtCount);

			acTaetStmt[0] = '\0';
			acFilter[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
			strcat (acStmtCount, acTaetStmt);
					
			strcat (acStmt," Group by trunc(trunc(((startzeit-trunc(startzeit))"
							"*24*60))/5), TAETID, (FERTZEIT-STARTZEIT)");
			memset (lData, 0, sizeof (lData));


			iRv = TExecSql (hMaskRl, acStmtCount,
								SELLONG (lAnz),
								NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
			if (lAnz > 1000) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Bitte mit DATUM einschränken!\n"),
						NULL);
				return (-1);
			}

			iRv = 0;

			memset (lDatatmp, 0, sizeof (lDatatmp));
			memset (acTaetId, 0, sizeof (acTaetId));
			memset (lData, 0, sizeof (lData));

			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lDatatmp[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 SELLONG(lData[0]),
							 NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}

			GrSet (objChart, GrNchartArraySize, lAnz);
			GrSet(objChart, GrNchartLabel,
					 (Value )StrForm(" %s ", acTaetId[0]));

			GrSet (objChart, GrNchartArrayCount, 1);
			GrSet(objChart,GrNchartArrayLXaxisString,(Value)TRUE);
			data    = (double *) GrGet (objChart,GrNchartArray);
			c_labels = (char **) GrGet (objChart, GrNchartArrayLXaxisString);
			if (data) {
				for (nI = 0; nI < iRv; nI++) {
					GrSet(objChart,GrNchartArrayActive,1);
					if (lData[nI] > lMaxWert) {
						lMaxWert = lData[nI];
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%d", lData[nI]));
						}
					} else {
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%s", "  "));
						}
					}
					if (lData[nI] < 0 ) {
						data[nI] = 0;
					} else {
						data[nI] = lData[nI];
					}
				}
			}
		break;

		case 2: /* Woche */
			strcpy (acStmtSel, "SELECT TAEPWP.DAUER/3600,"
							" TAEPWP.TAETID FROM TAEPWK, TAEPWP");

			strcpy (acStmtSelCount, "SELECT COUNT(TAETID) FROM TAEPWP");

			acStmt[0] = '\0';
			acStmtCount[0] = '\0';
			strcat (acStmt, acStmtSel);
			strcat (acStmtCount, acStmtSelCount);

			acFilter[0] = '\0';
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPWP, FN_Datum,
					ptMc->ttDatumVon,
					ptMc->ttDatumBis),
				FILTER_END )) {
					return (-1);
			}

			strcat (acStmt, acFilter);
			strcat (acStmtCount, acFilter);

			if (ptMc->cPersNr[0] != '\0') {
				GetWhereAnd (acStmt);
				GetWhereAnd (acStmtCount);
				sprintf (acFilter, " TAEPWP.PERSNR = '%s'", ptMc->cPersNr);
				strcat (acStmt, acFilter);
				strcat (acStmtCount, acFilter);
			}


			GetWhereAnd (acStmt);
			GetWhereAnd (acStmtCount);

			acTaetStmt[0] = '\0';
			acFilter[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
			strcat (acStmtCount, acTaetStmt);

			iRv = TExecSql (hMaskRl, acStmtCount,
								SELLONG (lAnz),
								NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
			if (lAnz > 1000) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Bitte mit DATUM einschränken!\n"),
						NULL);
				return (-1);
			}

			iRv = 0;

			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lData[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 NULL);


			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
			GrSet (objChart, GrNchartArraySize, iRv);
			GrSet(objChart, GrNchartLabel,
					 (Value )StrForm(" %s ", acTaetId[0]));

			GrSet (objChart, GrNchartArrayCount, 1);
			GrSet(objChart,GrNchartArrayActive,1);
			data    = (double *) GrGet (objChart,GrNchartArray);
			c_labels = (char **) GrGet (objChart, GrNchartArrayLXaxisString);
			if (data) {
				for (nI = 0; nI < iRv; nI++) {
					if (lData[nI] > lMaxWert) {
						lMaxWert = lData[nI];
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%d", lData[nI]));
						}
					} else {
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%s", "  "));
						}
					}
					if (lData[nI] < 0 ) {
						data[nI] = 0;
					} else {
						data[nI] = lData[nI];
					}
				}
			}
		break;

		case 3: /* Monat */
			strcpy (acStmtSel, "SELECT TAEPMP.DAUER/3600,"
							" TAEPMP.TAETID FROM TAEPMK, TAEPMP");

			strcpy (acStmtSelCount, "SELECT COUNT(TAETID) FROM TAEPMP");

			acStmt[0] = '\0';
			acStmtCount[0] = '\0';
			strcat (acStmt, acStmtSel);
			strcat (acStmtCount, acStmtSelCount);

			acFilter[0] = '\0';
			if ( -1 == MakeFilter (hMaskRl, acFilter,
				FilterDate (TN_TAEPMP, FN_Datum,
					ptMc->ttDatumVon,
					ptMc->ttDatumBis),
				FILTER_END )) {
					return (-1);
			}

			strcat (acStmt, acFilter);
			strcat (acStmtCount, acFilter);

			if (ptMc->cPersNr[0] != '\0') {
				GetWhereAnd (acStmt);
				GetWhereAnd (acStmtCount);
				sprintf (acFilter, " TAEPMP.PERSNR = '%s'", ptMc->cPersNr);
				strcat (acStmt, acFilter);
				strcat (acStmtCount, acFilter);
			}


			GetWhereAnd (acStmt);
			GetWhereAnd (acStmtCount);

			acTaetStmt[0] = '\0';
			acFilter[0] = '\0';

			iAnzTaet = GetTaet (hMaskRl, acFilter);
			if (iAnzTaet < 0) {
				return (-1);
			}

			sprintf (acTaetStmt," TAETID IN (%s)", acFilter);
			strcat (acStmt, acTaetStmt);
			strcat (acStmtCount, acTaetStmt);
					
			iRv = TExecSql (hMaskRl, acStmtCount,
								SELLONG (lAnz),
								NULL);

			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
			if (lAnz > 1000) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Bitte mit DATUM einschränken!\n"),
						NULL);
				return (-1);
			}

			iRv = 0;

			iRv = TExecSqlX (hMaskRl, NULL, acStmt,
							 BLOCKSIZE_TAET,0,
							 SELLONG(lData[0]),
							 SELSTR(acTaetId[0], TAETID_LEN+1),
							 NULL);


			if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
				WamasBox(SHELL_OF(hMaskRl),
						WboxNboxType,   WBOX_ALERT,
						WboxNbutton,    WboxbNok,
						WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
						WboxNtext,
						MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
						NULL);

				LogPrintf(FAC_ME_TAETST, LT_ALERT,
					MlM("TAEST = TSqlErr: %s"),
					TSqlErrTxt(hMaskRl));
				TSqlRollback(hMaskRl);
				return (-1);
			}
			GrSet (objChart, GrNchartArraySize, iRv);
			GrSet(objChart, GrNchartLabel,
					 (Value )StrForm(" %s ", acTaetId[0]));

			GrSet (objChart, GrNchartArrayCount, 1);
			GrSet(objChart,GrNchartArrayActive,1);
			data    = (double *) GrGet (objChart,GrNchartArray);
			c_labels = (char **) GrGet (objChart, GrNchartArrayLXaxisString);
			if (data) {
				for (nI = 0; nI < iRv; nI++) {
					if (lData[nI] > lMaxWert) {
						lMaxWert = lData[nI];
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%d", lData[nI]));
						}
					} else {
						if (c_labels) {
							c_labels[nI] = StrUpdate(c_labels[nI], 
												StrForm ("%s", "  "));
						}
					}
					if (lData[nI] < 0 ) {
						data[nI] = 0;
					} else {
						data[nI] = lData[nI];
					}
				}
			}
			break;
		}
	break;
	}


	return (iRv);
}

#define NAME_REPORT             "MyReport"
#define FNAME_RLAYOUTS          "mrep.lrc"



static int CbReport_taetst(MskDialog hMaskRl,MskStatic ef,MskElement el,
						   int reason, void *cbc)
{
	OWidget			shell;
    RdHandle 		rdh;
	RdLayoutReport 	rdlayout;
	RdLayoutBlock 	rdblock_row;
	RdLayoutField 	rdfield=NULL;
	OwObject  		globchart,chart; 
	RdLayoutBox  	rdbox_row;
	int				portrait = 0, rv = 0;
    Container   	cont;
	MskDialog 		m_print;
	TTaetStCtx      *ptMc;

    switch (reason) {
    case FCB_XF:
		ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		MskTransferMaskDup (hMaskRl);

        rdh     = NULL;
		rdh = (RdHandle)RdReportVaCreateNew ("MyReport",
				RdNpreviewHPages,  (Value )FALSE,
				RdNpreviewLandscape,  (Value )portrait,
				RdNpreviewTextReport,  (Value )FALSE,
				RdNpreviewQuick,  (Value )TRUE,
				RdNpreviewGridPerc,  (Value )TRUE,
				RdNplPercentage, (Value) 100,
				NULL);

		if (rdh == NULL) {
			return (-1);
		}

		rdlayout = (RdLayoutReport)RdLayoutReportFromReport(rdh);

		rv = RdLayoutSectionCreate(rdlayout, "SECTION" );
		if (rv == TRUE) {
			/* 
			 *  dem RdHandle mitteilen, dass jetzt die kreierte 
			 *      Section befüttert wird
			 */
		   RdFeedSelectSection(rdh,"SECTION");
		}
		/*
		 * erzeuge einen Detail-Block in der Section, 
		 *		der meine RdHBox enthalten soll.
		 */

		rdblock_row = RdLayoutBlockCreate(rdlayout,"SECTION",
					RdBlockDetail,"DETAIL"); 

		/*
		 * Eine horizontale oder vertik. 
		 *	Box erzeugen, mit verschiedenen Attributen
		 * RdNboxGeometry=GEOM_H_ALIGN fuer HBOX, GEOM_V_ALIGN fuer VBOX
		 */

		rdbox_row = RdLayoutBoxVaCreate(rdblock_row,NULL,
			  RdNboxName,  (Value) "BOX",    /* hbox beliebig */
			  RdNboxGeometry,  (Value)GEOM_H_ALIGN,  /* horizontal */
			  RdNboxWSize,  (Value)GEOM_W_SHRINK,
			  RdNboxHSize,  (Value)GEOM_H_SHRINK,
			  RdNboxBackground, (Value)C_LIGHTBLUE, /* erzeugte Color */
			  RdNboxW,  (Value)20,         /* in Pixel */
			  RdNboxH,  (Value)20,        /* in Pixel */
			  RdNboxGrid,  (Value)RD_GridThick,  /* Rahmen */
			  RdNboxGridPerc,  (Value)1,             /* Rahmenstaerke 0..9 */
			  NULL);

		RdLayoutFontVaCreate(rdlayout, "Arial", 
				/* font beliebig */
				RdNfontFamily, (Value)"Arial",
				RdNfontPoints, (Value)12,
				RdNfontAttr, (Value)RD_FontNormal,
				NULL);
		 /*
		  *		!!!! Wichtig einen zweiten Font mit gleichem Namen wie der 
		  *			erste Font,
	      *			allderdings mit vorangestelltem _ fuer den Drucker
		  */

		 RdLayoutFontVaCreate(rdlayout, StrForm("_%s","Arial"),
			  RdNfontFamily, (Value)"Arial",
			  RdNfontPoints, (Value)12,
			  RdNfontAttr, (Value)RD_FontNormal,
			  NULL);

			rdfield = RdLayoutFieldVaCreate(rdblock_row,rdbox_row,
				RdNfieldName,  (Value)"FIELD",
				RdNfieldType,  (Value)RD_FT_Chart,
				RdNfieldCustomWidth, (Value)100,
				RdNfieldCustomHeight, (Value)100,
				RdNfieldContainer, (Value)TRUE,
				NULL);
		/* 
		 *    Den Chartfield-Container mit einigen Defaultwerten befuellen 
		 * 	  Falls einige Werte schon feststehen, koennen diese zu diesem
	     *	  Zeitpunkt natuerlich auch gefuettert werden,
         *    wie Fonts fuer X-achsenlabels,horiontale Grids,....
		 */

		cont = (Container)RdLayoutFieldGet(rdfield,RdNfieldContainer);
		ContAddXAtom(cont,ATOM_Chart,"MyChart",
					CONT_MODE_DEFAULT,0,(Value)"MyChart");

		ContAddXAtom(cont,ATOM_ChartCount,
					StrForm("%d",1),CONT_MODE_DEFAULT,0,(Value)1);

/*
		ContAddAtom(cont,ATOM_ChartTitle,StrForm ("%s","Tätigkeitsstatistik"));
*/

		 /* 
		  * 	!!! OwChartKind,....
		  * !!! Wichtig damit zumindest ein Array im Speicher erzeugt wird
		  */
		ContAddXAtom(cont,ATOM_ChartArrayCount,
					StrForm("%d",1),CONT_MODE_DEFAULT,0,(Value)1);

		ContAddXAtom(cont,ATOM_ChartKind,
					StrForm("%d",ptMc->iDiagramtyp),
					CONT_MODE_DEFAULT,0,(Value)4);
		/*
		 *		dem RdHandle mitteilen, das nun der befuellte Detailblock zum
		 *		Zuge kommt ---- 
		 */

		RdFeedAddBlock(rdh,RdBlockDetail,"DETAIL");
/*
		RdFeedField(rdh,RdBlockDetail,"FIELD");
*/



		/* 
		 *	--- nun ist das Chart erzeugt und der Pointer darauf kann geholt
		 *		werden ---- 
		 */

		globchart = (OwObject)RdFeedFieldInquireObject(rdh,RdBlockDetail,
					"FIELD");

		chart = (OwObject)GrGet(globchart,GrNchartActiveChart);
		GrSet ( chart, GrNchartAreaMainW, (Value) 300);
		GrSet ( chart, GrNchartAreaMainH, (Value) 300);
		GrSet(chart,GrNchartTitle,
					(Value) "Taetigkeitsstatistik ");

/*
		my_chart = (MskElement)MskQueryRl (hMaskRl, MskGetElement("MyChart")
											, KEY_DEF); 
		my_object = (OwObject) (my_chart?MskElementGet(my_chart,
														MskNclientData):VNULL);

		my_activ = (OwObject )GrGet(my_object, GrNchartActiveChart);
		GrSet (my_activ, GrNchartArrayActive, 0);

		GrSet (chart, GrNchartArrayCount, 1);
		GrSet (chart, GrNchartArraySize, 10);

		my_data1 = (double *)GrGet (chart, GrNchartArray);
		GrSet (chart, GrNchartArrayActive, 0);	

		for (nI = 0;nI <10;nI ++) {
			my_data1 = nI;
		}
*/
/*
		GrSet (chart, GrNchartArray, (Value) my_data1);
*/
		
		/* 
		 *		Fuer den sofortigen Druck muss bevor der Owil-Druckdialog
		 *		geoeffnet wird, das Atom:ATOM_RdPrintImmediate=TRUE gesetzt 
		 *		werden
		 */

		RdPrinterParmsAdd(hMaskRl,el,NULL,ATOM_RdHandle,NULL,(Value )rdh);
		GrContextVaCreate("lpr",
						  GrcNprinterDriver,    (Value )"*POSTSCRIPT",
						  GrcNprinterDevice,    (Value )"/tmp/kzung.prn",
						  GrcNprinterOutput,    (Value )NULL,
						  GrcNprinterLandscape, (Value )0,
						  GrcNprinter,          (Value )NULL,
						  NULL);

		ApAtomGet(ATOM_RdNameGrc);
		ApAtomRemove(ATOM_RdNameGrc, FALSE);
		ApAtomAdd(ATOM_RdNameGrc, "lpr", VNULL);

		MskAtomAdd(hMaskRl,NULL,ATOM_RdPrintImmediate,StrForm("%d",1),VNULL);

		m_print = MskOpenMask(NULL,"OwRdMskPrint");
		shell=ApShellModalCreate(SHELL_OF(hMaskRl),AP_CENTER,AP_CENTER);
		MskDialogSet(m_print,MskNmaskClientData,(Value)rdh);
		RdPrintDialog( shell, rdh);
		if (MskRcCreateDialog(shell,m_print)) {
			WdgMainLoop();
		}

        break;
    }
	return (1);
}

static int CbPrnConfig (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
						int iReason, void *pvCbc,void *pvCalldata) 
{
	int iRv = 0;

    switch(iReason) {
        case FCB_XF:

		iRv = PrnDrucken (GetRootShell(), hMaskRl,
			PURPOSE_LISTEN,"/tmp/kzung.prn", 1, PRNBOX_SEL, hMaskRl);

		if (iRv <= 0) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_ALERT,
					WboxNbutton,    WboxbNok,
					WboxNmwmTitle,  MlM("Tätigkeitsstatistik Drucken"),
					WboxNtext,     MlM("Fehler beim Drucken!"),
					NULL);
			return(1);
		}
		break;
		default:
		break;
	}
	return (0);
}

static int CbOk (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
						int iReason, void *pvCbc,void *pvCalldata) 
{
	OwObject		cglob;
    OwObject		chart;
	OWidget     	OWd;	
	MskElement      canvas;
	
    switch(iReason) {
        case FCB_XF:
			MskDialogSet(hMaskRl,MskNmaskTransferDup2Var,(Value )TRUE);


			/* Daten aus der Datenbank lesen und aufbereiten ... */

			canvas = MskQueryRl(hMaskRl,
								MskGetElement("CanvasChart"), 
								KEY_DEF);

			cglob = (OwObject )
					(canvas?MskElementGet(canvas,MskNclientData):VNULL);
			
			if (cglob) {
				chart   = (OwObject )GrGet(cglob,GrNchartActiveChart);

				/* Daten in den Chart schreiben und anzeigen ... */
                GrSet(chart, GrNchartDrawLegend, (Value )TRUE);
                GrSet(chart, GrNchartLegendHoriz, (Value )TRUE);

				GrSet(chart,GrNchartTitle,
							(Value) "Taetigkeitsstatistik ");

				GrSet(chart,GrNchartArrayLXaxisString,(Value)TRUE);

				GrSet(chart,GrNchartColorNameTitle,(Value)C_BLUE);
				GrSet(chart,GrNchartRedrawHint, OWCHART_RHINT_ALL);
				GrSet(chart,GrNchartDrawVGrid, (Value) FALSE);
				GrSet(chart,GrNchartDrawHGrid, (Value) FALSE);

				FillChartTaet (hMaskRl, chart);

				
				/* Chart updaten ... */
				GrVaSet(chart, GrNchartUpdateLayout, (Value )TRUE, NULL);
				GrVaSet(chart, GrNchartUpdateData,(Value )TRUE, NULL);
				GrVaSet(chart,	GrNchartUpdateScale, (Value )TRUE, NULL);

				/* Canvas neu zeichnen */
				OWd = (OWidget )MskElementGet (canvas, MskNwMain);
				WdgSingleUserRedrawEvent (OWd);
			}
		break;
	default:
		break;
	}

	return (EFCBM_CONTINUE);

} /* CbOk() */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ 
-*		static int fillLb(MskDialog hMaskRl, LBFREC_RADIO *patLbfRecRadio, 
-*						  int iAnz)
-* DESCRIPTION
-*		fills the data from radiomenu into Listbuffer from chatmenu.
-* RETURNS
-*		0
-*--------------------------------------------------------------------------*/
static int FillLbTaet(MskDialog hMaskRl)
{
	TTaetStCtx		*ptMc;
	TAEST			atTaest[BLOCKSIZE_TAET];
	int				nI=0, iRv = 0;
	LBFREC_TAETST	tLbfTaetRec;


	ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
	ListBufferUnload (ptMc->Lb);

	memset (atTaest, 0, sizeof (atTaest));

	iRv = TExecSqlX (hMaskRl, NULL, "Select %TAEST From Taest",
					 BLOCKSIZE_TAET, 0,
					 SELSTRUCT (TN_TAEST, atTaest[0]),
					 NULL);

	if (iRv <= 0 && TSqlError (hMaskRl) != SqlNotFound) {
		WamasBox(SHELL_OF(hMaskRl),
			WboxNboxType,   WBOX_ALERT,
			WboxNbutton,    WboxbNok,
			WboxNmwmTitle,  MlM("Taetigkeitsstatistik"),
			WboxNtext,
			MlM("Datenbankfehler bei Taetigkeitsstatistik!\n"),
			NULL);

		LogPrintf(FAC_ME_TAETST, LT_ALERT,
			MlM("TAEST = TSqlErr: %s"),
			TSqlErrTxt(hMaskRl));
		TSqlRollback(hMaskRl);
		return (-1);
	}
	/* fill Listbuffer */
	for (nI = 0; nI < iRv; nI++) {
		tLbfTaetRec.tTaest = atTaest[nI];
		ListBufferAppendElement(ptMc->Lb, &tLbfTaetRec);
	}

	ListBufferUpdate(ptMc->Lb);
    return(EFCBM_CONTINUE);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		MaskenCallback fuer das Menue. 
-*		Parameter;
-*		 	MskDialog 	hMaskRl		Handle auf die Maske
-*			int			iReason		Reason - Was ist geschehen?	
-* RETURNS
-*--------------------------------------------------------------------------*/
static int CbMask(MskDialog hMaskRl, int iReason)
{
	TTaetStCtx		*ptMc;
	int         iRet = MSK_OK_TRANSFER;

	switch(iReason) {
	case MSK_CM:
		ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);

		MskVaAssign(hMaskRl,	MskGetElement("UM_PERS_PersNr_t"),
			MskNattr,   		EF_ATTR_NONE,
			MskNkey,        	(Value)NULL,
			MskNvariable,		(Value)&ptMc->cPersNr,
			NULL);

		MskVaAssign(hMaskRl,	MskGetElement("WorkTime_Start_t"),
			MskNattr,   		EF_ATTR_NONE,
			MskNvariable,		(Value)&ptMc->ttDatumVon,
			NULL);

		MskVaAssign(hMaskRl,	MskGetElement("WorkTime_End_t"),
			MskNattr,   		EF_ATTR_NONE,
			MskNvariable,		(Value)&ptMc->ttDatumBis,
			NULL);

		MskVaAssign(hMaskRl,	MskGetElement("Radio_Zeitraum_sh_sc"),
			MskNattr,   		EF_ATTR_NONE,
			MskNvariable,		(Value)&ptMc->iZeitraum,
			NULL);

		MskVaAssign(hMaskRl,	MskGetElement("Diagramtyp_sh_sc"),
			MskNattr,   		EF_ATTR_NONE,
			MskNvariable,		(Value)&ptMc->iDiagramtyp,
			NULL);

		MskVaAssign(hMaskRl,	MskGetElement("Kurventyp_sh_sc"),
			MskNattr,   		EF_ATTR_NONE,
			MskNvariable,		(Value)&ptMc->iKurventyp,
			NULL);

		EfcbmInstByName ( hMaskRl, "Diagramtyp_sh_sc", 
								KEY_DEF,CbDiagTyp, NULL ) ;

		EfcbmInstByName ( hMaskRl, "Kurventyp_sh_sc", 
								KEY_DEF,CbKurvTyp, NULL ) ;

		ptMc->iKurventyp = 1;
		ptMc->iZeitraum = 1;
		ptMc->iDiagramtyp = LINE;
		ptMc->ttDatumVon = time (NULL);
		ptMc->ttDatumBis = time (NULL);

		MskUpdateMaskVar (hMaskRl);
		break;

    case MSK_RA:
		ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);

        /* Initialisierung
        -*/
        ptMc->Lb=ListBufferInquireName(hMaskRl, "ListBuffer_me_taetst",KEY_DEF);
        if ( ptMc->Lb  == NULL) {
            fprintf(stderr, "Fehler beim ListBuffer\n");
            return(RETURN_ERROR);
        }
        break;
	case MSK_DM:
		ptMc = (TTaetStCtx *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		TSqlRollback(hMaskRl);
		free(ptMc);
		break;

	default:
		break;
	}

	return iRet;
} /* CbMask() */
DdTableRec Cb_Table[] = {
    {"CbCancel_taetst",       		(Value)CbCancel_taetst},
    {"CbOk",       			(Value)CbOk},
    {"CbReport_taetst",     (Value)CbReport_taetst},
    {"CbPrnConfig",     (Value)CbPrnConfig},
    {NULL}
};


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Aufruf der Maske	
-*		Parameter:
-*			OWidget	hWidget	Handle auf das Widget
-* RETURNS
-*		RETURN_ERROR im Errorfall
-*		RETURN_ACCEPTED wenn alles gut geht	
-*--------------------------------------------------------------------------*/
int _me_taetst(OWidget hWidget)
{
	MskDialog	       	hMaskRl;
	OWidget		       	hWidgetSh;
	TTaetStCtx			*ptMc;

	extern MskTdataConnect DC_UM_PERS[];

	ApGuiSet(ApNregisterMskCallback,(Value)Cb_Table);
	OwrcLoadObject(ApNregisterNumConstant, data_taet);
	OwrcLoadObject(ApNregisterDataConnect,DC_UM_PERS);

	OwrcLoadObject (ApNconfigFile, "mrad.rc");
	OwrcLoadObject (ApNconfigFile, "user.rc");
	OwrcLoadObject (ApNconfigFile, "owprint.rc");
	OwrcLoadObject (ApNconfigFile, "owreport.rc");
	OwrcLoadObject (ApNconfigFile, "me_taetst.rc");

	if((hMaskRl = MskOpenMask(NULL, RC_NAME)) == NULL) {
		return(RETURN_ERROR);
	}


	if((hWidgetSh = SHELL_OF(hMaskRl)) == NULL) {
		if((ptMc = malloc(sizeof(TTaetStCtx))) == NULL) {
			MskDestroy(hMaskRl);
			fprintf(stderr, "MALLOC ERROR (ptMc)\n");
			return(RETURN_ERROR);
		}

		memset(ptMc, 0, sizeof(TTaetStCtx));

		MskRlMaskSet(hMaskRl, MskNmaskCalldata,   (Value)ptMc);
		MskRlMaskSet(hMaskRl, MskNmaskCallback,   (Value)CbMask);

		hWidgetSh = ApShellModelessCreate(hWidget, AP_CENTER, AP_CENTER);

		WamasWdgAssignMenu(hWidgetSh,RC_NAME);
	}

	MskCreateDialog(hWidgetSh, hMaskRl, MlM("Tätigkeitsstatistik"), 
					NULL, HSL_NI, SMB_All);

	FillLbTaet (hMaskRl);

	return(RETURN_ACCEPTED);
} /* _me_taetst() */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*		Callback wenn aus die Maske aus dem Hauptmenue aus sufgerufen wird. 
-* RETURNS
-*		keine Returns - MENUFCT ist ein Define fuer 'void'
-*--------------------------------------------------------------------------*/

MENUFCT me_taetst (	MskDialog hMaskRl, MskStatic hEf, 
				MskElement hEl, int iReason, void *pvCbc)
{
	OWidget    hOwParent;

	hOwParent = SHELL_OF(hMaskRl);

	switch(iReason) {
	case  FCB_EF:
		_me_taetst(hOwParent);
		break;

	default:
		break;
	}
} /* me_taetst() */

