/*
 *  System        : 
 *  Module        : 
 *  Object Name   : $RCSfile: tourpl.c,v $
 *  Revision      : $Revision: 1.1 $
 *  Date          : $Date: 2009/10/28 09:03:46 $
 *  Author        : $Author: wamas $
 *  Created By    : Stefan Sibitz
 *  Created       : Wed Mar 29 16:56:37 2000
 *  Last Modified : <:00329.1656>
 *
 *	A New Dialog with a Tree-View Element to transfer the Mak's from 
 *  	One Tour to an other or to a new created. 
 * 
 *
 *	Copyright (c) 2000 by ZoSo FSG
 *	All Rights Reserved. Alle Rechte Vorbehalten.
 *
 *	This  document  may  not, in  whole  or in  part, be  copied,  photocopied,
 *	reproduced,  translated,  or  reduced to any  electronic  medium or machine
 *	readable form without prior written consent from ZoSo FSG.
 *
 * 
 */

#include <dbsqlstd.h>
#include <owil.h>
#include "pftour.h"
#include <svar.h>
#include <ma_stat.h>
#include <db_util.h>
#include <hist_util.h>
#include <ma_split.h>
#include <me_util.h>
#include <futil.h>
#include <ma.h>
#include <sfac.h>
#include <logtool2.h>
#include <mumalloc.h>
#include <tools.h>
#include <sutil.h>
#include <cpp_util.h>
#include <wamasexception.h>
#include <WamasSqlReadListSimpleArgs.h>
#include <WamasSqlEditListSimple.h>
#include <WamasSqlExecArgs.h>
#include <blockupdate.h>
#include <proc.h>
#include <cycle.h>
#include <diff_tables.h>
#include <wamaswdg.h>
#include "tourpl.h"
#include <string_utils.h>

using namespace Tools;

#define DLGNAME_TPL "WamDlgttpl" 			/* Name of Dialog Tourplanning */
#define DLGNAME_TPL_NAME "WamDlgTourName" 	/* Name of Dialog New TourName */
#define TREENAME_OLD "Tree_Old"				/* Name of the Old Tree */
#define TREENAME_NEW "Tree_New"				/* Name of the New Tree */

/*#define MAX_BUFFER_CNT_TPL 5000		*/		/* Maximum Count of Record read by one fetch */
#define MAX_BUFFER_CNT_TPL 200				/* Maximum Count of Record read by one fetch */

#define TOURPL_EXIST_NO 0
#define TOURPL_EXIST_YES 1 

/* Some simple macro's */
#define CLIENT_DATA(x) (x)MskDialogGet(mask,MskNmaskClientData)
#define CALL_DATA(x) (x)MskDialogGet(mask,MskNmaskCalldata)
#define USER_DATA(x) (x)MGrObjGet(grobj,MGrNuserdata)
#define GET_MASK(x) (MskDialog )MskElementGet(x,MskNdialog)
#define APBOX(x) ApBoxAlert(SHELL_OF(mask),0,x)
#define COBOX(x) ApBoxCommit(SHELL_OF(mask),HSL_Not_Intended,x)

/* defines for logprintf */
#define FAC_ME_TOURPL "tourpl"

static std::string AuftrName( const MAK *pmak )
{
	return format( "%s (%s) %s", pmak->makRefNr, pmak->makUrTourNr, pmak->makKLName );
}

int tourpl_auto_freigabe(const std::string & fac, void *tid,MAM *mam,MAK *mak)
{
	int rv=RETURN_ACCEPTED;
	int dbrv=0;
	const char *gpzeit=NULL;
	// int tornr=0;
	char ziel[LBEREICH_LEN+1];
	
	/* 
	 *  Set default values 
	 */
	
	/* set startgp time if not */
	if ((rv==RETURN_ACCEPTED) && 
		(mak->makStartGP==0)) {
		gpzeit=Var_GetString("GP_FreigabeZeit","Sofort");
		if (strcmp(gpzeit,"Sofort")==0)
			mak->makStartGP=time(NULL);
		else if (strcmp(gpzeit,"Komm")==0)
			mak->makStartGP=mak->makKomZeit;
	}		
	
	/* Get the destiny from a function */
	if (rv==RETURN_ACCEPTED) {
/*		memset(ziel,0,strlen(ziel));*/
		memset(ziel,0,sizeof(ziel));
		dbrv=TDB_GetMakZiel(tid,mak,ziel);
		rv=(dbrv==TDB_MASTAT_OK)?RETURN_ACCEPTED:RETURN_ERROR;
		
		if (rv==RETURN_ACCEPTED) {
			StrCpy(mak->makTorBereich,ziel);
		}
	}
	
	/* Set the kommzeit if not set */
	if ((rv==RETURN_ACCEPTED) && 
		(mak->makKomZeit==0)) {
		gpzeit=Var_GetString("Llr.Freigabe_KomZeit","Sofort");
		if (strcmp(gpzeit,"Sofort")==0)
			mak->makKomZeit=time(NULL);
		else if (strcmp(gpzeit,"Verlade")==0)
			mak->makKomZeit=mak->makLiefZeit;
		if (mak->makKomZeit==0) {
			mak->makKomZeit=time(NULL);
		}
	}
	
	/* Update status of mam */
	if (rv==RETURN_ACCEPTED) {
		/* Update mam-status */
		dbrv=TExecSql(tid,
					  "update mam set prestatus="STR_MASTAT_FREIGABE
					  " where rectyp="STR_RECTYP_MAK
					  " and parentid=:a and mandant=:b"
					  " and maetyp="STR_MAETYP_PK,
					  SQLSTRING(mak->makId),
					  SQLSTRING(mak->makMandant),
					  NULL);
		rv=(dbrv>0)?RETURN_ACCEPTED:RETURN_ERROR;
		/* Update mak */
		if (rv==RETURN_ACCEPTED) {
			dbrv=TExecSql(tid,
						  "update mak set komzeit=:a,liefzeit=:b, "
						  "startgp=:c where id=:d",
						  SQLTIMET(mak->makKomZeit),
						  SQLTIMET(mak->makLiefZeit),
						  SQLTIMET(mak->makStartGP),
						  SQLSTRING(mak->makId),
						  NULL);
			rv=(dbrv>0)?RETURN_ACCEPTED:RETURN_ERROR;
		}
	}
	
	/* Write a log entry */
	LogPrintf(fac,LT_NOTIFY,
			  " Auftrag: %s,Tournr '%s',Status:%s,Prestatus:%s, rv=%d",
			  mak->makRefNr,
			  mak->makTourNr,
			  l2sGetNameByValue(&l2s_MASTAT,mam->mamStatus),
			  l2sGetNameByValue(&l2s_MASTAT,mam->mamPreStatus),
			  rv
			  );
	
	return rv;
}

