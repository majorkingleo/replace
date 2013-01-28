/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include <mumalloc.h>
#include <sqllist.h>
#include <owrcloader.h>

#if TOOLS_VERSION < 33
#include <sqlkey.h>

#else
#include <t_util.h>
#endif
#include <li_util.h>
#include <lidatim.h>
#include <ml.h>
#include <sqltable.h>
#include <lilo2str.h>

/* ------- Local-Headers -------------------------------------------------- */
#include <whc.h>
#include <te_whc.h>

#include "whc_util.h"
#include "whc_view_util.h"
#include "whc_teleview.h"


#define _WHC_LI_WHCTP_C
#include "whc_li_whctp.h"
#undef _WHC_LI_WHCTP_C

#include "menufct.h"
/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

/* ------- Allg. Defines ------------------------------------------------- */

#define PN_WHCTP	"LIWHCTP"
#define ID_WHCTP	"LI_WHCTP"
#define TITLE	"Protokoll Telegramme"

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
-*      Aufruf aus:         sel_callback
-* RETURNS
-*      void Funktion->kein Return
-*--------------------------------------------------------------------------*/
static void destroy_callback(ListTdialog *ptListDialog)
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
-*      Aufruf aus        :     Anbindung erfolgt in local_li_art, Funktion
-*                              wird als Callback bei einer Auswahl eines
-*                              Datensatzart in der Liste aufgerufen.
-* RETURNS
-*      0   bzw.
-*      SEL_RETURN_NOACT
-*--------------------------------------------------------------------------*/
static int sel_callback(elThandle *hEh, int iSel_reason, OWidget hW, 
						selTdata *ptSelData, void *pvCalldata)
{
	LM2_DESC	*ptLd;
	CbViewRec	*ptCbv;
	long		lLine, lRecnum, lRecoffset, lType, lLiType;
	char		*tRecord;
	int         iObjSize;

	switch (iSel_reason) {
	case SEL_REASON_SEL:

#if TOOLS_VERSION < 33
		iObjSize = sizeof(WHCT);
#else
		iObjSize = getStructSize(TN_WHCTP);

		if (iObjSize < sizeof(ListTfilter))
			iObjSize = sizeof(ListTfilter);
#endif

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

		lLiType = elPart2LM2_TYPE(hEh->ehList, elNbody, PN_WHCTP);
		if (lType == lLiType)
		{
			WhcViewTele((OWidget)GetRootShell(), (WHCT *)(void *)&tRecord[0],
						WHC_FTPNFS, (Value)WhcPrintFile);
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
static int local_li(OWidget hParent, char *pcMtitle,
					Value ptUd, MskDialog *hPmaskRl)
{
	#undef __o
	#define __o(x) offsetof(struct _WHCT, x)

	static elTitem it_WHCT[] = {
	 {"Quelle",{0},{WHCTHOST_LEN,"%-10.10s",__o(whctQuelle),EL_STR}},
	 {"Ziel",{0},{WHCTHOST_LEN,"%-10.10s",__o(whctZiel),EL_STR}},
	 {"Satzart",{0},{WHCTSA_LEN,"%-5.5s",__o(whctSatzart),EL_STR}},
	 {"AnlZeit",{0},{20,"YYYY.MM.DD  HH24:MI:SS",__o(whctAnlZeit),lcb_datim}},
	 {"TeleLfdNr",{0},{WHCTLFDNR_LEN, "%6ld",__o(whctTeleLfdNr),EL_LONG}},
	 {"TeleFile",{0},{WHCTFILE_LEN,"%-15.15s",__o(whctTeleFile),EL_STR}},
	 {"Status",{0},{STRVALUELEN_WHCTSTATUS,(char*)&tl2s_WHCTSTATUS,__o(whctStatus),lcb_lo2str}},
	 {"Status_l",{0},{LABELLEN_WHCTSTATUS,(char*)&tl2s_WHCTSTATUS,__o(whctStatus),lcb_lo2str_l}},
	 {"Status_s",{0},{SHORTLABELLEN_WHCTSTATUS,(char*)&tl2s_WHCTSTATUS,__o(whctStatus),lcb_lo2str_s}},
	 {"Fehler",{0},{WHCTFEHLER_LEN,"%-20.20s",__o(whctFehler),EL_STR}},
	 {"NeuSend",{0},{STRVALUELEN_WHCTNEUSEND,(char*)&tl2s_WHCTNEUSEND,__o(whctNeuSend),lcb_lo2str}},
	 {"NeuSend_l",{0},{LABELLEN_WHCTNEUSEND,(char*)&tl2s_WHCTNEUSEND,__o(whctNeuSend),lcb_lo2str_l}},
	 {"NeuSend_s",{0},{SHORTLABELLEN_WHCTNEUSEND,(char*)&tl2s_WHCTNEUSEND,__o(whctNeuSend),lcb_lo2str_s}},
	 {"Tele1",{0},{80,"%-80.80s",__o(whctTele),EL_STR}},
	 {"Tele2",{0},{80,"%-80.80s",__o(whctTele)+80,EL_STR}},
	 {"Tele3",{0},{80,"%-80.80s",__o(whctTele)+160,EL_STR}},
	 {"Tele4",{0},{80,"%-80.80s",__o(whctTele)+240,EL_STR}},
	 {"Tele5",{0},{80,"%-80.80s",__o(whctTele)+320,EL_STR}},
	 {"Tele6",{0},{80,"%-80.80s",__o(whctTele)+400,EL_STR}},
	 {"Tele7",{0},{32,"%-32.32",__o(whctTele)+480,EL_STR}},
	 {NULL}
	};

	static elTlistPart elp_WHCT[] = {
		{"WHCT", sizeof(struct _WHCT), it_WHCT},
		{NULL}
	};

	static elTitem shLIWHCTP_ITEM[] = {
		{TN_WHCTP,   {0, NULL},   {0, (char *)elp_WHCT, 0, EL_SUB},},
		{NULL}
	};

	static elTlistPart shElp_LIWHCTP[] = {
#if TOOLS_VERSION >= 33
		{FILTER_NAME, 	sizeof(ListTfilter), 	FilterItem},
		{PN_WHCTP, 		0, 						shLIWHCTP_ITEM},
#else
		{PN_WHCTP, 		sizeof(WHCT),			shLIWHCTP_ITEM},
#endif
		{FOOT_NAME, 	sizeof(FOOT4LISTS),		FOOT_ITEM},
		{NULL}
	};

	/*
 	 *  DB-Statement
	 */
	static elTlist shEl_whctp = {
		"whctp",
		shElp_LIWHCTP,
		0,
		"SELECT %WHCTP FROM WHCTP ",
	};

	extern listSubSel SE_WHCT;

	/* 
	 *  Filter und Sortierkriterien
	 */
	static listSelItem shSelector[] = {
		{"Telegramme", TN_WHCTP, &SE_WHCT,
			{&shEl_whctp},
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
    MskDialog       hMaskRl;
#if TOOLS_VERSION < 33
    MskTmaskDescr   *hMaskLst;
#endif
    char            *phLiWhctp;
	int             iObjSize;

#if TOOLS_VERSION < 33
	iObjSize = sizeof(WHCT);
#else
    iObjSize = getStructSize(TN_WHCTP);
#endif
    shElp_LIWHCTP[1].elpObjSize = iObjSize;

	if ( VaMultipleMalloc ( (int)sizeof(ListTdialog),   (void**)&phListDialog,
                            (int)sizeof(ListTXdialog),  (void**)&phListXDialog,
                            (int)sizeof(ListTaction),   (void**)&phListAction,
                            (int)sizeof(shSelector),    (void**)&phSelector,
                            (int)iObjSize,              (void**)&phLiWhctp,
                            (int)(-1)) == (void*)NULL )
		return -1;

#ifdef CHRISXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	/* vorbelegen.....
     */
    if ((pSel = GetSelByName (&Selector[0], "ART_TaNr_t")) != NULL) {
        pSel->lsiSort.lsActiveColNames =
            "ART.TaNr,ART.TeId";
        pSel->lsiSort.lsOrderByColName =
            "ART.TaNr,ART.TeId";
    }
#endif


	memset(phListDialog, 0, sizeof(ListTdialog));
	phListDialog->ldSelector		= phSelector;
	phListDialog->ldHsl				= HSL_NI;
	phListDialog->ldAction			= phListAction;
#if TOOLS_VERSION < 33
    phListDialog->ldGenCallback      = lgGenSqlList;
#else
    phListDialog->ldGenCallback     = FilterGenSqlList;
#endif
	phListDialog->ldTitle			= pcMtitle;
	phListDialog->ldGenCalldata		= phListXDialog;
#if TOOLS_VERSION < 33
    phListDialog->ldIsModelessMask   = 1;
#endif
	phListDialog->ldDestroyCallback	= destroy_callback;
	phListDialog->ldPMaskRl			= hPmaskRl;
#if TOOLS_VERSION < 33
    phListDialog->ldSelectPrinter    = tTermCtx.fctGetPrn;
#else
    phListDialog->ldSelectPrinter    = tTermCtx.fctGetPrn;
#endif
	phListDialog->ldSelMask			= "WHC_SE_WHCTP";
	phListDialog->ldPrintButResValTable = mgPrnButResValTable;

	memcpy(phSelector, shSelector, sizeof(shSelector));

	memset(phListXDialog, 0, sizeof(ListTXdialog));
	phListXDialog->list				= phListDialog;
	phListXDialog->cb_genSqlList	= cb_makeListFoot;
	phListXDialog->bd 				= NULL;

	memset(phListAction, 0, sizeof(ListTaction));
	phListAction->sel_callback		= sel_callback;
	phListAction->sel_calldata		= phLiWhctp;
	phListAction->sel_nookbutton    = 1;

#if TOOLS_VERSION < 33
    hMaskLst = listSelMaskOpenMaskOnly (phListDialog, NULL);
    if (hMaskLst == NULL) {
        if (hMaskLst) {
            *hPmaskRl = (MskDialog )NULL;
        }
        free (phListDialog);
        return 0 ;
    }
#endif

	hMaskRl = listSelMaskOpen (phListDialog, NULL);
	if (hMaskRl == (MskDialog )NULL) {
		if (hPmaskRl) {
			*hPmaskRl = (MskDialog )NULL;
		}
		free (phListDialog);
		return 0 ;
	}

	phListXDialog->mask_rl = hMaskRl;	

#if TOOLS_VERSION < 33
    phListDialog->ldMask = hMaskLst;

    listProcessListAtoms(phListDialog);
#endif

	return listSelMask(phListDialog, hParent);
}

/* ===========================================================================
 * GLOBAL Funktions 
 * =========================================================================*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Funktion zum Aufruf der Liste, laedt ausserdem die Maskenbeschreibung
-*      aus dem RC-File.
-*      Uebergabeparameter: OWidget parent
-*                          Value 	tUd       Userdata
-* RETURNS
-*      Rueckgabewert der local_li - Funktion
-*--------------------------------------------------------------------------*/
int _WhcWhcTPLi(OWidget hParent, Value ptUd) 
{
	static MskDialog	hMaskRl = (MskDialog )NULL;

	if (hMaskRl  &&  SHELL_OF(hMaskRl)) {
		WdgGuiSet (GuiNrestoreShell, (Value)SHELL_OF(hMaskRl));
		WdgGuiSet (GuiNmakeActiveShell, (Value)SHELL_OF(hMaskRl));
		return RETURN_ACCEPTED;
	}

	OwrcLoadObject(ApNconfigFile, "whc.rc");
	OwrcLoadObject(ApNregisterDataConnect, DC_WHCT);
	OwrcLoadObject(ApNconfigFile, "whc_se_whctp.rc");

    return local_li(hParent,MlM(TITLE), ptUd, &hMaskRl);
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
MENUFCT WhcWhcTPLi(MskDialog hMaskRl, MskStatic hEf, MskElement hEl,
				   int iReason, void *pvCbc)
{
	switch (iReason) {
	case FCB_XF:
		_WhcWhcTPLi((OWidget)GetRootShell(), (Value)NULL);
		break;

	default:
		break;
	}
}

