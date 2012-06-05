/****************************************************************************
+* PROJECT:   SCHLAU
+* PACKAGE:   Listen
+* FILE:      verlliste_util.c
+* CONTENTS:  Verladeliste
+* COPYRIGHT NOTICE:
+*         (c) Copyright 2001 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+****************************************************************************/

/* ===========================================================================
 * INCLUDES
 * =========================================================================*/

#include <fstream>

#ifdef WIN32
#include <windows.h>
#include <winsock.h>
#endif /* WIN32 */

#ifndef WIN32
#include <dirent.h>
#else
#include <direct.h>
#endif

#include <sys/stat.h>
#include <stddef.h>
#include <dbsqlstd.h>
#include <math.h>
#include <elements.h>
#include <logtool2.h>
#include <mlpack.h>
#include <stime.h>
#include <sbits.h>
#include <sql_util.h>

#include "ml.h"
#include "ml_util.h"
#include "pos.h"
#include "prot_util.h"
#include "prn_util.h"
#include "sqllist.h"
#include "mng_util.h"
#include "init_table.h"
#include "makefilter.h"
#include "facility.h"
#include "parameter.h"
#include "hist_util.h"

#include "vpl_util2.h"

#include "verlliste_util.h"
#include "aus_util.h"
#include "last.h"
#include "wea.h"
#include "primanut.h"
#include "barcode.h"
#include "wamas.h"
#include "ascii2pcl.h"
#include "reallocx.h"
#include "crossdock.h"
#include <xml.h>

#define BLOCKSIZE   	200
#define BARCODE_LEN 	3500
#define	CLEAN_DIR		100
#define FAC_DEFAULT 	"default"

#define PN_FORMFEED 	"FORMFEED"

#define ESC_SEQ         27
#define BACKSLASH		"\\"


using namespace Tools;

typedef struct _LIB_TITLE {
    char acFormFeed [6];
} LIB_TITLE;

#undef _o
#define _o(x) offsetof (LIB_TITLE,x)

static elTitem shFORMFEED_ITEM [] = {
	{"FormFeed",   {0}, {4, "%s", _o (acFormFeed),  EL_STR},},
	{NULL}
};

static elTitem it_Colli[] = {
    {"SollHe",  {0}, {9, "%9.0lf", offsetof(LIVERLCOLLI, SollHe), EL_DOUBLE},},
    {"SollRest",{0}, {13, "%13.3lf",offsetof(LIVERLCOLLI,SollRest),EL_DOUBLE},},
    {"IstHe",   {0}, {9, "%9.0lf", offsetof(LIVERLCOLLI, IstHe),  EL_DOUBLE},},
    {"IstRest", {0}, {13, "%13.3lf",offsetof(LIVERLCOLLI,IstRest), EL_DOUBLE},},
    {"BuchHe",  {0}, {9, "%9.0lf", offsetof(LIVERLCOLLI, BuchHe), EL_DOUBLE},},
    {"BuchRest",{0}, {13, "%13.3lf",offsetof(LIVERLCOLLI,BuchRest),EL_DOUBLE},},
	{NULL }
};

static elTitem it_Ber[] = {
	{"Bereich",  {0}, {4, "%4ld", offsetof(LIVERLBER, Bereich),   EL_LONG},},
	{NULL}
};

static elTitem it_MyPOS[] = {
 {"FeldId",{0},{FELDID_LEN,"%-14.14s",offsetof(POS, FeldId),EL_STR}},
 {"Offs",{0},{0,(char*)elp_OFFS,offsetof(POS, Offs),EL_SUB}},
 {"Ras",{0},{0,(char*)elp_RAS,offsetof(POS,Ras),EL_SUB}},
 {"DptNr",{0},{DPTNR_LEN, "%9ld",offsetof(POS,DptNr),EL_LONG}},
 {NULL}
};

static elTitem it_Footer[] = {
    {"SumGewNet", {0},{5,"%5.0lf",offsetof(LIVERLFOOTER,SumGewNet),EL_DOUBLE},},
    {"SumGewBrut",{0},{5,"%5.0lf",offsetof(LIVERLFOOTER,SumGewBrut),EL_DOUBLE},}
	, {"SumTE",  {0}, {5, "%5ld", offsetof(LIVERLFOOTER, SumTE),   EL_LONG},},
	{NULL }
};

static elTitem it_Footer_KuNr[] = {
	{"ArtNr",  {0}, {15, "%s", offsetof(LIVERLFOOTERKUNR, ArtNr),  EL_STR},},
	{"ArtBez", {0}, {35, "%s", offsetof(LIVERLFOOTERKUNR, ArtBez), EL_STR},},
	{"AnzThm", {0}, {5, "%5ld", offsetof(LIVERLFOOTERKUNR, AnzThm), EL_LONG},},
	{NULL }
};

static elTitem it_Footer_Ausk[] = {
	{"ArtNr",  {0}, {15, "%s", offsetof(LIVERLFOOTERAUSK, ArtNr),  EL_STR},},
	{"ArtBez", {0}, {35, "%s", offsetof(LIVERLFOOTERAUSK, ArtBez), EL_STR},},
	{"AnzThm", {0}, {5, "%5ld", offsetof(LIVERLFOOTERAUSK, AnzThm), EL_LONG},},
	{NULL }
};

static elTitem it_Standort[] = {
	{"Standort",  {0}, {STANDORTBEZ_LEN, 
					  "%s", offsetof(LIVERLSTANDORT, acStandort),  EL_STR},},
	{NULL }
};

static elTlistPart elp_COLLI_LIST[] = {
    {TN_COLLI, sizeof(LIVERLCOLLI), it_Colli},
    {NULL}
};

static elTlistPart elp_BER[] = {
	{TN_BER, sizeof(LIVERLBER), it_Ber},
	{NULL}
};

static elTlistPart elp_FOOTER_AUSK[] = {
    {TN_FOOTER_AUSK, sizeof(LIVERLFOOTERAUSK), it_Footer_Ausk},
    {NULL}
};

static elTlistPart elp_FOOTER_KUNR[] = {
    {TN_FOOTER_KUNR, sizeof(LIVERLFOOTERKUNR), it_Footer_KuNr},
    {NULL}
};


static elTlistPart elp_STANDORT_LIST[] = {
    {PN_STANDORT, sizeof(LIVERLSTANDORT), it_Standort},
    {NULL}
};

static elTlistPart elp_ZIEL_LIST[] = {
    {TN_ZIEL, sizeof(POS), it_MyPOS},
    {NULL}
};

static elTlistPart elp_FOOTER_LIST[] = {
    {PN_FOOTER, sizeof(LIVERLFOOTER), it_Footer},
    {NULL}
};

static elTitem LITOUR_ITEM[] = {
	{TN_TOUREN, {0, NULL}, 	 {0, (char *)elp_TOUREN,
							offsetof (LIVERL, tTour), EL_SUB},},
	{NULL}
};	

static elTitem LIAUSK_ITEM[] = {
	{TN_AUSK,   {1, NULL},   {0, (char *)elp_AUSK, 
							offsetof(LIVERL, tAusk), EL_SUB},},
	{TN_ZIEL, {1, NULL},   {0, (char *)elp_ZIEL_LIST, 
							offsetof(LIVERL, tZiel), EL_SUB},},
	{NULL}
};

static elTitem LITEK_ITEM[] = {
	{TN_TEK,   {1, NULL},   {0, (char *)elp_TEK,
							offsetof(LIVERL, tTek), EL_SUB},},
	{TN_VPLK,   {1, NULL},   {0, (char *)elp_VPLK,
							offsetof(LIVERL, tVplk), EL_SUB},},
	{TN_VPLP,  {1, NULL},   {0, (char *)elp_VPLP,
							offsetof(LIVERL, tVplpOne), EL_SUB},},
	{NULL}
};

static elTitem LIVPLP_ITEM[] = {
	{TN_VPLP,   {1, NULL},   {0, (char *)elp_VPLP,
							offsetof(LIVERL, tVplp), EL_SUB},},
	{TN_COLLI, {1, NULL},   {0, (char *)elp_COLLI_LIST, 
							offsetof(LIVERL, tColli), EL_SUB},},
	{NULL}
};

static elTitem LIVERDVPLP_ITEM[] = {
	{TN_VPLP,   {1, NULL},   {0, (char *)elp_VPLP,
							offsetof(LIVERL, tVplp), EL_SUB},},
	{NULL}
};

static elTitem LITHMVPLP_ITEM[] = {
	{TN_VPLP,   {1, NULL},   {0, (char *)elp_VPLP,
							offsetof(LIVERL, tThmVplp), EL_SUB},},
	{TN_VPLK,   {1, NULL},   {0, (char *)elp_VPLK,
                        	offsetof(LIVERL, tVplk), EL_SUB},},
	{TN_COLLI, {1, NULL},   {0, (char *)elp_COLLI_LIST,
                            offsetof(LIVERL, tColli), EL_SUB},},
	{TN_BER,   {1, NULL},   {0, (char *)elp_BER,
							offsetof(LIVERL, tBereich), EL_SUB},},
	{NULL}
};

static elTitem LIFOOTER_ITEM[] = {
	{PN_FOOTER,   {0, NULL},   {0, (char *)elp_FOOTER_LIST, 
							offsetof(LIVERL, tFooter), EL_SUB},},
	{NULL}
};

static elTitem LISTANDORT_ITEM[] = {
	{PN_STANDORT,   {0, NULL},   {0, (char *)elp_STANDORT_LIST, 
							offsetof(LIVERL, tStandort), EL_SUB},},
	{NULL}
};

static elTitem LIFOOTERAUSK_ITEM[] = {
	{TN_FOOTER_AUSK,   {0, NULL},   {0, (char *)elp_FOOTER_AUSK, 
							offsetof(LIVERL, tFooter_Ausk), EL_SUB},},
	{NULL}
};

static elTitem LIFOOTERKUNR_ITEM[] = {
	{TN_FOOTER_KUNR,   {0, NULL},   {0, (char *)elp_FOOTER_KUNR, 
							offsetof(LIVERL, tFooter_KuNr), EL_SUB},},
	{NULL}
};



elTlistPart elp_LIVERL[] = {
	{PN_TOUR,		sizeof(LIVERL),				LITOUR_ITEM},
	{PN_AUSK, 		sizeof(LIVERL), 			LIAUSK_ITEM},
	{PN_TEK, 		sizeof(LIVERL), 			LITEK_ITEM},
	{PN_VPLP, 		sizeof(LIVERL), 			LIVPLP_ITEM},
	{PN_STANDORT, 	sizeof(LIVERL), 			LISTANDORT_ITEM},
	{PN_VERDVPLP, 	sizeof(LIVERL), 			LIVERDVPLP_ITEM},
	{PN_THMVPLP, 	sizeof(LIVERL), 			LITHMVPLP_ITEM},
	{TN_FOOTER_AUSK,sizeof(LIVERL), 			LIFOOTERAUSK_ITEM},
	{TN_FOOTER_KUNR,sizeof(LIVERL),         	LIFOOTERKUNR_ITEM},
	{PN_FORMFEED, 	sizeof(LIB_TITLE), 			shFORMFEED_ITEM},
	{PN_FOOTER,		sizeof(LIVERL), 			LIFOOTER_ITEM},
	{NULL}
};

elTlist el_liverl = {
	"Verladeliste",
	elp_LIVERL,
	0,
    "SELECT %AUSK FROM AUSK "
};

static int CleanVerlArch (void *pvTid, char *pcDir, long lDays, char *pcFac)
{
	DIR				*ptDir = NULL;
	struct dirent	*ptDirent = NULL;
	struct stat		tStatBuf;
	time_t			zOldest = 0;
	char			acFileName[255+1];
	int				iRv = 0;

	/* --- facility check --- */
	if (pcFac == NULL) {
		pcFac = FAC_DEFAULT;
	}

	/* --- check other parameters --- */
	if (pcDir == NULL) {
		LogPrintf (pcFac, LT_NOTIFY,
			"<CleanVerlArch>: no directory transfered!");
		return (-1);
	}

	if (lDays == 0) {
		LogPrintf (pcFac, LT_NOTIFY,
			"<CleanVerlArch>: no value how long data should be stored!\n"
			"default value: %d",
			CLEAN_DIR);
		lDays = CLEAN_DIR;
	}

	/* --- open directory --- */
	ptDir = opendir (pcDir);
	if (ptDir == NULL) {
		LogPrintf (pcFac, LT_NOTIFY,
			"<CleanVerlArch>: cannot open directory [%s]",
			pcDir);
		return (-1);
	}

	/* --- remove all files which are older than <lDays> days --- */
	zOldest = today_morning () - (86400 * lDays);

	while ((ptDirent = readdir (ptDir)) != NULL) {
		if (strcmp (ptDirent->d_name, ".") == 0 ||
			strcmp (ptDirent->d_name, "..") == 0) {
			continue;
		}			
		
		memset (&acFileName[0], '\0', sizeof (acFileName));
		sprintf (acFileName, "%s/%s", pcDir, ptDirent->d_name);

		stat (acFileName, &tStatBuf);
		if (tStatBuf.st_mtime < zOldest) {
			LogPrintf (pcFac, LT_ALERT,
				"<CleanVerlArch>: delete file [%s]",
				acFileName);
			iRv = unlink (acFileName);
			if (iRv < 0) {
				LogPrintf (pcFac, LT_ALERT,
					"<CleanVerlArch>: remove file [%s] failed!",
					acFileName);
			}
		}	
	}
	closedir (ptDir);

	return (0);
}

/***************************************************************************
 * FUNCTION HEADER:														   *
 * 	static int VerlListArch (...);										   *
 * DESCRIPTION:															   *
 * 	archivates lists after printing in directory which is given by the	   *
 *	the env ENVVERLARCH													   *
 * RETURN VALUES:														   *
 * 	<>																	   *
 ***************************************************************************/
static int VerlListArch (void *pvTid, char *pcFileName, AUSK *ptAusk,
						 char *pcFac)
{
	char 		*pcDir = NULL;
	TOUREN		tTour;
	int			iPrmRv = 0, iRv = 0, iDbRv = 0;
	long		lVerlArchDel = 0;
	time_t		zNow = 0;
	char		acYear[2+1], acVerlkZeit[6+1], acCmd[255+1];
	char		acFileName[8+2+6+3+1];
		/* TOURID = 7 ... YEAR = 2 ... TIME = 6 ... "-" = 3 ... '\0' = 1 */

	/* --- check parameter P_VerlArchDel --- */		
	iPrmRv = PrmGet1Parameter (pvTid, P_VerlArchDel, PRM_CACHE, &lVerlArchDel);
	if (iPrmRv != PRM_OK || lVerlArchDel == 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<VerlListArch>: paramert P_VerlArchDel isn't defined or 0!");
		lVerlArchDel = 0;
	}
	
	/* --- get the directory of the archive --- */
	pcDir = getenv ("ENVVERLARCH");
	if (pcDir == NULL) {
		LogPrintf (pcFac, LT_ALERT,
			"<VerlListArch>: the ENV variable isn't defined yet!");
		return (-1);
	}

	/* --- clean directory depend of the parameter --- */
	if (lVerlArchDel != 0) {
		iRv = CleanVerlArch (pvTid, pcDir, lVerlArchDel, pcFac);
		if (iRv < 0) {
			LogPrintf (pcFac, LT_ALERT,	
				"<VerlListArch>: delete old lists failed!");
		}
	} else if( lVerlArchDel == 0) {
		return 0;
	}

	/* --- get tour data for read the time --- */
	memset (&tTour, 0, sizeof (tTour));
	tTour.tourTourId = ptAusk->auskTourId;

	iDbRv = TExecStdSql (pvTid, StdNselect, TN_TOUREN, &tTour);
	if (iDbRv <= 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<VerlListArch>: error reading TOUREN!\n"
			"Error: %s",
			TSqlErrTxt (pvTid));
		return (-1);
	}

	/* --- get the actual year --- */
	memset (&acYear[0], '\0', sizeof (acYear));

	zNow = time ((time_t)0);
	strftime (acYear, sizeof (acYear), "%y", localtime (&zNow));

	/* --- get verlktime --- */
	memset (&acVerlkZeit[0], '\0', sizeof (acVerlkZeit));

	strftime (acVerlkZeit, sizeof (acVerlkZeit), "%H%M%S", 
		localtime (&tTour.tourVerlkZeit));

	/* --- build string for archivating new list --- */
	memset (&acFileName[0], '\0', sizeof (acFileName));
	
	sprintf (acFileName, "%s-%s-%s",
		tTour.tourTourId.Tour,
		acYear,
		acVerlkZeit);

	/* --- build command to archivate the list --- */
	memset (&acCmd[0], '\0', sizeof (acCmd));

	sprintf (acCmd, "cp %s %s/%s",
		pcFileName, pcDir, acFileName);

	iRv = system (acCmd);
	if (iRv < 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<VerlListArch>: archivate list failed!\n"
			"Cmd: %s",
			acCmd);
		return (-1);
	}

	/* --- compress saved file --- */
	memset (&acCmd[0], '\0', sizeof (acCmd));

	sprintf (acCmd, "gzip %s/%s", pcDir, acFileName);

	iRv = system (acCmd);
	if (iRv < 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<VerlListArch>: compress list failed!\n"
			"Cmd: %s",
			acCmd);
		return (-1);
	}

	return (0);
}	

void GetAutypStmt(long lAutyp, char *pcStmt) 
{
    if ((lAutyp & TOURAUTYP_WA) &&
        (!(lAutyp & TOURAUTYP_TE)) &&
        (lAutyp & TOURAUTYP_GA) &&
        (lAutyp & TOURAUTYP_ANDERE)) {

        sprintf (pcStmt,
            "AUSK.TYP NOT IN ('TE') ");

    } else if ((lAutyp & TOURAUTYP_WA) &&
        (lAutyp & TOURAUTYP_TE) &&
        (lAutyp & TOURAUTYP_GA) &&
        (!(lAutyp & TOURAUTYP_ANDERE))) {

        sprintf (pcStmt,
            "AUSK.TYP IN ('WA', 'TE', 'GA')  ");

    } else if ((lAutyp & TOURAUTYP_WA) &&
               ((lAutyp & TOURAUTYP_TE) == 0) &&
               ((lAutyp & TOURAUTYP_GA) == 0) &&
               (lAutyp & TOURAUTYP_ANDERE)) {

        sprintf (pcStmt,
                "AUSK.TYP NOT IN ('TE')  ");

	} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   (lAutyp & TOURAUTYP_GA) &&
			   (lAutyp & TOURAUTYP_ANDERE)) {

		sprintf (pcStmt,
				"AUSK.TYP != 'WA'  ");

	} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {

		sprintf (pcStmt,
				"AUSK.TYP = 'TE'  ");

	} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   ((lAutyp & TOURAUTYP_TE) == 0) &&
			   (lAutyp & TOURAUTYP_GA) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {

		sprintf (pcStmt,
				"AUSK.TYP = 'GA'  ");

	} else if ((lAutyp & TOURAUTYP_WA) &&
			   ((lAutyp & TOURAUTYP_TE) == 0) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {

		sprintf (pcStmt,
				"AUSK.TYP = 'WA'  ");

	} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   ((lAutyp & TOURAUTYP_TE) == 0) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   (lAutyp & TOURAUTYP_ANDERE)) {

		sprintf (pcStmt,
			"AUSK.TYP NOT IN ('WA', 'TE')  ");
	}
}

