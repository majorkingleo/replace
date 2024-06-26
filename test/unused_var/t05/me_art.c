/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include <fstd20.h>

#include <tep.h>
#include <art.h>
#include <te_art.h>
#include <te_tep.h>
#include <hist.h>
#include <te_hist.h>
#include <db_util.h>
#include <sfac20.h>
#include <hist_util.h>
#include <futil20.h>

#include "pfwart.h"

#define MAX_LANGS 6

static int CompFct(Ptr l_elem,Ptr c_elem,Value calldata)
{
	EAN 	*b1=l_elem,*b2=c_elem;
	int		r;

	r = b1->eanTyp - b2->eanTyp;
	if(r != 0) 
		return r > 0 ? RETURN_LESS : RETURN_GREATER;
}


/* Alle Felder l�schen -> aufgerufen von cb_clrfld, cb_write, cb_delete 
-----------------------------------------------------------------------*/
static void ClearFieldsArt (MskTmaskRlPtr mask)
{
    ART         	*pArtNow = NULL;
    ART         	*pArtBefore = NULL;
	REC_DB			*mc = NULL;
	ContainerAtom 	a = NULL;
	ListBuffer 		lb = NULL;

	a = MskAtomGet(mask, NULL, ATOM_ART);
	lb = ListBufferInquireName(mask, "WamListBufferEAN", KEY_DEF);
	if (a && a->client_data && lb) {
		mc = (REC_DB *)a->client_data;

		mc->readBefore = 0;
		pArtNow = (ART *)mc->pRecNow;
		memset (pArtNow, 0, sizeof (ART));
		pArtBefore = (ART *)mc->pRecBefore;
		memset (pArtBefore, 0, sizeof (ART));
		ListBufferUnload(lb);
		ListBufferUpdate(lb);
		MskUpdateMaskVar (mask);
	}

	a = MskAtomGet( mask, NULL, ATOM_LANG );

	if( a && a->client_data )
	  {
		LANG *pLang = (LANG*)a->client_data;

		memset(pLang, 0, sizeof( LANG ) * MAX_LANGS );

		MskUpdateMaskVar (mask);
	  }
}


/* Static function for reading Positions from the DB - TEP
-----------------------------------------------------------------------*/
int Art_pos_lesen(MskTmaskRlPtr mask)
{
    int             rv,i;
    EAN             ean[500];
    ART         	*pArtNow = NULL;
	REC_DB			*mc = NULL;
	ContainerAtom	a = NULL;
	ListBuffer 		lb = NULL;

	a = MskAtomGet(mask, NULL, ATOM_ART);
	lb = ListBufferInquireName(mask, "WamListBufferEAN", KEY_DEF);
	if (a && a->client_data && lb) {
		mc = (REC_DB *)a->client_data;
		pArtNow = (ART *)mc->pRecNow;
    
		memset(ean, 0, sizeof(ean));

		strcpy(ean[0].eanUnit.unitArtNr, pArtNow->artArtNr);
		strcpy(ean[0].eanUnit.unitMandant, pArtNow->artMandant);		

    	rv = TExecStdSqlX(  mask, NULL, StdNselectUpdNo, TN_EAN, ean,
        	                500, "Unit_Mandant,Unit_ArtNr", NULL);

		TSqlRollback(mask);

    	if(rv == 500) {
	    	ApBoxAlert(SHELL_OF(mask), HSL_NI,
            	    "Krisenstimmung! Mehr als 500 Datens�tze mit ArtNr: %s!\n",
                	pArtNow->artArtNr);
    	}

		ListBufferUnload(lb);
	
    	for (i = 0; i < rv; i++) {
        	ListBufferAppendElement(lb,&ean[i]);
                if(i%2) {
                    ListBufferSetElementColor(lb,i,GrColorLock("LbBgWhite"));
                } else {
                    ListBufferSetElementColor(lb,i,GrColorLock("LbBgBlue"));
                }

    	}

	    ListBufferUpdate(lb);
	}
    return 0;
}

static int modified_ean(EAN *ean1, EAN *ean2)
{
	int rv=0;

	if (memcmp(ean1, ean2, sizeof(EAN))!=0)
			rv=1;

	return rv;
}		



