/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include <fstd20.h>
#include <pal.h>
#include <tep.h>
#include <mam.h>
#include <spo.h>
#include <ftdb20.h>

/* #include "sqlutil.h"*/
#include "stime.h"
#include "combo_to.h"
#include "me_util.h"
#include <hist_util.h>
#include "me_atoms.h"
#include <map_util.h>
#include <tek_util.h>
#include <svar20.h>
#include <ma_stat.h>
#include <tet_util.h>
#include <sfac20.h>
#include "me_mask.h"
#include <err_util.h>
#include <sperr_util.h>
#include "me_barcode.h"
#include "log_util.h"

#include <sprn20.h>
#include <fprn20.h>


#define KK_STMT_LEN			2024		/* max len of sql stmt */
#define KK_BUF_COUNT		200			/* number of records read with one db-Xfetch */

#define MY_FAC_NAME			"me_prod"	/* facility name */

#define KZ_PAL_SPERRE		'S'			/* displayed when pal is locked */

#define KK_KOMBIMENGE_LEN	12			/* anzeige von menge/pal[+] in listen */
#define KK_PRODCHK_AUTOREFRESH_NAME	"chk_prodAutoRefresh"	/* name autorefresh checkbutton */
#define CHARGE_DEFAULT		"0000"		/* default wert bei ungueltigen chargen */

/*
 * ListBuffer name aliases
 */
enum {
	LB_PAL=0,
	LB_AUFTRAG,
	LB_MAX
};

static char *LB[] = {
	"li_prodPalettierer",
	"li_prodAuftrag",
	NULL
};


 
/*
 * buffer structure of one Pal-List entry
 */
typedef struct _PalEntryRec {
	PAL pal;
	char pal_sperre[2]; /*20*/
	MAK mak;
	MAP map;
	MAE mae;
	char ArtBez[ARTBEZ_LEN+1];
	char b_menge[KK_KOMBIMENGE_LEN+1];
	char l_menge[KK_KOMBIMENGE_LEN+1];
	char prod_menge[KK_KOMBIMENGE_LEN+1];
} PalEntryRec, *PalEntry;
 
/*
 * buffer structure of one Auftrag-List entry
 */
typedef struct _AuftragEntryRec {
	MAK mak;
	MAP map;
	char ArtBez[ARTBEZ_LEN+1];
	char b_menge[KK_KOMBIMENGE_LEN+1];
	char l_menge[KK_KOMBIMENGE_LEN+1];
	char prod_menge[KK_KOMBIMENGE_LEN+1];	
} AuftragEntryRec, *AuftragEntry;

/*
 * mask context dlg 'zuordnung' 
 */
typedef struct _zuordMcRec {
	ART art;
	MAP map;
	long palFlag;
} zuordMcRec, *zuordMc;


/*
 * filter structure
 */
typedef struct _FilterRec {
	char group[ARTGRP_LEN+1];
	time_t liefdat;
	long pos_status;
	long kk;
} FilterRec,*Filter;


typedef struct _prodMcRec {
	FilterRec filter;	
	long filter_on;
} prodMcRec,*prodMc;


/*
 * some tools ...
 */

/* [endfold] */

static char *_strstr2tmpstr(char *s)
{
	char *rs	= NULL;
	
	if (s) {
		rs	= StrForm("%s",s+1);
		if (rs) rs[strlen(rs)-1]	= '\0';	
	}
	return rs;
}


 
int _is_empty_str(char *s)
{
	int rv	= TRUE;

	if (s&&s[0]) 
		for (;*s;s++) 
			if (*s!=' ') {
				rv	= FALSE;
				break;
			}
	return rv;
}

 
/*
 * create and init new REC_DB
 */
static REC_DB *_alloc_REC_DB(char *table)
{
	int size;
	long r_size;
	long t_size;
	long all_size;
	REC_DB *r;
	char *ptr;

	GetTableSize(table, &size);
	t_size=MemAlignSize(size);
	r_size=MemAlignSize(sizeof(REC_DB));
	all_size=r_size+3*t_size;
	r= MemAlloc(all_size);
	if (r) {
		ptr = (char *)r;
		memset(r, 0, all_size);
		r->signature = TDB_TABLE_SIG;
		r->rec_size = size;
		strcpy(r->tablename, table);
		StrUpper(r->tablename);
		ptr += r_size;
		r->pRecBefore = (void *)ptr;

		ptr += t_size;
		r->pRecNow = (void *)ptr;

		ptr += t_size;
		r->pRecInDb = (void *)ptr;
	}
	return r;
}

 
/*
 * unload listbuffer "name" in dialog "mask"
 */
void unload_lb(MskDialog mask, char *name)
{
	ListBuffer lb   = NULL;

	if (mask && name) {
		lb = ListBufferInquireName(mask, name, KEY_DEF);
		if (lb) {
			ListBufferUnload(lb);
		}
	}
}


static void auftrag2tep(MAK *mak,MAP *map, TEP *tep)
{
	if (map && tep) {
		memset(tep,0,sizeof(TEP));
		strcpy(tep->tepMaNr,map->mapMaNr);
		strcpy(tep->tepRefNr,mak->makRefNr);
		tep->tepMaPos	= map->mapMaPos;
		strcpy(tep->tepKLNr,mak->makKLNr);
		strcpy(tep->tepMapIdProd,map->mapId);
		tep->tepUnit.unitMHD	= map->mapBUnit.unitMHD;
		strcpy(tep->tepUnit.unitMandant,map->mapBUnit.unitMandant);		
		strcpy(tep->tepUnit.unitArtNr,map->mapBUnit.unitArtNr);
		strcpy(tep->tepUnit.unitCharge,map->mapBUnit.unitCharge);
		tep->tepUnit.unitMenge	= map->mapBUnit.unitMenge;
		/* 
		 * Sap-LineNr in PreisCode merken -
		 * wird für WESTORNO-Meldung gebraucht
		 */
		strncpy(tep->tepPreisCode, map->mapRes1, QMCODE_LEN);
	}
}


/*
 * zuordnung pal/auftrag herstellen (pmap!=NULL) oder aufheben (pmap==NULL)
 */
static int pal_zuord(MskDialog mask, zuordMc mc,REC_DB *ppal, REC_DB *pmap)
{
	int	dbrv=-1,rv	= FALSE;
	PAL	*pal;
	MAP	*map;



	char user[USER_LEN+1];
	char maeid[RECID_LEN+1];
	
	if (ppal) {
		pal	= ppal->pRecNow;

		if (LockRec(mask,ppal,TN_PAL,SqlNwhithoutRollCom,SiNmsg,MY_FAC_NAME)<0)
			return -1;

		if (pmap&&mc) {

			/*
			 * connect pal and map
			 */

			map	= pmap->pRecNow;
			GetTermUser(user);
			dbrv	= TDB_SetMaStatus(mask, NULL, map,MASTAT_FREIGABE,
									  RECTYP_MAP,map->mapBUnit.unitMandant,user);
			if (dbrv>=0) {
				dbrv	= TExecSql(mask,"select id from mae where "
							"parentid=:a and maetyp=:b and lunit_artnr=:c and "
							"lunit_charge=:d and lunit_mhd=:e",
							SELSTR(maeid,RECID_LEN+1),
							SQLSTRING(map->mapId),
							SQLSTRING(l2sGetNameByValue(&l2s_MAETYP, MAETYP_PK)),
							SQLSTRING(map->mapBUnit.unitArtNr),
							SQLSTRING(map->mapBUnit.unitCharge),
							SQLTIMET(map->mapBUnit.unitMHD),
							NULL);
				if (dbrv>=0) {			
					strcpy(pal->palMaeId,maeid);
					dbrv	= WriteRec(mask,ppal,TN_PAL,SqlNwhithoutRollCom,SiNmsg,MY_FAC_NAME);
				}

				LogAction("Zuteilung: Pal:%s,MAP:%s,Artikel:%s (status:%d)",
						pal->palId,					
						map->mapId,
						map->mapBUnit.unitArtNr,
						dbrv);
						
			}
		} else {
			/*
			 * disconnect
			 */
			memset(pal->palMaeId,0,RECID_LEN+1);
			dbrv	= WriteRec(mask,ppal,TN_PAL,SqlNwhithoutRollCom,SiNmsg,MY_FAC_NAME);

			LogAction("Zuteilung aufgehoben: Pal:%s (status:%d)",
					pal->palId,					
					dbrv);
					
		}
		if (dbrv>=0) {
			TSqlCommit(mask);
			rv	= TRUE;
		} else {
			DspPrintfUpdate(OW_DHE,"%s(%d) pal_zuord(): %s\n",__FILE__,__LINE__,TSqlErrTxt(mask));
			TSqlRollback(mask);
		}
	}
	return rv;
}


