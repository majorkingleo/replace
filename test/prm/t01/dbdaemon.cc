/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#ifdef WIN32
#include <windows.h>
#endif
#include <time.h>
#include <stdio.h>

/* ------- Owil-Headers --------------------------------------------------- */

/* ------- Tools-Headers -------------------------------------------------- */
#include <dbsql.h>
#include <dbsqlstd.h>
#include <logtool2.h>
#include <sqlkey.h>
#include <cycle.h>
#include <tsleep.h>
#include <ts.h>
#include <hist_util.h>
#include <elements.h>

#include <errmsg.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "global.h"
#include "vglobal.h"
#include "proc.h"
#include "facility.h"
#include "aus.h"
#include "lvs_client_fct.h"
#include "inv_util.h"
#include "verd_util.h"
#include "aus_util.h"
#include "te_util.h"
#include "pos_util.h"
#include "tpa_util.h"
#include "pbl_mgr_util.h"
#include "umvpl.h"
#include "rfco_util.h"
#include "parameter.h"
#include "tstdrvauab.h"
#include "sim_visi.h"
#include "init_app.h"
#include "lsmp_util.h"
#include "lsmp.h"
#include <fac_def.h>

#define _DBDAEMON_C
#include "dbdaemon.h"
#undef _DBDAEMON_C

/******* NEXT PACKAGEHEADERS *******/


/* ===========================================================================
 * LOCAL DEFINES AND MACROS
 * =========================================================================*/

#define		BLOCKSIZE		500
#define 	BLOCKSIZE_AUSK	(100)

/* ===========================================================================
 * LOCAL TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

typedef struct _S_BlockedPlace {
    int     iGlobIdx;
    char    acFspCheckPlace[FELDID_LEN+1];
} S_BlockedPlace;

/* ===========================================================================
 * LOCAL (STATIC) Variables and Function-Prototypes
 * =========================================================================*/

 /* ------- Variables ----------------------------------------------------- */

 /* ------- Function-Prototypes ------------------------------------------- */

static S_BlockedPlace matUebPlace [] = {
    {UEBPLACE_WAP_01, FELDID_WAP_UEB_01},
    {UEBPLACE_WAP_02, FELDID_WAP_UEB_02},
    {UEBPLACE_WAP_03, FELDID_WAP_UEB_03},
    {UEBPLACE_WAP_04, FELDID_WAP_UEB_04},
    {UEBPLACE_WAP_05, FELDID_WAP_UEB_05},
    {UEBPLACE_WAP_06, FELDID_WAP_UEB_06},
    {UEBPLACE_WAP_07, FELDID_WAP_UEB_07},
    {UEBPLACE_WAP_08, FELDID_WAP_UEB_08},
    {UEBPLACE_WAP_09, FELDID_WAP_NRP_01},
    {UEBPLACE_WAP_10, FELDID_WAP_NRP_02},
    {UEBPLACE_WAP_11, FELDID_WAP_NRP_03},
    {UEBPLACE_WAP_12, FELDID_WAP_NRP_04},
    {UEBPLACE_WAP_13, FELDID_WAP_NRP_05},
    {UEBPLACE_WAP_14, FELDID_WAP_NRP_06},
    {UEBPLACE_ZTL_01, FELDID_ZTL_UEB_01},
    {UEBPLACE_ZTL_02, FELDID_ZTL_UEB_02},
    {UEBPLACE_ZTL_03, FELDID_ZTL_UEB_03},
    {UEBPLACE_ZTL_04, FELDID_ZTL_UEB_04},
};



/* ===========================================================================
 * GLOBAL VARIABLES
 * =========================================================================*/

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*		static int Del_Inv (void	*pvCalldata, ...
-*
-* DESCRIPTION
-* Alles INVK bzw. INVP mit Status='FERTIG' und HostStatus='FERTIG' sollen
-* gelï¿½scht und protokolliert werden
-*
-* RETURNS
-*		-1 ... Error
-*		 1 ... Ok
-*		 0 ... Keine Daten gefunden
-*--------------------------------------------------------------------------*/
static int Check_Inv (void			*pvCalldata,
				   		  	 long			lTrigger,
				   		  	 char			*pcMessage,
				   		  	 CYCLEFUNCT		*ptCf,
				   		  	 CYCLETIME		*ptCt)
{
	int		iRv	= 0;
	long	lAnzInv	= 0;
	long	lAnzInvMeld	= 0;
	LSMP    tLsmp;
	time_t  zNow = time(0);;
	void	*pvTid = NULL;

	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "Check_Inv: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Check Inv" );

	memset(&tLsmp, 0, sizeof(tLsmp));

	iRv = TExecSql(pvTid,
		"SELECT COUNT(*) FROM INVK WHERE INVK.Status = 'AKTIV'",
		SELLONG(lAnzInv),
		NULL);

	if (iRv <= 0 && TSqlError (pvTid) != SqlNotFound)
	{  
		LogPrintf( FAC_DBSDAEMON, LT_ALERT,
		"Check_Inv: ERROR SELECT COUNT  SqlError=%ld  SqlErrTxt:%s",
		TSqlError(pvTid), TSqlErrTxt(pvTid ));
	}

	iRv = PrmGet1Parameter (pvTid, P_ANZ_INV_MELD, 
							PRM_CACHE, &lAnzInvMeld);

	if (iRv < 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT, "Check_Inv: PrmGet1Parameter %s failed", 
			P_ANZ_INV_MELD);
	}

	if (lAnzInvMeld <= 0 || lAnzInv < lAnzInvMeld) {

		if (lAnzInv == 0) {
			iRv = TExecSql (pvTid,
					"DELETE from LSMP "
					"WHERE LSMP.komment1 like '%%aktive Inventur%%' "
					"and LSMP.QUITTIERT=0 ",
					NULL);

			if (iRv < 0)
			{  
				LogPrintf( FAC_DBSDAEMON, LT_ALERT,
				"Check_Inv: ERROR DELETE LSMP SqlError=%ld  SqlErrTxt:%s",
				TSqlError(pvTid), TSqlErrTxt(pvTid ));

				TSqlRollback (pvTid);
			}
			else {
				LogPrintf( FAC_DBSDAEMON, LT_DEBUG, 
					"Check_Inv: %d LSMPs DELETED",
					iRv);

				TSqlCommit (pvTid);
			}
		}

		LogPrintf( FAC_DBSDAEMON, LT_DEBUG, 
			"=> End: Check Inv => %s=%ld lAnzInv=%ld nothing found",
			P_ANZ_INV_MELD, lAnzInvMeld, lAnzInv);

		return (0);
	}

	/*
	 * Gibt's diese Meldung schon?
	 * 'CNT' inkrementieren bzw.
	 * neuen Datensatz anlegen.
	 */
	iRv = TExecSql (pvTid,
				"SELECT "
				"LSMP.protnr from LSMP "
				"WHERE LSMP.komment1 like '%%aktive Inventur%%' "
				"and LSMP.QUITTIERT=0 "
				"for update nowait ",
				SELLONG(tLsmp.lsmpProtNr),
				NULL);

	if (iRv <= 0 && TSqlError (pvTid) != SqlNotFound)
	{  
		LogPrintf( FAC_DBSDAEMON, LT_ALERT,
		"Check_Inv: ERROR SELECT LSMP SqlError=%ld  SqlErrTxt:%s",
		TSqlError(pvTid), TSqlErrTxt(pvTid ));
	}

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, 
			"Check_Inv: found LSMP with ProtNr=%ld Parameter %s=%ld lAnzInv=%ld",
			tLsmp.lsmpProtNr, P_ANZ_INV_MELD, lAnzInvMeld, lAnzInv);

	/* alter record existiert
	-*/
    if (iRv == 1) {

		LogPrintf( FAC_DBSDAEMON, LT_DEBUG, 
				"Check_Inv: update LSMP with ProtNr=%ld",
				tLsmp.lsmpProtNr);

		iRv = TExecSql (pvTid,
				"UPDATE LSMP SET "
				"LSMP.cnt=0, LSMP.Hist_AeZeit=:zeit, "
				"LSMP.Hist_AnZeit=:zeit, LSMP.Hist_AeUser=:usr "
				"where LSMP.protnr=:protnr",
				SQLTIMET(zNow),
				SQLTIMET(zNow),
				SQLSTRING(GetUserOrTaskName()),
				SQLLONG(tLsmp.lsmpProtNr),
				NULL);

		if ( iRv <= 0 )
		{  
			LogPrintf( FAC_DBSDAEMON, LT_ALERT,
			"Check_Inv: ERROR UPDATE LSMP SqlError=%ld  SqlErrTxt:%s",
			TSqlError(pvTid), TSqlErrTxt(pvTid ));
		}

    } else {
		LogPrintf( FAC_DBSDAEMON, LT_DEBUG, 
				"Check_Inv: write LSM  '%d aktive Inventur(en) !'",
				lAnzInv);

		LogPrintf(FAC_LSMP, LT_ALERT, "%d aktive Inventur(en) !", lAnzInv);
	}

	if ( iRv <= 0 )  /* Fehler bzw. keine Daten gefunden */
	{  
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}/** IF **/

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Check Inv" );

	return (1);
} /* Check_Inv */


/* ===========================================================================
 * LOCAL (STATIC) Functions
 * =========================================================================*/

static unsigned long get_microtime()
{
    struct timeval tv1;
    struct timezone tz1;

    gettimeofday(&tv1,&tz1);

    return (unsigned long)(tv1.tv_sec * 1000 + tv1.tv_usec / 1000) ;
}


/*---------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _DelEmptyTekTepPoolWab(void *pvTid,
-*                                   SqlContext *hSqlCtxt, char *pcFac)
-*
-* DESCRIPTION
-*  function deletes empty TEKs without TEPs on 'AP%'.
-*
-* RETURNS
-*  -1 ... error
-*   0 ... nothing done
-*   > ... number of successfully deleted transport-units
-*-------------------------------------------------------------------------*/
static int _DelEmptyTekTepPoolWab (void *pvTid, SqlContext *hSqlCtxt, const char *pcFac)
{
    long        nlDbRv = 0;

	nlDbRv = TExecSql (pvTid,
				 "DELETE FROM " TN_TEK
				 " WHERE "
				 TCN_TEK_Pos_FeldId" LIKE 'POOL-WAB%%' AND "
				 " HIST_AEZEIT < SYSDATE - 20 AND "
				 "NOT EXISTS ("
					"SELECT 1 "
					"FROM " TN_TEP
					" WHERE "  TCN_TEK_TeId" = "  TCN_TEP_TeId")"   ,
				 NULL);

	if (nlDbRv < 0 && TSqlError (pvTid) != SqlNotFound)
	{
		LogPrintf (pcFac, LT_ALERT,
			"<_DelEmptyTekTepPoolWab>\n"
			"SQL: %s", TSqlErrTxt (pvTid));
		return (-1);
	}

	LogPrintf (pcFac, LT_ALERT,
		"<_DelEmptyTekTepPoolWab>: Deleted [%ld] TEK entries",
		nlDbRv);

    return (nlDbRv);
}

/* ===========================================================================
 * GLOBAL (PUBLIC) Functions
 * =========================================================================*/

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _DelEmptyTekTepAp(void *pvTid,
-*                                   SqlContext *hSqlCtxt, char *pcFac)
-*
-* DESCRIPTION
-*  function deletes empty TEKs without TEPs on 'AP%'.
-*
-* RETURNS
-*  -1 ... error
-*   0 ... nothing done
-*   > ... number of successfully deleted transport-units
-*-------------------------------------------------------------------------*/
static int _DelEmptyTekTepAp (void *pvTid, SqlContext *hSqlCtxt, const char *pcFac)
{
    long        nlDbRv = 0;

	nlDbRv = TExecSql (pvTid,
                                 "DELETE FROM " TN_TEK
                                 " WHERE "
                                 TCN_TEK_Pos_FeldId" LIKE 'AP%%' AND "
                                 " HIST_AEZEIT < SYSDATE - 10 AND "
                                 "NOT EXISTS ("
                                        "SELECT 1 "
                                        "FROM " TN_TEP
					" WHERE "  TCN_TEK_TeId" = "  TCN_TEP_TeId")"   ,
				 NULL);

	if (nlDbRv < 0 && TSqlError (pvTid) != SqlNotFound)
	{
		LogPrintf (pcFac, LT_ALERT,
			"<_DelEmptyTekTepAp>\n"
			"SQL: %s", TSqlErrTxt (pvTid));
		return (-1);
	}

	LogPrintf (pcFac, LT_ALERT,
		"<_DelEmptyTekTepAp>: Deleted [%ld] TEK entries",
		nlDbRv);

    return (nlDbRv);
}

static int DelEmptyTekTepAp (void			*pvCalldata,
						   long			lTrigger,
						   char			*pcMessage,
						   T_CYCLEFUNCT	*ptCycleFunct,
						   T_CYCLETIME	*ptCycleTime)
{
	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "DelEmptyTekTepAp: triggered, message: %s",
                                   pcMessage == NULL ? "NO MSG" : pcMessage );
        }

        SqlContext              *hSqlCtxt = NULL;
        const char              *pcFac = FAC_DBSDAEMON;
        void                    *pvTid = NULL;
        long                    lRv = 0;

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete empty-TE on AP" );

	lRv = _DelEmptyTekTepAp(pvTid, hSqlCtxt, pcFac);

	if (lRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT, "DelEmptyTekTepAp: ROLLBACK to all");
		TSqlRollback (pvTid);	
	} else {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT, "DelEmptyTekTepAp: COMMIT to all");
		TSqlCommit (pvTid);
	}
	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete empty-TE's on AP" );

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete empty-TE on POOL-WAB" );

	lRv = _DelEmptyTekTepPoolWab(pvTid, hSqlCtxt, pcFac);

	if (lRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT, "DelEmptyTekTepPoolWab: ROLLBACK to all");
		TSqlRollback (pvTid);	
	} else {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT, "DelEmptyTekTepPoolWab: COMMIT to all");
		TSqlCommit (pvTid);
	}
	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete empty-TE's on POOL-WAB" );

	return (0);
}

