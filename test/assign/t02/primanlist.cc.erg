/****************************************************************************
+* PROJECT:   
+* PACKAGE:   package name
+* FILE:      primanli.c
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

#ifndef _WIN32
#include <unistd.h>
#endif /* !_WIN32 */

#include <string>

/* ------- Owil-Headers --------------------------------------------------- */

/* ------- Tools-Headers -------------------------------------------------- */
#include <prexec.h>
#include <sqllist.h>
#include <owrcloader.h>
#include <wamasbox.h>
#include <wamaswdg.h>
#include <efcbm.h>

#include <t_util.h>
#include <li_util.h>
#include <ml.h>
#include <sqltable.h>

#include <ml_util.h>
#include <cpp_util.h>
#include <owil_util.h>
#include "unix2dos.h"
#include <format.h>
#include "file_copy.h"
#include "cppdir.h"
#include "set_focus.h"

/* ------- Local-Headers -------------------------------------------------- */

//#include "ef_etc.h"

#define _PRIMANLI_C
#include "primanlist.h"
#undef _PRIMANLI_C

#define FAC_LISTPRINT "listprint"
#define LOCALEXPORTDIR "vtrlist"

using namespace Tools;

/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define PAGE_LEN	(64)

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

/* ===========================================================================
 * LOCAL (STATIC) Variables and Function-Prototypes
 * =========================================================================*/

 /* ------- Variables ----------------------------------------------------- */
static int WinSize=5;       /* Größe des sichtbaren Fensters  (Y-Wert) */
                            /* ....UNgerader Wert !!!                  */

static int AnzItem;         /* Anzahl der Items                         */

static int Item=0;          /* ausgewältes Item    */
static int f;               /* Flag    */
static int fm;              /* FlagMaxWert    */

static printerCap *P = NULL;

static char title[]="Druckerauswahl";   /* Fenster-Titel   */

static MskTgeneric pc_dec_grp_end = {
   {EK_DEC_GRPEND},                     /* kind of edit field */
};

static MskTdecGrp pc_dec_grp_vert_down = {
    {EK_DEC_GRPSTART},                  /* kind of edit field */
    GEOM_V_ALIGN,                       /* align */
    GEOM_V_TJOINED | GEOM_H_LJOINED,    /* join */
    AP_EXPAND,                          /* w */
    AP_EXPAND,                          /* h */
    AP_PLATE_DEEP,                      /* fw */
    AP_PLATE_DEEP,                      /* fh */
    -AP_PLATE_DEEP,                     /* deep */
};

static MskTtext OWFAR ef_filename = {
    {EK_EF_TEXT,    "filename"},
    EFT_STRG,
    "Filename",
    15,
    15,
    0,
    0,
};

typedef struct _LIBREC {
    char    lb_printername[PCAP_NAME_LEN +1];
    char    lb_printertext[PCAP_TEXT_LEN +1];
} LIBREC;


static int tabs_printerlist[]= {
    3,10,-1
};

static MskTlist ef_printerlist = {
    {EK_EF_LIST,"PrinterList",
    		ListBufferCallback},  		/* kind of edit field */
    "Druckerauswahl",                   /* label */
    70,                                /* width of line in list */
    AP_SCROLLBAR_RIGHT,                 /* style of list */
    POLICY_SINGLESELECT,                /* policy of list */
    0,                                  /* number of elements in buffer */
    8,                                  /* number of visible elements */
    0,
    tabs_printerlist
};


static  MskTmaskDescr filenamemask[] = {
        {&pc_dec_grp_vert_down},
            {&ef_filename },
        {&pc_dec_grp_end},
        {NULL}
};


static void cb_printerlist(MskTmaskRlPtr mask_rl, MskTgenericPtr ef,
                        MskTgenericRlPtr ef_rl, int reason, void *cbc);

static  MskTmaskDescr mask[] = {
        {&pc_dec_grp_vert_down},
        {&ef_printerlist, cb_printerlist, KEY_DEF, NULL, 0, "-"},
        {&pc_dec_grp_end},
        {NULL}
};