static void _build_mstr(MskDialog mask,UNIT *unit,char *buf)
{
	int		eanrv;
	EAN		eanbuf[MAX_EANTYP];
	long 	smenge=0,rmenge=0;

	int	si=0;
	
	if (unit&&buf) {
		memset(buf,0,KK_KOMBIMENGE_LEN+1);
		smenge	= rmenge	= 0;
		eanrv	= TDB_LoadEan4Art(mask,unit->unitMandant,unit->unitArtNr,eanbuf,LOAD_EAN_ASC);
		if (eanrv>0) {
			si	= TDB_SplitGPorTT(eanbuf,eanrv,unit->unitMenge,&smenge,&rmenge);
		}		
		if (si>=0) {
			sprintf(buf,"%d/%d",unit->unitMenge,smenge);
			if (rmenge>0) strcat(buf,"+");
		} else {
			sprintf(buf,"%d/?",unit->unitMenge);
		}
	}
}

static void _build_mprod(MskDialog mask,MAP *map,char *buf, int nogp)
{
	int dbrv;
	long eanz,panz,l;
	 
	if (mask&&map&&buf) {
		eanz	= panz	= l	= 0;
		dbrv = TExecSql(mask,
	 				"SELECT SUM(eanzahl),SUM(eanzeinzel) FROM mam WHERE "
	 				"rectyp="STR_RECTYP_MAP" and parentid=:a and ( "
					"maetyp="STR_MAETYP_GP" OR maetyp="STR_MAETYP_TT")",
	 				SELLONG(panz),
	 				SELLONG(eanz),
	 				SQLSTRING(map->mapId),
	 				NULL);
		dbrv = TExecSql(mask,
		 			"SELECT SUM(eanzeinzel) FROM mam WHERE "
	 				"rectyp="STR_RECTYP_MAP" and parentid=:a and maetyp="STR_MAETYP_PK,
	 				SELLONG(l),
	 				SQLSTRING(map->mapId),
	 				NULL);
		eanz	+= l;
		if (nogp) {
			sprintf(buf,"%ld/??",eanz);			
		} else {
			sprintf(buf,"%ld/%ld",eanz,panz);
			if (l) strcat(buf,"+");
		}
	}	
}

/************************************
 * dialog 'uebersicht'
 */


static void feed_lb_pal(MskDialog mask)
{
	ListBuffer 	lb	= NULL;
	PalEntryRec	per;
	PAL			pal[KK_BUF_COUNT];
	MAP			map[KK_BUF_COUNT];
	MAE			mae[KK_BUF_COUNT];
	MAK			mak[KK_BUF_COUNT];
	char		artbez[KK_BUF_COUNT][ARTBEZ_LEN+1];
	int			dbrv	= 0;
	int			i;
	
	lb = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
	if (mask && lb) {
		ListBufferUnload(lb);
		dbrv	= 0;
		do {
			memset(&pal[0],0,KK_BUF_COUNT*sizeof(PAL));
			memset(&map[0],0,KK_BUF_COUNT*sizeof(MAP));
			memset(&mae[0],0,KK_BUF_COUNT*sizeof(MAE));
			memset(&mak[0],0,KK_BUF_COUNT*sizeof(MAK));
			memset(artbez,0,KK_BUF_COUNT*(ARTBEZ_LEN+1));
			if (dbrv == 0) {
#if 0
				dbrv = TExecSqlX(mask, NULL,
							"SELECT %PAL,%MAP,%MAE,%MAK,art.artbez FROM art,pal,mae,map,mak "
							"where pal.maeid=mae.id(+) "
							"and map.id(+)=mae.parentid "
							"and map.bunit_MHD(+)=mae.BUnit_MHD "
							"and map.bunit_Charge(+)=mae.BUnit_Charge "
							"and mak.id(+)=map.parentid "
							"and art.artnr(+)=map.bunit_artnr "
							"order by pal.id",
							KK_BUF_COUNT,0,
	 						SELSTRUCT(TN_PAL, pal[0]),
	 						SELSTRUCT(TN_MAP, map[0]),
	 						SELSTRUCT(TN_MAE, mae[0]),
	 						SELSTRUCT(TN_MAK, mak[0]),
							SELSTR(artbez[0],ARTBEZ_LEN+1),
	 						NULL);
#else
				dbrv = TExecSqlX(mask, NULL,
							"SELECT %PAL,%MAP,%MAE,%MAK,art.artbez FROM art,pal,mae,map,mak "
							"where pal.maeid=mae.id(+) "
							"and map.id(+)=mae.parentid "
							"and mak.id(+)=map.parentid "
							"and art.artnr(+)=map.bunit_artnr "
							"order by pal.id",
							KK_BUF_COUNT,0,
	 						SELSTRUCT(TN_PAL, pal[0]),
	 						SELSTRUCT(TN_MAP, map[0]),
	 						SELSTRUCT(TN_MAE, mae[0]),
	 						SELSTRUCT(TN_MAK, mak[0]),
							SELSTR(artbez[0],ARTBEZ_LEN+1),
	 						NULL);
#endif
	 		} else {
				dbrv = TExecSqlV(mask, NULL, NULL, NULL, NULL, NULL);
	 		}
			for (i=0;i<dbrv;i++) {
				memset(&per,0,sizeof(PalEntryRec));
				memmove(&per.pal,&pal[i],sizeof(PAL));
				if (!_is_empty_str(pal[i].palMaeId)) {
					memmove(&per.map,&map[i],sizeof(MAP));
					memmove(&per.mae,&mae[i],sizeof(MAE));
					memmove(&per.mak,&mak[i],sizeof(MAK));
					strncpy(per.ArtBez,artbez[i],ARTBEZ_LEN);
					_build_mstr(mask,&map[i].mapBUnit,per.b_menge);
					_build_mstr(mask,&map[i].mapLUnit,per.l_menge);
					_build_mprod(mask,&map[i],per.prod_menge,0);
				}
				if (OWBIT_GET(per.pal.palFlag,PALFLAG_SPERRE)) {
/*					strncpy(per.pal_sperre,"\\b{BM_LOCK}",19);*/
					per.pal_sperre[0]	= KZ_PAL_SPERRE;
				}
				ListBufferAppendElement(lb,&per);
                if(i%2) {
                    ListBufferSetElementColor(lb,i,GrColorLock("LbBgWhite"));
                } else {
                    ListBufferSetElementColor(lb,i,GrColorLock("LbBgBlue"));
                }
			}
		} while (dbrv == KK_BUF_COUNT);
		ListBufferUpdate(lb);
		TSqlRollback(mask);
	}
}

static void add_filter(Filter f,char *stmt)
{
	char *p;
	OwDateTimeRec odate;
	
	if (f && stmt) {
		p	= stmt+strlen(stmt);
		if (f->group[0]) {
			strcat(stmt,StrForm("%s%s%s","and art.prodgrp like '",f->group,"' "));
		}
		if (f->liefdat > 50000) {
			OwDateTimeFromUxTime(&odate,&f->liefdat);
			strcat(stmt,StrForm("and mak.komzeit >= TO_DATE('%02d.%02d.%04d %02d.%02d','DD.MM.YYYY HH24.MI') ",
								odate.v[OWDATETIME_DAY],
								odate.v[OWDATETIME_MON],
								odate.v[OWDATETIME_YEAR],
								0,0));
			strcat(stmt,StrForm("and mak.komzeit <= TO_DATE('%02d.%02d.%04d %02d.%02d','DD.MM.YYYY HH24.MI') ",
								odate.v[OWDATETIME_DAY],
								odate.v[OWDATETIME_MON],
								odate.v[OWDATETIME_YEAR],
								23,59));
		}
		if (f->pos_status) {
			strcat(stmt,"and (");
			if (OWBIT_GET(f->pos_status,1)) {
				strcat(stmt,StrForm("map.status='%s' or ",l2sGetNameByValue(&l2s_MASTAT,MASTAT_NEU)));
			}
			if (OWBIT_GET(f->pos_status,2)) {
				strcat(stmt,StrForm("map.status='%s' or ",l2sGetNameByValue(&l2s_MASTAT,MASTAT_FREIGABE)));
			}
			if (OWBIT_GET(f->pos_status,4)) {
				strcat(stmt,StrForm("map.status='%s' or ",l2sGetNameByValue(&l2s_MASTAT,MASTAT_FERTIG)));
			}
			stmt[strlen(stmt)-3]	= '\0';	/* remove last "or" */
			strcat(stmt,") ");
		}
		if (f->kk) {
			strcat(stmt,"and art.ArtAttr_KK=1 ");
		}
		uwc(p);
	}
}

#define OLD_PROD 0

