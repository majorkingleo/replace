/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include <mumalloc.h>
#include <sqllist.h>
#include <owrcloader.h>

#include <t_util.h>
#include <li_util.h>
#include <ml.h>
#include <sqltable.h>

/* ------- Local-Headers -------------------------------------------------- */

#define _LI_TES_C
#include "li_tes.h"
#undef _LI_TES_C

#include "me_tes.h"

#include "menufct.h"
/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

/* ------- Allg. Defines ------------------------------------------------- */

#define PN_TES	"LITES"
#define TN_TES	"TES"
#define ID_TES	"LI_TES"

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
 * LOCAL (STATIC) Funktions 
 * =========================================================================*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Zerstoert den Listendialog(Destroy-Callback)
-*      Uebergabeparameter: ListTdialog *pListDialog    -Pointer auf den
-*                                                       Listendialog
-*      Aufruf aus:         sel_callback_tes
-* RETURNS
-*      void Funktion->kein Return
-*--------------------------------------------------------------------------*/
static void destroy_callback_tes (ListTdialog *ptListDialog)
{
	if (ptListDialog)
		free(ptListDialog);
	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Wird in der Liste ein Datensatz ausgwaehlt, so wird automatisch das
-*      das dazugehoerige Wartungsmenue geoeffnet(existiert es schon so wird
-*      es in den Vordergrund gegeben.
-*      Uebergabeparameter:     diverse Daten aus der Liste
-*      Aufruf aus        :     Anbindung erfolgt in local_li_tes, Funktion
-*                              wird als Callback bei einer Auswahl eines
-*                              Datensatztes in der Liste aufgerufen.
-* RETURNS
-*      0   bzw.
-*      SEL_RETURN_NOACT
-*--------------------------------------------------------------------------*/
static int sel_callback_tes (elThandle *hEh, int iSel_reason, OWidget hW, 
								selTdata *ptSelData, void *pvCalldata)
{
	LM2_DESC	*ptLd;
	CbViewRec	*ptCbv;
	long		lLine, lRecnum, lRecoffset, lType, lLiType;
	char		*tRecord;
	int         iObjSize;

	switch (iSel_reason) {
	case SEL_REASON_SEL:

		iObjSize = getStructSize(TN_TES);
		if (iObjSize < sizeof (ListTfilter))
			iObjSize = sizeof (ListTfilter);

		ptLd = hEh->ehLM2_DESC;
		ptCbv = &ptSelData->selView;
		lLine = ptCbv->u.input.line;

		if (lm2_lin2rec(ptLd, lLine, &lRecnum, &lRecoffset) < 0)
			return 0;
		if (lm2_seek(ptLd, lRecnum, 0) < 0)
			return 0;

        tRecord = malloc (iObjSize);
        if (tRecord == NULL)
            break;
        memset (tRecord, 0, iObjSize);

		if (lm2_read(ptLd, &lType, tRecord) < 0) {
            free (tRecord);
            return 0;
        }

		lLiType = elPart2LM2_TYPE(hEh->ehList, elNbody, PN_TES);
		if (lType == lLiType)
		{
			entry_tes(GetRootShell (), &tRecord[0]);
			free (tRecord);
			return SEL_RETURN_NOACT;
		}

		free (tRecord);

		return SEL_RETURN_NOACT;

	case SEL_REASON_PROCESS:
		return 0;

	case SEL_REASON_END:
		return 0;

	default:
		return 0;
	}
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Eigentliche Listenfunktion
-* RETURNS
-*      0   bzw.
-*     -1   im Fehlerfall
-*--------------------------------------------------------------------------*/
static int local_li_tes (OWidget hParent, char *pcMtitle,
							Value ptUd, MskDialog *hPmaskRl)
{
	extern elTlistPart elp_TES[];
	static elTitem shLITES_ITEM[] = {
		{TN_TES,   {0, NULL},   {0, (char *)elp_TES, 0, EL_SUB},},
		{NULL}
	};

	static elTlistPart shElp_LITES[] = {
		{FILTER_NAME, 	sizeof(ListTfilter), 	FilterItem},
		{PN_TES, 		0, 						shLITES_ITEM},
		{FOOT_NAME, 	sizeof(FOOT4LISTS),		FOOT_ITEM},
		{NULL}
	};

	/*
 	 *  DB-Statement
	 */
	static elTlist shEl_tes = {
		"tes",
		shElp_LITES,
		0,
		"SELECT %TES FROM TES ",
	};

	extern listSubSel SE_TES;

	/* 
	 *  Filter und Sortierkriterien
	 */
	static listSelItem shSelector[] = {
		{"TE_Sperre", TN_TES, &SE_TES,
			{&shEl_tes},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{ NULL, }
	};

	ListTdialog     *phListDialog;
    ListTXdialog    *phListXDialog;
    ListTaction     *phListAction;
    listSelItem     *phSelector;
    listSelItem     *pSel;
    MskDialog       hMaskRl;
    char            *phLiTes;
	int             iObjSize;

    iObjSize = getStructSize(TN_TES);
	shElp_LITES[1].elpObjSize = iObjSize;

	if ( VaMultipleMalloc ( (int)sizeof(ListTdialog),   (void**)&phListDialog,
                            (int)sizeof(ListTXdialog),  (void**)&phListXDialog,
                            (int)sizeof(ListTaction),   (void**)&phListAction,
                            (int)sizeof(shSelector),    (void**)&phSelector,
                            (int)iObjSize,              (void**)&phLiTes,
                            (int)(-1)) == (void*)NULL )
		return -1;

    /* Untersortierung festlegen
     */
    if ((pSel= GetSelByName (&shSelector[0], "TES_TeId_t")) != NULL) {
        pSel->lsiSort.lsOrderByColName = "TES.TeId,TES.ProtZeit,TES.MatId_ArtId_Mandant,TES.MatId_ArtId_ArtNr";
    }
    if ((pSel= GetSelByName (&shSelector[0], "TES_MatId_ArtId_Mandant_t")) != NULL) {
        pSel->lsiSort.lsOrderByColName = "TES.TeId,TES.MatId_ArtId_Mandant,TES.MatId_ArtId_ArtNr,TES.TeId,TES.MatId_ArtId_Variante,TES.ProtZeit";
    }
    if ((pSel= GetSelByName (&shSelector[0], "TES_MatId_ArtId_ArtNr_t")) != NULL) {
        pSel->lsiSort.lsSort = 0;
        pSel->lsiSort.lsActiveColNames = NULL;
        pSel->lsiSort.lsOrderByColName = NULL;
    }
    if ((pSel= GetSelByName (&shSelector[0], "TES_MatId_ArtId_Variante_t")) != NULL) {
        pSel->lsiSort.lsSort = 0;
        pSel->lsiSort.lsActiveColNames = NULL;
        pSel->lsiSort.lsOrderByColName = NULL;
    }
    if ((pSel= GetSelByName (&shSelector[0], "TES_MatId_Charge_t")) != NULL) {
        pSel->lsiSort.lsOrderByColName = "TES.MatId_Charge,TES.MatId_ArtId_Variante,TES.TeId,TES.MatId_ArtId_Mandant,TES.MatId_ArtId_ArtNr,TES.ProtZeit";
    }
    if ((pSel= GetSelByName (&shSelector[0], "TES_SpKz_t")) != NULL) {
        pSel->lsiSort.lsOrderByColName = "TES.SpKz,TES.MatId_Charge,TES.MatId_ArtId_Variante,TES.TeId,TES.MatId_ArtId_Mandant,TES.MatId_ArtId_ArtNr,TES.ProtZeit";
    }


	memset(phListDialog, 0, sizeof(ListTdialog));
	phListDialog->ldSelector		= phSelector;
	phListDialog->ldHsl				= HSL_NI;
	phListDialog->ldAction			= phListAction;
	phListDialog->ldGenCallback		= FilterGenSqlList;
	phListDialog->ldTitle			= pcMtitle;
	phListDialog->ldGenCalldata		= phListXDialog;
	phListDialog->ldDestroyCallback	= destroy_callback_tes;
	phListDialog->ldPMaskRl			= hPmaskRl;
	phListDialog->ldSelectPrinter    = tTermCtx.fctGetPrn;
	phListDialog->ldSelMask			= "SE_TES";
	phListDialog->ldPrintButResValTable = mgPrnButResValTable;

	memcpy(phSelector, shSelector, sizeof(shSelector));

	memset(phListXDialog, 0, sizeof(ListTXdialog));
	phListXDialog->list				= phListDialog;
	phListXDialog->cb_genSqlList	= cb_makeListFoot;
	phListXDialog->bd 				= NULL;

	memset(phListAction, 0, sizeof(ListTaction));
	phListAction->sel_callback		= sel_callback_tes;
	phListAction->sel_calldata		= phLiTes;
	phListAction->sel_nookbutton    = 1;

	hMaskRl = listSelMaskOpen (phListDialog, NULL);
	if (hMaskRl == (MskDialog )NULL) {
		if (hPmaskRl) {
			*hPmaskRl = (MskDialog )NULL;
		}
		free (phListDialog);
		return 0 ;
	}

	phListXDialog->mask_rl = hMaskRl;	

	return listSelMask(phListDialog, hParent);
}

/* ===========================================================================
 * LOCAL (STATIC) Funktions 
 * =========================================================================*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Funktion zum Aufruf der Liste, laedt ausserdem die Maskenbeschreibung
-*      aus dem RC-File.
-*      Uebergabeparameter: OWidget parent
-*                          Value 	tUd       Userdata
-*      Aufruf aus der li_tes Funktion, oder aus dem Unimenue fuer TES
-* RETURNS
-*      Rueckgabewert der local_li_tes - Funktion
-*--------------------------------------------------------------------------*/
int _li_tes (OWidget hParent, Value ptUd) 
{
	static MskDialog	hMaskRl = (MskDialog )NULL;

	if (hMaskRl  &&  SHELL_OF(hMaskRl)) {
		WdgGuiSet (GuiNrestoreShell, (Value)SHELL_OF(hMaskRl));
		WdgGuiSet (GuiNmakeActiveShell, (Value)SHELL_OF(hMaskRl));
		return RETURN_ACCEPTED;
	}

	OwrcLoadObject(ApNconfigFile, "tes.rc");
	OwrcLoadObject(ApNconfigFile, "se_tes.rc");

    return local_li_tes (hParent,MlM("Liste Transporteinheit-Sperre"), 
								 ptUd, 
								 &hMaskRl);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Aufruf der Listenaufruffunktion
-*      Uebergabeparameter: standart OWIL
-*      Aufruf aus Menuebaum(Hauptmenue)
-* RETURNS
-*--------------------------------------------------------------------------*/
MENUFCT li_tes (MskDialog hMaskRl, MskStatic hEf, MskElement hEl,
				int iReason, void *pvCbc)
{
	switch (iReason) {
	case FCB_XF:
		_li_tes (GetRootShell(), (Value)NULL);
		break;

	default:
		break;
	}
}