static int lm2_dump_verl_kst (LM2_DESC *pld, long page_len, const char *title, 
			 const char *initString, const char *exitString,
			 long lLfdNr, char *pcCode, FILE *fp, char **filename)
{
	struct PrintHeap_struct * PrintHeapPtr;
    FILE                    *hFpOut=NULL;
    struct PrintHeap_struct * HelpPrintHeapPtr;
	LM2_INFO 				* info;
	int						my_fp, head_len;
	long					line, page, no_pages;
	long 					lPagesLen, lTitleLen, lDateLen, lSpace;
	char					acPages[120];
	char					acSep[250];
	char					acTitleStr[250];
	char					acTime[30];
	char					formfeed = '\014';
	char 					* head;
	char 					* text;
	time_t					timer = time(0);
    char                    acBar[BARCODE_LEN+1];
    char                    acLfdNr[50+1];
	char					*pcHelp = NULL;
    JANEIN                  ePrintBarcode = JANEIN_J;
    char                    *pcBarcode = NULL;
	char					*pcStr =NULL;

	/****************************************************************/
	/* query geometry information */
	/****************************************************************/
	if (!(info = lm2_get_info(pld)))
		return(-1);
	if (info->li_no_lins <= 0)
		return(0);

	/****************************************************************/
	/* query header information */
	/****************************************************************/
		if (!(head = lm2_get_head(pld, &head_len)))
			head_len = 7;
		else
			head_len += 8;
		
	/****************************************************************/
	/* calculate page length */
	/****************************************************************/
	if (page_len <= 0) {
		page_len = 100;
		formfeed = 0;		/* don't insert formfeed */
		no_pages = 1;
	}
	else {
		page_len -= head_len;
		no_pages = (info->li_no_lins+page_len-1)/(page_len);
	}

	if (page_len <= 0)
		return(-1);


	memset(acSep, '=', sizeof(acSep));
	acSep[info->li_no_cols] = '\0';

	/****************************************************************/
	/* decide, where to direct the output to */
	/****************************************************************/
	if (!fp) {
		static char sacTmpNam[L_tmpnam];
		if (!(*filename) && !(*filename = tmpnam(sacTmpNam)))
			return(-1);
		if (!(fp = fopen(*filename, "w")))
			return(-1);
		my_fp = 1;
	}
	else
		my_fp = 0;

	/****************************************************************/
	/* generate init sequence */
	/****************************************************************/
	if ( initString != NULL ) fputs( initString, fp );

	/****************************************************************/
	/* generate pages of output */
	/****************************************************************/
	for (line=0, page=1; line < info->li_no_lins; line += page_len, page++) {

		/******************************************************/
		/* query appropriate text segment */
		/******************************************************/
		if (!(text = lm2_get_text(pld, line, page_len)))
			break;

		/******************************************************/
		/* append form feed (if required) */
		/******************************************************/
		if (line > 0 && formfeed)
			if (fputc(formfeed, fp) == EOF)
				break;

		/******************************************************/
		/* output page header (if acTitle not NULL) */
		/******************************************************/
		if (title && (formfeed || line == 0)) {
			memset(acTitleStr, ' ', sizeof(acTitleStr));
			acTitleStr[info->li_no_cols] = '\0';

			sprintf(acPages, LsMessage(MlM("Seite %ld von %ld")), page, no_pages);

			strftime(acTime,sizeof(acTime),"%d.%m.%Y / %H:%M",
					Localtime(&timer));

			lPagesLen = strlen(acPages);
			lTitleLen = strlen(title);
			lDateLen  = strlen(acTime);

			if ( info->li_no_cols >= (lPagesLen+lTitleLen+lDateLen+4) ) {
				lSpace = info->li_no_cols-(lPagesLen+lTitleLen+lDateLen);

				strncpy(acTitleStr, acPages, strlen(acPages));

				strncpy(&acTitleStr[lPagesLen+(lSpace/2)], 
						title, strlen(title));

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols > (lPagesLen+lDateLen+10) ) {
				lSpace = info->li_no_cols-(lPagesLen+lDateLen);

				strncpy(acTitleStr, acPages, strlen(acPages));

				strncpy(&acTitleStr[lPagesLen+2], 
						title, lSpace-4);

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols >= (lPagesLen+lDateLen+1) ) {
				strncpy(acTitleStr, acPages, strlen(acPages));

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols >= lPagesLen+1 ) {
				strcpy(acTitleStr, acPages);	
			}
			else {
				strncpy(acTitleStr, title, info->li_no_cols);
			}

			if (fprintf(fp, "\n\n%s\n\n", acTitleStr) < 0)
				break;
			if (!StringIsEmpty(head)) {
				if (fprintf(fp, "%s\n", acSep) < 0)
					break;
				if (fputs(head, fp) < 0)
					break;
			}
			if (fprintf(fp, "%s\n\n", acSep) < 0)
				break;
		}
		else {
			if (!StringIsEmpty(head)) {
				if (fputs(head, fp) < 0)
					break;
			}
		}

		if (ePrintBarcode == JANEIN_J &&
		    lLfdNr > 0) {
			/* Print Barcode only on first page */
			//ePrintBarcode = JANEIN_N;

			memset (acLfdNr, 0, sizeof (acLfdNr));

			if (ScdGetLocation () == STANDORT_BB) {
				sprintf (acLfdNr, "22%07ld", lLfdNr);
			} else if (ScdGetLocation () == STANDORT_OS) {
				sprintf (acLfdNr, "21%07ld", lLfdNr);
			} else if (ScdGetLocation () == STANDORT_PW) {
				sprintf (acLfdNr, "20%07ld", lLfdNr);
			} else if (ScdGetLocation () == STANDORT_HH) {
				sprintf (acLfdNr, "23%07ld", lLfdNr);
			} else {
				sprintf (acLfdNr, "00%07ld", lLfdNr);
			}

			/* Barcode ans File anhängen
			*/
			PrintHeapPtr = DruckeStethosBarcode(
				 PCLLANGUAGE,	/* PCL */
				 BC2OF5INTPZ,  	/* Barcodetyp */
				 acLfdNr,    	/* Strichcode */
				 2.0,          	/* Strichverhltnis */
				 15,           	/* Höhe in mm */
				 50,           	/* Breite in mm */
				 0,            	/* Druckrichtung: 0 Grad */
				 0,            	/* Human readable text */
				 BCTFCOURIER,  	/* typeface for human readable text */
				 BCSTDEFAULT,  	/* Style for human readable text */
				 pcCode,
				 0, 0, 0);

			if ((BarcodeFehler != 0) || (PrintHeapPtr == NULL)) {
			   /* Error Handling */
			   fprintf (stderr,
						"\nError value: %d", (unsigned int) BarcodeFehler);
			   fprintf (stderr,
						"\nFor further information read ASCII file BC_ERROR.TXT\n");
			   if (PrintHeapPtr == NULL) {
					break;
			   }
			} else {

				/* Print PCL to file */
				fprintf (fp, "%c*p1700X%c*p%dY", ESC_SEQ, ESC_SEQ, 50);

				HelpPrintHeapPtr = PrintHeapPtr;

				while (HelpPrintHeapPtr != NULL) {
				
					memset(acBar, 0, sizeof(acBar));
					strncpy(acBar,(char *)&HelpPrintHeapPtr->PrintData,
								   HelpPrintHeapPtr->HeapAktLen);

					fprintf(fp,"%s\n", acBar);

					HelpPrintHeapPtr = HelpPrintHeapPtr->NextHeapSegment;
				}
			}

			HelpPrintHeapPtr = PrintHeapPtr;
			while (PrintHeapPtr != NULL)
			{
				HelpPrintHeapPtr = PrintHeapPtr->NextHeapSegment;
				free(PrintHeapPtr);
				PrintHeapPtr = HelpPrintHeapPtr;
			}
		}

		/******************************************************/
		/* output body of page */
		/******************************************************/
		if (fputs(text, fp) < 0)
			break;
	}

	/****************************************************************/
	/* generate exit sequence */
	/****************************************************************/
	if ( exitString != NULL ) fputs( exitString, fp );

	/****************************************************************/
	/* close the output stream (if our's) */
	/****************************************************************/
	if (my_fp)
		fclose(fp);

	if (line <= 0)
		return(0);
	return(no_pages);
}

static int lm2_dump_verl(	LM2_DESC *pld, long page_len, const char *title, 
			const char *initString, const char *exitString,
			FILE *fp, char **filename)
{
	int			my_fp, head_len;
	long		line, page, no_pages;
	long 		lPagesLen, lTitleLen, lDateLen, lSpace;
	char		acPages[120];
	char		acSep[250];
	char		acTitleStr[250];
	char		acTime[30];
	char		formfeed = '\014';
	char *		head;
	char *		text;
	time_t		timer = time(0);
	LM2_INFO *	info;

	/****************************************************************/
	/* query geometry information */
	/****************************************************************/
	if (!(info = lm2_get_info(pld)))
		return(-1);
	if (info->li_no_lins <= 0)
		return(0);

	/****************************************************************/
	/* query header information */
	/****************************************************************/
		if (!(head = lm2_get_head(pld, &head_len)))
			head_len = 7;
		else
			head_len += 8;
		
	/****************************************************************/
	/* calculate page length */
	/****************************************************************/
	if (page_len <= 0) {
		page_len = 100;
		formfeed = 0;		/* don't insert formfeed */
		no_pages = 1;
	}
	else {
		page_len -= head_len;
		no_pages = (info->li_no_lins+page_len-1)/(page_len);
	}

	if (page_len <= 0)
		return(-1);


	memset(acSep, '=', sizeof(acSep));
	acSep[info->li_no_cols] = '\0';

	/****************************************************************/
	/* decide, where to direct the output to */
	/****************************************************************/
	if (!fp) {
		static char sacTmpNam[L_tmpnam];
		if (!(*filename) && !(*filename = tmpnam(sacTmpNam)))
			return(-1);
		if (!(fp = fopen(*filename, "w")))
			return(-1);
		my_fp = 1;
	}
	else
		my_fp = 0;

	/****************************************************************/
	/* generate init sequence */
	/****************************************************************/
	if ( initString != NULL ) fputs( initString, fp );

	/****************************************************************/
	/* generate pages of output */
	/****************************************************************/
	for (line=0, page=1; line < info->li_no_lins; line += page_len, page++) {

		/******************************************************/
		/* query appropriate text segment */
		/******************************************************/
		if (!(text = lm2_get_text(pld, line, page_len)))
			break;

		/******************************************************/
		/* append form feed (if required) */
		/******************************************************/
		if (line > 0 && formfeed)
			if (fputc(formfeed, fp) == EOF)
				break;

		/******************************************************/
		/* output page header (if acTitle not NULL) */
		/******************************************************/
		if (title && (formfeed || line == 0)) {
			memset(acTitleStr, ' ', sizeof(acTitleStr));
			acTitleStr[info->li_no_cols] = '\0';

			sprintf(acPages, LsMessage(MlM("Seite %ld von %ld")), page, no_pages);

			strftime(acTime,sizeof(acTime),"%d.%m.%Y / %H:%M",
					Localtime(&timer));

			lPagesLen = strlen(acPages);
			lTitleLen = strlen(title);
			lDateLen  = strlen(acTime);

			if ( info->li_no_cols >= (lPagesLen+lTitleLen+lDateLen+4) ) {
				lSpace = info->li_no_cols-(lPagesLen+lTitleLen+lDateLen);

				strncpy(acTitleStr, acPages, strlen(acPages));

				strncpy(&acTitleStr[lPagesLen+(lSpace/2)], 
						title, strlen(title));

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols > (lPagesLen+lDateLen+10) ) {
				lSpace = info->li_no_cols-(lPagesLen+lDateLen);

				strncpy(acTitleStr, acPages, strlen(acPages));

				strncpy(&acTitleStr[lPagesLen+2], 
						title, lSpace-4);

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols >= (lPagesLen+lDateLen+1) ) {
				strncpy(acTitleStr, acPages, strlen(acPages));

				strcpy(&acTitleStr[info->li_no_cols-strlen(acTime)],
						acTime);
			}
			else if ( info->li_no_cols >= lPagesLen+1 ) {
				strcpy(acTitleStr, acPages);	
			}
			else {
				strncpy(acTitleStr, title, info->li_no_cols);
			}

			if (fprintf(fp, "\n\n%s\n\n", acTitleStr) < 0)
				break;
			if (!StringIsEmpty(head)) {
				if (fprintf(fp, "%s\n", acSep) < 0)
					break;
				if (fputs(head, fp) < 0)
					break;
			}
			if (fprintf(fp, "%s\n\n", acSep) < 0)
				break;
		}
		else {
			if (!StringIsEmpty(head)) {
				if (fputs(head, fp) < 0)
					break;
			}
		}

		/******************************************************/
		/* output body of page */
		/******************************************************/
		if (fputs(text, fp) < 0)
			break;
	}

	/****************************************************************/
	/* generate exit sequence */
	/****************************************************************/
	if ( exitString != NULL ) fputs( exitString, fp );

	/****************************************************************/
	/* close the output stream (if our's) */
	/****************************************************************/
	if (my_fp)
		fclose(fp);

	if (line <= 0)
		return(0);

	return (no_pages);
}

