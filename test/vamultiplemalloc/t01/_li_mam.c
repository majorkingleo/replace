/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include "tstd.h"

#include "../dbcfg/shared/core.h"

#include "sqlligen.h"
#include "sqlopm.h"
#include "mumalloc.h"
#include "callback.h"

#include "sqllist.h"
#include "liutil.h"
#include "me_atoms.h"
#include "me_util.h"

#include "mam.h"
#include "te_mam.h"

#define  PN_MAM  "LIMAM"
#define  ID_MAM   "LI_MAM"



typedef struct {
	MAM			mam;
}LIMAM;


#   undef _o
#   define _o(x) offsetof(LIMAM,x)


static elTitem LIMAM_ITEM[] = {
	{TN_MAM,   {0, NULL},   {0,       (char *)elp_MAM, _o(mam),   EL_SUB},},
	{NULL}
};

static elTlistPart elp_LIMAM[] = {
	{PN_MAM, 	sizeof(LIMAM), 		LIMAM_ITEM},
	{FOOT_NAME, sizeof(FOOT4LISTS),	FOOT_ITEM},
	{NULL}
};


/*
 * db_statement
 */

static elTlist el_MAM = {
	"mam",
	elp_LIMAM,
	0,
	"SELECT %MAM FROM MAM "
};




static void destroy_callback_mam (ListTdialog *pListDialog)
{
	if (pListDialog) 
		free(pListDialog);

	return;
}


/*
 * Parameter fuer die MAM Liste
 */
static int local_li_mam (OWidget parent, int id, char *mtitle, Value ud,
					MskDialog *pmask_rl)
{
	ListTdialog		*pListDialog;
	ListTXdialog	*pListXDialog;
	ListTaction		*pListAction;
	MskTmaskDescr	*plstmsk, *pmsk;
	MskDialog		mask_rl;
	static char		orderByStrg[5][100];
	char     first_part[80];

	/* 
	 *Filter und Sortierkriterien
	*/

	static listSelItem Selector[] = {
		{"RecTyp", {1, &el_MAM, NULL, 0, 0x1L, ""},
			{"MAM_RecTyp_r", NULL},
			{ME_NOP, NULL},
			{LQ_LO2STR, NULL,0,NULL,NULL, FN_MAM_RecTyp},
			LSI_TYPE_RC
		},
		{"MaeTyp", {1, &el_MAM, NULL, 0, 0x1L, ""},
			{"MAM_MaeTyp_r", NULL},
			{ME_NOP, NULL},
			{LQ_LO2STR, NULL,0,NULL,NULL, FN_MAM_MaeTyp},
			LSI_TYPE_RC
		},
		{"Status", {1, &el_MAM, NULL, 0, 0x1L, ""},
			{"MAM_Status_r", NULL},
			{ME_NOP, NULL},
			{LQ_LO2STR, NULL,0,NULL,NULL, FN_MAM_Status},
			LSI_TYPE_RC
		},
		{"KomZeit",	{1, &el_MAM, NULL,	0,0x1L, ""},
        	{"MAM_KomZeit_t", NULL},
       		{"MAM_KomZeit_t", NULL},
			{LQ_SQLTIME, NULL,0,NULL,NULL, FN_MAM_KomZeit},
			LSI_TYPE_RC
		},
		{"LiefZeit",	{1, &el_MAM, NULL,	0,0x1L, ""},
        	{"MAM_LiefZeit_t", NULL},
       		{"MAM_LiefZeit_t", NULL},
			{LQ_SQLTIME, NULL,0,NULL,NULL, FN_MAM_LiefZeit},
			LSI_TYPE_RC
		},
		{	NULL,	}
	};

	sprintf(first_part, "RecTyp, MaeTyp, LPAD(ParentId,%d,' ') ", RECID_LEN);
 
    sprintf(orderByStrg[0], "%s", first_part);
    Selector[GetSelByName(&Selector[0],"MAM_RecTyp_r")].lsiSort.lsColName =
                        &orderByStrg[0][0];

    sprintf(orderByStrg[1], "MaeTyp, RecTyp, LPAD(ParentId,%d,' ') ", RECID_LEN);
    Selector[GetSelByName(&Selector[0],"MAM_MaeTyp_r")].lsiSort.lsColName =
                        &orderByStrg[1][0];

    sprintf(orderByStrg[2], "Status, %s", first_part);
    Selector[GetSelByName(&Selector[0],"MAM_Status_r")].lsiSort.lsColName =
                        &orderByStrg[2][0];

    sprintf(orderByStrg[3], "KomZeit, %s ", first_part);
    Selector[GetSelByName(&Selector[0],"MAM_KomZeit_t")].lsiSort.lsColName =
                        &orderByStrg[3][0];

    sprintf(orderByStrg[4], "LiefZeit, %s ", first_part);
    Selector[GetSelByName(&Selector[0],"MAM_LiefZeit_t")].lsiSort.lsColName =
                        &orderByStrg[4][0];


	if ( VaMultipleMalloc (	(int)sizeof(ListTdialog),	(void**)&pListDialog,
							(int)sizeof(ListTXdialog),	(void**)&pListXDialog,
							(int)sizeof(ListTaction),	(void**)&pListAction,
							(int)(-1)) == (void*)NULL )
		return -1;


	memset(pListDialog, 0, sizeof(ListTdialog));
	pListDialog->ldSelector			= Selector;
	pListDialog->ldHsl				= HSL_NI;
	pListDialog->ldAction			= pListAction;
	pListDialog->ldGenCallback		= lgGenSqlList;
	pListDialog->ldTitle			= mtitle;
	pListDialog->ldGenCalldata		= pListXDialog;
	pListDialog->ldIsModelessMask	= 1;
	pListDialog->ldDestroyCallback	= destroy_callback_mam;
	pListDialog->ldPMaskRl			= pmask_rl;
/*	pListDialog->ldSelectPrinter    = _get_prn;
*/
	memset(pListXDialog, 0, sizeof(ListTXdialog));
	pListXDialog->list				= pListDialog;
	pListXDialog->cb_genSqlList		= cb_makeListFoot;
	pListXDialog->bd 				= NULL;

	memset(pListAction, 0, sizeof(ListTaction));

	if ((plstmsk = listSelMaskOpenMaskOnly(pListDialog , NULL)) ==
			(MskTmaskDescr*)0) {
		if (pmask_rl)
			*pmask_rl = (MskDialog )0;
		free(pListDialog);
		return 0 ;
	}

	if ((mask_rl = listSelMaskOpen( pListDialog , ID_MAM )) ==
			NULL ) {
		if (pmask_rl)
			*pmask_rl = (MskDialog )0;
		free(pListDialog);
		return 0 ;
	}

	pListXDialog->mask_rl = mask_rl;	

	pListDialog->ldMask	= plstmsk;

	return listSelMask(pListDialog, parent);
}



void li_mammask(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{

	OWidget    parent=SHELL_OF(mask);
	static MskDialog mask_rl = (MskDialog )NULL;
	int rv;
	MskTcbcXf *cbc_xf;
	
	switch (reason) {
	case FCB_XF:
		cbc_xf = (MskTcbcXf *)cbc;
		if (mask_rl && SHELL_OF(mask_rl)) {
			WdgGuiSet(GuiNactiveShell, (Value)SHELL_OF(mask_rl));
			return;
		}
		rv = local_li_mam (parent, 0, "Liste Materialauftragsmengen", NULL, &mask_rl);
		break;	
	}
}

/*eof*/