static void feed_lb_auftrag(MskDialog mask)
{
	ListBuffer 		lb	= NULL;
	AuftragEntryRec	aer;
	AuftragEntry	ae;
	prodMc			mc	= NULL;
	MAP				map[KK_BUF_COUNT];
	MAK				mak[KK_BUF_COUNT];
	char	 		stmt[KK_STMT_LEN+1];
	char 			artbez[KK_BUF_COUNT][ARTBEZ_LEN+1];
	long			eanmenge[KK_BUF_COUNT];
	char            eantyp[KK_BUF_COUNT][STRVALUELEN_MAETYP+1];
	int				dbrv	= 0;
	int				i,inserted=0,nogp=0;
	long			smenge,rmenge;
	OwObject 		sc=NULL;
	char            *lastmapid;
	
	lb = ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
	mc	= (prodMc)MskDialogGet(mask,MskNmaskClientData);
	if (mask && lb && mc) {
		sc = TSqlNewContext(mask, NULL);
		if (sc == NULL ) {
			LogPrintf(FAC_TERM, LT_ALERT,"[%s:%d] Can't get SqlContext!\n",__FILE__,__LINE__);
			return;
		}
		ListBufferUnload(lb);
		dbrv	= 0;
		MskVaAssignMatch(mask,"*-Filter-*",
						 MskNtransferDup2Var,	(Value)TRUE,
						 NULL);

#if OLD_PROD
		sprintf(stmt,"SELECT %%MAK,%%MAP,art.artbez,ean.unit_menge "
				"FROM ean,art,mae,map,mak "
				"where mak.bwart=%s "
				"and mak.status<>"STR_MASTAT_FERTIG
				" and mak.id=map.parentid "
				"and map.PosTyp=%s "
				"and map.id=mae.parentid(+) "
				"and not exists (select maeid from pal where maeid in "
				"(select id from mae where parentid=map.id)) "
							"and art.artnr=map.bunit_artnr "
				"and ean.unit_artnr=art.artnr and "
				"(ean.maetyp="STR_MAETYP_GP
				" or ean.maetyp="STR_MAETYP_TT")",STR_BWART_ZPR,STR_MATYP_STD);
		
#else
		
		sprintf(stmt,"SELECT %%MAK,%%MAP,art.artbez,ean.unit_menge,ean.maetyp "
				"FROM ean,art,mae,map,mak "
				"where mak.bwart=%s "
				"and mak.status<>"STR_MASTAT_FERTIG
				" and mak.id=map.parentid "
				"and map.PosTyp=%s "
				"and map.id=mae.parentid(+) "
				"and not exists (select maeid from pal where maeid in "
				"(select id from mae where parentid=map.id)) "
				"and art.artnr=map.bunit_artnr "
				"and ean.unit_artnr=art.artnr ",STR_BWART_ZPR,STR_MATYP_STD);
#endif
		
		if (mc->filter_on)
			add_filter(&mc->filter,stmt);
#if OLD_PROD
		strcat(stmt,"order by mak.komzeit,mak.refnr,map.mapos");
#else
		strcat(stmt,"order by mak.komzeit,mak.refnr,map.mapos,ean.typ desc");
#endif
		lastmapid=NULL;
		do {
			memset(&map[0],0,KK_BUF_COUNT*sizeof(MAP));
			memset(&mak[0],0,KK_BUF_COUNT*sizeof(MAK));
			memset(&eanmenge[0],0,KK_BUF_COUNT*sizeof(long));
			memset(artbez,0,KK_BUF_COUNT*(ARTBEZ_LEN+1));
			memset(eantyp,0,sizeof(eantyp));
			
			if (dbrv == 0) {
#if OLD_PROD
				dbrv = TExecSqlX(mask, sc,
								 stmt,
								 KK_BUF_COUNT,0,
								 SELSTRUCT(TN_MAK, mak[0]),
								 SELSTRUCT(TN_MAP, map[0]),
								 SELSTR(artbez[0],ARTBEZ_LEN+1),
								 SELLONG(eanmenge[0]),
								 NULL);
#else
				dbrv = TExecSqlX(mask, sc,
								 stmt,
								 KK_BUF_COUNT,0,
								 SELSTRUCT(TN_MAK, mak[0]),
								 SELSTRUCT(TN_MAP, map[0]),
								 SELSTR(artbez[0],ARTBEZ_LEN+1),
								 SELLONG(eanmenge[0]),
								 SELSTR(eantyp[0],STRVALUELEN_MAETYP+1),
								 NULL);
#endif				
	 		} else {
				dbrv = TExecSqlV(mask, sc, NULL, NULL, NULL, NULL);
	 		}
			for (i=0;i<dbrv;i++) {
#if OLD_PROD
				if (!lastmapid||strcmp(lastmapid,map[i].mapId)) {
					if (lastmapid) {
						ListBufferAppendElement(lb,&aer);
					}
					memset(&aer,0,sizeof(AuftragEntryRec));
					memmove(&aer.map,&map[i],sizeof(MAP));
					memmove(&aer.mak,&mak[i],sizeof(MAK));
					lastmapid		= map[i].mapId;
				}
				
				strncpy(aer.ArtBez,artbez[i],ARTBEZ_LEN);

				smenge=0; rmenge=0;
				if (eanmenge[i]>0) {
					smenge=map[i].mapBUnit.unitMenge/eanmenge[i];
					rmenge=map[i].mapBUnit.unitMenge%eanmenge[i];
				}
				sprintf(aer.b_menge,"%d/%d",map[i].mapBUnit.unitMenge,smenge);
				if (rmenge>0) strcat(aer.b_menge,"+");
				if (eanmenge[i]>0) {
					smenge=map[i].mapLUnit.unitMenge/eanmenge[i];
					rmenge=map[i].mapLUnit.unitMenge%eanmenge[i];
				}
				sprintf(aer.l_menge,"%d/%d",map[i].mapLUnit.unitMenge,smenge);
				if (rmenge>0) strcat(aer.l_menge,"+");

				_build_mprod(mask,&map[i],aer.prod_menge);
#else
				if (!lastmapid||strcmp(lastmapid,map[i].mapId)) {
					if (lastmapid) {
						ListBufferAppendElement(lb,&aer);
					}
					memset(&aer,0,sizeof(AuftragEntryRec));
					memmove(&aer.map,&map[i],sizeof(MAP));
					memmove(&aer.mak,&mak[i],sizeof(MAK));
					lastmapid		= map[i].mapId;
					inserted=0;
				}
				if (!inserted) {
					strncpy(aer.ArtBez,artbez[i],ARTBEZ_LEN);

					smenge=0; rmenge=0;
					if (eanmenge[i]>0) {
						smenge=map[i].mapBUnit.unitMenge/eanmenge[i];
						rmenge=map[i].mapBUnit.unitMenge%eanmenge[i];
					}
					/* if we have only PK-Menge, we do not know the GP-Anzahl*/
					if (!strcmp(eantyp[i],"PK")) {
						sprintf(aer.b_menge,"%d/??",
								map[i].mapBUnit.unitMenge);
						nogp=1;
					}
					else {
						sprintf(aer.b_menge,"%d/%d",
								map[i].mapBUnit.unitMenge,smenge);
						if (rmenge>0) strcat(aer.b_menge,"+");
						nogp=0;
					}
					if (eanmenge[i]>0) {
						smenge=map[i].mapLUnit.unitMenge/eanmenge[i];
						rmenge=map[i].mapLUnit.unitMenge%eanmenge[i];
					}
					if (!strcmp(eantyp[i],"PK"))
						sprintf(aer.l_menge,"%d/??",
								map[i].mapLUnit.unitMenge);
					else {
						sprintf(aer.l_menge,"%d/%d",
								map[i].mapLUnit.unitMenge,smenge);
						if (rmenge>0) strcat(aer.l_menge,"+");
					}

					_build_mprod(mask,&map[i],aer.prod_menge,nogp);
					inserted=1;
				}
#endif				
			}
			if (lastmapid)
				ListBufferAppendElement(lb,&aer);
		} while (dbrv == KK_BUF_COUNT);

		dbrv= ListBufferLastElement(lb);
		for(i=0;i<=dbrv;i++) {
			ae=(AuftragEntry)ListBufferGetElement(lb,i)->data;
			if(ae->map.mapStatus==MASTAT_NEU) {
				ListBufferSetElementTextColor(lb,i,GrColorLock(C_RED));
			}
			if(ae->map.mapStatus==MASTAT_FERTIG) {
				ListBufferSetElementTextColor(lb,i,GrColorLock(C_BLUE));
			}
            if(i%2) {
				ListBufferSetElementColor(lb,i,GrColorLock("LbBgWhite"));
            } else {
				ListBufferSetElementColor(lb,i,GrColorLock("LbBgBlue"));
            }
            
		}

		ListBufferUpdate(lb);
		TSqlDestroyContext(mask,sc);
		TSqlRollback(mask);
	}
}


/*
 * listbuffer 'palettierer' callback
 */
static int cbli_prodPalettierer(ListCbInfo li)
{
	if (li->reason==LIST_REASON_TIMEOUT) {
		if (*(long *)MskElementGet(MskQueryRl(li->mask_rl,MskGetElement(KK_PRODCHK_AUTOREFRESH_NAME),KEY_DEF),MskNduplicate)==1) {
			feed_lb_pal(li->mask_rl);
			feed_lb_auftrag(li->mask_rl);
		}
	}
	return RETURN_ACCEPTED;
} 