int PrintVerlListeKst (void *pvTid, char *pcFac, AUSK *ptAusk,
					   TOUREN *ptTouren, int iAufrufKz, long lVerKontKz,
					   long lAutyp, char *pcKuNr, PRIMANDOPRINT *ptPrnPrn,
					   int iKundePerSeite, int iErwVerlList, int iIsLastPos)
{

    AUSK        		atAusk[BLOCKSIZE], tAusk, tAuskSave;
    LIVERL      		tLiVerl;
	elThandle   		*handle;
	SqlContext  		*hCtx;
	LIB_TITLE 			tTitle;
	LAST				tLast;
	time_t      		zNow;
	char	   		*pcFileName = NULL;
	char	    		*pcFileName2 = NULL;
	char    			*pcRootDir = NULL;
	char				acFileName[L_tmpnam+1];
	char				acFileName2[L_tmpnam+1];
	char				acFileNameMore[L_tmpnam+1];
	char        		acStmt[2048], acStmt1[1024], acStmt2[1024];
	char				acLagId[LAGID_LEN+1];
	char				acArtBez[BLOCKSIZE][ARTBEZ_LEN+1];
	char				acArtNr[BLOCKSIZE][ARTNR_LEN+1];
	char        		acCmd[2048+1];
	char				acSpool[30+1];
	char        		acDate[8+1];
	char        		acBackslash[1+1];
	char                acBarcode[BARCODE_LEN+1];
	double				dSumGewNet, dGesSumGewNet;
	double				dSumGewBrut, dGesSumGewBrut;
	long				lThmAnz[BLOCKSIZE];
	long				lLfdNr = 0;
	long				lSumTE, lGesSumTE;
	long				lAnzVerlListe = 0;
	int					iDruckTour = 0, iFoundNormalAusk=0;
	int         		iRvDbThm = 0, iNoCrossdocking=0;
	int					iRvDb = 0, iRvPrm = 0;
	int					nK = 0, iNoTE = 0;
	int					nJ = 0, nE = 0;
	int         		nI = 0, iRv = 0, iOk = 0, iFirst = 0, iPrintKunde = 0;
	int					iPrintNewPage = 0;
	int					iFormFeed = JANEIN_N;
	FILE 				*fp =NULL;

	memset (acSpool, 0, sizeof (acSpool));
	memset (acDate, 0, sizeof (acDate));
	memset (acBackslash, 0, sizeof (acBackslash));
	memset (acBarcode, 0, sizeof (acBarcode));

    /* next pages will be feed */
    memset (&tTitle, 0, sizeof (tTitle));
    strcpy (tTitle.acFormFeed, "\f");

	memset (acArtBez, 0, sizeof(acArtBez));
	memset (acArtNr, 0, sizeof(acArtNr));

	hCtx = TSqlNewContext (pvTid, NULL);
	if (hCtx == NULL) {
		return -1;
	}

	/* Druckfile erstellen */
	if (iErwVerlList == JANEIN_J) {
		handle = elCreateList (&el_liverl);
		if ( elOpenList(handle, "lists.ger", "VERLLISTKD" ) == NULL) {
			handle = elDestroyList (handle);
			(void)TSqlDestroyContext (pvTid, hCtx);
			return ( -1 );
		}
	} else {
		handle = elCreateList (&el_liverl);
		if ( elOpenList(handle, "lists.ger", "VERLLISTKN" ) == NULL) {
			handle = elDestroyList (handle);
			(void)TSqlDestroyContext (pvTid, hCtx);
			return ( -1 );
		}
	}

	/********************************************************************
	-* Drucken aller Auslagerauftraege die im aktuellen Standort
	-* kommissionier bzw. ausgelagert wurden
	-*/
	if (iAufrufKz == VERLLIST_AUSUEB) {
		if (ptAusk == NULL) {
            LogPrintf(pcFac,LT_ALERT,
                "PrintVerlListe: VERLLIST_AUSUEB aber ptAusk=NULL");
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
		}
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT %%AUSK FROM AUSK "
				"WHERE AUSK.AusId_Mand  = :a  "
				"  AND AUSK.AusId_AusNr = :b  "
				"  AND AUSK.AusId_AusKz = :c  "
				"  AND AUSK.STATUS <> 'FERTIG' "
				"ORDER BY AUSK.KuNr,"
				"		  AUSK.KuPosNr, "
				"		  AUSK.AUSID_MAND,"
				"         AUSK.AUSID_AUSNR,"
				"         AUSK.AUSID_AUSKZ");
	} else {
		if (ptTouren == NULL || pcKuNr == NULL) {
            LogPrintf(pcFac,LT_ALERT,
                "PrintVerlListe: VERLLIST_TOURUEB aber ptTouren=NULL bzw. pcKuNr=NULL");
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
		}
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT %%AUSK FROM AUSK "
				"WHERE AUSK.TOURID_MAND  = :a  "
				"  AND AUSK.TOURID_TOUR  = :b  "
				"  AND AUSK.TOURID_POSNR = :c  "
				"  AND AUSK.KuNr = :d "
				"  AND AUSK.STATUS <> 'FERTIG' "
				"  AND EXISTS (SELECT 1 FROM VPLP WHERE "
							" VPLP.AUSID_AUSNR = AUSK.AUSID_AUSNR AND "
							" VPLP.AUSID_AUSKZ = AUSK.AUSID_AUSKZ AND "
							" VPLP.ISTMNGS_MNG != 0) ");

		if (lAutyp != 0) {
			memset (acStmt1,0,sizeof(acStmt1));
			memset (acStmt2,0,sizeof(acStmt2));
			GetAutypStmt(lAutyp, acStmt1);

			if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur TE selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
						"AND %s "
						"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

			} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   ((lAutyp & TOURAUTYP_TE) == 0) &&
			   (lAutyp & TOURAUTYP_GA) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur GA selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
						"AND %s "
						"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
			} else {
				sprintf (acStmt2,
						"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", acStmt1);
			}
			strcat (acStmt, acStmt2);
		}

		strcat (acStmt,
				"ORDER BY AUSK.LADERF DESC, "
				"		  AUSK.KuNr, AUSK.KuPosNr, "
				"		  AUSK.AUSID_AUSKZ DESC, "
				"		  AUSK.STANDORT, "
				"		  AUSK.AUSID_MAND,"
				"         AUSK.AUSID_AUSNR");

		LogPrintf(pcFac,LT_ALERT,
			"PrintVerlListe: Tour:%s: -> stmt:%s",
			ptTouren->tourTourId.Tour,
			acStmt);
	}


	iRv = 0;
	iFirst = 1;
	dGesSumGewNet = 0;
	dGesSumGewBrut = 0;
	lGesSumTE = 0;
	memset(&tAuskSave, 0, sizeof(tAuskSave));

	do {
		memset (&atAusk[0], 0, sizeof (atAusk));

		if (iAufrufKz == VERLLIST_AUSUEB) {
			iRv = iRv == 0 ?
				TExecSqlX (pvTid, hCtx,
					acStmt,
					BLOCKSIZE, 0,
					SELSTRUCT (TN_AUSK, atAusk[0]),
					SQLSTRING (ptAusk->auskAusId.Mand),
					SQLSTRING (ptAusk->auskAusId.AusNr),
					SQLAUSKZ (ptAusk->auskAusId.AusKz),
					NULL) :
				TExecSqlV (pvTid, hCtx, NULL, NULL, NULL, NULL);

			if (iFirst == 1 && iRv < 0 && TSqlError(pvTid) == SqlNotFound) {
				LogPrintf(pcFac,LT_ALERT,
					"Auftrag nicht gefunden : %s %s %ld ",
					ptAusk->auskAusId.Mand,
					ptAusk->auskAusId.AusNr,
					l2sGetNameByValue(&l2s_AUSKZ, ptAusk->auskAusId.AusKz));
				(void)TSqlDestroyContext (pvTid, hCtx);
				handle = elDestroyList (handle);
				return (-1);
			}
		} else {
			iRv = iRv == 0 ?
				TExecSqlX (pvTid, hCtx,
					acStmt,
					BLOCKSIZE, 0,
					SELSTRUCT (TN_AUSK, atAusk[0]),
					SQLSTRING (ptTouren->tourTourId.Mand),
					SQLSTRING (ptTouren->tourTourId.Tour),
					SQLLONG (ptTouren->tourTourId.PosNr),
					SQLSTRING (pcKuNr),
					NULL) :
				TExecSqlV (pvTid, hCtx, NULL, NULL, NULL, NULL);

			if (iFirst == 1 && iRv < 0 && TSqlError(pvTid) == SqlNotFound) {
				LogPrintf(pcFac,LT_ALERT,
					"Keine Auftraege für Tour: %s/%s/%ld ",
					ptTouren->tourTourId.Mand,
					ptTouren->tourTourId.Tour,
					ptTouren->tourTourId.PosNr);
				break;
			}
		}

        if (iRv < 0 && TSqlError(pvTid) != SqlNotFound) {
            LogPrintf(pcFac,LT_ALERT,
                "Ein Fehler beim Lesen der Aufträge "
                    "ist aufgetreten!\n TSqlErr: %s",
                TSqlErrTxt(pvTid));
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
        }

        if (iRv > 0) {
        	iFoundNormalAusk = 1;
        }


		iFirst = 0;

		for( nI=0; nI<iRv; nI++) {

			/* Kundennummer, right Addresse */

			if (atAusk[nI].auskKuPosNr > 0) {
				/* Set Right Adresse */

				memset (&tLast, 0, sizeof(tLast));
				
				strcpy(tLast.lastKuNr, atAusk[nI].auskKuNr);
				tLast.lastKuPosNr = atAusk[nI].auskKuPosNr;
				strcpy(tLast.lastMand, atAusk[nI].auskAusId.Mand);
			
				iRvDb = TExecStdSql (pvTid, StdNselect, TN_LAST, &tLast);

				if (iRvDb <= 0 && TSqlError(pvTid) != SqlNotFound) {
					/* Db - Error */
					 LogPrintf(pcFac,LT_ALERT,
                		"Ein Fehler beim Lesen der Aufträge "
                    	"ist aufgetreten!\n TSqlErr: %s",
                		TSqlErrTxt(pvTid));
						(void)TSqlDestroyContext (pvTid, hCtx);
						handle = elDestroyList (handle);
            			return (-1);
				}		

				for(nK = 0; nK < (int) strlen(atAusk[nI].auskKuName); nK++) {

					if (atAusk[nI].auskKuName[nK] == ',') {
						/* New Ku Ort */
						strncpy(&(atAusk[nI].auskKuName[nK+2]), 
											tLast.lastAdrs.Ort, 
											(KUNAME_LEN - (nK+2)));

						atAusk[nI].auskKuName[KUNAME_LEN] = '\0';
						break;

					}
				}
			}

			memset(&tLiVerl, 0, sizeof(LIVERL));
			memset(&tAusk, 0,           sizeof(AUSK));
			memcpy(&tAusk, &atAusk[nI], sizeof(AUSK));
			dSumGewNet = 0;
			dSumGewBrut = 0;
			lSumTE = 0;

			/* Verladelisteeintrag fuer die TE eines
			-* Auftrages machen
			-*/

			LogPrintf(pcFac,LT_ALERT,
					"PrintSAVE ausid %s kunr: %s posnr %d crossdocking %d", 
					tAuskSave.auskAusId.AusNr,
					tAuskSave.auskKuNr,
					tAuskSave.auskKuPosNr,
					tAuskSave.auskCrossDocking); 

			LogPrintf(pcFac,LT_ALERT,
					"Printakt ausid %s kunr: %s posnr %d crossdocking %d", 
					atAusk[nI].auskAusId.AusNr,
					atAusk[nI].auskKuNr,
					tAuskSave.auskKuPosNr,
					atAusk[nI].auskCrossDocking); 


			if ((tAuskSave.auskStandort != atAusk[nI].auskStandort) ||
				(strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
				(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {

				if ((strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
					(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {
					iPrintKunde = 1;
					 LogPrintf(pcFac,LT_ALERT,"PrintKunde 1");
				} else {
					iPrintKunde = 0;
					 LogPrintf(pcFac,LT_ALERT,"PrintKunde 0");
				}

				if (iAufrufKz != VERLLIST_AUSUEB &&
					iPrintKunde == 1 &&
					iPrintNewPage == 1) {

					memset (&tLiVerl, 0, sizeof (tLiVerl));
					memset (acStmt,0,sizeof(acStmt));
					sprintf (acStmt,
							"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
								" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
								" VPLK WHERE "
								" 	  VPLK.KSNR = VPLP.KSNR "
								" AND VPLP.BUCHMNGS_MNG > 0 " 
								" AND VPLK.TourId_Mand  = :Mand  "
								" AND VPLK.TourId_Tour  = :Tour  "
								" AND VPLK.TourId_PosNr = :PosNr "
								" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
								" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
								" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
								" AND VPLP.ThmKz = 1 "
								" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
								" AND AUSK.KUNR = :KuNr "
								" AND AUSK.KUPOSNR = :KuPosNr ");

					if (lAutyp != 0) {
						memset (acStmt1,0,sizeof(acStmt1));
						memset (acStmt2,0,sizeof(acStmt2));
						GetAutypStmt(lAutyp, acStmt1);

						if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   (lAutyp & TOURAUTYP_TE) &&
						   ((lAutyp & TOURAUTYP_GA) == 0) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
							/* Nur TE selektiert -> kein Crossdocking
							-*/
							sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

						} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   ((lAutyp & TOURAUTYP_TE) == 0) &&
						   (lAutyp & TOURAUTYP_GA) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
							/* Nur GA selektiert -> kein Crossdocking
							-*/
							sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
						} else {
							sprintf (acStmt2,
								"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
								acStmt1);
						}
						strcat (acStmt, acStmt2);
					}

					strcat (acStmt,
							" GROUP BY VPLP.ARTBEZ, "
							"		   VPLP.MID_AID_ARTNR "
							" ORDER BY VPLP.ARTBEZ, "
							"		   VPLP.MID_AID_ARTNR "); 

					LogPrintf(pcFac,LT_ALERT,
						"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
						ptTouren->tourTourId.Tour,
						acStmt);

					iRvDbThm = TExecSqlX (pvTid, NULL, 
										acStmt,
										BLOCKSIZE, 0,
										SELSTR (acArtBez[0], ARTBEZ_LEN+1),
										SELSTR (acArtNr[0], ARTNR_LEN+1),
										SELLONG	(lThmAnz[0]),
										SQLSTRING (ptTouren->tourTourId.Mand),
										SQLSTRING (ptTouren->tourTourId.Tour),
										SQLLONG   (ptTouren->tourTourId.PosNr),	
										SQLSTRING (tAuskSave.auskKuNr),
										SQLLONG   (tAuskSave.auskKuPosNr),
										NULL);	

					if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
						(void)TSqlDestroyContext (pvTid, hCtx);
						handle = elDestroyList (handle);
						return (-1);
					}
					for (nE = 0; nE < iRvDbThm; nE++) {
						sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
													"%s", acArtBez[nE]);
						sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
													"%s", acArtNr[nE]);
						tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
							
						elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
															&tLiVerl, NULL);
						elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
															&tLiVerl, NULL);
					}

					if (iRvDbThm > 0 && 
						iKundePerSeite == JANEIN_J) {

						iFormFeed = JANEIN_J;
						iDruckTour = 0;
					}
				}

				if ((strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
					(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {
					if (iFormFeed == JANEIN_J &&
						iKundePerSeite == JANEIN_J) {
						if (elgWrite (handle, elNbody, PN_FORMFEED,
									(void *)&tTitle, NULL) < 0) {
							handle = elDestroyList (handle);
							(void)TSqlDestroyContext (pvTid, hCtx);
							return (-1);
						}

						iFormFeed = JANEIN_N;
					}
				}	


				if (atAusk[nI].auskAusId.AusKz == AUSKZ_INT) {
					/* Crossdocking-Auslagerauftrag */
                    memcpy(&tAuskSave, &atAusk[nI], sizeof(AUSK));
                    if (li_verlliste_kunr_cross ( pvTid,
                                                &atAusk[nI],
                                                &dSumGewNet,
                                                &dSumGewBrut,
                                                &lSumTE,
                                                handle,
                                                NULL,
                                                iAufrufKz,
                                                &iDruckTour,
                                                lVerKontKz,
                                                lAutyp,
                                                nI,
                                                iPrintKunde) < 0) {
                        handle = elDestroyList (handle);
                        (void)TSqlDestroyContext (pvTid, hCtx);
                        return -1;
                    } else {
                        iOk = 1;
                        dGesSumGewNet  = dGesSumGewNet + dSumGewNet;
                        dGesSumGewBrut = dGesSumGewBrut + dSumGewBrut;
                        lGesSumTE      = lGesSumTE + lSumTE;
                    }
					iPrintNewPage = 1;
				} else {
					/* Auslagerauftrag vom aktuellen Standort 
					-*/
					memcpy(&tAuskSave, &atAusk[nI], sizeof(AUSK));
					if (li_verlliste_kunr ( pvTid,
												&atAusk[nI],
												&dSumGewNet,
												&dSumGewBrut,
												&lSumTE,
												handle,
												NULL,
												iAufrufKz, 
												&iDruckTour,
												lVerKontKz,
                                                lAutyp,
												nI,
												iPrintKunde) < 0) {
						handle = elDestroyList (handle);
						(void)TSqlDestroyContext (pvTid, hCtx);
						return -1;
					} else {
						iOk = 1;
						dGesSumGewNet  = dGesSumGewNet + dSumGewNet;
						dGesSumGewBrut = dGesSumGewBrut + dSumGewBrut;
						lGesSumTE      = lGesSumTE + lSumTE;
					}

					if (iAufrufKz != VERLLIST_AUSUEB) {

						memset (&tLiVerl, 0, sizeof (tLiVerl));
						memset (acStmt,0,sizeof(acStmt));
						sprintf (acStmt,
								"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
									" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
									" VPLK WHERE "
									" 	  VPLK.KSNR = VPLP.KSNR "
									" AND VPLP.BUCHMNGS_MNG > 0 " 
									" AND VPLK.TourId_Mand  = :Mand  "
									" AND VPLK.TourId_Tour  = :Tour  "
									" AND VPLK.TourId_PosNr = :PosNr "
									" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
									" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
									" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
									" AND VPLP.ThmKz = 1 "
									" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
									" AND AUSK.KUNR = :KuNr "
									" AND AUSK.KUPOSNR = :KuPosNr ");

						if (lAutyp != 0) {
							memset (acStmt1,0,sizeof(acStmt1));
							memset (acStmt2,0,sizeof(acStmt2));
							GetAutypStmt(lAutyp, acStmt1);

							if (((lAutyp & TOURAUTYP_WA) == 0) &&
							   (lAutyp & TOURAUTYP_TE) &&
							   ((lAutyp & TOURAUTYP_GA) == 0) &&
							   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
								/* Nur TE selektiert -> kein Crossdocking
								-*/
								sprintf (acStmt2,
									"AND %s "
									"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

							} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
							   ((lAutyp & TOURAUTYP_TE) == 0) &&
							   (lAutyp & TOURAUTYP_GA) &&
							   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
								/* Nur GA selektiert -> kein Crossdocking
								-*/
								sprintf (acStmt2,
									"AND %s "
									"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
							} else {
								sprintf (acStmt2,
									"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
									acStmt1);
							}
							strcat (acStmt, acStmt2);
						}

						strcat (acStmt,
								" GROUP BY VPLP.ARTBEZ, "
								"		   VPLP.MID_AID_ARTNR "
								" ORDER BY VPLP.ARTBEZ, "
								"		   VPLP.MID_AID_ARTNR "); 

						LogPrintf(pcFac,LT_ALERT,
							"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
							ptTouren->tourTourId.Tour,
							acStmt);

						iRvDbThm = TExecSqlX (pvTid, NULL, 
											acStmt,
											BLOCKSIZE, 0,
											SELSTR (acArtBez[0], ARTBEZ_LEN+1),
											SELSTR (acArtNr[0], ARTNR_LEN+1),
											SELLONG	(lThmAnz[0]),
											SQLSTRING (ptTouren->tourTourId.Mand),
											SQLSTRING (ptTouren->tourTourId.Tour),
											SQLLONG   (ptTouren->tourTourId.PosNr),	
											SQLSTRING (atAusk[nI].auskKuNr),
											SQLLONG   (atAusk[nI].auskKuPosNr),
											NULL);	

						if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
							(void)TSqlDestroyContext (pvTid, hCtx);
							handle = elDestroyList (handle);
							return (-1);
						}

						for (nE = 0; nE < iRvDbThm; nE++) {
						
							sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
														"%s", acArtBez[nE]);
							sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
														"%s", acArtNr[nE]);
							tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
								
							elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
																&tLiVerl, NULL);
							elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
																&tLiVerl, NULL);
						}

						if (iRvDbThm > 0 && 
							iKundePerSeite == JANEIN_J) {


							/*
						 	 * write FORMFEED entry to the list
						 	 */
							//if (elgWrite (handle, elNbody, PN_FORMFEED,
							//			(void *)&tTitle, NULL) < 0) {
							//	handle = elDestroyList (handle);
							//	(void)TSqlDestroyContext (pvTid, hCtx);
							//	return (-1);
							//}
							iFormFeed = JANEIN_J;
							iDruckTour = 0;
						}
						iPrintNewPage = 0;
					}
				}
			}
		}
		

	} while (iRv == BLOCKSIZE);

	(void)TSqlDestroyContext (pvTid, hCtx);

	if (iAufrufKz != VERLLIST_AUSUEB &&
		iKundePerSeite == 1 &&
		iPrintNewPage == 1) {

		memset (&tLiVerl, 0, sizeof (tLiVerl));
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
					" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
					" VPLK WHERE "
					" 	  VPLK.KSNR = VPLP.KSNR "
					" AND VPLP.BUCHMNGS_MNG > 0 " 
					" AND VPLK.TourId_Mand  = :Mand  "
					" AND VPLK.TourId_Tour  = :Tour  "
					" AND VPLK.TourId_PosNr = :PosNr "
					" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
					" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
					" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
					" AND VPLP.ThmKz = 1 "
					" AND AUSK.KUNR = :KuNr "
					" AND AUSK.KUPOSNR = :KuPosNr ");

		if (lAutyp != 0) {
			memset (acStmt1,0,sizeof(acStmt1));
			memset (acStmt2,0,sizeof(acStmt2));
			GetAutypStmt(lAutyp, acStmt1);

			if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur TE selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
				"AND %s "
				"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

				} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   ((lAutyp & TOURAUTYP_TE) == 0) &&
						   (lAutyp & TOURAUTYP_GA) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
					/* Nur GA selektiert -> kein Crossdocking
					-*/
					sprintf (acStmt2,
					"AND %s "
					"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
				} else {
					sprintf (acStmt2,
					"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
					acStmt1);
				}
			strcat (acStmt, acStmt2);
		}

		strcat (acStmt,
				" GROUP BY VPLP.ARTBEZ, "
				"		   VPLP.MID_AID_ARTNR "
				" ORDER BY VPLP.ARTBEZ, "
				"		   VPLP.MID_AID_ARTNR "); 

		LogPrintf(pcFac,LT_ALERT,
			"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
			ptTouren->tourTourId.Tour,
			acStmt);

		iRvDbThm = TExecSqlX (pvTid, NULL, 
							acStmt,
							BLOCKSIZE, 0,
							SELSTR (acArtBez[0], ARTBEZ_LEN+1),
							SELSTR (acArtNr[0], ARTNR_LEN+1),
							SELLONG	(lThmAnz[0]),
							SQLSTRING (ptTouren->tourTourId.Mand),
							SQLSTRING (ptTouren->tourTourId.Tour),
							SQLLONG   (ptTouren->tourTourId.PosNr),	
							SQLSTRING (tAuskSave.auskKuNr),
							SQLLONG   (tAuskSave.auskKuPosNr),
							NULL);	

		if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
			handle = elDestroyList (handle);
			return (-1);
		}

		for (nE = 0; nE < iRvDbThm; nE++) {
		
			sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
										"%s", acArtBez[nE]);
			sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
										"%s", acArtNr[nE]);
			tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
				
			elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
												&tLiVerl, NULL);
			elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
												&tLiVerl, NULL);
		}

		if (iRvDbThm > 0 && 
			iKundePerSeite == JANEIN_J) {

			/*
			 * write FORMFEED entry to the list
			 */
			if (iIsLastPos == JANEIN_J) {
				if (elgWrite (handle, elNbody, PN_FORMFEED,
							(void *)&tTitle, NULL) < 0) {
					handle = elDestroyList (handle);
					return (-1);
				}
			}

			iDruckTour = 0;
		}
	} else if (iFormFeed == JANEIN_J) {
		if (iIsLastPos == JANEIN_J) {
			if (elgWrite (handle, elNbody, PN_FORMFEED,
						(void *)&tTitle, NULL) < 0) {
				handle = elDestroyList (handle);
				return (-1);
			}
		}

		iFormFeed = JANEIN_N;
	}	

	/****************************************************************
	-* Summenzeile ueber alles (normal + Crosdocking) andrucken
	-*/

	if (lGesSumTE == 0) {

		LogPrintf (pcFac, LT_ALERT, "PrintVerlListe: Keine TE fuer Verladeliste");

		if (iKundePerSeite == JANEIN_J && iIsLastPos == JANEIN_J) {
			iOk   = 1;
			iNoTE = 1;
		} else {
			handle = elDestroyList (handle);
			return (0);
		}
	} 
	if (iOk != 1) {
		LogPrintf(pcFac, LT_ALERT,
			"Es kann keine Verladeliste gedruckt werden");
		handle = elDestroyList (handle);
		return (-1);
	} else {
		/*
		tLiVerl.tFooter.SumGewNet  = dSumGewNet;
		tLiVerl.tFooter.SumGewBrut = dSumGewBrut;
		tLiVerl.tFooter.SumTE      = lSumTE;
		elgWrite (handle,
				  elNheader, PN_FOOTER, &tLiVerl, NULL);
		elgWrite (handle,
				  elNbody, PN_FOOTER, &tLiVerl, NULL);
		*/

		memset(&tLiVerl, 0, sizeof(tLiVerl));
		tLiVerl.tFooter.SumGewNet  = dGesSumGewNet;
		tLiVerl.tFooter.SumGewBrut = dGesSumGewBrut;
		tLiVerl.tFooter.SumTE      = lGesSumTE;

		elgWrite (handle,
				  elNheader, PN_FOOTER, &tLiVerl, NULL);
		elgWrite (handle,
				  elNbody, PN_FOOTER, &tLiVerl, NULL);
	
		if (iIsLastPos == JANEIN_J) {
			if (iAufrufKz != VERLLIST_AUSUEB) {	

				memset (&tLiVerl, 0, sizeof (tLiVerl));
				memset (acStmt,0,sizeof(acStmt));
				sprintf (acStmt,
						"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
							" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
							" VPLK WHERE "
							" 	  VPLK.KSNR = VPLP.KSNR "
							" AND VPLP.BUCHMNGS_MNG > 0 " 
							" AND VPLK.TourId_Mand  = :Mand  "
							" AND VPLK.TourId_Tour  = :Tour  "
							" AND VPLK.TourId_PosNr = :PosNr "
							" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
							" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
							" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
							" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
							" AND VPLP.ThmKz = 1");

				if (lAutyp != 0) {
					memset (acStmt1,0,sizeof(acStmt1));
					memset (acStmt2,0,sizeof(acStmt2));
					GetAutypStmt(lAutyp, acStmt1);

					if (((lAutyp & TOURAUTYP_WA) == 0) &&
					   (lAutyp & TOURAUTYP_TE) &&
					   ((lAutyp & TOURAUTYP_GA) == 0) &&
					   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
						/* Nur TE selektiert -> kein Crossdocking
						-*/
						sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
					} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
					   ((lAutyp & TOURAUTYP_TE) == 0) &&
					   (lAutyp & TOURAUTYP_GA) &&
					   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
						/* Nur GA selektiert -> kein Crossdocking
						-*/
						sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
					} else {
						sprintf (acStmt2,
								"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
								acStmt1);
					}
					strcat (acStmt, acStmt2);
				}

				strcat (acStmt,
						" GROUP BY VPLP.ARTBEZ, "
						"		   VPLP.MID_AID_ARTNR "
						" ORDER BY VPLP.ARTBEZ, "
						"		   VPLP.MID_AID_ARTNR "); 

				LogPrintf(pcFac,LT_ALERT,
					"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
					ptTouren->tourTourId.Tour,
					acStmt);

				iRvDbThm = TExecSqlX (pvTid, NULL, 
											acStmt,
											BLOCKSIZE, 0,
											SELSTR (acArtBez[0], ARTBEZ_LEN+1),
											SELSTR (acArtNr[0], ARTNR_LEN+1),
											SELLONG	(lThmAnz[0]),
											SQLSTRING (ptTouren->tourTourId.Mand),
											SQLSTRING (ptTouren->tourTourId.Tour),
											SQLLONG   (ptTouren->tourTourId.PosNr),	
											NULL);	

				if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
					handle = elDestroyList (handle);
					return (-1);
				}

				if (iRvDbThm > 0 &&
					iKundePerSeite == JANEIN_J)  {

					/* --- get tour data --- */
					tLiVerl.tTour = *ptTouren;

					elgWrite (handle, elNheader, PN_TOUR,
						&tLiVerl, NULL);
					elgWrite (handle, elNbody, PN_TOUR,
						&tLiVerl, NULL);
				}

				for (nI = 0; nI < iRvDbThm; nI++) {
				
					sprintf(tLiVerl.tFooter_Ausk.ArtBez, "%s", acArtBez[nI]);
					sprintf(tLiVerl.tFooter_Ausk.ArtNr, "%s", acArtNr[nI]);
					tLiVerl.tFooter_Ausk.AnzThm = lThmAnz[nI];
						
					elgWrite (handle, elNheader, TN_FOOTER_AUSK, 
																&tLiVerl, NULL);
					elgWrite (handle, elNbody,   TN_FOOTER_AUSK, 
																&tLiVerl, NULL);
				}
			}
		}
	}

    LogPrintf(pcFac,LT_ALERT,"DRUCK BEGINN");
	
	/*************************************
	-* Verladeliste drucken
	-*/	
	if (ptPrnPrn == NULL) {
		ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(pvTid,
								tAusk.auskZiel.FeldId,
								PURPOSE_LISTEN);
		if (ptPrnPrn == NULL) {
			iRv = TExecSql (pvTid,"SELECT LAGID FROM FES "
								  "WHERE FELDID=:feldid ",
								  SELSTR(acLagId,LAGID_LEN+1),
								  SQLSTRING(tAusk.auskZiel.FeldId),
								  NULL);
			if (iRv < 0){
				LogPrintf(pcFac, LT_ALERT,
					"Fehler beim Lesen der Position %s",tAusk.auskZiel.FeldId);
				handle = elDestroyList (handle);
				return -1;
			}
			ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(pvTid,
									acLagId,
									PURPOSE_LISTEN);
			if (ptPrnPrn == NULL) {
			   LogPrintf(FAC_LSM_OUT, LT_ALERT, "%s",
					MlMpack("Kein Drucker fuer diese Position1 %s gefunden",
						 0, "[%s]",
							tAusk.auskZiel.FeldId));

				LogPrintf(pcFac, LT_ALERT,
					"Kein Drucker fuer diese Position1 %s gefunden",
					tAusk.auskZiel.FeldId);
				handle = elDestroyList (handle);
				return -1;
			}
		}
	}

	pcFileName = acFileName;
	pcFileName2 = acFileName2;
	tmpnam(acFileName);
	tmpnam(acFileName2);


	if (iKundePerSeite == JANEIN_J)  {

		if (iNoTE == JANEIN_N && iErwVerlList == JANEIN_J)  {

			lLfdNr = SqlGetSequence (pvTid, SEQ_VERLLISTLFDNR, pcFac);
			if (lLfdNr < 0) {
				LogPrintf (pcFac, LT_ALERT,
						"<PrintVerlListeKst> error getting SEQ_VERLLISTLFDNR");
				return (-1);
			}
		}

		lm2_dump_verl_kst (handle->ehLM2_DESC, 65, NULL,
						   MlMsg ("START Verladeliste\n") , 
				  		   MlMsg ("ENDE Verladeliste\n"),
						   lLfdNr, acBarcode, NULL, &pcFileName);
	} else {
		lm2_dump(handle->ehLM2_DESC, ptPrnPrn->Printer.prnSize.Length,
			MlMsg("Verladeliste"), NULL,NULL, NULL, &pcFileName);
	}

	iRv = VerlListArch (pvTid, pcFileName, &atAusk[0], pcFac);
	if (iRv < 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<PrintVerlListe>: archivate Verlliste failed!");
	}

	if (iNoTE == JANEIN_N && iKundePerSeite == JANEIN_J && iErwVerlList == JANEIN_J)  {

		iRvPrm = PrmGet1Parameter (pvTid, P_ErwVerLstSpool, PRM_CACHE, acSpool);
		if (iRvPrm != PRM_OK || IsEmptyStrg (acSpool) == JANEIN_J) { 
			/* Default value */
			LogPrintf (pcFac, LT_ALERT,
					"<PrintVerlListeKst> error reading parameter ErwVerLstSpool "
					"--> set default value to '/data/erwverllist'");
			strcpy (acSpool, "\\data\\erwverllist");
		}

		pcRootDir = getenv (ENVROOTDIR);
		if (pcRootDir == NULL) {
			LogPrintf (pcFac, LT_ALERT,
					"<PrintVerlListeKst> error getting ROOTDIR");
			return (-1);
		}

		zNow = time ((time_t *)0);

		strftime (acDate, sizeof (acDate), "%d%m%Y", localtime (&zNow));

		if (!(fp = fopen(pcFileName2, "w")))
			 return(-1);

		fprintf (fp,"%s\n%s\n%s\n%s", acBarcode,
									  pcKuNr,
									  acDate,
									  ptTouren->tourTourId.Tour);
		
		fclose (fp);

		memset (acCmd, 0, sizeof (acCmd));

		sprintf(acCmd, "cp %s %s%s", acFileName2, pcRootDir, acSpool);	

		system (acCmd);

		strcpy (acBackslash, BACKSLASH);

		memset (acCmd, 0, sizeof (acCmd));

#ifdef OLD
		sprintf (acCmd, "mv %s%s%s %s%s%s%s%s%s%ld", pcRootDir,
												   	 acSpool,
												   	 acFileName2,
												     pcRootDir,
												     acSpool,
												     acBackslash,
												     pcKuNr,
												     acDate,
												     ptTouren->tourTourId.Tour,
												     lLfdNr); 
#endif /* OLD */

		sprintf (acCmd, "mv %s%s%s %s%s%s%s", pcRootDir,
											  acSpool,
											  acFileName2,
										  	  pcRootDir,
											  acSpool,
											  acBackslash,
											  acBarcode);

		system (acCmd);
	}

	/* Anz Verlliste */
	if (PrmGet1Parameter (pvTid, P_AnzVerlList,
							PRM_CACHE, &lAnzVerlListe) != PRM_OK) {
		/* Default value */
		LogPrintf (pcFac, LT_ALERT,
					"Fehler bei get P_AnzVerlList "
					"set P_AnzVerlList = 1");
		lAnzVerlListe = 1;
	}

	if (lAnzVerlListe == 0) {
		lAnzVerlListe = 1;
	}

    LogPrintf(pcFac,LT_ALERT,"DRUCK ANZAHL DRUCK %ld",lAnzVerlListe);

	for (nJ=1; nJ < lAnzVerlListe; nJ++) {	

		/* print more VerlList  */
		memset(acFileNameMore, 0, sizeof(acFileNameMore));
		sprintf(acFileNameMore , "%s%ld", acFileName, nJ);


		/* Copy Drucker file */
		memset (acCmd, 0, sizeof(acCmd));
		sprintf(acCmd, "cp %s %s", 
						acFileName,
						acFileNameMore);	
		LogPrintf ("MORE", LT_ALERT,
				"File_old: :%s, File_new: %s", acFileName, acFileNameMore);
		system(acCmd);
            LogPrintf(pcFac,LT_ALERT,"DRUCK nJ %ld",nJ);

		if (localprexecPrnPrn(ptPrnPrn, acFileNameMore) != 1) {
			handle = elDestroyList (handle);
			return -1;
		}
	}

	LogPrintf(pcFac,LT_ALERT,"DRUCK wird gemacht mit File: %s",acFileName);

    if (localprexecPrnPrn(ptPrnPrn, acFileName) != 1) {
        handle = elDestroyList (handle);
		LogPrintf(pcFac,LT_ALERT,"DRUCK RETURN -1");
        return -1;
    }

	handle = elDestroyList (handle);

	LogPrintf(pcFac,LT_ALERT,"DRUCK RETURN OK ");

	return (1);
}

