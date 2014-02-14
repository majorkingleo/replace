/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include <time.h>
#include <bstd20.h>
#include <dbsqlstd.h>
#include <slag20.h>
#include <tpa.h>
#include <ptpa.h>
#include <tpm.h>
#include "err_util.h"

#include "tpa_util.h"
#include "hist_util.h"
#include "db_util.h"
#include "sold20.h"

/* [endfold] */
/* [[ defines ]] */

#define BLOCKSIZE 500

/* [endfold] */
/* [[ globals ]] */

TPM	TPM_LLR={TPMIDX_LLS,1,0};

/* [endfold] */

/* [[ GetTpaNr ]] */

int GetTpaNr (void *tid)
{
    int  rv,tpanr,i=0;
	TPA tpa;

	for(;;i++) {
    	rv = TExecSql(tid, "SELECT TANR_SEQ.nextval FROM sys.dual",
						SELINT(tpanr),
						NULL);
    	if (rv <= 0)
       		return (-1);

		tpa.tpaTaNr=tpanr;
		rv=TExecStdSql(tid,StdNselect,TN_TPA,&tpa);
		if(rv<=0 && TSqlError(tid)!=SqlNotFound) {
			 return (-1);
		}
		if(rv<=0) {
			/* Nur wenn die TaNr noch nicht gibt*/
			 break;
		}
		if(i>=9999) {
			/* Ganzes Durchlauf keine freie TaNr gefunden */
			return (-1);
		}
	}

    return (tpanr);
}

/* [endfold] */
/* [[ TDB_tpaAnlegen ]] */

/* ------------------------------------------------------------
 * TPA anlegen
 */

int TDB_tpaAnlegen (void *tid, TPA *ptpa )
{
    int     tpa_nr, dbrv;
    int     i = 0, ret, no_tpa = -1;
	char	posstrg[POSSTRG_LEN+1];
	char	recid[RECID_LEN+1];

    ptpa->tpaHist.AnZeit = time((time_t *)0);
    ptpa->tpaStatus = TPASTATUS_NEU;
    if (strlen(ptpa->tpaHist.AnUser) == 0)
        strncpy(ptpa->tpaHist.AnUser,"UNKNOWN",USER_LEN);

	if( ptpa->tpaTpm.Typ == 0 )
		ptpa->tpaTpm = TPM_LLR;

	if (ptpa->tpaTpmId[0]=='\0' || ptpa->tpaTpmId[0]==' ')
		strcpy(ptpa->tpaTpmId, TPMID_LLS);
    for (i=0;i<9999;i++) {

        tpa_nr = GetTpaNr ( tid );
        if ( tpa_nr <= 0 ) {
			sprintf(errmsg1, "TANR_SEQ");
            return SUB_DBERR;
        }

		/* Auch RecId schreiben !!! */
		memset(recid,0,RECID_LEN+1);
		if (!TDB_GetRecId(tid, TN_TPA, recid))
		{
			sprintf(errmsg1, "RECID_SEQ");
			return SUB_DBERR;
		}

		strcpy(ptpa->tpaId,recid);
        ptpa->tpaTaNr = tpa_nr;
        ret = TExecStdSql (tid, StdNinsert, TN_TPA, (void *) ptpa);
        if ( ret <= 0) {
            if( TSqlError(tid) == SqlDuplicate ) {
				if( strstr(TSqlErrTxt(tid), "UN_"TN_TPA) ) {
					sprintf(errmsg1, "Fehler!");
					sprintf(errmsg2,
						"Palette %s hat schon einen Transportauftrag.",
						ptpa->tpaTeId);
					return SUB_IGNORE;
				}
                continue;
            }
			sprintf(errmsg1, TN_TPA);
			sprintf(errmsg2, "TeId %s, Quelle %s, Ziel %s", 
				ptpa->tpaTeId, Pos2Str(tid, &ptpa->tpaQuelle, NULL),
				Pos2Str(tid, &ptpa->tpaZiel, posstrg) );
			return SUB_DBERR;
        }

        return SUB_OK;
    }

	sprintf(errmsg1, "Schwerer Fehler!");
	sprintf(errmsg2, "Keine freie Transportauftragsnummer gefunden.");
    return SUB_ERR;
}

/* [endfold] */
/* [[ TDB_clear_tpa_FORCE ]] */

