/**********************************************************************
+*
+*  PACKAGE:	FESTO
+*
+*  FILE:		mt_lps.c
+*
+*  CONTENTS:	Wartungsmenue Koordinatenstamm
+*
+*  PURPOSE:
+*
+*  NOTES:
+*
+*
+*  COPYRIGHT NOTICE:
+*
+*          (c) Copyright 1994 by
+*
+*                  Salomon Automationstechnik Ges.m.b.H
+*                  Friesach 67
+*                  A-8114 Stuebing
+*                  Tel.: (++43) (3127) 2211-0
+*                  Fax.: (++43) (3127) 2211-22
+*
+*
+*
+*
+*  REVISION HISTORY:
+*
+*  Rev  Date       Comment                                         By
+*  ---  ---------  ----------------------------------------------  ---
+*    0	 24-Aug-99  created											rm
+*
+**********************************************************************/

#include <stddef.h>
#include <stdlib.h>
#include <stime.h>
#include <string.h>
#include "menu.h"
#include "status.h"
#include "lst_util.h"
#include "log.h"
/* XX-XX-XX-XX */
#include  "msk_util.h"
#include  "db_struc.h"
#include  "dbsql.h"
#include  "cb.h"
#include  "ef.h"
#include  "local_disperr.h"
#include  "me_teibest.h"
#include  "barcint.h"
#include  "proc.h"
#include  "cycle.h"
#include "conv.h"
#include "mt_lps.h"



typedef struct _LPS_CD {
	LPS		lps;
	int		liMode;
} LPS_CD;


/*------------------------------------------------------------------------------
-*
-* Nomanklatuer der Match-Codes:
-*	IX ... Invisible bei StepX
-*
-* ----------------------------------------------------------------------------*/
#define o(x) ((void*)offsetof(LPS, x))

static MskTmaskDescr mask[] = {
	{&dec_grp_vert_down},
		{&dec_filler_v0},
		{&dec_grp_vert_up},
			{&dec_grp_horiz},
				{&dec_marker_1},
				{&ef_LagNr,	NULL,KEY_DEF, o(lpsLp.LagNr), VK_OFFSET,
					 "All-D1-D2-D3-D4-"},
				{&dec_nix},
			{&dec_grp_end},		 
		{&dec_grp_end},		 
		{&dec_grp_vert_up},
			{&dec_grp_horiz},
				{&dec_marker_1},
				{&ef_LpPos,	NULL,KEY_DEF, o(lpsLp), VK_OFFSET,
					 "All-I0-D2-D3-D4-ELB_Lagerplatz"},
				{&dec_nix},
			{&dec_grp_end},		 
		{&dec_grp_end},		
		// project part start - FESTO - AA-0312-000338
		{&dec_grp_vert_up},
			{&dec_grp_horiz},
				{&dec_marker_1},
				{&ef_LagBereich, NULL,KEY_DEF, o(lpsLagerbereich), VK_OFFSET,
//					"All-I0-I1-D3-"},
					"All-I0-I1-D-"},
			{&dec_grp_end},
		{&dec_grp_end},
		// project part start - FESTO - AA-0312-000338
		{&dec_grp_vert_up},
				{&dec_marker_1},
				{&ef_Zwei, NULL, KEY_DEF, o(lpsCntKos), VK_OFFSET,
					 "All-I0-I1-D-ELB_Belegung-"},
		{&dec_grp_end},		 
		{&dec_grp_vert_up},
				{&dec_marker_1},
				{&ef_Zwei,NULL, KEY_1,	o(lpsLpTyp), VK_OFFSET, 
					 "All-I0-I1-D-ELB_Lagerplatz Typ-"},
				{&ef_LpSperre, NULL, KEY_DEF,o(lpsSperre), VK_OFFSET, 
					 "All-I0-I1-D3-"},
				{&ef_SpGrnd, NULL, KEY_DEF,	o(lpsSpGrnd), VK_OFFSET, 
					 "All-I0-I1-D3-"},
		{&dec_grp_end},
		{&dec_grp_vert_up},
	{&dec_grp_end},		 
	{NULL}
};

static char *m_title[]= {
	"Lagerstamm anlegen",
	"Lagerstamm aendern",
	"Lagerstamm loeschen",
	"Lagerstamm anzeigen"
};