int PrintVerlListe( void *pvTid, char *pcFac, AUSK *ptAusk, TOUREN  *ptTouren,
					int  iAufrufKz, long lVerKontKz, long lAutyp, 
					PRIMANDOPRINT *ptPrnPrn, int iKundePerSeite)
{

	int         		nI, iRv,iOk, iFirst, iPrintKunde=0;
    AUSK        		atAusk[BLOCKSIZE], tAusk, tAuskSave;
    LIVERL      		tLiVerl;
	char        		acStmt[2048], acStmt1[1024], acStmt2[1024];
	char				acFileName[L_tmpnam+1];
	char	    		*pcFileName;
	char				acFileNameMore[L_tmpnam+1];
	char				acLagId[LAGID_LEN+1];
	elThandle   		*handle;
	SqlContext  		*hCtx;
	double				dSumGewNet, dGesSumGewNet;
	double				dSumGewBrut, dGesSumGewBrut;
	long				lSumTE, lGesSumTE;
	int					iDruckTour = 0, iFoundNormalAusk=0;
	char				acArtBez[BLOCKSIZE][ARTBEZ_LEN+1];
	char				acArtNr[BLOCKSIZE][ARTNR_LEN+1];
	long				lThmAnz[BLOCKSIZE];
	int         		iRvDbThm = 0, iNoCrossdocking=0;
	int					iRvDb = 0;
	LAST				tLast;
	int					nK;
	long				lAnzVerlListe = 0;
	int					nJ, nE;
	char        		acCmd[512];
	LIB_TITLE 			tTitle;
	int					iPrintNewPage = 0;
	int					iFormFeed = JANEIN_N;


    /* next pages will be feed */
    memset (&tTitle, 0, sizeof (tTitle));
    strcpy (tTitle.acFormFeed, "\f");

	memset (acArtBez, 0, sizeof(acArtBez));
	memset (acArtNr, 0, sizeof(acArtNr));

	hCtx = TSqlNewContext (pvTid, NULL);
	if (hCtx == NULL) {
		return -1;
	}

	/* Druckfile erstellen */

	handle = elCreateList (&el_liverl);
	if ( elOpenList(handle, "lists.ger", "VERLLIST" ) == NULL) {
		handle = elDestroyList (handle);
		(void)TSqlDestroyContext (pvTid, hCtx);
		return ( -1 );
	}

	/********************************************************************
	-* Drucken aller Auslagerauftraege die im aktuellen Standort
	-* kommissionier bzw. ausgelagert wurden
	-*/
	if (iAufrufKz == VERLLIST_AUSUEB) {
		if (ptAusk == NULL) {
            LogPrintf(pcFac,LT_ALERT,
                "PrintVerlListe: VERLLIST_AUSUEB aber ptAusk=NULL");
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
		}
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT %%AUSK FROM AUSK "
				"WHERE AUSK.AusId_Mand  = :a  "
				"  AND AUSK.AusId_AusNr = :b  "
				"  AND AUSK.AusId_AusKz = :c  "
				"  AND AUSK.STATUS <> 'FERTIG' "
				"ORDER BY AUSK.KuNr,"
				"		  AUSK.KuPosNr, "
				"		  AUSK.AUSID_MAND,"
				"         AUSK.AUSID_AUSNR,"
				"         AUSK.AUSID_AUSKZ");
	} else {
		if (ptTouren == NULL) {
            LogPrintf(pcFac,LT_ALERT,
                "PrintVerlListe: VERLLIST_TOURUEB aber ptTouren=NULL");
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
		}
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT %%AUSK FROM AUSK "
				"WHERE AUSK.TOURID_MAND  = :a  "
				"  AND AUSK.TOURID_TOUR  = :b  "
				"  AND AUSK.TOURID_POSNR = :c  "
				"  AND AUSK.STATUS <> 'FERTIG' "
				"  AND EXISTS (SELECT 1 FROM VPLP WHERE "
							" VPLP.AUSID_AUSNR = AUSK.AUSID_AUSNR AND "
							" VPLP.AUSID_AUSKZ = AUSK.AUSID_AUSKZ AND "
							" VPLP.ISTMNGS_MNG != 0) ");

		if (lAutyp != 0) {
			memset (acStmt1,0,sizeof(acStmt1));
			memset (acStmt2,0,sizeof(acStmt2));
			GetAutypStmt(lAutyp, acStmt1);

			if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur TE selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
						"AND %s "
						"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

			} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   ((lAutyp & TOURAUTYP_TE) == 0) &&
			   (lAutyp & TOURAUTYP_GA) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur GA selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
						"AND %s "
						"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
			} else {
				sprintf (acStmt2,
						"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", acStmt1);
			}
			strcat (acStmt, acStmt2);
		}

		strcat (acStmt,
				"ORDER BY AUSK.LADERF DESC, "
				"		  AUSK.KuNr, AUSK.KuPosNr, "
				"		  AUSK.AUSID_AUSKZ DESC, "
				"		  AUSK.STANDORT, "
				"		  AUSK.AUSID_MAND,"
				"         AUSK.AUSID_AUSNR");

		LogPrintf(pcFac,LT_ALERT,
			"PrintVerlListe: Tour:%s: -> stmt:%s",
			ptTouren->tourTourId.Tour,
			acStmt);
	}


	iRv = 0;
	iFirst = 1;
	dGesSumGewNet = 0;
	dGesSumGewBrut = 0;
	lGesSumTE = 0;
	memset(&tAuskSave, 0, sizeof(tAuskSave));

	do {
		memset (&atAusk[0], 0, sizeof (atAusk));

		if (iAufrufKz == VERLLIST_AUSUEB) {
			iRv = iRv == 0 ?
				TExecSqlX (pvTid, hCtx,
					acStmt,
					BLOCKSIZE, 0,
					SELSTRUCT (TN_AUSK, atAusk[0]),
					SQLSTRING (ptAusk->auskAusId.Mand),
					SQLSTRING (ptAusk->auskAusId.AusNr),
					SQLAUSKZ (ptAusk->auskAusId.AusKz),
					NULL) :
				TExecSqlV (pvTid, hCtx, NULL, NULL, NULL, NULL);

			if (iFirst == 1 && iRv < 0 && TSqlError(pvTid) == SqlNotFound) {
				LogPrintf(pcFac,LT_ALERT,
					"Auftrag nicht gefunden : %s %s %ld ",
					ptAusk->auskAusId.Mand,
					ptAusk->auskAusId.AusNr,
					l2sGetNameByValue(&l2s_AUSKZ, ptAusk->auskAusId.AusKz));
				(void)TSqlDestroyContext (pvTid, hCtx);
				handle = elDestroyList (handle);
				return (-1);
			}
		} else {
			iRv = iRv == 0 ?
				TExecSqlX (pvTid, hCtx,
					acStmt,
					BLOCKSIZE, 0,
					SELSTRUCT (TN_AUSK, atAusk[0]),
					SQLSTRING (ptTouren->tourTourId.Mand),
					SQLSTRING (ptTouren->tourTourId.Tour),
					SQLLONG (ptTouren->tourTourId.PosNr),
					NULL) :
				TExecSqlV (pvTid, hCtx, NULL, NULL, NULL, NULL);

			if (iFirst == 1 && iRv < 0 && TSqlError(pvTid) == SqlNotFound) {
				LogPrintf(pcFac,LT_ALERT,
					"Keine Auftraege für Tour: %s/%s/%ld ",
					ptTouren->tourTourId.Mand,
					ptTouren->tourTourId.Tour,
					ptTouren->tourTourId.PosNr);
				break;
			}
		}

        if (iRv < 0 && TSqlError(pvTid) != SqlNotFound) {
            LogPrintf(pcFac,LT_ALERT,
                "Ein Fehler beim Lesen der Aufträge "
                    "ist aufgetreten!\n TSqlErr: %s",
                TSqlErrTxt(pvTid));
			(void)TSqlDestroyContext (pvTid, hCtx);
			handle = elDestroyList (handle);
            return (-1);
        }

        if (iRv > 0) {
        	iFoundNormalAusk = 1;
        }


		iFirst = 0;

		for( nI=0; nI<iRv; nI++) {

			/* Kundennummer, right Addresse */

			if (atAusk[nI].auskKuPosNr > 0) {
				/* Set Right Adresse */

				memset (&tLast, 0, sizeof(tLast));
				
				strcpy(tLast.lastKuNr, atAusk[nI].auskKuNr);
				tLast.lastKuPosNr = atAusk[nI].auskKuPosNr;
				strcpy(tLast.lastMand, atAusk[nI].auskAusId.Mand);
			
				iRvDb = TExecStdSql (pvTid, StdNselect, TN_LAST, &tLast);

				if (iRvDb <= 0 && TSqlError(pvTid) != SqlNotFound) {
					/* Db - Error */
					 LogPrintf(pcFac,LT_ALERT,
                		"Ein Fehler beim Lesen der Aufträge "
                    	"ist aufgetreten!\n TSqlErr: %s",
                		TSqlErrTxt(pvTid));
						(void)TSqlDestroyContext (pvTid, hCtx);
						handle = elDestroyList (handle);
            			return (-1);
				}		

				for(nK = 0; nK < (int) strlen(atAusk[nI].auskKuName); nK++) {

					if (atAusk[nI].auskKuName[nK] == ',') {
						/* New Ku Ort */
						strncpy(&(atAusk[nI].auskKuName[nK+2]), 
											tLast.lastAdrs.Ort, 
											(KUNAME_LEN - (nK+2)));

						atAusk[nI].auskKuName[KUNAME_LEN] = '\0';
						break;

					}
				}
			}

			memset(&tLiVerl, 0, sizeof(LIVERL));
			memset(&tAusk, 0,           sizeof(AUSK));
			memcpy(&tAusk, &atAusk[nI], sizeof(AUSK));
			dSumGewNet = 0;
			dSumGewBrut = 0;
			lSumTE = 0;

			/* Verladelisteeintrag fuer die TE eines
			-* Auftrages machen
			-*/

			LogPrintf(pcFac,LT_ALERT,
					"PrintSAVE ausid %s kunr: %s posnr %d crossdocking %d", 
					tAuskSave.auskAusId.AusNr,
					tAuskSave.auskKuNr,
					tAuskSave.auskKuPosNr,
					tAuskSave.auskCrossDocking); 

			LogPrintf(pcFac,LT_ALERT,
					"Printakt ausid %s kunr: %s posnr %d crossdocking %d", 
					atAusk[nI].auskAusId.AusNr,
					atAusk[nI].auskKuNr,
					tAuskSave.auskKuPosNr,
					atAusk[nI].auskCrossDocking); 


			if ((tAuskSave.auskStandort != atAusk[nI].auskStandort) ||
				(strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
				(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {

				if ((strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
					(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {
					iPrintKunde = 1;
					 LogPrintf(pcFac,LT_ALERT,"PrintKunde 1");
				} else {
					iPrintKunde = 0;
					 LogPrintf(pcFac,LT_ALERT,"PrintKunde 0");
				}

				if (iAufrufKz != VERLLIST_AUSUEB &&
					iPrintKunde == 1 &&
					iPrintNewPage == 1) {

					memset (&tLiVerl, 0, sizeof (tLiVerl));
					memset (acStmt,0,sizeof(acStmt));
					sprintf (acStmt,
							"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
								" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
								" VPLK WHERE "
								" 	  VPLK.KSNR = VPLP.KSNR "
								" AND VPLP.BUCHMNGS_MNG > 0 " 
								" AND VPLK.TourId_Mand  = :Mand  "
								" AND VPLK.TourId_Tour  = :Tour  "
								" AND VPLK.TourId_PosNr = :PosNr "
								" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
								" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
								" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
								" AND VPLP.ThmKz = 1 "
								" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
								" AND AUSK.KUNR = :KuNr "
								" AND AUSK.KUPOSNR = :KuPosNr ");

					if (lAutyp != 0) {
						memset (acStmt1,0,sizeof(acStmt1));
						memset (acStmt2,0,sizeof(acStmt2));
						GetAutypStmt(lAutyp, acStmt1);

						if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   (lAutyp & TOURAUTYP_TE) &&
						   ((lAutyp & TOURAUTYP_GA) == 0) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
							/* Nur TE selektiert -> kein Crossdocking
							-*/
							sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

						} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   ((lAutyp & TOURAUTYP_TE) == 0) &&
						   (lAutyp & TOURAUTYP_GA) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
							/* Nur GA selektiert -> kein Crossdocking
							-*/
							sprintf (acStmt2,
								"AND %s "
								"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
						} else {
							sprintf (acStmt2,
								"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
								acStmt1);
						}
						strcat (acStmt, acStmt2);
					}

					strcat (acStmt,
							" GROUP BY VPLP.ARTBEZ, "
							"		   VPLP.MID_AID_ARTNR "
							" ORDER BY VPLP.ARTBEZ, "
							"		   VPLP.MID_AID_ARTNR "); 

					LogPrintf(pcFac,LT_ALERT,
						"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
						ptTouren->tourTourId.Tour,
						acStmt);

					iRvDbThm = TExecSqlX (pvTid, NULL, 
										acStmt,
										BLOCKSIZE, 0,
										SELSTR (acArtBez[0], ARTBEZ_LEN+1),
										SELSTR (acArtNr[0], ARTNR_LEN+1),
										SELLONG	(lThmAnz[0]),
										SQLSTRING (ptTouren->tourTourId.Mand),
										SQLSTRING (ptTouren->tourTourId.Tour),
										SQLLONG   (ptTouren->tourTourId.PosNr),	
										SQLSTRING (tAuskSave.auskKuNr),
										SQLLONG   (tAuskSave.auskKuPosNr),
										NULL);	

					if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
						(void)TSqlDestroyContext (pvTid, hCtx);
						handle = elDestroyList (handle);
						return (-1);
					}
					for (nE = 0; nE < iRvDbThm; nE++) {
						sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
													"%s", acArtBez[nE]);
						sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
													"%s", acArtNr[nE]);
						tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
							
						elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
															&tLiVerl, NULL);
						elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
															&tLiVerl, NULL);
					}

					if (iRvDbThm > 0 && 
						iKundePerSeite == JANEIN_J) {

						iFormFeed = JANEIN_J;
						iDruckTour = 0;
					}
				}

				if ((strcmp (tAuskSave.auskKuNr, atAusk[nI].auskKuNr) != 0) ||
					(tAuskSave.auskKuPosNr != atAusk[nI].auskKuPosNr) ) {
					if (iFormFeed == JANEIN_J &&
						iKundePerSeite == JANEIN_J) {
						if (elgWrite (handle, elNbody, PN_FORMFEED,
									(void *)&tTitle, NULL) < 0) {
							handle = elDestroyList (handle);
							(void)TSqlDestroyContext (pvTid, hCtx);
							return (-1);
						}

						iFormFeed = JANEIN_N;
					}
				}	


				if (atAusk[nI].auskAusId.AusKz == AUSKZ_INT) {
					/* Crossdocking-Auslagerauftrag */
                    memcpy(&tAuskSave, &atAusk[nI], sizeof(AUSK));
                    if (li_verlliste_kunr_cross ( pvTid,
                                                &atAusk[nI],
                                                &dSumGewNet,
                                                &dSumGewBrut,
                                                &lSumTE,
                                                handle,
                                                NULL,
                                                iAufrufKz,
                                                &iDruckTour,
                                                lVerKontKz,
                                                lAutyp,
                                                nI,
                                                iPrintKunde) < 0) {
                        handle = elDestroyList (handle);
                        (void)TSqlDestroyContext (pvTid, hCtx);
                        return -1;
                    } else {
                        iOk = 1;
                        dGesSumGewNet  = dGesSumGewNet + dSumGewNet;
                        dGesSumGewBrut = dGesSumGewBrut + dSumGewBrut;
                        lGesSumTE      = lGesSumTE + lSumTE;
                    }
					iPrintNewPage = 1;
				} else {
					/* Auslagerauftrag vom aktuellen Standort 
					-*/
					memcpy(&tAuskSave, &atAusk[nI], sizeof(AUSK));
					if (li_verlliste_kunr ( pvTid,
												&atAusk[nI],
												&dSumGewNet,
												&dSumGewBrut,
												&lSumTE,
												handle,
												NULL,
												iAufrufKz, 
												&iDruckTour,
												lVerKontKz,
                                                lAutyp,
												nI,
												iPrintKunde) < 0) {
						handle = elDestroyList (handle);
						(void)TSqlDestroyContext (pvTid, hCtx);
						return -1;
					} else {
						iOk = 1;
						dGesSumGewNet  = dGesSumGewNet + dSumGewNet;
						dGesSumGewBrut = dGesSumGewBrut + dSumGewBrut;
						lGesSumTE      = lGesSumTE + lSumTE;
					}

					if (iAufrufKz != VERLLIST_AUSUEB) {

						memset (&tLiVerl, 0, sizeof (tLiVerl));
						memset (acStmt,0,sizeof(acStmt));
						sprintf (acStmt,
								"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
									" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
									" VPLK WHERE "
									" 	  VPLK.KSNR = VPLP.KSNR "
									" AND VPLP.BUCHMNGS_MNG > 0 " 
									" AND VPLK.TourId_Mand  = :Mand  "
									" AND VPLK.TourId_Tour  = :Tour  "
									" AND VPLK.TourId_PosNr = :PosNr "
									" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
									" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
									" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
									" AND VPLP.ThmKz = 1 "
									" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
									" AND AUSK.KUNR = :KuNr "
									" AND AUSK.KUPOSNR = :KuPosNr ");

						if (lAutyp != 0) {
							memset (acStmt1,0,sizeof(acStmt1));
							memset (acStmt2,0,sizeof(acStmt2));
							GetAutypStmt(lAutyp, acStmt1);

							if (((lAutyp & TOURAUTYP_WA) == 0) &&
							   (lAutyp & TOURAUTYP_TE) &&
							   ((lAutyp & TOURAUTYP_GA) == 0) &&
							   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
								/* Nur TE selektiert -> kein Crossdocking
								-*/
								sprintf (acStmt2,
									"AND %s "
									"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

							} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
							   ((lAutyp & TOURAUTYP_TE) == 0) &&
							   (lAutyp & TOURAUTYP_GA) &&
							   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
								/* Nur GA selektiert -> kein Crossdocking
								-*/
								sprintf (acStmt2,
									"AND %s "
									"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
							} else {
								sprintf (acStmt2,
									"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
									acStmt1);
							}
							strcat (acStmt, acStmt2);
						}

						strcat (acStmt,
								" GROUP BY VPLP.ARTBEZ, "
								"		   VPLP.MID_AID_ARTNR "
								" ORDER BY VPLP.ARTBEZ, "
								"		   VPLP.MID_AID_ARTNR "); 

						LogPrintf(pcFac,LT_ALERT,
							"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
							ptTouren->tourTourId.Tour,
							acStmt);

						iRvDbThm = TExecSqlX (pvTid, NULL, 
											acStmt,
											BLOCKSIZE, 0,
											SELSTR (acArtBez[0], ARTBEZ_LEN+1),
											SELSTR (acArtNr[0], ARTNR_LEN+1),
											SELLONG	(lThmAnz[0]),
											SQLSTRING (ptTouren->tourTourId.Mand),
											SQLSTRING (ptTouren->tourTourId.Tour),
											SQLLONG   (ptTouren->tourTourId.PosNr),	
											SQLSTRING (atAusk[nI].auskKuNr),
											SQLLONG   (atAusk[nI].auskKuPosNr),
											NULL);	

						if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
							(void)TSqlDestroyContext (pvTid, hCtx);
							handle = elDestroyList (handle);
							return (-1);
						}

						for (nE = 0; nE < iRvDbThm; nE++) {
						
							sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
														"%s", acArtBez[nE]);
							sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
														"%s", acArtNr[nE]);
							tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
								
							elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
																&tLiVerl, NULL);
							elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
																&tLiVerl, NULL);
						}

						if (iRvDbThm > 0 && 
							iKundePerSeite == JANEIN_J) {


							/*
						 	 * write FORMFEED entry to the list
						 	 */
							//if (elgWrite (handle, elNbody, PN_FORMFEED,
							//			(void *)&tTitle, NULL) < 0) {
							//	handle = elDestroyList (handle);
							//	(void)TSqlDestroyContext (pvTid, hCtx);
							//	return (-1);
							//}
							iFormFeed = JANEIN_J;
							iDruckTour = 0;
						}
						iPrintNewPage = 0;
					}
				}
			}
		}
		

	} while (iRv == BLOCKSIZE);

	(void)TSqlDestroyContext (pvTid, hCtx);

	if (iAufrufKz != VERLLIST_AUSUEB &&
		iKundePerSeite == 1 &&
		iPrintNewPage == 1) {

		memset (&tLiVerl, 0, sizeof (tLiVerl));
		memset (acStmt,0,sizeof(acStmt));
		sprintf (acStmt,
				"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
					" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
					" VPLK WHERE "
					" 	  VPLK.KSNR = VPLP.KSNR "
					" AND VPLP.BUCHMNGS_MNG > 0 " 
					" AND VPLK.TourId_Mand  = :Mand  "
					" AND VPLK.TourId_Tour  = :Tour  "
					" AND VPLK.TourId_PosNr = :PosNr "
					" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
					" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
					" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
					" AND VPLP.ThmKz = 1 "
					" AND AUSK.KUNR = :KuNr "
					" AND AUSK.KUPOSNR = :KuPosNr ");

		if (lAutyp != 0) {
			memset (acStmt1,0,sizeof(acStmt1));
			memset (acStmt2,0,sizeof(acStmt2));
			GetAutypStmt(lAutyp, acStmt1);

			if (((lAutyp & TOURAUTYP_WA) == 0) &&
			   (lAutyp & TOURAUTYP_TE) &&
			   ((lAutyp & TOURAUTYP_GA) == 0) &&
			   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
				/* Nur TE selektiert -> kein Crossdocking
				-*/
				sprintf (acStmt2,
				"AND %s "
				"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);

				} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
						   ((lAutyp & TOURAUTYP_TE) == 0) &&
						   (lAutyp & TOURAUTYP_GA) &&
						   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
					/* Nur GA selektiert -> kein Crossdocking
					-*/
					sprintf (acStmt2,
					"AND %s "
					"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
				} else {
					sprintf (acStmt2,
					"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
					acStmt1);
				}
			strcat (acStmt, acStmt2);
		}

		strcat (acStmt,
				" GROUP BY VPLP.ARTBEZ, "
				"		   VPLP.MID_AID_ARTNR "
				" ORDER BY VPLP.ARTBEZ, "
				"		   VPLP.MID_AID_ARTNR "); 

		LogPrintf(pcFac,LT_ALERT,
			"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
			ptTouren->tourTourId.Tour,
			acStmt);

		iRvDbThm = TExecSqlX (pvTid, NULL, 
							acStmt,
							BLOCKSIZE, 0,
							SELSTR (acArtBez[0], ARTBEZ_LEN+1),
							SELSTR (acArtNr[0], ARTNR_LEN+1),
							SELLONG	(lThmAnz[0]),
							SQLSTRING (ptTouren->tourTourId.Mand),
							SQLSTRING (ptTouren->tourTourId.Tour),
							SQLLONG   (ptTouren->tourTourId.PosNr),	
							SQLSTRING (tAuskSave.auskKuNr),
							SQLLONG   (tAuskSave.auskKuPosNr),
							NULL);	

		if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
			handle = elDestroyList (handle);
			return (-1);
		}

		for (nE = 0; nE < iRvDbThm; nE++) {
		
			sprintf(tLiVerl.tFooter_KuNr.ArtBez, 
										"%s", acArtBez[nE]);
			sprintf(tLiVerl.tFooter_KuNr.ArtNr, 
										"%s", acArtNr[nE]);
			tLiVerl.tFooter_KuNr.AnzThm = lThmAnz[nE];
				
			elgWrite (handle, elNheader, TN_FOOTER_KUNR, 
												&tLiVerl, NULL);
			elgWrite (handle, elNbody,   TN_FOOTER_KUNR, 
												&tLiVerl, NULL);
		}

		if (iRvDbThm > 0 && 
			iKundePerSeite == JANEIN_J) {

			/*
			 * write FORMFEED entry to the list
			 */
			if (elgWrite (handle, elNbody, PN_FORMFEED,
						(void *)&tTitle, NULL) < 0) {
				handle = elDestroyList (handle);
				return (-1);
			}

			iDruckTour = 0;
		}
	} else if (iFormFeed == JANEIN_J) {
		if (elgWrite (handle, elNbody, PN_FORMFEED,
					(void *)&tTitle, NULL) < 0) {
			handle = elDestroyList (handle);
			return (-1);
		}

		iFormFeed = JANEIN_N;
	}	

	/****************************************************************
	-* Summenzeile ueber alles (normal + Crosdocking) andrucken
	-*/

	if (lGesSumTE == 0) {
		LogPrintf(pcFac, LT_ALERT,
			"PrintVerlListe: Keine TE fuer Verladeliste");
		handle = elDestroyList (handle);
		return 0;
	} 
	if (iOk != 1) {
		LogPrintf(pcFac, LT_ALERT,
			"Es kann keine Verladeliste gedruckt werden");
		handle = elDestroyList (handle);
		return -1;
	} else {
		/*
		tLiVerl.tFooter.SumGewNet  = dSumGewNet;
		tLiVerl.tFooter.SumGewBrut = dSumGewBrut;
		tLiVerl.tFooter.SumTE      = lSumTE;
		elgWrite (handle,
				  elNheader, PN_FOOTER, &tLiVerl, NULL);
		elgWrite (handle,
				  elNbody, PN_FOOTER, &tLiVerl, NULL);
		*/

		memset(&tLiVerl, 0, sizeof(tLiVerl));
		tLiVerl.tFooter.SumGewNet  = dGesSumGewNet;
		tLiVerl.tFooter.SumGewBrut = dGesSumGewBrut;
		tLiVerl.tFooter.SumTE      = lGesSumTE;

		elgWrite (handle,
				  elNheader, PN_FOOTER, &tLiVerl, NULL);
		elgWrite (handle,
				  elNbody, PN_FOOTER, &tLiVerl, NULL);
	
		if (iAufrufKz != VERLLIST_AUSUEB) {	

			memset (&tLiVerl, 0, sizeof (tLiVerl));
			memset (acStmt,0,sizeof(acStmt));
			sprintf (acStmt,
					"SELECT VPLP.ARTBEZ, VPLP.MID_AID_ARTNR, "
						" COUNT(VPLP.ISTMNGS_MNG) FROM AUSK, VPLP, "
						" VPLK WHERE "
						" 	  VPLK.KSNR = VPLP.KSNR "
						" AND VPLP.BUCHMNGS_MNG > 0 " 
						" AND VPLK.TourId_Mand  = :Mand  "
						" AND VPLK.TourId_Tour  = :Tour  "
						" AND VPLK.TourId_PosNr = :PosNr "
						" AND AUSK.AusId_Mand  = VPLP.AusId_Mand  "
						" AND AUSK.AusId_AusNr = VPLP.AusId_AusNr  "
						" AND AUSK.AusId_AusKz = VPLP.AusId_AusKz  "
						" AND LTRIM(VPLP.ZielTeId) IS NOT NULL "
						" AND VPLP.ThmKz = 1");

			if (lAutyp != 0) {
				memset (acStmt1,0,sizeof(acStmt1));
				memset (acStmt2,0,sizeof(acStmt2));
				GetAutypStmt(lAutyp, acStmt1);

				if (((lAutyp & TOURAUTYP_WA) == 0) &&
				   (lAutyp & TOURAUTYP_TE) &&
				   ((lAutyp & TOURAUTYP_GA) == 0) &&
				   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
					/* Nur TE selektiert -> kein Crossdocking
					-*/
					sprintf (acStmt2,
							"AND %s "
							"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
				} else if (((lAutyp & TOURAUTYP_WA) == 0) &&
				   ((lAutyp & TOURAUTYP_TE) == 0) &&
				   (lAutyp & TOURAUTYP_GA) &&
				   ((lAutyp & TOURAUTYP_ANDERE) == 0)) {
					/* Nur GA selektiert -> kein Crossdocking
					-*/
					sprintf (acStmt2,
							"AND %s "
							"AND AUSK.AusId_AusKz != 'INT'   ", acStmt1);
				} else {
					sprintf (acStmt2,
							"AND ((%s) OR (AUSK.AusId_AusKz = 'INT')) ", 
							acStmt1);
				}
				strcat (acStmt, acStmt2);
			}

			strcat (acStmt,
					" GROUP BY VPLP.ARTBEZ, "
					"		   VPLP.MID_AID_ARTNR "
					" ORDER BY VPLP.ARTBEZ, "
					"		   VPLP.MID_AID_ARTNR "); 

			LogPrintf(pcFac,LT_ALERT,
				"PrintVerlListe: Tour:%s: -> THM-stmt:%s",
				ptTouren->tourTourId.Tour,
				acStmt);

			iRvDbThm = TExecSqlX (pvTid, NULL, 
										acStmt,
										BLOCKSIZE, 0,
										SELSTR (acArtBez[0], ARTBEZ_LEN+1),
										SELSTR (acArtNr[0], ARTNR_LEN+1),
										SELLONG	(lThmAnz[0]),
										SQLSTRING (ptTouren->tourTourId.Mand),
										SQLSTRING (ptTouren->tourTourId.Tour),
										SQLLONG   (ptTouren->tourTourId.PosNr),	
										NULL);	

			if (iRvDbThm <= 0 && TSqlError (pvTid) != SqlNotFound) {
				handle = elDestroyList (handle);
				return (-1);
			}

			if (iRvDbThm > 0 &&
    			iKundePerSeite == JANEIN_J)  {

				/* --- get tour data --- */
				tLiVerl.tTour = *ptTouren;

				elgWrite (handle, elNheader, PN_TOUR,
					&tLiVerl, NULL);
				elgWrite (handle, elNbody, PN_TOUR,
					&tLiVerl, NULL);
			}

			for (nI = 0; nI < iRvDbThm; nI++) {
			
				sprintf(tLiVerl.tFooter_Ausk.ArtBez, "%s", acArtBez[nI]);
				sprintf(tLiVerl.tFooter_Ausk.ArtNr, "%s", acArtNr[nI]);
				tLiVerl.tFooter_Ausk.AnzThm = lThmAnz[nI];
					
				elgWrite (handle, elNheader, TN_FOOTER_AUSK, 
															&tLiVerl, NULL);
				elgWrite (handle, elNbody,   TN_FOOTER_AUSK, 
															&tLiVerl, NULL);
			}
		}
	}

            LogPrintf(pcFac,LT_ALERT,"DRUCK BEGINN");
	
	/*************************************
	-* Verladeliste drucken
	-*/	
	if (ptPrnPrn == NULL) {
		ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(pvTid,
								tAusk.auskZiel.FeldId,
								PURPOSE_LISTEN);
		if (ptPrnPrn == NULL) {
			iRv = TExecSql (pvTid,"SELECT LAGID FROM FES "
								  "WHERE FELDID=:feldid ",
								  SELSTR(acLagId,LAGID_LEN+1),
								  SQLSTRING(tAusk.auskZiel.FeldId),
								  NULL);
			if (iRv < 0){
				LogPrintf(pcFac, LT_ALERT,
					"Fehler beim Lesen der Position %s",tAusk.auskZiel.FeldId);
				handle = elDestroyList (handle);
				return -1;
			}
			ptPrnPrn = (PRIMANDOPRINT *)local_Prn_GetPrn4Sto(pvTid,
									acLagId,
									PURPOSE_LISTEN);
			if (ptPrnPrn == NULL) {
			   LogPrintf(FAC_LSM_OUT, LT_ALERT, "%s",
					MlMpack("Kein Drucker fuer diese Position1 %s gefunden",
						 0, "[%s]",
							tAusk.auskZiel.FeldId));

				LogPrintf(pcFac, LT_ALERT,
					"Kein Drucker fuer diese Position1 %s gefunden",
					tAusk.auskZiel.FeldId);
				handle = elDestroyList (handle);
				return -1;
			}
		}
	}

	pcFileName = acFileName;
	tmpnam(acFileName);

	if (iKundePerSeite == JANEIN_J)  {
		lm2_dump_verl(handle->ehLM2_DESC, 0,
			NULL, MlMsg("START Verladeliste\n") , 
				  MlMsg("ENDE Verladeliste\n"), NULL, &pcFileName);
	} else {
		lm2_dump(handle->ehLM2_DESC, ptPrnPrn->Printer.prnSize.Length,
			MlMsg("Verladeliste"), NULL,NULL, NULL, &pcFileName);
	}

	iRv = VerlListArch (pvTid, pcFileName, &atAusk[0], pcFac);
	if (iRv < 0) {
		LogPrintf (pcFac, LT_ALERT,
			"<PrintVerlListe>: archivate Verlliste failed!");
	}	


	/* Anz Verlliste */
	if (PrmGet1Parameter (pvTid, P_AnzVerlList,
							PRM_CACHE, &lAnzVerlListe) != PRM_OK) {
		/* Default value */
		LogPrintf (pcFac, LT_ALERT,
					"Fehler bei get P_AnzVerlList "
					"set P_AnzVerlList = 1");
		lAnzVerlListe = 1;
	}

	if (lAnzVerlListe == 0) {
		lAnzVerlListe = 1;
	}
            LogPrintf(pcFac,LT_ALERT,"DRUCK ANZAHL DRUCK %ld",lAnzVerlListe);

	for (nJ=1; nJ < lAnzVerlListe; nJ++) {	

		/* print more VerlList  */
		memset(acFileNameMore, 0, sizeof(acFileNameMore));
		sprintf(acFileNameMore , "%s%ld", acFileName, nJ);


		/* Copy Drucker file */
#if 0		
		memset(acCmd, 0, sizeof(acCmd));
		sprintf(acCmd, "cp %s %s", 
						acFileName,
						acFileNameMore);	
#else
		std::string file_content;
		if( !XML::read_file( acFileName, file_content ) ) {
				 LogPrintf(pcFac,LT_ALERT,"Datei %s konnte nicht geoeffnet werden", acFileName );
        		 handle = elDestroyList (handle);
				 return -1;
		} else {		 
			std::ofstream file_copy( acFileNameMore, std::ios_base::trunc );

			if( !file_copy ) {
				LogPrintf(pcFac,LT_ALERT,"Datei %s konnte nicht geoeffnet werden", acFileNameMore );
        		handle = elDestroyList (handle);
				return -1;
			} else {
				file_copy << file_content;
				file_copy.close();
			}	
		}
#endif
		LogPrintf ("MORE", LT_ALERT,
				"File_old: :%s, File_new: %s", acFileName, acFileNameMore);
		system(acCmd);
            LogPrintf(pcFac,LT_ALERT,"DRUCK nJ %ld",nJ);

		if (localprexecPrnPrn(ptPrnPrn, acFileNameMore) != 1) {
			handle = elDestroyList (handle);
			return -1;
		}
	}
	LogPrintf(pcFac,LT_ALERT,"DRUCK wird gemacht mit File: %s",acFileName);

    if (localprexecPrnPrn(ptPrnPrn, acFileName) != 1) {
        handle = elDestroyList (handle);
		LogPrintf(pcFac,LT_ALERT,"DRUCK RETURN -1");
        return -1;
    }

	handle = elDestroyList (handle);

	LogPrintf(pcFac,LT_ALERT,"DRUCK RETURN OK ");

	return (1);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Function inserts into table VERKP for all Thm-Vplp of Ausk
-* RETURNS
-*      0 ... ok
-*     -1 ... error
-*--------------------------------------------------------------------------*/
int WriteVerkpForSkippedAusk(void *pvTid, 
				AUSK *ptAusk,
				char *pcFac) {
	
	TEK    			tTek;
	VPLP   			atVplp[BLOCKSIZE];
	SqlContext 		*hCtx = NULL;
	int 			iRv = 0, iRvDb = 0;
	int     		nI;

	memset(&tTek, 0, sizeof(tTek));

	hCtx = TSqlNewContext (pvTid, NULL);
	if (hCtx == NULL) {
		LogPrintf(pcFac,LT_ALERT,
			"Error Creating Context");
		return -1;
	}

	do {
		memset (atVplp,0,sizeof(atVplp));
		iRvDb = iRvDb == 0 ?
			TExecSqlX(pvTid,hCtx,
				" SELECT %VPLP FROM VPLP "
				" WHERE AusId_Mand = :Mand "
				" AND AusId_AusNr =  :AusNr "
				" AND AusId_AusKz =  :AusKz "
				" AND ThmKz = 1 ",
				BLOCKSIZE, 0,
				SELSTRUCT(TN_VPLP, atVplp[0]),
				SQLSTRING(ptAusk->auskAusId.Mand),
				SQLSTRING(ptAusk->auskAusId.AusNr),
				SQLAUSKZ(ptAusk->auskAusId.AusKz),
				NULL):
			TExecSqlV(pvTid,hCtx,NULL,NULL,NULL,NULL);
		
		if (iRvDb <= 0 && TSqlError(pvTid) != SqlNotFound) {
			LogPrintf(pcFac,LT_ALERT,
				"Error Reading VPLP: %s",
				TSqlErrTxt(pvTid));
			iRv = -1;	
			break;
		}

		for(nI = 0; nI<iRvDb; nI++) {
			strcpy(tTek.tekTeId, atVplp[nI].vplpZielTeId);
			
			iRv = WriteVerkp(pvTid,
						&tTek,
						&ptAusk->auskTourId,
						ptAusk,
						GetUserOrTaskName(),
						STATUSVERKP_OK,
						pcFac,
						JANEIN_N,
						atVplp[nI].vplpVerdTeId,
						JANEIN_N,
						JANEIN_J);

			if (iRv < 0) {
			    break;
			}	
		}
			
		if (iRv < 0) {
			break;
		}	

	}while (iRvDb == BLOCKSIZE);
			
	(void)TSqlDestroyContext (pvTid, hCtx);

	return iRv;
}				



/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Function inserts into table VERKP
-* RETURNS
-*      0 ... ok
-*     -1 ... error
-*--------------------------------------------------------------------------*/
int WriteVerkp(void *pvTid, TEK *ptTek, TOURID *ptTourId, 
				AUSK *ptAusk, char *pcPersNr, long lVerkpStatus, char *pcFac,
				JANEIN eFunk,
				char *pcVerdTeId,
				JANEIN eScanVerd,
				JANEIN eSkipped)
{
	VERKP	tVerkp;

	memset(&tVerkp, 0, sizeof(VERKP));

	tVerkp.verkpProtNr = GetProtNr (pvTid, "VERKPNR_SEQ");
    if (tVerkp.verkpProtNr < 0) {
        LogPrintf(pcFac,LT_ALERT,
                "WriteVerkp: GetProtNr -> -1");
        return (-1);
    }
	tVerkp.verkpProtZeit = time(NULL);
	strcpy(tVerkp.verkpTeId, ptTek->tekTeId);
	strcpy(tVerkp.verkpVerkPersNr, pcPersNr);
	tVerkp.verkpTourId = *ptTourId;
	tVerkp.verkpAusId  = ptAusk->auskAusId;
	tVerkp.verkpPos    = ptTek->tekPos;
	tVerkp.verkpStatus = lVerkpStatus;	
	strcpy(tVerkp.verkpKuNr, ptAusk->auskKuNr);
	tVerkp.verkpLiefTerm = ptAusk->auskLiefTerm;
	
	if (eFunk) {
		tVerkp.verkpArt = ARTVERKP_FUNK;
	} else {
		tVerkp.verkpArt = ARTVERKP_VORD;
	}
	/** Verdichtete TE-Id */
	if (pcVerdTeId != NULL) {
	    strcpy(tVerkp.verkpVerdTeId, pcVerdTeId);
	}
	
	if (eScanVerd) {
		tVerkp.verkpScan = SCANVERKP_VERD;
	} else {
		tVerkp.verkpScan = SCANVERKP_NORM;
	}

	if (eSkipped) {
		tVerkp.verkpSkip = SKIPVERKP_SKIP;
	} else {
		tVerkp.verkpSkip = SKIPVERKP_DONE;
	}


	if (TExecStdSql(pvTid, StdNinsert, TN_VERKP, &tVerkp) <= 0) {
        LogPrintf(pcFac,LT_ALERT,
                "WriteVerkp: %s", TSqlErrTxt (pvTid));
        return (-1 );
    }

	return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Check whether all TE have VerKontKz=FERTIG 
-* RETURNS
-*      0 ... Not all TE have VerKontKz=FERTIG
-*      1 ... All TE have VerKontKz=FERTIG
-*     -1 ... error
-*--------------------------------------------------------------------------*/
int VerlkFromAuskFERTIG(void *pvTid, AUSK *ptAusk, char *pcFac)
{
    int     iRv=0;
	long	lCntTENotReady;

    iRv = TExecSql(pvTid,
                    "SELECT "
                    "/*+ ordered */ "
					"COUNT(1) "
                    "FROM VPLP,TEK "
                    "WHERE VPLP.AusId_Mand  = :mand "
                    "  AND VPLP.AusId_AusNr = :ausnr "
                    "  AND VPLP.AusId_AusKz = :auskz "
                    "  AND VPLP.Status = 'FERTIG' "
                    "  AND VPLP.ThmKz = 1 "
                    "  AND TEK.TeId = VPLP.ZielTeId "
                    "  AND TEK.VerKontKz = 'AKTIV' ",
                     SELLONG(lCntTENotReady),
					 SQLSTRING(ptAusk->auskAusId.Mand),
					 SQLSTRING(ptAusk->auskAusId.AusNr),
					 SQLAUSKZ(ptAusk->auskAusId.AusKz),
                     NULL);
    if (iRv < 0) {
        LogPrintf (pcFac, LT_ALERT, "VerlkFromAuskFERTIG: "
                                         " SqlErrTxt:%s",
                                         TSqlErrTxt(pvTid));
        return(-1);
    }
	if (lCntTENotReady > 0) {
		LogPrintf ("TEST", LT_ALERT, "VerlkFromAuskFERTIG: %s", ptAusk->auskAusId.AusNr); 
		return 0;
	} else {
		return 1;
	}
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Function updates TEK.VerKontKz with the status you 
-*      give over (lVerKontKz)
-* RETURNS
-*      0 ... ok
-*     -1 ... error
-*--------------------------------------------------------------------------*/
int ResetVerKontKzForTE(void *pvTid, char *pcTeId, char *pcFac)
{
    int     iRv=0;
	TEK		tTek;
	long	lAnzVplp=0;

	memset(&tTek, 0, sizeof(tTek));

    iRv = TExecSql(pvTid,
                    "SELECT %TEK FROM TEK "
                    "WHERE TEK.VerKontKz != " STR_SHCONTKZ_NEIN
                    " AND TEK.TeId = :teid ",
                    SELSTRUCT(TN_TEK, tTek),
                    SQLSTRING(pcTeId),
                    NULL);
    if (iRv <= 0) {
    	if (TSqlError (pvTid) != SqlNotFound) {
			LogPrintf (pcFac, LT_ALERT, "ResetVerKontKzForTE (TeId:'%s')\n"
                                         " SqlErrTxt:%s",
                                         tTek.tekTeId,
                                         TSqlErrTxt(pvTid));
			return(-1);
		} else {
			return 0;
		}
    }
	
	/* Exists oder Vplp with Mng and this Ziel-TE-Id */

	iRv = TExecSql (pvTid,
					"SELECT COUNT (*) FROM VPLP "
					" WHERE VPLP.ZIELTEID = :TeId "
					" AND VPLP.THMKZ = 0 "
					" AND VPLP.BUCHMNGS_MNG > 0 ",
					SELLONG(lAnzVplp),
					SQLSTRING(pcTeId),
					NULL);

	if ((iRv <= 0) && (TSqlError (pvTid) != SqlNotFound)) {
		LogPrintf (pcFac, LT_ALERT, "ResetVerKontKzForTE (TeId:'%s')\n"
									 " SqlErrTxt:%s",
									 tTek.tekTeId,
									 TSqlErrTxt(pvTid));
		return(-1);
	} else if (lAnzVplp > 0) {
		/* not reset VerKontKz */
		return 0;
	}	

	if (SetTekVerKontKz(pvTid, &tTek, SHCONTKZ_NEIN, pcFac) < 0) {
		return(-1);
	}
	LogPrintf (pcFac, LT_DEBUG, 
			"ResetVerKontKzForTE: Set SHCONTKZ_NEIN for TeId:'%s'",
			 tTek.tekTeId);

    return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Function updates TEK.VerKontKz with the status you 
-*      give over (lVerKontKz)
-* RETURNS
-*      0 ... ok
-*     -1 ... error
-*--------------------------------------------------------------------------*/
int SetTekVerKontKz(void *pvTid, TEK *ptTek, long lVerKontKz, char *pcFac)
{
    int     iRv=0;

    if ( TExecStdSql (  pvTid, StdNselectUpdNo, TN_TEK, ptTek) < 0) {
        LogPrintf(pcFac, LT_ALERT,
            "Lock TEK: %s ", TSqlErrTxt(pvTid));
        return -1;
    }
    iRv = TExecSql(pvTid,
                    "UPDATE TEK "
                    "SET TEK.VerKontKz=:a "
                    "WHERE teid = :b ",
                     SQLSHCONTKZ(lVerKontKz),
                     SQLSTRING(ptTek->tekTeId),
                     NULL);
    if (iRv < 0) {
        LogPrintf (pcFac, LT_ALERT, "SetTekVerKontKz TEK '%s'\n"
                                         " SqlErrTxt:%s",
                                         ptTek->tekTeId,
                                         TSqlErrTxt(pvTid));
        return(-1);
    }

    return 0;
}


/* Funktion mit Userdata als Paramter
-----------------------------------------------------------------------*/
int li_verlliste_direkt (	void 		*pvTid, 
							AUSK 		*ptAusk,
							double		*pdSumGewNet,
							double		*pdSumGewBrut,
							long		*plSumTE,
							elThandle 	*hHandle,
							PRIMANDOPRINT    	*ptPrnPrn,
							int			iAufrufKz,
							long		lVerKontKz)
{

	char		acStmt[1024];
	char		acFileName[L_tmpnam+1];
	char	    *pcFileName, acSaveTeId[TEID_LEN+1], 
                acSaveFeldId[FELDID_LEN+1];
	int			iDbRv, nI, iAnzAus=0, nAnzAus=-1, iFirst;
	ArrDesc		AusDesc;
	TEK			atTek[BLOCKSIZE];
	VPLP		atVplp[BLOCKSIZE];
	LIVERL		atLiVerl[BLOCKSIZE], *ptLiVerl, tLiVerl;
	elThandle   *handle;

	if (ptAusk == NULL) {
		return (-1);
	}
	if (hHandle == NULL) {
		if (ptPrnPrn == NULL) {
			return (-1);
		}
	}

	/* Vorbereitungsarbeiten */

	iDbRv = 0;
	iAnzAus = 0;
	pcFileName = acFileName;

	memset(&AusDesc, 0, sizeof(ArrDesc));

	sprintf (acStmt, 
	   "SELECT %%VPLP, %%TEK "
       " FROM VPLP, TEK "
       "  WHERE VPLP.AusId_Mand =:mand "
       "    AND VPLP.AusId_AusNr   =:ausnr "
       "    AND VPLP.AusId_AusKz   =:auskz "
       "    AND VPLP.Status = 'FERTIG' "
	   "    AND VPLP.IstMngs_Mng != 0 "
       "    AND VPLP.ThmKz != 1"
       "    AND VPLP.ZielTeId!='                  '"
       "    AND TEK.TeId = VPLP.ZielTeId ");

	if (iAufrufKz == VERLLIST_AUSUEB ||
		iAufrufKz == VERLLIST_TOURUEB) {
		MakeFilter(pvTid, acStmt,
			FilterEnum  (TN_TEK, FN_VerKontKz, lVerKontKz),
			FILTER_END);
	}

	strcat(acStmt,
       "  ORDER BY   TEK.Pos_FeldId,"
       "             TEK.EinZeit,"
	   "             VPLP.ZielTeId,"
       "             VPLP.MId_AId_Mand,"
       "             VPLP.MId_AId_ArtNr,"
       "             VPLP.MId_AId_Var,"
       "             VPLP.MId_Charge");

	/* Daten lesen */

	iDbRv = 0;
	do {
		memset(atVplp, 0, sizeof(atVplp));
		memset(atTek,  0, sizeof(atTek));
		memset(atLiVerl, 0, sizeof(atLiVerl));

		iDbRv = TExecSqlX (pvTid, NULL,
						  (iDbRv == 0) ? acStmt : NULL,
						  BLOCKSIZE, 0,
						  SELSTRUCT(TN_VPLP, atVplp[0]),
						  SELSTRUCT(TN_TEK, atTek[0]),
						  SQLSTRING(ptAusk->auskAusId.Mand),
						  SQLSTRING(ptAusk->auskAusId.AusNr),
						  SQLAUSKZ(ptAusk->auskAusId.AusKz),
						  NULL);

		if (iDbRv < 0 && TSqlError (pvTid) != SqlNotFound) {
            if (AusDesc.DataArr) {
                free(AusDesc.DataArr);
                AusDesc.DataArr = NULL;
            }
            return (-1);
        }

        nAnzAus = -1;

		if (iDbRv > 0) {
			for (nI = 0; nI < iDbRv; nI++) {
				nAnzAus++;
				strcpy (atLiVerl[nAnzAus].tZiel.FeldId, 
					    atTek[nAnzAus].tekPos.FeldId);
				atLiVerl[nAnzAus].tAusk = *ptAusk;
				atLiVerl[nAnzAus].tTek  = atTek[nI];
				atLiVerl[nAnzAus].tVplpOne = atVplp[nI];
				atLiVerl[nAnzAus].tVplp    = atVplp[nI];

				*pdSumGewNet += atVplp[nI].vplpIstMngs.Gew;

				CALCCOLLI(&atVplp[nI].vplpSollMngs.Mng, 
						  &atVplp[nI].vplpSollMngs.VeHeFa, 
						  &atLiVerl[nAnzAus].tColli.SollHe, 
						  &atLiVerl[nAnzAus].tColli.SollRest);

				CALCCOLLI(&atVplp[nI].vplpIstMngs.Mng, 
						  &atVplp[nI].vplpSollMngs.VeHeFa, 
						  &atLiVerl[nAnzAus].tColli.IstHe,
						  &atLiVerl[nAnzAus].tColli.IstRest);

				CALCCOLLI(&atVplp[nI].vplpBuchMngs.Mng, 
						  &atVplp[nI].vplpSollMngs.VeHeFa,
						  &atLiVerl[nAnzAus].tColli.BuchHe, 
						  &atLiVerl[nAnzAus].tColli.BuchRest);
				
			}

			if ( nAnzAus >= 0) {
				iAnzAus = AppendEle(&AusDesc, 
									atLiVerl, 
									nAnzAus+1, 
									sizeof(LIVERL));
				if (iAnzAus < 0) {
					if (AusDesc.DataArr) {
						free(AusDesc.DataArr);
						AusDesc.DataArr = NULL;
					}
					return (-1);
				}
			}
		}

	} while (iDbRv == BLOCKSIZE);

	if (iAnzAus <= 0) {
		if (AusDesc.DataArr) {
			free(AusDesc.DataArr);
			AusDesc.DataArr = NULL;
		}
		return 0;
	}

	if (hHandle != NULL) {
        handle = hHandle;
    }

    if (hHandle == NULL) {

		/* Druckfile erstellen */

		handle = elCreateList (&el_liverl);

		if ( elOpenList(handle, "lists.ger", "VERLLIST" ) == NULL) {
			handle =  elDestroyList (handle);

			if (AusDesc.DataArr) {
				free(AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return ( -1 );
		}
	}


	/**********************************************************
	-* Verladeliste schreiben
	-**********************************************************/
	if (iAnzAus > 0) {
		ptLiVerl = (LIVERL *)(void *)AusDesc.DataArr;


		memset(acSaveTeId, 0, sizeof(acSaveTeId));
		memset(acSaveFeldId, 0, sizeof(acSaveFeldId));

		for (nI = 0; nI < iAnzAus; nI++) {

			if (strcmp(ptLiVerl[nI].tTek.tekPos.FeldId,
					   acSaveFeldId) != 0) {
				strcpy(acSaveFeldId, ptLiVerl[nI].tTek.tekPos.FeldId);
				elgWrite (handle, elNheader, PN_AUSK, &ptLiVerl[nI], NULL);
				elgWrite (handle, elNbody,   PN_AUSK, &ptLiVerl[nI], NULL);
			}

			if (nI == 0) {
				elgWrite (handle, elNheader, PN_TEK,  &ptLiVerl[nI], NULL);
				elgWrite (handle, elNheader, PN_VPLP, &ptLiVerl[nI], NULL);
			}
			if (strcmp(ptLiVerl[nI].tTek.tekTeId, acSaveTeId) != 0) {
				if ( ((nI+1) < iAnzAus) && 
				     strcmp(ptLiVerl[nI].tVplpOne.vplpZielTeId,
				            ptLiVerl[nI+1].tVplpOne.vplpZielTeId) == 0) {
					/* TE ist eine Mischpalette-> ArtNr,ArtBez ueberschreiben
					-*/
					memset(&ptLiVerl[nI].tVplpOne.vplpMId, 0, sizeof(MID));
					strcpy(ptLiVerl[nI].tVplpOne.vplpMId.AId.ArtNr, 
							MlMsg("Mischpalette"));
					strcpy(ptLiVerl[nI].tVplpOne.vplpArtBez, 
							MlMsg("Mischpalette"));
					memset(&ptLiVerl[nI].tVplpOne.vplpSollMngs,0,sizeof(MNGS));
					memset(&ptLiVerl[nI].tVplpOne.vplpIstMngs,0,sizeof(MNGS));
					memset(&ptLiVerl[nI].tVplpOne.vplpBuchMngs,0,sizeof(MNGS));
				}
				/* TEK with VerKontKz = NEIN setting to AKTIV
				-*/
				if (iAufrufKz != VERLLIST_LISTE &&
					ptLiVerl[nI].tTek.tekVerKontKz == SHCONTKZ_NEIN) {
					if (SetTekVerKontKz(pvTid,
										&ptLiVerl[nI].tTek, 
										SHCONTKZ_AKTIV,
										"Verlk") < 0) {
						return -1;
					}
				}
				elgWrite (handle, elNbody, PN_TEK, &ptLiVerl[nI], NULL);
				strcpy(acSaveTeId, ptLiVerl[nI].tTek.tekTeId);
				*plSumTE = *plSumTE + 1; 
				*pdSumGewBrut += ptLiVerl[nI].tTek.tekGesGewicht;
			}
			elgWrite (handle, elNbody, PN_VPLP, &ptLiVerl[nI], NULL);
		}


		/* THM-Verplanungen anzeigen */
		if (ptAusk->auskThmErf == JANEIN_J) {

			iDbRv = 0;
			iAnzAus = 0;

			sprintf (acStmt, 
			   "SELECT %%VPLP "
			   " FROM VPLP "
			   "  WHERE VPLP.AusId_Mand =:mand "
			   "    AND VPLP.AusId_AusNr   =:ausnr "
			   "    AND VPLP.AusId_AusKz   =:auskz "
			   "    AND VPLP.Status = 'FERTIG' "
			   "    AND VPLP.IstMngs_Mng != 0 "
			   "    AND VPLP.ThmKz = 1 "
			   " ORDER BY "
			   "        VPLP.ZielTeId,"
			   "        VPLP.MId_AId_Mand,"
			   "        VPLP.MId_AId_ArtNr,"
			   "        VPLP.MId_AId_Var");

			/* Daten lesen */

			iDbRv = 0;
			iFirst = 1;
			do {
				memset(atVplp, 0, sizeof(atVplp));
				memset(atLiVerl, 0, sizeof(atLiVerl));

				iDbRv = TExecSqlX (pvTid, NULL,
								  (iDbRv == 0) ? acStmt : NULL,
								  BLOCKSIZE, 0,
								  SELSTRUCT(TN_VPLP, atVplp[0]),
								  SQLSTRING(ptAusk->auskAusId.Mand),
								  SQLSTRING(ptAusk->auskAusId.AusNr),
								  SQLAUSKZ(ptAusk->auskAusId.AusKz),
								  NULL);

				if (iDbRv < 0 && TSqlError (pvTid) != SqlNotFound) {
					if (AusDesc.DataArr) {
						free(AusDesc.DataArr);
						AusDesc.DataArr = NULL;
					}
					return (-1);
				}

				if (iDbRv > 0) {
					if (iFirst == 1) {
						iFirst = 0;
						elgWrite (handle, elNheader, 
									PN_THMVPLP, &atLiVerl[0], NULL);
					}
					for (nI = 0; nI < iDbRv; nI++) {
						nAnzAus++;
						atLiVerl[0].tThmVplp = atVplp[nI];
						elgWrite (handle, elNbody, 
									PN_THMVPLP, &atLiVerl[0], NULL);
						
					}
				}

			} while (iDbRv == BLOCKSIZE);
		}

	} else {
		memset(&tLiVerl, 0, sizeof(LIVERL));
		memcpy(&tLiVerl.tAusk, ptAusk, sizeof(AUSK));
		elgWrite (handle, elNheader, PN_AUSK, &tLiVerl, NULL);
		elgWrite (handle, elNbody,   PN_AUSK, &tLiVerl, NULL);
	}

	elgWrite (handle, elNheader, TN_FOOTER_AUSK, &tLiVerl, NULL);
	elgWrite (handle, elNbody, 	 TN_FOOTER_AUSK, &tLiVerl, NULL);

	if (hHandle == NULL) {
		tmpnam(acFileName);

		lm2_dump(handle->ehLM2_DESC, ptPrnPrn->Printer.prnSize.Length,
				"Verladeliste", NULL,NULL, NULL, &pcFileName);

		handle = elDestroyList (handle);

		/* Drucken */

		if (localprexecPrnPrn(ptPrnPrn, acFileName) != 1) {
			if (AusDesc.DataArr) {
				free(AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return -1;
		}
	}

	if (AusDesc.DataArr) {
		free(AusDesc.DataArr);
		AusDesc.DataArr = NULL;
	}

    return (1);
}

/***************************************************************************
 * FUNCTION HEADER:														   *
 * 	int GetThmVplp (...);												   *
 * DESCRIPTION:															   *
 * 	get the vplp data of the thm's if the thm's are included			   *
 * RETURN VALUES:														   *
 *	<-1>		error													   *
 *	< 0>		no thm's included										   *
 * 	< 1>		vplp's read												   *
 ***************************************************************************/
int GetThmVplp (void *pvTid, AUSK *ptAusk, elThandle *hListHandle)
{
	int			iDbRvX = 0, nI = 0;
	VPLP		atVplp[BLOCKSIZE];
	
	if (ptAusk->auskThmErf == JANEIN_N) {
		return (0);
	}

	do {
		memset (&atVplp[0], 0, sizeof (atVplp));

		iDbRvX = (nI == 0) ?
			TExecSqlX (pvTid, NULL,
				"SELECT "
					"%VPLP "
				"FROM "
					"VPLP "
				"WHERE "
					"VPLP.Status = 'FERTIG' AND "
					"VPLP.ThmKz  = 1 AND "
					"VPLP.IstMngs_Mng != 0 AND "
					"VPLP.Status = 'FERTIG' AND "
					"VPLP.AusId_Mand = :Mand AND "
					"VPLP.AusId_AusNr = :AusNr AND "
					"VPLP.AusId_AusKz = :AusKz "
				"ORDER BY "
					"VPLP.ZielTeId, "
					"VPLP.MId_AId_Mand, "
					"VPLP.MId_AId_ArtNr, "
					"VPLP.MId_AId_Var",
				BLOCKSIZE, 0,
				SELSTRUCT (TN_VPLP, atVplp[0]),
				SQLSTRING (ptAusk->auskAusId.Mand),
				SQLSTRING (ptAusk->auskAusId.AusNr),
				SQLAUSKZ  (ptAusk->auskAusId.AusKz),
				NULL) :
			TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError (pvTid) != SqlNotFound) {
			return (-1);
		}
		
		if (iDbRvX <= 0) {
			return (0);
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			if (nI == 0) {
				elgWrite (hListHandle, elNheader, PN_THMVPLP, 
					(void *)&atVplp[nI], NULL);
			}
			elgWrite (hListHandle, elNbody, PN_THMVPLP, (void *)&atVplp[nI],
				NULL);
		}
	} while (iDbRvX == BLOCKSIZE);

	return (1);
}

/***************************************************************************
 * FUNCTION HEADER:														   *
 * 	int GetKstData (...);												   *
 * DESCRIPTION:															   *
 * 	get the kst data if it is the standard address				  		   *
 * RETURN VALUES:														   *
 *	<-1>		error													   *
 *	< 0>		not the std address										   *
 * 	< 1>		address read from db									   *
 ***************************************************************************/
int GetKstData (void *pvTid, AUSK *ptAusk, KST *ptKst)
{
	int			iDbRv = 0;
	KST			tKst;

	/* --- a ausk have to be transfered --- */
	if (ptAusk == NULL) {
		return (-1);
	}	
	
	/* --- if it isn't the std address the address 
		   of ausk should be used --- */
	if (ptAusk->auskKuPosNr != 0) {
		return (0);
	}
	
	memset (&tKst, 0, sizeof (tKst));

	strncpy (tKst.kstKuNr, ptAusk->auskKuNr, KUNR_LEN);
	tKst.kstKuNr[KUNR_LEN] = '\0';
	
	/* --- read the std address from the kst --- */
	iDbRv = TExecStdSql (pvTid, StdNselect, TN_KST, &tKst);

	/* --- the kst have to exist - every error is a fatal one --- */
	if (iDbRv <= 0) {
		return (-1);
	}

	/* --- save read data --- */
	*ptKst = tKst;

	return (1);
}

/***************************************************************************
 * FUNCTION HEADER:														   *
 *	int li_verlliste_kunr (...);										   *
 * DESCRIPTION:															   *
 *	prints a list per tour and customer with all te's in the orders		   *
 * RETURN VALUES:														   *
 *	<-1>		error													   *
 *	< 0>		ok														   *
 ***************************************************************************/
int li_verlliste_kunr (void 			*pvTid,
					   AUSK 			*ptAusk,
					   double 			*pdSumGewNet,
					   double			*pdSumGewBrt,
					   long	  			*plSumTe,
					   elThandle		*hHandle,
					   PRIMANDOPRINT	*ptPrnPrn,
					   int				iAufrufKz,
					   int				*piDruckTour,
					   long				lVerKontKz,
					   long				lAutyp,
					   int				iAufruf,
					   int 				iPrintKunde)
{
	char		acStmt[2048], acSavArtNr[TEID_LEN + 1];
	char		acStmt1[1024], acStmt2[1024];
	int			iDbRvX = 0, iDbRv = 0, nI = 0, iCntArr = 0, iRv = 0, nI2 = 0;
	TOUREN		tTour;
	TEK			atTek[BLOCKSIZE];
	VPLP		atVplp[BLOCKSIZE];
	VPLK		atVplk[BLOCKSIZE];
	LIVERL		tLiVerl, *ptLiVerl = NULL, tLiVerlStandort;
	KST			tKst;
	AUSK 		tAusk;
	static TOURID tTourId;
	elThandle	*hListHandle = NULL;
	ArrDesc		AusDesc;
	char        acFileName[L_tmpnam+1], *pcFileName = NULL;
	long		lSavKsNr, lSavIdx;
	long		lSavThmKsNr, lSavThmIdx;
	long		lAnzThm;
	int			iKopf = 0, nJ, nK;

	/* --- check transfered parameters --- */
	if (ptAusk == NULL) {
		return (-1);
	}

	if (hHandle == NULL) {
		if (ptPrnPrn == NULL) {
			return (-1);
		}
	}	

	memset (&tTourId, 0, sizeof (tTourId));
	
	/* --- get the whole tour structure --- */
	memset (&tLiVerl, 0, sizeof (tLiVerl));
	memset (&AusDesc, 0, sizeof (AusDesc));

	if (strcmp (tTourId.Mand, ptAusk->auskTourId.Mand) != 0 ||
		strcmp (tTourId.Tour, ptAusk->auskTourId.Tour) != 0 ||
		tTourId.PosNr != ptAusk->auskTourId.PosNr) {

		memset (&tTour, 0, sizeof (tTour));
		tTour.tourTourId = ptAusk->auskTourId;

		iDbRv = TExecStdSql (pvTid, StdNselect, TN_TOUREN, &tTour);
		if (iDbRv <= 0) {
			return (-1);
		}	

		/* --- get tour data --- */
		tLiVerl.tTour = tTour;
		if (AppendEle (&AusDesc, &tLiVerl, 1, sizeof (LIVERL)) < 0) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return (-1);
		}	
		iCntArr++;

	}

	/* --- get all vplp's of the actual KuNr+KuPosNr -order- which aren't thm's --- */
	memset (&acStmt[0], 0, sizeof (acStmt));

	sprintf (acStmt, 
		"SELECT "
			"%%TEK, %%VPLP, %%VPLK "
		"FROM "
			"TEK, VPLP, VPLK, AUSK "
		"WHERE "
			"TEK.TeId = VPLP.ZielTeId AND "
			"VPLP.KsNr = VPLK.KsNr AND "
			"VPLP.Status = 'FERTIG' AND "
			"AUSK.AusId_Mand = VPLP.AusId_Mand AND "
			"AUSK.AusId_AusNr = VPLP.AusId_AusNr AND "
			"AUSK.AusId_AusKz = VPLP.AusId_AusKz AND "
			"AUSK.AusId_AusKz != 'INT' AND " /* not crossdocking */
			"VPLP.ThmKz	= 1 AND "
			"VPLP.IstMngs_Mng != 0 AND "
			"VPLK.KuNr = :KuNr AND "
			"VPLK.KuPosNr = :KuPosNr AND "
			"VPLK.TourId_Mand = :Mand AND "
			"VPLK.TourId_Tour = :Tour AND "
			"VPLK.TourId_PosNr = :PosNr ");

	if (lAutyp != 0) {
		memset (acStmt1,0,sizeof(acStmt1));
		memset (acStmt2,0,sizeof(acStmt2));
		GetAutypStmt(lAutyp, acStmt1);

		sprintf (acStmt2,
					"AND %s ", acStmt1);
		strcat (acStmt, acStmt2);
	}

	strcat (acStmt,
				"ORDER BY VPLP.VERDTEID, VPLP.MId_AId_ArtNr, TEK.TeId");

	iDbRvX = 0;
	nI = 0;

	do {
		memset (&atTek[0], 0, sizeof (atTek));
		memset (&atVplp[0], 0, sizeof (atVplp));
		memset (&atVplk[0], 0, sizeof (atVplk));

		iDbRvX = (nI == 0) ?
			TExecSqlX (pvTid, NULL,
				acStmt,
				BLOCKSIZE, 0,
				SELSTRUCT (TN_TEK, atTek[0]),
				SELSTRUCT (TN_VPLP, atVplp[0]),
				SELSTRUCT (TN_VPLK, atVplk[0]),
				SQLSTRING (ptAusk->auskKuNr),
				SQLLONG	  (ptAusk->auskKuPosNr),
				SQLSTRING (ptAusk->auskTourId.Mand),
				SQLSTRING (ptAusk->auskTourId.Tour),
				SQLLONG   (ptAusk->auskTourId.PosNr),
				NULL) :
			TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError (pvTid) != SqlNotFound) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}	
			return (-1);
		}

		if (iDbRvX <= 0) {
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			memset (&tLiVerl, 0, sizeof (tLiVerl));
			strncpy (tLiVerl.tZiel.FeldId, 
				atTek[nI].tekPos.FeldId, FELDID_LEN);
			tLiVerl.tZiel.FeldId[FELDID_LEN] = '\0';

			tLiVerl.tAusk = *ptAusk;
			tLiVerl.tTek = atTek[nI];
			tLiVerl.tVplpOne = atVplp[nI];
			tLiVerl.tVplp = atVplp[nI];
			tLiVerl.tThmVplp = atVplp[nI];
			tLiVerl.tVplk = atVplk[nI];
			tLiVerl.tVplk = atVplk[nI];
			tLiVerl.tBereich.Bereich = atTek[nI].tekBereich;

			*pdSumGewNet += atVplp[nI].vplpIstMngs.Gew;

			CALCCOLLI (&atVplp[nI].vplpSollMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.SollHe,
					   &tLiVerl.tColli.SollRest);

			CALCCOLLI (&atVplp[nI].vplpIstMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.IstHe,
					   &tLiVerl.tColli.IstRest);

			CALCCOLLI (&atVplp[nI].vplpBuchMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.BuchHe,
					   &tLiVerl.tColli.BuchRest);

			if (AppendEle (&AusDesc, &tLiVerl, 1, sizeof (LIVERL)) < 0) {
				if (AusDesc.DataArr) {
					free (AusDesc.DataArr);
					AusDesc.DataArr = NULL;
				}
				return (-1);
			}	
			iCntArr++;
		}
	} while (iDbRvX == BLOCKSIZE);
	lAnzThm = iCntArr - 1;

	/* --- destination for printing list --- */
	if (hHandle != NULL) {
        hListHandle = hHandle;
    } else {
		hListHandle = elCreateList (&el_liverl);

		if (elOpenList(hListHandle, "lists.ger", "VERLLIST" ) == NULL) {
			hListHandle =  elDestroyList (hListHandle);
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}	
			return (-1);
		}
	}

	/* --- print the list with the actual data --- */
	if (iCntArr != 0) {
		/* --- get stored data --- */
		ptLiVerl = (LIVERL *)(void *)AusDesc.DataArr;
		if (ptLiVerl == NULL) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return (-1);
		}

		memset (&tAusk, 0, sizeof (tAusk));
		memset (&acSavArtNr[0], '\0', sizeof (acSavArtNr));
		lSavKsNr = 0;
		lSavIdx = 0;
		lSavThmKsNr = 0;
		lSavThmIdx = 0;

		if (lAnzThm > 0) {
			if (*piDruckTour == 0) {
				elgWrite (hListHandle, elNheader, PN_TOUR,
					(void *)&ptLiVerl[0], NULL);
				elgWrite (hListHandle, elNbody, PN_TOUR,
					(void *)&ptLiVerl[0], NULL);
				*piDruckTour = 1;
			}

			memset (&tTourId, 0, sizeof (tTourId));
			tTourId = ptAusk->auskTourId;

			for (nI = 1; nI < iCntArr; nI++) {
				if (strcmp (tAusk.auskKuNr,
						ptLiVerl[nI].tAusk.auskKuNr) != 0 ||
					tAusk.auskKuPosNr !=
						ptLiVerl[nI].tAusk.auskKuPosNr) {

					if (iPrintKunde == 1) {

						iRv = GetKstData (pvTid, &ptLiVerl[nI].tAusk, &tKst);
						
						if (iRv > 0) {
							ptLiVerl[nI].tAusk.auskAdrs = tKst.kstAdrs;
						}	
						
						elgWrite (hListHandle, elNheader, PN_AUSK,
							(void *)&ptLiVerl[nI], NULL);
						elgWrite (hListHandle, elNbody, PN_AUSK,
							(void *)&ptLiVerl[nI], NULL);
					}

					memset (&tAusk, 0, sizeof (tAusk));
					tAusk = ptLiVerl[nI].tAusk;

					/* Es wird eine Standortzeile geschrieben
					-* d.h. es muss eine Standortzeile geschrieben werden
					-*/
					memset(&tLiVerlStandort, 0, sizeof(tLiVerlStandort));
					switch(tAusk.auskStandort) {
						case STANDORT_BB:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"BRANDENBURG");
							break;
						case STANDORT_OS:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"OSTRAU");
							break;
                        case STANDORT_HH:
                            strcpy(tLiVerlStandort.tStandort.acStandort,
                                    "HAMBURG");
                            break;
						default:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"PORTA WESTFALICA");
							break;
					}
					elgWrite (hListHandle,
							  elNheader, PN_STANDORT, &tLiVerlStandort, NULL);
					elgWrite (hListHandle,
							  elNbody, PN_STANDORT, &tLiVerlStandort, NULL);
				}
				if (nI != 0 &&
					strcmp (ptLiVerl[nI].tVplp.vplpMId.AId.ArtNr,
							ptLiVerl[nI-1].tVplp.vplpMId.AId.ArtNr) != 0 ||
					strcmp (ptLiVerl[nI].tVplp.vplpVerdTeId,
							ptLiVerl[nI-1].tVplp.vplpVerdTeId) != 0) {
					iKopf = 0;
				}

				if (nI == 0 ||
					strcmp(ptLiVerl[nI].tVplp.vplpVerdTeId, 
						   ptLiVerl[nI-1].tVplp.vplpVerdTeId) !=0) {

					elgWrite (hListHandle, elNbody, PN_VERDVPLP,
												&ptLiVerl[nI], NULL);
				}				

				if (iKopf == 0) {
					nK = 1;
					for (nJ = nI; nJ < iCntArr; nJ = nK) {
                       ptLiVerl[nJ].tColli.IstHe = 0;

					   for (nK = nJ;
							nK < iCntArr &&
							strcmp (ptLiVerl[nJ].tVplp.vplpMId.AId.ArtNr,
								ptLiVerl[nK].tVplp.vplpMId.AId.ArtNr) == 0 &&
							strcmp (ptLiVerl[nJ].tVplp.vplpVerdTeId,
								ptLiVerl[nK].tVplp.vplpVerdTeId) == 0;
							nK++) { 
								ptLiVerl[nJ].tColli.IstHe +=
                                ptLiVerl[nK].tVplp.vplpIstMngs.Mng;
						}
						elgWrite (hListHandle, elNheader, PN_VPLP,
                                       &ptLiVerl[nJ], NULL);
                        elgWrite (hListHandle, elNbody, PN_VPLP,
                                       &ptLiVerl[nJ], NULL);
						break;
					}
					iKopf = 1;
				}

				
				elgWrite (hListHandle, elNbody, PN_THMVPLP,
												&ptLiVerl[nI], NULL);
			}
			*plSumTe = *plSumTe + 1;
		}

		if (hHandle == NULL) {
			tmpnam(acFileName);
			pcFileName = acFileName;

			lm2_dump(hListHandle->ehLM2_DESC, ptPrnPrn->Printer.prnSize.Length,
					"Verladeliste", NULL,NULL, NULL, &pcFileName);
			hListHandle = elDestroyList (hListHandle);

			if (localprexecPrnPrn(ptPrnPrn, acFileName) != 1) {
				if (AusDesc.DataArr) {
					free(AusDesc.DataArr);
					AusDesc.DataArr = NULL;
				}
				return -1;
			}
		}

	}

	if (AusDesc.DataArr) {
		free (AusDesc.DataArr);
		AusDesc.DataArr = NULL;
	}	

	return (1);
}