/* ------------------------------------------------------------
 * TPA wird auf aus DB geloescht, und eventuell existierende 
 * VZS-Auftraege werden ebenfalls storniert 
 * ACHTUNG: TPA wird nicht auf READY und STORNO gesetzt und
 * 			wie normalerweise vom OPT geloescht, sondern er wird  
 *			knallhart in dieser Funktion aus der DB geloescht
 *
 * 	SUB_ABORT ... TPA konnte nicht geloescht werden
 *	SUB_OK 	  ... TPA wurde geloescht
 */
int TDB_clear_tpa_FORCE (void *tid, TPA *ptpa )
{
	int				dbrv;
	int				aidx=0, count=0;
	char        	User[USER_LEN+1];



	/* TPA aus DB lesen */

	dbrv = TExecStdSql(tid, StdNselect, TN_TPA, ptpa);

	if (dbrv <= 0)
	{
		if (TSqlError(tid) == SqlNotFound)	
		{
			return SUB_OK;
		}
		else
		{
			sprintf(errmsg1, "Tpa konnte nicht gelesen werden !");
			return SUB_ABORT;
		}
	}


	memset(User, 0, sizeof(User));
	strncpy(User, "ClearForce", USER_LEN);


	/* TPA protokollieren und loeschen !!! */

	dbrv = TDB_protTpa (NULL, ptpa->tpaTaNr, User);

	if (dbrv != DB_UTIL_SQLOK)
    {
		sprintf(errmsg1, "Tpa konnte nicht geloescht werden !");
		return SUB_ABORT;
    }



	return SUB_OK;

}

/* Fuer TE mit Aktpos einen TPA generieren bzw. einen vorhandenen 
   weiterverwenden */
/*****************************************************************
* SYNOPSIS
*      int GenTa4TeWithAktpos(void *tid, char *TeId, POS *Aktpos, TET Tet)
* DESCRIPTION
*      generiert fuer eine TE mit Aktpos 22.8, 22.9, 44.3, 51.1 einen
*	   TPA oder aendert einen bereits vorhandenen (mit Status IDLE) ab 
* RETURNS
*   SUB_OK      TPA angelegt (ALLES OK)	
*   SUB_ABORT   Fehler beim Anlegen des TPAs   
******************************************************************/

int GenTa4TeWithAktpos(void *tid, char *TeId, POS *Aktpos, TET *Tet)
{
	int 			i, rv, ret;
	int				last;
	TPA       		tpa;
	TEK      		tek;

	memset(&tek, 0, sizeof(tek));
	memset(&tpa, 0, sizeof(tpa));

	strncpy(tek.tekTeId, TeId, TEID_LEN);

	if (TExecStdSql(tid, StdNselectUpdNo, TN_TEK, &tek) <= 0) 
	{
		if (TSqlError(tid) != SqlNotFound) {

			sprintf(errmsg1, "Fehler beim Lesen der TE[%s] aus der Datenbank!",
							 tek.tekTeId);
			return SUB_ABORT;

		}

		sprintf(errmsg1, "TE[%s] in Datenbank nicht angelegt !!",
						 tek.tekTeId);
		return SUB_ABORT;

	}


	/* Checken ob Position der TE Ok oder nicht schon z.B. AL */

	if (strncmp(tek.tekPos.FeldId, "AL", 2) == 0)
	{
		sprintf(errmsg1, "TE[%s] schon im AL verbucht !!", tek.tekTeId);
		return SUB_ABORT;
	}

	
	
	/* --- bestimmen des TE-Typs --- */

	if (Tet != NULL)
	{
		tek.tekTet = *Tet;

		if (tek.tekTet.Vol.L[KOO_Z] <= TE_Z_VOL_N)
		{
			tek.tekTet.Vol.L[KOO_Z] = TE_Z_VOL_N;
		}
		else
		{
			tek.tekTet.Vol.L[KOO_Z] = TE_Z_VOL;
		}
	}

	tek.tekPos = *Aktpos;

	if ( TExecStdSql (tid, StdNupdate, TN_TEK, &tek) <= 0 ) {
		sprintf(errmsg1, "Fehler beim UPDATEN der TE[%s] !!",
						 tek.tekTeId);
		return SUB_ABORT;
	}


	memset(&tpa, 0, sizeof(tpa));

	/* --- Ueberpruefung ob es schon einen LLR-TPA gibt --- */

	if ( TExecSql (tid, "SELECT %TPA FROM TPA WHERE TPA.TeId=:TeId",
						 SELSTRUCT (TN_TPA, tpa),
						 SQLSTRING (tek.tekTeId),
						 NULL) < 0 ) {

		if ( TSqlError(tid) != SqlNotFound ) {
			sprintf(errmsg1, "Fehler beim Lesen des TPAs fuer TE[%s] !!",
							 tek.tekTeId);
			return SUB_ABORT;
		}
		else if ( TSqlError(tid) == SqlNotFound ) 
		{

			/* --- Generiere LLR-Transportauftrag --- */

			tpa.tpaQuelle = *Aktpos;
			tpa.tpaAktpos = *Aktpos;
			tpa.tpaNextpos = *Aktpos;
			tpa.tpaZiel = *Aktpos;


			strcpy(tpa.tpaTeId, tek.tekTeId);
			tpa.tpaTet =  tek.tekTet;

			if ( TDB_tpaAnlegen (tid, &tpa) <= 0 ) {
				sprintf(errmsg1, "Fehler beim Anlegen des neuen TPA's fuer TE [%s]!!", tek.tekTeId);
				return SUB_ABORT;

			}	

		}

	} else {

		/* --- geh"ort der Transportauftrag der VZS ? --- */

		switch (tpa.tpaStatus) {
			case TPASTATUS_IDLE:

			default:

				sprintf(errmsg1,"Es existiert bereits TPA fuer TeId: [%s],"
								"der bereits aktiv ist !! ",
								tek.tekTeId);
				return SUB_ABORT;

		}

	}

	return SUB_OK;
}