static int db_buchen(OWidget w, MaskContextRec *mc)
{
	int dbrv = 0, commit = 0;
	LPS	*ptLps;

	ptLps = (LPS*) mc->cr;

	switch (mc->db_status) {
	case AEN:

		/*
		 * Wenn LP reigegeben wird, dann Sperrgrund auch rausloeschen
		 */
		if (ptLps->lpsSperre == KOS_FREI) {
			ptLps->lpsSpGrnd[0] = '\0';
		}

		dbrv = ExecSql("UPDATE LPSTAMM SET lpsSperre=:z, lpsSpGrnd=:y "
			"WHERE lpsLpLagNr=:a AND lpsLpKoo1=:b "
			"AND lpsLpKoo2=:c AND lpsLpKoo3=:d "
			"AND lpsLpKoo4=:e AND lpsLpKoo5=:f",
			SQLINT(ptLps->lpsSperre), SQLSTRING(ptLps->lpsSpGrnd),
			SQLINT(ptLps->lpsLp.LagNr), SQLINT(ptLps->lpsLp.Koo[0]),
            SQLINT(ptLps->lpsLp.Koo[1]), SQLINT(ptLps->lpsLp.Koo[2]),
			SQLINT(ptLps->lpsLp.Koo[3]), SQLINT(ptLps->lpsLp.Koo[4]), NULL);

		if (dbrv != 1) {
			dbrv = -1;
		}
		commit = 1;
		break;
	default:
		break;
	}

	if (commit == 1) {
		if (dbrv < 0 || SqlCommit() < 0) {
			SqlRollback();
		}
		else {
			mc->mrv = IS_Ok;
			logPrintf(1, "LPS: Lagerplatz: %s %s",
				LpPos2Str(&ptLps->lpsLp, NULL),
				ptLps->lpsSperre == KOS_FREI ?
					"freigegeben" : "gesperrt");
		}
	}

	return dbrv;
}


/******************************************************************************/
/************************* Masken-Callback  ***********************************/
/******************************************************************************/

static int cb_mask_mt_lps(MskTmaskRl *mask_rl, int reason)
{
	MaskContextRec	*mc;
	LPS_CD			*ptCd;
	LPS				*ptLps;
	LST				*ptLst;
	int				dbrv, i, rv = 1;
	
	mc  = (MaskContext )MskRlMaskGet(mask_rl,MskNmaskCalldata);	
	ptCd = (LPS_CD*)mc->cr;
	ptLps = &ptCd->lps;

	switch (reason) {
	case MSK_CM:
		MskVaAssignMatch(mask_rl, "All*",
			MskNvariableStruct,  (Value)ptLps,
			NULL);
		LoadEfState(mask_rl,-1,0);
		SetMaskState(mask_rl, EF_ATTR_NONE, Step_0);
		break;

	case MSK_OK:
		switch (mask_rl->status) {
		case Step_0:
			MskRlMaskSet(mask_rl, MskNmaskTransferDup2Var, 1);
			ptLst = GetLstByNr(ptLps->lpsLp.LagNr);
			if (ptLst == NULL) {
				MenuBoxAlert(mask_rl->shell, HSL_Not_Intended,
				   "Lagernummer ungueltig");
				if (ptCd->liMode != 0)
					rv = MSK_OK_LEAVE;
				else 
					rv = MSK_OK_UPDATEVAR;
				break;
			}
			SetMaskState(mask_rl, EF_ATTR_NONE, Step_1);
			rv = MSK_OK_UPDATEVAR;
			break;

		case Step_1:
			SqlRollback();
			MskRlMaskSet(mask_rl, MskNmaskTransferDup2Var, 1);

			if (ptLps->lpsLp.Koo[0] == 0 && ptLps->lpsLp.Koo[1] == 0 &&
				ptLps->lpsLp.Koo[2] == 0 && ptLps->lpsLp.Koo[3] == 0 &&
				ptLps->lpsLp.Koo[4] == 0)
			{
				rv = MSK_OK_UPDATEVAR;
				break;
			}

			if (mc->db_status != NEU) {
				dbrv = ExecSql("SELECT %LPS FROM LPSTAMM "
					 "WHERE lpsLpLagNr=:a AND lpsLpKoo1=:b "
					 "AND lpsLpKoo2=:c AND lpsLpKoo3=:d "
					 "AND lpsLpKoo4=:e AND lpsLpKoo5=:f",
					SELSTRUCT("LPS",*ptLps),
					SQLINT(ptLps->lpsLp.LagNr),
            		SQLINT(ptLps->lpsLp.Koo[0]),
            		SQLINT(ptLps->lpsLp.Koo[1]),
            		SQLINT(ptLps->lpsLp.Koo[2]),
            		SQLINT(ptLps->lpsLp.Koo[3]),
            		SQLINT(ptLps->lpsLp.Koo[4]),
					NULL);

				if (dbrv < 0) {
					DB_disperr(mask_rl->shell,  "Lagerplatzstamm", "");
					rv = MSK_OK_UPDATEVAR;
					break;
				}
			}

			if (mc->db_status == AEN)
				SetMaskState(mask_rl, EF_ATTR_NONE, Step_2);
			else
				SetMaskState(mask_rl, EF_ATTR_NONE, Step_3);

			rv = MSK_OK_UPDATEVAR;
			break;

		case Step_2:
			MskRlMaskSet(mask_rl, MskNmaskTransferDup2Var, 1);
			switch (mc->db_status) {
			case AEN:
				if ((rv = MskCheckLm(mask_rl)) == 0)
					break;
				if (MenuBoxCommitXY(mask_rl->shell, AP_CENTER, -60,
					HSL_Not_Intended,  "%s ?", m_title[mc->db_status]) == IS_Ok)
					db_buchen(mask_rl->shell, mc);

				SetMaskState(mask_rl,EF_ATTR_NONE,Step_1);
			}
			if (ptCd->liMode != 0)
				rv = MSK_OK_LEAVE;
			else
				rv = MSK_OK_UPDATEVAR;
			break;

		case Step_3:
			MskRlMaskSet(mask_rl, MskNmaskTransferDup2Var, 1);
			if (ptCd->liMode != 0) {
				rv = MSK_OK_LEAVE;
			}
			else {
				SetMaskState(mask_rl,EF_ATTR_NONE,Step_1);
				rv = MSK_OK_UPDATEVAR;
			}
			SqlRollback();
			break;
		}
		break;


	case MSK_ESC:
		rv = 1;
		if (ptCd->liMode != 0)
			break;


		SqlRollback();
		if (mask_rl->status > Step_1) {
			SetMaskState(mask_rl, EF_ATTR_NONE, Step_1);
			MskUpdateMaskVar(mask_rl);
			rv = 0;
		}
		else if (mask_rl->status > Step_0) {
			for(i=0; i<MAXKOO; ++i)
				ptLps->lpsLp.Koo[i] = 0;
			SetMaskState(mask_rl, EF_ATTR_NONE, Step_0);
			MskUpdateMaskVar(mask_rl);
			rv = 0;
		}
		break;

	case MSK_DM:
		SqlRollback();
		break;
	}

	return rv;
}