static int WriteEan(MskTmaskRlPtr mask, EAN *ean, char *user)
{
	int rv, dbrv;
	EAN ean1;

	memcpy(&ean1, ean, sizeof(EAN));
    rv = TExecStdSql((void *)mask, StdNselectUpdNo, TN_EAN, &ean1);

    if (rv > 0) {
		if (modified_ean(ean, &ean1)) {
			SetHist(TN_EAN, (void *)ean, HIST_UPDATE, user);
			dbrv=TExecStdSql((void *)mask, StdNupdate,TN_EAN, ean);

			if (dbrv!=1) {
				ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
							"Fehler beim Schreiben der Verpackungsdaten!\n (EAN:'%s')", ean->eanEAN); 
				return -1;
			}
		}

	} else {
		if ( TSqlError(mask) != SqlNotFound) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
							"Fehler beim Lesen der Verpackungsdaten!"); 
			return -1;
		}

		if (!TDB_GetRecId((void *)mask, TN_EAN, ean->eanId)) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
						"Fehler beim Anlegen der Record Id!");
			return -1;
		}
 
		SetHist(TN_EAN, (void *)ean, HIST_INSERT, user);
		dbrv=TExecStdSql((void *)mask, StdNinsert,TN_EAN, ean);
		if (dbrv<=0) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
						"Fehler beim Ean '%s' schreiben!", ean->eanEAN); 
			return -1;
		}
	} 

	return 0;
}


static int DeleteEan(MskTmaskRlPtr mask, EAN *ean, char *user)
{
	int rv;
	EAN ean1;

 	memcpy(&ean1, ean, sizeof(EAN));
    rv = TExecStdSql((void *)mask, StdNselectUpdNo, TN_EAN, &ean1);
    if (rv > 0) {

		SetHist(TN_EAN, (void *)&ean1, HIST_DELETE, user);
    
		if (TExecStdSql((void *)mask, StdNdelete, TN_EAN, &ean1)<0) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
							"Fehler beim L�schen der Verpackungsdaten (EAN:'%s')!\n%s", ean1.eanEAN,TSqlErrTxt(mask)); 
			return -1;
		}
	} else {
		if ( TSqlError(mask) != SqlNotFound) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
							"Fehler beim Lesen der Verpackungsdaten!"); 
			return -1;
		}
	}
		
	return 0;
}



/* Static function for writing Positions to the DB ------------------------------------*/

static int pos_schreiben(MskTmaskRlPtr mask, ART *pArtNow, ListBuffer lb, char *user)
{
	EAN				*ean;
    int             i,a, rv;
	char			*pStr;
    long            last;
    ListElement     le;

    last = ListBufferLastElement(lb);
    for (i = 0;i <= last;i++) 
	{
        le = ListBufferGetElement(lb,i);
		ean = (EAN *)le->data;
		
		if (strcmp(ean->eanUnit.unitArtNr, pArtNow->artArtNr)) {
	        strcpy(ean->eanUnit.unitArtNr, pArtNow->artArtNr);
	        ean->eanId[0] = '\0';
	    }
		if (strcmp(ean->eanUnit.unitMandant, pArtNow->artMandant)) {
	        strcpy(ean->eanUnit.unitMandant, pArtNow->artMandant);
	        ean->eanId[0] = '\0';
	    }	    

		if (le->status == LIST_ELEM_DELETED) {
			if (DeleteEan(mask, ean, user)<0)
				return -1;
		}
    }

  /* In 2 parts because otherwise it is posible to delete one ean
	that you want to write (if there are 2 ean's with the same
	key, one to insert and one to delete */

    
    for (i = 0;i <= last;i++)   
	{
        le = ListBufferGetElement(lb,i);
		ean = (EAN *)le->data;

		if (le->status != LIST_ELEM_DELETED) {
			if (WriteEan(mask, ean, user) < 0) 
				return -1;
		}
    }
    return 0;
}