int tourpl_updateKaks( void *tid,MAK *mak)
{
  /* update KAKs */

    int dbrv = TExecSql( tid, "update " TN_KAK " set " TCN_KAK_TourNr " = :tournr where "
                    TCN_KAK_MakId " = :id ",
                    SQLSTRING( mak->makTourNr ),
                    SQLSTRING( mak->makId ),
                    NULL );

    if( dbrv < 0 && TSqlError( tid ) != SqlNotFound ) {
        throw SQL_EXCEPTION( tid );
    }

    LogPrintf( FAC_ME_TOURPL, LT_ALERT, "updated %d KAKs", dbrv );

    return dbrv;
}

int tourpl_auto_freigabe_tour(const std::string & fac, void *tid,MAM *mam,MAK *mak)
{
    int dbrv=0;


    std::vector<MAM> vMAM;
    wamas::wms::WamasSqlReadListSimpleArgs (tid,
                                            fac.c_str(),
                                            TN_MAM,
                                            vMAM,
                                            "     " TCN_MAM_ParentId " = :tournr "
                                            " and " TCN_MAM_Mandant " = :mandant ",
                                            " " TCN_MAM_ParentId " for update nowait ",
                                            SQLSTRING(mak->makTourNr),
                                            SQLSTRING(mak->makMandant),
                                            0);

    for( unsigned i = 0; i < vMAM.size(); i++ )
      {
        MAM & mam = vMAM[i];

        if( mam.mamMaeTyp == MAETYP_PK )
          {
            mam.mamPreStatus = MASTAT_FREIGABE;
          }

        mam.mamStatus = MASTAT_FREIGABE;
      }


    dbrv = BlockTExecStdSqlX( tid, fac, StdNupdate, TN_MAM, vMAM );

    if( dbrv <= 0 ) {
        LogPrintf( fac, LT_ALERT, "update Fehlgeschlagen. dbrv %d parentid: %s mandant: %s",dbrv, mam->mamParentId, mam->mamMandant );
        throw SQL_EXCEPTION( tid );
    }

    /* Write a log entry */
    LogPrintf(fac,LT_NOTIFY,
              " Tour Freigegeben Auftrag: %s,Tournr '%s'",
              mak->makRefNr,
              mak->makTourNr
              );

    return RETURN_ACCEPTED;
}

/* 
 * Selections Callback's of the Old Canvas Tree on the left side
 * If you select an entry the type will be set to ORDER or TOUR 
 * and the recid will be stored in recid_old ( mam.ParentId or mak.Id )
 * 
 * If you diselect an entry the type will be set to UNDEF
 * and the recid_old will be cleaned.
 * 
 * If you delete an entry the memory will be given free !
 * 
 */
void cb_oldtree(OwObject grobj,int reason)
{
	MskElement el=NULL;
	MskDialog mask=NULL;
	FTourDataRec *cd=NULL;
	FTourmcRec *mc=NULL;
	
	switch(reason) {
	case MGR_TREE_SELECT:
		el = MGrObjDataFindMskElement(grobj);
		if (el) {
			mask = GET_MASK(el);
		}
		if (mask) {
			mc = CLIENT_DATA(FTourmcRec *);
			cd  = USER_DATA(FTourDataRec *);
			if(cd && mc) {
				mc->grobj_old=grobj;
				mc->sel_old=cd->typ;
			}
		}
		break;
	case MGR_TREE_UNSELECT:
		el = MGrObjDataFindMskElement(grobj);
		if (el) {
			mask = GET_MASK(el);
			if (mask) {
				mc = CLIENT_DATA(FTourmcRec *);
				if (mc) {
					mc->sel_old=UNDEF;
					mc->grobj_old=NULL;
				}
			}
		}
		break;
	case MGR_TREE_DESTROY:
		cd = USER_DATA(FTourDataRec *);
		if (cd) {
			MGrObjSet(grobj,MGrNuserdata,VNULL);
			cd = (FTourDataRec*)MemDealloc(cd);
		}
		break;
	}
}

/* 
 * Selections Callback's of the New Canvas Tree on the right side
 * If you select an entry the type will be set to ORDER or TOUR 
 * and the recid will be stored in recid_new ( mam.ParentId or mak.Id )
 * 
 * If you diselect an entry the type will be set to UNDEF 
 * and the recid_new will be cleaned.
 * 
 * If you delete an entry the memory of the entry will be given free 
 * 
 */
void cb_newtree(OwObject grobj,int reason)
{
	MskElement el=NULL;
	MskDialog mask=NULL;
	FTourDataRec *cd=NULL;
	FTourmcRec *mc=NULL;
	
	switch(reason) {
	case MGR_TREE_SELECT:
		el = MGrObjDataFindMskElement(grobj);
		if (el) {
			mask = GET_MASK(el);
		}
		if (mask) {
			mc = CLIENT_DATA(FTourmcRec *);
			cd  = USER_DATA(FTourDataRec *);
			if(cd && mc) {
				mc->sel_new=cd->typ;
				mc->grobj_new=grobj;
			}
		}
		break;
	case MGR_TREE_UNSELECT:
		el = MGrObjDataFindMskElement(grobj);
		if (el) {
			mask = GET_MASK(el);
			if (mask) {
				mc = CLIENT_DATA(FTourmcRec *);
				if (mc) {
					mc->sel_new=UNDEF;
					mc->grobj_new=NULL;
				}
			}
		}
		break;
	case MGR_TREE_DESTROY:
		cd = USER_DATA(FTourDataRec *);
		if (cd) {
			MGrObjSet(grobj,MGrNuserdata,VNULL);
			cd = (FTourDataRec*)MemDealloc(cd);
		}
		break;
	}
}

/* 
 * Function to refresh the entries of both Tree's 
 * by adding an entry to both Tree's 
 * Only to refresh the data's of the Trees or
 * delete the changes.
 * 
 */