int lpsio(OWidget w, LPS *lps, int wahl, int liMode)
{
	MskTmaskRl		*mask_rl;
	MaskContextRec	*mc;
	OWidget			m_sh;
	int				rv = 0;
	LPS_CD			Cd;

	memset(&Cd, 0, sizeof(Cd));
	memcpy(&Cd.lps, lps, sizeof(Cd.lps));
	Cd.liMode = liMode;

	mask_rl = MskOpenMask(mask, NULL);
	mask_rl->cb_mask = cb_mask_mt_lps;
	m_sh = ApShellModalCreateRel(w);

	mc = MaskContextCreate(NULL, mask_rl, m_sh, wahl, EF_ATTR_NONE, 0);
	if (mc != 0) {

		mc->cr = &Cd;

		mc->mrv = liMode == 0 ? IS_Ok : IS_Cancel;
		MskRlMaskSet(mask_rl,MskNmaskCalldata,(Value)mc);

		MskCreateDialog(m_sh, mask_rl, m_title[wahl], NULL,
						HSL_Not_Intended, SMB_Ignore);

		if (wahl != NEU && liMode == 1) {
			rv = WdgDefaultButton(m_sh);
			if (rv != RETURN_ABORT)
				 rv = WdgDefaultButton(m_sh);
		}

		WdgMainLoop();

		SqlRollback();

		rv = mc->mrv;
        MemDealloc(mc);
	}
	return rv;
}



/*----------------------------------------------------------------------------
-* Koordinatenbestand warten
-*/

static int _mt_lps(OWidget w, Value data, Value calldata)
{
	LPS			lps;
	int			wahl = (int)calldata;

	memset(&lps, 0, sizeof(LPS));

	return lpsio(w, &lps, wahl, 0);
}

int mt_lps(OWidget w, int menu_id, char *menu_title, Value userdata)
{
	OWidget			shell;
	cdBar			*pBar;
	int				display;
	int				rv;

	pBar = getBarCd ();
	pBar->pinMenu = TRUE;
	pBar->directByBarcode = FALSE;
	shell = WdgFindShell (w);
	rv = ApMenuNextCreate(shell,  "Koordinatenstamm warten", 
							MenuId2Str(menu_id), HSL_NI,
		 "~Aendern",       AP_KEY_SPECIAL, _mt_lps, (Value)AEN,
		"~Anzeigen",       AP_KEY_SPECIAL, _mt_lps, (Value)ANZ,
		NULL);

	if (pBar->directByBarcode == TRUE)
	{
		rv = pBar->menuId;
		pBar->directByBarcode = FALSE;
	}
	pBar->pinMenu = FALSE;
	display = WdgGuiGet(GuiNdisplay);
	if (rv > 0 && display == DISPLAY_TEXT)
		ApMenuSet (w, ApNmenuStartupId, (Value) rv);
	return (RETURN_ACCEPTED);
}



/*----------------------------------------------------------------------------
-* Datenbank Koordinatenbestand Ïber Liste warten
-*/

int mt_lt_lps(OWidget w, LPS *ptLps, int wahl)
{
	if (wahl == NEU)
		memset(ptLps, 0, sizeof(LPS));
	return lpsio(w, ptLps, wahl, 1);
}

