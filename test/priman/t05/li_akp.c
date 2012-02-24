/*
 *	li_akp.c
 *
 *	Aktion protokoll Liste
 *
 *	Copyright (c) 1997 by ZoSo
 *	All Rights Reserved. Alle Rechte Vorbehalten.
 *
 */

#include <fstd.h>
#include <dbsqlstd.h>
#include <sqlligen.h>
#include <mumalloc.h>
#include <stime.h>
#include <core.h>
#include "liutil.h"
#include "list_util.h"

#include <akp.h>
#include <te_akp.h>
#include <stime.h>

#include <lidate.h>
#include <fstd.h>
#include <tpa.h>



#define LI_AKP "LI_AKP"


typedef struct _LIAKP {
	AKP     akp;
} LIAKP;


static void destroy_callback_akp (ListTdialog *pListDialog)
{
	if (pListDialog) 
		free(pListDialog);

	return;
}



/*
 * Parameter fuer die Bewegungsprotokoll-Liste
 */

int local_li_akp(OWidget parent, int id, char *mtitle, Value ud,
									MskDialog *pmask_rl )
{

static elTitem LIAKP_ITEM[] = {
	{"AKP",    {0, NULL},   {0,       (char *)elp_AKP, 0,   EL_SUB},},
	{NULL}
	};

static elTlistPart elp_LIAKP[] = {
		{"LIAKP", 	sizeof(LIAKP), 		LIAKP_ITEM},
		{FOOT_NAME, sizeof(FOOT4LISTS),	FOOT_ITEM},
		{NULL}
	};


	/*
 	 * db_statement
	 */

	static elTlist el_AKP = {
		"akp",
		elp_LIAKP,
		0,
		"SELECT %AKP FROM AKP ",
	};


	/* 
	 *Filter und Sortierkriterien
	*/
#ifdef _ALTELISTEN_
	static listSelItem Selector[] = {
        {"Zeit", AKP_Zeit, NULL,{&el_AKP,AKP_Zeit},
         {AKP_Zeit_t},{AKP_Zeit_t},
         {LQ_SQLTIME},
		 LSI_TYPE_RC
        },
		{"Aktion",AKP_Aktion,NULL,{&el_AKP,AKP_Aktion},
			{AKP_Aktion_t},
            {ME_NOP, NULL},
			{LQ_LO2STR},
 			LSI_TYPE_RC
		},
		{"Benutzer", AKP_Benutzer,NULL,{&el_AKP,AKP_Benutzer},
            {AKP_Benutzer_t},
            {ME_NOP, NULL},
			{LQ_VARSTR},
 			LSI_TYPE_RC
		},
		{"Typ", AKP_Typ,NULL,{&el_AKP,AKP_Typ},
            {AKP_Typ_t},
            {ME_NOP, NULL},
			{LQ_VARSTR},
 			LSI_TYPE_RC
		},
		{	NULL,	}
	};
#else

	static listSelItem Selector[] = {
		{"Aktionsprotokoll",TN_AKP,&SE_AKP,
        	{&el_AKP},
       		{NULL},
			{NULL},
			{NULL},
			LSI_TYPE_RC
		},
		{	NULL,	}
	};


#endif
	ListTdialog		*pListDialog;
	ListTXdialog	*pListXDialog;
	ListTaction		*pListAction;
	MskDialog		mask_rl;
	time_t			ptimeVon, ptimeBis;
	char			*acSelMaskName="SE_LIAKP";
    listSelItem     *pSel;
	listSelItem		*pSelector;
	
#ifdef _ALTELISTEN_
	sprintf(first_part, "Zeit, Aktion, RPAD(Benutzer,%d,' ')",USER_LEN);
	
	/* Order by time */
	rv=SetOrderByName(&Selector[0],
					  "Zeit",
					  StrForm("%s ", first_part));
	
	/* Order by action */
	rv=SetOrderByName(&Selector[0],
					  "Aktion",
					  StrForm("Aktion, Zeit, RPAD(Benutzer,%d,' ')",USER_LEN));
	
	/* Order by user */
	rv=SetOrderByName(&Selector[0],
					  "Benutzer",
					  StrForm("RPAD(Benutzer,%d,' '), Aktion, Zeit", USER_LEN));
	
	/* Order by type */
	rv=SetOrderByName(&Selector[0],
					  "Typ",
					  StrForm("RPAD(typ,%d,' '), Aktion, Zeit", AKPTYP_LEN));
#endif
    
	if ( VaMultipleMalloc (	(int)sizeof(ListTdialog),	(void**)&pListDialog,
							(int)sizeof(ListTXdialog),	(void**)&pListXDialog,
							(int)sizeof(ListTaction),	(void**)&pListAction,
							(int)sizeof(Selector),		(void**)&pSelector,
							(int)(-1)) == (void*)NULL )
		return -1;

	/* vorbelegen
	 */
    if ((pSel = GetSelByName (&Selector[0], AKP_Zeit_t)) != NULL) {
        ptimeVon = today_morning ();
        ptimeBis = today_evening ();
        pSel->lsiFilter.lfLQinitVon = (void *)&ptimeVon;
        pSel->lsiFilter.lfLQinitBis = (void *)&ptimeBis;
    }
	
    memset(pListDialog, 0, sizeof(ListTdialog));
	pListDialog->ldSelector			= Selector;
	pListDialog->ldHsl				= HSL_NI;
	pListDialog->ldAction			= pListAction;
	pListDialog->ldGenCallback		= lgGenSqlList;
	pListDialog->ldTitle			= mtitle;
	pListDialog->ldGenCalldata		= pListXDialog;
/*	pListDialog->ldIsModelessMask	= 1;*/
	pListDialog->ldSelMask  		= acSelMaskName;
	pListDialog->ldDestroyCallback	= destroy_callback_akp;
	pListDialog->ldPMaskRl			= pmask_rl;
	pListDialog->ldSelectPrinter    = WamSelectPrinter;


	memset(pListXDialog, 0, sizeof(ListTXdialog));
	pListXDialog->list				= pListDialog;
	pListXDialog->cb_genSqlList		= cb_makeListFoot;
	pListXDialog->bd 				= NULL;

	memset(pListAction, 0, sizeof(ListTaction));

#ifdef _ALTELISTEN_
/*	ptimeVon = time((time_t *)0)-3600;
	ptimeBis = time((time_t *)0)+3600; */
	ptimeVon = today_morning();
	ptimeBis = today_evening();
	pListDialog->ldSelector[0].lsiFilter.lfLQinitVon=(void *)&ptimeVon;
	pListDialog->ldSelector[0].lsiFilter.lfLQinitBis=(void *)&ptimeBis;

	if ((plstmsk = listSelMaskOpenMaskOnly(pListDialog , NULL)) ==
			(MskTmaskDescr*)0) {
		if (pmask_rl)
			*pmask_rl = (MskDialog )0;
		free(pListDialog);
		return 0 ;
	}
#endif
	if ((mask_rl = listSelMaskOpen( pListDialog , LI_AKP )) ==
			NULL ) {
		if (pmask_rl)
			*pmask_rl = (MskDialog )0;
		free(pListDialog);
		return 0 ;
	}

	pListXDialog->mask_rl = mask_rl;	

/*	pListDialog->ldMask	= plstmsk;*/

	/*return listSelMask(pListDialog, GetMenuShell(parent));*/
	return listSelMask(pListDialog, parent);
}


void li_akpmask(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
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
		rv = local_li_akp (parent, 0,LsMessage(LSM("Aktionsprotokoll")), 0, &mask_rl);
		break;	
	}
}

/*eof*/