int refresh_Trees(MskDialog mask,int key)
{
	int rv=RETURN_ACCEPTED;
	FTourDataRec *old = NULL,*xnew = NULL;
	MAM mam[MAX_BUFFER_CNT_TPL];
	MAK mak[MAX_BUFFER_CNT_TPL];
	int dbrv=0,i=0;
	// int dbrv1=0,i1=0;
	// float AnzAuftr=0;
	float AnzPick=0,AnzPal=0;
	char last_tournr[TOURNR_LEN+1];
	char last_makid[RECID_LEN+1];
	char last_mandant[MANDANT_LEN+1];
	OwObject tour_old=NULL,auftr_old=NULL,root_old=NULL;
	OwObject tour_new=NULL,auftr_new=NULL,root_new=NULL;
	MskElement el_Tree_Old;
	MskElement el_Tree_New;
	FTourmcRec *mc=NULL;
	char buffer[2000];
	char buffer1[2000];
	// FTourDataRec *parent = NULL;
	
	if (mask) {
		/* Get pointer to Tree by the name */
		el_Tree_Old=MskQueryCreateRl(mask,MskGetElement(TREENAME_OLD),KEY_DEF);
		el_Tree_New=MskQueryCreateRl(mask,MskGetElement(TREENAME_NEW),KEY_DEF);
		
		/* Get pointer to Mask - ClientData Record */
		
		MskVaAssignMatch(mask,"*-MAK-*",
						 MskNtransferDup2Var,(Value)TRUE,
						 MskNupdate,	(Value)TRUE,
						 NULL);
		
		mc = CLIENT_DATA(FTourmcRec *);
		
		/* Check if Tree's and Clientdata are OK */
		if (mc && el_Tree_Old && el_Tree_New) {
			
			/* Delete entries of the Trees */
			OwTreeDestroy(el_Tree_Old,NULL);
			OwTreeDestroy(el_Tree_New,NULL);
			
			/* Set Root - Tree  ( Old & New ) */
			root_old = OwTreeVaInsert(mask,TREENAME_OLD,key,
									  NULL,
									  NULL,
									  TO_CHAR(format("\b{BM_PLUS,BM_MINUS} %s",LsMessage(LSM("Touren Alt")))),
									  MGrNcheck,(Value )1,
									  NULL);
			root_new = OwTreeVaInsert(mask,TREENAME_NEW,key,
									  NULL,
									  NULL,
									  TO_CHAR(format("\b{BM_PLUS,BM_MINUS} %s",LsMessage(LSM("Touren Neu")))),
									  MGrNcheck,(Value )1,
									  NULL);
			if (root_old && root_new) {
				/* Copy root entry to mask Clientdata */
				mc->grobj_root_old = root_old;
				mc->grobj_root_new = root_new;
				
				/* Read All MAM's with MAK's */
/*				memset(last_tournr,0,strlen(last_tournr));*/
/*				memset(last_mandant,0,strlen(last_mandant));*/
/*				memset(last_makid,0,strlen(last_makid));*/
				memset(last_tournr,0,sizeof(last_tournr));
				memset(last_mandant,0,sizeof(last_mandant));
				memset(last_makid,0,sizeof(last_makid));
				do {
					memset(mam,0,MAX_BUFFER_CNT_TPL*sizeof(MAM));
					memset(mak,0,MAX_BUFFER_CNT_TPL*sizeof(MAK));
					
					if (dbrv == 0) {
						dbrv = TExecSqlX(mask,NULL,
										 "SELECT %MAM,%MAK FROM mak,mam "
										 "WHERE mam.rectyp="STR_RECTYP_TOUR
										 " AND (mam.status="STR_MASTAT_NEU
										 "  OR mam.status="STR_MASTAT_FREIGABE
										 " ) AND mam.mandant=:a"
										 " AND mak.tournr=mam.parentid"
										 " AND mak.mandant=mam.mandant"
										 " AND mak.bwart="STR_BWART_AKU
										 " ORDER BY mam.parentid,mak.id",
										 MAX_BUFFER_CNT_TPL,0,
										 SQLSTRING(mc->mak.makMandant),
										 SELSTRUCT(TN_MAM,mam[0]),
										 SELSTRUCT(TN_MAK,mak[0]),
										 NULL);
					} else {
						dbrv = TExecSqlV(mask, NULL, NULL, NULL, NULL, NULL);			
					}
		
					for (i=0;i<dbrv;i++) {
						/* Check if the last_tournr and last_mandant is empty
						 * or the last_mandant is not equal to the current 
						 * or the last_tournr is not equal to the current
						 * ... and add a new Tree-Entry
						 */
						
						
						if ((last_mandant[0]=='\0' && last_tournr[0]=='\0') ||
							(strcmp(last_mandant,mam[i].mamMandant) ||
							 strcmp(last_tournr,mam[i].mamParentId))) {
				
							/* Allocate new memory for a new element */
							old = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
							xnew = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
				
							if (old && xnew) {
								memset(old,0,sizeof(FTourDataRec));
								memset(xnew,0,sizeof(FTourDataRec));
/*								memset(buffer,0,strlen(buffer));*/
								memset(buffer,0,sizeof(buffer));
								
								/* Set the Userdata's */
								memcpy(&old->mam,&mam[i],sizeof(MAM));
								StrCpy(old->new_tournr,mam[i].mamParentId);
								old->typ=TOUR;
								old->changed=NOT_CHANGED;
								
								memcpy(&xnew->mam,&mam[i],sizeof(MAM));
								StrCpy(xnew->new_tournr,mam[i].mamParentId);
								xnew->typ=TOUR;
								xnew->changed=NOT_CHANGED;
					
								/* Add a new Tour */
#if 0								
								if (mam[i].mamStatus==MASTAT_NEU &&
									mam[i].mamPreStatus!=MASTAT_FREIGABE) {
#endif									
									tour_old = OwTreeVaInsert(NULL,NULL,0,
														  root_old,
														  tour_old,
														  old->mam.mamParentId,
														  MGrNuserdata,	(Value )old,
														  MGrNcbTree,	(Value )cb_oldtree,
														  NULL);
//								}
								tour_new = OwTreeVaInsert(NULL,NULL,0,
														  root_new,
														  tour_new,
														  xnew->mam.mamParentId,
														  MGrNuserdata,	(Value )xnew,
														  MGrNcbTree,	(Value )cb_newtree,
														  NULL);
							}
						}
						StrCpy(last_tournr,mam[i].mamParentId);
						StrCpy(last_mandant,mam[i].mamMandant);
						
						if (0) {
							if (mam[i].mamMaeTyp==MAETYP_PK) {
								AnzPick+=mam[i].mamAnzahl;
								sprintf(buffer,"%7.2f",AnzPick);
								sprintf(buffer1,"%s %0.*s%s",mam[i].mamParentId,10-strlen(buffer),"             ",buffer);
								MGrObjSet(tour_old,MGrNtext,(Value)buffer1);
							} else if (mam[i].mamMaeTyp==MAETYP_GP) {
								AnzPal+=mam[i].mamAnzahl;
							}
						}
						
						/* Check if the last_makid is empty 
						 * or the last_makid is not equal to the current
						 * ... and add a new Tree Entry
						 */
						if ((last_makid[0]=='\0') ||
							strcmp(last_makid,mak[i].makId)) {
							
							/* Allocate new memory for a new entry */
							old = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
							xnew = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
				
							if (old && xnew) {
								memset(old,0,sizeof(FTourDataRec));
								memset(xnew,0,sizeof(FTourDataRec));
					
								/* Set the Userdata 's */
								memcpy(&old->mak,&mak[i],sizeof(MAK));
								StrCpy(old->new_tournr,mak[i].makTourNr);
								old->typ=ORDER;
					
								memcpy(&xnew->mak,&mak[i],sizeof(MAK));
								StrCpy(xnew->new_tournr,mak[i].makTourNr);
								xnew->typ=ORDER;
					
								/* Add a new Tree entry */
#if 0								
								if (mam[i].mamStatus==MASTAT_NEU &&
									mam[i].mamPreStatus!=MASTAT_FREIGABE) {
#endif									
								auftr_old  = OwTreeVaInsert(NULL,NULL,0,
															tour_old,
					  									    auftr_old,
					  									    TO_CHAR(AuftrName(&(old->mak))),
														    MGrNuserdata,	(Value )old, 
															MGrNcbTree,	(Value )cb_oldtree,
															NULL);
	//							}
								auftr_new  = OwTreeVaInsert(NULL,NULL,0,
															tour_new,
					  									    auftr_new,
														    TO_CHAR(AuftrName(&(xnew->mak))),
														    MGrNuserdata,	(Value )xnew,
															MGrNcbTree,	(Value )cb_newtree,
															NULL);
							}
						}
						StrCpy(last_makid,mak[i].makId);
					}
				} while (dbrv == MAX_BUFFER_CNT_TPL);
				
				/* Rollback for sure */
				TSqlRollback(mask);
			
				/* Update the Tree */
				rv=OwTreeUpdate(mask,TREENAME_OLD,key);
				rv=OwTreeUpdate(mask,TREENAME_NEW,key);
			}
		}
	}
	return rv;
}

int tourpl_updateMak( MskDialog mask, MAK & mak_new )
{
  MAK mak_indb = mak_new;

  int rv = TExecStdSql( mask, StdNselectUpdNo, TN_MAK, &mak_indb );

  if( rv != 1 ) {
      throw SQL_EXCEPTION( mask );
  }

  std::vector<wamas::DiffTableDesc> vtable_desc;

  if( !load_table_descr( FAC_ME_TOURPL, TN_MAK, vtable_desc, &mak_indb, &mak_new ) )
    throw REPORT_EXCEPTION( "load_table_descr" );

  bool tables_differ = false;

  for( unsigned i = 0; i < vtable_desc.size(); i++ )
    {
      if( vtable_desc[i].diff && vtable_desc[i].name != FN_TourNr )
        {
          tables_differ = true;

          LogPrintf( FAC_ME_TOURPL, LT_NOTIFY, "diff on: %s->%s: %s",
              TN_MAK, vtable_desc[i].name,
              vtable_desc[i].diff_text );
        }
    }

  if( tables_differ ) {
      WamasBoxAlert (SHELL_OF (mask),  WboxNtext,
          TO_CHAR( auto_linebreak( "Die Tour wurde in der Zwischenzeit verändert/gestartet. Bitte nochmals einlesen.")  ),
          NULL );
      return false;
  }

  SetHist( TN_MAK, &mak_new, HIST_UPDATE, GetUserOrTaskName() );

  rv = TExecStdSql( mask, StdNupdate, TN_MAK, &mak_new );

  if( rv != 1 ) {
      throw SQL_EXCEPTION( mask );
  }

  return true;
}

/* 'OwTreeWalk-Callback' of New Tree */
int walking_NewTree(OwObject grobj,int deep,Value userdata)
{
	FTourmcRec *data = (FTourmcRec *)userdata;
	FTourDataRec *xnew = NULL;
	FTourDataRec *old = NULL;
	FTourDataRec *new_entry = NULL;
	FTourDataRec *parent = NULL;
	FTourDataRec *parent1 = NULL;
	//MAM mam;
	//int dbrv;
	//OwObject tour=NULL,order=NULL;
	MGrObjData ich=NULL;
	MGrObjData ich1=NULL;
	int already_exists=TOURPL_EXIST_NO;
	MskElement el=NULL;
	MskDialog mask=NULL;
	char *user=Utl_GetUser();
	char old_tournr[TOURNR_LEN+1];
	int rv=RETURN_ACCEPTED;
	
	if (data) {
		switch(data->todo) {
		case FTOUR_INS_OLD_IN_NEW:
			if (MGrObjGet(grobj,MGrNselect) && (data->changed == NOT_CHANGED)) {
				/* Get selected entry from Old Tree */
				old = (FTourDataRec *)MGrObjGet(data->grobj_old,MGrNuserdata);
				
				/* Get selected entry from New Entry */
				xnew = (FTourDataRec *)MGrObjGet(data->grobj_new,MGrNuserdata);
				
				/* Allocate new memory */
				new_entry = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
		
				if (new_entry && xnew && old) {
					memset(new_entry,0,sizeof(FTourDataRec));
					memcpy(&new_entry->mak,&old->mak,sizeof(MAK));
					StrCpy(new_entry->new_tournr,xnew->new_tournr);
					new_entry->typ=ORDER;
					
					if ((strcmp(new_entry->new_tournr,old->mak.makTourNr)!=0)) {
						/* Insert a new element */
						OwTreeVaInsert(NULL,NULL,0,
									   grobj,
									   TREE_INSERT_FIRST,
									   TO_CHAR(format("%s ( %s )",old->mak.makRefNr,old->mak.makTourNr)),
									   MGrNuserdata,	(Value)new_entry,
									   MGrNcbTree,		(Value)cb_newtree,
									   MGrNtextColor,	(Value)GrColorLock(C_RED),
									   MGrNcheck,(Value )1,
									   NULL);
						MGrObjSet(grobj,MGrNtextColor,(Value )GrColorLock(C_RED));
						parent = (FTourDataRec *)MGrObjGet(grobj,MGrNuserdata);
						parent->changed=CHANGED;
						
						data->changed = CHANGED;
						data->saved = NOT_SAVED;
					} else {
						data->changed = EXISTS;
					}
				}
			}
			break;
		case FTOUR_INS_NEW_IN_NEW:
			if (MGrObjGet(grobj,MGrNcheck) && (data->changed == NOT_CHANGED)) {
				/* Check if the Entry already exists */
				already_exists=TOURPL_EXIST_NO;
				/* Walk trow each element of the New Tree */
				for(ich = ((MGrObjData)(void *)data->grobj_root_new)->child;ich;ich = ich->sibling) {
					parent = (FTourDataRec *)MGrObjGet((OwObject)ich,MGrNuserdata);
					if (strcmp(parent->new_tournr,data->tourname)==0) {
						already_exists=TOURPL_EXIST_YES;
						data->changed=EXISTS;
						break;
					}
				}
				/* If entry not exist - Add */
				if (already_exists==TOURPL_EXIST_NO) {
					
					/* Allocate new memory */
					new_entry = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
				
					if (new_entry) {
						memset(new_entry,0,sizeof(FTourDataRec));
						StrCpy(new_entry->new_tournr,data->tourname);
						new_entry->typ=TOUR;
						new_entry->changed=CHANGED;
						OwTreeVaInsert(NULL,NULL,0,
									   data->grobj_root_new,
									   TREE_INSERT_FIRST,
									   TO_CHAR(format("%s",new_entry->new_tournr)),
									   MGrNuserdata,	(Value )new_entry,
									   MGrNcbTree,		(Value )cb_newtree,
									   MGrNtextColor,	(Value )GrColorLock(C_RED),
									   MGrNcheck,(Value )1,
									   NULL);
						data->changed = CHANGED;
						data->saved = NOT_SAVED;
					}
				}
			}
			break;
		case FTOUR_RESTORE_ORDER:
			if (MGrObjGet(grobj,MGrNselect) && (data->changed !=CHANGED)) {
				/* Delete selected entry */
				if (data) {
/*					MGrObjSet((OwObject)((MGrObjData )(void *)grobj)->parent,MGrNtextColor,(Value )GrColorLock(C_BLACK));*/
					OwTreeDestroy(NULL,(OwObject )grobj);
					data->changed = CHANGED;
					data->saved = NOT_SAVED;
				}
			}
			break;
		case FTOUR_SAVE_CHANGES:
			/* List all changes */
			if (MGrObjGet(grobj,MGrNcheck) && (data->changed == NOT_CHANGED)) {
				/* List all changes */
				el = MGrObjDataFindMskElement(grobj);
				if (el) {
					mask = GET_MASK(el);
				}
				try {
				    for(ich = ((MGrObjData)(void *)data->grobj_root_new)->child;ich;ich = ich->sibling) {
				        parent = (FTourDataRec *)MGrObjGet((OwObject)ich,MGrNuserdata);
				        if (parent->changed==CHANGED) {
				            ich1=ich->child;
				            if (ich1 && (rv==RETURN_ACCEPTED)) {
				                for (;ich1;ich1=ich1->sibling) {
				                    parent1 = (FTourDataRec *)MGrObjGet((OwObject)ich1,MGrNuserdata);
				                    /* Check if it was changed */
				                    if (strcmp(parent1->new_tournr,parent1->mak.makTourNr)!=0) {
				                        /* Save the old TourNr for log-Entry */
				                        StrCpy(old_tournr,parent1->mak.makTourNr);
				                        StrCpy(parent1->mak.makTourNr,parent1->new_tournr);
				                        /* Update mak */

				                        if( !tourpl_updateMak( mask, parent1->mak ) ) {
				                            rv = RETURN_ERROR;
				                        } else {
				                            rv = RETURN_ACCEPTED;
				                            tourpl_updateKaks( mask,&parent1->mak );
				                        }
				                    }
				                }
				                if (rv==RETURN_ACCEPTED) {
				                    /* Create a new mam */
				                    rv=TDB_CreateMam(mask,NULL,&parent1->mak,TDB_CREMAM_MAMTOUR,user);

				                    rv=(rv==TDB_MASTAT_OK)?RETURN_ACCEPTED:RETURN_ERROR;
				                }
				                LogPrintf(FAC_ME_TOURPL,LT_NOTIFY,
				                    "User:%s --> Ref. '%s', Tour alt '%s', Tour neu: '%s', Create/Update TourMAM, rv=%d",
				                    user,
				                    parent1->mak.makRefNr,
				                    old_tournr,
				                    parent1->mak.makTourNr,rv);

				                if (rv==RETURN_ACCEPTED) {
				                    /* Update prestatus of mak if it was free */
				                    if ((parent->mam.mamStatus>=MASTAT_FREIGABE) ||
				                        (parent->mam.mamPreStatus>=MASTAT_FREIGABE)) {

				                        /* Update mam with rectype mak */
				                        rv=tourpl_auto_freigabe(FAC_ME_TOURPL,mask,&parent->mam,&parent1->mak);

				                    } else if ( parent1->mak.makStatus == MASTAT_FREIGABE ) {
				                        // sobald ein Auftrag der Tour freigeben ist, muss auch die Tour Freigegeben sein.

				                        rv=tourpl_auto_freigabe_tour(FAC_ME_TOURPL,mask,&parent->mam,&parent1->mak);
				                    }

				                    tourpl_updateKaks( mask,&parent1->mak );

				                }

				            } else {
				                if (rv==RETURN_ACCEPTED) {
				                    TDB_DeleteMam(mask,parent->mam.mamParentId,RECTYP_TOUR,parent->mam.mamMandant);
				                }
				            } // else
				        } // if (parent->changed==CHANGED) {
				    } // for

				} catch( const std::exception & err ) {
				    LogPrintf( FAC_ME_TOURPL, LT_ALERT, "Error: %s", err.what() );
				    WamasBoxAlert(SHELL_OF(mask),WboxNtext,auto_linebreak( err.what() ).c_str(), NULL );
				    rv = RETURN_ERROR;
				    break;
				}

				if (rv==RETURN_ACCEPTED) {
					TSqlCommit(mask);
				} else {
					TSqlRollback(mask);
				}
				data->changed=CHANGED;
			} // if (MGrObjGet(grobj,MGrNcheck) && (data->changed == NOT_CHANGED))
			break;
		}
	}
	return 0;
}
					
/* Walking trow the Tour's if Old_to_New was Pressed */
int walking_OldTree(OwObject grobj,int deep,Value userdata)
{
	FTourmcRec *data = (FTourmcRec *)userdata;
	FTourDataRec *old = NULL;
	FTourDataRec *old_entry = NULL;
	FTourDataRec *parent = NULL;
	FTourDataRec *parent1 = NULL;
	OwObject tour=NULL;
	MGrObjData ich=NULL;
	MGrObjData ich1=NULL;
	MskElement el=NULL;
	MskDialog mask=NULL;
	REC_DB *mc;
	MskDialog next_mask=NULL;
	OWidget shell=NULL;
	char *user=Utl_GetUser();
	int rv;
	
	switch(data->todo) {
	case FTOUR_INS_OLD_IN_NEW:
		if (MGrObjGet(grobj,MGrNselect)) {
			/* Destroy selected element of Old Tree */
			tour = (OwObject)((MGrObjData )(void *)grobj)->parent;
			old = (FTourDataRec *)MGrObjGet(tour,MGrNuserdata);
			
			if (tour && data && old) {
				OwTreeDestroy(NULL,(OwObject )grobj);
				MGrObjSet(tour,MGrNtextColor,(Value )GrColorLock(C_RED));
				old->changed=CHANGED;
				
				data->changed = CHANGED;
				data->saved = NOT_SAVED;
			}
		}
		break;
	case FTOUR_RESTORE_ORDER:
		if (MGrObjGet(data->grobj_new,MGrNselect) && (data->changed!=CHANGED)) {
			/* Get Data's from Old Tree */
			old = (FTourDataRec *)MGrObjGet(data->grobj_new,MGrNuserdata);
			/* Check if the Tour was changed */
			if (strcmp(old->new_tournr,old->mak.makTourNr)!=0) {
				
				/* Allocate new memory for an entry */
				old_entry = (FTourDataRec*)MemAlloc(sizeof(FTourDataRec));
				
				if (old_entry) {
					/* re-copy the old TourNr back to the order */
					memset(old_entry,0,sizeof(FTourDataRec));
					memcpy(&old_entry->mak,&old->mak,sizeof(MAK));
					StrCpy(old_entry->new_tournr,old->mak.makTourNr);
					old_entry->typ=ORDER;
					
					/* walk trow the old tree and search for the old tour */
					for(ich = ((MGrObjData)(void *)data->grobj_root_old)->child;ich;ich = ich->sibling) {
						parent = (FTourDataRec *)MGrObjGet((OwObject)ich,MGrNuserdata);
						
						/* Compare with TourNr */
						if (strcmp(parent->mam.mamParentId,old->mak.makTourNr)==0) {
							/* Re-Insert if found */
							OwTreeVaInsert(NULL,NULL,0,
										   (OwObject)ich,
										   TREE_INSERT_FIRST,
										   TO_CHAR(AuftrName(&(old_entry->mak))),
										   MGrNuserdata,	(Value )old_entry,
										   MGrNcbTree,		(Value )cb_oldtree,
										   MGrNcheck,(Value )1,
/*										   MGrNtextColor,	(Value )GrColorLock(C_BLACK),*/
										   NULL);
							
							/* Reset the Color */
/*							MGrObjSet((OwObject)ich,MGrNtextColor,(Value )GrColorLock(C_BLACK));*/
							
							data->changed = CHANGED;
							data->saved = NOT_SAVED;
							break;
						}
					}
				}
			}
		}
		break;
	case FTOUR_SAVE_CHANGES:
		/* List all changes */
		el = MGrObjDataFindMskElement(grobj);
		if (el) {
			mask = GET_MASK(el);
		}
		if (MGrObjGet(grobj,MGrNcheck) && (data->changed == NOT_CHANGED)) {
			for(ich = ((MGrObjData)(void *)data->grobj_root_old)->child;ich;ich = ich->sibling) {
				parent = (FTourDataRec *)MGrObjGet((OwObject)ich,MGrNuserdata);
				
				if (parent->changed==CHANGED) {
					if (mask) {
						ich1=ich->child;
						if (ich1) {
							parent1 = (FTourDataRec *)MGrObjGet((OwObject)ich1,MGrNuserdata);
							
							/* Update Tour */
							if (parent1) {
								rv=TDB_CreateMam(mask,NULL,&parent1->mak,TDB_CREMAM_MAMTOUR,user);
								if (rv==TDB_MASTAT_OK) {
									TSqlCommit(mask);
								} else {
									TSqlRollback(mask);
								}
							}
						} else {
							/* Clear MAM */
							TDB_DeleteMam(mask,parent->mam.mamParentId,RECTYP_TOUR,parent->mam.mamMandant);
							TSqlCommit(mask);
						}
					}
				}
			}
			data->changed=CHANGED;
		}
			
		break;
	case FTOUR_SHOW_ORDER:
		if (MGrObjGet(data->grobj_new,MGrNselect) && (data->changed!=CHANGED)) {
			parent = (FTourDataRec *)MGrObjGet((OwObject)grobj,MGrNuserdata);
			mc=(REC_DB*)MemAllocate(sizeof(REC_DB),AllocStruct);
			if (mc) {
				memset(mc,0,sizeof(REC_DB));
				VaMultipleMalloc(
								 (int)sizeof(MAK),     (void**)&mc->pRecBefore,
								 (int)sizeof(MAK),     (void**)&mc->pRecNow,
								 (int)sizeof(MAK),     (void**)&mc->pRecInDb,
								 -1);
				if (mc->pRecBefore && mc->pRecNow && mc->pRecInDb) {
					memset(mc->pRecBefore, 0, sizeof(MAK) );
					memset(mc->pRecNow, 0, sizeof(MAK) );
					memset(mc->pRecInDb, 0, sizeof(MAK) );
					memmove(mc->pRecNow,&parent->mak,sizeof(MAK));
					shell =	ApShellModalCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
					WamasWdgAssignMenu(shell,"tourpl.cc");
					WdgSet(shell, WdgNshellAttrSizeable, (Value )1);
					
					next_mask	= MskOpenMask(NULL,"WamDlgWartungMAK");
					MskAtomAdd(next_mask, NULL, "atom_mak", NULL, (Value )mc);
					if (MskRcCreateDialog(shell,next_mask)) {
						WdgMainLoop();
					}			
				}
			}
		}
		break;
	}
	return 0;
}

/* Callback to transfer old order to new order list 
 * At the old tree an order must be selected
 * and at the new tree an tour must be selected
 */
void cbbt_OldToNew(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *mc=NULL;
	char *msg1=NULL;
	
	switch(reason) {
	case FCB_XF:
		if (mask) {
			mc = CLIENT_DATA(FTourmcRec *);
			if (mc) {
				/* First check if an tour and an order are selected */
				if ((mc->sel_old == ORDER) && (mc->sel_new == TOUR)) {
					/* Get what todo from clientdata of button */
					mc->todo= (int)MskElementGet(el,MskNclientData);
					
					/* Insert old selected entry to the new Tree */
					OwTreeWalkName(mask,TREENAME_NEW,KEY_DEF,walking_NewTree,(Value )mc);
					
					/* Refresh New Tree */
					if (mc->changed==CHANGED) {
						OwTreeUpdate(mask,TREENAME_NEW,KEY_DEF);
						mc->changed=NOT_CHANGED;
					} else if (mc->changed==EXISTS) {
						APBOX(LsMessage(LSM("Auftrag befindet sich schon in Tour !")));
						mc->changed=NOT_CHANGED;
						break;
					}
					
					/* Remove selected entry from old Tree */
					OwTreeWalkName(mask,TREENAME_OLD,KEY_DEF,walking_OldTree,(Value )mc);
					
					/* Refresh old Tree */
					if (mc->changed==CHANGED) {
						OwTreeUpdate(mask,TREENAME_OLD,KEY_DEF);
						mc->changed=NOT_CHANGED;
					}
				} else {
					msg1=StrCreate(LsMessage(LSM("Bitte einen alten Auftrag und eine neue Tour selektieren !")));
					APBOX(msg1);
					if (msg1) msg1=StrGiveup(msg1);
				}
			}
		}
		break;
	}
}
/* Callback to restore the old entry
 * from the selected in the new tree
 */
void cbbt_NewToOld(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *mc=NULL;
	char *msg1=NULL;
	
	switch(reason) {
	case FCB_XF:
		if (mask) {
			mc = CLIENT_DATA(FTourmcRec *);
			if (mc) {
				/* First check if a new order is selected */
				if (mc->sel_new == ORDER) {
					/* Get what todo from clientdata of Button */
					mc->todo= (int)MskElementGet(el,MskNclientData);
					
					/* Re-Insert selected element to old Tree */
					OwTreeWalkName(mask,TREENAME_OLD,KEY_DEF,walking_OldTree,(Value )mc);
					/* Refresh old Tree */
					if (mc->changed==CHANGED) {
						OwTreeUpdate(mask,TREENAME_OLD,KEY_DEF);
						mc->changed=NOT_CHANGED;
					} else {
						break;
					}
					
					/* Remove selected entry from new Tree */
					OwTreeWalkName(mask,TREENAME_NEW,KEY_DEF,walking_NewTree,(Value )mc);
					/* Refresh New Tree */
					if (mc->changed==CHANGED) {
						OwTreeUpdate(mask,TREENAME_NEW,KEY_DEF);
						mc->changed=NOT_CHANGED;
					}
				} else {
					msg1=StrCreate(LsMessage(LSM("Bitte einen neuen Auftrag selektieren !")));
					APBOX(msg1);
					if (msg1) msg1=StrGiveup(msg1);
				}
			}
		}
		break;
	}
}

void cbbt_Newftour(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *mc=NULL;
	// char *msg1=NULL;
	MskDialog next_mask=NULL;
	OWidget next_shell=NULL;
	int rv;
	
	switch(reason) {
	case FCB_XF:
		if (mask) {
			mc = CLIENT_DATA(FTourmcRec *);
			if (mc) {
				if (mc->grobj_root_new) {
					mc->todo= (int)MskElementGet(el,MskNclientData);
					
					next_shell=ApShellModalCreate(SHELL_OF(mask),
												  AP_CENTER,AP_CENTER);
					WamasWdgAssignMenu(next_shell,"tourpl.cc");
					next_mask = MskOpenMask(NULL,DLGNAME_TPL_NAME);
					if (next_mask && next_shell) {
						MskDialogSet(next_mask,MskNmaskCalldata,(Value)mc);
						if (MskRcCreateDialog(next_shell,next_mask)) {
							rv=WdgMainLoop();
						
							if (rv==IS_Ok) {
								/* Insert to new Tree */
								OwTreeWalkName(mask,TREENAME_NEW,KEY_DEF,walking_NewTree,(Value )mc);
					
								/* Refresh Tree if add was OK */
								if (mc->changed==CHANGED) {
									rv = OwTreeUpdate(mask,TREENAME_NEW,KEY_DEF);
									mc->changed=NOT_CHANGED;
								} else if (mc->changed==EXISTS) {
									APBOX(LsMessage(LSM("Tourname bereits vorhanden !")));
/*									memset(mc->tourname,0,strlen(mc->tourname));*/
									mc->tourname[0]='\0';
									mc->changed=NOT_CHANGED;
								}
							}
						}
					}
				} else {
					APBOX(LsMessage(LSM("Bitte zuerst Touren aktualiseren!")));
				}
			}
		}
		break;
	}
}

/* Callback for Refreshing the Old Tree */
void cbbt_ttplRefresh(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *mc=NULL;
	int rv;
	
	switch (reason) {
	case FCB_XF:
		
		if (mask) {
			MskVaAssignMatch(mask,"*-MAK-*",
							 MskNtransferDup2Var,(Value)TRUE,
							 MskNupdate,	(Value)TRUE,
							 NULL);
			mc=CLIENT_DATA(FTourmcRec *);
			if (mc) {
				/* First check the Mandant */
				if (!Util_IsEmptyStr(mc->mak.makMandant)) {
					/* Check if anything was changed in the Dialog */
					if (mc->saved==NOT_SAVED) {
						rv = COBOX(LsMessage(LSM("Änderungen verwerfen ??")));
						if (rv == IS_Ok) {
							mc->saved=SAVED;
							mc->changed=NOT_CHANGED;
							refresh_Trees(mask,KEY_DEF);
						}
					} else {
						/* ... else Refresh the Tree without ask */
						refresh_Trees(mask,KEY_DEF);
					}
				} else {
					APBOX(LsMessage(LSM("Bitte einen Mandanten eintragen !")));
					set_focus_elem(mask,"MAK_Mandant_t");

				}
			}
		}
		break;
	}
}

void cbbt_Saveftour(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *mc=NULL;
	int rv;
	
	switch (reason) {
	case FCB_XF:
		
		if (mask) {
			MskVaAssignMatch(mask,"*-MAK-*",
							 MskNtransferDup2Var,(Value)TRUE,
							 MskNupdate,	(Value)TRUE,
							 NULL);
			mc=CLIENT_DATA(FTourmcRec *);
			if (mc) {
				if (mc->saved == NOT_SAVED) {
					rv = COBOX(LsMessage(LSM("Aenderungen uebernehmen ??")));
					if (rv == IS_Ok) {
						/* Update new Tree */
						mc->todo= (int)MskElementGet(el,MskNclientData);
						OwTreeWalkName(mask,TREENAME_NEW,KEY_DEF,walking_NewTree,(Value )mc);					
					
						mc->changed=NOT_CHANGED;
						mc->saved=NOT_SAVED;
					
						/* Update old Tree */
						mc->todo= (int)MskElementGet(el,MskNclientData);
						OwTreeWalkName(mask,TREENAME_OLD,KEY_DEF,walking_OldTree,(Value )mc);
					
						if (mc->changed==CHANGED) {
							refresh_Trees(mask,KEY_DEF);
						}
					
						mc->saved=SAVED;
						mc->changed=NOT_CHANGED;

						CfTrigger (PROC_UPDATE, FUNC_FIXLOSTMAMS, 0, FAC_ME_TOURPL);
						CfTrigger (PROC_UPDATE, FUNC_UPDATEMAM, 0, FAC_ME_TOURPL);
						CfTrigger( PROC_UPDATE, FUNC_FIXTEKTOURNR, 0, FAC_ME_TOURPL );
					}
					/* Update Tree's */
				} else {
					APBOX(LsMessage(LSM("Daten bereits gesichert !")));
				}
			}
		}
		break;
	}
	
}
/* Callback for Setting the new Tourname */
void cbbt_ftNewName(MskDialog mask,MskStatic ef, MskElement el, int reason,void *cbc) 
{
	FTourmcRec *cd=NULL;
	MAM mam;
	
	switch (reason) {
	case FCB_XF:
		
		if (mask) {
			/* Update mask */
			MskVaAssignMatch(mask,"*-FTOUR-*",MskNtransferDup2Var,
							 (Value )TRUE,NULL);
			/* Get the Calldata's */
			cd=CALL_DATA(FTourmcRec *);
			if (cd) {
				/* Check the Name of new Tour */
				if (!Util_IsEmptyStr(cd->tourname)) {
					/* Check if Tourname already exists */
					if (TExecSql(mask,"SELECT %MAM from mam where mam.status<>"STR_MASTAT_FERTIG
								 " and mam.status<>"STR_MASTAT_NI
								 " and mam.rectyp="STR_RECTYP_TOUR
								 " and mam.parentid=:a"
								 " and mam.mandant=:b",
								 SELSTRUCT(TN_MAM,mam),
								 SQLSTRING(cd->tourname),
								 SQLSTRING(cd->mak.makMandant),
								 NULL)>0) {
						APBOX(LsMessage(LSM("Tourname existiert schon !")));
						set_focus_elem(mask,"FTourNewTourName");
					} else {
						MskCbOk(mask,ef,el,reason,cbc);
					}
				} else {
					APBOX(LsMessage(LSM("Bitte einen Tournamen angeben !")));
					set_focus_elem(mask,"FTourNewTourName");
				}
			}
		}
		break;
	}
}

/* Mask Callback of Tourenplanung */
int cbWamDlgttpl(MskDialog mask, int reason)
{
	FTourmcRec *mc=NULL;
	char *title=NULL;
	const char *DefaultMandant=NULL;
	int rv=1;
	
	title=StrCreate(LsMessage(LsMessage(LSM("Tourenplanung"))));
	
	switch (reason) {
	case MSK_CM:
		mc = (FTourmcRec *)MemAlloc(sizeof(FTourmcRec));
		if (mc) {
			memset(mc,0,sizeof(FTourmcRec));
			DefaultMandant=Var_GetString("DefaultMandant","");
			if (DefaultMandant) {
				strncpy(mc->mak.makMandant,DefaultMandant,strlen(DefaultMandant));
			}
			MskDialogSet(mask,MskNmaskClientData,(Value)mc);
		}
		break;
	case MSK_CF:
		mc = CLIENT_DATA(FTourmcRec *);
		if (mc) {
			MskVaAssignMatch(mask,"*-MAK-*",
							 MskNvariableStruct,	(Value)&mc->mak,
							 NULL);
		}
		break;
	case MSK_RA:
		WdgSet(SHELL_OF(mask),WdgNshellCaption,(Value)title);

		cbbt_ttplRefresh( mask, NULL, NULL, FCB_XF,NULL);

		break;
	case MSK_LM:
		mc = CLIENT_DATA(FTourmcRec *);
		
		if (mc->saved == NOT_SAVED) {
			rv = COBOX(LsMessage(LSM("Aenderungen verwerfen ??")));
			rv= (rv == IS_Ok)? 1:0;
		}
		break;
	case MSK_DM:
		mc = CLIENT_DATA(FTourmcRec *);
		if (mc) {
			mc = (FTourmcRec*)MemDealloc(mc);
			if (title) {
				title=StrGiveup(title);
			}
		}

	    CfTrigger (PROC_UPDATE, FUNC_FIXLOSTMAMS, 0, FAC_ME_TOURPL);
	    CfTrigger( PROC_UPDATE, FUNC_FIXTEKTOURNR, 0, FAC_ME_TOURPL );

		break;
	}
	return rv;
}

/* Mask Dialog of New Tour Name */
void cbWamDlgttpl_name(MskDialog mask, int reason)
{
	FTourmcRec *cd=NULL;
	char *title=NULL;
	
	title=StrCreate(LsMessage(LSM("Neuer Tourenname")));
	
	switch (reason) {
	case MSK_CM:
		cd = CALL_DATA(FTourmcRec *);
/*		memset(cd->tourname,0,strlen(cd->tourname));*/
		cd->tourname[0]='\0';
		break;
	case MSK_CF:
		cd = CALL_DATA(FTourmcRec *);
		if (cd) {
			MskVaAssignMatch(mask,"*-FTOUR-*",
							 MskNvariableStruct,	(Value)cd,
							 NULL);
		}
		break;
	case MSK_RA:
		WdgSet(SHELL_OF(mask),WdgNshellCaption,(Value)title);
		break;
	case MSK_DM:
		if (title) {
			title=StrGiveup(title);
		}
		break;
	}
}

/* Numeric Table and Offset's */
DdTableRec table_numtourpl[]= {
	{"FTOUR_INS_OLD_IN_NEW",	(Value )FTOUR_INS_OLD_IN_NEW},
	{"FTOUR_INS_NEW_IN_NEW",	(Value )FTOUR_INS_NEW_IN_NEW},
	{"FTOUR_RESTORE_ORDER",		(Value )FTOUR_RESTORE_ORDER},
	{"FTOUR_SAVE_CHANGES",		(Value )FTOUR_SAVE_CHANGES},
	{"FTOUR_SHOW_ORDER",		(Value )FTOUR_SHOW_ORDER},
	{"NEW_TOURNAME_LEN",		(Value )TOURNR_LEN},
	{"ofs_FtourNewTour",		(Value )MemberOffset(FTourmcRec,tourname)},
	{NULL}
};

/* Table of Callback's */
DdTableRec table_cbtourpl[]= {
	{"cbWamDlgttpl",			(Value)cbWamDlgttpl},
	{"cbWamDlgttpl_name",		(Value)cbWamDlgttpl_name},
	
	{"cbbt_ttplRefresh",		(Value)cbbt_ttplRefresh},
	{"cbbt_OldToNew",			(Value)cbbt_OldToNew},
	{"cbbt_NewToOld",			(Value)cbbt_NewToOld},
	{"cbbt_Newftour",			(Value)cbbt_Newftour},
	
	{"cbbt_ftNewName",			(Value)cbbt_ftNewName},
	{"cbbt_Saveftour",			(Value)cbbt_Saveftour},
	{NULL}
};


/* eof */