static VaInfoRec args_printer[]={
	{VA_TYPE_STRUCT, {NULL}, MemberOffset (LIBREC, lb_printername), },
	{VA_TYPE_STRUCT, {NULL}, MemberOffset (LIBREC, lb_printertext), },
	{VA_TYPE_END}
};


typedef struct  {
	ListBuffer      lb;
} PRISELMASK_CTX;


 /* ------- Function-Prototypes ------------------------------------------- */
static void fill_LB (MskTmaskRlPtr mask_rl);



/* ===========================================================================
 * GLOBAL VARIABLES
 * =========================================================================*/

/* ===========================================================================
 * LOCAL (STATIC) Functions
 * =========================================================================*/

/*
 * Callback for list buffer
 * => Changes the selection color
 */
static void cb_printerlist(MskTmaskRlPtr mask_rl, MskTgenericPtr ef,
                            MskTgenericRlPtr ef_rl, int reason, void *cbc)
{
	switch (reason) {
	    case FCB_RF:
	        WdgSet(ef_rl->ec_rl.w,WdgNselectCname, (Value)C_LIGHTBLUE );
	        break;
	    default:
	    	break;
	    }

}

/**
 * Fill list buffer with parsed data
 */
static void fill_LB (MskTmaskRlPtr mask_rl) {

  PRISELMASK_CTX     *ptCtx = (PRISELMASK_CTX *)MskRlMaskGet(mask_rl, MskNmaskCalldata);

  ListBufferUnload(ptCtx->lb);
  for (int idx = 0; idx < AnzItem; idx++) {

      LIBREC tLbRec;
      memset (&tLbRec, 0, sizeof (tLbRec));
      strncpy (tLbRec.lb_printername, P[idx].name, PCAP_NAME_LEN);
      tLbRec.lb_printername [PCAP_NAME_LEN] = '\0';
      strncpy (tLbRec.lb_printertext, P[idx].text, PCAP_TEXT_LEN);
      tLbRec.lb_printertext [PCAP_TEXT_LEN] = '\0';


      if( strcmp(tLbRec.lb_printername, LOCALFILE ) == 0 )
        {
          if( !WdgGuiGet(GuiNdisplayIsLocal) ) {
              ListBufferAppendElement (ptCtx->lb, &tLbRec);
          }
        } else {
            ListBufferAppendElement (ptCtx->lb, &tLbRec);
        }

  }
  ListBufferUpdate(ptCtx->lb);

}

/*
 * Parsed the printtab file
 *
 * Source: $TOOLSSRCDIR:/ui/owil/list/printcap.c
 *
 */
static int read_printtab(void)
{
    FILE    *dat;
    printerCap  *tmp;
    char    buf[800], *a, *b;


    dat=fopen("etc/" PRNTAB,"r");
    if( dat == NULL)  {
        /*fprintf(stderr,"\nError opening %s...\n",PRNTAB); */
        return -1;
    }


    if ( P != NULL ) free(P),P=NULL;
    P = (printerCap*)malloc ( sizeof(printerCap) );
    if ( P== NULL ) return -1;
    AnzItem=0;

    while( fgets(buf,sizeof(buf),dat) != NULL ) {
        memset(&P[AnzItem],0,sizeof(*P));

        a = strchr ( buf,'#' ); if ( a != NULL ) *a = '\0';
        a = strchr ( buf, '\n' ); if ( a != NULL ) *a = '\0';

        a = buf;
        if ( a == NULL || *a == '\0' ) continue;
        b = strchr( a, ':' );
        if ( b != NULL ) *b++ = '\0';
        strncpy(P[AnzItem].name,a,sizeof(P[AnzItem].name));

        a = b;
        if ( a == NULL || *a == '\0' ) continue;
        b = strchr( a, ':' );
        if ( b != NULL ) *b++ = '\0';
        strncpy(P[AnzItem].text,a,sizeof(P[AnzItem].text));

        a = b;
		 if ( a == NULL  || *a == '\0') continue;
        b = strchr( a, ':' );
        if ( b != NULL ) *b++ = '\0';
        strncpy(P[AnzItem].typ,a,sizeof(P[AnzItem].typ));

        a = b;
        if ( a != NULL && *a != '\0' ) {
            b = strchr( a, ':' );
            if ( b != NULL ) *b++ = '\0';
            P[AnzItem].width = atoi(a);
        }

        a = b;
        if ( a != NULL && *a != '\0' ) {
            b = strchr( a, ':' );
            if ( b != NULL ) *b++ = '\0';
            P[AnzItem].pagelen = strlen(a) > 0 ? atoi(a) : -1;
        }

        a = b;
        if ( a != NULL && *a != '\0' ) {
            strncpy(P[AnzItem].init,a,sizeof(P[AnzItem].init));
        }

        tmp=(printerCap*)realloc(P,(AnzItem+2)*sizeof(printerCap));
        if ( tmp != NULL ) {
            P = tmp;
            AnzItem++;
        }
    }

    fclose(dat);
    return 1;
}