/***************************************************************************
 * FUNCTION HEADER:														   *
 *	int li_verlliste_kunr (...);										   *
 * DESCRIPTION:															   *
 *	prints a list per tour and customer with all te's in the orders		   *
 * RETURN VALUES:														   *
 *	<-1>		error													   *
 *	< 0>		ok														   *
 ***************************************************************************/
int li_verlliste_kunr_cross (void 			*pvTid,
					   AUSK 			*ptAusk,
					   double 			*pdSumGewNet,
					   double			*pdSumGewBrt,
					   long	  			*plSumTe,
					   elThandle		*hHandle,
					   PRIMANDOPRINT	*ptPrnPrn,
					   int				iAufrufKz,
					   int				*piDruckTour,
					   long				lVerKontKz,
					   long				lAutyp,
					   int				iAufruf,
					   int              iPrintKunde)
{
	char		acStmt[1024], acSavArtNr[TEID_LEN + 1];
	int			iDbRvX = 0, iDbRv = 0, nI = 0, iCntArr = 0, iRv = 0, nI2 = 0;
	TOUREN		tTour;
	TEK			atTek[BLOCKSIZE];
	VPLP		atVplp[BLOCKSIZE];
	VPLK		atVplk[BLOCKSIZE];
	LIVERL		tLiVerl, *ptLiVerl = NULL, tLiVerlStandort;
	KST			tKst;
	AUSK 		tAusk;
	static TOURID tTourId;
	elThandle	*hListHandle = NULL;
	ArrDesc		AusDesc;
	char        acFileName[L_tmpnam+1], *pcFileName = NULL;
	long		lSavKsNr, lSavIdx;
	long		lSavThmKsNr, lSavThmIdx;
	long		lAnzThm;
	int			iKopf = 0, nJ, nK;

	/* --- check transfered parameters --- */
	if (ptAusk == NULL) {
		return (-1);
	}

	if (hHandle == NULL) {
		if (ptPrnPrn == NULL) {
			return (-1);
		}
	}	

	memset (&tTourId, 0, sizeof (tTourId));
	
	/* --- get the whole tour structure --- */
	memset (&tLiVerl, 0, sizeof (tLiVerl));
	memset (&AusDesc, 0, sizeof (AusDesc));

	if (strcmp (tTourId.Mand, ptAusk->auskTourId.Mand) != 0 ||
		strcmp (tTourId.Tour, ptAusk->auskTourId.Tour) != 0 ||
		tTourId.PosNr != ptAusk->auskTourId.PosNr) {

		memset (&tTour, 0, sizeof (tTour));
		tTour.tourTourId = ptAusk->auskTourId;

		iDbRv = TExecStdSql (pvTid, StdNselect, TN_TOUREN, &tTour);
		if (iDbRv <= 0) {
			return (-1);
		}	

		/* --- get tour data --- */
		tLiVerl.tTour = tTour;
		if (AppendEle (&AusDesc, &tLiVerl, 1, sizeof (LIVERL)) < 0) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return (-1);
		}	
		iCntArr++;

	}

	/* --- get all vplp's of the actual KuNr+KuPosNr -order- which aren't thm's --- */
	memset (&acStmt[0], 0, sizeof (acStmt));

	sprintf (acStmt, 
		"SELECT "
			"%%TEK, %%VPLP, %%VPLK "
		"FROM "
			"TEK, VPLP, VPLK, AUSK "
		"WHERE "
			"TEK.TeId (+) = VPLP.ZielTeId AND "
			"VPLP.KsNr = VPLK.KsNr AND "
			"AUSK.AusId_Mand = VPLP.AusId_Mand AND "
			"AUSK.AusId_AusNr = VPLP.AusId_AusNr AND "
			"AUSK.AusId_AusKz = VPLP.AusId_AusKz AND "
			"AUSK.AusId_AusKz = 'INT' AND " /* must be crossdocking */
			"AUSK.Standort = :Standort AND " /* crossdocking standort */
			"VPLP.ThmKz	= 1 AND "
			"VPLP.IstMngs_Mng != 0 AND "
			"VPLK.KuNr = :KuNr AND "
			"VPLK.KuPosNr = :KuPosNr AND "
			"VPLK.TourId_Mand = :Mand AND "
			"VPLK.TourId_Tour = :Tour AND "
			"VPLK.TourId_PosNr = :PosNr "
			"ORDER BY VPLP.VERDTEID, VPLP.MId_AId_ArtNr, VPLP.ZielTeId");

	iDbRvX = 0;
	nI = 0;

	do {
		memset (&atTek[0], 0, sizeof (atTek));
		memset (&atVplp[0], 0, sizeof (atVplp));
		memset (&atVplk[0], 0, sizeof (atVplk));

		iDbRvX = (nI == 0) ?
			TExecSqlX (pvTid, NULL,
				acStmt,
				BLOCKSIZE, 0,
				SELSTRUCT (TN_TEK, atTek[0]),
				SELSTRUCT (TN_VPLP, atVplp[0]),
				SELSTRUCT (TN_VPLK, atVplk[0]),
				SQLSTANDORT (ptAusk->auskStandort),
				SQLSTRING (ptAusk->auskKuNr),
				SQLLONG	  (ptAusk->auskKuPosNr),
				SQLSTRING (ptAusk->auskTourId.Mand),
				SQLSTRING (ptAusk->auskTourId.Tour),
				SQLLONG   (ptAusk->auskTourId.PosNr),
				NULL) :
			TExecSqlV (pvTid, NULL, NULL, NULL, NULL, NULL);

		if (iDbRvX <= 0 && TSqlError (pvTid) != SqlNotFound) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}	
			return (-1);
		}

		if (iDbRvX <= 0) {
			break;
		}

		for (nI = 0; nI < iDbRvX; nI++) {
			memset (&tLiVerl, 0, sizeof (tLiVerl));
			strncpy (tLiVerl.tZiel.FeldId, 
				atTek[nI].tekPos.FeldId, FELDID_LEN);
			tLiVerl.tZiel.FeldId[FELDID_LEN] = '\0';

			tLiVerl.tAusk = *ptAusk;
			tLiVerl.tTek = atTek[nI];
			tLiVerl.tVplpOne = atVplp[nI];
			tLiVerl.tVplp = atVplp[nI];
			tLiVerl.tThmVplp = atVplp[nI];
			tLiVerl.tVplk = atVplk[nI];
			tLiVerl.tVplk = atVplk[nI];
			tLiVerl.tBereich.Bereich = atTek[nI].tekBereich;

			*pdSumGewNet += atVplp[nI].vplpIstMngs.Gew;

			CALCCOLLI (&atVplp[nI].vplpSollMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.SollHe,
					   &tLiVerl.tColli.SollRest);

			CALCCOLLI (&atVplp[nI].vplpIstMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.IstHe,
					   &tLiVerl.tColli.IstRest);

			CALCCOLLI (&atVplp[nI].vplpBuchMngs.Mng,
					   &atVplp[nI].vplpSollMngs.VeHeFa,
					   &tLiVerl.tColli.BuchHe,
					   &tLiVerl.tColli.BuchRest);

			if (AppendEle (&AusDesc, &tLiVerl, 1, sizeof (LIVERL)) < 0) {
				if (AusDesc.DataArr) {
					free (AusDesc.DataArr);
					AusDesc.DataArr = NULL;
				}
				return (-1);
			}	
			iCntArr++;
		}
	} while (iDbRvX == BLOCKSIZE);
	lAnzThm = iCntArr - 1;

	/* --- destination for printing list --- */
	if (hHandle != NULL) {
        hListHandle = hHandle;
    } else {
		hListHandle = elCreateList (&el_liverl);

		if (elOpenList(hListHandle, "lists.ger", "VERLLIST" ) == NULL) {
			hListHandle =  elDestroyList (hListHandle);
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}	
			return (-1);
		}
	}

	/* --- print the list with the actual data --- */
	if (iCntArr != 0) {
		/* --- get stored data --- */
		ptLiVerl = (LIVERL *)(void *)AusDesc.DataArr;
		if (ptLiVerl == NULL) {
			if (AusDesc.DataArr) {
				free (AusDesc.DataArr);
				AusDesc.DataArr = NULL;
			}
			return (-1);
		}

		memset (&tAusk, 0, sizeof (tAusk));
		memset (&acSavArtNr[0], '\0', sizeof (acSavArtNr));
		lSavKsNr = 0;
		lSavIdx = 0;
		lSavThmKsNr = 0;
		lSavThmIdx = 0;

		if (lAnzThm > 0) {
			if (*piDruckTour == 0) {
				elgWrite (hListHandle, elNheader, PN_TOUR,
					(void *)&ptLiVerl[0], NULL);
				elgWrite (hListHandle, elNbody, PN_TOUR,
					(void *)&ptLiVerl[0], NULL);
				*piDruckTour = 1;
			}

			memset (&tTourId, 0, sizeof (tTourId));
			tTourId = ptAusk->auskTourId;

			for (nI = 1; nI < iCntArr; nI++) {
				if (strcmp (tAusk.auskKuNr,
						ptLiVerl[nI].tAusk.auskKuNr) != 0 ||
					tAusk.auskKuPosNr !=
						ptLiVerl[nI].tAusk.auskKuPosNr) {

					if (iPrintKunde == 1) {
						iRv = GetKstData (pvTid, &ptLiVerl[nI].tAusk, &tKst);

						if (iRv > 0) {
							ptLiVerl[nI].tAusk.auskAdrs = tKst.kstAdrs;
						}	
						
						elgWrite (hListHandle, elNheader, PN_AUSK,
							(void *)&ptLiVerl[nI], NULL);
						elgWrite (hListHandle, elNbody, PN_AUSK,
							(void *)&ptLiVerl[nI], NULL);
					}

					memset (&tAusk, 0, sizeof (tAusk));
					tAusk = ptLiVerl[nI].tAusk;

					/* Es wird eine Standortzeile geschrieben
					-*/
					memset(&tLiVerlStandort, 0, sizeof(tLiVerlStandort));
					switch(tAusk.auskStandort) {
						case STANDORT_BB:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"BRANDENBURG");
							break;
						case STANDORT_OS:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"OSTRAU");
							break;
						case STANDORT_HH:
						     strcpy(tLiVerlStandort.tStandort.acStandort,
						            "HAMBURG");
						    break;	
						default:
							strcpy(tLiVerlStandort.tStandort.acStandort,
									"PORTA WESTFALICA");
							break;
					}
					elgWrite (hListHandle,
							  elNheader, PN_STANDORT, &tLiVerlStandort, NULL);
					elgWrite (hListHandle,
							  elNbody, PN_STANDORT, &tLiVerlStandort, NULL);
				}

				if (nI != 0 &&
					strcmp (ptLiVerl[nI].tVplp.vplpMId.AId.ArtNr,
							ptLiVerl[nI-1].tVplp.vplpMId.AId.ArtNr) != 0 ||
					strcmp (ptLiVerl[nI].tVplp.vplpVerdTeId,
							ptLiVerl[nI-1].tVplp.vplpVerdTeId) != 0) {
					iKopf = 0;
				}

				if (nI == 0 ||
					strcmp(ptLiVerl[nI].tVplp.vplpVerdTeId, 
						   ptLiVerl[nI-1].tVplp.vplpVerdTeId) !=0) {

					elgWrite (hListHandle, elNbody, PN_VERDVPLP,
												&ptLiVerl[nI], NULL);
				}				

				if (iKopf == 0) {
					nK = 1;
					for (nJ = nI; nJ < iCntArr; nJ = nK) {
                       ptLiVerl[nJ].tColli.IstHe = 0;

					   for (nK = nJ;
							nK < iCntArr &&
							strcmp (ptLiVerl[nJ].tVplp.vplpMId.AId.ArtNr,
								ptLiVerl[nK].tVplp.vplpMId.AId.ArtNr) == 0 &&
							strcmp (ptLiVerl[nJ].tVplp.vplpVerdTeId,
								ptLiVerl[nK].tVplp.vplpVerdTeId) == 0;
							nK++) { 
								ptLiVerl[nJ].tColli.IstHe +=
                                ptLiVerl[nK].tVplp.vplpIstMngs.Mng;
						}
						elgWrite (hListHandle, elNheader, PN_VPLP,
                                       &ptLiVerl[nJ], NULL);
                        elgWrite (hListHandle, elNbody, PN_VPLP,
                                       &ptLiVerl[nJ], NULL);
						break;
					}
					iKopf = 1;
				}

				elgWrite (hListHandle, elNbody, PN_THMVPLP,
												&ptLiVerl[nI], NULL);
			}
			*plSumTe = *plSumTe + 1;
		}

		if (hHandle == NULL) {
			tmpnam(acFileName);
			pcFileName = acFileName;

			lm2_dump(hListHandle->ehLM2_DESC, ptPrnPrn->Printer.prnSize.Length,
					"Verladeliste", NULL,NULL, NULL, &pcFileName);
			hListHandle = elDestroyList (hListHandle);

			if (localprexecPrnPrn(ptPrnPrn, acFileName) != 1) {
				if (AusDesc.DataArr) {
					free(AusDesc.DataArr);
					AusDesc.DataArr = NULL;
				}
				return -1;
			}
		}

	}

	if (AusDesc.DataArr) {
		free (AusDesc.DataArr);
		AusDesc.DataArr = NULL;
	}	

	return (1);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Check whether the AUSK can be set to status VERLB or READY