/*---------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _DelEmptyTekTep(void *pvTid,
-*                                   SqlContext *hSqlCtxt, char *pcFac)
-*
-* DESCRIPTION
-*  function searches for empty TEKs (TEP with amount = 0)on KCL-EP-01 and delete them.
-*
-* RETURNS
-*  -1 ... error
-*   0 ... nothing done
-*   > ... number of successfully deleted transport-units
-*-------------------------------------------------------------------------*/
static int _DelEmptyTekTep (void *pvTid, SqlContext *hSqlCtxt, const char *pcFac)
{
    long        nlDbRv = 0, lRv = 0, nI, nlProcessed = 0;
    TEK         atTek[BLOCKSIZE];
    const char        *pcPos_KCL_EP_01  = FELDID_KCL_EP_01;

    do
    {
        memset (atTek, 0, sizeof (atTek));

        nlDbRv = nlDbRv == 0
        ? TExecSqlX (pvTid,
                     hSqlCtxt,
                     "SELECT %" TN_TEK
					 " FROM " TN_TEK
                     " WHERE "
                     TCN_TEK_Pos_FeldId" = :aktpos AND "
                     TCN_TEK_Tet_TetId" = :tetid AND "
                     "NOT EXISTS ("
						"SELECT 1 "
						"FROM " TN_TEP
						" WHERE "  TCN_TEK_TeId" = "  TCN_TEP_TeId" AND "  
						TCN_TEP_Mngs_Mng" != 0)" ,
                     BLOCKSIZE, 0,
                     SELSTRUCT  (TN_TEK, atTek[0]),
                     SQLFELDID  (pcPos_KCL_EP_01),
                     SQLTETID(TETID_P_2000),
                     NULL)
        : TExecSqlV (pvTid, hSqlCtxt, NULL, NULL, NULL, NULL);

        if (nlDbRv <= 0 && TSqlError (pvTid) != SqlNotFound)
        {
            LogPrintf (pcFac, LT_ALERT,
                "<_DelEmptyTekTep>\n"
                "SQL: %s", TSqlErrTxt (pvTid));
            return (-1);
        } else  if (nlDbRv <= 0 && TSqlError (pvTid) == SqlNotFound) {
            LogPrintf (pcFac, LT_ALERT,
                "<_DelEmptyTekTep>: No TEK on pos[%s] found",
				pcPos_KCL_EP_01);
		}

        for (nI = 0; nI < nlDbRv; nI ++)
        {
            lRv = ProtDelTe (pvTid, (char*)&atTek[nI], pcFac);
            if (lRv < 0)
            {
                return (-1);
            }

            LogPrintf (pcFac, LT_DEBUG,
                "deleted TE [%s] on position [%s]",
                atTek[nI].tekTeId,
                atTek[nI].tekPos.FeldId);

            nlProcessed ++;
        }
    }
    while (nlDbRv == BLOCKSIZE);

	LogPrintf (pcFac, LT_ALERT,
		"<_DelEmptyTekTep>: Deleted and protocolled [%ld] TEK entries",
		nlProcessed);

    return (nlProcessed);
}

static int DelEmptyTekTep (void			*pvCalldata,
						   long			lTrigger,
						   char			*pcMessage,
						   T_CYCLEFUNCT	*ptCycleFunct,
						   T_CYCLETIME	*ptCycleTime)
{
	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "DelEmptyTekTep: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

        LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete empty-TE on KCL-EP-01" );

        SqlContext              *hSqlCtxt = NULL;
        const char              *pcFac = "delEmptyTe";
        void                    *pvTid = NULL;
        long                    lRv = 0;

	hSqlCtxt = TSqlNewContext (pvTid, NULL);
	if (hSqlCtxt == NULL)
	{
		LogPrintf (pcFac, LT_ALERT,
			"DelEmptyTekTep\n"
			"error in call <TSqlNewContext>\n"
			"SQL: %s",
			TSqlErrTxt (pvTid));
		return (-1);
	}

	lRv = _DelEmptyTekTep(pvTid, hSqlCtxt, pcFac);

	TSqlDestroyContext (pvTid, hSqlCtxt);

	if (lRv <= 0) {
		LogPrintf (pcFac, LT_ALERT, "DelEmptyTekTep: ROLLBACK to all");
		TSqlRollback (pvTid);	
	} else {
		LogPrintf (pcFac, LT_ALERT, "DelEmptyTekTep: COMMIT to all");
		TSqlCommit (pvTid);
	}
	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete empty-TE's" );

	return (0);
}

static int  _DelNullTep (void           *pvTid, 
                                                 SqlContext     *hSqlCtxt, 
                                                 const char             *pcFac,
                                                 const char             *pcOraSp)
{
        long                    nlProcessed = 0;
        long                    lRv, lDbRv, nlDbRv = 0, nI, nlTepCnt;
	char			aacTeId [BLOCKSIZE] [TEID_LEN+1];
	char			aacTetId [BLOCKSIZE] [TETID_LEN+1];
	PBL_MGR_CTXT	tPblMgrCtxt ;

	memset (&tPblMgrCtxt, 0, sizeof (tPblMgrCtxt));

	tPblMgrCtxt.pcFac = pcFac;

	do
	{
		memset (aacTeId, 0, sizeof (aacTeId));
		memset (aacTetId, 0, sizeof (aacTetId));

		nlDbRv = nlDbRv == 0
		? TExecSqlX (pvTid,
					 hSqlCtxt,
					 "SELECT TEK.TEID, TEK.Tet_TetId "
					 "FROM TEK, TEP, FES, LAS "
					 "WHERE TEK.TeId = TEP.TeId "
					 "AND TEK.Pos_FeldId = FES.FeldId "
					 "AND FES.LagId = LAS.LagId "
					 "AND LAS.LAttr_FindLp = 1 "
					 "AND LTRIM (TEK.AusId_Mandant) IS NULL "
					 "AND LTRIM (TEK.AusId_AusNr) IS NULL "
					 "AND LTRIM (TEK.AusId_AusKz) IS NULL "
					 "AND TEP.Mngs_Mng <= 0 "
					 "GROUP BY TEK.TeId, TEK.Tet_TetId "
					 "",
					 BLOCKSIZE, 0,
					 SELSTR (aacTeId[0], TEID_LEN+1),
					 SELSTR (aacTetId[0], TETID_LEN+1),
					 NULL)
		: TExecSqlV (pvTid, hSqlCtxt, NULL, NULL, NULL, NULL);

		if (nlDbRv <= 0)
		{
			if (TSqlError (pvTid) != SqlNotFound)
			{
				LogPrintf (pcFac, LT_ALERT,
					"_DelNullTep\n"
					"error during read TEK's with null quantity\n"
					"SQL: %s",
					TSqlErrTxt (pvTid));
				return (-1);
			}

			break;		/* end do-while loop */
		}

		for (nI = 0; nI < nlDbRv; nI ++)
		{
			lRv = _PBL_MGR_SetOraSavepoint (pvTid, pcOraSp);
			if (lRv < 0)
			{
				LogPrintf (pcFac, LT_ALERT,
					"_DelNullTep\n"
					"can't set oracle savepoint\n"
					"SQL: %s",
					TSqlErrTxt (pvTid));
				return (-1);
			}

			lRv = _PBL_MGR_CheckTepQuantity (pvTid, &tPblMgrCtxt, aacTeId [nI]);
			if (lRv <= 0)
			{
				lRv = _PBL_MGR_RollbackOraSavepoint (pvTid, pcOraSp);
				if (lRv < 0)
				{
					LogPrintf (pcFac, LT_ALERT,
						"_DelNullTep\n"
						"can't make rollback to oracle savepoint\n"
						"SQL: %s",
						TSqlErrTxt (pvTid));
					return (-1);
				}

				continue;		/* with next TE-Id */
			}

			/*****************************************************************
			 * if TE-Type is a tablar with meassure heigth = 300 [mm]
			 * and no positions still exists modify the 
			 ****************************************************************/
			if (strcmp (aacTetId [nI], TETID_T_300) == 0)
			{
				TEK		_tTek;

				lDbRv = TExecSql (pvTid,
								  "SELECT COUNT (TEP.TeId) FROM TEP "
								  "WHERE TEP.TeId = :TeId ",
								  SELLONG	(nlTepCnt),
								  SQLSTRING	(aacTeId [nI]),
								  NULL);
				if (lDbRv != 1)
				{
					LogPrintf (pcFac, LT_ALERT,
						"_DelNullTep\n"
						"can't read count of TEP's for TE-Id: %s\n"
						"SQL: %s",
						aacTeId [nI],
						TSqlErrTxt (pvTid));
					return (-1);
				}

				if (nlTepCnt <= 0)
				{
					memset (&_tTek, 0, sizeof (TEK));
					strcpy (_tTek.tekTeId, aacTeId [nI]);

					lDbRv = TExecStdSql (pvTid, StdNselectUpdNo, TN_TEK,&_tTek);
					if (lDbRv != 1)
					{
						if (TSqlError (pvTid) != SqlLocked)
						{
							LogPrintf (pcFac, LT_ALERT,
								"_DelNullTep\n"
								"can't lock TEK's for TE-Id: %s\n"
								"SQL: %s",
								aacTeId [nI],
								TSqlErrTxt (pvTid));
							return (-1);
						}

						lRv = _PBL_MGR_RollbackOraSavepoint (pvTid, pcOraSp);
						if (lRv < 0)
						{
							LogPrintf (pcFac, LT_ALERT,
								"_DelNullTep\n"
								"can't make rollback to oracle savepoint\n"
								"SQL: %s",
								TSqlErrTxt (pvTid));
							return (-1);
						}

						continue;		/* with next TE-Id */
					}

					strcpy (_tTek.tekTet.TetId, TETID_T_160);

					SetHist (TN_TEK, &_tTek, HIST_UPDATE, GetUserOrTaskName());

					lDbRv = TExecStdSql (pvTid, StdNupdate, TN_TEK, &_tTek);
					if (lDbRv != 1)
					{
						LogPrintf (pcFac, LT_ALERT,
							"_DelNullTep\n"
							"can't update TEK's for TE-Id: %s\n"
							"SQL: %s",
							aacTeId [nI],
							TSqlErrTxt (pvTid));
						return (-1);
					}
				}
			}
					
			nlProcessed ++;
		}
	}
	while (nlDbRv == BLOCKSIZE);

	return (nlProcessed);
}