/**
 * This is for the file-print
 *
 * Source: $TOOLSSRCDIR:/ui/owil/list/printcap.c
 *
 */
static printerCap *
doSelectPrinter ( OWidget w, Value data, printerCap *toSelect )
{

	static printerCap selected;
    OWidget     m_sh;
    MskDialog   mask_rl;

    selected = *toSelect;

    if ( strcmp( selected.name, "FILE" ) != 0 ) return &selected;

    mask_rl = MskOpenMask(filenamemask, "PRCAP_GETFILENAME");
    MskVaAssign(mask_rl,  (void*)&ef_filename, 
                MskNkey,         (Value)KEY_DEF, 
                MskNvariable,    (Value)selected.typ, 
                NULL);
    m_sh = ApShellModalCreate(w, AP_CENTER, AP_CENTER);

#if defined(TOOLS_VERSION) && (TOOLS_VERSION >= 33)
    WamasWdgAssignMenu(m_sh, "PRCAP_GETFILENAME");
#endif /* (TOOLS_VERSION >= 33) */

    MskVaCreateDialog(m_sh, mask_rl, MlM("Exportfile"), NULL,
                    HSL_Not_Intended, SMB_Ignore, NULL);
    WdgMainLoop();
    return strlen(selected.typ) > 0 ? &selected : NULL;
	

}

/**
 * Mask-callback for the new selection mask (with list-buffer)
 */

static int cb_mask ( MskTmaskRlPtr mask_rl, int reason )
{
	int				rv=1;
	PRISELMASK_CTX  *ptCtx;

	ptCtx  = ( PRISELMASK_CTX * ) MskRlMaskGet ( mask_rl, MskNmaskCalldata );

	switch ( reason ) {

	case MSK_CM:

		ptCtx->lb=ListBufferCreate(mask_rl,
				(MskTgeneric *)&ef_printerlist,
				KEY_DEF,
				NULL,
				NULL,
				(Value )NULL,
				70,
				sizeof(LIBREC),
				"\tDrucker\tBezeichnung",
				"\t%s\t%s",
				args_printer);

		if (ptCtx->lb == NULL) {
			fprintf (stderr, "ListBufferCreate failed");
			return NULL;
		}

		break;

	case MSK_RA:

		fill_LB(mask_rl);
		SetFocus(mask_rl, &ef_printerlist);
		break;

	case MSK_OK:

		rv = MSK_OK_TRANSFER;
		{
			// Get idx of selected record from LB and assign it
			int lastEle = ListBufferLastElement(ptCtx->lb);
			fprintf (stderr, "LastEle %d\n", lastEle);
			for (int idx = 0; idx <= lastEle; idx++) {
				ListElement le = ListBufferGetElement(ptCtx->lb, idx);
				if (le != NULL && (le->hint & LIST_HINT_SELECTED)) {
					Item = idx;
					break;
				}
			}
		}
		break;

	case MSK_DM:

		free ( ptCtx );
		ptCtx = NULL;
		break;
	}

	return rv;
}