static int lang_write( MskTmaskRlPtr mask, ART *pArtNow, char *user )
{
  ContainerAtom	a=NULL;

  a = MskAtomGet( mask, NULL, ATOM_LANG );

  if( a && a->client_data )
	{
	  LANG *pLang = (LANG*)a->client_data;
	  int dbrv;
	  int i;

	  dbrv = TExecSql( mask, 
					   "delete from LANG where "
					   " mandant = :a "
					   " and artnr = :b ",
					   SQLSTRING( pArtNow->artMandant ),
					   SQLSTRING( pArtNow->artArtNr ),
					   NULL );

	  if( dbrv < 0 )
		{
		  LogPrintf( FAC_TERM, LT_ALERT, "SqlError: %s", TSqlErrTxt( mask ) );
		  return -1;
		}

	  for( i = 0; i < MAX_LANGS; i++ )
		{
		  if( Util_IsEmptyStr( pLang[i].langCode ) )
			continue;

		  strcpy( pLang[i].langMandant, pArtNow->artMandant );
		  strcpy( pLang[i].langArtNr, pArtNow->artArtNr );

		  dbrv = TExecStdSql( mask, StdNinsert, TN_LANG, &pLang[i] );

		  if( dbrv <= 0 )
			{
			  LogPrintf( FAC_TERM, LT_ALERT, "SqlError: %s", TSqlErrTxt( mask ) );
			  return -1;
			}
		}

	} else {

	  return -1;
	}

  return 0;
}

/* Hintergrundfarbe des markierten Elements auf weiss -----------------------------*/

void cb_list_Art(MskDialog mask, MskTgenericPtr ef, 
					MskTgenericRlPtr ef_rl, int reason, void *cbc)
{
    MskTcbcLb *cbc_lb;

    switch (reason) {
    case FCB_RF:
/*        WdgSet(ef_rl->ec_rl.w,WdgNselectCname, (Value)C_WHITE );
*/        break;
    }
}

/* ListBuffercallback -------------------------------------*/

static int cb_pos_edit(ListCbInfo li)
{
	REC_LB      *mcp;
	EAN			*ean, eanRec;
	int			last_index, kind;
	ListElement	le;
	MskDialog mask = NULL;
	OWidget shell = NULL;
	int rv=RETURN_ACCEPTED;
    ART         *pArtNow    = NULL;
	REC_DB	*mc = NULL;
	ContainerAtom a=NULL;
	
	switch (li->reason) {
	case LIST_REASON_ACTION:
	case LIST_REASON_NEW:
		a = MskAtomGet(li->mask_rl, NULL, ATOM_ART);
		if (a && a->client_data) {
			mc = (REC_DB *)a->client_data;
			pArtNow    = (ART *)mc->pRecNow;

			MskTransferMaskDup(li->mask_rl);

			if (!pArtNow->artMandant[0]) {
				ApBoxAlert(SHELL_OF(li->mask_rl), HSL_Not_Intended,
							"Mandant darf nicht leer sein!"); 
				set_focus_elem(li->mask_rl,"ART_Mandant_t"); 
				break;
			}
			if (!pArtNow->artArtNr[0]) {
				ApBoxAlert(SHELL_OF(li->mask_rl), HSL_Not_Intended,
							"ArtNr darf nicht leer sein!"); 
				set_focus_elem(li->mask_rl,"ART_ArtNr_t"); 
				break;
			}
			
			if (li->reason == LIST_REASON_NEW) {
				memset(&eanRec,0,sizeof(EAN));
				ean = (EAN *)&eanRec;
			} else {
				ean = (EAN *)li->l_elem->data;
			}
			strcpy(ean->eanUnit.unitMandant, pArtNow->artMandant);
			strcpy(ean->eanUnit.unitArtNr, pArtNow->artArtNr);			
			mcp = MemAllocate(sizeof(REC_LB),AllocStruct);
			shell = ApShellModalCreate(SHELL_OF(li->mask_rl),AP_CENTER, AP_CENTER);
			mask = (MskDialog )MskOpenMask(NULL, "WamDlgEAN");
			if (mcp && mask && shell) {
				memset( mcp, 0, sizeof(REC_LB) );
				mcp->mask_rl = li->mask_rl;
				mcp->lb = li->lb;
				mcp->ptr = (void *)ean;

				MskAtomAdd(mask, NULL, ATOM_EAN, NULL, 
										(Value )(REC_LB *)mcp);
				if (MskRcCreateDialog(shell, mask))
					WdgMainLoop();

				rv=RETURN_ABORT; 
			}
		}
		break;
	}
	return rv;
}