/* [endfold] */
/* [[ TDB_tpaDefault ]] */

/*
=head1 leere TPA-Struktur anlegen
=head2 Signatur
   

=head2 Beschreibung
fuellt eine TPA-Struktur im Speicher mit Werten fuer einen
Standard-Tpa. Es wird nichts mit der Datenbank rumgenudelt, dafuer ist
der Aufrufer verantwortlich (z.B. mit C<TDB_tpaAnlegen()>).

=head2 Argumente
=over 3
=item tpa
Zeiger auf einen TPA-Record. Wenn hier NULL uebergeben wird, dann wird
der Record dynamisch alloziert.
=back

=head2 Rueckgabewert
liefert einen Zeiger auf den gefuellten TPA-Record oder NULL, wenn ein
Fehler passiert sein sollte (unwahrscheinlich). Wurde NULL uebergeben,
dann muss der retournierte Zeiger irgendwann wieder einmal freigegeben
werden.
*/
TPA *TDB_tpaDefault (TPA *itpa)
{
	TPA *tpa;
	
	if(itpa==NULL) {
		tpa=malloc(sizeof(TPA));
		if(tpa==NULL) {
			return NULL;
		}
	} else {
		tpa=itpa;
	}

	memset(tpa,0,sizeof(TPA));
	SetHist(TN_TPA,tpa,HIST_INSERT,NULL);
	tpa->tpaCMD=TPACOMMAND_OK;
	tpa->tpaStatus=TPASTATUS_NEU;	
	tpa->tpaTyp=TPATYP_AUSLAG;
	tpa->tpaTpm.Typ=TPMIDX_LLS;
	strcpy(tpa->tpaTpmId, TPMID_LLS);
	tpa->tpaZTyp=TPAZIELTYP_STD;
	strncpy(tpa->tpaTet.TetId,"EURO-1",TETID_LEN+1);
	memset(tpa->tpaTeId,' ',TEID_LEN);

/*
	strncpy(tpa->tpaQuelle.FeldId,FELDID_UNBEKANNT,FELDID_LEN+1);
	strncpy(tpa->tpaAktpos.FeldId,FELDID_UNBEKANNT,FELDID_LEN+1);
	strncpy(tpa->tpaNextpos.FeldId,FELDID_UNBEKANNT,FELDID_LEN+1);
	strncpy(tpa->tpaZiel.FeldId,FELDID_UNBEKANNT,FELDID_LEN+1);
*/

	return tpa;
}

/* [endfold] */

