/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include <owil.h>

/* ------- Tools-Headers -------------------------------------------------- */
#include <menufct.h>
#include <owrcloader.h>
#include <t_util.h>
#include <elements.h>
#include <global.h>
#include <logtool.h>
#include <wamasbox.h>
#include <dbsqlstd.h>
#include <cycle.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "facility.h"
#include "rbra.h"
#include "global.h"
#include "vglobal.h"
#include "fes.h"
#include "fsp.h"
#include "aplcomm.h"
#include "aplcomm_util.h"
#include "proc.h"
#include "init_table.h"

#define _ME_BZSET_C
#include "me_bzset.h"
#undef _ME_BZSET_C


/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define _RC_NAME        "ME_BZSET"
#define _ME_NAME_RC     "me_bzset.rc"
#define BLOCKSIZE 		500

#define CREATE 	1
#define REMOVE  0
#define ABFRAGE_LEN		(50)

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/
 /* ------- Enums --------------------------------------------------------- */

#define POS_VZ6_17			1017
#define POS_VZ6_1316		1316
#define QVW_7071			7071
#define OG6_STAUBAHN_07 	1007
#define OG6_STAUBAHN_08 	1008
#define OG6_STAUBAHN_59 	1059

 /* ------- Structures ---------------------------------------------------- */
typedef struct _MY_CTX {
	int 	aiSoll1[3];
	int 	aiSoll2[8];
	int 	aiSoll21[8];
	int 	aiSoll6[2];
	int 	aiSollPos[4];
	int 	aiIst1[3];
	int 	aiIst2[8];
	int 	aiIst21[8];
	int 	aiIst6[2];
	int 	aiIstPos[4];
	int 	aiRbgSperreSoll[4];
	int 	aiRbgSperreIst[4];
	int 	aiRbgTaStop[4];
	int 	iAusIstC4;
	int 	iAusIst48;
	int 	iAusIstE0;
	int 	iAusIstE5;
	int 	iAusSollC4;
	int 	iAusSoll48;
	int 	iAusSollE0;
	int 	iAusSollE5;
	int 	iUmschaltC4;
	int 	iUmschalt48;
	int 	iUmschaltE0;
	int 	iUmschaltE5;
	int 	iLift5354;
	int 	iLiftE3E4;
	int 	iLift6364;
	int		iWicklerIst;
	int		iWicklerSoll;
	int		iWicklerUm;
	int 	iSperre_E_A_5556;
	int 	iSperre_E_A_5E5F;
	int 	iSperre_E_A_E5E7;
	int 	iSperre_E_A_E1E2;
	int 	iSperre_E_A_E9EB;
	int 	iSperre_E_A_ECED;
	long	iSperre6OG[MAX_SPERREN_6OG];
	int		iIstPalAnz[MAX_VERTEILARTEN];
	int		iSollPalAnz[MAX_VERTEILARTEN];
	int		iVerteilSperre[MAX_VERTEILARTEN];
	int		iStauSoll[MAX_UMSCHALT];
	int		iStauIst[MAX_UMSCHALT];
	int		iStauUmschalt[MAX_UMSCHALT];
	int		iStauSollVzs2B[MAX_UMSCHALT];
	int		iStauIstVzs2B[MAX_UMSCHALT];
	int		iStauUmschaltVzs2B[MAX_UMSCHALT];
	int		iSperreQvw7071;
	int		iSperrePos17;
	int		iSperrePos1316;
	int		iNachAnz;
	int 	iStopScan50;
	int 	iStopScanE0;
} MY_CTX;

typedef struct {
	int		nLift;
	char	*pFeldId;
}_LiftSperrPos;

static _LiftSperrPos LiftSperrPos[] = {
	/* Lift, 		Position,FeldId	*/
	/*------------------------------*/
	{LIFT_5354,		FELDID_VZS_50,},
	{LIFT_5354,		FELDID_VZS_51,},
	{LIFT_5354,		FELDID_VZS_52,},
	{LIFT_5354,		FELDID_VZS_53,},
	{LIFT_5354,		FELDID_VZS_54,},
	{LIFT_5354,		FELDID_VZS_55,},
	{LIFT_5354,		FELDID_VZS_56,},
	{LIFT_5354,		FELDID_VZS_5E,},
	{LIFT_5354,		FELDID_VZS_5F,},
	{LIFT_E3E4,		FELDID_VZS_E0,},
	{LIFT_E3E4,		FELDID_VZS_E1,},
	{LIFT_E3E4,		FELDID_VZS_E2,},
	{LIFT_E3E4,		FELDID_VZS_E3,},
	{LIFT_E3E4,		FELDID_VZS_E4,},
	{LIFT_E3E4,		FELDID_VZS_E5,},
	{LIFT_E3E4,		FELDID_VZS_E6,},
	{LIFT_E3E4,		FELDID_VZS_E7,},
	{LIFT_E3E4,		FELDID_VZS_E8,},
	{LIFT_E3E4,		FELDID_VZS_E9,},
	{LIFT_E3E4,		FELDID_VZS_EA,},
	{LIFT_E3E4,		FELDID_VZS_EB,},
	{LIFT_E3E4,		FELDID_VZS_EC,},
	{LIFT_E3E4,		FELDID_VZS_ED,},
	{LIFT_E3E4,		FELDID_VZS_EE,},
	{LIFT_6364,		FELDID_VZS_61,},
	{LIFT_6364,		FELDID_VZS_62,},
	{LIFT_6364,		FELDID_VZS_65,},
	{LIFT_6364,		FELDID_VZS_66,},
	{LIFT_6364,		FELDID_VZS_06,},
	{LIFT_6364,		FELDID_VZS_44,},
	{LIFT_6364,		FELDID_VZS_45,},
	{LIFT_6364,		FELDID_VZS_67,},
	{LIFT_6364,		FELDID_VZS_68,},
	{LIFT_6364,		FELDID_VZS_69,},
};

typedef struct {
	int		nEinAus;
	char	*pFeldId;
}_EinAusSperrPos;

static _EinAusSperrPos EinAusSperrPos[] = {
	/* EinAusBahn, 	Position,FeldId	*/
	/*------------------------------*/
	{EINAUS_5556,	FELDID_VZS_55,},
	{EINAUS_5556,	FELDID_VZS_56,},
	{EINAUS_5E5F,	FELDID_VZS_5E,},
	{EINAUS_5E5F,	FELDID_VZS_5F,},
	{EINAUS_E5E7,	FELDID_VZS_E5,},
	{EINAUS_E5E7,	FELDID_VZS_E6,},
	{EINAUS_E5E7,	FELDID_VZS_E7,},
	{EINAUS_E1E2,	FELDID_VZS_E1,},
	{EINAUS_E1E2,	FELDID_VZS_E2,},
	{EINAUS_E9EB,	FELDID_VZS_E9,},
	{EINAUS_E9EB,	FELDID_VZS_EA,},
	{EINAUS_E9EB,	FELDID_VZS_EB,},
	{EINAUS_ECED,	FELDID_VZS_EC,},
	{EINAUS_ECED,	FELDID_VZS_ED,},
};
/* ===========================================================================
 * LOCAL (STATIC) Variables and Function-Prototypes
 * =========================================================================*/

 /* ------- Variables ----------------------------------------------------- */

 /* ------- Function-Prototypes ------------------------------------------- */