/* add position to ListBuffer 
-----------------------------------------------------------------------*/
static void cb_pos_add(MskTmaskRlPtr mask, MskTgenericPtr ef, 
						  MskElement ef_rl_but, int reason, void *cbc)
{
	ListBuffer lb=NULL;

	switch (reason) {
    case FCB_EF:                                    /* pressed button */
		MskTransferMaskDup(mask);
		lb = ListBufferInquireName(mask, "WamListBufferEAN", KEY_DEF);
        if (lb) {
			ListBufferExecNewElement(lb);
			ListBufferUpdate(lb);
		}
        break;
    }
}

/*---------------------------------------------------------------------
*					Ende ListBufferfunktionen				
----------------------------------------------------------------------*/

/*
 *  Funktion kopiert aus sold/10/db_util.c
 */
 
static int TDB_protArtEan_1 (void *tid, char *mand, char *artNr, char *pUser)
{
	int 	rv, dbrv, i, anz, sollMng;
	long 	anzahl = 0, tep_num = 0;
	ART 	art;
	EAN		arEan[NMAX_EAN];
	TEP		tep;

	LogPrintf(FAC_TERM,LT_NOTIFY,
			  "%s: protArtEan_1 M=%s A=%s",pUser,mand,artNr);

	memset (&art, 0, sizeof (ART));
	strncpy(art.artArtNr, artNr, ARTNR_LEN);
	strncpy(art.artMandant, mand, MANDANT_LEN);
	art.artMandant[MANDANT_LEN] = '\0';	
	art.artArtNr[ARTNR_LEN] = '\0';
	
	dbrv = TExecStdSql (tid, StdNselectUpdNo, TN_ART, (void *) &art);
	if (dbrv <= 0) {
		if ( TSqlError(tid) != SqlNotFound ) {
			return DB_UTIL_SQLFEHLER;
		} else {
			return DB_UTIL_INEXIST_ART;
		}
	}

	memset (&tep, 0, sizeof (TEP));
	strncpy(tep.tepUnit.unitMandant, mand, MANDANT_LEN);	
	strncpy(tep.tepUnit.unitArtNr, artNr, ARTNR_LEN);
	tep.tepUnit.unitMandant[MANDANT_LEN] = '\0';	
	tep.tepUnit.unitArtNr[ARTNR_LEN] = '\0';
	
	dbrv = TExecSql (tid, 
						"SELECT COUNT(*) FROM tep WHERE unit_Mandant=:a AND "
						"unit_ArtNr=:b",
						SELLONG(tep_num),
						SQLSTR(mand, MANDANT_LEN+1),
						SQLSTR(artNr, ARTNR_LEN+1),
						NULL);

	if (tep_num > 0) {
		return DB_UTIL_ART_IN_TEP;
	}

	memset (&arEan[0], 0, NMAX_EAN * sizeof (EAN));
	strncpy(arEan[0].eanUnit.unitArtNr, artNr, ARTNR_LEN);
	strncpy(arEan[0].eanUnit.unitMandant, mand, MANDANT_LEN);	
	arEan[0].eanUnit.unitArtNr[ARTNR_LEN] = '\0';
	arEan[0].eanUnit.unitMandant[MANDANT_LEN] = '\0';	

	dbrv = TExecStdSqlX (tid, NULL, StdNselectUpdNo, TN_EAN, (void *) &arEan[0],
						  NMAX_EAN, "Unit_Mandant,Unit_ArtNr", NULL);
	if (dbrv > 0) {
		/*
		 *	Alle Positionen l�schen + protokollieren -------------------------
		 */
		for (i = 0,anz = dbrv; i < anz; i++) {

			SetHist(TN_EAN, (void *)&arEan[i], HIST_DELETE, pUser);
		}
		/*
		 *	Positionen l�schen -----------------------------------------------
		 */
		dbrv = TExecStdSqlX (tid, NULL, StdNdelete, TN_EAN,
							 (void *) &arEan[0], anz, NULL, NULL);
		if (dbrv <= 0)
			return DB_UTIL_SQLFEHLER;

	} else if ( TSqlError(tid) != SqlNotFound ) {
		return DB_UTIL_SQLFEHLER;
	}

	SetHist(TN_ART, (void *)&art, HIST_DELETE, pUser);
	rv = TExecStdSql (tid, StdNdelete, TN_ART, (void *) &art);
	if (rv <= 0)
		return DB_UTIL_SQLFEHLER;

	return DB_UTIL_SQLOK;
}