/*
 * callback button 'Filter Neu'
 */
static void cbbt_prodFilterNeu(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	prodMc	mc	= NULL;
	
	switch (reason) {
	case FCB_XF:
		mc	= (prodMc)MskDialogGet(mask,MskNmaskClientData);
		if (mc) {
			memset(&mc->filter,0,sizeof(FilterRec));
			MskVaAssignMatch(mask,"*-Filter-*",
				MskNtransferVar2Dup,(Value)TRUE,
				MskNupdate,			(Value)TRUE,
				NULL);
		}
		break;
	}
}



/*
 * callback button 'Aktualisieren'
 */
static void cbbt_prodAktualisieren(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	switch (reason) {
	case FCB_XF:
		feed_lb_pal(mask);
		feed_lb_auftrag(mask);
		break;
	}
}


/*
 * callback button 'Sel. aufheben'
 */
static void cbbt_prodUnselect(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer 	lb	= NULL;
	long		last;
	long		l;
	ListElement	le;
	
	switch (reason) {
	case FCB_XF:
		lb = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		if (lb) {
			last	= ListBufferNumberOfElements(lb,LIST_HINT_ALL)-1;
			for (l=0;l<=last;l++) {
				le	= ListBufferGetElement(lb,l);
				if (le) {
					le->hint	= LIST_HINT_NONE;
				}			
			}		
			ListBufferUpdate(lb);
		}
		lb = ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
		if (lb) {
			last	= ListBufferNumberOfElements(lb,LIST_HINT_ALL)-1;
			for (l=0;l<=last;l++) {
				le	= ListBufferGetElement(lb,l);
				if (le) {
					le->hint	= LIST_HINT_NONE;
				}			
			}		
			ListBufferUpdate(lb);
		}
		break;
	}
}


/*
 * starte Auftragsdialog modal/nichtmodal mit uebergebener makid
 */
void cbStartAuftragDlg(MskDialog mask,char *makid,int modal)
{
	OWidget		shell;
	MskDialog	next_mask;
	REC_DB		*mc	= NULL;
	MAK			mak;
	void		*tid	= &mak;
	int			rv;
	
	if (!mask)
		return;

	if (makid) {
		mc	= MemAllocate(sizeof(REC_DB),AllocStruct);
		if (!mc)
			return;

		memset( mc, 0, sizeof(REC_DB));
		VaMultipleMalloc(
			(int)sizeof(MAK),      (void**)&mc->pRecBefore, 
	        (int)sizeof(MAK),      &mc->pRecNow, 
    	    (int)sizeof(MAK),      &mc->pRecInDb, 
        	-1);
		if (mc->pRecBefore && mc->pRecNow && mc->pRecInDb) {
			memset(mc->pRecBefore, 0, sizeof(MAK) );
			memset(mc->pRecNow, 0, sizeof(MAK) );
			memset(mc->pRecInDb, 0, sizeof(MAK) );
			memset(&mak,0,sizeof(MAK));
			strcpy(mak.makId,makid);

 			rv = TExecStdSql(tid, StdNselectUpdNo, TN_MAK, &mak);
			TSqlRollback (tid);

			if (rv < 0)
				return;
		
			memmove(mc->pRecNow,&mak,sizeof(MAK));
		}
		
		if (modal) {
			shell		= ApShellModalCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
			next_mask	= MskOpenMask(NULL,"WamDlgWartungMAK");
			if (mc)
				MskAtomAdd(next_mask, NULL, ATOM_MAK, NULL, (Value )mc);
			if (MskRcCreateDialog(shell,next_mask)) {
				WdgMainLoop();
			}
		} else {
			shell		= ApShellModelessCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
			next_mask	= MskOpenMask(NULL,"WamDlgWartungMAK");
			if (mc)
				MskAtomAdd(next_mask, NULL, ATOM_MAK, NULL, (Value )mc);
			MskRcCreateDialog(shell,next_mask);
		}			
	}
}

/*
 * callback button Auftragswartung
 */
static void cbbt_prodWartung(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lb;
	ListElement	le;
	long		l=0;
	long		cnt	= 0;
	char 		*makid;
	
	switch (reason) {
	case FCB_XF:
		lb 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		do {
			le	= find_lb_sel(lb,l,&l);
			if (!le&&!cnt) {
				cnt++;
				lb 	= ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
				le	= find_lb_sel(lb,0,NULL);
			}		
			l++;
			if (le && le->data) {
				if (cnt==0)
					makid	= ((PalEntry)le->data)->mak.makId;
				else
					makid	= ((AuftragEntry)le->data)->mak.makId;

				cbStartAuftragDlg(mask,makid,TRUE);

			}
		} while (le&&le->data&&!cnt);
		break;
	}
}


/*
 * Callback Mask 'Produktion'
 */
 int cbWamDlgProd(MskDialog mask, int reason)
{
	int rv	= TRUE;
	prodMc	mc;
	
	switch (reason) {
	case MSK_CM:
		mc	= MemAlloc(sizeof(prodMcRec));
		if (!mc) {
			rv	= FALSE;
			break;
		}
		memset(mc,0,sizeof(prodMcRec));
		MskDialogSet(mask,MskNmaskClientData,(Value)mc);
		break;
	case MSK_CF:
		mc	= (prodMc)MskDialogGet(mask,MskNmaskClientData);
		if (mc) {
			MskVaAssignMatch(mask,"*-MC-*",MskNvariableStruct,(Value)mc,NULL);
		}
		break;
	case MSK_RA:
		break;
	case MSK_DM:
		mc	= (prodMc)MskDialogGet(mask,MskNmaskClientData);
		if (mc) {
			MemDeallocate(mc);
		}
		TSqlRollback(mask);		/* for sure ... */
		break;
	}
	return rv;	
}


static ListElement _einzig_zugeordneter_palettierer(ListBuffer lb)
{
	ListElement rv	= NULL;
	ListElement tmp	= NULL;
	long		l	= 0;
	int			nz	= 0;
	
	if (lb) {
		do {
			tmp	= find_lb_sel(lb,l,&l);
			l++;
			if (tmp) {
				if (!_is_empty_str(((PalEntry)tmp->data)->pal.palMaeId)) 
					if (rv)
						return NULL;
					else
						rv	= tmp;
				else
					nz++;
			}
		} while (tmp);			
	}
	if (!nz)
		rv	= NULL;
	return rv;
}


static int _ist_pal_auftrag(MAP *map)
{
	int rv	= FALSE;
	EAN ean[MAX_EANTYP];
	int eancnt;

	if (map) {
		eancnt	= TDB_LoadEan4Art(&rv,map->mapBUnit.unitMandant,map->mapBUnit.unitArtNr,&ean[0],FALSE);
		if (eancnt>0) {
			if (TDB_FindEanType(&ean[0],eancnt,-1,MAETYP_GP)>=0) {
				rv	= TRUE;
			}	

		}
	}
	return rv;
}


#define NUR_PALETTEN_AUF_PALETTIERER 0

/*
 * callback button 'Zuordnung'
 */
static void cbbt_prodZuordnen(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lba	= NULL;
	ListBuffer	lbp	= NULL;
	ListElement	lea;
	ListElement	lep;
	MskDialog	mask_zo;
	OWidget		shell;
	long		l	= 0;
	int			refresh_needed	= FALSE;
	REC_DB		*r;
	char 		tatom[TDB_ATOM_NAME_LEN+1];
	MAP 		*map;
	
	switch (reason) {
	case FCB_XF:
		lbp = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		lba = ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
		if (lba && lbp) {
			lea	= find_lb_sel(lba,0,NULL);
			if (lea) {
				map	= &((AuftragEntry)lea->data)->map;
#if NUR_PALETTEN_AUF_PALETTIERER
				if (!_ist_pal_auftrag(map)) {
					ApBoxAlert(SHELL_OF(mask),0,"Kein Palettenauftrag!");
					break;
				}
#endif
			} else if ((lep=_einzig_zugeordneter_palettierer(lbp))) {
				map	= &((PalEntry)lep->data)->map;
			} else map	= NULL;

			if (map->mapStatus==MASTAT_FERTIG) {
				ApBoxAlert(SHELL_OF(mask),0,"Auftragsposition ist schon 'FERTIG'\nZuordnung nicht mehr möglich!");
				break;
			}
			
			do {
				lep	= find_lb_sel(lbp,l,&l);
				if (lep&&map) {
					l++;
					if (!_is_empty_str(((PalEntry)lep->data)->pal.palMaeId)) {
						if (lea) {
							if (ApBoxCommit(SHELL_OF(mask),0,StrForm("Palettierer %s ist bereits zugeordnet."
									"\nBestehende Zuordnung aufheben?",((PalEntry)lep->data)->pal.palId))!=IS_Yes) {
								lep	= NULL;
								break;
							}
						} else {
							continue;
						}
					}
					shell	= ApShellModalCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
					mask_zo	= MskOpenMask(NULL,"WamDlgZuord");
					TdbCreateTableAtom(mask_zo,NULL,TDB_FLAG_LOCAL_ATOM);
					TdbCreateAtomName(mask_zo,NULL,"PAL",tatom);
					TdbAtomClientData(mask_zo,NULL,tatom,(Value *)&r);
					if (r) {
						r->readBefore	= TRUE;
						*((PAL *)r->pRecNow)	= ((PalEntry)lep->data)->pal;
					}
					TdbCreateAtomName(mask_zo,NULL,"MAP",tatom);
					TdbAtomClientData(mask_zo,NULL,tatom,(Value *)&r);
					if (r) {
						r->readBefore	= TRUE;
						*((MAP *)r->pRecNow)	= *map;
					}
					if (MskRcCreateDialog(shell,mask_zo)) {
						if (WdgMainLoop()!=IS_Cancel) {
							refresh_needed	= TRUE;
						}
					}			
				}
			} while (lep);
			if (refresh_needed) {
				feed_lb_pal(mask);
				feed_lb_auftrag(mask);
			}
		}
		break;
	}
}