static int me_abfrage (OWidget w, char *pacAbfrage);
static MY_CTX tMcCheck;

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
-* RETURNS
-*--------------------------------------------------------------------------*/
static int SetFsp (MskDialog hMaskRl, FSP *ptFsp) 
{
	int		iRv;

	iRv = TExecStdSql(hMaskRl, StdNinsert, TN_FSP, ptFsp);

	if(iRv != 1 && TSqlError(hMaskRl) != SqlDuplicate) {
		LogPrintf(FAC_ME_BZSET, LT_ALERT,
				"Fehler beim Sperren der Position: %s\n"
				"ErrTxt: %s",
				ptFsp->fspPos.FeldId, 
				TSqlErrTxt(hMaskRl));

		return -1;
	}

	LogPrintf(FAC_ME_BZSET, LT_ALERT,
				"Position: %s von %s gesperrt",
				ptFsp->fspPos.FeldId,
				GetUserOrTaskName ());

	return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int RemoveFsp(MskDialog hMaskRl, FSP *ptFsp) 
{
    int     iRv;

    iRv = TExecStdSql(hMaskRl, StdNdelete, TN_FSP, ptFsp);

    if(iRv < 0 && TSqlError(hMaskRl) != SqlNotFound) {
        LogPrintf(FAC_ME_BZSET, LT_ALERT,
                "Fehler beim Entsperren der Position %s\n %s", 
				ptFsp->fspPos.FeldId, TSqlErrTxt(hMaskRl));

        return (-1);
    }

	LogPrintf(FAC_ME_BZSET, LT_ALERT,
				"Position: %s von %s entsperrt",
				ptFsp->fspPos.FeldId,
				GetUserOrTaskName ());

    return 0;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int DoPosSperre (MskDialog hMaskRl) {
	
	int 	iRv;
	int		nI;
	MY_CTX	*ptMc;
	FSP		tFsp;

	ptMc = (MY_CTX *) MskRlMaskGet(hMaskRl, MskNmaskCalldata);

	/* Die Positionen 30/32 und die Positionen 34/36 haengen 
	   direkt zusammen */

	if( ptMc->aiSollPos[0] == 1) {
		ptMc->aiSollPos[1] = 1;
	} else {
		ptMc->aiSollPos[1] = 0;
	}

	if( ptMc->aiSollPos[2] == 1) {
		ptMc->aiSollPos[3] = 1;
	} else {
		ptMc->aiSollPos[3] = 0;
	}
		
	for(nI = 0; nI < 4; nI ++) {
		if(ptMc->aiSollPos[nI] == 1 &&
		   isset(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_posstart, nI) == 1){
			continue;	/* Keine Aenderung */
		}

		if(ptMc->aiSollPos[nI] == 0 &&
			isset(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_posstart, nI) == 0){
            continue;   /* Keine Aenderung */
        }

		memset(&tFsp, 0, sizeof(FSP));
        strcpy(tFsp.fspFspGrund, 	"BZ-NOTAUSLAGERUNG");
		sprintf(tFsp.fspPos.FeldId, "VZS-3%d", nI*2);
		tFsp.fspFspModus = FSPMODUS_TOT;

		if(ptMc->aiSollPos[nI] == 1) {
			/* Sperren */
			iRv = SetFsp(hMaskRl, &tFsp);
			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Position 3%d", 2*nI);
				return -1;
			}
		}

		if(ptMc->aiSollPos[nI] == 0) {
			/* Entsperren */
			iRv = RemoveFsp(hMaskRl, &tFsp);
			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Position 3%d", 2*nI);
				return -1;
			}
		}
	}
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int DoEinAusSperre (MskDialog hMaskRl, int nEinAus) 
{
	int 	iRv, nI;
    MY_CTX  *ptMc;
	FSP		tFsp;
	RBRAK	rbrak;

    ptMc = (MY_CTX *) MskRlMaskGet(hMaskRl, MskNmaskCalldata);
	
	switch (nEinAus) {
	case EINAUS_5556:
		if (ptMc->iSperre_E_A_5556 == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5556]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_5556) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_5556 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case EINAUS_5E5F:
		if (ptMc->iSperre_E_A_5E5F == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5E5F]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_5E5F) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_5E5F != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case EINAUS_E5E7:
		if (ptMc->iSperre_E_A_E5E7 == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E5E7]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_E5E7) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_E5E7 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case EINAUS_E1E2:
		if (ptMc->iSperre_E_A_E1E2 == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E1E2]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_E1E2) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_E1E2 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case EINAUS_E9EB:
		if (ptMc->iSperre_E_A_E9EB == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E9EB]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_E9EB) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_E9EB != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case EINAUS_ECED:
		if (ptMc->iSperre_E_A_ECED == C->vzs_ctxt[2].vd.vd_einaus[EINAUS_ECED]){
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (EinAusSperrPos); nI++) {

			if (EinAusSperrPos[nI].nEinAus != EINAUS_ECED) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-EINAUSSPERRE");
			strcpy(tFsp.fspPos.FeldId,   EinAusSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iSperre_E_A_ECED != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer EinAuslagerbahn");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer EinAuslagerbahn.");
					return -1;
				}
			}
		}

		break;

	case QVW_7071:
		if (ptMc->iSperreQvw7071 == C->PosSperren[0]){
			return 1;
		}

		memset (&tFsp, 0, sizeof (tFsp));

		strcpy(tFsp.fspFspGrund, "BZ-Notauslagerung");
		strcpy(tFsp.fspPos.FeldId, FELDID_VZS_B1);
		tFsp.fspFspModus = FSPMODUS_TOT;

		if (ptMc->iSperreQvw7071 != 0) {
			iRv = SetFsp (hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}
		} else {
			iRv = RemoveFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
							"fuer Staurollenfoerderer.");
				return -1;
			}
		}

		break;

	case OG6_STAUBAHN_07:
		if (ptMc->iSperre6OG[0] == C->sperren6OG[0]){
			return 1;
		}
		memset (&tFsp, 0, sizeof (tFsp));

		strcpy(tFsp.fspFspGrund, "BZ-SPERRE");
		strcpy(tFsp.fspPos.FeldId, FELDID_VZ6_07);
		tFsp.fspFspModus = FSPMODUS_TOT;

		if (ptMc->iSperre6OG[0] != 0) {
			iRv = SetFsp (hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}
		} else {
			iRv = RemoveFsp(hMaskRl, &tFsp);
			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
							"fuer Staurollenfoerderer.");
				return -1;
			}
		}

		break;

	case OG6_STAUBAHN_08:
		if (ptMc->iSperre6OG[1] == C->sperren6OG[1]){
			return 1;
		}
		memset (&tFsp, 0, sizeof (tFsp));

		strcpy(tFsp.fspFspGrund, "BZ-SPERRE");
		strcpy(tFsp.fspPos.FeldId, FELDID_VZ6_08);
		tFsp.fspFspModus = FSPMODUS_TOT;
		fprintf(stderr,"Sperre6OG_08: %ld\n",ptMc->iSperre6OG[1]);
		if (ptMc->iSperre6OG[1] != 0) {
			iRv = SetFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}
		} else {
			iRv = RemoveFsp(hMaskRl, &tFsp);
			fprintf(stderr,"Sperre6OG_08 FSP geloescht\n");
			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
							"fuer Staurollenfoerderer.");
				return -1;
			}
		}
		break;

	case OG6_STAUBAHN_59:
		if (ptMc->iSperre6OG[2] == C->sperren6OG[2]){
			return 1;
		}
		memset (&tFsp, 0, sizeof (tFsp));

		strcpy(tFsp.fspFspGrund, "BZ-SPERRE");
        strcpy(tFsp.fspPos.FeldId, FELDID_VZ6_20);
        tFsp.fspFspModus = FSPMODUS_TOT;

		if (ptMc->iSperre6OG[2] != 0) {
		
			iRv = SetFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}
		} else {
			iRv = RemoveFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
                SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
                            "fuer Staurollenfoerderer.");
                return -1;
            }
		}
		break;

	case POS_VZ6_17:
		if (ptMc->iSperrePos17 == C->PosSperren[1]){
			return 1;
		}

		if (ptMc->iSperrePos17 != 0) {
			
			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-Notauslagerung");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_16);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = SetFsp (hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}

			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-Notauslagerung");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_20);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = SetFsp (hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
							"fuer Staurollenfoerderer");
				return -1;
			}

			LogPrintf (FAC_ME_BZSET, LT_ALERT, 
							"BZ Position VZ6-17 sperren aktiviert");

		} else {
			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-Notauslagerung");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_16);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = RemoveFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
							"fuer Staurollenfoerderer.");
				return -1;
			}

			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-Notauslagerung");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_20);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = RemoveFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
							"fuer Staurollenfoerderer.");
				return -1;
			}

			LogPrintf (FAC_ME_BZSET, LT_ALERT, 
							"BZ Position VZ6-17 sperren deaktiviert");
		}

		break;

	case POS_VZ6_1316:
		if (ptMc->iSperrePos1316 == C->PosSperren[2]){
			return 1;
		}

		if (ptMc->iSperrePos1316 != 0) {
			
			memset (&rbrak, 0, sizeof ( rbrak ));

			strcpy (rbrak.rbrakFrom.FeldId, "VZ6-09" );
			strcpy (rbrak.rbrakTo.FeldId, "VZ6-13" );
			strcpy (rbrak.rbrakTpmId, "VZS6" );

			iRv = TExecStdSql (hMaskRl,StdNselectUpd,TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Lesen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-09");
				return -1;
			}
			
			rbrak.rbrakFixCost = 9999;	

			iRv = TExecStdSql (hMaskRl, StdNupdate, TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Eintragen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-09");
				return -1;
			}

			memset (&rbrak, 0, sizeof ( rbrak ));

			strcpy (rbrak.rbrakFrom.FeldId, "VZ6-0A" );
			strcpy (rbrak.rbrakTo.FeldId, "VZ6-13" );
			strcpy (rbrak.rbrakTpmId, "VZS6" );

			iRv = TExecStdSql (hMaskRl,StdNselectUpd,TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Lesen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-0A");
				return -1;
			}
			
			rbrak.rbrakFixCost = 100;	

			iRv = TExecStdSql (hMaskRl, StdNupdate, TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Eintragen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-0A");
				return -1;
			}

			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-6OG-WOCHENENDE");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_16);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = SetFsp (hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
						"fuer Wochenendbetrieb Pos: %s",tFsp.fspPos.FeldId);
				return -1;
			}

			LogPrintf (FAC_ME_BZSET, LT_ALERT, 
							"Wochenendbetrieb 6. OG aktiviert");

		} else {

			memset (&rbrak, 0, sizeof ( rbrak ));

			strcpy (rbrak.rbrakFrom.FeldId, "VZ6-09" );
			strcpy (rbrak.rbrakTo.FeldId, "VZ6-13" );
			strcpy (rbrak.rbrakTpmId, "VZS6" );

			iRv = TExecStdSql (hMaskRl,StdNselectUpd,TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Lesen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-09");
				return -1;
			}
			
			rbrak.rbrakFixCost = 100;	

			iRv = TExecStdSql (hMaskRl, StdNupdate, TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Eintragen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-09");
				return -1;
			}

			memset (&rbrak, 0, sizeof ( rbrak ));

			strcpy (rbrak.rbrakFrom.FeldId, "VZ6-0A" );
			strcpy (rbrak.rbrakTo.FeldId, "VZ6-13" );
			strcpy (rbrak.rbrakTpmId, "VZS6" );

			iRv = TExecStdSql (hMaskRl,StdNselectUpd,TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Lesen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-0A");
				return -1;
			}
			
			rbrak.rbrakFixCost = 10;	

			iRv = TExecStdSql (hMaskRl, StdNupdate, TN_RBRAK, (void *)&rbrak);

			if(iRv != 1) {
				SetErrmsg1( "Fehler beim Eintragen der Routingkosten\n"
						"fuer Wochenendbetrieb Pos: %s","VZ6-0A");
				return -1;
			}

			memset (&tFsp, 0, sizeof (tFsp));

			strcpy (tFsp.fspFspGrund, "BZ-6OG-WOCHENENDE");
			strcpy (tFsp.fspPos.FeldId, FELDID_VZ6_16);
			tFsp.fspFspModus = FSPMODUS_TOT;

			iRv = RemoveFsp(hMaskRl, &tFsp);

			if(iRv != 0) {
				SetErrmsg1( "Fehler beim Loeschen der Feldsperre\n"
						"fuer Wochenendbetrieb Pos: %s",tFsp.fspPos.FeldId);
				return -1;
			}

			LogPrintf (FAC_ME_BZSET, LT_ALERT, 
							"Wochenendbetrieb 6. OG deaktiviert");
		}

		break;

	default:	
		break;

	}
	
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int DoLiftSperre (MskDialog hMaskRl, int nLift) {

	int 	iRv, nI;
    MY_CTX  *ptMc;
	FSP		tFsp;

    ptMc = (MY_CTX *) MskRlMaskGet(hMaskRl, MskNmaskCalldata);
	
	MskTransferMaskDup(hMaskRl);

	switch (nLift) {
	case LIFT_5354:
		if(ptMc->iLift5354 == C->vzs_ctxt[2].vd.vd_lift[LIFT_5354]) {
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (LiftSperrPos); nI++) {

			if (LiftSperrPos[nI].nLift != LIFT_5354) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-LIFTSPERRE");
			strcpy(tFsp.fspPos.FeldId,   LiftSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iLift5354 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer Senkrechtfoerderer");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer Senkrechtfoerderer.");
					return -1;
				}
			}
		}

		break;

	case LIFT_E3E4:
		if(ptMc->iLiftE3E4 == C->vzs_ctxt[2].vd.vd_lift[LIFT_E3E4]) {
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (LiftSperrPos); nI++) {

			if (LiftSperrPos[nI].nLift != LIFT_E3E4) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-LIFTSPERRE");
			strcpy(tFsp.fspPos.FeldId,   LiftSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iLiftE3E4 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer Senkrechtfoerderer");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer Senkrechtfoerderer.");
					return -1;
				}
			}
		}

		break;

	case LIFT_6364:
		if(ptMc->iLift6364 == C->vzs_ctxt[2].vd.vd_lift[LIFT_6364]) {
			/* Keine Aenderung */
			return 1;
		}

		for (nI = 0; nI < NO_ELE (LiftSperrPos); nI++) {

			if (LiftSperrPos[nI].nLift != LIFT_6364) {
				continue;
			} 

			memset(&tFsp, 0, sizeof(FSP));
			strcpy(tFsp.fspFspGrund, 	"BZ-LIFTSPERRE");
			strcpy(tFsp.fspPos.FeldId,   LiftSperrPos[nI].pFeldId);
			tFsp.fspFspModus = FSPMODUS_TOT;

			if(ptMc->iLift6364 != 0) {
				iRv = SetFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Eintragen der Feldsperre\n"
								"fuer Senkrechtfoerderer");
					return -1;
				}
			} else {
				iRv = RemoveFsp(hMaskRl, &tFsp);

				if(iRv != 0) {
					SetErrmsg1( "Fehler beim Loeschen der Feldsperre \n"
								"fuer Senkrechtfoerderer.");
					return -1;
				}
			}
		}

		break;

	default:	
		break;

	}
	
	return 1;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Schreibt den Zustand der einzelnen Anlagenteile in die Globalen.
