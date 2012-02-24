/*
+* PROJECT:   ELB
+* PACKAGE:   MEAUS
+* FILE:      li_aus.c
+* CONTENTS:  Liste Auslagerauftrag	 
+* COPYRIGHT NOTICE:
+*         (c) Copyright 1998 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+****************************************************************************/

/* ===========================================================================
 * INCLUDES
 * =========================================================================*/

/* ------- System-Headers ------------------------------------------------- */

/* ------- Owil-Headers --------------------------------------------------- */

/* ------- Tools-Headers -------------------------------------------------- */
#include <sqllist.h>
#include <mumalloc.h>
#include <owrcloader.h>

#include <t_util.h>
#include <li_util.h>
#include <ml.h>
#include <ml_util.h>
#include <sqltable.h>

/* ------- Local-Headers -------------------------------------------------- */
#include <te_aus.h>
#include <te_art.h>
#include <primanlist.h>

#define _LI_AUS_C
#include "li_aus.h"
#undef _LI_AUS_C

#include "me_aus.h"
#include "me_vpl.h"
#include "menufct.h"

/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define PN_AUSK		"LIAUSK"
#define ID_AUSK		"LI_AUSK"
#define TN_AUSK		"AUSK"
#define PN_AUSP		"LIAUSP"
#define TN_AUSP		"AUSP"
#define PN_VPLP		"LIVPLP"
#define TN_VPLP		"VPLP"
#define TN_ART		"ART"

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

/* ===========================================================================
 * FUNKTIONEN
 * =========================================================================*/


/* Destroy-Callback
-----------------------------------------------------------------------*/
static void destroy_callback_aus (ListTdialog *pListDialog)
{
	if (pListDialog)
		free(pListDialog);
	return;
}


/* Selektor-Callback
-----------------------------------------------------------------------*/
static int sel_callback_aus (	elThandle *eh, 	int sel_reason, 
								OWidget w, 		selTdata *selData, 
								void	*calldata) {
	LM2_DESC	*ld;
	CbViewRec	*cbv;
	long		line, recnum, recoffset, type, liType;
	char 		*pcRecord;
	int			iObjSize, iOffset;

	switch (sel_reason) {
	case SEL_REASON_SEL:

		iObjSize  = getStructSize(TN_AUSK);
	    iObjSize += getStructSize(TN_AUSP);
	    iObjSize += getStructSize(TN_ART);
	    iObjSize += getStructSize(TN_VPLP);

		if (iObjSize < sizeof (ListTfilter))
			iObjSize = sizeof (ListTfilter);

		ld = eh->ehLM2_DESC;
		cbv = &selData->selView;
		line = cbv->u.input.line;

		if (lm2_lin2rec(ld, line, &recnum, &recoffset) < 0)
			return 0;
		if (lm2_seek(ld, recnum, 0) < 0)
			return 0;

		pcRecord = malloc (iObjSize);
        if (pcRecord == NULL)
            break;
        memset (pcRecord, 0, iObjSize);

		if (lm2_read(ld, &type, pcRecord) < 0)
			return 0;

		liType = elPart2LM2_TYPE(eh->ehList, elNbody, PN_AUSK);
		if (type == liType)
		{
			entry_aus(GetRootShell (), &pcRecord[0]);
		    free(pcRecord);
			return SEL_RETURN_NOACT;
		}

		liType = elPart2LM2_TYPE(eh->ehList, elNbody, PN_VPLP);
		if (type == liType)
		{
			iOffset = getStructSize(TN_AUSK) +
					  getStructSize(TN_AUSP) +
					  getStructSize(TN_ART);

			entry_vpl(GetRootShell (), &pcRecord[iOffset]);
		    free(pcRecord);
			return SEL_RETURN_NOACT;
		}

		free(pcRecord);

		return SEL_RETURN_NOACT;

	case SEL_REASON_PROCESS:
		return 0;

	case SEL_REASON_END:
		return 0;

	case SEL_REASON_PRINT:
	    return PrimanListSelCallbackLocal(eh, sel_reason, w, selData, calldata);

	default:
		return 0;
	}
}