/*
 * callback button 'Charge'
 */
static void cbbt_prodCharge(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lbp	= NULL;
	ListElement	lep;
	MskDialog	mask_zo;
	OWidget		shell;
	long		l	= 0;
	int			refresh_needed	= FALSE;
	REC_DB		*r;
	char 		tatom[TDB_ATOM_NAME_LEN+1];
	
	switch (reason) {
	case FCB_XF:
		lbp = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		if (lbp) {
			do {
				lep	= find_lb_sel(lbp,l,&l);
				if (lep) {
					l++;
					shell	= ApShellModalCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
					mask_zo	= MskOpenMask(NULL,"WamDlgZuord");
					TdbCreateTableAtom(mask_zo,NULL,TDB_FLAG_LOCAL_ATOM);
					TdbCreateAtomName(mask_zo,NULL,"PAL",tatom);
					TdbAtomClientData(mask_zo,NULL,tatom,(Value *)&r);
					if (r) {
						r->readBefore	= TRUE;
						*((PAL *)r->pRecNow)	= ((PalEntry)lep->data)->pal;
					} else continue;
					TdbCreateAtomName(mask_zo,NULL,"MAP",tatom);
					TdbAtomClientData(mask_zo,NULL,tatom,(Value *)&r);
					if (r) {
						r->readBefore	= TRUE;
						*((MAP *)r->pRecNow)	= ((PalEntry)lep->data)->map;
					} else continue;
					TdbCreateAtomName(mask_zo,NULL,"MAE",tatom);
					TdbAtomClientData(mask_zo,NULL,tatom,(Value *)&r);
					if (r) {
						r->readBefore	= TRUE;
						*((MAE *)r->pRecNow)	= ((PalEntry)lep->data)->mae;
					} else continue;
					if (MskRcCreateDialog(shell,mask_zo)) {
						if (WdgMainLoop()!=IS_Cancel) {
							refresh_needed	= TRUE;
						}
					}			
				}
			} while (lep);
			if (refresh_needed) {
				feed_lb_pal(mask);
				feed_lb_auftrag(mask);
			}
		}
		break;
	}
}


/*
 * callback button 'Zuordnung aufheben'
 */
static void cbbt_prodAufheben(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lbp	= NULL;

	ListElement	lep;


	long		l	= 0;
	REC_DB		*r;
	
	switch (reason) {
	case FCB_XF:
		lbp = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		r	= _alloc_REC_DB(TN_PAL);
		r->readBefore	= TRUE;
		if (lbp && r) {
			do {
				lep	= find_lb_sel(lbp,l,&l);
				if (lep) {
					l++;
					memmove(r->pRecNow,&((PalEntry)lep->data)->pal,r->rec_size);
					memmove(r->pRecBefore,&((PalEntry)lep->data)->pal,r->rec_size);
					pal_zuord(mask,NULL,r,NULL);
				}
			} while (lep);
			if (l) {
				feed_lb_pal(mask);
				feed_lb_auftrag(mask);
			}
		}
		MemDealloc(r);
		break;
	}
}


/*
 * callback selection pal+auftrag check
 */
static void cb_prodSelCheck12(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lbp,lba;
	ListElement	le;
	MskTcbcXf	*cbc_xf;
	
	switch (reason) {
	case FCB_XF:
		cbc_xf	= (MskTcbcXf *)cbc;
		lbp 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		le	= find_lb_sel(lbp,0,NULL);		
		if (!le || !le->data) {
			cbc_xf->result	= FALSE;
		} else {
			lba 	= ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
			le	= find_lb_sel(lba,0,NULL);		
			if (!le || !le->data) {
				if (!_einzig_zugeordneter_palettierer(lbp)) {
					cbc_xf->result	= FALSE;
				}
			}
		}
		if (!cbc_xf->result) {
			ApBoxAlert(SHELL_OF(mask),0,"Kein Palettierer und/oder Auftrag selektiert!");
		}
		break;
	}
}


/*
 * callback selection pal or auftrag check
 */
static void cb_prodSelCheck1or2(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lb;
	ListElement	le;
	MskTcbcXf	*cbc_xf;
	
	switch (reason) {
	case FCB_XF:
		cbc_xf	= (MskTcbcXf *)cbc;
		lb 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		le	= find_lb_sel(lb,0,NULL);		
		if (!le || !le->data) {
			lb 	= ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
			le	= find_lb_sel(lb,0,NULL);		
			if (!le || !le->data) {
				cbc_xf->result	= FALSE;
			}
		}
		if (!cbc_xf->result) {
			ApBoxAlert(SHELL_OF(mask),0,"Kein Palettierer oder Auftrag selektiert!");
		}
		break;
	}
}


/*
 * callback selection pal check
 */
static void cb_prodSelCheck1(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lb;
	ListElement	le;
	MskTcbcXf	*cbc_xf;
	
	switch (reason) {
	case FCB_XF:
		cbc_xf	= (MskTcbcXf *)cbc;
		lb 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		le	= find_lb_sel(lb,0,NULL);		
		if (!le || !le->data) {
			cbc_xf->result	= FALSE;
			ApBoxAlert(SHELL_OF(mask),0,"Kein Palettierer selektiert!");
		}
		break;
	}
}


/*
 * callback selection pal/zuordnung check
 */
static void cb_prodSelCheckPalZuord(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lb;
	ListElement	le;
	PalEntry 	pe;
	MskTcbcXf	*cbc_xf;
	long		l=0;
	
	switch (reason) {
	case FCB_XF:
		cbc_xf	= (MskTcbcXf *)cbc;
		lb 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		do {
			le	= find_lb_sel(lb,l,&l);		
			l++;
			if (le && le->data) {
				pe	= (PalEntry)le->data;
				if (_is_empty_str(pe->pal.palMaeId)) {
					ApBoxAlert(SHELL_OF(mask),0,"Palettierer %s ist kein Auftrag zugewiesen!",pe->pal.palId);
					cbc_xf->result	= FALSE;
				}
			}
		} while (le&&le->data);
		break;
	}
}


/*
 * callback Artikel
 */
static void cbbt_prodArtikel(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	ListBuffer	lb;
	ListElement	le;
	long		l=0;
	long		lbcnt	= 0;
	UNIT		unit;
	
	switch (reason) {
	case FCB_XF:
		lb 	= ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		do {
			le	= find_lb_sel(lb,l,&l);
			if (!le&&!lbcnt) {
				lbcnt++;
				lb 	= ListBufferInquireName(mask, LB[LB_AUFTRAG], KEY_DEF);
				le	= find_lb_sel(lb,0,NULL);
			}		
			l++;
			if (le && le->data) {
				if (lbcnt==0) {
					strcpy(unit.unitMandant,((PalEntry)le->data)->map.mapBUnit.unitMandant);
					strcpy(unit.unitArtNr,((PalEntry)le->data)->map.mapBUnit.unitArtNr);
				} else {
					strcpy(unit.unitMandant,((AuftragEntry)le->data)->map.mapBUnit.unitMandant);
					strcpy(unit.unitArtNr,((AuftragEntry)le->data)->map.mapBUnit.unitArtNr);
				}
				cbStartArtDlg(mask,&unit,TRUE);

			}
		} while (le&&le->data&&!lbcnt);
		break;
	}
}


/*
 * callback button 'Sperren' 
 */