-*      In this case function set the status of AUSK
-* RETURNS
-*  0...Status not set
-*  1...Status to set
-*--------------------------------------------------------------------------*/
int SetAuskStatusVerlb(void *pvTid, AUSK *ptAusk, char *pcFac)
{
    int     iRv=0;

	if (ptAusk->auskStatus != AUSKSTATUS_VERLK) {
		/* Ausk nor VERLK -> status not to change
		-*/
		return 0;
	}

	iRv = VerlkFromAuskFERTIG(pvTid, ptAusk, pcFac);
	switch (iRv) {
		case 0:
			return 0;
		case 1:
			break;
		default:
			return -1;
	}

    if ( TExecStdSql (  pvTid, StdNselectUpdNo, TN_AUSK, ptAusk) < 0) {
        LogPrintf(pcFac, LT_ALERT,
            "Lock TEK: %s ", TSqlErrTxt(pvTid));
        return -1;
    }
	iRv = AusVerlkontr(pvTid, ptAusk, pcFac);
	if (iRv < 0){
        LogPrintf (pcFac, LT_ALERT, "SetAuskStatusVerlb AUSK '%s/%s/%d'\n"
                                         " SqlErrTxt:%s",
                                         ptAusk->auskAusId.Mand,
                                         ptAusk->auskAusId.AusNr,
                                         ptAusk->auskAusId.AusKz,
                                         TSqlErrTxt(pvTid));
		return (-1);
	}

    return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Check whether the AUSK can be set to status VERLB or READY
-*      In this case function set the status of AUSK
-* RETURNS
-*  0...Status not set
-*  1...Status to set
-*--------------------------------------------------------------------------*/
int SetAuskStatusVerlbFunk(void *pvTid, AUSK *ptAusk, char *pcFac)
{
    int     iRv=0;

    if (TExecStdSql (  pvTid, StdNselectUpdNo, TN_AUSK, ptAusk) < 0) {
        LogPrintf(pcFac, LT_ALERT,
            "Lock TEK: %s ", TSqlErrTxt(pvTid));
        return -1;
    }

	iRv = AusVerlkontrFunk (pvTid, ptAusk, pcFac);

	if (iRv < 0){
        LogPrintf (pcFac, LT_ALERT, "SetAuskStatusVerlb AUSK '%s/%s/%d'\n"
                                         " SqlErrTxt:%s",
                                         ptAusk->auskAusId.Mand,
                                         ptAusk->auskAusId.AusNr,
                                         ptAusk->auskAusId.AusKz,
                                         TSqlErrTxt(pvTid));
		return (-1);
	}

    return 0;
}

int AusVerlkontrFunk(void *pvTid, AUSK *ptAusk, char *pcFac)
{
    SetHist (TN_AUSK,ptAusk,HIST_UPDATE,who_am_i);

    if (ptAusk->auskVerlMeld == JANEIN_J) {

		if (ptAusk->auskMeldHostVerlb == JANEIN_J) {
			if (IsEmptyStrg(ptAusk->auskTourId.Tour) == 1 &&
				ptAusk->auskHostStatus == AUSKHOSTSTATUS_NEU) {
				if (SetAuskHostStatus (pvTid, pcFac,
								ptAusk, AUSKHOSTSTATUS_SENDENAB) < 0) {
					return (-1);
				}
			}

			if (ptAusk->auskHostStatus != AUSKHOSTSTATUS_FERTIG &&
				ptAusk->auskHostStatus != AUSKHOSTSTATUS_FERTIGAB) {
				
				if (SetAuspVplpHostStatus(pvTid, pcFac, ptAusk) < 0) {
					return (-1);
				}

			}
		}

		return(SetAuskStatus(pvTid, pcFac, ptAusk, AUSKSTATUS_VERLB));
	} else {
		if (IsEmptyStrg(ptAusk->auskTourId.Tour) == 1 &&
			ptAusk->auskHostStatus == AUSKHOSTSTATUS_NEU) {
			if (SetAuskHostStatus (pvTid, pcFac,
							ptAusk, AUSKHOSTSTATUS_SENDENAB) < 0) {
				return (-1);
			}
		}
		if (SetAuspVplpHostStatus(pvTid, pcFac, ptAusk) < 0) {
			return (-1);
		}
		return(SetAuskStatus(pvTid, pcFac, ptAusk, AUSKSTATUS_FERTIG));
	}
}
int AusVerlkontr(void *pvTid, AUSK *ptAusk, char *pcFac)
{
	if (ptAusk->auskStatus != AUSKSTATUS_VERLK) {
		/* don't change Status of AUSK
		-*/
		return 0;
	}
    SetHist (TN_AUSK,ptAusk,HIST_UPDATE,who_am_i);

    if (ptAusk->auskVerlMeld == JANEIN_J) {

		if (ptAusk->auskMeldHostVerlb == JANEIN_J) {
			if (IsEmptyStrg(ptAusk->auskTourId.Tour) == 1 &&
				ptAusk->auskHostStatus == AUSKHOSTSTATUS_NEU) {
				if (SetAuskHostStatus (pvTid, pcFac,
								ptAusk, AUSKHOSTSTATUS_SENDENAB) < 0) {
					return (-1);
				}
			}
			if (SetAuspVplpHostStatus(pvTid, pcFac, ptAusk) < 0) {
				return (-1);
			}
		}

		return(SetAuskStatus(pvTid, pcFac, ptAusk, AUSKSTATUS_VERLB));
	} else {
		if (IsEmptyStrg(ptAusk->auskTourId.Tour) == 1 &&
			ptAusk->auskHostStatus == AUSKHOSTSTATUS_NEU) {
			if (SetAuskHostStatus (pvTid, pcFac,
							ptAusk, AUSKHOSTSTATUS_SENDENAB) < 0) {
				return (-1);
			}
		}
		if (SetAuspVplpHostStatus(pvTid, pcFac, ptAusk) < 0) {
			return (-1);
		}
		return(SetAuskStatus(pvTid, pcFac, ptAusk, AUSKSTATUS_FERTIG));
	}
}