static int DelNullTep (void			*pvCalldata,
					   long			lTrigger,
					   char			*pcMessage,
					   T_CYCLEFUNCT	*ptCycleFunct,
                                           T_CYCLETIME  *ptCycleTime)
{
        SqlContext              *hSqlCtxt = NULL;
        const char                      *pcFac = FAC_DBSDAEMON;
        void                    *pvTid = NULL;
        long                    lRv;

	hSqlCtxt = TSqlNewContext (pvTid, NULL);
	if (hSqlCtxt == NULL)
	{
		LogPrintf (pcFac, LT_ALERT,
			"DelNullTep\n"
			"error in call <TSqlNewContext>\n"
			"SQL: %s",
			TSqlErrTxt (pvTid));
		return (-1);
	}

	lRv = _DelNullTep (pvTid, hSqlCtxt, pcFac, "ORA_SP");

	TSqlDestroyContext (pvTid, hSqlCtxt);

	if (lRv <= 0)
	{
		TSqlRollback (pvTid);	
	}
	else
	{
		TSqlCommit (pvTid);
	}

	return (0);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*      Funktion sichert das SHM zyklisch auf die Platte
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int SaveGlobals (void *pvCalldata,
                        long lTrigger,
                        char *pcMessage,
                        T_CYCLEFUNCT *ptFunc,
                        T_CYCLETIME *ptTime)
{
    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Globale sichern");

    SaveGlobalDemonFunct ();

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Globale sichern");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _QuitDirtyAus   (void            *pvCalldata,
-*							    long            lTrigger,
-*							    char            *pcMessage,
-*							    T_CYCLEFUNCT    *ptFunc,
-*							    T_CYCLETIME     *ptTime
-* DESCRIPTION
-*	Quittieren von schrecklichen Auslagerauftrï¿½gen an das LVS
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int _QuitDirtyAus(	void 			*pvCalldata,
                      		long 			lTrigger,
                       		char 			*pcMessage,
                       		T_CYCLEFUNCT 	*ptFunc,
                       		T_CYCLETIME 	*ptTime
					)
{
        long    lRet=0;
        void    *pvTid=NULL;

    LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> Start: QuitDirtyAus");

        lRet = _QuitOrderNoPos  (pvTid, FAC_DBSDAEMON);
	if (lRet <= 0)
	{
		TSqlRollback (pvTid);
	}
	else
	{
		TSqlCommit (pvTid);
	}

    LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> End: QuitDirtyAus");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _CBFL   (void            *pvCalldata,
-*						long            lTrigger,
-*						char            *pcMessage,
-*						T_CYCLEFUNCT    *ptFunc,
-*						T_CYCLETIME     *ptTime
-* DESCRIPTION
-*	Gesperrte Plï¿½tze und Gassen umverplanen
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int _CBFL	(	void 			*pvCalldata,
		long 			lTrigger,
		char 			*pcMessage,
		T_CYCLEFUNCT 	*ptFunc,
		T_CYCLETIME 	*ptTime
)
{

	void    *pvTid=NULL;

	LogPrintf (FAC_DBFDAEMON, LT_DEBUG, "=> Start: CBFL");

	CheckBlockedFieldsAndLanes       (pvTid, FAC_DBFDAEMON);

	LogPrintf (FAC_DBFDAEMON, LT_DEBUG, "=> End: CBFL");

	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _RFCO   (void            *pvCalldata,
-*						long            lTrigger,
-*						char            *pcMessage,
-*						T_CYCLEFUNCT    *ptFunc,
-*						T_CYCLETIME     *ptTime
-* DESCRIPTION
-*	'Ueberlaufende' Ruestfronten leerraeumen.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int _RFCO	(	void 			*pvCalldata,
		long 			lTrigger,
		char 			*pcMessage,
		T_CYCLEFUNCT 	*ptFunc,
		T_CYCLETIME 	*ptTime
)
{

	void    *pvTid=NULL;

	LogPrintf (FAC_DBFDAEMON, LT_DEBUG, "=> Start: RFCO");

	RF_CheckOverflow (pvTid);

	LogPrintf (FAC_DBFDAEMON, LT_DEBUG, "=> End: RFCO");

	return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int SndWeTe2Lvs   (  void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-* DESCRIPTION
-*	Send Paletten am Wareneingang an das LVS
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
#if 0
static int SndWeTe2Lvs  (       void                    *pvCalldata,
                                long                    lTrigger,
                                char                    *pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;

    LogPrintf(FAC_DBSDAEMON, LT_DEBUG,
              "=> Start: Send WE-TE");

	lRet = LLS_QuittWeTe	(	pvTid,
								FAC_QUITTWETE
							);

	switch (lRet) {
		case 0:
			TSqlRollback (pvTid);
			break;
		case 1:
			TSqlCommit (pvTid);
			break;
		default:
			TSqlRollback (pvTid);
			LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
						"Abort Function LLS_WeTe");
			break;
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Send WE-TE");

    return (0);
}
#endif

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int SndAus2Lvs   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Send Auslagerauftrag to LVS 
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int SndAus2Lvs	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
	void	*pvTid=NULL;
	int		nI=0;
	long	lDbRv=0, nlDbRv=0, lRet=0, lFirst=0;
        long    l2s_AuskHostStatus;
        AUSK    atAusk[BLOCKSIZE];
        SqlContext      *ptSqlCtx = NULL;
        const char      *pcFacility=FAC_DBFDAEMON;
        unsigned long start_time = get_microtime();

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Send Auslagerauftrag");
                          

	/*  Create Context  */
    ptSqlCtx = TSqlNewContext (pvTid, NULL);
    if (ptSqlCtx == NULL) 
	{
        LogPrintf (pcFacility, LT_ALERT,
                    "SndAus2Lvs: Function TSqlNewContext failed");
        return (-1);
    }

	l2s_AuskHostStatus 	= AUSKHOSTSTATUS_SENDEN;
	nlDbRv = 0;

	do 
	{
		memset (&atAusk[0], 0, sizeof (atAusk));

		nlDbRv = (nlDbRv == 0) ?
			TExecSqlX (pvTid, ptSqlCtx,
					"Select "
						"%AUSK "
					"From "
						"AUSK "
					"Where "
						"AUSK.HostStatus = :hoststatus "
					"Order By "
						"Prio, LiefTerm "
					"For Update NoWait ",
					BLOCKSIZE, 0,
					SELSTRUCT 			(TN_AUSK, atAusk[0]),
					SQLAUSKHOSTSTATUS	(l2s_AuskHostStatus),
					NULL) :

			TExecSqlV (pvTid, ptSqlCtx, NULL, NULL, NULL, NULL);


		if (nlDbRv < BLOCKSIZE && TSqlError (pvTid) != SqlNotFound) {
        	LogPrintf (pcFacility, LT_ALERT, "Fehler beim Select der AUSK's: \n"
					"SQL <%s>",
                    TSqlErrTxt (pvTid));
			TSqlRollback(pvTid);
            TSqlDestroyContext (pvTid, ptSqlCtx);
            return (-1);
        }
		if (nlDbRv <= 0) {
			/* no (more) records found */
            break;
        }

		for (nI=0; nI < nlDbRv; nI++)
		{
			lDbRv = TExecSql (NULL, "SAVEPOINT SNDAUS2LVS", NULL);
			if (lDbRv < 0) 
			{ 	/* --- Savepoint Error --- */
  				TSqlDestroyContext (pvTid, ptSqlCtx);
				LogPrintf(pcFacility, LT_ALERT,
						  "SndAus2Lvs:\n"
						  "Set SAVEPOINT 'SNDAUS2LVS' failed!");
				return (-1);
        	}

			LogPrintf(pcFacility, LT_DEBUG, "Start transfer for AusNr |%s|", 
				atAusk[nI].auskAusId.AusNr);

			lRet = LLS_QuittAUS	(	pvTid,
									FAC_QUITTAUS,
									&atAusk[nI]
								);
			if (lRet < 0)
			{
			 	lDbRv = TExecSql (NULL, "ROLLBACK TO SNDAUS2LVS", NULL);
            	if (lDbRv < 0) 
				{  	/* --- Rollback to Savepoint Error  --- */
  					TSqlDestroyContext (pvTid, ptSqlCtx);
                	LogPrintf(pcFacility, LT_ALERT,
							  "SndAus2Lvs:\n"
							  "ROLLBACK to SAVEPOINT 'SNDAUS2LVS' failed!");
                	return (-1);
            	}
            	continue;   /* PROCESS NEXT AUSP */
			}

			TSqlCommit (pvTid);
		}

		lFirst = 1;

	} while (BLOCKSIZE == nlDbRv);

  	TSqlDestroyContext (pvTid, ptSqlCtx);

	if (lFirst == 0)
	{
		LogPrintf (pcFacility, LT_ALERT,
					"Keine AUSK-Daten gefunden ...");
		return (0);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Send Auslagerauftrag runtime: %.3fsec\n", (get_microtime() - start_time )  / 1000.0 );

    return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int SndInv2Lvs   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Send Inventuren to LVS 
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int SndInv2Lvs	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
	long	lRet=0;
        void    *pvTid=NULL;
        unsigned long start_time = get_microtime();

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Send Inventurauftrag");

	lRet = LLS_QuittINV	(	pvTid,
							FAC_QUITTINV
						);

	switch (lRet) {
		case 0:
			TSqlRollback (pvTid);
			break;
		case 1:
			TSqlCommit (pvTid);
			break;
		default:
			TSqlRollback (pvTid);
			LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
						"Abort Function LLS_QuittINV");
			break;
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Send Inventurauftrag runtime: %.3fsec\n", (get_microtime() - start_time )  / 1000.0 );

    return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _Process_AUSP_	(	void	*pvTid,
-*									char	*pcFac
-*									AUSK	*ptAusk
-*								)
-*
-* DESCRIPTION
-*	Set AUSP HostStatus to SENDEN
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _Process_AUSP_       (       void    *pvTid,
                                                                const char      *pcFac,
                                                                AUSK    *ptAusk
                                                        )
{
	int			nI=0;
	long		nlDbRv=0, lFirst=0;
	AUSP		atAusp[BLOCKSIZE];
	SqlContext	*ptSqlCtx = NULL;
	long		lval_AUSPHOSTSTATUS_NEU = AUSPHOSTSTATUS_NEU;


	/*  Create Context  */
    ptSqlCtx = TSqlNewContext (pvTid, NULL);
    if (ptSqlCtx == NULL) 
	{
        LogPrintf (pcFac, LT_ALERT,
                    "_Process_AUSP_: Function TSqlNewContext failed");
        return (-1);
    }

	nlDbRv = 0;

	do 
	{
		memset (&atAusp[0], 0, sizeof (atAusp));

		nlDbRv = nlDbRv == 0 ?
			TExecSqlX (pvTid, ptSqlCtx,
					"Select "
						"%AUSP "
					"From "
						"AUSP "
					"Where "
						"AUSP.AusId_Mandant = :mandant And "
						"AUSP.AusId_AusNr   = :ausnr   And "
						"AUSP.AusId_AusKz   = :auskz And "
						"AUSP.HostStatus = :AUSPHOSTSTATUS_NEU "		/* */
					"For Update NoWait ",
					BLOCKSIZE, 0,
					SELSTRUCT	(TN_AUSP, atAusp[0]),
					SQLSTRING	(ptAusk->auskAusId.Mandant),
					SQLSTRING	(ptAusk->auskAusId.AusNr),
					SQLSTRING	(ptAusk->auskAusId.AusKz),
					SQLAUSPHOSTSTATUS (lval_AUSPHOSTSTATUS_NEU),		/* */
					NULL) :

			TExecSqlV (pvTid, ptSqlCtx, NULL, NULL, NULL, NULL);


		if (nlDbRv < BLOCKSIZE && TSqlError (pvTid) != SqlNotFound) {
                LogPrintf (pcFac, LT_ALERT,
                        "Fehler beim Select der AUSP's: \n"
						"SQL <%s>",
                            TSqlErrTxt (pvTid));
				TSqlRollback(pvTid);
                TSqlDestroyContext (pvTid, ptSqlCtx);
                return (-1);
		}
        
		if (nlDbRv <= 0) {
            break;
        }

		for (nI=0; nI < nlDbRv; nI++)
		{
			atAusp[nI].auspHostStatus = AUSPHOSTSTATUS_SENDEN;
			SetHist (TN_AUSP, 
					 (void*)&atAusp[nI], 
					 HIST_UPDATE, 
					 GetUserOrTaskName());
		}
		lFirst = 1;

		if (TExecStdSqlX (pvTid, NULL, StdNupdate, TN_AUSP,
                        atAusp, nlDbRv, NULL, NULL) != nlDbRv)
        {
            TSqlDestroyContext (pvTid, ptSqlCtx);
			LogPrintf (pcFac, LT_ALERT,
                    	"Fehler beim Update der AUSP's: \n"
						"SQL <%s>",
                        	TSqlErrTxt (pvTid));
			return (-1);
		}	

	} while (BLOCKSIZE == nlDbRv);

  	TSqlDestroyContext (pvTid, ptSqlCtx);

	if (lFirst == 0)
	{
		LogPrintf (pcFac, LT_ALERT,
					"Keine AUSP-Daten gefunden ...");
		return (0);
	}

	return (1);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _Handle_AUS_	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Set AUSK HostStatus to SENDEN
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _Handle_AUS_ (       void    *pvTid,
                                                        const char      *pcFac
                                                )
{
        int                     nI=0;
	long		nlDbRv=0, lRet=0, lDbRv=0, lFirst=0;
	long		l2s_AuskStatusGepuffert, l2s_AuskHostStatusNeu;
	long		l2s_AuskStatusFertig;
	long		l2s_AuskStatusVerlB;
	long		l2s_AuspStatusFertig,l2s_AuspStatusStorno;
	long		l2s_AuspHostStatusFertig, l2s_AuspHostStatusNeu;
	AUSK		atAusk[BLOCKSIZE];
	SqlContext	*ptSqlCtx = NULL;


	/*  Create Context  */
    ptSqlCtx = TSqlNewContext (pvTid, NULL);
    if (ptSqlCtx == NULL) 
	{
        LogPrintf (pcFac, LT_ALERT,
                    "_Handle_AUS_: Function TSqlNewContext failed");
        return (-1);
    }


	l2s_AuskStatusGepuffert	= AUSKSTATUS_GEPUFFERT;
	l2s_AuskStatusFertig	= AUSKSTATUS_FERTIG;
	l2s_AuskStatusVerlB		= AUSKSTATUS_VERLB;
	l2s_AuskHostStatusNeu 	= AUSKHOSTSTATUS_NEU;

	l2s_AuspStatusFertig 	= AUSPSTATUS_FERTIG;
	l2s_AuspStatusStorno 	= AUSPSTATUS_STORNO;
	l2s_AuspHostStatusNeu 	= AUSPHOSTSTATUS_NEU;
	l2s_AuspHostStatusFertig= AUSPHOSTSTATUS_FERTIG;

	nlDbRv = 0;

	do 
	{
		memset (&atAusk[0], 0, sizeof (atAusk));

		nlDbRv = nlDbRv == 0 ?
			TExecSqlX (pvTid, ptSqlCtx,
					"Select "
						"%AUSK "
					"From "
						"AUSK "
					"Where "
						"AUSK.Status IN (:status1, :status2, :status3) And "
						"AUSK.HostStatus = :hoststatus1 And "
					"0 = ( "
							"Select "
								"Count (AUSP.Status) "
							"From "		
								"AUSP "
							"Where "
								"AUSK.AusId_Mandant = AUSP.AusId_Mandant And "
								"AUSK.AusId_AusNr   = AUSP.AusId_AusNr   And "
								"AUSK.AusId_AusKz   = AUSP.AusId_AusKz   And "
								"AUSP.Status != :status20 And "
								"AUSP.Status != :status21 "
						") And "
					"0 = ( "
							"Select "
								"Count (AUSP.Status) "
							"From "		
								"AUSP "
							"Where "
								"AUSK.AusId_Mandant = AUSP.AusId_Mandant And "
								"AUSK.AusId_AusNr   = AUSP.AusId_AusNr   And "
								"AUSK.AusId_AusKz   = AUSP.AusId_AusKz   And "
								"AUSP.HostStatus != :hoststatus2 And "
								"AUSP.HostStatus != :hoststatus3 "
						") And "
					"0 < ( "
							"Select "
								"Count (AUSP.Status) "
							"From "		
								"AUSP "
							"Where "
								"AUSK.AusId_Mandant = AUSP.AusId_Mandant And "
								"AUSK.AusId_AusNr   = AUSP.AusId_AusNr   And "
								"AUSK.AusId_AusKz   = AUSP.AusId_AusKz   And "
								"AUSP.HostStatus = :hoststatus4 "
						") "
					"For Update NoWait ",
					BLOCKSIZE, 0,
					SELSTRUCT 			(TN_AUSK, atAusk[0]),
					SQLAUSKSTATUS 		(l2s_AuskStatusGepuffert),
					SQLAUSKSTATUS 		(l2s_AuskStatusFertig),
					SQLAUSKSTATUS 		(l2s_AuskStatusVerlB),
					SQLAUSKHOSTSTATUS 	(l2s_AuskHostStatusNeu),
					SQLAUSPSTATUS 		(l2s_AuspStatusFertig),
					SQLAUSPSTATUS 		(l2s_AuspStatusStorno),
					SQLAUSPHOSTSTATUS	(l2s_AuspHostStatusNeu),
					SQLAUSPHOSTSTATUS	(l2s_AuspHostStatusFertig),
					SQLAUSPHOSTSTATUS	(l2s_AuspHostStatusNeu),
					NULL) :

			TExecSqlV (pvTid, ptSqlCtx, NULL, NULL, NULL, NULL);


		if (nlDbRv < BLOCKSIZE && TSqlError (pvTid) != SqlNotFound) {
                LogPrintf (pcFac, LT_ALERT,
                        "Fehler beim Select der AUSK's: \n"
						"SQL <%s>", TSqlErrTxt (pvTid));
				TSqlRollback(pvTid);
                TSqlDestroyContext (pvTid, ptSqlCtx);
                return (-1);
        }  

		if (nlDbRv <= 0) {
            break;
        }

		for (nI=0; nI < nlDbRv; nI++)
		{

			lDbRv = TExecSql (NULL, "SAVEPOINT SETAUS2SND_LVS", NULL);
			if (lDbRv < 0) 
			{ 	/* --- Savepoint Error --- */
  				TSqlDestroyContext (pvTid, ptSqlCtx);
				LogPrintf(pcFac, LT_ALERT,
						  "_Handle_AUS_:\n"
						  "Set SAVEPOINT 'SETAUS2SND_LVS' failed!");
				return (-1);
        	}

			lRet = _Process_AUSP_	(	pvTid,
										pcFac,
										&atAusk[nI]
									);

			if (lRet < 0)
			{	
			 	lDbRv = TExecSql (NULL, "ROLLBACK TO SETAUS2SND_LVS", NULL);
            	if (lDbRv < 0) 
				{  	/* --- Rollback to Savepoint Error  --- */
  					TSqlDestroyContext (pvTid, ptSqlCtx);
                	LogPrintf(pcFac, LT_ALERT,
							  "_Handle_AUS_:\n"
							  "ROLLBACK to SAVEPOINT 'SETAUS2SND_LVS' failed!");
                	return (-1);
            	}
            	continue;   /* PROCESS NEXT AUSP */
			}

			atAusk[nI].auskHostStatus = AUSKHOSTSTATUS_SENDEN;
			SetHist (TN_AUSK, 
					 (void*)&atAusk[nI], 
					 HIST_UPDATE, 
					 GetUserOrTaskName());

			if (TExecStdSql (pvTid, StdNupdate, TN_AUSK, &atAusk[nI]) != 1) 
			{
				LogPrintf (pcFac, LT_ALERT,
							"Update AUSK failed! \n"
							"SQL <%s> \n",
								TSqlErrTxt (pvTid));
				return (-1);
			}
		}

		lFirst = 1;

	} while (BLOCKSIZE == nlDbRv);

  	TSqlDestroyContext (pvTid, ptSqlCtx);

	if (lFirst == 0)
	{
		LogPrintf (pcFac, LT_ALERT,
					"Keine AUSK-Daten gefunden ...");
		return (0);
	}

	return (1);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int SetAus4Snd   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Set AUSK HostStatus to SENDEN
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int SetAus4Snd	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;


    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Send AUSK HostStatus");

        lRet = _Handle_AUS_ (   pvTid,
                                                        pcFacility
                                                );
	
	if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);

		CF_TRIGGER (PROC_DBFDAEMON, FUNC_DBFDAEMON_SNDAUS2LVS, 0, "sethstate");
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Set AUSK HostStatus");

    return (0);
}



/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _FreeClUebh	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Creates TPA's for TE's at CL-UEBH-x without TPA
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _FreeClUebh  (       void    *pvTid,
                                                        const char      *pcFac
                                                )
{
        int                     nI=0;
	long		nlDbRv=0, lRet=0, lFirst=1;
	TPA			tTpa;
	TEK			atTek[BLOCKSIZE];
	SqlContext	*ptSqlCtx = NULL;


	/*  Create Context  */
    ptSqlCtx = TSqlNewContext (pvTid, NULL);
    if (ptSqlCtx == NULL) 
	{
        LogPrintf (pcFac, LT_ALERT,
                    "_FreeClUebh: Function TSqlNewContext failed");
        return (-1);
    }


	do 
	{
		nlDbRv = TExecSqlX (pvTid, ptSqlCtx,
					lFirst == 1 ?
						"SELECT %TEK FROM TEK "
						"WHERE TEK.Pos_FeldId IN ('" FELDID_CL1_UEBH_1"',"  
												  "'" FELDID_CL2_UEBH_1"',"  
												"'" FELDID_CL3_UEBH_1"',"  
												"'" FELDID_CL4_UEBH_1"'," 
												"'" FELDID_CL5_UEBH_1"'," 
												"'" FELDID_CL6_UEBH_1"'," 
												"'" FELDID_CL7_UEBH_1"'," 
												"'" FELDID_CL8_UEBH_1"'," 
												"'" FELDID_CL9_UEBH_1"'," 
												"'" FELDID_CL10_UEBH_1"'," 
												"'" FELDID_CL11_UEBH_1"'," 
												"'" FELDID_CL12_UEBH_1"'," 
												"'" FELDID_CL13_UEBH_1"'," 
												"'" FELDID_CL14_UEBH_1"'," 
												"'" FELDID_CL15_UEBH_1"')" 
						" AND NOT EXISTS "
						"("
						"SELECT 1 FROM TPA WHERE TPA.TeId = TEK.TeId"
						") "
						"AND NOT EXISTS "
                    	"( "
                    	"SELECT 1 FROM FSP WHERE TEK.Pos_FeldId = FSP.Pos_FeldId "
                    	"AND FSP.FSPMODUS = 'TOT'"
                    	") "
						: NULL,
					BLOCKSIZE, 0,
					SELSTRUCT (TN_TEK, atTek[0]),
					NULL);

		if (nlDbRv < 0)
        {
            if (TSqlError (pvTid) != SqlNotFound)
            {
                TSqlDestroyContext (pvTid, ptSqlCtx);
                LogPrintf (pcFac, LT_ALERT,
                        "Fehler beim Select der TEK's: \n"
						"SQL <%s>",
                            TSqlErrTxt (pvTid));

                return (-1);
            }
			break;
        }

		for (nI=0; nI < nlDbRv; nI++)
		{
			memset(&tTpa, 0, sizeof(TPA));
			tTpa.tpaQuelle = atTek[nI].tekPos;
			tTpa.tpaAktpos = atTek[nI].tekPos;
			strcpy (tTpa.tpaTeId, atTek[nI].tekTeId);
			strncpy (tTpa.tpaZiel.FeldId, FELDID_DR, FELDID_LEN);
			tTpa.tpaZiel.FeldId[FELDID_LEN] = 0;

			strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
			tTpa.tpaTpmId[TPMID_LEN] = 0;

			lRet = TpaAnlegen(pvTid, &tTpa, pcFac);

			if (lRet < 0) {
				LogPrintf(pcFac, LT_ALERT,
					"CreateTPA for TeId [%s] failed",
					tTpa.tpaTeId);
				return (-1);
			}
			else {
				LogPrintf(pcFac, LT_DEBUG,
					"TPA for TeId [%s] from [%s] to '" FELDID_DR"' created" ,
					tTpa.tpaTeId,
					Pos2Str(pvTid, &atTek[nI].tekPos, NULL));
			}
		}

		lFirst = 0;

	} while (BLOCKSIZE == nlDbRv);

  	TSqlDestroyContext (pvTid, ptSqlCtx);

	return (1);
}
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _FreeWapUeb	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Creates TPA's for TE's at WAP-UEB-xx without TPA
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _FreeWapUeb  (       void    *pvTid,
                                                        const char      *pcFac
                                                )
{
        int                     nI=0;
	long		nlDbRv=0, lRet=0, lNewTpa;
	TPA			tTpa;
	TEK			atTek[BLOCKSIZE];

	memset (&atTek[0], 0, sizeof (atTek));

	nlDbRv = TExecSqlX (pvTid, NULL,
						"SELECT "
						"   %TEK "
						"FROM "
						"   TEK "
						"WHERE "
						" TEK.Pos_FeldId IN ('" FELDID_WAP_UEB_01"'  " 
						"				   , '" FELDID_WAP_UEB_02"'  " 
						"				   , '" FELDID_WAP_UEB_03"'  " 
						"				   , '" FELDID_WAP_UEB_04"'  " 
						"				   , '" FELDID_WAP_UEB_05"'  " 
						"				   , '" FELDID_WAP_UEB_06"'  " 
						"				   , '" FELDID_WAP_UEB_07"'  " 
						"				   , '" FELDID_WAP_UEB_08"') " 
						"AND NOT EXISTS ( "
						"   SELECT 1 FROM TPA WHERE TPA.TeId = TEK.TeId) "
						"AND NOT EXISTS ( "
                    	"   SELECT 1 FROM FSP WHERE TEK.Pos_FeldId = FSP.Pos_FeldId "
                    	"      AND FSP.FSPMODUS = 'TOT' ) ",
					BLOCKSIZE, 0,
					SELSTRUCT (TN_TEK, atTek[0]),
					NULL);

	if (nlDbRv <= 0 && TSqlError (pvTid) != SqlNotFound) {

            LogPrintf (pcFac, LT_ALERT,
                    "Select TEK failed! \n SQL <%s>", 
					TSqlErrTxt (pvTid));

            return (-1);
    }

	lNewTpa = 0;

	for (nI=0; nI < nlDbRv; nI++)
	{
		memset(&tTpa, 0, sizeof(TPA));
		tTpa.tpaQuelle = atTek[nI].tekPos;
		tTpa.tpaAktpos = atTek[nI].tekPos;
		strcpy (tTpa.tpaTeId, atTek[nI].tekTeId);
		strncpy (tTpa.tpaZiel.FeldId, FELDID_WAP, FELDID_LEN);
		tTpa.tpaZiel.FeldId[FELDID_LEN] = 0;
		tTpa.tpaBewTyp = BEWTYP_UM;

		strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
		tTpa.tpaTpmId[TPMID_LEN] = 0;

		lRet = TpaAnlegen (pvTid, &tTpa, pcFac);

		if (lRet < 0) {
			LogPrintf(pcFac, LT_ALERT,
					"CreateTPA for TeId [%s] failed",
					tTpa.tpaTeId);
			return (-1);
		}

		LogPrintf(pcFac, LT_DEBUG,
					"TPA for TeId [%s] from [%s] to '" FELDID_WAP"' created" ,
					tTpa.tpaTeId,
					Pos2Str(pvTid, &atTek[nI].tekPos, NULL));

		lNewTpa = 1;
	}

	return (lNewTpa);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _FreeWapNRP	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Creates TPA's for TE's at WAP-NRP-xx without TPA
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _FreeWapNRP  (       void    *pvTid,
                                                        const char      *pcFac
                                                )
{
        int                     nI=0;
	long		nlDbRv=0, lRet=0, lNewTpa;
	TPA			tTpa;
	TEK			atTek[BLOCKSIZE];

	memset (&atTek[0], 0, sizeof (atTek));

	nlDbRv = TExecSqlX (pvTid, NULL,
						"SELECT "
						"   %TEK "
						"FROM "
						"   TEK "
						"WHERE "
						" TEK.Pos_FeldId IN ('" FELDID_WAP_NRP_01"'  " 
						"				   , '" FELDID_WAP_NRP_02"'  " 
						"				   , '" FELDID_WAP_NRP_03"'  " 
						"				   , '" FELDID_WAP_NRP_04"'  " 
						"				   , '" FELDID_WAP_NRP_05"'  " 
						"				   , '" FELDID_WAP_NRP_06"') " 
						"AND NOT EXISTS ( "
						"   SELECT 1 FROM TPA WHERE TPA.TeId = TEK.TeId)"
						"AND NOT EXISTS ( "
                    	"   SELECT 1 FROM FSP WHERE TEK.Pos_FeldId = FSP.Pos_FeldId "
                    	"      AND FSP.FSPMODUS = 'TOT' ) ",
					BLOCKSIZE, 0,
					SELSTRUCT (TN_TEK, atTek[0]),
					NULL);

	if (nlDbRv <= 0 && TSqlError (pvTid) != SqlNotFound) {

            LogPrintf (pcFac, LT_ALERT,
                    "Select TEK failed! \n SQL <%s>", 
					TSqlErrTxt (pvTid));

            return (-1);
    }

	lNewTpa = 0;

	for (nI=0; nI < nlDbRv; nI++)
	{
		memset(&tTpa, 0, sizeof(TPA));
		tTpa.tpaQuelle = atTek[nI].tekPos;
		tTpa.tpaAktpos = atTek[nI].tekPos;
		strcpy (tTpa.tpaTeId, atTek[nI].tekTeId);
		strncpy (tTpa.tpaZiel.FeldId, FELDID_WAB_NR_1, FELDID_LEN);
		tTpa.tpaZiel.FeldId[FELDID_LEN] = 0;
		tTpa.tpaBewTyp = BEWTYP_AUSSCHL;

		strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
		tTpa.tpaTpmId[TPMID_LEN] = 0;

		lRet = TpaAnlegen (pvTid, &tTpa, pcFac);

		if (lRet < 0) {
			LogPrintf(pcFac, LT_ALERT,
					"CreateTPA for TeId [%s] failed",
					tTpa.tpaTeId);
			return (-1);
		}

		LogPrintf(pcFac, LT_DEBUG,
					"TPA for TeId [%s] from [%s] to '" FELDID_WAB_NR_1"' created" ,
					tTpa.tpaTeId,
					Pos2Str(pvTid, &atTek[nI].tekPos, NULL));

		lNewTpa = 1;
	}

	return (lNewTpa);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _FreeZtlUeb	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Creates TPA's for TE's at ZTL-UEB-xx without TPA
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _FreeZtlUeb  (       void    *pvTid,
                                                        const char      *pcFac
                                                )
{
        int                     nI=0;
	long		nlDbRv=0, lRet=0, lAnzTpa;
	TPA			tTpa;
	TEK			atTek[BLOCKSIZE];

	memset (&atTek[0], 0, sizeof (atTek));

	nlDbRv = TExecSqlX (pvTid, NULL,
						"SELECT "
						"   %TEK "
						"FROM "
						"   TEK "
						"WHERE "
						"   TEK.Pos_FeldId IN ('" FELDID_ZTL_UEB_01"'  " 
						"                    , '" FELDID_ZTL_UEB_02"'  " 
						"                    , '" FELDID_ZTL_UEB_03"'  " 
						"					 , '" FELDID_ZTL_UEB_04"') " 
						"AND NOT EXISTS ( "
						"   SELECT 1 FROM TPA WHERE TPA.TeId = TEK.TeId)"
						"AND NOT EXISTS ( "
                    	"   SELECT 1 FROM FSP WHERE TEK.Pos_FeldId = FSP.Pos_FeldId "
                    	"      AND FSP.FSPMODUS = 'TOT' ) ",
						BLOCKSIZE, 0,
						SELSTRUCT (TN_TEK, atTek[0]),
						NULL);

	if (nlDbRv <= 0 && TSqlError (pvTid) != SqlNotFound) {

    	LogPrintf (pcFac, LT_ALERT,
                "Select TEK failed! \n SQL <%s>",
                TSqlErrTxt (pvTid));

		return (-1);
	}

	lAnzTpa = 0;

	for (nI=0; nI < nlDbRv; nI++)
	{
		memset(&tTpa, 0, sizeof(TPA));
		tTpa.tpaQuelle = atTek[nI].tekPos;
		tTpa.tpaAktpos = atTek[nI].tekPos;
		strcpy (tTpa.tpaTeId, atTek[nI].tekTeId);
		strncpy (tTpa.tpaZiel.FeldId, FELDID_ZTL, FELDID_LEN);
		tTpa.tpaZiel.FeldId[FELDID_LEN] = 0;
		tTpa.tpaBewTyp = BEWTYP_UM;

		strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
		tTpa.tpaTpmId[TPMID_LEN] = '\0';

		lRet = TpaAnlegen (pvTid, &tTpa, pcFac);

		if (lRet < 0) {
			LogPrintf(pcFac, LT_ALERT,
					"CreateTPA for TeId [%s] failed",
					tTpa.tpaTeId);
			return (-1);
		}

		LogPrintf(pcFac, LT_DEBUG,
					"TPA for TeId [%s] from [%s] to '" FELDID_ZTL"' created" ,
					tTpa.tpaTeId,
					Pos2Str(pvTid, &atTek[nI].tekPos, NULL));

		lAnzTpa = 1;
	}

	return (lAnzTpa);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int _FreeFspUeb	(	void	*pvTid,
-*								char	*pcFac
-*							)
-*
-* DESCRIPTION
-*	Check UEB-Places for FSP
-*
-* RETURNS
-*     0 ... NotFound
-*	  -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _FreeFspUeb  (       void    *pvTid,
                                                const   char    *pcFac
                                                )
{
        int                     nI;
        long            lDbRv, lCntFsp;

        for (nI = 0; nI < (int)NO_ELE (matUebPlace); nI++) {

                if (matUebPlace[nI].iGlobIdx < 0) {
                        continue;
		}	

		if (matUebPlace[nI].iGlobIdx > MAX_UEBPLACE) {
			continue;
		}	

		if (IsEmptyStrg (matUebPlace[nI].acFspCheckPlace) == 1) {
			continue;
		}	

		lDbRv = TExecSql (pvTid,
                      "SELECT "
                      "   COUNT (FSP.Pos_FeldId) "
                      "FROM "
                      "   FSP "
                      "WHERE "
                      "   FSP.Pos_FeldId = :a",
                      SELLONG   (lCntFsp),
                      SQLSTRING (matUebPlace[nI].acFspCheckPlace),
                      NULL);

    	if (lDbRv <= 0 && TSqlError(pvTid) != SqlNotFound) {

        	LogPrintf (pcFac, LT_ALERT,
               	"Select FSP for '%s' failed! \n SQL: %s",
               	matUebPlace[nI].acFspCheckPlace, 
				TSqlErrTxt (pvTid));

        	return (-1);
    	}

		if (lCntFsp > 0) {
			C->ldctxt.ld.fspcheck[matUebPlace[nI].iGlobIdx] = 1;

        	LogPrintf (pcFac, LT_ALERT,
               	"FeldId '%s' with Idx '%d' is BLOCKED!",
				matUebPlace[nI].acFspCheckPlace,
				matUebPlace[nI].iGlobIdx);
		}
		else {

			C->ldctxt.ld.fspcheck[matUebPlace[nI].iGlobIdx] = 0;
		}
	}

	return (1);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int FreeCLUebh   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Creates TPA's for TE's which are on CL-UEBH-Places and have no TPA.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int FreeClUebh	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;


    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start:  Create ClUebh-Tpa");

        lRet = _FreeClUebh (pvTid,
                                                pcFacility
                                                );
	
	if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End:  Create ClUebh-Tpa");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int FreeWapUeb   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Creates TPA's for TE's which are on WAP-UEB-Places and have no TPA.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int FreeWapUeb	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Create TPA for 'WAP-Uebergabeplaetze'");

        lRet = _FreeWapUeb (pvTid, pcFacility);
        
        if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Create TPA for 'WAP-Uebergabeplaetze'");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int FreeWapNRP   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Creates TPA's for TE's which are on WAP-NoRead-Places and have no TPA.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int FreeWapNRP	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Create TPA for 'WAP-NoRead-Places'");

        lRet = _FreeWapNRP (pvTid, pcFacility);
        
        if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Create TPA for 'WAP-NoRead-Places'");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int FreeZtlUeb   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Creates TPA's for TE's which are on ZTL-UEB-Places and have no TPA.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int FreeZtlUeb	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Create TPA for 'ZTL-Uebergabeplaetze'");

        lRet = _FreeZtlUeb (pvTid, pcFacility);
        
        if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> End: Create TPA for 'ZTL-Uebergabeplaetze'");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*  static int FreeFspUeb   (   void            *pvCalldata,
-*								long            lTrigger,
-*								char            *pcMessage,
-*								T_CYCLEFUNCT    *ptFunc,
-*								T_CYCLETIME     *ptTime
-*							)
-* DESCRIPTION
-*	Creates TPA's for TE's which are on UEB-Places and have a FSP 
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int FreeFspUeb	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
        long    lRet=0;
        void    *pvTid=NULL;
        const char      *pcFacility=FAC_DBFDAEMON;

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Check UEB-Places for FSP");

        lRet = _FreeFspUeb (pvTid, pcFacility);
        
        if (lRet <= 0) 
	{
		TSqlRollback (pvTid);
	} else {
		TSqlCommit (pvTid);
	}

    LogPrintf(FAC_DBFDAEMON, LT_DEBUG,
              "=> Start: Check UEB-Places for FSP");

    return (0);
}

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-*
-* DESCRIPTION
-*	Function changes the TETID of LTabs from T-300 to T-160
-*
-* RETURNS
-*     0 ... NotFound
-*        -1 ... Error
-*     1 ... Ok
-*--------------------------------------------------------------------------*/
static int _ChangeLTabTet (void *pvTid, const char *pcFac)
{
        int                     nI=0;
        long            nlDbRv=0, lRet=0, lFirst=1;
	TEK			atTek[BLOCKSIZE];
	SqlContext	*ptSqlCtx = NULL;


	/*  Create Context  */
    ptSqlCtx = TSqlNewContext (pvTid, NULL);
    if (ptSqlCtx == NULL) 
	{
        LogPrintf (pcFac, LT_ALERT,
                    "_ChangeLTabTet: Function TSqlNewContext failed");
        return (-1);
    }


	do 
	{
		nlDbRv = TExecSqlX (pvTid, ptSqlCtx,
					lFirst == 1 ?
						"SELECT %TEK FROM TEK, FES, LAS "
						"WHERE TEK.Tet_TetId = '" TETID_T_300"' " 
						" AND TEK.TeId BETWEEN '" T_NUM_CIRC_VON"' AND " 
						" '" T_NUM_CIRC_BIS"' " 
						" AND NOT EXISTS "
						"("
						 "SELECT 1 FROM TEP WHERE TEP.TeId = TEK.TeId"
						") "
						" AND FES.FeldId = TEK.Pos_FeldId "
						" AND LAS.LagId = FES.LagId "
						" AND LAS.LAttr_FindLp = 1 "
						" FOR UPDATE OF TEK.Tet_TetId NOWAIT "
						: NULL,
					BLOCKSIZE, 0,
					SELSTRUCT 			(TN_TEK, atTek[0]),
					NULL);

		/* check to DB-error */
		if (nlDbRv < BLOCKSIZE)
        {
            if (TSqlError (pvTid) != 0 &&
				TSqlError (pvTid) != SqlNotFound)
            {
                TSqlDestroyContext (pvTid, ptSqlCtx);
                LogPrintf (pcFac, LT_ALERT,
                        "Fehler beim Select der TEK's: \n"
						"SQL <%s>",
						TSqlErrTxt (pvTid));

                return (-1);
            }
        }

		/* nothing found */
		if (nlDbRv <= 0) {
			break;
		}

		for (nI=0; nI < nlDbRv; nI++)
		{
			strncpy(atTek[nI].tekTet.TetId, TETID_T_160, TETID_LEN);
			atTek[nI].tekTet.TetId[TETID_LEN] = 0;

			SetHist(TN_TEK, (void*)&atTek[nI], HIST_UPDATE, 
					GetUserOrTaskName());
		}

		lRet = TExecStdSqlX(pvTid, NULL, StdNupdate, TN_TEK,
							 atTek, nlDbRv, NULL, NULL);

		if (nlDbRv != lRet) {
			TSqlDestroyContext (pvTid, ptSqlCtx);
			LogPrintf(pcFac, LT_ALERT,
				"Error Updating TEK: %s",
				TSqlErrTxt (pvTid));
			return (-1);
		}
		
		lFirst = 0;

	} while (BLOCKSIZE == nlDbRv);

  	TSqlDestroyContext (pvTid, ptSqlCtx);

	return (1);
}



/*---------------------------------------------------------------------------
-* SYNOPSIS
-*   $$ (automatically replaced with function prototype by autodoc-tool)
-*	
-* DESCRIPTION
-*	Calls subroutine for changing TETID of LTAB's.
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int ChangeLTabTet(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{
	long	lRet=0;


    LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
              "=> Start:  Change LTab-TetId");

        lRet = _ChangeLTabTet (NULL, FAC_DBSDAEMON);
        
        if (lRet <= 0) {
		TSqlRollback (NULL);
	} else {
		TSqlCommit (NULL);
	}

    LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
              "=> End:  Change LTab-TetId");

    return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-* int DelAllReadyAus_and_Prot(void *pvTid, SqlContext *hSqlCtxt,char *pcFac)
-* pvTid: TID
-* hSqlCtxt: Sql-Kontext
-* pcFac: Facility
-*
-* DESCRIPTION
-* Alle AUSK,AUSP mit Status='FERTIG' und Hoststatus='FERTIG' , deren Fertigzeit
-*	mehr als  1 Stunde her ist, werden gelï¿½scht
-* und protokollliert
-*
-* RETURNS
-* 	1  : OK
-* 	0  : Keine Daten gefunden
-* (-1): Fehler
-*--------------------------------------------------------------------------*/
static int DelAllReadyAus_and_Prot(void *pvTid, SqlContext *hSqlCtxt,
                                                                        const char *pcFac)
{
        int                     iDbrv   = 0, 
                                iRvProt = 0, 
				nI		= 0;
	long		lval_AuskStatus_FERTIG 		= 0,
				lval_AuskHoststatus_FERTIG  = 0,
				lval_AuspHoststatus_FERTIG  = 0,
				lval_AuspStatus_FERTIG		= 0,
				lval_AuspStatus_STORNO		= 0,
				lval_VplpStatus_FERTIG		= 0,
				lval_VplpStatus_STORNO		= 0,
				lval_VplkStatus_FERTIG		= 0,
				lval_VplkStatus_STORNO		= 0;
	long		lval_WapStatus_FERTIG 		= 0;
	time_t		zTimeBorder = time (NULL) - 3600;
	AUSK        atAusk[BLOCKSIZE_AUSK];

	lval_AuskStatus_FERTIG  	= AUSKSTATUS_FERTIG;
	lval_AuskHoststatus_FERTIG  = AUSKHOSTSTATUS_FERTIG;
	lval_AuspHoststatus_FERTIG  = AUSPHOSTSTATUS_FERTIG;
	lval_AuspStatus_FERTIG      = AUSPSTATUS_FERTIG;
	lval_AuspStatus_STORNO      = AUSPSTATUS_STORNO;
	lval_VplpStatus_FERTIG		= VPLPSTATUS_FERTIG;
	lval_VplpStatus_STORNO		= VPLPSTATUS_STORNO;
	lval_VplkStatus_FERTIG		= VPLKSTATUS_FERTIG;
	lval_VplkStatus_STORNO		= VPLKSTATUS_STORNO;
	lval_WapStatus_FERTIG		= WAPSTATUS_FERTIG;

	/* Facility eingetragen? */
    if(pcFac == NULL) { 
		pcFac = "DelAllReadyAus_and_Prot"; 
	}


	iDbrv = 0;
    do {

		memset(&atAusk[0], 0, sizeof(atAusk));

        iDbrv = (iDbrv == 0) ?
            TExecSqlX(pvTid, hSqlCtxt,
					"SELECT		%AUSK 										"
					"FROM 		AUSK 										"
					"WHERE 		AUSK.FertZeit   < :AuskFertZeit				"
					"	AND 	AUSK.Status 	= :AuskStatus_FERTIG		"
					"	AND 	AUSK.HostStatus = :AuskHoststatus_FERTIG 	"
					"	AND 	AUSK.WapStatus  = :Wapstatus_FERTIG 	    "
					"	AND		NOT EXISTS 									"
					"	 	    (											"
					"			SELECT 	AUSP.Status							"
					"	 		FROM	AUSP								"
					"	 		WHERE  AUSP.AusId_Mandant = AUSK.AusId_Mandant "
					"	 			AND AUSP.AusId_AusNr   = AUSK.AusId_AusNr "
					"	 			AND AUSP.AusId_AusKz   = AUSK.AusId_AusKz "
					"	 			AND AUSP.Status	 != :AuspStatus1_FERTIG	"
					"	 			AND AUSP.Status  != :AuspStatus_STORNO  "
					"	 		)											"
					"	AND		NOT EXISTS									"
					"	 	    (											"
					"			SELECT 	AUSP.Status							"
					"	 		FROM	AUSP								"
					"	 		WHERE  AUSP.AusId_Mandant = AUSK.AusId_Mandant "
					"	 			AND AUSP.AusId_AusNr   = AUSK.AusId_AusNr "
					"	 			AND AUSP.AusId_AusKz   = AUSK.AusId_AusKz "
					"	 			AND AUSP.HostStatus	 != 				"
					"								:AuspHostStatus_FERTIG	"
					"	 		)											"
					"	AND		NOT EXISTS									"
					"	 	    (											"
					"			SELECT 	VPLP.Status							"
					"	 		FROM	VPLP								"
					"	 		WHERE  VPLP.AusId_Mandant= AUSK.AusId_Mandant "
					"	 		  AND VPLP.AusId_AusNr   = AUSK.AusId_AusNr "
					"	 		  AND VPLP.AusId_AusKz   = AUSK.AusId_AusKz "
					"	 		  AND VPLP.Status != :VplpStatus_FERTIG "
					"	 		  AND VPLP.Status != :VplpStatus_STORNO "
					"	 		)											"
					"	AND		NOT EXISTS									"
					"	 	    (											"
					"			SELECT 	VPLK.Status							"
					"	 		FROM	VPLK								"
					"	 		WHERE  VPLK.AusId_Mandant= AUSK.AusId_Mandant "
					"	 		  AND VPLK.AusId_AusNr   = AUSK.AusId_AusNr "
					"	 		  AND VPLK.AusId_AusKz   = AUSK.AusId_AusKz "
					"	 		  AND VPLK.Status != :VplkStatus_FERTIG "
					"	 		  AND VPLK.Status != :VplkStatus_STORNO "
					"	 		)											",
                    BLOCKSIZE_AUSK, 0,
					SELSTRUCT(TN_AUSK, atAusk[0]),
					SQLTIMET	  (zTimeBorder),
					SQLAUSKSTATUS (lval_AuskStatus_FERTIG),
					SQLAUSKHOSTSTATUS (lval_AuskHoststatus_FERTIG),
					SQLWAPSTATUS (lval_WapStatus_FERTIG),
					SQLAUSPSTATUS (lval_AuspStatus_FERTIG),
					SQLAUSPSTATUS (lval_AuspStatus_STORNO),
					SQLAUSPHOSTSTATUS (lval_AuspHoststatus_FERTIG),
					SQLVPLPSTATUS (lval_VplpStatus_FERTIG),
					SQLVPLPSTATUS (lval_VplpStatus_STORNO),
					SQLVPLKSTATUS (lval_VplkStatus_FERTIG),
					SQLVPLKSTATUS (lval_VplkStatus_STORNO),
                    NULL) :

            TExecSqlV(pvTid, hSqlCtxt, NULL, NULL, NULL, NULL);

		/* Fehler */
		if ( (iDbrv <= 0) && (TSqlError(pvTid) != SqlNotFound) ) 
		{
			TSqlRollback (NULL);

            LogPrintf(pcFac, LT_ALERT,
            "Del_Aus:DelAllReadyAus_and_Prot -> Fehler beim Lesen der AUSK\n%s",
                TSqlErrTxt(pvTid));
            return (-1);
		}

		LogPrintf( pcFac, LT_ALERT, "found %d AUSK to prot", iDbrv );

		/* Keine Daten gefunden */
		if ( (iDbrv <= 0) && (TSqlError(pvTid) == SqlNotFound) ) 
		{
			TSqlRollback (NULL);
            return (0);
        }

		/*
		-* Auftrï¿½ge lï¿½schen und protokollieren: Es dï¿½rfen aber nur diejenigen
		-* Auftrï¿½ge gelï¿½scht werden, deren Fertigzeit ï¿½lter als 1 Stunde ist
		-*/
		for (nI = 0; nI < iDbrv; nI++) 
		{

			iRvProt =  TD_ProtAus(pcFac, NULL, &atAusk[nI], ProtAus_Delete);

			if (iRvProt < 0) 
			{

				TSqlRollback (NULL);

				LogPrintf(pcFac, LT_ALERT, "Del_Aus: TD_ProtAus: %s: \n%s", 
								GetErrmsg1(), GetErrmsg2());

				continue;
			}


			TSqlCommit (NULL);

        }/** FOR **/

    } while ( iDbrv == BLOCKSIZE_AUSK );

	TSqlRollback (NULL);

	return (1);
} /* DelAllReadyAus_and_Prot */


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*		static int Del_Aus (void	*pvCalldata, ...
-*
-* DESCRIPTION
-* Alle AUSK bzw. AUSP mit Status='FERTIG' und HostStatus='FERTIG' sollen
-* gelï¿½scht und protokolliert werden
-*
-* RETURNS
-*		-1 ... Error
-*		 1 ... Ok
-*		 0 ... Keine Daten gefunden
-*--------------------------------------------------------------------------*/
static int Del_Aus (void			*pvCalldata,
				   		  	 long			lTrigger,
				   		  	 char			*pcMessage,
				   		  	 CYCLEFUNCT		*ptCf,
				   		  	 CYCLETIME		*ptCt)
{

	SqlContext  *hSqlCtx = NULL;

	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "Del_Aus: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

        LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete Aus" );

        /* Sql-Kontext erstellen */
        hSqlCtx = TSqlNewContext(NULL, NULL);

	if (hSqlCtx == NULL) 
	{
		TSqlRollback (NULL);
		LogPrintf(FAC_DBSDAEMON, LT_ALERT,
			 "Del_Aus: Fehler bei TSqlNewContext():\n%s",
			  TSqlErrTxt (NULL));
		return (1);
	} /** IF (hSqlCtx) **/

	/*
	-* Löschen und protokollieren der AUSK,AUSP's
	-*/
	DelAllReadyAus_and_Prot(NULL, hSqlCtx,  FAC_DBSDAEMON);

	/* Sql-Kontext wieder lï¿½schen */
	TSqlDestroyContext(NULL, hSqlCtx);

	TSqlRollback (NULL);


	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete Aus" );

	return (1);
} /* Del_Aus */


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*		static int Del_Inv (void	*pvCalldata, ...
-*
-* DESCRIPTION
-* Alles INVK bzw. INVP mit Status='FERTIG' und HostStatus='FERTIG' sollen
-* gelï¿½scht und protokolliert werden
-*
-* RETURNS
-*		-1 ... Error
-*		 1 ... Ok
-*		 0 ... Keine Daten gefunden
-*--------------------------------------------------------------------------*/
static int Del_Inv (void			*pvCalldata,
				   		  	 long			lTrigger,
				   		  	 char			*pcMessage,
				   		  	 CYCLEFUNCT		*ptCf,
				   		  	 CYCLETIME		*ptCt)
{
	int		iRv	= 0;

	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "Del_Inv: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

        LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete Inv" );

        /*
        -* Lï¿½schen und protokollieren der INVK,INVP's
        -*/
	iRv = DelAllReadyInv(NULL, FAC_DBSDAEMON);
	if ( iRv <= 0 )  /* Fehler bzw. keine Daten gefunden */
	{  
		TSqlRollback (NULL);
	} else {
		TSqlCommit (NULL);
	}/** IF **/

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete Inv" );

	return (1);
} /* Del_Inv */


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*		static int Del_Verd (void	*pvCalldata, ...
-*
-* DESCRIPTION
-* Alles VERDK bzw. VERDP mit Status='FERTIG' oder 'STORNO' sollen
-* gelï¿½scht und protokolliert werden
-*
-* RETURNS
-*		-1 ... Error
-*		 1 ... Ok
-*		 0 ... Keine Daten gefunden
-*--------------------------------------------------------------------------*/
static int Del_Verd (void			*pvCalldata,
				  	 long			lTrigger,
				   	 char			*pcMessage,
				   	 CYCLEFUNCT		*ptCf,
				   	 CYCLETIME		*ptCt)
{
	int		iRv	= 0;

	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "Del_Verd: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

        LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete Verd" );

        /*
        -* Lï¿½schen und protokollieren der VERDK,VERDP's
        -*/
	iRv = DelAllReadyVerd (NULL, FAC_DBSDAEMON);
	if ( iRv <= 0 )  /* Fehler bzw. keine Daten gefunden */
	{  
		TSqlRollback (NULL);
	} else {
		TSqlCommit (NULL);
	}/** IF **/

	LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete Verd" );

	return (1);
} /* Del_Verd */


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*
-* DESCRIPTION
-*
-* RETURNS
-*--------------------------------------------------------------------------*/
static int _DelPseudoTe (void			*pvCalldata,
				 	 	 long			lTrigger,
				     	 char			*pcMessage,
				   	  	 CYCLEFUNCT		*ptCf,
				   	  	 CYCLETIME		*ptCt)
{
	int		iRv	= 0;

	/*************************************************************************
	 * dump trigger and triggermessage
	 ************************************************************************/
	if (lTrigger) 
	{
		LogPrintf ( FAC_DBSDAEMON,  LT_ALERT,
				   "_DelPseudoTe: triggered, message: %s",
				   pcMessage == NULL ? "NO MSG" : pcMessage );
	}

        LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> Start: Delete Pseudo-TE" );

        /*
        -* Lï¿½schen und protokollieren der Pseudo-TE's auf NO-Read
        -*/
	iRv = DelPseudoTeOnNoRead (NULL, FAC_DBSDAEMON);
	if ( iRv <= 0 )  /* Fehler bzw. keine Daten gefunden */
	{  
		TSqlRollback (NULL);
	} else {
		TSqlCommit (NULL);
	}/** IF **/

	LogPrintf( FAC_DBSDAEMON, LT_DEBUG, "=> End: Delete Pseudo-TE's" );

	return (1);
} /* _DelPseudoTe */

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Legt TPAs an: Testen Umlagerungen RBG4
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
#if 0
static int _TestDrvAuAbCOM13_NEU (void)
{
        int     iRv;
	char	acTeId[TEID_LEN + 1];
	long	lCntTpa;
	TEK		tTek;
	TPA 	tTpa;
	long	lSpalte;

	/* 
	 * 	Auftrag notwendig ?
     */

	iRv = TExecSql(NULL, "SELECT COUNT(*) FROM TPA WHERE "
						"AktPos_Feldid like 'CL13-%%' OR "
						"NextPos_Feldid like 'CL13-%%' OR "
						"TPMID IN ('" TPMID_COM13"') " ,
						SELLONG(lCntTpa),
						NULL);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT COUNT TPA: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (lCntTpa >= TESTDRVAUAB_RBG4_ANZ_TPA) {
		LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
					"# Auftrï¿½ge gefunden: [%ld] Es werden "
					"keine neuen angelegt.",
					lCntTpa);
		return (0);
	}

	/*
     *  Das im ZTL schon am lï¿½ngsten nicht mehr bewegte Tablar suchen
	 */

	memset(&tTek, 0, sizeof (TEK));

	iRv = TExecSql(NULL, "SELECT TeId, Spalte FROM ("
							"SELECT TEK.TeId, "
								"TEK.EinZeit lasttpa, "
								"FES.SPALTE "
							"FROM TEK,  FES WHERE "
							"TEK.Pos_FeldId = FES.FeldId AND "
							"FES.LagId = '" LAS_CL13"' AND " 
							"TEK.Pos_FeldId LIKE 'CL13-%%-%%-%%' AND "
							"TEK.TeId NOT IN (SELECT TeId FROM TPA) "
							"ORDER BY lasttpa"
						")",
						SELSTR(acTeId, TEID_LEN + 1),
						SELLONG(lSpalte),
						NULL);

	if (iRv <= 0 && TSqlError(NULL) != SqlNotFound) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT TEK, TPATP: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (iRv <= 0) {
		return (0);
	}

	memset(&tTek, 0, sizeof (TEK));

	strcpy(tTek.tekTeId, acTeId);

	iRv = TExecStdSql(NULL, StdNselect, TN_TEK, &tTek);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei StdNselect TEK [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));
		return (-1);
	}

	/*
     *  TPA anlegen mit Ziel NL2 --> Umlagerung
	 */

	memset(&tTpa, 0, sizeof (tTpa));

	tTpa.tpaQuelle = tTek.tekPos;
	tTpa.tpaAktpos = tTek.tekPos;

	strcpy(tTpa.tpaZiel.FeldId, FELDID_CL13);

	tTpa.tpaStatus              = TPASTATUS_NEU;
    tTpa.tpaCMD                 = TPACOMMAND_OK;
    strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
    tTpa.tpaTpmId [TPMID_LEN]   = '\0';

	tTpa.tpaTet                 = tTek.tekTet;
	strncpy (tTpa.tpaTeId, tTek.tekTeId, TEID_LEN);
	tTpa.tpaTeId [TEID_LEN]     = '\0';

	iRv = TpaAnlegen (NULL, &tTpa, FAC_DBSDAEMON);

	if (iRv < 0) {
		 
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei TpaAnlegen Teid: [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));

		return (-1);
	}

	LogPrintf (FAC_DBSDAEMON, LT_TRACE,
				"TPA angelegt: TaNr [%ld] ; TeId [%s] ; Quelle [%s] ; Ziel "
				"[%s]", 
				tTpa.tpaTaNr,
				tTpa.tpaTeId,
				tTpa.tpaQuelle.FeldId,
				tTpa.tpaZiel.FeldId);

        return (1);

}
#endif
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Legt TPAs an: Testen Umlagerungen RBG4
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
#if 0
static int _TestDrvAuAbCOM14_NEU (void)
{
        int     iRv;
	char	acTeId[TEID_LEN + 1];
	long	lCntTpa;
	TEK		tTek;
	TPA 	tTpa;
	long	lSpalte;

	/* 
	 * 	Auftrag notwendig ?
     */

	iRv = TExecSql(NULL, "SELECT COUNT(*) FROM TPA WHERE "
						"AktPos_Feldid like 'CL14-%%' OR "
						"NextPos_Feldid like 'CL14-%%' OR "
						"TPMID IN ('" TPMID_COM14"') " ,
						SELLONG(lCntTpa),
						NULL);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT COUNT TPA: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (lCntTpa >= TESTDRVAUAB_RBG4_ANZ_TPA) {
		LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
					"# Auftrï¿½ge gefunden: [%ld] Es werden "
					"keine neuen angelegt.",
					lCntTpa);
		return (0);
	}

	/*
     *  Das im ZTL schon am lï¿½ngsten nicht mehr bewegte Tablar suchen
	 */

	memset(&tTek, 0, sizeof (TEK));

	iRv = TExecSql(NULL, "SELECT TeId, Spalte FROM ("
							"SELECT TEK.TeId, "
								"TEK.EinZeit lasttpa, "
								"FES.SPALTE "
							"FROM TEK,  FES WHERE "
							"TEK.Pos_FeldId = FES.FeldId AND "
							"FES.LagId = '" LAS_CL14"' AND " 
							"TEK.Pos_FeldId LIKE 'CL14-%%-%%-%%' AND "
							"TEK.TeId NOT IN (SELECT TeId FROM TPA) "
							"ORDER BY lasttpa"
						")",
						SELSTR(acTeId, TEID_LEN + 1),
						SELLONG(lSpalte),
						NULL);

	if (iRv <= 0 && TSqlError(NULL) != SqlNotFound) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT TEK, TPATP: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (iRv <= 0) {
		return (0);
	}

	memset(&tTek, 0, sizeof (TEK));

	strcpy(tTek.tekTeId, acTeId);

	iRv = TExecStdSql(NULL, StdNselect, TN_TEK, &tTek);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei StdNselect TEK [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));
		return (-1);
	}

	/*
     *  TPA anlegen mit Ziel NL2 --> Umlagerung
	 */

	memset(&tTpa, 0, sizeof (tTpa));

	tTpa.tpaQuelle = tTek.tekPos;
	tTpa.tpaAktpos = tTek.tekPos;

	strcpy(tTpa.tpaZiel.FeldId, FELDID_CL14);

	tTpa.tpaStatus              = TPASTATUS_NEU;
    tTpa.tpaCMD                 = TPACOMMAND_OK;
    strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
    tTpa.tpaTpmId [TPMID_LEN]   = '\0';

	tTpa.tpaTet                 = tTek.tekTet;
	strncpy (tTpa.tpaTeId, tTek.tekTeId, TEID_LEN);
	tTpa.tpaTeId [TEID_LEN]     = '\0';

	iRv = TpaAnlegen (NULL, &tTpa, FAC_DBSDAEMON);

	if (iRv < 0) {
		 
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei TpaAnlegen Teid: [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));

		return (-1);
	}

	LogPrintf (FAC_DBSDAEMON, LT_TRACE,
				"TPA angelegt: TaNr [%ld] ; TeId [%s] ; Quelle [%s] ; Ziel "
				"[%s]", 
				tTpa.tpaTaNr,
				tTpa.tpaTeId,
				tTpa.tpaQuelle.FeldId,
				tTpa.tpaZiel.FeldId);

        return (1);

}
#endif
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Legt TPAs an: Testen Umlagerungen RBG4
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
#if 0
static int _TestDrvAuAbCOM15_NEU (void)
{
        int     iRv;
	char	acTeId[TEID_LEN + 1];
	long	lCntTpa;
	TEK		tTek;
	TPA 	tTpa;
	long	lSpalte;

	/* 
	 * 	Auftrag notwendig ?
     */

	iRv = TExecSql(NULL, "SELECT COUNT(*) FROM TPA WHERE "
						"AktPos_Feldid like 'CL15-%%' OR "
						"NextPos_Feldid like 'CL15-%%' OR "
						"TPMID IN ('" TPMID_COM15"') " ,
						SELLONG(lCntTpa),
						NULL);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT COUNT TPA: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (lCntTpa >= TESTDRVAUAB_RBG4_ANZ_TPA) {
		LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
					"# Auftrï¿½ge gefunden: [%ld] Es werden "
					"keine neuen angelegt.",
					lCntTpa);
		return (0);
	}

	/*
     *  Das im ZTL schon am lï¿½ngsten nicht mehr bewegte Tablar suchen
	 */

	memset(&tTek, 0, sizeof (TEK));

	iRv = TExecSql(NULL, "SELECT TeId, Spalte FROM ("
							"SELECT TEK.TeId, "
								"TEK.EinZeit lasttpa, "
								"FES.SPALTE "
							"FROM TEK,  FES WHERE "
							"TEK.Pos_FeldId = FES.FeldId AND "
							"FES.LagId = '" LAS_CL15"' AND " 
							"TEK.Pos_FeldId LIKE 'CL15-%%-%%-%%' AND "
							"TEK.TeId NOT IN (SELECT TeId FROM TPA) "
							"ORDER BY lasttpa"
						")",
						SELSTR(acTeId, TEID_LEN + 1),
						SELLONG(lSpalte),
						NULL);

	if (iRv <= 0 && TSqlError(NULL) != SqlNotFound) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT TEK, TPATP: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (iRv <= 0) {
		return (0);
	}

	memset(&tTek, 0, sizeof (TEK));

	strcpy(tTek.tekTeId, acTeId);

	iRv = TExecStdSql(NULL, StdNselect, TN_TEK, &tTek);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei StdNselect TEK [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));
		return (-1);
	}

	/*
     *  TPA anlegen mit Ziel NL2 --> Umlagerung
	 */

	memset(&tTpa, 0, sizeof (tTpa));

	tTpa.tpaQuelle = tTek.tekPos;
	tTpa.tpaAktpos = tTek.tekPos;

	strcpy(tTpa.tpaZiel.FeldId, FELDID_CL15);

	tTpa.tpaStatus              = TPASTATUS_NEU;
    tTpa.tpaCMD                 = TPACOMMAND_OK;
    strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
    tTpa.tpaTpmId [TPMID_LEN]   = '\0';

	tTpa.tpaTet                 = tTek.tekTet;
	strncpy (tTpa.tpaTeId, tTek.tekTeId, TEID_LEN);
	tTpa.tpaTeId [TEID_LEN]     = '\0';

	iRv = TpaAnlegen (NULL, &tTpa, FAC_DBSDAEMON);

	if (iRv < 0) {
		 
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei TpaAnlegen Teid: [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));

		return (-1);
	}

	LogPrintf (FAC_DBSDAEMON, LT_TRACE,
				"TPA angelegt: TaNr [%ld] ; TeId [%s] ; Quelle [%s] ; Ziel "
				"[%s]", 
				tTpa.tpaTaNr,
				tTpa.tpaTeId,
				tTpa.tpaQuelle.FeldId,
				tTpa.tpaZiel.FeldId);

        return (1);

}
#endif