static void cbbt_prodSperren(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	long		l	= 0;
	ListBuffer	lb;
	ListElement	le;
	REC_DB		*r;
	
	switch (reason) {
	case FCB_XF:
		lb = ListBufferInquireName(mask, LB[LB_PAL], KEY_DEF);
		r	= _alloc_REC_DB(TN_PAL);
		r->readBefore	= TRUE;
		if (lb&&r) {
			do {
				le	= find_lb_sel(lb,l,&l);		
				if (le) {
					l++;
					memmove(r->pRecBefore,&((PalEntry)le->data)->pal,r->rec_size);
					OWBIT_XCHANGE(((PalEntry)le->data)->pal.palFlag,PALFLAG_SPERRE,
						OWBIT_GET(((PalEntry)le->data)->pal.palFlag,PALFLAG_SPERRE));
					memmove(r->pRecNow,&((PalEntry)le->data)->pal,r->rec_size);
					WriteRec(mask,r,TN_PAL,SqlNrollCom,SiNmsg,MY_FAC_NAME);

					LogAction("Pal gesperrt/entsperrt: Pal:%s",
							((PalEntry)le->data)->pal.palId);
							
				}
			} while (le);
			feed_lb_pal(mask);
		}
		MemDealloc(r);
		break;
	}
}


/*
 * callback button 'EAN 128 drucken' 
 */
static void cbbt_prodDruckenEAN128(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	long		l	= 0;
	ListBuffer	lb;
	ListElement	le;
	OWidget		shell;
	MskDialog	mask_ean;
	char 		tatom[TDB_ATOM_NAME_LEN+1];

	REC_DB		*r;
	TEP			*tep;
	int			i;

	
	switch (reason) {
	case FCB_XF:
		for (i=LB_PAL;i<=LB_MAX;i++) {
			lb 	= ListBufferInquireName(mask, LB[i], KEY_DEF);
			l	= 0;
			if (lb) {
				do {
					le	= find_lb_sel(lb,l,&l);		
					if (le) {
						l++;
						shell	= ApShellModalCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
						mask_ean	= MskOpenMask(NULL,"DlgEAN128");
						TdbCreateTableAtom(mask_ean,NULL,TDB_FLAG_LOCAL_ATOM);
						TdbCreateAtomName(mask_ean,NULL,"TEP",tatom);
						TdbAtomClientData(mask_ean,NULL,tatom,(Value *)&r);
						if (r) {
							tep	= (TEP *)r->pRecNow;
							if (i==LB_PAL) {
								if (_is_empty_str(((PalEntry)le->data)->pal.palMaeId)) {
									ApBoxAlert(SHELL_OF(mask),0,"Palettierer %s ist keinem Auftrag zugewiesen!",((PalEntry)le->data)->pal.palId);
									continue;
								} else {
									auftrag2tep(&((PalEntry)le->data)->mak,&((PalEntry)le->data)->map,tep);
									strcpy(tep->tepUnit.unitCharge,((PalEntry)le->data)->mae.maeLUnit.unitCharge);
									tep->tepUnit.unitMHD	= ((PalEntry)le->data)->mae.maeLUnit.unitMHD;
								}
							} else {
								auftrag2tep(&((AuftragEntry)le->data)->mak,&((AuftragEntry)le->data)->map,tep);
							}
						}
						if (MskRcCreateDialog(shell,mask_ean)) {
							if (WdgMainLoop()==IS_Ok) {
#ifdef das_ist_mist
								/*
								 * update mhd/charge in map oder mae
								 */
								if (i==LB_PAL) {
									dbrv	= TExecSql(mask,"update mae set bunit_mhd=:a,bunit_charge=:b where id=:c",
												SQLTIMET(tep->tepUnit.unitMHD),
												SQLSTRING(tep->tepUnit.unitCharge),
												SQLSTRING(((PalEntry)le->data)->pal.palMaeId),
												NULL);
								} else {
	 								dbrv	= TExecSql(mask,"update map set bunit_mhd=:a,bunit_charge=:b where id=:c",
												SQLTIMET(tep->tepUnit.unitMHD),
												SQLSTRING(tep->tepUnit.unitCharge),
												SQLSTRING(((AuftragEntry)le->data)->map.mapId),
												NULL);
								}
								if (dbrv>=0)
									TSqlCommit(mask);
								else {
									ApBoxAlert(SHELL_OF(mask),0,"Fehler beim Updaten von MHD/Charge");
									TSqlRollback(mask);
								}
#endif
							}		
						}			
					} 
				} while (le);
			}
		}
		feed_lb_pal(mask);
		feed_lb_auftrag(mask);
		break;
	}
}

/* Liste der noch nicht abgelieferten Paletten erstellen */
PRIVATE int prod_UngeliefertePals(MskDialog mask,
									char *mapIdProd,
									char *mapIdParent,
									char **retPal)
{
	int rv,i;
	char *sql;
	char *id;
	char artnr[20][ARTNR_LEN+1];
	char teid[20][TEID_LEN+1];
	char pos[20][FELDID_LEN+1];
	char *palList;

	if(mapIdProd==NULL) {
		sql="SELECT tep.unit_artnr,tep.teid,tek.pos_feldid"
			" FROM tep,tek,map"
			" WHERE tep.mapidprod=map.id"
			" AND tep.teid=tek.teid"
			" AND map.parentid=:a"
			" ORDER BY tep.unit_artnr,tek.pos_feldid";
		id=mapIdParent;
	} else {
		sql="SELECT tep.unit_artnr,tep.teid,tek.pos_feldid"
			" FROM tep,tek"
			" WHERE"
			" tek.teid=tep.teid"
			" AND tep.mapidprod=:a"
			" ORDER BY tep.unit_artnr,tek.pos_feldid";			
		id=mapIdProd;
	}
	rv=TExecSqlX(mask,NULL,sql,20,0,
		SELSTR(artnr,ARTNR_LEN+1),
		SELSTR(teid,TEID_LEN+1),
		SELSTR(pos,FELDID_LEN+1),
		SQLSTR(id,RECID_LEN+1),
		NOCODE);
	if((rv<=0)&&(TSqlError(mask)!=SqlNotFound)) {
		if(retPal!=NULL) {
			palList=StrForm("Kann offene Paletten nicht lesen\n"
			"Sql: %s", TSqlErrTxt(mask));
			*retPal=palList;
		}
		return RETURN_ERROR;
	}
	palList="";
	for(i=0;i<rv;i++) {
		palList=StrForm("%s\tArtNr=%s, TeId=%s, Pos=%s\n",palList,
				artnr[i],teid[i],pos[i]);
	}
	if(i==20) {
		palList=StrForm("%s... mehr",palList);
	}

	if(retPal!=NULL) {
		*retPal=palList;
	}
	if(rv>0) {
		return RETURN_FOUND;
	} else {
		return RETURN_NOTFOUND;
	}
}

/*
 * callback button 'Fertig' 
 */