/**
 * This is for the new printer selection mask
 *
 * Source: $TOOLSSRCDIR:/ui/owil/list/printcap.c
 *
 * Adapted: Using list-buffer now
 *
 */
printerCap * selectPrinter(OWidget w, void *data)
{
    OWidget     m_sh;
    MskDialog   mask_rl;
    printerCap  *ret = NULL;
    int         rv;
    PRISELMASK_CTX	*ptCtx;

    f=-(WinSize-1)/2;   /* Flag    */
    fm=-f;              /* FlagMaxWert    */
    read_printtab();
    if(AnzItem<=0) {
        ApBoxAlert((OWidget)w, HSL_Not_Intended,
            MlM("Keine Drucker gefunden!"));
        return NULL;
    }

    ptCtx = (PRISELMASK_CTX *)malloc (sizeof (PRISELMASK_CTX));
    if (ptCtx == NULL) {
    	fprintf (stderr, "malloc failed");
    	 return NULL;
    }


    mask_rl = MskOpenMask(mask, "PRCAP_SELECTPRINTER");
    MskVaAssign(mask_rl,  (void*)&ef_filename, 
                MskNkey,         (Value)KEY_DEF, 
                NULL);

    MskRlMaskSet(mask_rl, MskNmaskCalldata,(Value)ptCtx);
    MskRlMaskSet(mask_rl,MskNmaskCallback,(Value)cb_mask);

    m_sh = ApShellModalCreate((OWidget)w, AP_CENTER, AP_CENTER);

    MskVaCreateDialog(m_sh, mask_rl, title, NULL,
                     HSL_Not_Intended, SMB_Ignore, NULL);


#if defined(TOOLS_VERSION) && (TOOLS_VERSION >= 33)
    WamasWdgAssignMenu(m_sh, "PRCAP_SELECTPRINTER");
#endif /* (TOOLS_VERSION >= 33) */

    rv = WdgMainLoop();

    if( rv != IS_Cancel ) {
        ret = doSelectPrinter ( (OWidget)w, (Value)data, P + Item );
        fprintf (stderr, "Selected printer idx %d |%s - %s|\n",
        		Item, ret->name, ret->text);
    }

    if( P != NULL ) {
    	free(P);
    	P=NULL;
    }
    return ret;
}



/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/