/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Legt TPAs an: Testen Umlagerungen RBG4
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
#if 0
static int _TestDrvAuAbRBG4_NEU (void)
{
        int     iRv;
	char	acTeId[TEID_LEN + 1];
	long	lCntTpa;
	TEK		tTek;
	TPA 	tTpa;
	long	lSpalte;

	/* 
	 * 	Auftrag notwendig ?
     */

	iRv = TExecSql(NULL, "SELECT COUNT(*) FROM TPA WHERE "
						"AktPos_Feldid like 'NL2-%%' OR "
						"NextPos_Feldid like 'NL2-%%' OR "
						"TPMID IN ('" TPMID_RBG4"') " ,
						SELLONG(lCntTpa),
						NULL);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT COUNT TPA: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (lCntTpa >= TESTDRVAUAB_RBG4_ANZ_TPA) {
		LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
					"# Auftrï¿½ge gefunden: [%ld] Es werden "
					"keine neuen angelegt.",
					lCntTpa);
		return (0);
	}

	/*
     *  Das im ZTL schon am lï¿½ngsten nicht mehr bewegte Tablar suchen
	 */

	memset(&tTek, 0, sizeof (TEK));

	iRv = TExecSql(NULL, "SELECT TeId, Spalte FROM ("
							"SELECT TEK.TeId, "
								"TEK.EinZeit lasttpa, "
								"FES.SPALTE "
							"FROM TEK, FES WHERE "
							"TEK.Pos_FeldId = FES.FeldId AND "
							"FES.LagId = '" LAS_NL2"' AND " 
							"TEK.Pos_FeldId LIKE 'NL2-%%-%%-%%' AND "
							"TEK.TeId NOT IN (SELECT TeId FROM TPA) "
							"ORDER BY lasttpa"
						")",
						SELSTR(acTeId, TEID_LEN + 1),
						SELLONG(lSpalte),
						NULL);

	if (iRv <= 0 && TSqlError(NULL) != SqlNotFound) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei SELECT TEK, TPATP: %s", 
					TSqlErrTxt (NULL));
		return (-1);
	}

	if (iRv <= 0) {
		return (0);
	}

	memset(&tTek, 0, sizeof (TEK));

	strcpy(tTek.tekTeId, acTeId);

	iRv = TExecStdSql(NULL, StdNselect, TN_TEK, &tTek);

	if (iRv <= 0) {
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei StdNselect TEK [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));
		return (-1);
	}

	/*
     *  TPA anlegen mit Ziel NL2 --> Umlagerung
	 */

	memset(&tTpa, 0, sizeof (tTpa));

	tTpa.tpaQuelle = tTek.tekPos;
	tTpa.tpaAktpos = tTek.tekPos;

	strcpy(tTpa.tpaZiel.FeldId, FELDID_NL2);

	if (lSpalte <= 20 ) {
		strcpy(tTpa.tpaZiel.FeldId, FELDID_NL2_KP_5);
	} else if (lSpalte > 20 && lSpalte <= 40) {
		strcpy(tTpa.tpaZiel.FeldId, FELDID_NL2_KP_6);
	} else if (lSpalte > 40 && lSpalte <= 60) {
		strcpy(tTpa.tpaZiel.FeldId, FELDID_NL2_KP_7);
	} else {
		strcpy(tTpa.tpaZiel.FeldId, FELDID_NL2_KP_8);
	}

	tTpa.tpaStatus              = TPASTATUS_NEU;
    tTpa.tpaCMD                 = TPACOMMAND_OK;
    strncpy (tTpa.tpaTpmId, TPMID_LLS, TPMID_LEN);
    tTpa.tpaTpmId [TPMID_LEN]   = '\0';

	tTpa.tpaTet                 = tTek.tekTet;
	strncpy (tTpa.tpaTeId, tTek.tekTeId, TEID_LEN);
	tTpa.tpaTeId [TEID_LEN]     = '\0';

	iRv = TpaAnlegen (NULL, &tTpa, FAC_DBSDAEMON);

	if (iRv < 0) {
		 
		LogPrintf (FAC_DBSDAEMON, LT_ALERT,
					"Fehler bei TpaAnlegen Teid: [%s]: %s", 
					tTek.tekTeId,
					TSqlErrTxt (NULL));

		return (-1);
	}

	LogPrintf (FAC_DBSDAEMON, LT_TRACE,
				"TPA angelegt: TaNr [%ld] ; TeId [%s] ; Quelle [%s] ; Ziel "
				"[%s]", 
				tTpa.tpaTaNr,
				tTpa.tpaTeId,
				tTpa.tpaQuelle.FeldId,
				tTpa.tpaZiel.FeldId);

        return (1);

}
#endif
/*----------------------------------------------------------------------------
-* SYNOPSIS
-*      $$ (automatically replaced with function prototype by autodoc-tool)
-* DESCRIPTION
-*	Legt TPAs an: Testdrive umlagern
-*
-* RETURNS
-*     0 ... immer
-*--------------------------------------------------------------------------*/
static int TestDrvAuAb	(	void 			*pvCalldata,
                        	long 			lTrigger,
                        	char 			*pcMessage,
                        	T_CYCLEFUNCT 	*ptFunc,
                        	T_CYCLETIME 	*ptTime
						)
{

	int		iTestDriveRBG3 = JANEIN_N;
	int		iTestDriveRBG4 = JANEIN_N;
	int		iTestDriveRBG78 = JANEIN_N;
	int		iTestDriveCOM9 = JANEIN_N;
	int		iTestDriveCOM10 = JANEIN_N;
	int		iTestDriveCOM11 = JANEIN_N;
	int		iTestDriveCOM12 = JANEIN_N;
	int		iTestDriveCOM13 = JANEIN_N;
	int		iTestDriveCOM14 = JANEIN_N;
	int		iTestDriveCOM15 = JANEIN_N;
	int 	iRv;

	TESTDRVCTX tTestDrvCtx;

	LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> Start: TestDrvAuAb" );

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbRBG3,
                          PRM_CACHE, &iTestDriveRBG3) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbRBG3' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbRBG4,
                          PRM_CACHE, &iTestDriveRBG4) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbRBG4' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbRBG78,
                          PRM_CACHE, &iTestDriveRBG78) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbRBG78' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM9,
                          PRM_CACHE, &iTestDriveCOM9) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM9' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM10,
                          PRM_CACHE, &iTestDriveCOM10) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM10' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM11,
                          PRM_CACHE, &iTestDriveCOM11) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM11' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM12,
                          PRM_CACHE, &iTestDriveCOM12) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM12' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM13,
                          PRM_CACHE, &iTestDriveCOM13) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM13' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM14,
                          PRM_CACHE, &iTestDriveCOM14) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM14' failed!");
	}

	if (PrmGet1Parameter (NULL, P_TestDrvAuAbCOM15,
                          PRM_CACHE, &iTestDriveCOM15) != PRM_OK) {
        LogPrintf (FAC_DBSDAEMON,  LT_ALERT,
                   "TestDrvAuAb: Get parameter 'TestDrvAuAbCOM15' failed!");
	}