static void cbbt_prodFertig(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	int 		i;
	ListBuffer	lb;
	ListElement	le;
	long		l;
	MAP			*map;
	int 		dbrv	= TRUE;


	char 		user[USER_LEN+1];
	char 		dummy[RECID_LEN+1];
	MAP			mapbuf;
	int			not_possible;
	REC_DB		*r;
	int rv;
	char *reasonStr; 
	
	switch (reason) {
	case FCB_XF:
		GetTermUser(user);
		for (i=LB_PAL;i<=LB_MAX;i++) {
			lb 	= ListBufferInquireName(mask, LB[i], KEY_DEF);
			l	= 0;
			if (lb) {
				do {
					le	= find_lb_sel(lb,l,&l);		
					if (le) {
						l++;

						/*
						 * hole MAP aus listbuffereintrag pal/auftrag
						 */
						 
						if (i==LB_PAL) {
							if (_is_empty_str(((PalEntry)le->data)->pal.palMaeId)) {
								ApBoxAlert(SHELL_OF(mask),0,
									"Palettierer %s ist keinem Auftrag zugewiesen!",
									((PalEntry)le->data)->pal.palId);
								continue;
							} else {
								map	= &((PalEntry)le->data)->map;							
							}
						} else {
							map	= &((AuftragEntry)le->data)->map;							
						}

						/*
						 * hole map fuer ein update
						 */
						
						dbrv	= TExecSql(mask,
									"select %MAP from map where id=:a for update nowait",
									SQLSTRING(map->mapId),
									SELSTRUCT(TN_MAP,mapbuf),
									NULL);

						if (dbrv<=0) {
							ApBoxAlert(SHELL_OF(mask),0,
								"Fehler beim Lesen von Position %s",
								map->mapBUnit.unitArtNr);
							TSqlRollback(mask);
							continue;
						}

						/*
 						 * nochmal bestaetigung einholen
						 */

						if (ToBoxCommit(SHELL_OF(mask),0,
							StrForm("Auftragsposition %d, ArtNr %s,\nwirklich fertigsetzen?",
									mapbuf.mapMaPos,mapbuf.mapBUnit.unitArtNr))!=IS_Yes) {
							TSqlRollback(mask);
							continue;
						}
						
						/*
						 * wenn noch eine palette zwischen palettierer und i-punkt, dann
						 * abbrechen. 
						 * wenn letzte nicht fertige position, dann check ueber den
						 * ganzen auftrag. koennte von anderer position noch eine
						 * palette untewegs sein.
						 */
						 
						not_possible	= FALSE;
						if (TExecSql(mask,
								"select map.id from map,mak"
								" where map.parentid=mak.id"
								" and map.status<>"STR_MASTAT_FERTIG
								" and map.postyp="STR_ARTTYP_STD
								" and map.id<>:a"
								" and mak.id=:b",
							SQLSTRING(mapbuf.mapId),
							SQLSTRING(mapbuf.mapParentId),
							SELSTR(dummy,RECID_LEN+1),
							NULL)<=0) {
							rv=prod_UngeliefertePals(mask,NULL,mapbuf.mapParentId,&reasonStr);
							if(rv!=RETURN_NOTFOUND) {
								not_possible=TRUE;
							}
#ifdef fuer_nix
							if (TExecSql(mask,
									"select tep.teid from tep,map"
									" where tep.mapidprod=map.id"
									" and map.parentid=:a",
									SQLSTRING(mapbuf.mapParentId),
									SELSTR(dummy,RECID_LEN+1),
									NULL)>0) {
									not_possible	= TRUE;		
							}
#endif
						} else {
							rv=prod_UngeliefertePals(mask,mapbuf.mapId,NULL,&reasonStr);
							if(rv!=RETURN_NOTFOUND) {
								not_possible=TRUE;
							}
#ifdef fuer_nix
							if (TExecSql(mask,"select teid from tep where mapidprod=:a",
								SQLSTRING(mapbuf.mapId),
								SELSTR(dummy,RECID_LEN+1),
								NULL)>0) {
								not_possible	= TRUE;		
							}
#endif
						}

						if (not_possible) {
							ApBoxAlert(SHELL_OF(mask),0,
							"Noch nicht alle Paletten des Auftrags %s im Lager.\n%s"
								"Fertigsetzen noch nicht möglich.",
								map->mapMaNr,
								reasonStr);
							TSqlRollback(mask);
							continue;
						}
						

						/* Wenn Pos. auf Palettierer, dann Zuordnung vor fertigsetzen
						   aufheben, damit auch das ev. protokollieren moeglich ist */
						if (i==LB_PAL) {
							r	= _alloc_REC_DB(TN_PAL);
							r->readBefore	= TRUE;
							if (r) {
								memmove(r->pRecNow,&((PalEntry)le->data)->pal,r->rec_size);
								memmove(r->pRecBefore,&((PalEntry)le->data)->pal,r->rec_size);
								pal_zuord(mask,NULL,r,NULL);
							}
							MemDealloc(r);
						}

						/*
						 * setze MAP auf fertig
						 */
						dbrv	= TDB_SetMaStatus(mask,
										NULL,
										&mapbuf,
										MASTAT_FERTIG,
										RECTYP_MAP,mapbuf.mapBUnit.unitMandant,
										user);

						LogAction("Position fertiggesetzt: MAP:%s,MaNr:%s (status:%d)",
									mapbuf.mapId,					
									mapbuf.mapMaNr,					
									dbrv);
									
						if (dbrv<0) {
							ApBoxAlert(SHELL_OF(mask),0,
								"Fehler beim DB-Update!\n"
								"Position %s/Auftrag %s nicht fertiggesetzt.!",
								map->mapBUnit.unitArtNr,
								map->mapMaNr);
							TSqlRollback(mask);
						} else {
							TSqlCommit(mask);
						}		
					}
				} while (le);
			}
		}
		feed_lb_pal(mask);
		feed_lb_auftrag(mask);
		break;
	}
}


/*****************************
 * dialog 'Zuordnung'
 */

/*
 * Callback Mask 'Zuordnung'
 */
static int cbWamDlgZuord(MskDialog mask, int reason)
{
	int 			ok	= FALSE,rv	= TRUE;
	zuordMc			mc=0;
	REC_DB			*ppal,*pmap,*pmae;
	char	 		tatom[TDB_ATOM_NAME_LEN+1];
	int				dbrv;
	char 			maetyp[20];
	EAN				ean;
	
	rv	= Tdb_TdbMaskCB(mask,reason);
	if (rv) {
		switch (reason) {
		case MSK_CM:
			strcpy(maetyp,_strstr2tmpstr(STR_MAETYP_GP));
			memset(&ean,0,sizeof(EAN));
			TdbCreateAtomName(mask,NULL,"MAP",tatom);
			TdbAtomClientData(mask,NULL,tatom,(Value *)&pmap);
			TdbCreateAtomName(mask,NULL,"PAL",tatom);
			TdbAtomClientData(mask,NULL,tatom,(Value *)&ppal);
			TdbCreateAtomName(mask,NULL,"MAE",tatom);
			TdbAtomClientData(mask,NULL,tatom,(Value *)&pmae);
			if (pmap&&ppal) {
				memmove(pmap->pRecBefore,pmap->pRecNow,pmap->rec_size);
				memmove(ppal->pRecBefore,ppal->pRecNow,ppal->rec_size);
				mc	= MemAllocate(sizeof(zuordMcRec),AllocStruct);
				if (mc) {
 					memset(mc,0,sizeof(zuordMcRec));
					ok	= TRUE;
					mc->palFlag	= ((PAL *)ppal->pRecNow)->palFlag;
					mc->map	= *(MAP *)pmap->pRecNow;
					if (pmae&&!_is_empty_str(((MAE *)pmae->pRecNow)->maeId)) {
						strcpy(mc->map.mapBUnit.unitCharge,((MAE *)pmae->pRecNow)->maeLUnit.unitCharge);
						mc->map.mapBUnit.unitMHD	= ((MAE *)pmae->pRecNow)->maeLUnit.unitMHD;						
					}
					dbrv	= TExecSql(mask,"select %ART from art where artnr=:a",
								SQLSTRING(mc->map.mapBUnit.unitArtNr),
								SELSTRUCT(TN_ART,mc->art),
								NULL);
					if (dbrv>=0) {
						dbrv = TExecSqlX(mask, NULL,
	 								"SELECT %EAN FROM EAN WHERE "
	 								"Unit_ArtNr=:a and (maetyp=:b or maetyp=:c)"
	 								"ORDER BY maetyp DESC",
	 								1, 0,
	 								SELSTRUCT(TN_EAN,ean),
	 								SQLSTR(mc->map.mapBUnit.unitArtNr, ARTNR_LEN+1),
	 								SQLSTRING(l2sGetNameByValue(&l2s_MAETYP, MAETYP_GP)),
	 								SQLSTRING(l2sGetNameByValue(&l2s_MAETYP, MAETYP_TT)),
	 								NULL);
					}
					if (dbrv<0) {
						ok	= FALSE;
						ApBoxAlert(SHELL_OF(mask),0,"Keine Artikeldaten oder EAN (Ganzpalette/TT)\nfuer %s gefunden!",
							((MAP *)pmap->pRecNow)->mapBUnit.unitArtNr);
					}
					TSqlRollback(mask);
				}
			}
			if (!ok) {
				MemDeallocate(mc);
				rv	= FALSE;
			} else {
				/*
				 * wenn charge leer oder mit blanks aufgefuellt mit 0000 init.
				 * -> hannes
				 */
#if 0
				if (_is_empty_str(mc->map.mapBUnit.unitCharge))
					strcpy(mc->map.mapBUnit.unitCharge,CHARGE_DEFAULT);
#endif
				MskDialogSet(mask,MskNmaskClientData,(Value)mc);
			}
			break;
		case MSK_CF:
			mc	= (zuordMc )MskDialogGet(mask,MskNmaskClientData);
			if (mc) {
				MskVaAssignMatch(mask,"*-MC-*",MskNvariableStruct,(Value)mc,NULL);
				MskVaAssignMatch(mask,"*-MAP-*",MskNvariableStruct,(Value)&mc->map,NULL);
				MskVaAssignMatch(mask,"*-ART-*",MskNvariableStruct,(Value)&mc->art,NULL);
			}
			break;
		case MSK_DM:
			mc	= (zuordMc )MskDialogGet(mask,MskNmaskClientData);
			if (mc) {
				MemDeallocate(mc);
			}
			TSqlRollback(mask);
			break;
		}	
	}
	return rv;	
}


