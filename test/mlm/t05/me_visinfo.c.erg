/*****************************************************************************
+* PROJECT:   WAMAS-K
+* PACKAGE:   MRAD
+* FILE:      me_visinfo..c
+* CONTENTS:  overview description, list of functions, ...
+* COPYRIGHT NOTICE:
+*         (c) Copyright 2003 by
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
#ifdef _WIN32
#include <windows.h>
#include <winsock2.h>
#endif /* WIN32 */
#ifndef _WIN32
#include <dirent.h>
#endif /* ! WIN32 */

/* ------- Owil-Headers --------------------------------------------------- */
#include <dbsqlstd.h>
#include <owil.h>
#include <owrcloader.h>
#include <module/owss.h> /* List Buffer SS */
#include <module/_owudb.h>

/* ------- Tools-Headers -------------------------------------------------- */
#include <logtool.h>     /* LogPrint */
#include <efcbm.h>
#include <errmsg.h>              
#include <owrcloader.h>
#include <telo2str.h>
#include <wamaswdg.h>   /* Wamas Box */
#include <ml.h>
#include <ml_util.h>
#include <t_util.h>
#include <disperr.h>
#include <sqlkey.h>
#include <sscb.h>      /* List Buffer SS */

/* ------- Local-Headers -------------------------------------------------- */
#include "radfac.h"

#define _ME_VISINFO_C
#include "me_visinfo.h"
#undef _ME_VISINFO_C

#include "me_visarea.h"
#include "me_visregal.h"
#include "me_visrampe.h"
#include "me_vistxt.h"
#include "me_visbmp.h"
#include "me_visap.h"
#include "me_viswatchl.h"
#include "me_visual.h"
/*#include "transformer_util.h"*/
/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/
#define _RC_NAME    "ME_VISINFO"
#define BLOCKSIZE   500
#define _PATH_BMP "/bitmaps/"
#define WATCHL_DEF "WAT"

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

typedef struct _LB_VISREGAL {
    VISREGAL    tVisRegal;
} LB_VISREGAL;

typedef struct _LB_VISTXT {
    VISTXT      tVisTxt;
} LB_VISTXT;

typedef struct _LB_VISBMP {
    VISBMP      tVisBmp;
} LB_VISBMP;

typedef struct _LB_VISLINE {
    VISLINE     tVisLine;
} LB_VISLINE;

typedef struct _LB_VISAP {
    VISAP       tVisAP;
} LB_VISAP;

typedef struct _LB_VISRAMPE {
    VISRAMPE    tVisRampe;
} LB_VISRAMPE;

typedef struct _MlMDesc {
    ErrMlM  eErrMlM;
    char    *pTextMsg;
} MlMDesc;

