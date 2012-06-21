/****************************************************************************
+* PROJECT:   M_BASEL99
+* PACKAGE:   METEK
+* FILE:      li_te.c
+* CONTENTS:  Liste Transporteinheit	 
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

#include <mumalloc.h>
#include <sqllist.h>
#include <owrcloader.h>

#include <t_util.h>
#include <li_util.h>
#include <ml.h>
#include <ml_util.h>
#include <sqltable.h>


/* ------- Local-Headers -------------------------------------------------- */

#define _LI_TE_C
#include "li_te.h"
#undef _LI_TE_C

#include "wamas.h"
#include "inv.h"
#include "callback.h"
#include "pos_util.h"
#include "term_util.h"
#include "infouser_util.h"
#include "hist_util.h"
#include "fes.h"
#include "me_te.h"
#ifdef LPUEB_AUFRUF
#include "tpa.h"
#endif /* LPUEP_AUFRUF */

#ifdef DBTEP
#ifdef METES
#include "me_tes.h"
#endif /* METES */
#endif /* DBTEP */

#include "menufct.h"
#include "primanlist.h"
/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define TITLE	"Liste Transporteinheiten"

#define PN_TEK	"LITEK"
#define TN_TEK	"TEK"

#ifdef DBTEP
#define PN_TEP	"LITEP"
#define PN_TES	"LITES"
#define TN_TEP	"TEP"
#define TN_ART	"ART"
#define TN_TES	"TES"
#define  TN_COLLI       "COLLI_LIST"
#endif /* DBTEP */

#define ID_TE	"LI_TE"

#define DBLALIGN(s) ((s)+((s)%8))




/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

/* ===========================================================================
 * FUNKTIONEN
 * =========================================================================*/

static int cb_sqlList(lgRecords *tSql, ListTdialog *tList, int iReason,
                      char *pcBuf, void *pvUd)
{
	char	acFeldIdVon[FELDID_LEN+1], acFeldIdBis[FELDID_LEN+1];
	int     iKey, iliqQueryEle=0;
	char 	*pcSqlStmt;
	int		iSel;
	char 	acTmp[100000+1];
	char 	*pcTmp;

    switch (iReason) {
    case genListStart:	
		pcTmp = (char *) realloc (tSql->SqlStmt, sizeof(char) * 10000);
		if (pcTmp == NULL) {
			perror("realloc");
			return -1;
		}
		tSql->SqlStmt = pcTmp;
		pcSqlStmt = tSql->SqlStmt;
		strcpy(acTmp, pcSqlStmt);

	    /*TeId */
        iliqQueryEle = 0;
        iKey = iliqQueryEle;
        iliqQueryEle++;
	    /*TetId*/
        iKey = iliqQueryEle;
        iliqQueryEle++;
		/* ART.AId_Mand */ /*TEP.MId_AId_Mand */
		if (tTermCtx.iSingleClient != 1 ) {
        	iKey = iliqQueryEle;
        	iliqQueryEle++;
		}
		/* ART.AId_ArtNr */ /* TEP.MId_AId_ArtNr */
        iKey = iliqQueryEle;
        iliqQueryEle++;
		/* ART.AId_Var */ /* TEP.MId_AId_Var */
		if (tTermCtx.iSingleVariant != 1 ) {
        	iKey = iliqQueryEle;
        	iliqQueryEle++;
		}
		/* TEP_MId_MatKz_t */
		if (tTermCtx.iSingleMatKz != 1) {
			iKey = iliqQueryEle;
            iliqQueryEle++;
        }
		/* ART.ArtBez */ 
        iKey = iliqQueryEle;
        iliqQueryEle++;
		/* TEP.MId_Charge */ 
		if (tTermCtx.iSingleBatch != 1) {
        	iKey = iliqQueryEle;
        	iliqQueryEle++;
		}
		/* FES.FeldId */ 
        iKey = iliqQueryEle;
        iliqQueryEle++;
        iSel = (tList->ldQuery.liqSort.lsLQmask & (1L<<iKey)) ? 1 : 0;
        if (iSel == 1) {
			pcTmp = strstr (pcSqlStmt, "AND  TEK.Pos_FeldId");
			if (pcTmp != NULL) {
				*pcTmp = '\0';
			}
			strncpy(acFeldIdVon, tList->ldQuery.liqQuery[iKey].lq_val[0].str,
				FELDID_LEN);
			acFeldIdVon[FELDID_LEN]='\0';
			strncpy(acFeldIdBis, tList->ldQuery.liqQuery[iKey].lq_val[1].str,
				FELDID_LEN);
			acFeldIdBis[FELDID_LEN]='\0';

			Filter_FeldId(tList->ldMaskRl, acFeldIdVon,acFeldIdBis, 
							pcSqlStmt, TN_FES); 
        }
		pcTmp = strstr(acTmp, "TEP.ErstErfZeit");
		if (pcTmp != NULL) {
			strcat (pcSqlStmt, " AND ");
			strcat (pcSqlStmt, pcTmp);
			break;
		}
		pcTmp = strstr(acTmp, "TEK.EinZeit");
		if (pcTmp != NULL) {
			strcat (pcSqlStmt, " AND ");
            strcat (pcSqlStmt, pcTmp);
            break;
        }
		pcTmp = strstr(acTmp, "TEP.FifoDatum");
		if (pcTmp != NULL) {
			strcat (pcSqlStmt, " AND ");
            strcat (pcSqlStmt, pcTmp);
            break;
        }
		pcTmp = strstr(acTmp, "TES.SpKz");
		if (pcTmp != NULL) {
			strcat (pcSqlStmt, " AND ");
            strcat (pcSqlStmt, pcTmp);
            break;
        }
		pcTmp = strstr(acTmp, "TEP.ResNr");
		if (pcTmp != NULL) {
			strcat (pcSqlStmt, " AND ");
            strcat (pcSqlStmt, pcTmp);
            break;
        }

		break; 
	case genListWriteFoot:
		/*Fusszeile schreiben*/
		{
        FOOT4LISTS  *ptFoot = (FOOT4LISTS *)NULL;

        ptFoot = (FOOT4LISTS *)tSql->footRec;

        if (ptFoot != NULL) {
			ptFoot->cntDat = tSql->recordsRead;

            elgFooter(tList->ldListHandle, elNbody,
                          FOOT_NAME, (void*)ptFoot);
		}

        }
		break;
	default: 
		break;

	}
	return SQL_NO_ACT;
}