static int _unit_ok(MskDialog mask,UNIT *bunit,UNIT *lunit,ART *art)
{
	int 	rv	= TRUE;
	time_t 	heute;
	
	if (mask&&bunit&&lunit&&art) {
		time(&heute);
		if (!lunit->unitMHD) {
			ApBoxAlert(SHELL_OF(mask),0,"MHD darf nicht leer sein.");
			rv	= FALSE;

/*
		} else if (lunit->unitMHD+artRHDEin<heute) {
			ApBoxAlert(SHELL_OF(mask),0,"MHD unterschreitet Resthaltbarkeitsdauer im Lager.");
			rv	= FALSE;
*/
		} else if (lunit->unitMHD<=heute) {
			ApBoxAlert(SHELL_OF(mask),0,"MHD kleiner/gleich Tagesdatum!");
			rv	= FALSE;

/*
		} else if (lunit->unitMHD<bunit->unitMHD) {
			ApBoxAlert(SHELL_OF(mask),0,"MHD kleiner dem bestellten MHD!");
			rv	= FALSE;

*/

		} else if (_is_empty_str(lunit->unitCharge)&&OWBIT_GET(art->artArtAttr,ARTATTR_CHARGE)) {
			ApBoxAlert(SHELL_OF(mask),0,"Charge darf nicht leer sein.");
			rv	= FALSE;

		} else if (lunit->unitMHD<bunit->unitMHD) {
			if (ToBoxCommit(SHELL_OF(mask),0,"MHD kleiner dem bestellten MHD!\n"
					"Trotzdem zuteilen?")!=IS_Yes)
				rv	= FALSE;
		}
	}
	return rv;
}

/*
 * callback button 'Zuordnung Ok' 
 */
static void cbbt_zuordOk(MskDialog mask, MskStatic ef, MskElement el, int reason, void *cbc)
{
	MskTcbcXf	*cbc_xf;
	zuordMc		mc;
	REC_DB		*ppal,*pmap;
	char 		tatom[TDB_ATOM_NAME_LEN+1];
	
	switch (reason) {
	case FCB_XF:
		MskVaAssignMatch(mask,"*",MskNtransferDup2Var,(Value)TRUE,NULL);
		cbc_xf	= (MskTcbcXf *)cbc;
		mc	= (zuordMc)MskDialogGet(mask,MskNmaskClientData);
		TdbCreateAtomName(mask,NULL,"MAP",tatom);
		TdbAtomClientData(mask,NULL,tatom,(Value *)&pmap);
		TdbCreateAtomName(mask,NULL,"PAL",tatom);
		TdbAtomClientData(mask,NULL,tatom,(Value *)&ppal);

		if (LockRec(mask,pmap,TN_MAP,SqlNwhithoutRollCom,SiNmsg,MY_FAC_NAME)<0) {
			ApBoxAlert(SHELL_OF(mask),0,"Fehler beim Auslesen der aktuellen Daten!\n");			
			return;
		}
		TSqlRollback(mask);
		*(MAP *)pmap->pRecNow	= mc->map;
		if (memcmp(&((MAP *)pmap->pRecBefore)->mapLUnit,
					&((MAP *)pmap->pRecNow)->mapLUnit,sizeof(UNIT))) {
			ApBoxAlert(SHELL_OF(mask),0,"Daten sind nicht aktuell!\n"
										"Aktualisieren und dann noch einmal versuchen.");
			return;
		}
		if (_unit_ok(mask,&((MAP *)pmap->pRecBefore)->mapBUnit,
					&((MAP *)pmap->pRecNow)->mapBUnit,&mc->art)) {
			if (ppal&&pmap&&mc) {
				OWBIT_CHANGE(((PAL *)ppal->pRecNow)->palFlag,PALFLAG_SPERRE,
							OWBIT_GET(mc->palFlag,PALFLAG_SPERRE));
				pal_zuord(mask,mc,ppal,pmap);
			}
			cbc_xf->result	= MASK_LEAVE;
		}
		break;
	}
}

/* sis: Erfassungs-Dialog Produktion ungeplant */
void CbStartProdUngeplant(MskDialog mask, MskStatic ev, MskElement el, 
		int reason, void *cbc)
{
    MskDialog   mask_be=NULL;
    OWidget     shell;
	int         nValue=1;

    switch (reason) 
	{
    case FCB_XF:
        shell   = ApShellModelessCreate(SHELL_OF(mask),AP_CENTER,AP_CENTER);
        mask_be = MskOpenMask(NULL,"WamDlgTeErf");
		if (mask_be && shell)
		{
			/* Als ClientData Produktion ungeplant setzen !! */
			MskDialogSet(mask_be,MskNmaskClientData,(Value)&nValue);
			MskVaCreateDialog(shell, mask_be, LsMessage(LSM("Produktion ungeplant")), " ",HSL_NI, SMB_Ignore, NULL);
		}
        break;
    }
}

/* bmk */
/*****************************
 * dialog 'TE Erfassung Produktion'
 */


/*
 * some simple transformer for the listbuffer...
 */
int kkCbXtime2str(void **dest,void *src,Value calldata)
{
	static char 	tstr[MAX_PIC_LEN*2+1] ;
	time_t 			_t = *(time_t *)src;


	*dest=tstr;
	if (_t < 0) _t= 0;	/* M$-bug ? */
	if (!_t) {
		strcpy(tstr,"--.--.----");
/*		tstr[0]	= '\0';
*/		return 0;
	} else {
		return CbXtime2str(dest,src,calldata);		
	}
}


/*
 * Arguments for the ListBuffer 'Palettierer'
 */
VaInfoRec args_pal[]={
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,pal.palId)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,pal.palName)},	
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,pal_sperre)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,mak.makRefNr)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,map.mapBUnit.unitArtNr)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,ArtBez)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,mae.maeLUnit.unitMHD),kkCbXtime2str,TTYPE_DATE},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,mae.maeLUnit.unitCharge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,b_menge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,prod_menge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(PalEntryRec,l_menge)},
	{VA_TYPE_END}
};


extern int kkCbXMaStatus(void **dest,void *src,Value calldata);

/*
 * Arguments for the ListBuffer 'Auftrag'
 */
VaInfoRec args_auftrag[]={
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,mak.makRefNr)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,map.mapMaPos)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,map.mapStatus), kkCbXMaStatus,0},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,map.mapBUnit.unitArtNr)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,ArtBez)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,map.mapBUnit.unitMHD),kkCbXtime2str,TTYPE_DATE},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,b_menge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,prod_menge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,l_menge)},
	{VA_TYPE_STRUCT,NULL,MemberOffset(AuftragEntryRec,mak.makKomZeit),kkCbXtime2str,TTYPE_DATE},
	{VA_TYPE_END}
};


/*
 * num-table of module
 */
DdTableRec table_numprod[] = {
	{"prod_args_pal",			(Value )args_pal},
	{"prod_sizeofPalEntry",		(Value )sizeof(PalEntryRec)},
	{"prod_args_auftrag",		(Value )args_auftrag},
	{"prod_sizeofAuftragEntry",	(Value )sizeof(AuftragEntryRec)},
	{"ofs_zuordPalSperre",		(Value )MemberOffset(zuordMcRec,palFlag)},
	{"ofs_prodStatus",			(Value )MemberOffset(prodMcRec,filter.pos_status)},
	{"ofs_prodFiltOn",			(Value )MemberOffset(prodMcRec,filter_on)},
	{"ofs_prodProdGrp",			(Value )MemberOffset(prodMcRec,filter.group)},
	{"ofs_prodLiefDat",			(Value )MemberOffset(prodMcRec,filter.liefdat)},
	{"ofs_prodFiltKK",			(Value )MemberOffset(prodMcRec,filter.kk)},	
	{NULL}
};


/*
 * callback-table of module
 */
DdTableRec table_cbprod[] = {
	{"cbWamDlgProd",			(Value)cbWamDlgProd},
	{"cbli_prodPalettierer",	(Value)cbli_prodPalettierer},
	{"cbbt_prodAktualisieren",	(Value)cbbt_prodAktualisieren},
	{"cbbt_prodUnselect",		(Value)cbbt_prodUnselect},
	{"cbWamDlgZuord",			(Value)cbWamDlgZuord},
	{"cbbt_prodZuordnen",		(Value)cbbt_prodZuordnen},
	{"cbbt_prodAufheben",		(Value)cbbt_prodAufheben},
	{"cbbt_prodWartung",		(Value)cbbt_prodWartung},
	{"cb_prodSelCheck1",		(Value)cb_prodSelCheck1},
	{"cb_prodSelCheck12",		(Value)cb_prodSelCheck12},
	{"cb_prodSelCheck1or2",		(Value)cb_prodSelCheck1or2},
	{"cb_prodSelCheckPalZuord",	(Value)cb_prodSelCheckPalZuord},
	{"cbbt_zuordOk",			(Value)cbbt_zuordOk},
	{"cbbt_prodSperren",		(Value)cbbt_prodSperren},
	{"cbbt_prodCharge",			(Value)cbbt_prodCharge},
	{"cbbt_prodDruckenEAN128",	(Value)cbbt_prodDruckenEAN128},
	{"cbbt_prodFertig",			(Value)cbbt_prodFertig},
	{"cbbt_prodArtikel",		(Value)cbbt_prodArtikel},
	{"cbbt_prodFilterNeu",		(Value)cbbt_prodFilterNeu},
	{NULL}
};

/*eof*/