#ifdef ABARMET
	if (iTestDriveRBG3 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_RBG3);
		strcpy (tTestDrvCtx.acLagId, LAS_ZTL);

		tTestDrvCtx.lMinR = TESTDRVAUAB_RBG3_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_RBG3_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_RBG3_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_RBG3_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_RBG3_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_RBG3_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_RBG3_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
#endif /* ABARMET */

	if (iTestDriveRBG3 == JANEIN_J) {
		iRv = TestDrvAuAbRBG3_NEU();

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}

	if (iTestDriveRBG78 == JANEIN_J) {
		iRv = TestDrvAuAbRBG78();

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}

	if (iTestDriveRBG4 == JANEIN_J) {
#ifdef ABARMET
		iRv = _TestDrvAuAbRBG4_NEU();
#endif /*ABARMET */

		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_RBG4);
		strcpy (tTestDrvCtx.acLagId, LAS_NL2);

		tTestDrvCtx.lMinR = TESTDRVAUAB_RBG4_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_RBG4_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_RBG4_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_RBG4_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_RBG4_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_RBG4_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_RBG4_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 2;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}

	if (iTestDriveCOM9 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM09);
		strcpy (tTestDrvCtx.acLagId, LAS_CL9);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM9_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM10 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM10);
		strcpy (tTestDrvCtx.acLagId, LAS_CL10);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM10_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM11 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM11);
		strcpy (tTestDrvCtx.acLagId, LAS_CL11);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM11_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM12 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM12);
		strcpy (tTestDrvCtx.acLagId, LAS_CL12);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM12_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM13 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM13);
		strcpy (tTestDrvCtx.acLagId, LAS_CL13);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM13_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);