/* Callback for Delete-Button
-----------------------------------------------------------------------*/
static void cb_delete_Art(MskTmaskRlPtr	mask, MskTgenericPtr ef,
							 MskTgenericRlPtr ef_rl, int reason, void *cbc)
{
	char			*pStr;
	OWidget     	w;
	int				rv;
    ART         	*pArtNow = NULL;
	REC_DB			*mc = NULL;
	ContainerAtom	a = NULL;
	ListBuffer		lb = NULL;
	char			user[USER_LEN+1];

	switch (reason) {
	case FCB_XF:

		a = MskAtomGet(mask, NULL, ATOM_ART);
		lb = ListBufferInquireName(mask, "WamListBufferEAN", KEY_DEF);
		if (a && a->client_data && lb) {
			set_focus_elem(mask,"ART_ArtNr_t"); 
			mc = (REC_DB *)a->client_data;
			pArtNow    = (ART *)mc->pRecNow;
	    
			MskTransferMaskDup(mask);
			out_name_spaces(pArtNow->artMandant);			
			out_name_spaces(pArtNow->artArtNr);
			if( !pArtNow->artMandant[0] ) 
				break;
			if( !pArtNow->artArtNr[0] ) 
				break;

			if( ToBoxCommit(SHELL_OF(mask),HSL_NI,
					" Artikel '%s' von Mandant '%s' l�schen ? ",pArtNow->artArtNr,pArtNow->artMandant) != IS_Ok)
				break;

			GetTermUser(user);
			
			rv = TDB_protArtEan_1((void *)mask, pArtNow->artMandant, pArtNow->artArtNr, user);
			switch (rv) {
			case DB_UTIL_SQLFEHLER:
				TSqlRollback(mask);
			    ApBoxAlert(SHELL_OF(mask), HSL_NI,
    	                    " Fehler beim Protokollieren der Artikel! ");
				break;
			case DB_UTIL_INEXIST_ART:
				TSqlRollback(mask);
			    ApBoxAlert(SHELL_OF(mask), HSL_NI,
    	                    " Kein ArtNr '%s' f�r Mandant '%s' im DB! ", pArtNow->artArtNr, pArtNow->artMandant);
				break;
			case DB_UTIL_ART_IN_TEP:
				TSqlRollback(mask);
			    ApBoxAlert(SHELL_OF(mask), HSL_NI,
    	                    " ArtNr '%s' f�r Mandant '%s' in TEP!\n "
							" L�schen nicht m�glich! ",pArtNow->artArtNr, pArtNow->artMandant);
				break;
			case DB_UTIL_SQLOK:
				TSqlCommit (mask);
				ClearFieldsArt (mask);
				break;
			}
		}
		break;
    default:
        break;	
	}
	return;
}

static int read_lang( MskTmaskRlPtr mask, ART *pArtNow )
{
  ContainerAtom	a=NULL;

  a = MskAtomGet( mask, NULL, ATOM_LANG );

  if( !a )
	{
	  LANG *pLang = NULL;

	  pLang = calloc( 6, sizeof( LANG ) );

	  MskAtomAdd( mask, NULL, ATOM_LANG, NULL,
				  (Value)pLang );

	  a = MskAtomGet( mask, NULL, ATOM_LANG );
	}

  if( a && a->client_data )
	{
	  LANG *pLang = (LANG*)a->client_data;
	  
	  if( !Util_IsEmptyStr( pArtNow->artMandant ) &&
		  !Util_IsEmptyStr( pArtNow->artArtNr ) )
		{
		  int dbrv = TExecSqlX( mask, NULL, 
								"Select %LANG from lang where "
								" mandant = :a "
								" and artnr = :b ",
								MAX_LANGS, 0, 
								SELSTRUCT( TN_LANG, pLang[0] ),
								SQLSTRING( pArtNow->artMandant ),
								SQLSTRING( pArtNow->artArtNr ),
								NULL );
		  
		  if( dbrv <= 0 && TSqlError( mask ) != SqlNotFound )
			{
			  LogPrintf( FAC_TERM, LT_ALERT, "Sql Error: %s",
						 TSqlErrTxt( mask ) );
			} else {
			  int i = 0;
			  char buffer[100];
			  
			  if( dbrv < 0 )
				dbrv = 0;
			  
			  if( dbrv < MAX_LANGS )
				memset( &pLang[dbrv], 0, sizeof( LANG ) * (MAX_LANGS-dbrv) );
			  
			  for( i = 0; i < MAX_LANGS; i++ )
				{												
				  snprintf( buffer, 100, "-LANG%d-", i+1 );
				  
				  MskVaAssignMatch( mask, buffer, 
									MskNvariableStruct, (Value) &pLang[i],
									MskNtransferVar2Dup, (Value)TRUE,
									MskNupdate,(Value)TRUE,
									NULL );
				}
			}
		  
		  MskUpdateMaskVar(mask);
		}
	} 

  return RETURN_ACCEPTED;
}