int PrimanListSelCallbackLocal(elThandle *ptHandle, int iReason, OWidget hW, 
    selTdata *ptSelData, void *pvCalldata)
{

  int		iRv = 0;
  LM2_DESC	*hLd;
  char		*pcFileName = NULL;
  printerCap      *prtr;
  char            acClientFileName[PATH_MAX+1]={0};
  bool            print_to_file = false;
  bool            print_to_remote_file = false;

  switch(iReason) {
  case SEL_REASON_PRINT:

    iRv = -1;

    prtr = selectPrinter (GetRootShell(), pvCalldata);

    if (prtr == NULL) {
        fprintf (stderr, "Failed to select printer!\n");
        return (-1);
    }

    if( strcmp(prtr->name, LOCALFILE ) == 0 )
      {
        iRv = GetFileNameOnWdg (hW, acClientFileName, "Datei Speichern", ".txt" );

        if( iRv != 0 ) {
            LogPrintf( FAC_LISTPRINT, LT_ALERT, "Fehler beim Öffnen oder Abbruch durch Benutzer" );
            return -1;
        }

        LogPrintf( FAC_LISTPRINT, LT_ALERT, "Filename from User: %s", acClientFileName );

        StrCpy( acClientFileName, addFileNameExtensionForWin(acClientFileName, "txt" ) );

        print_to_remote_file = true;
      }
    else if ( strcmp(prtr->name, PCAP_TOFILE ) == 0 )
      {

		std::stringstream sstr;
		sstr << LOCALEXPORTDIR << "/" << who_am_i;
		std::string LocalUserExportDir = sstr.str();

        CppDir::Directory dir( LocalUserExportDir );

        if( !dir ) {
            iRv = mkdir( LocalUserExportDir.c_str(), 0777 );

            if( iRv != 0 ) {
                std::string errtxt =  strerror(errno);

                LogPrintf( FAC_LISTPRINT, LT_ALERT, "Fehler bei mkdir: %s", errtxt );

                if( !iRv ) {
                    WamasBox(hW,
                        WboxNboxType,   WBOX_ALERT,
                        WboxNbutton,    WboxbNok,
                        WboxNmwmTitle,  "Fehler beim erzeugen des Exportverzeichnisses",
                        WboxNtext,      format("Das Verzeichnis %s konnte nicht erzeugt werden\n%s",
                            LocalUserExportDir, errtxt ).c_str(),
                        NULL);
                    break;
                }
            }
        }
		std::string ExportFileName = prtr->typ;
		std::replace (ExportFileName.begin(), ExportFileName.end(), ' ', '_');

        print_to_file = true;
        StrCpy( acClientFileName, CppDir::concat_dir(LocalUserExportDir.c_str(),ExportFileName.c_str()));
		// fprintf (stderr, "Zielfile = [%s]\n", acClientFileName);
      }

    hLd = ptHandle->ehLM2_DESC;

    const char *pcValue = elKvlGetValue(
        ptHandle->ehProfile->etLayoutKeyValueList,
        "ascii_noheader");

    const char *pcTitle = MlMsg(ptSelData->selListDialog.ldTitle);

    if( pcValue != NULL && strcmp(pcValue,"yes") == 0 ) {
        pcTitle = NULL;
    }

    if (lm2_dump(hLd,
        prtr->pagelen,
        pcTitle,
        NULL,   /* init */
        NULL,   /* exit */
        NULL,
        &pcFileName) <= 0
    ) {
        LogPrintf( FAC_LISTPRINT, LT_ALERT, "Fehler beim Dumpen der Liste" );
        break;
    }

    if( print_to_remote_file )
      {
        unix2dos( pcFileName );

        std::string errtxt;
        iRv = SaOwTransferFileToWdg( pcFileName, acClientFileName, errtxt );

        unlink(pcFileName);

        if( iRv < 0) {
            WamasBox(hW,
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  "Fehler beim Speichern",
                WboxNtext,      format("Die Datei %s konnte nicht übertragen werden\n%s", acClientFileName, errtxt ).c_str(),
                NULL);

            LogPrintf( FAC_LISTPRINT, LT_ALERT, "Fehler beim SaOwTransferFileToAppl" );
            break;
        }
      }
    else if( print_to_file )
      {
        iRv = file_copy ( pcFileName, acClientFileName);

        std::string errtxt =  strerror(errno);

        LogPrintf( FAC_LISTPRINT, LT_ALERT, "Fehler bei rename: %s", errtxt );

        if( !iRv ) {
            WamasBox(hW,
                WboxNboxType,   WBOX_ALERT,
                WboxNbutton,    WboxbNok,
                WboxNmwmTitle,  "Fehler beim Speichern",
                WboxNtext,      format("Die Datei %s konnte nicht kopiert werden werden\n%s", acClientFileName, errtxt ).c_str(),
                NULL);
            break;
        }

      }
    else
      {
        std::string sCmd;

        if( strcmp(prtr->typ,"PCL" ) == 0 )
          {
            sCmd = "locallp ";
          }
        else
          {
            sCmd = "locallp_a2pcl ";
          }

        sCmd += prtr->name;
        sCmd += " ";
        sCmd += pcFileName;
        const char *pcCmd = const_cast<char *>(sCmd.c_str());

        PrVaExec(pcFileName,
            PrexecNprintCommand,    (void*)pcCmd,
            PrexecNoptDest,         (void*)prtr->name,
            PrexecNsynchronMode,    (void*)1,
            NULL);

        iRv = 1;
        break;
      }
  }

  return iRv;
}