#ifdef ABARMET
		iRv = _TestDrvAuAbCOM13_NEU();
#endif
		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM14 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM14);
		strcpy (tTestDrvCtx.acLagId, LAS_CL14);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM14_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);
#ifdef ABARMET
		iRv = _TestDrvAuAbCOM14_NEU();
#endif /* ABARMET */

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}
	if (iTestDriveCOM15 == JANEIN_J) {
		memset (&tTestDrvCtx, 0, sizeof (tTestDrvCtx));

		strcpy (tTestDrvCtx.acTpmId, TPMID_COM15);
		strcpy (tTestDrvCtx.acLagId, LAS_CL15);

		tTestDrvCtx.lMinR = TESTDRVAUAB_COM15_MIN_REGAL;
		tTestDrvCtx.lMaxR = TESTDRVAUAB_COM15_MAX_REGAL;

		tTestDrvCtx.lMinS = TESTDRVAUAB_COM15_MIN_SPALTE;
		tTestDrvCtx.lMaxS = TESTDRVAUAB_COM15_MAX_SPALTE;

		tTestDrvCtx.lMinE = TESTDRVAUAB_COM15_MIN_EBENE;
		tTestDrvCtx.lMaxE = TESTDRVAUAB_COM15_MAX_EBENE;

		tTestDrvCtx.lMaxTpa = TESTDRVAUAB_COM15_ANZ_TPA;

		tTestDrvCtx.lLAMCorrValue = 0;

		iRv = TestDriveAuAbRBG (NULL, FAC_DBSDAEMON, &tTestDrvCtx);
#ifdef ABARMET
		iRv = _TestDrvAuAbCOM15_NEU();
#endif /* ABARMET */

		if (iRv <= 0) {
			TSqlRollback (NULL);
		} else {
			TSqlCommit (NULL);
		}
	}

	LogPrintf (FAC_DBSDAEMON, LT_DEBUG, "=> End: TestDrvAuAb" );

	return (0);
}