/* Destroy-Calllback
-----------------------------------------------------------------------*/
static void destroy_callback_te (ListTdialog *pListDialog)
{
	if (pListDialog)
		free(pListDialog);
	return;
}

/* Selektor-Calllback
-----------------------------------------------------------------------*/
static int sel_callback_te (elThandle *eh, int sel_reason, OWidget w, 
								selTdata *selData, void	*calldata)
{
	LM2_DESC		*ld;
	CbViewRec		*cbv;
	long			line, recnum, recoffset, type, liType;
	char			*tRecord;
	unsigned int    iObjSize;
#ifdef DBTEP
#ifdef METES
	unsigned int 	iOffset;
#endif /* DBTEP */
#endif /* METES */

#ifdef LPUEB_AUFRUF

#endif


	switch (sel_reason) {
	case SEL_REASON_SEL:


		iObjSize = DBLALIGN(getStructSize(TN_TEK));
#ifdef DBTEP
		iObjSize += DBLALIGN(getStructSize(TN_TEP));
		iObjSize += DBLALIGN(getStructSize(TN_ART));
		iObjSize += DBLALIGN(getStructSize(TN_TES));
#endif /* DBTEP */

		if (iObjSize < sizeof (ListTfilter))
			iObjSize = sizeof (ListTfilter);

		ld = eh->ehLM2_DESC;
		cbv = &selData->selView;
		line = cbv->u.input.line;

		if (lm2_lin2rec(ld, line, &recnum, &recoffset) < 0)
			return 0;
		if (lm2_seek(ld, recnum, 0) < 0)
			return 0;

		tRecord = (char *) malloc (iObjSize);
        if (tRecord == NULL)
            break;
        memset (tRecord, 0, iObjSize);

        if (lm2_read(ld, &type, tRecord) < 0) {
            free (tRecord);
            return 0;
        }	

		liType = elPart2LM2_TYPE(eh->ehList, elNbody, PN_TEK);
		if (type == liType)
		{

            entry_te(GetRootShell (), ((TEK *)tRecord));


            
			free (tRecord);
			return SEL_RETURN_NOACT;
		}

#ifdef DBTEP
#ifdef METES
		liType = elPart2LM2_TYPE(eh->ehList, elNbody, PN_TES);
		if (type == liType)
		{
			iOffset = DBLALIGN(getStructSize(TN_TEK)) + 
					  DBLALIGN(getStructSize(TN_TEP)) +
					  DBLALIGN(getStructSize(TN_ART));
#ifdef LPUEB_AUFRUF
		  	iOffset = iOffset; /* Warningkill */
#endif /* LPUEB_AUFRUF */
#ifndef LPUEB_AUFRUF
			entry_tes(GetRootShell (), &tRecord[iOffset]);
#endif /* LPUEB_AUFRUF */
			free (tRecord);
			return SEL_RETURN_NOACT;
		}
#endif /* METES */
#endif /* DBTEP */

		free (tRecord);


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
	return 0;
}

/* Parameter fuer die Liste
-----------------------------------------------------------------------*/
static int local_li_te (OWidget parent, char *pcMtitle,
							Value ud, MskDialog *pmask_rl)
{
	extern elTlistPart elp_TEK[];
	static elTitem LITEK_ITEM[] = {
		{TN_TEK,   {0, NULL},   {0, (char *)elp_TEK, 0, EL_SUB},},
		{NULL}
	};
	static elTitem it_Colli[] = {
    	{"Colli", {0}, {9, "%s", 0, lcb_Colli},},
    	{"Rest",  {0}, {13, "%s", 0, lcb_Rest},},
    	{NULL },
	};
	static elTitem it_TesColli[] = {
    	{"Colli", {0}, {9, "%s", 0, lcb_Colli},},
    	{"Rest",  {0}, {13, "%s", 0, lcb_Rest},},
    	{NULL },
	};

static elTlistPart elp_COLLI_LIST[] = {
    {TN_COLLI, 9, it_Colli},
    {NULL}
};
static elTlistPart elp_TES_COLLI_LIST[] = {
    {TN_COLLI, 9, it_TesColli},
    {NULL}
};

#ifdef DBTEP
	extern elTlistPart elp_TEP[];
	extern elTlistPart elp_ART[];
	static elTitem LITEP_ITEM[] = {
		{TN_TEP,   {1, NULL},   {0, (char *)elp_TEP, 0, EL_SUB},},
		{TN_ART,   {1, NULL},   {0, (char *)elp_ART, 0, EL_SUB},},
		{TN_COLLI, {1, NULL},   {0, (char *)elp_COLLI_LIST, 0, EL_SUB},},
		{NULL}
	};

	extern elTlistPart elp_TES[];
	static elTitem LITES_ITEM[] = {
		{TN_TES,   {1, NULL},   {0, (char *)elp_TES, 0, EL_SUB},},
		{TN_COLLI, {1, NULL},   {0, (char *)elp_TES_COLLI_LIST, 0, EL_SUB},},
		{NULL}
	};
#endif /* DBTEP */

	static elTlistPart elp_LITE[] = {
		{FILTER_NAME,   sizeof(ListTfilter),    FilterItem},
		{PN_TEK, 		0, 						LITEK_ITEM},
#ifdef DBTEP
		{PN_TEP, 		0, 						LITEP_ITEM},
		{PN_TES, 		0, 						LITES_ITEM},
#endif /* DBTEP */
		{FOOT_NAME, 	sizeof(FOOT4LISTS),		FOOT_ITEM},
		{NULL}
	};

	/*
 	 *  DB-Statement
	 */
	static elTlist el_te = {
		"te",
		elp_LITE,
		0,
#ifdef DBTEP
		"SELECT %TEK, %TEP, %ART, %TES FROM TEK, TEP, ART, TES, FES "
		"WHERE TEP.TeId(+) = TEK.TeId "
		"AND FES.FeldId = TEK.Pos_FeldId " 
		"AND ART.AId_Mand(+) = TEP.MId_AId_Mand "
		"AND ART.AId_ArtNr(+) = TEP.MId_AId_ArtNr "
		"AND ART.AId_Var(+) = TEP.MId_AId_Var "
		"AND TES.TeId(+) = TEP.TeId "
		"AND TES.PosNr(+) = TEP.PosNr ",
#else
		"SELECT %TEK FROM TEK "
#endif /* DBTEP */
	};

	extern listSubSel SE_TEK;
#ifdef DBTEP
	extern listSubSel SE_TEP;
	extern listSubSel SE_ART;
	extern listSubSel SE_TES;
	extern listSubSel SE_FES;
#endif /* DBTEP */

	/* 
	 *  Filter und Sortierkriterien
	 */
	static listSelItem Selector[] = {
		{"TE-Kopf", TN_TEK, &SE_TEK,
			{&el_te},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
#ifdef DBTEP
		{"TE-Position", TN_TEP, &SE_TEP,
			{&el_te},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"Artikelstamm", TN_ART, &SE_ART,
			{&el_te},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"Feldstamm", TN_FES, &SE_FES,
			{&el_te},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{"TE-Sperre", TN_TES, &SE_TES,
			{&el_te},
				{NULL},
				{NULL},
			{NULL},
			LSI_TYPE_RC
		},
#endif /* DBTEP */
		{NULL},
	};

	ListTdialog		*pListDialog;
	ListTXdialog	*pListXDialog;
	ListTaction		*pListAction;
	listSelItem		*pSelector;
	MskDialog		mask_rl;
	char			*pLiTe;
	int             iObjSize;

	iObjSize = DBLALIGN(getStructSize(TN_TEK));
	iObjSize += DBLALIGN(getStructSize(TN_TEP));
	iObjSize += DBLALIGN(getStructSize(TN_ART));
	iObjSize += DBLALIGN(getStructSize(TN_TES));

    elp_LITE[1].elpObjSize = iObjSize;
#ifdef DBTEP
    elp_LITE[2].elpObjSize = iObjSize;
    elp_LITE[3].elpObjSize = iObjSize;
    elp_LITE[4].elpObjSize = iObjSize;
    LITEP_ITEM[0].elBody.elaOffset = DBLALIGN(getStructSize(TN_TEK));
    LITEP_ITEM[1].elBody.elaOffset = DBLALIGN(getStructSize(TN_TEK)) + 
									 DBLALIGN(getStructSize(TN_TEP));
    LITES_ITEM[0].elBody.elaOffset = DBLALIGN(getStructSize(TN_TEK)) + 
									 DBLALIGN(getStructSize(TN_TEP)) +
									 DBLALIGN(getStructSize(TN_ART));
	it_Colli[0].elBody.elaOffset =
        DBLALIGN(getStructSize(TN_TEK)) + getFieldOffset (NULL,TN_TEP, "Mngs");
	it_Colli[1].elBody.elaOffset =
        DBLALIGN(getStructSize(TN_TEK)) + getFieldOffset (NULL,TN_TEP, "Mngs");

	it_TesColli[0].elBody.elaOffset =
        LITES_ITEM[0].elBody.elaOffset + getFieldOffset (NULL,TN_TES, "Mngs");
	it_TesColli[1].elBody.elaOffset =
        LITES_ITEM[0].elBody.elaOffset + getFieldOffset (NULL,TN_TES, "Mngs");
#endif /* DBTEP */


	if ( VaMultipleMalloc (	(int)sizeof(ListTdialog),	(void**)&pListDialog,
							(int)sizeof(ListTXdialog),	(void**)&pListXDialog,
							(int)sizeof(ListTaction),	(void**)&pListAction,
							(int)sizeof(Selector),		(void**)&pSelector,
							(int)iObjSize,				(void**)&pLiTe,
							(int)(-1)) == (void*)NULL )
		return -1;

	memset(pListDialog, 0, sizeof(ListTdialog));
	pListDialog->ldSelector			= pSelector;
	pListDialog->ldHsl				= HSL_NI;
	pListDialog->ldAction			= pListAction;
	pListDialog->ldGenCallback		= FilterGenSqlList;
	pListDialog->ldTitle			= pcMtitle;
	pListDialog->ldGenCalldata		= pListXDialog;
	pListDialog->ldDestroyCallback	= destroy_callback_te;
	pListDialog->ldPMaskRl			= pmask_rl;
	pListDialog->ldSelectPrinter    = tTermCtx.fctGetPrn;
	pListDialog->ldSelMask			= "SE_TE";
	pListDialog->ldPrintButResValTable = mgPrnButResValTable;

	memcpy(pSelector, Selector, sizeof(Selector));

	memset(pListXDialog, 0, sizeof(ListTXdialog));
	pListXDialog->list				= pListDialog;
	pListXDialog->cb_genSqlList		= cb_sqlList;
	pListXDialog->bd 				= NULL;

	memset(pListAction, 0, sizeof(ListTaction));
	pListAction->sel_callback		= sel_callback_te;
	pListAction->sel_calldata		= pLiTe;
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
int _li_te (OWidget parent, Value ud) 
{
	static MskDialog	mask_rl = (MskDialog )NULL;
	int iRv = 0;

	if (mask_rl  &&  SHELL_OF(mask_rl)) {
		WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(mask_rl));
		return RETURN_ACCEPTED;
	}

	OwrcLoadObject(ApNconfigFile, "tek.rc");
#ifdef DBTEP
	OwrcLoadObject(ApNconfigFile, "tep.rc");
	OwrcLoadObject(ApNconfigFile, "tes.rc");
	OwrcLoadObject(ApNconfigFile, "art.rc");
#endif /* DBTEP */
	OwrcLoadObject(ApNconfigFile, "fes.rc");
	OwrcLoadObject(ApNconfigFile, "se_te.rc");

    return local_li_te (parent, MlM("GD034 Liste Transporteinheiten"), ud, &mask_rl);
}


/* Funktionsaufruf aus dem Menuebaum
-----------------------------------------------------------------------*/
MENUFCT li_te (MskDialog mask, MskStatic ef, MskElement el,
				int reason, void *cbc)
{
	switch (reason) {
	case FCB_XF:
		_li_te (GetRootShell(), (Value)NULL);
		break;

	default:
		break;
	}
}