static int artikel_ok(MskTmaskRlPtr mask, ART *pArtNow)
{
	OWidget		w;
	ART 		Art;
	TEP 		*tep;
	int			rv=1, last, i, dbrv;
	FES			fes;
	
	if (mask && pArtNow) {

		out_name_spaces(pArtNow->artMandant);
		out_name_spaces(pArtNow->artArtNr);
		if( !pArtNow->artMandant[0] ) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
				"Mandant darf nicht leer sein! ");
			set_focus_elem(mask,"ART_Mandant_t"); 
			return 0;
		}
		if( !pArtNow->artArtNr[0] ) {
			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
				"ArtNr darf nicht leer sein! ");
			set_focus_elem(mask,"ART_ArtNr_t"); 
			return 0;
		}	
/*	 	if (!pArtNow->artRHDEin) {
 *			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
 *					"RHDEin darf nicht leer sein! ");
 *			set_focus_elem(mask,"ART_RHDEin_t"); 
 *			return 0;
 *		}
 *	 	if (!pArtNow->artRHDAus) {
 *			ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
 *					"RHDAus darf nicht leer sein! ");
 *			set_focus_elem(mask,"ART_RHDAus_t"); 
 *			return 0;
 *		}
 */
 		if (!(Util_IsEmptyStr(pArtNow->artLBereich))) {
			dbrv=TExecSql(mask,"select %FES from fes where feldid=:a",
							SELSTRUCT(TN_FES,fes),
							SQLSTR(pArtNow->artLBereich,LBEREICH_LEN+1),
							NULL);
			TSqlRollback(mask);
			if (dbrv<0) {
				ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
					"Ungueltiger Lagerbereich!\n "
					"Gueltige Werte: HRL-00-00-00, FW-00-00-00");
				set_focus_elem(mask,"ART_LBereich_t"); 					
				return 0;
			}
 		}
		return 1;
	}
	return 0;
}


/* Callback for Write-Button
-----------------------------------------------------------------------*/
static void cb_write_Art (MskTmaskRlPtr	mask, MskTgenericPtr ef, 
							 MskTgenericRlPtr ef_rl, int reason, void *cbc)
{
	OWidget			w;
	ART 			art;
	int 			rv, last, mode, Art_ok=1;
	int				brbFlag=0, ok_flag=1;
    ART         	*pArtNow = NULL;
	REC_DB			*mc = NULL;
	ContainerAtom	a=NULL;
	ListBuffer		lb=NULL;
	char			user[USER_LEN+1];
	
	switch (reason) {
	case FCB_XF:

		a = MskAtomGet(mask, NULL, ATOM_ART);
		lb = ListBufferInquireName(mask, "WamListBufferEAN", KEY_DEF);
		if (a && a->client_data && lb) {
			mc = (REC_DB *)a->client_data;
			pArtNow = (ART *)mc->pRecNow;
    
			MskTransferMaskDup(mask);

			if (artikel_ok(mask, pArtNow)) {

				if( ToBoxCommit(SHELL_OF(mask),HSL_NI,
						"Artikel '%s' f�r Mandant '%s' schreiben ? ",
						  pArtNow->artArtNr,pArtNow->artMandant) != IS_Ok)
					break;

				if (!pArtNow->artId[0]) {
					if (!TDB_GetRecId((void *)mask, TN_ART, pArtNow->artId)) {
						ApBoxAlert(SHELL_OF(mask), HSL_Not_Intended,
									" Fehler mit Record Id! ");
						set_focus_elem(mask,"ART_ArtNr_t");
						TSqlRollback(mask);
						break;
					}
				}
				if (WriteRec (mask, mc, TN_ART, SqlNroll, SiNmsg, FAC_ME_ART) == 0) {
				  int rv = -1;

				  GetTermUser(user);
				  rv = pos_schreiben(mask, pArtNow, lb, user );
				  
				  if( rv == 0 )
					lang_write( mask, pArtNow, user );

				  if ( rv == 0) {
					TSqlCommit (mask);
				  } else {
					TSqlRollback(mask);
				  }
				} 
				MskUpdateMaskVar(mask);
				set_focus_elem(mask,"ART_ArtNr_t");
			}
		}			
		break;
	default:
		break;
	}
	return;
}