int dbsdaemon (int argc, char *const*argv)
{
	static  CYCLEFUNCT cf_dbsdaemon[] = {

		{{ 	1, 60, 0L, 0L, 	 0, "CheckInv"},   Check_Inv},
		{{ 	20, 3600, 0L, 0L, 	 0, FUNC_DBSDAEMON_DELETEINV},   Del_Inv},
		{{ 	20, 3600, 0L, 0L, 	 0, FUNC_DBSDAEMON_DELETEVERD},  Del_Verd},
		{{ 	2,  30, 0L, 0L, 	 0, FUNC_DBSDAEMON_DELETEAUS},   Del_Aus},
		{{ 	20, 3600, 0L, 0L, 	 0, FUNC_DBSDAEMON_DELNULLTEP},  DelNullTep},
		{{ 	20, 3600, 0L, 0L, 	 0, FUNC_DBSDAEMON_DELPSTE},  	_DelPseudoTe},
		{{ 	20, 3600, 0L, 0L, 	 0, "DELDIRTYAUS"},  			_QuitDirtyAus},
		{{  1, 	7000, 0L, 0L,    0, FUNC_DBSDAEMON_CHANGE_LTABTET},	 
																ChangeLTabTet},
		{{  1, 	600,  0L, 0L,    0, FUNC_DBSDAEMON_FREE_CLUEBH},  FreeClUebh},
#ifdef _NOT_NEEDED_ANYMORE_IN_KCL46
#ifdef __SALOMON_HOME__
		{{   1, 180,  0L, 0L,    0, FUNC_DBSDAEMON_SNDWETE2LVS}, SndWeTe2Lvs},
#else
		{{  20, 3600, 0L, 0L,    0, FUNC_DBSDAEMON_SNDWETE2LVS}, SndWeTe2Lvs},
#endif /* __SALOMON_HOME__ */
#endif /* _NOT_NEEDED_ANYMORE_IN_KCL46 */

		{{  5, 5, 0L, 0L,    0, FUNC_DBSDAEMON_TESTDRVAUAB}, TestDrvAuAb},
                {{      20, 3600, 0L, 0L,        0, FUNC_DBSDAEMON_DELEMPTYTEKTEP},  DelEmptyTekTep},
                {{      60, 3600, 0L, 0L,        0, "DELEMPTYTEKTEPAP"},  DelEmptyTekTepAp},

#ifdef TESTSYSTEM
          {{  1,  10,  0L, 0L,    0, FUNC_SIMVISI},  wamas::CfSimVisi},
#endif

/* NEXT DBSDAEMON FUNCTION */
        };

#ifdef UMLAUT_TEST_DIETER
        if (0) {

        int nI;
        char buf [100];

      int rv = ExecSql ("SELECT SUBSTR(BEZEICHNUNG,1,15) "
                                "FROM BSCHL WHERE buschl ='TEST'",
                SELSTR (buf, 15+1),
                NULL );

      if( rv < 0 ) {
          LogPrintf(FAC_DBSDAEMON, LT_ALERT, "SqlError: %s", TSqlErrTxt(NULL) );
      } else {
        for (nI = 0; nI < 15; nI++) {
                        LogPrintf (FAC_DBSDAEMON, LT_DEBUG,
                                                "Umlaut %c=%X\n",
						buf[nI],
                                                buf[nI]);
        }
    }
  }
#endif /* UMLAUT_TEST_DIETER */

        if (CfSet(sizeof(cf_dbsdaemon)/sizeof(cf_dbsdaemon[0]),cf_dbsdaemon) < 0) {
        fprintf(stderr, "dbsdaemon: Error in CfSet\n");
        TsProcExit(0);
    }

	for( ; TsStopRequest()==0; tsleep(800)) {

        /* Cykluszeit Funktionen */
                CfExec(sizeof(cf_dbsdaemon)/sizeof(cf_dbsdaemon[0]), cf_dbsdaemon);

    }

    TsProcExit(0);

        return (0);
}