/* Parameter fuer die Liste
-----------------------------------------------------------------------*/
static int local_li_aus (	OWidget parent, char *pcMtitle,
							Value ud, 		MskDialog *pmask_rl) {

	extern elTlistPart elp_AUSK[];
	extern elTlistPart elp_AUSP[];
	extern elTlistPart elp_VPLP[];
	extern elTlistPart elp_ART[];

	static elTitem LIAUSK_ITEM[] = {
		{TN_AUSK,   {0, NULL},   {0, (char *)elp_AUSK, 0, EL_SUB},},
		{NULL}
	};
	static elTitem LIAUSP_ITEM[] = {
		{TN_AUSP,   {1, NULL},   {0, 	(char *)elp_AUSP, 	0, EL_SUB},},
		{TN_ART,   	{1, NULL},   {0, 	(char *)elp_ART, 	0, EL_SUB},},
		{NULL}
	};

	static elTitem LIVPLP_ITEM[] = {
		{TN_VPLP,   {1, NULL},   {0, 	(char *)elp_VPLP, 	0, EL_SUB},},
		{NULL}
	};

	static elTlistPart elp_LIAUSK[] = {
		{FILTER_NAME,	sizeof(ListTfilter),	FilterItem},
		{PN_AUSK, 		0,						LIAUSK_ITEM},
		{PN_AUSP, 		0, 						LIAUSP_ITEM},
		{PN_VPLP, 		0, 						LIVPLP_ITEM},
		{FOOT_NAME, 	sizeof(FOOT4LISTS),		FOOT_ITEM},
		{NULL}
	};

	/*
 	 *  DB-Statement
	 */
	static elTlist el_aus = {
		ID_AUSK,
		elp_LIAUSK,
		0,
		"SELECT %AUSK, %AUSP, %VPLP, %ART  FROM AUSK, AUSP, VPLP, ART  "
		"WHERE AUSP.AusId_Mandant(+) = AUSK.AusId_Mandant "
		"AND AUSP.AusId_AusNr(+) = AUSK.AusId_AusNr "
		"AND AUSP.AusId_AusKz(+) = AUSK.AusId_AusKz "
		"AND VPLP.AusId_Mandant(+) = AUSP.AusId_Mandant "
		"AND VPLP.AusId_AusNr(+) = AUSP.AusId_AusNr "
		"AND VPLP.AusId_AusKz(+) = AUSP.AusId_AusKz "
		"AND VPLP.PosNr(+) = AUSP.PosNr "
		"AND ART.ArtId_ArtNr(+) = AUSP.MatId_ArtId_ArtNr "
		"AND ART.ArtId_Mandant(+) = AUSP.MatId_ArtId_Mandant "
		"AND ART.ArtId_Variante(+) = AUSP.MatId_ArtId_Variante ",
	};

	extern listSubSel SE_AUSK;
	extern listSubSel SE_AUSP;
	extern listSubSel SE_VPLP;
	extern listSubSel SE_ART;

	/* 
	 *  Filter und Sortierkriterien
	 */
	static listSelItem Selector[] = {
		{"Auslagerauftrag-Kopf", TN_AUSK, &SE_AUSK,
			{&el_aus},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"Auslagerauftrag-Position", TN_AUSP, &SE_AUSP,
			{&el_aus},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"Artikelstamm", TN_ART, &SE_ART,
			{&el_aus},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"Auslagerauftrag-Verplanung", TN_VPLP, &SE_VPLP,
			{&el_aus},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{NULL},
	};

	ListTdialog		*pListDialog;
	ListTXdialog	*pListXDialog;
	ListTaction		*pListAction;
	listSelItem		*pSelector;
	MskDialog		mask_rl;
	char			*pLiOpm;
	int				iObjSize;

	iObjSize = getStructSize(TN_AUSK);
	iObjSize += getStructSize(TN_AUSP);
	iObjSize += getStructSize(TN_ART);
	iObjSize += getStructSize(TN_VPLP);
    elp_LIAUSK[1].elpObjSize = iObjSize;
    elp_LIAUSK[2].elpObjSize = iObjSize;
    elp_LIAUSK[3].elpObjSize = iObjSize;
	LIAUSP_ITEM[0].elBody.elaOffset = getStructSize(TN_AUSK);
	LIAUSP_ITEM[1].elBody.elaOffset = getStructSize(TN_AUSK) + 
									  getStructSize(TN_AUSP);
	LIVPLP_ITEM[0].elBody.elaOffset = getStructSize(TN_AUSK) + 
									  getStructSize(TN_AUSP) +
									  getStructSize(TN_ART);

	if ( VaMultipleMalloc (	(int)sizeof(ListTdialog),	(void**)&pListDialog,
							(int)sizeof(ListTXdialog),	(void**)&pListXDialog,
							(int)sizeof(ListTaction),	(void**)&pListAction,
							(int)sizeof(Selector),		(void**)&pSelector,
							(int)iObjSize,				(void**)&pLiOpm,
							(int)(-1)) == (void*)NULL )
		return -1;

	memset(pListDialog, 0, sizeof(ListTdialog));
	pListDialog->ldSelector			= pSelector;
	pListDialog->ldHsl				= HSL_NI;
	pListDialog->ldAction			= pListAction;
	pListDialog->ldGenCallback		= FilterGenSqlList;
	pListDialog->ldTitle			= pcMtitle;
	pListDialog->ldGenCalldata		= pListXDialog;
	pListDialog->ldDestroyCallback	= destroy_callback_aus;
	pListDialog->ldPMaskRl			= pmask_rl;
	pListDialog->ldSelectPrinter    = tTermCtx.fctGetPrn;
	pListDialog->ldSelMask			= "SE_AUS";
	pListDialog->ldPrintButResValTable = mgPrnButResValTable;

	memcpy(pSelector, Selector, sizeof(Selector));

	memset(pListXDialog, 0, sizeof(ListTXdialog));
	pListXDialog->list				= pListDialog;
	pListXDialog->cb_genSqlList		= cb_makeListFoot;
	pListXDialog->bd 				= NULL;

	memset(pListAction, 0, sizeof(ListTaction));
	pListAction->sel_callback		= sel_callback_aus;
	pListAction->sel_calldata		= pLiOpm;
	pListAction->sel_nookbutton     = 1;

	mask_rl = listSelMaskOpen (pListDialog, NULL);
	if (mask_rl == (MskDialog )NULL) {
		if (pmask_rl) {
			*pmask_rl = (MskDialog )NULL;
		}
		free (pListDialog);
		return 0 ;
	}

	pListXDialog->mask_rl = mask_rl;	

	return listSelMask(pListDialog, parent);
}


/* Funktion mit Userdata als Paramter
-----------------------------------------------------------------------*/
int _li_aus (OWidget parent, Value ud) 
{
	static MskDialog	mask_rl = (MskDialog )NULL;

	if (mask_rl  &&  SHELL_OF(mask_rl)) {
		WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(mask_rl));
		return RETURN_ACCEPTED;
	}

	OwrcLoadObject(ApNregisterDataConnect, DC_AUSK);
	OwrcLoadObject(ApNregisterDataConnect, DC_AUSP);
	OwrcLoadObject(ApNregisterDataConnect, DC_VPLP);
	OwrcLoadObject(ApNregisterDataConnect, DC_ART);
	OwrcLoadObject(ApNconfigFile, "aus.rc");
	OwrcLoadObject(ApNconfigFile, "art.rc");
	OwrcLoadObject(ApNconfigFile, "se_aus.rc");

    return local_li_aus (parent,(char *) MlM("Liste Auslagerauftrag"), 
						ud, &mask_rl);
}


/* Funktionsaufruf aus dem Menuebaum
-----------------------------------------------------------------------*/
MENUFCT li_aus (MskDialog mask, MskStatic ef, MskElement el,
				int reason, void *cbc)
{
	switch (reason) {
	case FCB_XF:	
		_li_aus (GetRootShell(), (Value)NULL);
		break;

	default:
		break;
	}
}