static MlMDesc atMlMDesc[] = {
    {
        /* MLM-Def    */        MlM_DbError,
        /* MLM-Text   */        MLM("Datenbankfehler! Tabelle: %s"),
    },
    {
        /* MLM-Def    */        MlM_NoData,
        /* MLM-Text   */        MLM("Keine Daten gefunden!"),
    },
    {
        /* MLM-Def    */        MlM_PrintErr,
        /* MLM-Text   */        MLM("Fehler beim Drucken!"),
    },
    {
        /* MLM-Def    */        MlM_SelDat,
        /* MLM-Text   */        MLM("Bitte selektieren Sie einen Datensatz!"),
    },
    {
        /* MLM-Def    */        MlM_GetTeIdErr,
        /* MLM-Text   */        MLM("Fehler beim Generieren "
                                    "einer neuen TE-Id!"),
    },
    {
        /* MLM-Def    */        MlM_BeaDat,
        /* MLM-Text   */        MLM("Fehler beim Bearbeiten der Daten!"),
    },
    {
        /* MLM-Def    */        MlM_Status,
        /* MLM-Text   */        MLM("Fehler beim Statushandling!"),
    },
    {
        /* MLM-Def    */        MlM_LogErr,
        /* MLM-Text   */        MLM("Fehler beim Logfileviewer!"),
    },
    {
        /* MLM-Def    */        MlM_AllSp,
        /* MLM-Text   */        MLM("Fehler bei der Speicheranforderung!"),
    },
    {
        /* MLM-Def    */        MlM_DelDat,
        /* MLM-Text   */        MLM("Wollen Sie den Datensatz löschen?"),
    },
    {
        /* MLM-Def    */        MlM_ChaDat,
        /* MLM-Text   */        MLM("Wollen Sie den Datensatz ändern?"),
    },
    {
        /* MLM-Def    */        MlM_NewDat,
        /* MLM-Text   */        MLM("Wollen Sie den Datensatz anlegen?"),
    },
    {
        /* MLM-Def    */        MlM_FileOpen,
        /* MLM-Text   */        MLM("Fehler beim Öffnen der Datei %s"),
    },
    {
        /* MLM-Def    */        MlM_WamErr,
        /* MLM-Text   */        MLM("Interner WAMAS-Fehler!"),
    },
    {
        /* MLM-Def    */        MlM_MenErr,
        /* MLM-Text   */        MLM("Fehler beim Öffnen der Maske!"),
    },
    {
        /* MLM-Def    */        MlM_GefGut,
        /* MLM-Text   */        MLM("Gefahrgut!"),
    },
    {
        /* MLM-Def    */        MlM_FctErr,
        /* MLM-Text   */        MLM("Fehler bei der Funktion %s"),
    },
    {
        /* MLM-Def    */        MlM_AuslagErr,
        /* MLM-Text   */        MLM("Auslagerauftrag.Kein Ersatzziel möglich"),
    },
    {
        /* MLM-Def    */        MlM_NoAuftrag,
        /* MLM-Text   */        MLM("Kein Auftrag gefunden"),
    },
    {
        /* MLM-Def    */        MlM_WrongPos,
        /* MLM-Text   */        MLM("Falsche Position gescannt"),
    },
    {
        /* MLM-Def    */        MlM_NoBuSchl,
        /* MLM-Text   */        MLM("Buchungsschl|ssel nicht vorhanden"),
    },
    {
        /* MLM-Def    */        MlM_NoKostSte,
        /* MLM-Text   */        MLM("Kostenstelle nicht bekannt"),
    },
    {
        /* MLM-Def    */        MlM_NoFunction,
        /* MLM-Text   */        MLM("Keine Funktion angebunden"),
    },
    {
        /* MLM-Def    */        MlM_NoTpa,
        /* MLM-Text   */        MLM("Keine Transportaufträge vorhanden"),
    },


    {MlM_Max, "MlM_Max"}

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
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Function gets the MlMText
-* RETURNS
-*      OK      -> pointer to InfoListDesc structure
-*      ERROR   -> NULL pointer
-*--------------------------------------------------------------------------*/
static char *GetInfoMlMText (ErrMlM eErrMlM) {

    int     nEle = 0;
    int     nI;

    nEle = (int )(sizeof(atMlMDesc) / sizeof(atMlMDesc[0]));

    for (nI = 0; nI < nEle; nI++) {

        if (atMlMDesc[nI].eErrMlM == eErrMlM) {
            return (atMlMDesc[nI].pTextMsg);
        }
    }

    return (NULL);

}


/*----------------------------------------------------------------------------
-*  Transformers for MRAD
-*--------------------------------------------------------------------------*/
int CbX_lo2str_s_visColor (void **dest, void *src)
{
    return CbX_lo2str_s (dest, src, (Value)&tl2s_VISCOLOR);
}

int CbX_lo2str_s_gangOff (void **dest, void *src)
{
    return CbX_lo2str_s (dest, src, (Value)&tl2s_GANGOFF);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  int MULTI_ColorLines (MskDialog     hMaskRl,
-*                                ListBuffer    hLb,
-*                                char          *pacSpreadSheet,
-*                                int           iKey,
-*                                char          *pacColor)
-* DESCRIPTION
-*  Set Color to Spread-Sheet Lines
-* RETURNS
-*  Error  -1
-*  Ok      1
-*--------------------------------------------------------------------------*/
int MULTI_Color (MskDialog     hMaskRl,
                              ListBuffer    hLb,
                              char          *pacSpreadSheet,
                              int           iKey,
                              char          *pacColor)
{
    long            lLast = 0, nI = 0;


    hLb = ListBufferInquireName (hMaskRl, pacSpreadSheet, iKey);
    if (hLb == NULL) {
        return (-1);
    }

    lLast = ListBufferLastElement (hLb);

    for (nI = 0; nI <= lLast; nI ++) {

        if ((nI+1)%2 == 0) {
            ListBufferSetElementColor (hLb, nI,
                                       GrColorLock (pacColor));
        }
    }

    return (1);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Callback for Backgroundpicture 
-* RETURNS
-*      immer EFCBM_CONTINUE
-*--------------------------------------------------------------------------*/
static int cb_RootBmpFile (MskDialog hMaskBmp,  MskStatic hEf, MskElement hEfRl,
                int iReason, void *pvCbc, void *pvCalldata) {

    MskTcbcOl       *cbc_ol;
    char            acPathName[255+1];
    char            bufaux[100+1];
    char            buf[100+1];
    DIR             *dir;
    struct dirent   *dentry;
    int             i = 0;

    switch (iReason) {
    case FCB_EF:
        break;
    case FCB_LF:
        break;
    case FCB_OM:
        cbc_ol = (MskTcbcOl *)pvCbc;
        switch (cbc_ol->xreason) {
        case XCB_OCLOSED:
            MskNextTabGroup ((MskElement )hEfRl);
            break;
        case XCB_OCREATE:
            memset (acPathName, 0, sizeof (acPathName));
            sprintf (acPathName, "%s%s", getenv("ROOTDIR"), _PATH_BMP);
            if ((dir = opendir (acPathName)) != NULL) {
                while ((dentry = readdir (dir)) != NULL) {
                    if (strstr (dentry->d_name, ".bmp") != NULL) {
                        strncpy (buf, dentry->d_name, VISBEZ_LEN);
                        bufaux[VISBEZ_LEN] = '\0';
                        i ++;
                        MskOptmLoad ((MskTtextRl *)(void *)hEfRl,
                                        &buf[0], cbc_ol);
                    }
                }
                MskElementSet ((MskElement )hEfRl, MskNtextOptMenuMaximum,
                                    (Value )i);
                if (i < cbc_ol->height) {
                    cbc_ol->height = i;
                }
            }
            closedir(dir);
            break;
        default:
            break;
        }
        break;

    default:
        break;
    }
    return (EFCBM_CONTINUE);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS 
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*              Check Mask values
-* RETURNS
-*     -1 ... case error
-*      0 ... everything alright
-*----------------------------------------------------------------------------*/
int CheckValuesArea (MskTmaskRlPtr hMaskInfo, VisInfoCtxt *ptCtxt) {

    if (ptCtxt->tVisArea.visareaVisWidth < 200) {
        WamasBox (SHELL_OF (hMaskInfo), 
                  WboxNboxType,   WBOX_ALERT,
                  WboxNbutton,    WboxbNok,  
                  WboxNmwmTitle,  MlM ("Alert"),
                  WboxNtext,                    
                  MlM ("Minimumwert für Maskenbreite ist 200\n"
                       "Maskenbreite ersetzt durch 200."),
                  NULL);
        ptCtxt->tVisArea.visareaVisWidth = 200;
        MskUpdateMaskVar (hMaskInfo);
        return (-1);
    }