int dbfdaemon (int argc, char *const*argv)
{
	static  CYCLEFUNCT cf_dbfdaemon[] = {
		{{  1,  30,  0L, 0L,    0, FUNC_DBFDAEMON_SAVEGLOB},     SaveGlobals},
		{{  5,  120, 0L, 0L,    0, FUNC_DBFDAEMON_SNDAUS2LVS},   SndAus2Lvs},
		{{  20, 120, 0L, 0L,    0, FUNC_DBFDAEMON_SNDINV2LVS},   SndInv2Lvs},
		{{  20, 30,  0L, 0L,    0, FUNC_DBFDAEMON_SETAUS4SND},	 SetAus4Snd},
		{{  1, 	15,  0L, 0L,    0, FUNC_DBFDAEMON_FREE_WAPUEB},	 FreeWapUeb},
		{{  1, 	15,  0L, 0L,    0, FUNC_DBFDAEMON_FREE_WAPNRP},	 FreeWapNRP},
		{{  1, 	15,  0L, 0L,    0, FUNC_DBFDAEMON_FREE_ZTLUEB},	 FreeZtlUeb},
		{{  1, 	120, 0L, 0L,    0, FUNC_DBFDAEMON_FREE_FSPUEB},	 FreeFspUeb},
		{{ 	20, 60,  0L, 0L, 	0, FUNC_DBSDAEMON_CBFL},  		_CBFL},
		{{ 	20, 60,  0L, 0L, 	0, "DBFDAEMON_RFCO"},  			_RFCO},
/* NEXT DBFDAEMON FUNCTION */
        };

        if (CfSet(sizeof(cf_dbfdaemon)/sizeof(cf_dbfdaemon[0]), cf_dbfdaemon) < 0) {
        fprintf(stderr, "dbfdaemon: Error in CfSet\n");
        TsProcExit(0);
    }

	for( ; TsStopRequest()==0; tsleep(800)) {

        /* Cykluszeit Funktionen */
                CfExec(sizeof(cf_dbfdaemon)/sizeof(cf_dbfdaemon[0]), cf_dbfdaemon);
    }

    TsProcExit(0);

        return (0);
}