/* Callback for Read-Button
-----------------------------------------------------------------------*/
static void cb_read_Art(MskTmaskRlPtr mask, MskTgenericPtr ef,
						   MskTgenericRlPtr	ef_rl, int reason, void	*cbc)
{
    ART         	*pArtNow = NULL;
    REC_DB			*mc=NULL;
    ContainerAtom	a=NULL;
	OWidget 		w;
	
	switch (reason) {
	case FCB_XF:
		a = MskAtomGet(mask, NULL, ATOM_ART);
		if (a && a->client_data) {
			mc = (REC_DB *)a->client_data;
			pArtNow = (ART *)mc->pRecNow;

			MskTransferMaskDup(mask);
			out_name_spaces(pArtNow->artMandant);			
			out_name_spaces(pArtNow->artArtNr);
			if(pArtNow->artMandant[0] && pArtNow->artArtNr[0]) {
				if (ReadRec(mask, mc, TN_ART, SqlNrollCom, SiNmsg, FAC_ME_ART) == 0 ) {
					Art_pos_lesen(mask);
					MskUpdateMaskVar(mask);
					set_focus_elem(mask,"ART_ArtNr_t"); 
				}
			}
		}

		read_lang( mask, pArtNow );


		break;
    default:
        break;
    }
	return;
}


/* Callback for Clear-Fields-Button
-----------------------------------------------------------------------*/
static void cb_clrfld_Art(MskTmaskRlPtr	mask, MskTgenericPtr ef,
							MskTgenericRlPtr ef_rl, int	reason, void *cbc)
{

	switch (reason) 
	{
		case FCB_XF:
/*			TSqlRollback(mask);
*/			ClearFieldsArt (mask);
			set_focus_elem(mask,"ART_ArtNr_t"); 
			break;
		default:
			break;
	}
	return;
}

/************************** Callback for artNr *************************************/

void cb_artNr(MskDialog mask, MskStatic ef, MskElement	el, int reason, void	*cbc)
{
	REC_DB			*mc = NULL;
	ART				*pArtNow = NULL;
	ContainerAtom	a = NULL;

	switch (reason) {
	case FCB_CH:

		a = MskAtomGet(mask, NULL, ATOM_ART);
		if (a && a->client_data) {
			mc = (REC_DB *)a->client_data;
			pArtNow = (ART *)mc->pRecNow;

			MskTransferMaskDup(mask);
			out_name_spaces(pArtNow->artMandant);			
			out_name_spaces(pArtNow->artArtNr);
    	    if(pArtNow->artMandant[0] && pArtNow->artArtNr[0] ) {
				if (ReadRec(mask, mc, TN_ART, SqlNrollCom, SiNsilent, FAC_ME_ART) == 0) {
					Art_pos_lesen(mask);
					MskUpdateMaskVar(mask);
				} else {
					pArtNow->artId[0] = '\0';
				}
			}
			read_lang( mask, pArtNow );
		}

		break;
    default:
        break;
    }
	return;
}