-* RETURNS
-*	void Funktion !! --> kein Returnwert
-*--------------------------------------------------------------------------*/
static void WriteGlobDataVZS (MskDialog hMaskRl) {

	MY_CTX 	*ptMc;
	int		nBit;
	int 	nI;

	ptMc = (MY_CTX *) MskRlMaskGet(hMaskRl, MskNmaskCalldata);

	for(nBit = 0; nBit < 3; nBit++) {
		if(ptMc->aiSoll1[nBit] == 1) {
			setbit (&C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_soll_start[0], nBit);
		} else {
			clrbit (&C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_soll_start[0], nBit);
		}
	}

	for(nBit = 3; nBit < 7; nBit++) {
        if(ptMc->aiSoll2[nBit] == 1) {
            setbit (&C->vzs_ctxt[TMNR_VZS_VZS2A].vd.vd_soll_start[0], nBit);
        } else {
            clrbit (&C->vzs_ctxt[TMNR_VZS_VZS2A].vd.vd_soll_start[0], nBit);
        }
    }

	for(nBit = 0; nBit < 3; nBit++) {
        if(ptMc->aiSoll2[nBit] == 1) {
            setbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], nBit);
        } else {
            clrbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], nBit);
        }
    }

	/* --- neuer Lift + Doppelmayr Lift --- */

	for(nBit = 0; nBit < 3; nBit++) {
        if(ptMc->aiSoll21[nBit] == 1) {
            setbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[1], nBit);
        } else {
            clrbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[1], nBit);
        }
    }

	if(ptMc->aiSoll2[7] != 0) {
		setbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], 7);
	} else {
		clrbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], 7);
	}
	
	for(nBit = 0; nBit < 4; nBit++) {
        if(ptMc->aiSollPos[nBit] != 0) {
            setbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_posstart, nBit);
        } else {
            clrbit(&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_posstart, nBit);
        }
    }

	/* --- Vorzone 6 --- */

	for (nBit = 0; nBit < 2; nBit++) {
        if(ptMc->aiSoll6[nBit] == 1) {
            setbit(&C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_soll_start[0], nBit);
        } else {
            clrbit(&C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_soll_start[0], nBit);
        }
    }

	/* --- Bewegungsrichtungen --- */

	C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_ausein[0].AusSoll  = ptMc->iAusSollC4;
	C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[0].AusSoll = ptMc->iAusSoll48;
	C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[1].AusSoll = ptMc->iAusSollE0;
	C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[2].AusSoll = ptMc->iAusSollE5;

	/* --- Wickler --- */

	C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_wickler.AusSoll = ptMc->iWicklerSoll;

	/* --- RBG --- */

	for(nBit = 0; nBit < 4; nBit++) {
		C->rbg_ctxt[nBit].rc_stop.soll= ptMc->aiRbgSperreSoll[nBit];
	}

	/* --- Lifte --- */

	if (ptMc->iLift5354 == 1){
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_5354] = 1;
	} else {
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_5354] = 0;
	}

	if (ptMc->iLiftE3E4 == 1){
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_E3E4] = 1;
	} else {
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_E3E4] = 0;
	}

	if (ptMc->iLift6364 == 1){
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_6364] = 1;
	} else {
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_6364] = 0;
	}

	/* --- Ein/Auslagerbahnen --- */

	if (ptMc->iSperre_E_A_5556 == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5556] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5556] = 0;
	}

	if (ptMc->iSperre_E_A_5E5F == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5E5F] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5E5F] = 0;
	}

	if (ptMc->iSperre_E_A_E5E7 == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E5E7] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E5E7] = 0;
	}

	if (ptMc->iSperre_E_A_E1E2 == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E1E2] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E1E2] = 0;
	}

	if (ptMc->iSperre_E_A_E9EB == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E9EB] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E9EB] = 0;
	}

	if (ptMc->iSperre_E_A_ECED == 1) {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_ECED] = 1;
	} else {
		C->vzs_ctxt[2].vd.vd_einaus[EINAUS_ECED] = 0;
	}

	/* --- Verteilung 6.OG --- */

	for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
		C->verteilung[nI].Sperre = ptMc->iVerteilSperre[nI];
	}

	for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
		C->verteilung[nI].SollPalAnz = ptMc->iSollPalAnz[nI];
	}

	/* --- Staubahnen 6.OG ---*/

	for (nI = 0; nI < MAX_UMSCHALT; nI++) {
		C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_stauen[nI].AusSoll =
			ptMc->iStauSoll[nI];
	}

	for (nI = 0; nI < MAX_SPERREN_6OG; nI++) {
		C->sperren6OG[nI] = ptMc->iSperre6OG[nI];
	}

	/* --- Qvw 70/71 --- */

	C->PosSperren[0] = ptMc->iSperreQvw7071;

	/* --- VZ6-17 --- */

	C->PosSperren[1] = ptMc->iSperrePos17;

	/* Wochenendbetrieb 6. OG */

	C->PosSperren[2] = ptMc->iSperrePos1316;

	/* --- Staubahnen VZS2B ---*/

	for (nI = 0; nI < MAX_UMSCHALT; nI++) {
		C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_stauen[nI].AusSoll = 
			ptMc->iStauSollVzs2B[nI];
	}

	C->reserve[1] = ptMc->iNachAnz;
	C->reserve[2] = ptMc->iStopScan50;
	C->reserve[3] = ptMc->iStopScanE0;

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Liest den Zustand der einzelnen Anlagenteile aus den Globalen
-* RETURNS
-* 	void Funktion !!! -> kein Returnwert
-*--------------------------------------------------------------------------*/
static void ReadGlobDataVZS (MY_CTX *ptMc) 
{
	int 	nBit, nI;

	for(nBit = 0; nBit < 3; nBit++) {
		if(isset (&C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_soll_start[0], nBit)) {
			ptMc->aiSoll1[nBit] = 1; 
		} else {
			ptMc->aiSoll1[nBit] = 0;
		}
		if(isset (&C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_ist_start[0], nBit)) {
			ptMc->aiIst1[nBit] = 1;
		} else {
			ptMc->aiIst1[nBit] = 0;
		}
	}

	for(nBit = 3; nBit < 7; nBit++) {
        if(isset (&C->vzs_ctxt[TMNR_VZS_VZS2A].vd.vd_soll_start[0], nBit)) {
			ptMc->aiSoll2[nBit] = 1; 
		} else {
			ptMc->aiSoll2[nBit] = 0;
		}
        if(isset (&C->vzs_ctxt[TMNR_VZS_VZS2A].vd.vd_ist_start[0], nBit)) {
			ptMc->aiIst2[nBit] = 1;
		} else {
			ptMc->aiIst2[nBit] = 0;
		}
    }

	for (nBit = 0; nBit < 3; nBit++) {
        if (isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], nBit)) {
			ptMc->aiSoll2[nBit] = 1; 
		} else {
			ptMc->aiSoll2[nBit] = 0;
		}
        if (isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ist_start[0], nBit)) {
			ptMc->aiIst2[nBit] = 1;
		} else {
			ptMc->aiIst2[nBit] = 0;
		}
    }

	if(isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[0], 7)) {
		ptMc->aiSoll2[7] = 1; 
	} else {
		ptMc->aiSoll2[7] = 0;
	}

	if(isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ist_start[0], 7)) {
		ptMc->aiIst2[7] = 1;
	} else {
		ptMc->aiIst2[7] = 0;
	}
	
	/* --- neuer Lift + Doppelmayr Lift --- */

	for (nBit = 0; nBit < 3; nBit++) {
        if (isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_start[1], nBit)) {
			ptMc->aiSoll21[nBit] = 1; 
		} else {
			ptMc->aiSoll21[nBit] = 0;
		}
        if (isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ist_start[1], nBit)) {
			ptMc->aiIst21[nBit] = 1;
		} else {
			ptMc->aiIst21[nBit] = 0;
		}
    }

	for(nBit = 0; nBit < 4; nBit++) {
		if( isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_soll_posstart, nBit)) {
			ptMc->aiSollPos[nBit] = 1;
		} else {
			ptMc->aiSollPos[nBit] = 0;
		}
        if(isset (&C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ist_posstart, nBit)) {
			ptMc->aiIstPos[nBit] = 1;
		} else {
			ptMc->aiIstPos[nBit] = 0;
		}
    }

	/* --- Vorzone 6 --- */

	for (nBit = 0; nBit < 2; nBit++) {
        if(isset (&C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_soll_start[0], nBit)) {
			ptMc->aiSoll6[nBit] = 1; 
		} else {
			ptMc->aiSoll6[nBit] = 0;
		}
        if(isset (&C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_ist_start[0], nBit)) {
			ptMc->aiIst6[nBit] = 1;
		} else {
			ptMc->aiIst6[nBit] = 0;
		}
    }

	ptMc->iAusSollC4  	= C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_ausein[0].AusSoll;
	ptMc->iAusSoll48  	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[0].AusSoll;
	ptMc->iAusSollE0  	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[1].AusSoll;
	ptMc->iAusSollE5  	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[2].AusSoll;
	ptMc->iAusIstC4   	= C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_ausein[0].AusIst;
	ptMc->iAusIst48   	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[0].AusIst;
	ptMc->iAusIstE0   	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[1].AusIst;
	ptMc->iAusIstE5   	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[2].AusIst;
	ptMc->iUmschaltC4 	= C->vzs_ctxt[TMNR_VZS_VZS1].vd.vd_ausein[0].Erhalten;
	ptMc->iUmschalt48 	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[0].Erhalten;
	ptMc->iUmschaltE0 	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[1].Erhalten;
	ptMc->iUmschaltE5 	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_ausein[2].Erhalten;

	/* --- Wickler --- */

	ptMc->iWicklerSoll  = C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_wickler.AusSoll;
	ptMc->iWicklerIst  	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_wickler.AusIst;
	ptMc->iWicklerUm  	= C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_wickler.Erhalten;

	for (nBit = 0; nBit < 4; nBit++) {
		ptMc->aiRbgSperreSoll[nBit] = C->rbg_ctxt[nBit].rc_stop.soll;
		ptMc->aiRbgSperreIst[nBit]  = C->rbg_ctxt[nBit].rc_stop.ist;
	}

	/* --- Auftragsbearbeitung RBG --- */

	for (nBit = 0; nBit < 4; nBit++) {
		ptMc->aiRbgTaStop[nBit] = C->rbg_ctxt[nBit].rc_ab_stop;
	}

	/* --- Lifte --- */

	if (C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_5354] == 1) {
		ptMc->iLift5354 = 1;
	} else {
		ptMc->iLift5354 = 0;
	}

	if (C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_E3E4] == 1) {
		ptMc->iLiftE3E4 = 1;
	} else {
		ptMc->iLiftE3E4 = 0;
	}

	if (C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_lift[LIFT_6364] == 1) {
		ptMc->iLift6364 = 1;
	} else {
		ptMc->iLift6364 = 0;
	}

	/* --- Ein/Auslagerbahnen --- */

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5556] == 1) {
		ptMc->iSperre_E_A_5556 = 1;
	} else {
		ptMc->iSperre_E_A_5556 = 0;
	}

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_5E5F] == 1) {
		ptMc->iSperre_E_A_5E5F = 1;
	} else {
		ptMc->iSperre_E_A_5E5F = 0;
	}

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E5E7] == 1) {
		ptMc->iSperre_E_A_E5E7 = 1;
	} else {
		ptMc->iSperre_E_A_E5E7 = 0;
	}

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E1E2] == 1) {
		ptMc->iSperre_E_A_E1E2 = 1;
	} else {
		ptMc->iSperre_E_A_E1E2 = 0;
	}

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_E9EB] == 1) {
		ptMc->iSperre_E_A_E9EB = 1;
	} else {
		ptMc->iSperre_E_A_E9EB = 0;
	}

	if (C->vzs_ctxt[2].vd.vd_einaus[EINAUS_ECED] == 1) {
		ptMc->iSperre_E_A_ECED = 1;
	} else {
		ptMc->iSperre_E_A_ECED = 0;
	}

	/* --- Verteilung 6.OG --- */

	for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
		ptMc->iVerteilSperre[nI] = C->verteilung[nI].Sperre;
	}

	for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
		ptMc->iSollPalAnz[nI] = C->verteilung[nI].SollPalAnz;
	}

	for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
		ptMc->iIstPalAnz[nI] = C->verteilung[nI].IstPalAnz;
	}

	/* --- Staubahnen 6.OG ---*/

	for (nI = 0; nI < MAX_UMSCHALT; nI++) {
		ptMc->iStauSoll[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_stauen[nI].AusSoll;

		ptMc->iStauIst[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_stauen[nI].AusIst;

		ptMc->iStauUmschalt[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS6].vd.vd_stauen[nI].Erhalten;
	}

	for (nI = 0; nI < MAX_SPERREN_6OG; nI++) {
			ptMc->iSperre6OG[nI] = C->sperren6OG[nI];
	}

	/* --- Qvw 70/71 --- */

	ptMc->iSperreQvw7071 = C->PosSperren[0];

	/* --- VZ6-17 --- */

	ptMc->iSperrePos17 = C->PosSperren[1];

	/* Wochenendbetrieb 6. OG */

	ptMc->iSperrePos1316 = C->PosSperren[2];

	/* --- Staubahnen VZS2B ---*/

	for (nI = 0; nI < MAX_UMSCHALT; nI++) {
		ptMc->iStauSollVzs2B[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_stauen[nI].AusSoll;

		ptMc->iStauIstVzs2B[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_stauen[nI].AusIst;

		ptMc->iStauUmschaltVzs2B[nI] = 
			C->vzs_ctxt[TMNR_VZS_VZS2B].vd.vd_stauen[nI].Erhalten;
	}

	ptMc->iNachAnz = C->reserve[1];
	ptMc->iStopScan50 = C->reserve[2];
	ptMc->iStopScanE0 = C->reserve[3];

	return;	
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static void cb_cancel(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                        int iReason, void *pvCbc)
{
	MskCbClose(hMaskRl, hEf, hEfRl, iReason, pvCbc);

    return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static void cb_akt (MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
				int iReason, void *pvCbc) 
{
	MY_CTX 	*ptMc, tMc;

	switch (iReason) {
	case FCB_XF:
		ptMc = (MY_CTX *)MskRlMaskGet(hMaskRl, MskNmaskCalldata);

		memset (&tMc, 0, sizeof (tMc));

		ReadGlobDataVZS (&tMc);

		MskUpdateMaskVar(hMaskRl);

		memcpy (ptMc, &tMc, sizeof (MY_CTX));
		memcpy (&tMcCheck, &tMc, sizeof (MY_CTX));

		break;

	default:
		break;
	}

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static void cb_ueb(MskDialog hMaskRl, MskStatic hEf, MskElement hEfRl,
                   int iReason, void *pvCbc) {

	MY_CTX 	*ptMc, tMc;
	char	acAbfrage[ABFRAGE_LEN+1];
	int 	iRv, nI;

	switch (iReason) {
	case FCB_XF:
		memset (&tMc, 0, sizeof (tMc));

		ReadGlobDataVZS (&tMc);

		if (memcmp (&tMcCheck, &tMc, sizeof (MY_CTX)) != 0) {
			GrBell ();
			WamasBoxAlert (SHELL_OF (hMaskRl),
				WboxNtext, "Daten wurden in der Zwischenzeit geaendert !\n"
							"Aktualisieren und erneut Aenderung druchfuehren.",
				NULL);
			break;
		}
		
		ptMc = (MY_CTX *) MskRlMaskGet(hMaskRl, MskNmaskCalldata);
		MskTransferMaskDup(hMaskRl);

		iRv = WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_WARN,
				WboxNbutton,    WboxbNok,
				WboxNbuttonRv,  1,
				WboxNbutton,    WboxbNcancel,
				WboxNbuttonRv,  0,
				WboxNmwmTitle,  "BZ-Setzen",
				WboxNtext,  	"Betriebszustand aendern ?",
				NULL);

		if(iRv != TRUE) {
			break;
		}

		/* --- EinAuslagern umschalten f�r Benco Halle --- */

		if (ptMc->iSperreQvw7071 == 1) {

			GrBell ();

			iRv = WamasBox(SHELL_OF(hMaskRl),
					WboxNboxType,   WBOX_WARN,
					WboxNbutton,    WboxbNok,
					WboxNbuttonRv,  1,
					WboxNbutton,    WboxbNcancel,
					WboxNbuttonRv,  0,
					WboxNmwmTitle,  "BZ-Setzen",
					WboxNtext,  "Sind Sie sicher, da� Sie die\n"
								"Bewegungsrichtung umschalten wollen ?\n"
								"Alle Aus - Einlagerungen erfolgen �ber\n"
								"die BENCO - Halle !", 
					NULL);

			if (iRv != TRUE) {
				return;
			}

			/* --- Sicherheitsabfrage --- */

			memset (acAbfrage, 0, sizeof (acAbfrage));

			iRv = me_abfrage (SHELL_OF (hMaskRl), acAbfrage);

			if (iRv != IS_Ok) {
				return;
			}

			LogPrintf (FAC_ME_BZSET, LT_ALERT, 
				"Benco Halle Aus/Einlagerung aktiv von %s",
				GetUserOrTaskName ());
		}

		/* Einzelne Auslagerpositionen Sperren */

		iRv = DoPosSperre(hMaskRl);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "BZ-Setzen - Fehler",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		/* Lift Sperren */

		iRv = DoLiftSperre(hMaskRl, LIFT_5354);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler LIFT_5354",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoLiftSperre(hMaskRl, LIFT_E3E4);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler LIFT_E3E4",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoLiftSperre(hMaskRl, LIFT_6364);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler LIFT_6364",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		/* --- Ein/Auslagerbahnen sperren --- */

		iRv = DoEinAusSperre(hMaskRl, EINAUS_5556);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_5556",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, EINAUS_5E5F);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_5E5F",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, EINAUS_E5E7);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_E5E7",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, EINAUS_E1E2);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_E1E2",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, EINAUS_E9EB);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_E9EB",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, EINAUS_ECED);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler EINAUS_ECED",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, QVW_7071);

		if (iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler QVW_7071",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, POS_VZ6_17);

		if (iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler POS_VZ6_17",
				WboxNtext,	GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, POS_VZ6_1316);

		if (iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler Umschalten Wochenendbetrieb",
				WboxNtext,	GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, OG6_STAUBAHN_07);

		if (iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler OG6_STAUBAHN_07",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, OG6_STAUBAHN_08);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler OG6_STAUBAHN_08",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}

		iRv = DoEinAusSperre(hMaskRl, OG6_STAUBAHN_59);

		if(iRv != 1) {
			TSqlRollback(hMaskRl);
			WamasBox(SHELL_OF(hMaskRl),
				WboxNboxType,   WBOX_ALERT,
				WboxNbutton,    WboxbNok,
				WboxNmwmTitle,  "Fehler OG6_STAUBAHN_59",
				WboxNtext,GetErrmsg1(),
				NULL );
			return;
		}
	
		TSqlCommit(hMaskRl);

		WriteGlobDataVZS (hMaskRl);

		LogPrintf (FAC_ME_BZSET, LT_DEBUG, 
			"Betriebszustand von %s ge�ndert",
			GetUserOrTaskName ());

		/* 
		 * Start und Stop-Telegramme der einzelene VZS
		 * vom TR Senden lassen 
		 */

		CF_TRIGGER (PROC_TR_VZS1,  	FUNC_TR_VZS_SS, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS2A, 	FUNC_TR_VZS_SS, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS2B, 	FUNC_TR_VZS_SS, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS6, 	FUNC_TR_VZS_SS, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS2B, 	FUNC_TR_VZS_TP, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS6, 	FUNC_TR_VZS_TP, 0, "me_bzset");

		CF_TRIGGER (PROC_TR_VZS1,  	FUNC_TR_VZS_BA, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS2A, 	FUNC_TR_VZS_BA, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS2B, 	FUNC_TR_VZS_BA, 0, "me_bzset");
		CF_TRIGGER (PROC_TR_VZS6, 	FUNC_TR_VZS_BA, 0, "me_bzset");

		CF_TRIGGER (PROC_TAM, FUNC_OPT_LSM, 0, "me_bzset");

		/* --- Auftragsstop f�r RBG --- */

		for (nI = 0; nI < 4; nI++) {
			C->rbg_ctxt[nI].rc_ab_stop = ptMc->aiRbgTaStop[nI];
		}

		/* --- Zeitstempel f�r Routing --- */

		for (nI = 0; nI < NO_ELE_CHTIME_IDX; nI++) {
			C->tChangeTime[nI].zChangeTime = time ((time_t *)0);
		}

		break;

	default:
		break;
    }

	return;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
static int cb_mask(MskDialog hMaskRl, int iReason)
{
    int     iRv=1,iKind;
    MY_CTX  *ptMc, tMc;
	int		nI;

    switch (iReason) {
    case MSK_PF:
        WdgGuiSet (GuiNactiveShell, (Value)SHELL_OF(hMaskRl));
        ptMc = (MY_CTX *) MskRlMaskGet (hMaskRl, MskNmaskCalldata);
        break;

    case MSK_CM:
        ptMc = malloc(sizeof(MY_CTX));
        if (ptMc == NULL) {
            iRv = MSK_OK_NOUPDATE;
        }

        memset(ptMc, 0, sizeof(MY_CTX));

        MskRlMaskSet (hMaskRl, MskNmaskCalldata, (Value)ptMc);

        iKind = MskDialogGet(hMaskRl, MskNmaskStatus);

        MskMatchEvaluate(hMaskRl, "*", iKind, MSK_MATCH_NODEF_ATTR);

        MskVaAssignMatch( hMaskRl, "*",
            MskNvariableStruct, (Value)&ptMc,
            NULL);

		for(nI = 0; nI < 3; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_1_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiSoll1[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}
		
		for(nI = 0; nI < 8; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_2_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiSoll2[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for (nI = 0; nI < 3; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_2_c"),
				MskNkey,    	(Value)nI+1+8,
				MskNvariable,   (Value)&ptMc->aiSoll21[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		/* --- Vorzone 6 --- */

		for (nI = 0; nI < 2; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_6_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiSoll6[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for(nI = 0; nI < 3; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_1_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiIst1[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for(nI = 0; nI < 8; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_2_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiIst2[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for(nI = 0; nI < 3; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_2_c"),
				MskNkey,    	(Value)nI+1+8,
				MskNvariable,   (Value)&ptMc->aiIst21[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for(nI = 0; nI < 4; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_Pos_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiIstPos[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for(nI = 0; nI < 4; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_Pos_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiSollPos[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		/* --- Vorzone 6 --- */

		for (nI = 0; nI < 2; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_6_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiIst6[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		MskVaAssign(hMaskRl, MskGetElement("Aus_Soll_c"),
			MskNkey, 	(Value)1,
			MskNvariable, 	(Value)&ptMc->iAusSollC4,
			MskNupdate, (Value)1,
			NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Aus_Soll_c"),
			MskNkey,	(Value)2,
			MskNvariable,	(Value)&ptMc->iAusSoll48,
			MskNupdate,	(Value)1,
			NULL);
	
		MskVaAssign(hMaskRl, MskGetElement("Aus_Soll_c"),
			MskNkey,	(Value)3,
			MskNvariable,	(Value)&ptMc->iAusSollE0,
			MskNupdate,	(Value)1,
			NULL);
	
		MskVaAssign(hMaskRl, MskGetElement("Aus_Soll_c"),
			MskNkey,	(Value)4,
			MskNvariable,	(Value)&ptMc->iAusSollE5,
			MskNupdate,	(Value)1,
			NULL);
	
		MskVaAssign(hMaskRl, MskGetElement("Aus_Soll_c"),
			MskNkey,	(Value)5,
			MskNvariable,	(Value)&ptMc->iWicklerSoll,
			MskNupdate,	(Value)1,
			NULL);
	
		MskVaAssign(hMaskRl, MskGetElement("Aus_Ist_c"),
			MskNkey, 	(Value)1,
			MskNvariable,	(Value)&ptMc->iAusIstC4,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Aus_Ist_c"),
			MskNkey,	(Value)2,
			MskNvariable, 	(Value)&ptMc->iAusIst48,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Aus_Ist_c"),
			MskNkey,	(Value)3,
			MskNvariable, 	(Value)&ptMc->iAusIstE0,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Aus_Ist_c"),
			MskNkey,	(Value)4,
			MskNvariable, 	(Value)&ptMc->iAusIstE5,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Aus_Ist_c"),
			MskNkey,		(Value)5,
			MskNvariable, 	(Value)&ptMc->iWicklerIst,
			MskNupdate,		(Value)1,
			NULL);

		/* --- Staurollenbahn 6.Og --- */

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Stau_Ist_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauIst[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Stau_Soll_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauSoll[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Stau_Umschalt_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauUmschalt[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		MskVaAssign(hMaskRl, MskGetElement("Umschalt_c"),
			MskNkey,	(Value)1,
			MskNvariable,	(Value)&ptMc->iUmschaltC4,
			MskNupdate,	(Value)1,
			NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Umschalt_c"),
			MskNkey,	(Value)2,
			MskNvariable, 	(Value)&ptMc->iUmschalt48,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Umschalt_c"),
			MskNkey,	(Value)3,
			MskNvariable, 	(Value)&ptMc->iUmschaltE0,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Umschalt_c"),
			MskNkey,	(Value)4,
			MskNvariable, 	(Value)&ptMc->iUmschaltE5,
			MskNupdate,	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Umschalt_c"),
			MskNkey,	(Value)5,
			MskNvariable, 	(Value)&ptMc->iWicklerUm,
			MskNupdate,	(Value)1,
			NULL);

		for(nI = 0; nI < 4; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Soll_RbgSperre_c"),
				MskNkey,    (Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiRbgSperreSoll[nI],
				MskNupdate, (Value)1,
				NULL);
		}

		for(nI = 0; nI < 4; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("Ist_RbgSperre_c"),
				MskNkey,    (Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiRbgSperreIst[nI],
				MskNupdate, (Value)1,
				NULL);
		}
		
		for (nI = 0; nI < 4; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("TaStop_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->aiRbgTaStop[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		MskVaAssign(hMaskRl, MskGetElement("Sperre_Lift_c"),
                MskNkey,        (Value)1,
                MskNvariable,   (Value)&ptMc->iLift5354,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_Lift_c"),
                MskNkey,        (Value)2,
                MskNvariable,   (Value)&ptMc->iLiftE3E4,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_Lift_c"),
                MskNkey,        (Value)3,
                MskNvariable,   (Value)&ptMc->iLift6364,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)1,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_5556,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)2,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_5E5F,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)3,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_E5E7,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)4,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_E1E2,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)5,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_E9EB,
                MskNupdate,     (Value)1,
                NULL);
		
		MskVaAssign(hMaskRl, MskGetElement("Sperre_EinAus_c"),
                MskNkey,        (Value)6,
                MskNvariable,   (Value)&ptMc->iSperre_E_A_ECED,
                MskNupdate,     (Value)1,
                NULL);
		
		/* --- Verteilung 6.OG --- */

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz1"),
			MskNkey,    	(Value)1,
			MskNvariable,   (Value)&ptMc->iSollPalAnz[0],
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz1"),
			MskNkey,    	(Value)2,
			MskNvariable,   (Value)&ptMc->iSollPalAnz[1],
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz2"),
			MskNkey,    	(Value)3,
			MskNvariable,   (Value)&ptMc->iSollPalAnz[2],
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz1"),
			MskNkey,    	(Value)4,
			MskNvariable,   (Value)&ptMc->iIstPalAnz[0],
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz1"),
			MskNkey,    	(Value)5,
			MskNvariable,   (Value)&ptMc->iIstPalAnz[1],
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_PalAnz2"),
			MskNkey,    	(Value)6,
			MskNvariable,   (Value)&ptMc->iIstPalAnz[2],
			MskNupdate, 	(Value)1,
			NULL);

		for (nI = 0; nI < MAX_VERTEILARTEN; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("InAktiv6_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->iVerteilSperre[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		for (nI = 0; nI < MAX_SPERREN_6OG; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("SperreStaubahn_c"),
				MskNkey,    	(Value)nI+1,
				MskNvariable,   (Value)&ptMc->iSperre6OG[nI],
				MskNupdate, 	(Value)1,
				NULL);
		}

		MskVaAssign(hMaskRl, MskGetElement("Sperre_Qvw_c"),
			MskNvariable,   (Value)&ptMc->iSperreQvw7071,
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Sperre_Pos17_c"),
			MskNvariable,   (Value)&ptMc->iSperrePos17,
			MskNupdate, 	(Value)1,
			NULL);

		MskVaAssign(hMaskRl, MskGetElement("Sperre_Pos1316_c"),
			MskNvariable,   (Value)&ptMc->iSperrePos1316,
			MskNupdate, 	(Value)1,
			NULL);

		/* --- Staurollenbahn VZS2B --- */

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("VZS2B_Stau_Ist_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauIstVzs2B[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("VZS2B_Stau_Soll_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauSollVzs2B[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		for (nI = 0; nI < MAX_UMSCHALT; nI++) {
			MskVaAssign(hMaskRl, MskGetElement("VZS2B_Stau_Umschalt_c"),
				MskNkey,		(Value)nI+1,
				MskNvariable, 	(Value)&ptMc->iStauUmschaltVzs2B[nI],
				MskNupdate,		(Value)1,
				NULL);
		}

		/* Nachschubanzahl */

		MskVaAssign(hMaskRl, MskGetElement("Ef_bzset_NachAnz"),
                MskNkey,        (Value)KEY_DEF,
                MskNvariable,   (Value)&ptMc->iNachAnz,
                MskNupdate,     (Value)1,
                NULL);

		/* Scanner deaktivieren */

		MskVaAssign(hMaskRl, MskGetElement("Bzset_StopScan50_c"),
                MskNkey,        (Value)KEY_DEF,
                MskNvariable,   (Value)&ptMc->iStopScan50,
                MskNupdate,     (Value)1,
                NULL);

		 MskVaAssign(hMaskRl, MskGetElement("Bzset_StopScanE0_c"),
                MskNkey,        (Value)KEY_DEF,
                MskNvariable,   (Value)&ptMc->iStopScanE0,
                MskNupdate,     (Value)1,
                NULL);

        EfcbmInstByName (hMaskRl, "Cancel_F", KEY_DEF, cb_cancel, NULL);
        EfcbmInstByName (hMaskRl, "Akt_b", KEY_DEF, cb_akt, NULL);
        EfcbmInstByName (hMaskRl, "Uebernehmen_b", KEY_DEF, cb_ueb, NULL);

		memset (&tMc, 0, sizeof (tMc));

		ReadGlobDataVZS (&tMc);
		
		memcpy (ptMc, &tMc, sizeof (MY_CTX));
		memcpy (&tMcCheck, &tMc, sizeof (MY_CTX));
		
		MskUpdateMaskVar (hMaskRl);	
        break;

    case MSK_DM:
		TSqlRollback(hMaskRl);
        ptMc = (MY_CTX *) MskRlMaskGet (hMaskRl, MskNmaskCalldata);

        if( ptMc != NULL) {
            free (ptMc);
        }

        break;
    default:

        break;
    }

    return iRv;
}

/* ===========================================================================
 * GLOBAL (PUBLIC) Functions
 * =========================================================================*/
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
int _me_bzset( OWidget hW) {

    static MskDialog        hMaskRl=NULL;
    OWidget                 hM_sh;

    OwrcLoadObject(ApNconfigFile, _ME_NAME_RC);

    hMaskRl = MskOpenMask(NULL, _RC_NAME);
    if (hMaskRl == NULL) {
        return RETURN_ERROR;
    }

    if ((hM_sh = SHELL_OF(hMaskRl)) == NULL) {
        hM_sh = ApShellModelessCreate(hW, AP_CENTER, AP_CENTER);

        MskRlMaskSet(hMaskRl,MskNmaskCallback,(Value)cb_mask);
    }

    MskCreateDialog(hM_sh, hMaskRl, "Betriebszustand setzen",
                    NULL, HSL_NI, SMB_All);

    return RETURN_ACCEPTED;
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-* RETURNS
-*--------------------------------------------------------------------------*/
MENUFCT me_bzset (MskDialog hMaskRl, MskStatic hEf, MskElement hEl,
                  int iReason, void *pvCbc)
{
    switch(iReason) {
	case FCB_XF:
		_me_bzset(GetRootShell());
		break;

	default:
		break;
    }
}


/*******************************************************************************
-* Sicherheitsabfrage
-******************************************************************************/


typedef struct _MY_CD {
	char 	acAbfrage[ABFRAGE_LEN+1];
} MY_CD;

static MskTtext ef_abfrage = {
    {EK_EF_TEXT,},
    EFT_STRG,
    "Sicherheitsabfrage",
    ABFRAGE_LEN,
    ABFRAGE_LEN,
};


static MskTmaskDescr mask_abfrage[] = {
    {&ef_abfrage},
    {NULL},
};

static int cbm_abfrage (MskTmaskRlPtr mask_rl, int reason)
{
    MskTgenericRl   *rl;
    MY_CD    		*pcon;
    char            *abfrage;
    int             rv = MSK_OK_TRANSFER;

    switch (reason) {
	case MSK_OK:
		pcon = (MY_CD *)MskRlMaskGet(mask_rl, MskNmaskCalldata);
		rv = MSK_OK_TRANSFER;

		rl = MskQueryRl(mask_rl,  &ef_abfrage,  KEY_DEF);

		abfrage = (char *)rl->ec_rl.duplicate;

		if ((abfrage[0] == ' ') || (strlen (abfrage) == 0 )) {
			GrBell ();
			ApBoxAlert (SHELL_OF (mask_rl), HSL_NI,
				"Bitte einen Sicherheitspa�wort eingeben !");

			rv = MSK_OK_UPDATEDUP;
			break;
		}

		if (strcmp (abfrage, "Auslagerung und Einlagerung BENCO") != 0) {
			GrBell ();
			ApBoxAlert (SHELL_OF (mask_rl), HSL_NI,
				"Falsches Sicherheitspa�wort !");

			rv = MSK_OK_UPDATEDUP;
			break;
		}

		strcpy (pcon->acAbfrage, abfrage);

		rv = MSK_OK_LEAVE;

		break;

	default :
		break;
    }

    return rv;
}

/*
-* Funktion: Filename
-*/

static int me_abfrage (OWidget w, char *pacAbfrage)
{
    MskTmaskRlPtr    mask_rl;
    OWidget          shell;
    MY_CD            con;
    int              rv = IS_Ok;

    memset (&con, 0, sizeof (con));

    shell   = ApShellModalCreate (w, AP_CENTER, AP_CENTER);

    mask_rl = MskOpenMask (mask_abfrage, "");

    MskRlMaskSet (mask_rl, MskNmaskCalldata, (Value)&con);
    MskRlMaskSet (mask_rl, MskNmaskCallback, (Value )cbm_abfrage);

    MskCreateDialog (shell, mask_rl, "Sicherheitsabfrage", NULL, HSL_NI, 
		SMB_Ignore);

    rv = WdgMainLoop ();

    strcpy ((char *)pacAbfrage, con.acAbfrage);

    return (rv);
}