/* Mask-Callback
-----------------------------------------------------------------------*/
static int cb_mask_Art(MskDialog mask, int reason)
{
	REC_DB      	*mc = NULL;
    ART         	*pArtNow,*pArtBefore, *pArtInDb;
	int				rv=1;
	ContainerAtom	a = NULL;
	LANG            *pLang = NULL;
	
	switch (reason) {
	case MSK_CF:
		MskMatchEvaluate(mask,"*",0,MSK_MATCH_LOADEF);

		a = MskAtomGet(mask, NULL, ATOM_ART);

		if (a && a->client_data) {
			mc = (REC_DB *)a->client_data;
			ReadRec(mask, mc, TN_ART, SqlNrollCom, SiNmsg, FAC_ME_ART);
			pArtNow = (ART *)mc->pRecNow;
		} else {
			VaMultipleMalloc(   (int)sizeof(REC_DB), 	  (void**)&mc, 
    		                        (int)sizeof(ART),      &pArtBefore, 
        		                    (int)sizeof(ART),      &pArtNow, 
            		                (int)sizeof(ART),      &pArtInDb, 
                		            (int)(-1));

			memset( mc, 0, sizeof(REC_DB) );
			memset( pArtBefore, 0, sizeof(ART) );
			memset( pArtNow, 0, sizeof(ART) );
			memset( pArtInDb, 0, sizeof(ART) );
	
			mc->pRecBefore = pArtBefore;
			mc->pRecNow    = pArtNow;
			mc->pRecInDb   = pArtInDb;

			MskAtomAdd(mask, NULL, ATOM_ART, NULL, 
									(Value )(REC_DB *)mc);
		}
		MskVaAssignMatch(mask,"-ART-*",
			MskNvariableStruct,  (Value)pArtNow,
			NULL);
		
		read_lang( mask, pArtNow );

		out_name_spaces(pArtNow->artMandant);
		out_name_spaces(pArtNow->artArtNr);
		if (pArtNow->artMandant[0] && pArtNow->artArtNr[0])
			Art_pos_lesen(mask);

/*		TSqlRollback(mask);
*/		break;
#if 0
	case MSK_ESC:
/*		TSqlRollback(mask);
*/		break;
#endif
	case MSK_DM:
		TSqlRollback(mask);
		a = MskAtomGet( mask, NULL, ATOM_LANG );

		if( a && a->client_data )
		  {		  
			LANG *pLang = (LANG*) a->client_data;
			free( pLang );
			a->client_data = VNULL;
		  }

		MskAtomRemove( mask, NULL, ATOM_LANG, 0 );


		a = MskAtomGet(mask, NULL, ATOM_ART);
		if (a && a->client_data) {
			mc = (REC_DB *)a->client_data;
			if (mc) {
				MemDeallocate(mc);
				mc = NULL;
				a->client_data = VNULL;
			}
		}
		MskAtomRemove(mask,NULL,ATOM_ART,0);

		break;
	case MSK_EM:
/*		TSqlRollback(mask);
*/		break;
	default:
		break;
	}
	return rv;
}

/*
 * lb-transformer. EANTYP
 */
int kkCbXeanTyp(void **dest,void *src,Value calldata)
{
	*dest	= l2sGetNameByValue(&l2s_EANTYP, *(long *)src);
	return 0;
}

/*
 * lb-transformer. EANMAETYP
 */
int kkCbXmaeEanTyp(void **dest,void *src,Value calldata)
{
	*dest	= l2sGetNameByValue(&l2s_MAETYP, *(long *)src);
	return 0;
}


/*Arguments for the ListBuffer
Think about which data we want to show, This are no the correct ones*/

VaInfoRec args_ean[]={
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanEAN)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanTyp),kkCbXeanTyp, VNULL},
		{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanVol.volumeEinheit)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanMaeTyp),kkCbXmaeEanTyp, VNULL},
/*	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanUnit.unitMHD),  CbXtime2str, TTYPE_DATE},
 *	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanUnit.unitCharge)},
 */
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanUnit.unitMenge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanUnit.unitGewNetto), CbXlong2double, 3},
	{VA_TYPE_STRUCT,NULL,MemberOffset(EAN,eanGewBrutto), CbXlong2double, 3},

	{VA_TYPE_END}
};


DdTableRec table_numart[] = {
	{"args_ean",		(Value )args_ean},
	{"SizeofEAN",		(Value )sizeof(EAN)},
	{NULL}
};


DdTableRec table_cbart[] = {
	{"WamClrFldArt",		(Value )cb_clrfld_Art},
	{"WamReadArt",			(Value )cb_read_Art},
	{"WamWriteArt",		(Value )cb_write_Art},
	{"WamDeleteArt",		(Value )cb_delete_Art},
	{"WamDlgArt",			(Value )cb_mask_Art},
	{"WamPosAddArt",		(Value )cb_pos_add}, 
	{"WamListArt",			(Value )cb_list_Art},
	{"WamPosEditEan",		(Value )cb_pos_edit},
	{"WamCompFctEan",		(Value )CompFct}, 	
	{"cb_artNr",			(Value )cb_artNr},
	{NULL}
};

/*eof*/

