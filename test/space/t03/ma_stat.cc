/**
* @file
* @todo describe file content
* @author Copyright (c) 2012 Salomon Automation GmbH
*/
#include "bgstd.h"

#include "ma_stat.h"

#include "mam.h"
#include "lhm.h"
#include "sqldef.h"
#include "dbsqlstd.h"
#include "hist_util.h"
#include "tele.h"
#include "db_util.h"

/* sis [sibitz], TS-68433 -> Start */
#include "tourprot.h"
/* sis [sibitz] -> Ende */
#include <appcont.h>
#include "logtool2.h"
#include "host_sap_wa.h"
#include "host_sap_we.h"
#include "host_sap_we_zli.h"
#include "util.h"
#include "sap_utils.h"
#include "check_to_much_ean_types.h"
#include <cpp_util.h>
#include "mae_util.h"
#include <cpp_util.h>
#include <string_utils.h>
#include <diff_tables.h>

using namespace Tools;

#define VERPART_LEN			12			/* !!!!! aus kernel !!!!! */
#define KK_BUF_COUNT        200
#define KK_STMT_LEN		2000


using namespace Tools;


static int TDB_CleanUpTour(void *tid,const char *tournr,const char *user);


std::string strstr2tmpstr(const std::string & s )
{
  return strip( s, "'" );
}


static int _recid2rectype(char *recid)
{
	int rv	= -1;

	if (recid) {
		if (strstr(recid,strstr2tmpstr(STR_RECTYP_TOUR).c_str()))
			rv	= RECTYP_TOUR;
		else if (strstr(recid,strstr2tmpstr(STR_RECTYP_MAK).c_str()))
			rv	= RECTYP_MAK;
		else if (strstr(recid,strstr2tmpstr(STR_RECTYP_MAP).c_str()))
			rv	= RECTYP_MAP;
		else if (strstr(recid,strstr2tmpstr(STR_RECTYP_MAE).c_str()))
			rv	= RECTYP_MAE;
		else if (strstr(recid,strstr2tmpstr(STR_RECTYP_KAK).c_str()))
			rv	= RECTYP_KAK;
	}
	return rv; 
}


/*
 * ermittle aus den artikeldaten und den mengen die entsprechenden
 * gewichtsdaten (nettogweicht) fuer den uebergebenen map
 */
static void _calc_mapgew(void *tid,MAP *map)
{
	EAN	ean;
	ART art;
	
	if (map) {
		if (TExecSql(tid,"select %EAN, %ART from art,ean "
				"where art.artnr=:a and "
				"ean.unit_artnr=art.artnr and ean.typ="STR_EANTYP_EVE,
				SELSTRUCT(TN_EAN, ean),
				SELSTRUCT(TN_ART, art),
				SQLSTRING(map->mapBUnit.unitArtNr),
				NULL)>0) {

			if (!(art.artArtAttr&ARTATTR_GEWART) || map->mapBUnit.unitGewNetto==0)
				map->mapBUnit.unitGewNetto	= map->mapBUnit.unitMenge*ean.eanUnit.unitGewNetto;
		}		
	}
}


static int _lock_rec(void *tid,char *recid,int rectyp,int lock,void **rvrec,void *rvbuf)
{
	int		rv		= TDB_MASTAT_NODATA;
	int 	dbrv	= TDB_MASTAT_ERR;
	int		idtyp;
	void 	*buf	= NULL;
	int 	bufsize=0;
	
	if (!recid||(!rvrec&&!rvbuf))
		return TDB_MASTAT_NODATA;
	idtyp	= _recid2rectype(recid);
	if (idtyp>=0) {
		switch (rectyp) {
		case  RECTYP_TOUR:
			break;
		case  RECTYP_MAK:
			bufsize	= sizeof(MAK);
			buf	= MemAllocate(bufsize,AllocStruct);
			if (buf) {
				if (rectyp==idtyp) {
					if (lock) {
						dbrv	= TExecSql(tid,"select %MAK	from mak where id=:a for update nowait",
									SELSTRUCT(TN_MAK,*(MAK *)buf),
									SQLSTRING(recid),
									NULL);
					} else {
						dbrv	= TExecSql(tid,"select %MAK	from mak where id=:a",
									SELSTRUCT(TN_MAK,*(MAK *)buf),
									SQLSTRING(recid),
									NULL);
					}
				}
			} else {
				rv	= TDB_MASTAT_ERR;
			}
			break;
		case  RECTYP_MAP:
			bufsize	= sizeof(MAP);
			buf	= MemAllocate(bufsize,AllocStruct);
			if (buf) {
				if (rectyp==idtyp) {
					if (lock) {
						dbrv	= TExecSql(tid,"select %MAP	from map where id=:a for update nowait",
										SELSTRUCT(TN_MAP,*(MAP *)buf),
										SQLSTRING(recid),
										NULL);
					} else {
						dbrv	= TExecSql(tid,"select %MAP	from map where id=:a",
										SELSTRUCT(TN_MAP,*(MAP *)buf),
										SQLSTRING(recid),
										NULL);
					}
				}
			} else {
				rv	= TDB_MASTAT_ERR;
			}
			break;
		case  RECTYP_MAE:
			break;
		case  RECTYP_KAK:
			break;
		default:
			break;
		}
		if (dbrv<0) {
			MemDeallocate(buf);
			buf	= NULL;
			rv	= TDB_MASTAT_ERR;
		} else {
			rv	= TDB_MASTAT_OK;
		}
		if (rv==TDB_MASTAT_OK) {
			if (rvbuf) {
				memmove(rvbuf,buf,bufsize);
				MemDeallocate(buf);
				buf	= NULL;
			} else {
				*rvrec	= buf;
			}
		}
	}
	return rv;
}


/*
 * loading all maps of a mak or all maks of a tour or all mams of given tour into lstick-list
 * maps of a mak are ordered by splitnr and
 * maks of one tour ordered by batchnr (handy splitting into kaks)
 * ist filter != NULL, so wird er als zusaetzliches kriterium in die where-klausel montiert
 */
static LStick _load_rec_list(void *tid, int rectyp, int lock, const char *arg, const char *filter)
{
	LStick 	list	= NULL;
	union {
		MAK mak[KK_BUF_COUNT];
		MAP map[KK_BUF_COUNT];
		MAM mam[KK_BUF_COUNT];
	} buf;
	int		recsize=0;
	char 	stmt[KK_STMT_LEN];
	int		dbrv;
	char 	*rec;
	int 	i;
	
	if (arg&&(rectyp==RECTYP_MAP||rectyp==RECTYP_MAK||rectyp==RECTYP_TOUR)) {
		dbrv	= 0;
		do {
			memset(&buf.mak[0],0,sizeof(buf));
			if (dbrv==0) {
				if (rectyp==RECTYP_MAP) {
					recsize	= sizeof(MAP);
					StrCpy(stmt,"select %MAP from map where parentid=:a");
					if (filter) {
						strcat(stmt," and ");
						strcat(stmt,filter);
					}
					if (lock) {
						strcat(stmt," order by splitnr for update nowait");
						dbrv	= TExecSqlX(tid,NULL,
										stmt,
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAP,buf.map[0]),
										SQLSTRING(arg),
										NULL);
					} else { 
						strcat(stmt," order by splitnr");
						dbrv	= TExecSqlX(tid,NULL,
										stmt,
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAP,buf.map[0]),
										SQLSTRING(arg),
										NULL);
					}
				} else if (rectyp==RECTYP_MAK) {
					recsize	= sizeof(MAK);
					StrCpy(stmt,"select %MAK from mak where tournr=:a");
					if (filter) {
						strcat(stmt," and ");
						strcat(stmt,filter);
					}
					if (lock) {
						strcat(stmt," order by batchnr for update nowait");
						dbrv	= TExecSqlX(tid,NULL,
										stmt,
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAK,buf.mak[0]),
										SQLSTRING(arg),
										NULL);
					} else { 
						strcat(stmt," order by batchnr");
						dbrv	= TExecSqlX(tid,NULL,
										stmt,
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAK,buf.mak[0]),
										SQLSTRING(arg),
										NULL);
					}
				} else {
					recsize	= sizeof(MAM);
					if (lock)
						dbrv	= TExecSqlX(tid,NULL,
										"select %MAM from mam where"
										" rectyp="STR_RECTYP_TOUR
										" and parentid=:a"
										" for update nowait",
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAM,buf.mam[0]),
										SQLSTRING(arg),
										NULL);
					else  
						dbrv	= TExecSqlX(tid,NULL,
										"select %MAM from mam where"
										" rectyp="STR_RECTYP_TOUR
										" and parentid=:a",				
										KK_BUF_COUNT,0,
										SELSTRUCT(TN_MAM,buf.mam[0]),
										SQLSTRING(arg),
										NULL);
				}
			} else {
				dbrv	= TExecSqlV(tid,NULL,NULL,NULL,NULL,NULL);
			}
			for (i=0;i<dbrv;i++) {
			  rec	= static_cast<char*>(MemAlloc(recsize));
				if (rec) {
					if (rectyp==RECTYP_MAP)
						memmove(rec,&buf.map[i],recsize);
					else if (rectyp==RECTYP_MAK)
						memmove(rec,&buf.mak[i],recsize);
					else 
						memmove(rec,&buf.mam[i],recsize);
					list	= LStickAddElement(list,rec);					
				}
			}
		} while (dbrv==KK_BUF_COUNT);	
		if (dbrv<0 && TSqlError(tid)!=SqlNotFound) {
			LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"reclist-load: %s %s (rectyp:%d, lock:%d)\n%s",
					   	arg,
						filter?filter:"no filter",
						rectyp,lock,
						TSqlErrTxt(tid));
		}
	}
	return list;
}

static LStick _free_rec_list(LStick list)
{
	LStick l;
	
	for (l=list;l&&l->data;l=l->next) {
		MemDealloc(l->data);
	}
	return LStickDestroy(list);
}

/*
 * load EANs according Art into eanbuf
 * bufsize:	eanbuf[MAX_EANTYP]
 * returns:	-1 (error) or eans found (>=0,ok)
 */
int TDB_LoadEan4Art(void *tid, const char *artnr, EAN *eanbuf, int desc)
{
	int eanrv	= -1;
	
	if (artnr&&eanbuf) {
		memset(eanbuf,0,MAX_EANTYP*sizeof(EAN));
		if (desc==LOAD_EAN_DESC) {
			eanrv = TExecSqlX(tid, NULL,
		 				"SELECT %EAN FROM EAN WHERE "
	 					"EAN.Unit_ArtNr=:a and GewFlag=0 "
	 					"ORDER BY typ DESC",
	 					MAX_EANTYP, 0,
	 					SELSTRUCT(TN_EAN, eanbuf[0]),
	 					SQLSTR(artnr, ARTNR_LEN+1),
	 					NULL);
		} else {
			eanrv = TExecSqlX(tid, NULL,
		 				"SELECT %EAN FROM EAN WHERE "
	 					"EAN.Unit_ArtNr=:a and GewFlag=0 "
	 					"ORDER BY typ",
	 					MAX_EANTYP, 0,
	 					SELSTRUCT(TN_EAN, eanbuf[0]),
	 					SQLSTR(artnr, ARTNR_LEN+1),
	 					NULL);
		}
	}
	return eanrv;
}



/*
 * eanbuf: EAN		eanbuf[MAX_EANTYP];
 * liefert index von erstem ean in eanbuf mit typ==typ und/oder maetyp==maetyp
 * oder <0 (fehler bzw. nicht gefunden)
 * soll nur typ oder nur maetyp beruecksichtigt werden, so muss fuer den jeweils anderen typ
 * ein wert <0 uebergeben werden.
 */
int TDB_FindEanType(EAN *eanbuf,int maxean,int typ,int maetyp)
{
	int rv	= RETURN_ERROR;
	int i;
	int found;
	
	if (eanbuf) {
		for (i=0;i<maxean;i++) {
			found	= 0;
			if (typ>=0) {
				found	+= (typ==eanbuf[i].eanTyp)?1:0; 
			} else {
				found++;
			}
			if (maetyp>=0) {
				found	+= (maetyp==eanbuf[i].eanMaeTyp)?1:0; 
			} else {
				found++;
			}
			if (found==2) {
				rv	= i;
				break;
			}
		}
	}
	return rv;
}

/*
 * versucht 'menge' [EVE] auf GP (prioritaet) oder TT aufzusplitten. in 'ean' (=EAN[maxean]) muessen
 * die moeglichen EANs uebergeben werden. wenn OK, wird der index der verwendeten EAN
 * geliefert und die splitmenge ('smenge') bzw. restmenge ('rmenge') ermittelt.
 * fehler: -1
 */
int TDB_SplitGPorTT(EAN *ean, int maxean, long menge, long *smenge, long *rmenge)
{
	int rv	= -1;
	int i;
	
	if (ean&&maxean>0&&smenge&&rmenge) {
		i	= TDB_FindEanType(ean,maxean,-1,MAETYP_GP);
		if (i<0) 
			i	= TDB_FindEanType(ean,maxean,-1,MAETYP_TT);
		if (i>=0) {
			*smenge	= menge/ean[i].eanUnit.unitMenge;
			*rmenge	= menge%ean[i].eanUnit.unitMenge;
			rv	= i;
		}
	}
	return rv;
}

/*
 * setzt in  alle tourrelevanten records die tornr
 * RETURN:	siehe SetMaStatus()
 */
int TDB_SetTourTorNr(void *tid,char *tour, long tor, char *torbereich)
{
	int rv	= TDB_MASTAT_ERR;

	if (tour) {
			if (TExecSql(tid,"update mak set tornr=:a, " TCN_MAK_TorBereich " =:b where tournr=:c",
					SQLLONG(tor),
					SQLSTRING(torbereich),
					SQLSTRING(tour),
					NULL)>=0)
				if (TExecSql(tid,"update mam set tornr=:a where "
						"rectyp="STR_RECTYP_TOUR
						" and parentid=:b",
						SQLLONG(tor),
						SQLSTRING(tour),
						NULL)>=0)
					rv	= TDB_MASTAT_OK;
	}
	return rv;
}

enum MAE_CONTROL
{
	IM_PK_ONLY = 1  /* insert_mae nur PK */,
	IM_PKNGP,       /* insert_mae PK+GP(TT) */
	IM_GP           /* insert_mae PK mit Menge0 und GP */
};

/*
 * returns:	<0 error / 0 ok
 */
static int _insert_mae(const std::string &fac, void *tid,MAP *map,const UNIT *punit, MAE_CONTROL control, const char *user)
{
	int 	rv	= -1;
	MAE 	newmae;
	char 	maetyp[20];
	EAN		eanbuf[MAX_EANTYP];
	int		eancnt = 0;
	int		si;
	long	smenge=0,rmenge=0;
	UNIT    unit = *punit;


	if( control == IM_GP ) {
		eancnt	= TDB_LoadEan4Art(tid,map->mapBUnit.unitArtNr,eanbuf,LOAD_EAN_DESC);
		if (eancnt>0) {
			if( TDB_FindEanType( eanbuf, eancnt, -1, MAETYP_TT ) >= 0 ) {
				LogPrintf( fac, LT_DEBUG, "Artikel %s ist ein Tetratainer Artikel, Verplanung erfolgt nach IM_PKNGP", map->mapBUnit.unitArtNr );
				// Tetratainer Artikel, wir gehen nach altem Schema vor
				control = IM_PKNGP;
			}
		}
	}
	
	switch (control) {
	case IM_GP:
		
		/*
		 * suche entsprechenden MAE. falls dieser existiert lasse
		 * ihn unveraendert und liefere OK
		 */

		StrCpy(maetyp, strstr2tmpstr(STR_MAETYP_GP));
		if (TExecSql(tid,"select id from mae where "
				"parentid=:a and maetyp=:b and bunit_artnr=:c and "
				"bunit_charge=:d and bunit_mhd=:e",
				SELCHARN(newmae.maeId,RECID_LEN+1),
				SQLSTRING(map->mapId),
				SQLSTRING(maetyp),
				SQLSTRING(unit.unitArtNr),
				SQLSTRING(unit.unitCharge),
				SQLTIMET(unit.unitMHD),
				NULL)>=0)
			return 0;						

		/*
		 * sonst erzeuge neuen mae typ PK und schreibe ihn in die DB
		 */

		memset(&newmae,0,sizeof(MAE));
		newmae.maeMaeTyp				= MAETYP_GP;		
		newmae.maeStatus				= MASTAT_NEU;
		newmae.maeBUnit.unitMenge 		= unit.unitMenge;
		newmae.maeBUnit.unitGewNetto 	= unit.unitGewNetto;
		StrCpy(newmae.maeBUnit.unitCharge,unit.unitCharge);
		StrCpy(newmae.maeBUnit.unitArtNr,unit.unitArtNr);
		newmae.maeBUnit.unitMHD 		= unit.unitMHD;
		StrCpy(newmae.maeLUnit.unitCharge,unit.unitCharge);
		StrCpy(newmae.maeLUnit.unitArtNr,unit.unitArtNr);
		newmae.maeLUnit.unitMHD 		= unit.unitMHD;
		rv = TDB_GetRecId(tid, TN_MAE, newmae.maeId);

		if( rv < 0 ) {
			LogPrintf( fac, LT_ALERT, "rv: %d SqlError: %s", rv, TSqlErrTxt(tid));
			return rv;
		}

		StrCpy(newmae.maeParentId,map->mapId);
		if (user) 
			SetHist(TN_MAE, &newmae, HIST_INSERT, user);

		DIFF_MAE(tid,&newmae,user?user:"no user detected");

		rv = TExecStdSql(tid, StdNinsert, TN_MAE, &newmae);

		if( rv < 0 ) {
			LogPrintf( fac, LT_ALERT, "rv: %d SqlError: %s", rv, TSqlErrTxt(tid));
			return rv;
		}

		unit.unitMenge = 0;
		unit.unitGewNetto = 0;

		 /* fallthrough*/

	case IM_PK_ONLY:

		/*
		 * suche entsprechenden MAE. falls dieser existiert lasse
		 * ihn unveraendert und liefere OK
		 */

		StrCpy(maetyp,strstr2tmpstr(STR_MAETYP_PK));
		if (TExecSql(tid,"select id from mae where "
				"parentid=:a and maetyp=:b and bunit_artnr=:c and "
				"bunit_charge=:d and bunit_mhd=:e",
				SELCHARN(newmae.maeId,RECID_LEN+1),
				SQLSTRING(map->mapId),
				SQLSTRING(maetyp),
				SQLSTRING(unit.unitArtNr),
				SQLSTRING(unit.unitCharge),
				SQLTIMET(unit.unitMHD),
				NULL)>=0)
			return 0;						

		/*
		 * sonst erzeuge neuen mae typ PK und schreibe ihn in die DB
		 */

		memset(&newmae,0,sizeof(MAE));
		newmae.maeMaeTyp				= MAETYP_PK;		
		newmae.maeStatus				= MASTAT_NEU;
		newmae.maeBUnit.unitMenge 		= unit.unitMenge;
		newmae.maeBUnit.unitGewNetto 	= unit.unitGewNetto;
		StrCpy(newmae.maeBUnit.unitCharge,unit.unitCharge);
		StrCpy(newmae.maeBUnit.unitArtNr,unit.unitArtNr);
		newmae.maeBUnit.unitMHD 		= unit.unitMHD;
		StrCpy(newmae.maeLUnit.unitCharge,unit.unitCharge);
		StrCpy(newmae.maeLUnit.unitArtNr,unit.unitArtNr);
		newmae.maeLUnit.unitMHD 		= unit.unitMHD;
		rv = TDB_GetRecId(tid, TN_MAE, newmae.maeId);
		if (rv>0) {
			StrCpy(newmae.maeParentId,map->mapId);
			if (user) 
				SetHist(TN_MAE, &newmae, HIST_INSERT, user);

			DIFF_MAE(tid,&newmae,user?user:"no user detected");

			rv = TExecStdSql(tid, StdNinsert, TN_MAE, &newmae);
		} else rv	= -1;
		break;

	case IM_PKNGP:
	
		/*
		 * versuche erst einen TT/GP MAE abzusplitten und diesen
		 * in der DB anzulegen, falls er noch nicht existiert.
		 */

		eancnt	= TDB_LoadEan4Art(tid,map->mapBUnit.unitArtNr,eanbuf,LOAD_EAN_DESC);
		if (eancnt>0) {
			si	= TDB_SplitGPorTT(eanbuf,eancnt,map->mapBUnit.unitMenge,&smenge,&rmenge);
			if (si>=0) {
				if (eanbuf[si].eanMaeTyp==MAETYP_GP)
					StrCpy(maetyp,strstr2tmpstr(STR_MAETYP_GP));
				else 
					StrCpy(maetyp,strstr2tmpstr(STR_MAETYP_TT));
				if (TExecSql(tid,"select id from mae where "
						"parentid=:a and maetyp=:b and bunit_artnr=:c and "
						"bunit_charge=:d and bunit_mhd=:e",
						SELCHARN(newmae.maeId,RECID_LEN+1),
						SQLSTRING(map->mapId),
						SQLSTRING(maetyp),
						SQLSTRING(unit.unitArtNr),
						SQLSTRING(unit.unitCharge),
						SQLTIMET(unit.unitMHD),
						NULL)<0) {
					memset(&newmae,0,sizeof(MAE));
					StrCpy(newmae.maeParentId,map->mapId);
					newmae.maeStatus	= MASTAT_NEU;
					newmae.maeMaeTyp 	= eanbuf[si].eanMaeTyp;
					StrCpy(newmae.maeBUnit.unitCharge,unit.unitCharge);
					StrCpy(newmae.maeBUnit.unitArtNr,unit.unitArtNr);
					newmae.maeBUnit.unitMHD = unit.unitMHD;
					StrCpy(newmae.maeLUnit.unitCharge,unit.unitCharge);
					StrCpy(newmae.maeLUnit.unitArtNr,unit.unitArtNr);
					newmae.maeLUnit.unitMHD = unit.unitMHD;

					std::string strat( map->mapStrat );

					if( UseSap() &&  
						( Sap::requireOverload( strat ) ||
						 Sap::requireCharge( strat ) ) ) {
						// Überlifern erlaubt, zuerst einmal ist der Auslag drann
						// daher alles auf GP
						rmenge = 0;
						newmae.maeBUnit.unitMenge = map->mapBUnit.unitMenge;
						newmae.maeBUnit.unitGewNetto = map->mapBUnit.unitGewNetto;
					} else {
						newmae.maeBUnit.unitMenge = smenge*eanbuf[si].eanUnit.unitMenge;
						newmae.maeBUnit.unitGewNetto = smenge*eanbuf[si].eanUnit.unitGewNetto;
					}

					DIFF_MAE(tid,&newmae,user?user:"no user detected");

					rv = TDB_GetRecId(tid, TN_MAE, newmae.maeId);
					if (rv>0) {
						if (user) 
							SetHist(TN_MAE, &newmae, HIST_INSERT, user);
						rv = TExecStdSql(tid, StdNinsert, TN_MAE, &newmae);
					} else rv	= -1;
				} else rv	= 0;
			} else {
				rmenge	= map->mapBUnit.unitMenge;
				rv	= 0;
			}

			/*
			 * erzeuge fuer die restmenge einen MAE typ PK und
			 * schreibe ihn, falls noch nicht existent.
			 */
			if (rv>=0) {
				si	= TDB_FindEanType(eanbuf,eancnt,EANTYP_EVE,-1);
				if (si<0) {
				  LogPrintf( FAC_LIB, LT_ALERT, "Kann den EAN Typ EVE nicht finden!!" );
				  LogPrintf( fac, LT_ALERT, "Kann den EAN Typ EVE nicht finden!!" );

				  /* Das könnte nun daran Liegen, dass in der DB zuviele EANs gespeichert sind.
					 2 EAN Typen mit LE2 zum Beispiel. SB: 176656*/
				  check_too_much_ean_types( fac, tid, map );

				  rv	= -1;
				  break;
				}
				StrCpy(maetyp,strstr2tmpstr(STR_MAETYP_PK));
				if (TExecSql(tid,"select id from mae where "
						"parentid=:a and maetyp=:b and bunit_artnr=:c and "
						"bunit_charge=:d and bunit_mhd=:e",
						SELCHARN(newmae.maeId,RECID_LEN+1),
						SQLSTRING(map->mapId),
						SQLSTRING(maetyp),
						SQLSTRING(unit.unitArtNr),
						SQLSTRING(unit.unitCharge),
						SQLTIMET(unit.unitMHD),
						NULL)<0) {
					memset(&newmae,0,sizeof(MAE));
					StrCpy(newmae.maeParentId,map->mapId);
					newmae.maeStatus	= MASTAT_NEU;
					newmae.maeMaeTyp 	= MAETYP_PK;
					StrCpy(newmae.maeBUnit.unitCharge,unit.unitCharge);
					StrCpy(newmae.maeBUnit.unitArtNr,unit.unitArtNr);
					newmae.maeBUnit.unitMHD = unit.unitMHD;
					StrCpy(newmae.maeLUnit.unitCharge,unit.unitCharge);
					StrCpy(newmae.maeLUnit.unitArtNr,unit.unitArtNr);
					newmae.maeLUnit.unitMHD = unit.unitMHD;
					newmae.maeBUnit.unitMenge = eanbuf[si].eanUnit.unitMenge>0?rmenge/eanbuf[si].eanUnit.unitMenge:0;
					newmae.maeBUnit.unitGewNetto = newmae.maeBUnit.unitMenge*eanbuf[si].eanUnit.unitGewNetto;
					rv = TDB_GetRecId(tid, TN_MAE, newmae.maeId);
					if (rv>0) {
						if (user) 
							SetHist(TN_MAE, &newmae, HIST_INSERT, user);

						DIFF_MAE(tid,&newmae,user?user:"no user detected");

						rv = TExecStdSql(tid, StdNinsert, TN_MAE, &newmae);
					} else rv	= -1;
				}
			}
		} else {
			rv=0;
			LogPrintf(FAC_LIB,LT_NOTIFY,
					"Create_MAE: Keine EAN-Daten fuer Artikel %s",
						map->mapBUnit.unitArtNr);
			LogPrintf(FAC_TERM,LT_NOTIFY,
					"Fehler: Keine EAN-Daten fuer Artikel %s !\n",
						map->mapBUnit.unitArtNr);
		}			
		break;
	}
	return rv;
}


#define BUFSIZE 500

typedef struct {
	MAM  mam[MAM_TYPEIND_MAX];
} mam_buf_rec;

static int _create_mam(void *tid,const void *rec, int rectyp, mam_buf_rec *mam, const char *user)
{
	int 	rv	= 1;
	int 	i,cnt;
	char	mapid[BUFSIZE][RECID_LEN+1];		
	long	eanmenge[BUFSIZE];
	long	eangew[BUFSIZE];	
	char	eantyp[BUFSIZE][STRVALUELEN_MAETYP+1];
	long	bmenge[BUFSIZE];
	long	lmenge[BUFSIZE];
	long	bgew[BUFSIZE];
	long	lgew[BUFSIZE];
	mam_buf_rec buf;
	char	last_mapid[RECID_LEN+1];
	long	sbm=0,slm=0,rbm=0,rlm=0;
	long	sbg=0,slg=0,rbg=0,rlg=0;	
	MAK 	*mak=NULL;
	KAK		*kak=NULL;
	char	stmt[5000];
	char	*ptr;
	const char	*sel="select /*+ RULE */ map.id,"
				 " ean.unit_menge,ean.unit_gewnetto,ean.maetyp,"
				 " map.bunit_menge,map.lunit_menge,map.bunit_gewnetto,map.lunit_gewnetto ";
	char	from[200];
	
	memset(eanmenge,0,sizeof(eanmenge));
	memset(eangew,0,sizeof(eangew));	
	memset(eantyp,0,sizeof(eantyp));
	memset(mapid,0,sizeof(mapid));	
	memset(bmenge,0,sizeof(bmenge));
	memset(lmenge,0,sizeof(lmenge));
	memset(bgew,0,sizeof(bgew));
	memset(lgew,0,sizeof(lgew));
	memset(last_mapid,0,sizeof(last_mapid));	

	memset(&buf,0,sizeof(mam_buf_rec));

	if (!rec) return RETURN_ERROR;

	buf.mam[MAM_TYPEIND_PK].mamMaeTyp=MAETYP_PK;
	buf.mam[MAM_TYPEIND_GP].mamMaeTyp=MAETYP_GP;
	buf.mam[MAM_TYPEIND_TT].mamMaeTyp=MAETYP_TT;

	memset(from,0,sizeof(from ));
	switch(rectyp) {
		case RECTYP_MAK: 
			mak	= (MAK *)rec;
			StrCpy(buf.mam[MAM_TYPEIND_PK].mamParentId,mak->makId);
			buf.mam[MAM_TYPEIND_PK].mamRecTyp=RECTYP_MAK;
			buf.mam[MAM_TYPEIND_PK].mamTorNr=mak->makTorNr;
			buf.mam[MAM_TYPEIND_PK].mamStatus=mak->makStatus;
			buf.mam[MAM_TYPEIND_PK].mamMaFlags=mak->makMaFlags;
			StrCpy(buf.mam[MAM_TYPEIND_GP].mamParentId,mak->makId);
			buf.mam[MAM_TYPEIND_GP].mamRecTyp=RECTYP_MAK;
			buf.mam[MAM_TYPEIND_GP].mamTorNr=mak->makTorNr;			
			buf.mam[MAM_TYPEIND_GP].mamStatus=mak->makStatus;
			buf.mam[MAM_TYPEIND_GP].mamMaFlags=mak->makMaFlags;
			StrCpy(buf.mam[MAM_TYPEIND_TT].mamParentId,mak->makId);
			buf.mam[MAM_TYPEIND_TT].mamRecTyp=RECTYP_MAK;
			buf.mam[MAM_TYPEIND_TT].mamTorNr=mak->makTorNr;			
			buf.mam[MAM_TYPEIND_TT].mamStatus=mak->makStatus;
			buf.mam[MAM_TYPEIND_TT].mamMaFlags=mak->makMaFlags;
			sprintf(from,"from ean,map,mak where mak.id='%s' and map.parentid=mak.id",mak->makId);
			break;
		case RECTYP_KAK: 
			kak	= (KAK *)rec;
			StrCpy(buf.mam[MAM_TYPEIND_PK].mamParentId,kak->kakId);
			buf.mam[MAM_TYPEIND_PK].mamRecTyp=RECTYP_KAK;
			buf.mam[MAM_TYPEIND_PK].mamTorNr=kak->kakTorNr;
			buf.mam[MAM_TYPEIND_PK].mamKomZeit=kak->kakKomZeit;
			buf.mam[MAM_TYPEIND_PK].mamFreigabeZeit=time(NULL);
			buf.mam[MAM_TYPEIND_PK].mamStatus=kak->kakStatus;
			buf.mam[MAM_TYPEIND_PK].mamMaFlags=kak->kakMaFlags;			
			StrCpy(buf.mam[MAM_TYPEIND_GP].mamParentId,kak->kakId);
			buf.mam[MAM_TYPEIND_GP].mamRecTyp=RECTYP_KAK;
			buf.mam[MAM_TYPEIND_GP].mamTorNr=kak->kakTorNr;
			buf.mam[MAM_TYPEIND_GP].mamKomZeit=kak->kakKomZeit;			
			buf.mam[MAM_TYPEIND_GP].mamFreigabeZeit=time(NULL);		
			buf.mam[MAM_TYPEIND_GP].mamStatus=kak->kakStatus;
			buf.mam[MAM_TYPEIND_GP].mamMaFlags=kak->kakMaFlags;			
			StrCpy(buf.mam[MAM_TYPEIND_TT].mamParentId,kak->kakId);
			buf.mam[MAM_TYPEIND_TT].mamRecTyp=RECTYP_KAK;
			buf.mam[MAM_TYPEIND_TT].mamTorNr=kak->kakTorNr;
			buf.mam[MAM_TYPEIND_TT].mamKomZeit=kak->kakKomZeit;						
			buf.mam[MAM_TYPEIND_TT].mamFreigabeZeit=time(NULL);
			buf.mam[MAM_TYPEIND_TT].mamStatus=kak->kakStatus;
			buf.mam[MAM_TYPEIND_TT].mamMaFlags=kak->kakMaFlags;			
			sprintf(from,"from ean,map,kak where kak.id='%s' and map.kakid=kak.id",kak->kakId);
			break;
	}
	strcat(from," and map.postyp=" STR_ARTTYP_STD
				" and map.bunit_menge != 0"
				" and ean.unit_artnr=map.bunit_artnr");

	memset(stmt,0,sizeof(stmt));
	ptr=stmt;
	ptr+=sprintf(ptr,"%s %s and ean.maetyp="STR_MAETYP_GP,sel,from);
	ptr+=sprintf(ptr," union %s %s and ean.maetyp="STR_MAETYP_TT,sel,from);
	ptr+=sprintf(ptr," union %s %s and ean.typ=" STR_EANTYP_LE2
					 " and ean.maetyp="STR_MAETYP_PK,sel,from);
	ptr+=sprintf(ptr," union %s %s and ean.typ=" STR_EANTYP_EVE
					 " and ean.maetyp="STR_MAETYP_PK
					 " and not exists (select id from ean where ean.unit_artnr=map.bunit_artnr"
					 " and ean.typ="STR_EANTYP_LE2" and ean.maetyp="STR_MAETYP_PK")",sel,from);
	ptr+=sprintf(ptr," order by 1,2 desc");
				
 	cnt = 0;

	do {
		if (cnt==0)
			cnt	= TExecSqlX(tid,NULL,
					stmt,
					BUFSIZE,0,
					SELSTR(mapid[0],RECID_LEN+1),
					SELLONG(eanmenge[0]),
					SELLONG(eangew[0]),
					SELSTR(eantyp[0],STRVALUELEN_MAETYP+1),
					SELLONG(bmenge[0]),
					SELLONG(lmenge[0]),
					SELLONG(bgew[0]),
					SELLONG(lgew[0]),
					NULL);
		else
			cnt = TExecSqlV(tid,NULL,NULL,NULL,NULL,NULL);

		for (i=0;i<cnt;i++) {
			if (eanmenge[i]>0) {
				if (strcmp(last_mapid,mapid[i])) {	/* neuer Artikel */
					sbm=0; slm=0; rbm=0; rlm=0;
					sbg=0; slg=0; rbg=0; rlg=0;					

					sbm=bmenge[i]/eanmenge[i];		/* zuerst GP bzw. TT */
					rbm=bmenge[i]%eanmenge[i];
					slm=lmenge[i]/eanmenge[i];
					rlm=lmenge[i]%eanmenge[i];

					if (eangew[i]>0) {
						sbg=bgew[i]/eangew[i];
						rbg=bgew[i]%eangew[i];						
						slg=lgew[i]/eangew[i];
						rlg=lgew[i]%eangew[i];
					}
				}
				else {
					sbm=rbm/eanmenge[i];			/* LE2 bzw. Pick zuletzt */
					rbm=rbm%eanmenge[i];			/* mit Restmenge von GP */
					slm=rlm/eanmenge[i];
					rlm=rlm%eanmenge[i];
					if (eangew[i]>0) {
						sbg=rbg/eangew[i];
						rbg=rbg%eangew[i];						
						slg=rlg/eangew[i];
						rlg=rlg%eangew[i];
					}
				}
				StrCpy(last_mapid,mapid[i]);
			}
			else {	/* Notfall */
				sbm=0; slm=0; rbm=0; rlm=0;
				sbg=0; slg=0; rbg=0; rlg=0;

				if (rectyp==RECTYP_MAK)
					LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"Create_MAM: %s (%s) (%s), eanmenge=0, stmt=%s",
					   		mak->makRefNr,mak->makId,mapid[i],stmt);
				else
					LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"Create_MAM: %s (%s) (%s), eanmenge=0, stmt=%s",
					   		kak->kakRefNr,kak->kakId,mapid[i],stmt);

			}
			if (!strcmp(eantyp[i],"GP")) {
				buf.mam[MAM_TYPEIND_GP].mamAnzEinzel+=sbm*eanmenge[i];
				buf.mam[MAM_TYPEIND_GP].mamEAnzEinzel+=slm*eanmenge[i];    
				buf.mam[MAM_TYPEIND_GP].mamAnzahl+=sbm;
				buf.mam[MAM_TYPEIND_GP].mamEAnzahl+=slm;
				buf.mam[MAM_TYPEIND_GP].mamGewNetto+=sbg*eangew[i];
				buf.mam[MAM_TYPEIND_GP].mamEGewNetto+=slg*eangew[i];
				if (sbm>0)
					buf.mam[MAM_TYPEIND_GP].mamSubpos++;
			} else if (!strcmp(eantyp[i],"TT")) {
				buf.mam[MAM_TYPEIND_TT].mamAnzEinzel+=sbm*eanmenge[i];
				buf.mam[MAM_TYPEIND_TT].mamEAnzEinzel+=slm*eanmenge[i];    
				buf.mam[MAM_TYPEIND_TT].mamAnzahl+=sbm;
				buf.mam[MAM_TYPEIND_TT].mamEAnzahl+=slm;
				buf.mam[MAM_TYPEIND_TT].mamGewNetto+=sbg*eangew[i];
				buf.mam[MAM_TYPEIND_TT].mamEGewNetto+=slg*eangew[i];
				if (sbm>0)
					buf.mam[MAM_TYPEIND_TT].mamSubpos++;
			}
			else if (!strcmp(eantyp[i],"PK")) {
				buf.mam[MAM_TYPEIND_PK].mamAnzEinzel+=sbm*eanmenge[i]+rbm;
				buf.mam[MAM_TYPEIND_PK].mamEAnzEinzel+=slm*eanmenge[i]+rlm;    
				buf.mam[MAM_TYPEIND_PK].mamAnzahl+=sbm+rbm;
				buf.mam[MAM_TYPEIND_PK].mamEAnzahl+=slm+rlm;
				buf.mam[MAM_TYPEIND_PK].mamGewNetto+=sbg*eangew[i]+rbg;
				buf.mam[MAM_TYPEIND_PK].mamEGewNetto+=slg*eangew[i]+rlg;
				if (sbm+rbm>0)
					buf.mam[MAM_TYPEIND_PK].mamSubpos++;
			}
			else {
				if (rectyp==RECTYP_MAK)
					LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"Create_MAM: %s (%s) (%s), eantyp='%s'\nstmt=%s",
					   	mak->makRefNr,mak->makId,mapid[i],eantyp[i],stmt);
				else
					LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"Create_MAM: %s (%s) (%s), eantyp='%s'\nstmt=%s",
					   	kak->kakRefNr,kak->kakId,mapid[i],eantyp[i],stmt);

			}			
		}
		if (cnt<0 && TSqlError(tid)!=SqlNotFound)
			rv=RETURN_ERROR;

		if (cnt<=0) {
			if (rectyp==RECTYP_MAK)
				LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Create_MAM: %s (%s) (%s), cnt<0, %s\nstmt=%s",
				   		mak->makRefNr,mak->makId,mapid[i],SqlErrTxt,stmt);
			else
				LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Create_MAM: %s (%s) (%s), cnt<0, %s\n stmt=%s",
				   		kak->kakRefNr,kak->kakId,mapid[i],SqlErrTxt,stmt);
		}			
	} while (cnt==BUFSIZE);

	if (rv>0 && mam)
		memcpy(mam,&buf,sizeof(mam_buf_rec));

	if (buf.mam[MAM_TYPEIND_PK].mamAnzahl<=0 &&
		buf.mam[MAM_TYPEIND_GP].mamAnzahl<=0 &&
		buf.mam[MAM_TYPEIND_TT].mamAnzahl<=0) {
			
			if (rectyp==RECTYP_MAK)
				LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Create_MAM: %s (%s) (%s), alle Mengen 0\n",
				   		mak->makRefNr,mak->makId,mapid[i]);
			else
				LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Create_MAM: %s (%s) (%s), alle Mengen 0\n",
				   		kak->kakRefNr,kak->kakId,mapid[i]);
	}
	
	return rv;
}


/*
 * rectyp indiziert den rectyp fuer den mams angelegt werden soll (map/mak/tour)
 */


static int _insert_mam(const std::string & fac, void *tid,const void *rec, int rectyp, const char *user)
{
	int 	rv	= RETURN_ERROR;
	MAM 	mam,mamcpy;
	mam_buf_rec mambuf;
	int		eanrv;
	EAN		eanbuf[MAX_EANTYP];
	long 	smenge,rmenge;
	int		si;
	MAP 	*map;
	MAK 	*mak;
	time_t	komzeit;
	time_t	liefzeit;
	
	if (rec) {
		smenge	= rmenge	= 0;
		switch (rectyp) {
		case RECTYP_MAP: 
			map	= (MAP *)rec;
			
			/*
			 * lade alle EANs fuer gegebenen Artikel
			 */
			eanrv	= TDB_LoadEan4Art(tid,map->mapBUnit.unitArtNr,eanbuf,LOAD_EAN_DESC);
			if (eanrv>0) {
				memset(&mam,0,sizeof(MAM));
				mam.mamRecTyp	= RECTYP_MAP;
				mam.mamStatus	= MASTAT_NEU;
				StrCpy(mam.mamParentId,map->mapId);

				/*
				 * versuche GP oder TT abzuspalten, wenn moeglich
				 */
			    si	= TDB_SplitGPorTT(eanbuf,eanrv,map->mapBUnit.unitMenge,&smenge,&rmenge);
				if (si>=0) {		    
					mam.mamMaeTyp		= eanbuf[si].eanMaeTyp;
					mam.mamAnzahl		= smenge;
					mam.mamAnzEinzel	= smenge*eanbuf[si].eanUnit.unitMenge;		
					mam.mamGewNetto		= smenge*eanbuf[si].eanUnit.unitGewNetto;
					mam.mamGewBrutto	= smenge*eanbuf[si].eanGewBrutto;
					mamcpy	= mam;
					if (TExecStdSql(tid,StdNselect,TN_MAM,&mamcpy)>=0) {
						rv	= 0;
					} else {
						if (user)
							SetHist(TN_MAM, &mam, HIST_INSERT, user);
						rv	= TExecStdSql(tid,StdNinsert,TN_MAM,&mam);
						if( rv < 0 ) {
							LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
						}
					}
				} else {

					/*
					 * keine Abspaltung moeglich - ok, restmenge=bestellmenge
					 */
					rv	= 0;
					rmenge	= map->mapBUnit.unitMenge;
				}
				if (rv>=0) {
					mam.mamMaeTyp	= MAETYP_PK;

					/*
					 * versuche LE2 typ zu finden und ggf. die anzahl entsprechend zu setzen
					 */
					si	= TDB_FindEanType(eanbuf,eanrv,EANTYP_LE2,-1);
					if (si>=0) {
						if (eanbuf[si].eanUnit.unitMenge>0) {
							mam.mamAnzahl	= rmenge/eanbuf[si].eanUnit.unitMenge;
							mam.mamAnzahl	+= rmenge%eanbuf[si].eanUnit.unitMenge>0?1:0;
						} else mam.mamAnzahl	= 0;
						mam.mamAnzEinzel	= rmenge;		
						si	= TDB_FindEanType(eanbuf,eanrv,EANTYP_EVE,-1);
						if (si<0) {
							rv	= -1;
							LogPrintf( fac, LT_ALERT, "TDB_FindEanType EVE liefert %d fÃ¼r Artikel: %s", si, map->mapBUnit.unitArtNr );
							break;
						}
						mam.mamGewNetto		= mam.mamAnzEinzel*eanbuf[si].eanUnit.unitGewNetto;
						mam.mamGewBrutto	= mam.mamAnzEinzel*eanbuf[si].eanGewBrutto;
					} else {
						/*
						 * bleibt nur noch der EVE typ - dieser muss existieren
						 */
						si	= TDB_FindEanType(eanbuf,eanrv,EANTYP_EVE,-1);
						if (si<0) {
							rv	= -1;
							LogPrintf( fac, LT_ALERT, "TDB_FindEanType EVE liefert %d", si );
							break;
						}
						mam.mamAnzahl		= mam.mamAnzEinzel	= rmenge;		
						mam.mamGewNetto		= mam.mamAnzahl*eanbuf[si].eanUnit.unitGewNetto;
						mam.mamGewBrutto	= mam.mamAnzahl*eanbuf[si].eanGewBrutto;
 					}
					mamcpy	= mam;
					if (TExecStdSql(tid,StdNselect,TN_MAM,&mamcpy)>=0) {
						rv	= 0;
					} else {
						if (user)
							SetHist(TN_MAM, &mam, HIST_INSERT, user);
							rv	= TExecStdSql(tid,StdNinsert,TN_MAM,&mam);
							if( rv < 0 ) {
								LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
							}
 					}
				}
			}	else {
				LogPrintf( fac, LT_ALERT, "eanrv: %d", eanrv );
			}
			break;
		case RECTYP_MAK:
		case RECTYP_KAK:
			memset(&mambuf,0,sizeof(mam_buf_rec));
			rv=_create_mam(tid,rec, rectyp, &mambuf, user);
			if (rv>0) {
				if (TExecStdSql(tid,StdNselect,TN_MAM,&(mambuf.mam[MAM_TYPEIND_GP]))>=0) {
					rv	= 0;
					LogPrintf(FAC_LIB,LT_NOTIFY,"Insert_MAM: GP-MAM fuer %s schon vorhanden!",
					   		mambuf.mam[MAM_TYPEIND_GP].mamParentId);
				} else {
					if (user)
						SetHist(TN_MAM, &(mambuf.mam[MAM_TYPEIND_GP]), HIST_INSERT, user);
					rv	= TExecStdSql(tid,StdNinsert,TN_MAM,&(mambuf.mam[MAM_TYPEIND_GP]));
				}
				if (TExecStdSql(tid,StdNselect,TN_MAM,&(mambuf.mam[MAM_TYPEIND_PK]))>=0) {
					rv	= 0;
					LogPrintf(FAC_LIB,LT_NOTIFY,"Insert_MAM: PK-MAM fuer %s schon vorhanden!",
					   		mambuf.mam[MAM_TYPEIND_GP].mamParentId);
				} else {
					if (user)
						SetHist(TN_MAM, &(mambuf.mam[MAM_TYPEIND_PK]), HIST_INSERT, user);
					rv	= TExecStdSql(tid,StdNinsert,TN_MAM,&(mambuf.mam[MAM_TYPEIND_PK]));
				}
				if (TExecStdSql(tid,StdNselect,TN_MAM,&(mambuf.mam[MAM_TYPEIND_TT]))>=0) {
					rv	= 0;
					LogPrintf(FAC_LIB,LT_NOTIFY,"Insert_MAM: TT-MAM fuer %s schon vorhanden!",
					   		mambuf.mam[MAM_TYPEIND_GP].mamParentId);
				} else {
					if (user)
						SetHist(TN_MAM, &(mambuf.mam[MAM_TYPEIND_TT]), HIST_INSERT, user);
					rv	= TExecStdSql(tid,StdNinsert,TN_MAM,&(mambuf.mam[MAM_TYPEIND_TT]));
				}
			} else {
				LogPrintf(FAC_LIB,LT_NOTIFY,"Insert_MAM: error _create_mam(%s): rv=%d!",
					mambuf.mam[MAM_TYPEIND_GP].mamParentId,rv);
			}
			break;
		case RECTYP_TOUR:
		  {
			mak	= (MAK *)rec;

			/*
			 * versuche bestehende PK tour-mam zu lesen
			 * sonst erzeuge ein neues
			 */
			 
			if (TExecSql(tid,
					"select %MAM from mam"
					" where rectyp="STR_RECTYP_TOUR
					" and parentid=:a"
					" and maetyp="STR_MAETYP_PK,
					SQLSTRING(mak->makTourNr),
					SELSTRUCT(TN_MAM,mam),
					NULL)<=0) {
				memset(&mam,0,sizeof(MAM));
				StrCpy(mam.mamParentId,mak->makTourNr);
				mam.mamRecTyp	= RECTYP_TOUR;
				mam.mamMaeTyp	= MAETYP_PK;
				mam.mamMaFlags	= MAFLAG_O;
				mam.mamStatus	= MASTAT_NEU;
				mam.mamTorNr	= mak->makTorNr;
				if (user)
					SetHist(TN_MAM, &mam, HIST_INSERT, user);
				if (TExecStdSql(tid,StdNinsert,TN_MAM,&mam)<0)
					return RETURN_ERROR;
			}

			/*
 			 * update mengen
			 */
			
			int dbrv;

			dbrv = TExecSql(tid,
					"select SUM(mam.anzahl),SUM(mam.anzeinzel),"
					" SUM(mam.gewnetto),SUM(mam.gewbrutto),"
					" COUNT(mak.id),MIN(mak.komzeit),MIN(mak.liefzeit)"
			 		" from mam,mak"
					" where mak.tournr=:a"
					" and mam.rectyp="STR_RECTYP_MAK
					" and mam.parentid=mak.id"
					" and mam.maetyp="STR_MAETYP_PK,
					SELLONG(mam.mamAnzahl),
					SELLONG(mam.mamAnzEinzel),
					SELLONG(mam.mamGewNetto),
					SELLONG(mam.mamGewBrutto),
					SELLONG(mam.mamSubpos),
					SELTIMET(komzeit),
					SELTIMET(liefzeit),					
					SQLSTRING(mak->makTourNr),
					NULL);

			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			mam.mamKomZeit = komzeit;
			mam.mamLiefZeit = liefzeit;			
			if (user)
				SetHist(TN_MAM, &mam, HIST_UPDATE, user);
			if (TExecStdSql(tid,StdNupdate,TN_MAM,&mam)<0)
				return RETURN_ERROR;
					
			mamcpy	= mam;

			/*
			 * versuche bestehende GP tour-mam zu lesen
			 * sonst erzeuge ein neues
			 */
			 
			if (TExecSql(tid,
					"select %MAM from mam"
					" where mam.rectyp="STR_RECTYP_TOUR					
					" and parentid=:a"
					" and maetyp="STR_MAETYP_GP,
					SQLSTRING(mak->makTourNr),
					SELSTRUCT(TN_MAM,mam),
					NULL)<=0) {
				memset(&mam,0,sizeof(MAM));
				StrCpy(mam.mamParentId,mak->makTourNr);
				mam.mamRecTyp	= RECTYP_TOUR;
				mam.mamMaeTyp	= MAETYP_GP;
				mam.mamMaFlags	= MAFLAG_O;
				mam.mamStatus	= MASTAT_NEU;
				mam.mamTorNr	= mak->makTorNr;
				if (user)
					SetHist(TN_MAM, &mam, HIST_INSERT, user);
				if (TExecStdSql(tid,StdNinsert,TN_MAM,&mam)<0)
					return RETURN_ERROR;
			}

			/*
 			 * update mengen
			 */
			
			dbrv = TExecSql(tid,
					"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto) "
					"from mam,mak where mak.tournr=:a "
					" and mam.rectyp="STR_RECTYP_MAK
					" and mam.parentid=mak.id and mam.maetyp="STR_MAETYP_GP,
					SELLONG(mam.mamAnzahl),
					SELLONG(mam.mamAnzEinzel),
					SELLONG(mam.mamGewNetto),
					SELLONG(mam.mamGewBrutto),
					SQLSTRING(mak->makTourNr),
					NULL);

			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			mam.mamSubpos	= mamcpy.mamSubpos;
			mam.mamKomZeit = komzeit;
			mam.mamLiefZeit = liefzeit;
			
			if (user)
				SetHist(TN_MAM, &mam, HIST_UPDATE, user);
			if (TExecStdSql(tid,StdNupdate,TN_MAM,&mam)<0)
				return RETURN_ERROR;
				

			/*
			 * versuche bestehende TT tour-mam zu lesen
			 * sonst erzeuge ein neues
			 */
			 
			if (TExecSql(tid,
					"select %MAM from mam"
					" where rectyp="STR_RECTYP_TOUR
					" and parentid=:a"
					" and maetyp="STR_MAETYP_TT,
					SQLSTRING(mak->makTourNr),
					SELSTRUCT(TN_MAM,mam),
					NULL)<=0) {
				memset(&mam,0,sizeof(MAM));
				StrCpy(mam.mamParentId,mak->makTourNr);
				mam.mamRecTyp	= RECTYP_TOUR;
				mam.mamMaeTyp	= MAETYP_TT;
				mam.mamMaFlags	= MAFLAG_O;
				mam.mamStatus	= MASTAT_NEU;
				mam.mamTorNr	= mak->makTorNr;
				if (user)
					SetHist(TN_MAM, &mam, HIST_INSERT, user);
				if (TExecStdSql(tid,StdNinsert,TN_MAM,&mam)<0)
					return RETURN_ERROR;
			}

			/*
 			 * update mengen
			 */
			
			dbrv = TExecSql(tid,
					"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto) "
					"from mam,mak where mak.tournr=:a"
					" and mam.rectyp="STR_RECTYP_MAK
					" and mam.parentid=mak.id and mam.maetyp="STR_MAETYP_TT,
					SELLONG(mam.mamAnzahl),
					SELLONG(mam.mamAnzEinzel),
					SELLONG(mam.mamGewNetto),
					SELLONG(mam.mamGewBrutto),
					SQLSTRING(mak->makTourNr),
					NULL);

			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			mam.mamSubpos	= mamcpy.mamSubpos;
			mam.mamKomZeit = komzeit;
			mam.mamLiefZeit = liefzeit;
			
			if (user)
				SetHist(TN_MAM, &mam, HIST_UPDATE, user);
			if (TExecStdSql(tid,StdNupdate,TN_MAM,&mam)<0)
				return RETURN_ERROR;

			rv	= 0;
		  }
		  break;
		}
	}
	return rv;
}




/*
 * Parameter:
 *	tid		transaktions id fuer db-calls
 *	id		record id des ma eintrags
 *	rec		zeiger auf entsprechendes record (kann auch NULL sein)
 *	control	Art der anzulegenden Records
 *	user	user fuer log (optional)
 * Ergebnis:
 *  siehe TDB_SetMaStatus()
 */
int TDB_CreateMam( const std::string & fac, void *tid, char *id, void *rec, int control, const char *user)
{
	int 	rv	= TDB_MASTAT_NODATA;
	int 	idtyp;
	MAP 	*map;
	MAK 	*mak;
	MAK 	mapmak;
	KAK		*kak;
	int 	freerec	= FALSE;

	if ((!id&&!rec))
		return TDB_MASTAT_NODATA;

	if (rec)
		id	=  (char *)rec;		/* hack: id is the first entry */
	idtyp	= _recid2rectype(id);
	if (idtyp<0)
		return TDB_MASTAT_NODATA;

	if (!rec) {
		rv	= _lock_rec(tid,id,idtyp,FALSE,&rec,NULL);
		if (rv!=TDB_MASTAT_OK)
			return rv;
		freerec	= TRUE;
	} else {
		freerec	= FALSE;
	}

	switch (control) {

	case TDB_CREMAM_MAE:

		if (idtyp!=RECTYP_MAP)
			return TDB_MASTAT_IMP;
		map	= (MAP *)rec;
		rv	= _lock_rec(tid,map->mapParentId,RECTYP_MAK,FALSE,NULL,&mapmak);
		if (rv!=TDB_MASTAT_OK)
 			return rv;
		switch (mapmak.makBwArt) {
		case BWART_ZPR:
		case BWART_ZLI:
		case BWART_ZKU:
		case BWART_ZSP:
			if (_insert_mae(fac, tid,map,&map->mapBUnit,IM_PK_ONLY,user)<0) {
				rv	= TDB_MASTAT_ERR;
				LogPrintf( fac, LT_ALERT, "_insert_mae fehlgeschlagen" );
			}
			break;
		case BWART_AKU:
		case BWART_ALI:
		case BWART_ASP:

		  if( verplaner_neu( *map ) ) {
			if (_insert_mae(fac, tid,map,&map->mapBUnit,IM_GP,user)<0) {
			  LogPrintf( fac, LT_ALERT, "Fehler beim Erzeugen der MAE Einträge für die Ganzpaletten" );
			  rv	= TDB_MASTAT_ERR;
			}
		  } else {
			if (_insert_mae(fac, tid,map,&map->mapBUnit,IM_PKNGP,user)<0) {
			  LogPrintf( fac, LT_ALERT, "Fehler beim Erzeugen der MAE Einträge für die Ganzpaletten" );
			  rv	= TDB_MASTAT_ERR;
			}
		  }
		  break;

		default:
			rv	= TDB_MASTAT_IMP;
		} /* switch (BwArt) */
		break;

	case TDB_CREMAM_MAMMAP:

		if (idtyp!=RECTYP_MAP)
			return TDB_MASTAT_IMP;
		map	= (MAP *)rec;
		rv	= _lock_rec(tid,map->mapParentId,RECTYP_MAK,FALSE,NULL,&mapmak);
		if (rv!=TDB_MASTAT_OK)
			return rv;
		switch (mapmak.makBwArt) {
		case BWART_ZPR:
 			if (_insert_mam(fac, tid,map,RECTYP_MAP,user)<0) {
				rv	= TDB_MASTAT_ERR;
				LogPrintf( fac, LT_ALERT, "_insert_mam RECTYP_MAP fehlgeschlagen" );
			}
			break;
		case BWART_ZLI:
		case BWART_ZKU:
		case BWART_ZSP:
		case BWART_AKU:
		case BWART_ALI:
		case BWART_ASP:
			break;
		default:
			rv	= TDB_MASTAT_IMP;
		}
		break;

	case TDB_CREMAM_MAMMAK:

		if (idtyp!=RECTYP_MAK)
			return TDB_MASTAT_IMP;
		mak	= (MAK *)rec;
		switch (mak->makBwArt) {
		case BWART_ZLI:
		case BWART_ZKU:
		case BWART_ZSP:
		case BWART_AKU:
		case BWART_ALI:
		case BWART_ASP:
 			if (_insert_mam(fac, tid,mak,RECTYP_MAK,user)<0) {
				rv	= TDB_MASTAT_ERR;
				LogPrintf( fac, LT_ALERT, "_insert_mam fehlgeschlagen" );
			} else {
				rv	= TDB_MASTAT_OK;
			}
			break;
		default:
			rv	= TDB_MASTAT_IMP;
		}
		break;

	case TDB_CREMAM_MAMTOUR:

		if (idtyp!=RECTYP_MAK)
			return TDB_MASTAT_IMP;
		mak	= (MAK *)rec;
		switch (mak->makBwArt) {
		case BWART_AKU:
		case BWART_ALI:
		case BWART_ASP:
 			if (_insert_mam(fac, tid,mak,RECTYP_TOUR,user)<0) {
				rv	= TDB_MASTAT_ERR;
				LogPrintf( fac, LT_ALERT, "_insert_mam fehlgeschlagen" );
			} else {
				rv	= TDB_MASTAT_OK;
			}
			break;
		default:
			rv	= TDB_MASTAT_IMP;
			break;
		}
		break;

	case TDB_CREMAM_MAMKAK:

		if (idtyp!=RECTYP_KAK)
			return TDB_MASTAT_IMP;
		kak	= (KAK *)rec;
		if (_insert_mam(fac, tid,kak,RECTYP_KAK,user)<0) {
			LogPrintf( fac, LT_ALERT, "_insert_mam fehlgeschlagen" );
			rv	= TDB_MASTAT_ERR;
		} else {
			rv	= TDB_MASTAT_OK;
		}
		break;

 	default:
		rv	= TDB_MASTAT_NODATA;
		break;
	}	/* switch(control) */
	if (freerec)
		MemDeallocate(rec);
	return rv;
}


/*
 * liefert 1 (OK) oder 0 (Fehler)
 */
static int _mak2kak(void *tid,KAK *kak, MAK *mak,const char *usr)
{
	int dbrv	= 0;

	if (kak&&mak) {
		memset(kak,0,sizeof(KAK));
		kak->kakBatchNr	= mak->makBatchNr;
		kak->kakBatchAnz = 1;	/* Anzahl Kunden default 1*/
		kak->kakTorNr	= mak->makTorNr;
		StrCpy(kak->kakTourNr,mak->makTourNr);
		/* ??? ok ??? */
		kak->kakStatus	= MASTAT_FREIGABE;
		kak->kakMaFlags	= MAFLAG_O;
		/* StrCpy(kak->kakKomissionierer,?); */
		StrCpy(kak->kakInfo1,mak->makInfo1);
		/* ---------- */
		kak->kakKomZeit	= mak->makKomZeit;
/*		time(&kak->kakZuteil);*/
		time(&kak->kakStartZeit);
		if (usr)
			SetHist(TN_KAK, kak, HIST_INSERT, usr);
		dbrv = TDB_GetRecId(tid, TN_KAK, kak->kakId);
	}
	return dbrv;
}



/*
 * loescht alle MAMs zum gegebenen recid/rectyp und ggf. subrecords (MAP/MAK/TOUR)
 */
void TDB_DeleteMam(void *tid,const char *recid,int rectyp)
{
	int rv=0;
	int myrv=0;
	time_t mynulltime=0;
	KAK kak;

	if (recid) {
		switch (rectyp) {
		case RECTYP_MAP:
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_MAP
					" and parentid=:a",
					SQLSTRING(recid),
					NULL);

            if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
                LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
            }

			break;
		case RECTYP_MAK:

			/*
			 * loesche alle mams zu allen maps  des auftrags
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_MAP
					" and parentid in (select id from map where parentid=:a)",
					SQLSTRING(recid),
					NULL);

			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			/*
			 * loesche mak-mam
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_MAK
					" and parentid=:a",
					SQLSTRING(recid),
					NULL);

			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		case RECTYP_KAK:
			/* sis: Tourenprotokollierung vorher
			 * kak holen !!
             */
			memset(&kak,0,sizeof(KAK));
			myrv=TExecSql(tid,
					"Select %KAK from kak where"
					" id=:a",
					SQLSTRING(recid),
					SELSTRUCT(TN_KAK,kak),
					NULL);

			if( myrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			if (myrv>0)
			{
				myrv=WriteTourProtAll(tid,kak.kakTourNr,mynulltime);
			}

			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_KAK
					" and parentid=:a",
					SQLSTRING(recid),
					NULL);

			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		case RECTYP_TOUR:
            /* sis: Tourenprotokollierung hier */
            /* Freigabe-Zeiten schreiben */
            myrv=WriteTourProtAll(tid,recid,mynulltime);
            /* Fertig-Zeiten schreiben */
            myrv=WriteTourProtById(tid,PROT_FERTIG,recid,mynulltime);

			/*
			 * loesche alle mams zu allen maps der tour
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_MAP
					" and parentid in "
					"(select id from map where parentid in "
					"(select id from mak where tournr=:a))",
					SQLSTRING(recid),
					NULL);

			if( rv < 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			/*
			 * loesche mak-mams
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_MAK
					" and parentid in (select id from mak where tournr=:a)",
					SQLSTRING(recid),
					NULL);

			if( rv < 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s %d", TSqlErrTxt(tid), rv);
			}

			/*
			 * loesche kak-mams
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_KAK
					" and parentid in (select id from kak where tournr=:a)",
					SQLSTRING(recid),
					NULL);

			if( rv < 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			/*
			 * loesche tour-mams
			 */
			rv=TExecSql(tid,
					"delete from mam where"
					" rectyp="STR_RECTYP_TOUR
					" and parentid=:a",
					SQLSTRING(recid),
					NULL);

			if( rv < 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		}
	}
}


/*
 * loescht alle MAEs zum gegebenen recid/rectyp und ggf. subrecords (MAP/MAK/TOUR)
 */
void TDB_DeleteMae(void *tid,const char *recid,int rectyp)
{
  int rv;

	if (recid) {
		switch (rectyp) {
		case RECTYP_MAP:
			rv = TExecSql(tid,
					"delete from mae where parentid=:a",
					SQLSTRING(recid),
					NULL);
			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		case RECTYP_MAK:

			/*
			 * loesche alle maes zu allen maps des auftrags
			 */
			rv = TExecSql(tid,
					"delete from mae where parentid in (select id from map where parentid=:a)",
					SQLSTRING(recid),
					NULL);
			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		case RECTYP_TOUR:

			/*
			 * loesche alle maes zu allen maps der tour
			 */
			rv = TExecSql(tid,
					"delete from mae where parentid in "
					"(select id from map where parentid in "
					"(select id from mak where tournr=:a))",
					SQLSTRING(recid),
					NULL);

			if( rv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			break;
		}
	}
}


/*
 * loescht alle TPAs/TEKs/TEPs/LHMs zum gegebenen Auftrag
 */
static int TDB_DeleteTek(const std::string & fac, void *tid,const char *recid, const std::string & usr)
{
	int rv = TDB_ProtLHM(tid, recid, usr.c_str());

	if (rv != DB_UTIL_SQLOK ) {
		LogPrintf( fac, LT_ALERT, "TDB_ProtLHM fehlgeschlagen: Error: %s", TSqlErrTxt(tid));
		return TDB_MASTAT_ERR;
	}

	FetchTable<TEK> vTek(tid, NULL, TN_TEK,
						 format("," TN_TEP "," TN_MAP " where "
								TCN_MAP_ParentId " = '%s' and "
								TCN_TEP_MapId " = " TCN_MAP_Id " and "
								TCN_TEK_TeId " = " TCN_TEP_TeId " order by " TCN_TEK_TeId, recid));

	if (!vTek) {
		LogPrintf( fac, LT_ALERT, "Fehler bei %s SqlError: %s", vTek.get_sql(), TSqlErrTxt(tid));
		return TDB_MASTAT_ERR;
	}
	else if (vTek.empty()) {
		LogPrintf( fac, LT_ALERT, "nix gefunden mit %s", vTek.get_sql());
		return TDB_MASTAT_OK;
	}

	std::string last_teid;

	for (unsigned i = 0; i < vTek.size(); i++) {

		if (last_teid == vTek[i].tekTeId) {
			continue;
		}
		else {
			last_teid = vTek[i].tekTeId;
		}

		bool prot_tek_tep = true;

		FES fes = {};
		StrCpy(fes.fesFeldId, vTek[i].tekPos.FeldId);
		rv = TExecStdSql(tid, StdNselect, TN_FES, &fes);

		if (rv != 1) {
			LogPrintf( fac, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			return TDB_MASTAT_ERR;
		}

		if (fes.fesWHLoc != WH_UNDEF) {

			// hier ist ein Ausbuchen auch erlaubt.
			prot_tek_tep = true;

		}
		else if (strstr(vTek[i].tekPos.FeldId, "TO-") != &vTek[i].tekPos.FeldId[0]) {

			LogPrintf( fac, LT_ALERT, "Palette %s befindet sich nicht am TO-XX sondern auf %s "
			" und wird dher nicht protokolliert.",
			vTek[i].tekTeId, vTek[i].tekPos.FeldId);
		}


		if (prot_tek_tep) {
			rv = TDB_protTekTep(fac, tid, vTek[i].tekTeId, usr.c_str(), TRUE);

			if (rv != DB_UTIL_SQLOK) {
				LogPrintf( fac, LT_ALERT, "Fehler beim Protokollieren der Te: %s",
				vTek[i].tekTeId);
				return TDB_MASTAT_ERR;
			}

			int dbrv = TExecSql(tid, "delete from lhm where teid=:a",
								SQLSTR(vTek[i].tekTeId, TEID_LEN+1),
								NULL);

			if (dbrv < 0 && TSqlError(tid) != SqlNotFound) {
				LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
				return TDB_MASTAT_ERR;
			}

		} // if prot_tek_tep
	} // for

	return TDB_MASTAT_OK;
}



/*
 * versuche kak zu map/mak zu finden. fuer normale und gesplittete maks wird nach kakid in den maps des
 * auftrages gesucht, welche die selbe splitnr haben. ist der mak teil eines batches, so wird in allen
 * maps des batches gesucht. wird ein kak gefunden, so wird er im kakbuf zurueckgeliefert.
 * RETURN:	FALSE	-	not found
 *			TRUE	-	found
 */
static int _find_kak(void *tid, MAK *mak, MAP *map, KAK *kakbuf)
{
	int rv	= FALSE;

	if (mak&&map&&kakbuf) {
		if (mak->makBatchNr) {		/* ist teil eines batchs */
			rv	= TExecSql(tid,"select %KAK from kak,map,mak where "
							"kak.id=map.kakid and "
							"map.parentid=mak.id and "
							"mak.tournr=:a and "
							"mak.batchnr=:b and "
							"(mak.bwart="STR_BWART_AKU" or "
							" mak.bwart="STR_BWART_ALI" or "
							" mak.bwart="STR_BWART_ASP") ",
							SELSTRUCT(TN_KAK,*kakbuf),
							SQLSTRING(mak->makTourNr),
							SQLLONG(mak->makBatchNr),
							NULL)>0?TRUE:FALSE;
		} else {					/* normal oder split */
			rv	= TExecSql(tid,"select %KAK from kak,map where "
							"map.parentid=:a and "
							"map.splitnr=:b and "
							"kak.id=map.kakid",
							SELSTRUCT(TN_KAK,*kakbuf),
							SQLSTRING(mak->makId),
							SQLLONG(map->mapSplitNr),
							NULL)>0?TRUE:FALSE;
		}
	}
	return rv;
}

/*
 * ist der mak gesplittet, so wird versucht, fuer die splitnr des gegebenen maps alle verpackungsmaps
 * des maks zu duplizieren. existieren diese bereits (mit identer kakid!), so wird nur ok geliefert.
 * in allen duplikaten wird dann die kakid des maps eingetragen. im map muss die kakid bereits gueltig sein!!
 * RETURN:	TRUE	- 	alles Ok
 *			FALSE	- 	Fehler (DB,...)
 */
static int _split_vermaps(void *tid,MAK *mak,MAP *map,const char *usr)
{
	int	 	rv	= FALSE;
	LStick	l,vermaplist	= NULL;
	char	buf[RECID_LEN+1];
	MAP		*tmpmap;
	int		dbrv;

	if (mak&&map&&map->mapKakId[0]) {
		if (map->mapSplitNr) {

			/*
			 * map ist teil eines splits.
			 * versuche mapduplikate fuer die geg. kakid zu finden...
			 */

			if (TExecSql(tid,"select map.id from map where "
							"map.postyp<>"STR_ARTTYP_STD" and "
							"map.splitnr=:a and "
							"map.kakid=:b",
							SELSTR(buf,RECID_LEN+1),
							SQLLONG(map->mapSplitNr),
							SQLSTRING(map->mapKakId),
							NULL)<=0) {

				/*
				 * noch keine da -> lies liste der originalen vermaps ...
				 */

				vermaplist	= _load_rec_list(tid, RECTYP_MAP, FALSE,
								mak->makId,
								"map.postyp<>"STR_ARTTYP_STD
								" and map.kakid is NULL");

				/*
				 * erzeuge fuer alle lhm maps ein duplikat fuer diese splitnr/kakid
				 */

				dbrv	= TRUE;
				for (l=vermaplist;dbrv&&l&&l->data;l=l->next) {
					tmpmap	= (MAP *)l->data;
					StrCpy(tmpmap->mapKakId,map->mapKakId);
					tmpmap->mapSplitNr	= map->mapSplitNr;
					dbrv	= TDB_GetRecId(tid, TN_MAP, tmpmap->mapId);
					if (usr)
						SetHist(TN_MAP, tmpmap, HIST_INSERT, usr);
					dbrv	= dbrv>0?TExecStdSql(tid,StdNinsert,TN_MAP,tmpmap):FALSE;
				}
				_free_rec_list(vermaplist);
				if (dbrv) rv	= TRUE;	/* all done (od. keine lhm maps zu diesem auftrag), ok */
			} else rv	= TRUE;		/* duplikate zu diesem split existieren bereits, ok */
		} else {

			/*
			 * kein split.
			 * lies liste der vermaps die noch keine kakid haben ...
			 */

			vermaplist	= _load_rec_list(tid, RECTYP_MAP, TRUE, mak->makId, "map.postyp<>'STD' and map.kakid is NULL");

			/*
			 * kopiere fuer alle lhm maps die kakid
			 */

			dbrv	= TRUE;
			for (l=vermaplist;dbrv&&l&&l->data;l=l->next) {
				tmpmap	= (MAP *)l->data;
				StrCpy(tmpmap->mapKakId,map->mapKakId);
				if (usr)
					SetHist(TN_MAP, tmpmap, HIST_UPDATE, usr);
				dbrv	= dbrv>0?TExecStdSql(tid,StdNupdate,TN_MAP,tmpmap):FALSE;
			}
			_free_rec_list(vermaplist);
			if (dbrv) rv	= TRUE;	/* all done (od. keine vermaps zu diesem auftrag), ok */
		}
	}
	return rv;
}


/*
 * update alle MAP-MAMs Erfuellungsmengen/BMengen des uebergebenen Auftrags
 * aus den MAEs
 */
static void _update_makmapMAMs_from_MAE(void *tid,const char *recid,int which,const char *user)
{
	char 	mapid[1000][RECID_LEN+1];
	int		mapcnt,pkcnt,gpcnt;
	long	mpk[1000],mgp[1000];
	int 	dbrv;
	
	/*
	 * hole LE2/Pick Mengen aus zugehoerigen eans
	 */

	memset(mpk,0,sizeof(mpk));	 
	pkcnt	= TExecSqlX(tid,NULL,
				"select map.id,ean.unit_menge from ean,map"
				" where map.parentid=:a and"
				" map.postyp="STR_ARTTYP_STD" and"
				" (map.status="STR_MASTAT_FREIGABE" or" 
				" map.status="STR_MASTAT_FERTIG") and" 
				" ean.unit_artnr=map.bunit_artnr and"
				" ean.maetyp="STR_MAETYP_PK" and"
				" ean.typ="STR_EANTYP_LE2
				" union"
				" select map.id,1 from ean,map"
				" where map.parentid=:b and"
				" map.postyp="STR_ARTTYP_STD" and"
				" (map.status="STR_MASTAT_FREIGABE" or" 
				" map.status="STR_MASTAT_FERTIG") and" 
				" ean.unit_artnr=map.bunit_artnr and"
				" not exists (select id from ean where"
				" unit_artnr=map.bunit_artnr and"
				" maetyp="STR_MAETYP_PK" and"
				" typ="STR_EANTYP_LE2")"
				" order by 1",
				1000,0,
				SQLSTRING(recid),
				SQLSTRING(recid),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mpk[0]),
				NULL);

	/*
	 * hole GP/TT Mengen aus zugehoerigen eans
	 */
	 
	memset(mgp,0,sizeof(mgp));	 
	gpcnt	= TExecSqlX(tid,NULL,
				"select map.id,ean.unit_menge from ean,map"
				" where map.parentid=:a and"
				" map.postyp="STR_ARTTYP_STD" and"
				" (map.status="STR_MASTAT_FREIGABE" or" 
				" map.status="STR_MASTAT_FERTIG") and" 
				" ean.unit_artnr=map.bunit_artnr and"
				" (ean.maetyp="STR_MAETYP_GP" or ean.maetyp="STR_MAETYP_TT")"
				" union"
				" select map.id,1 from ean,map"
				" where map.parentid=:b and"
				" map.postyp="STR_ARTTYP_STD" and"
				" (map.status="STR_MASTAT_FREIGABE" or" 
				" map.status="STR_MASTAT_FERTIG") and" 
				" ean.unit_artnr=map.bunit_artnr and"
				" not exists (select id from ean where"
				" unit_artnr=map.bunit_artnr and"
				" (maetyp="STR_MAETYP_GP" or maetyp="STR_MAETYP_TT"))"
				" order by 1",
				1000,0,
				SQLSTRING(recid),
				SQLSTRING(recid),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mgp[0]),
				NULL);

	if (pkcnt!=gpcnt||pkcnt<=0) {
		return;
	} else {
		mapcnt	= pkcnt;
	}
	
	for (;mapcnt>0;mapcnt--) {
		switch (which) {

		case TDB_UPD_INIT:
			dbrv = TExecSql(tid,
				"update mam set anzahl=anzeinzel/:a, "
				"anzeinzel=(select SUM(bunit_menge) from mae "
				"where mae.maetyp="STR_MAETYP_PK" and parentid=:b), "
				"gewnetto=(select SUM(bunit_gewnetto) from mae "
				"where mae.maetyp="STR_MAETYP_PK" and parentid=:c) "
				"where maetyp="STR_MAETYP_PK" and parentid=:d",
				SQLLONG(mpk[mapcnt-1]),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				NULL);

			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			dbrv = TExecSql(tid,
				"update mam "
				"set anzahl=anzeinzel/:a, "
				"anzeinzel="
					"(select SUM(bunit_menge) from mae "
					"where (mae.maetyp="STR_MAETYP_TT" or mae.maetyp="STR_MAETYP_GP") "
					"and parentid=:b), "
				"gewnetto="
					"(select SUM(bunit_gewnetto) from mae "
					"where (mae.maetyp="STR_MAETYP_TT" or mae.maetyp="STR_MAETYP_GP") "
					"and parentid=:c) "
				"where (maetyp="STR_MAETYP_TT" or maetyp="STR_MAETYP_GP") "
				"and parentid=:d",
				SQLLONG(mgp[mapcnt-1]),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				SQLSTR(mapid[mapcnt-1],RECID_LEN+1),
				NULL);

			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}
		    break;
		
		case TDB_UPD_ERF:

			dbrv	= TExecSql(tid,
						"update mam set eanzahl=eanzeinzel/:a, "
						"eanzeinzel=(select nvl(SUM(lunit_menge),0) from mae "
						"where mae.maetyp="STR_MAETYP_PK" and parentid=:b), "
						"egewnetto=(select nvl(SUM(lunit_gewnetto),0) from mae "
						"where mae.maetyp="STR_MAETYP_PK" and parentid=:c) "
						"where maetyp="STR_MAETYP_PK" and parentid=:d",
						SQLLONG(mpk[mapcnt-1]),
						SQLSTRING(mapid[mapcnt-1]),
						SQLSTRING(mapid[mapcnt-1]),
						SQLSTRING(mapid[mapcnt-1]),
						NULL);


			if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			}

			if (mgp[mapcnt-1]>1) {
			    dbrv	= TExecSql(tid,
			        "update mam "
			        "set eanzahl=eanzeinzel/:a, "
			        "eanzeinzel="
			        "(select nvl(SUM(lunit_menge),0) from mae "
			        "where (mae.maetyp="STR_MAETYP_TT" or mae.maetyp="STR_MAETYP_GP") "
			        "and parentid=:b), "
			        "egewnetto="
			        "(select nvl(SUM(lunit_gewnetto),0) from mae "
			        "where (mae.maetyp="STR_MAETYP_TT" or mae.maetyp="STR_MAETYP_GP") "
			        "and parentid=:c) "
			        "where (maetyp="STR_MAETYP_TT" or maetyp="STR_MAETYP_GP") "
			        "and parentid=:d",
			        SQLLONG(mgp[mapcnt-1]),
			        SQLSTRING(mapid[mapcnt-1]),
			        SQLSTRING(mapid[mapcnt-1]),
			        SQLSTRING(mapid[mapcnt-1]),
			        NULL);

			    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
			        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
			    }

			}
			break;
		
		} /* switch (which) */
	}
}

/*
 * update die Mengen in den MAMs zur gegebenen recid/rectyp.rekursion ueber die subrecs!
 * recid:	makId oder tournr
 * rectyp:	RECTYP_MAK oder RECTYP_TOUR
 * which:	TDB_UPD_INIT (update bestellte mengen) oder TDB_UPD_ERF (liefermengen)
 * RETURN:	siehe _SetMaStatus()
 */
int TDB_UpdateMam(void *tid,const char *recid,int rectyp,int which,const char *usr)
{
	int rv	= TDB_MASTAT_ERR;
	struct {
		MAM mam_gp;
		MAM mam_pk;
		MAM mam_tt;
	} m;
	struct {
		long l[3];
	} mm[13];
	char 	makid[1000][RECID_LEN+1];
	int		makcnt;
	int dbrv;
	
	if (recid) {
		memset(&m,0,sizeof(m));
		/*
		 * lade mak/tour-MAMs in der richtigen reihenfolge, es muss 3 geben
		 */
		if (TExecSqlX(tid,NULL,"select %MAM from mam where parentid=:a order by maetyp",
				3,0,
				SELSTRUCT(TN_MAM,m),				
				SQLSTRING(recid),
				NULL)==3) {
			switch (rectyp) {
			case RECTYP_MAK:
				switch (which) {
				case TDB_UPD_INIT:
					_update_makmapMAMs_from_MAE(tid,recid,which,usr);
					dbrv = TExecSql(tid,
							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto) from mam,map "
							"where map.parentid=:a and mam.parentid=map.id and mam.maetyp='PK'",
							SELLONG(m.mam_pk.mamAnzahl),
							SELLONG(m.mam_pk.mamAnzEinzel),
							SELLONG(m.mam_pk.mamGewNetto),
							SELLONG(m.mam_pk.mamGewBrutto),
							SQLSTRING(recid),
							NULL);

					if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
					    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
					}

					dbrv = TExecSql(tid,
 							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto) from mam,map "
							"where map.parentid=:a and mam.parentid=map.id and mam.maetyp='GP'",
							SELLONG(m.mam_gp.mamAnzahl),
							SELLONG(m.mam_gp.mamAnzEinzel),
							SELLONG(m.mam_gp.mamGewNetto),
							SELLONG(m.mam_gp.mamGewBrutto),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					dbrv = TExecSql(tid,
							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto) from mam,map "
							"where map.parentid=:a and mam.parentid=map.id and mam.maetyp='TT'",
							SELLONG(m.mam_tt.mamAnzahl),
							SELLONG(m.mam_tt.mamAnzEinzel),
							SELLONG(m.mam_tt.mamGewNetto),
							SELLONG(m.mam_tt.mamGewBrutto),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					break;
				case TDB_UPD_ERF:
					_update_makmapMAMs_from_MAE(tid,recid,which,usr);

					memset(mm,0,sizeof(mm));
					dbrv = TExecSqlX(tid,NULL,
							"select 1,SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto),"
							" 0,0,0,0,0,0,0,0 from mam,map "
							" where map.parentid=:a and mam.parentid=map.id and mam.maetyp='PK'"
							" union"
							" select 2,0,0,0,0,"
							" SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto),"
							" 0,0,0,0 from mam,map "
							" where map.parentid=:b and mam.parentid=map.id and mam.maetyp='GP'"
							" union"
							" select 3,0,0,0,0,0,0,0,0,"
							" SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto)"
							" from mam,map "
							" where map.parentid=:c and mam.parentid=map.id and mam.maetyp='TT'"
							" order by 1",
							3,0,
							SELLONG(mm[0].l[0]),
							SELLONG(mm[1].l[0]),
							SELLONG(mm[2].l[0]),
							SELLONG(mm[3].l[0]),
							SELLONG(mm[4].l[0]),
							SELLONG(mm[5].l[0]),
							SELLONG(mm[6].l[0]),
							SELLONG(mm[7].l[0]),
							SELLONG(mm[8].l[0]),
							SELLONG(mm[9].l[0]),
							SELLONG(mm[10].l[0]),
							SELLONG(mm[11].l[0]),
							SELLONG(mm[12].l[0]),
							SQLSTRING(recid),
							SQLSTRING(recid),
							SQLSTRING(recid),
							NULL);
							
                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					m.mam_pk.mamEAnzahl = mm[1].l[0];					
					m.mam_pk.mamEAnzEinzel = mm[2].l[0];					
					m.mam_pk.mamEGewNetto = mm[3].l[0];					
					m.mam_pk.mamEGewBrutto = mm[4].l[0];					

					m.mam_gp.mamEAnzahl = mm[5].l[1];					
					m.mam_gp.mamEAnzEinzel = mm[6].l[1];					
					m.mam_gp.mamEGewNetto = mm[7].l[1];					
					m.mam_gp.mamEGewBrutto = mm[8].l[1];					

					m.mam_tt.mamEAnzahl = mm[9].l[2];					
					m.mam_tt.mamEAnzEinzel = mm[10].l[2];					
					m.mam_tt.mamEGewNetto = mm[11].l[2];					
					m.mam_tt.mamEGewBrutto = mm[12].l[2];					

					break;
				}
				break;
			case RECTYP_TOUR:
				makcnt	= TExecSqlX(tid,NULL,
							"select id from mak where tournr=:a",
							1000,0,
							SELSTR(makid[0],RECID_LEN+1),
							SQLSTRING(recid),
							NULL);
				for (;makcnt>0;makcnt--) {
					TDB_UpdateMam(tid,makid[makcnt-1],RECTYP_MAK,which,usr);
				}
				switch (which) {
				case TDB_UPD_INIT:
					dbrv = TExecSql(tid,
							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto),COUNT(mak.id) "
							"from mam,mak where mak.tournr=:a and mam.parentid=mak.id and mam.maetyp='PK'",
							SELLONG(m.mam_pk.mamAnzahl),
							SELLONG(m.mam_pk.mamAnzEinzel),
							SELLONG(m.mam_pk.mamGewNetto),
							SELLONG(m.mam_pk.mamGewBrutto),
							SELLONG(m.mam_pk.mamSubpos),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					dbrv = TExecSql(tid,
							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto),COUNT(mak.id) "
							"from mam,mak where mak.tournr=:a and mam.parentid=mak.id and mam.maetyp='TT'",
							SELLONG(m.mam_tt.mamAnzahl),
							SELLONG(m.mam_tt.mamAnzEinzel),
							SELLONG(m.mam_tt.mamGewNetto),
							SELLONG(m.mam_tt.mamGewBrutto),
							SELLONG(m.mam_tt.mamSubpos),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					dbrv = TExecSql(tid,
							"select SUM(mam.anzahl),SUM(mam.anzeinzel),SUM(mam.gewnetto),SUM(mam.gewbrutto),COUNT(mak.id) "
							"from mam,mak where mak.tournr=:a and mam.parentid=mak.id and mam.maetyp='GP'",
							SELLONG(m.mam_gp.mamAnzahl),
							SELLONG(m.mam_gp.mamAnzEinzel),
							SELLONG(m.mam_gp.mamGewNetto),
							SELLONG(m.mam_gp.mamGewBrutto),
							SELLONG(m.mam_gp.mamSubpos),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					break;
				case TDB_UPD_ERF:

					memset(mm,0,sizeof(mm));
					dbrv = TExecSqlX(tid,NULL,
							"select 1,SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto),"
							" 0,0,0,0,0,0,0,0 from mam,mak "
							" where mak.tournr=:a and mam.parentid=mak.id and mam.maetyp='PK'"
							" union"
							" select 2,0,0,0,0,"
							" SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto),"
							" 0,0,0,0 from mam,mak "
							" where mak.tournr=:b and mam.parentid=mak.id and mam.maetyp='GP'"
							" union"
							" select 3,0,0,0,0,0,0,0,0,"
							" SUM(mam.eanzahl),SUM(mam.eanzeinzel),SUM(mam.egewnetto),SUM(mam.egewbrutto)"
							" from mam,mak "
							" where mak.tournr=:c and mam.parentid=mak.id and mam.maetyp='TT'"
							" order by 1",
							3,0,
							SELLONG(mm[0].l[0]),
							SELLONG(mm[1].l[0]),
							SELLONG(mm[2].l[0]),
							SELLONG(mm[3].l[0]),
							SELLONG(mm[4].l[0]),
							SELLONG(mm[5].l[0]),
							SELLONG(mm[6].l[0]),
							SELLONG(mm[7].l[0]),
							SELLONG(mm[8].l[0]),
							SELLONG(mm[9].l[0]),
							SELLONG(mm[10].l[0]),
							SELLONG(mm[11].l[0]),
							SELLONG(mm[12].l[0]),
							SQLSTRING(recid),
							SQLSTRING(recid),
							SQLSTRING(recid),
							NULL);

                    if( dbrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					m.mam_pk.mamEAnzahl = mm[1].l[0];					
					m.mam_pk.mamEAnzEinzel = mm[2].l[0];					
					m.mam_pk.mamEGewNetto = mm[3].l[0];					
					m.mam_pk.mamEGewBrutto = mm[4].l[0];					

					m.mam_gp.mamEAnzahl = mm[5].l[1];					
					m.mam_gp.mamEAnzEinzel = mm[6].l[1];					
					m.mam_gp.mamEGewNetto = mm[7].l[1];					
					m.mam_gp.mamEGewBrutto = mm[8].l[1];					

					m.mam_tt.mamEAnzahl = mm[9].l[2];					
					m.mam_tt.mamEAnzEinzel = mm[10].l[2];					
					m.mam_tt.mamEGewNetto = mm[11].l[2];					
					m.mam_tt.mamEGewBrutto = mm[12].l[2];					

					break;
				}
				break;
			}
			if (usr) {
				SetHist(TN_MAM, &m.mam_pk, HIST_UPDATE, usr);
				SetHist(TN_MAM, &m.mam_tt, HIST_UPDATE, usr);
				SetHist(TN_MAM, &m.mam_gp, HIST_UPDATE, usr);
			}
			rv	= TExecStdSql(tid,StdNupdate,TN_MAM,&m.mam_pk)>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
			rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&m.mam_tt)>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
			rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&m.mam_gp)>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
		}
	}
	return rv;
}

static int _getNewTourNr(void *tid, MAK *mak)
{
	char akttour[REFNR_LEN+1];
	char current_tour[REFNR_LEN+1];
	int lfnr,rv,dbrv;
	MAM mam;
	int maxcnt=AppContextGetDataNumDef("HOST_MaxTourNrTry",1);
	memset(akttour,0,REFNR_LEN+1);
	memset(current_tour,0,REFNR_LEN+1);
	
	/* Save the tournr */
	StrCpy(akttour,mak->makTourNr);
	
/*	StrCpy(current_tour,akttour);*/
	lfnr=0;
	rv=TRUE;
	fprintf(stderr,"\nCheck TourNR '%s'\n",akttour);
	while ((rv==TRUE) && (lfnr<=maxcnt)) {
	
		if (lfnr==0) {
			sprintf(current_tour,"%s",akttour);
		} else {
			sprintf(current_tour,"%s%c",akttour,'_');
		}
		
		dbrv=TExecSql(tid,"select %MAM from MAM where "
					  "rectyp='TOUR' and parentid=:a and maetyp='PK'",
					  SELSTRUCT(TN_MAM,mam),
					  SQLSTRING(current_tour),
					  NULL);
		if (dbrv>0) {
			/* Check if tour status is not free */
			if ((mam.mamStatus<=MASTAT_FREIGABE) &&
				(mam.mamPreStatus<=MASTAT_FREIGABE)) {
				
				fprintf(stderr," Existing Tour '%s', status=%ld,prestatus=%ld\n",
						current_tour, mam.mamStatus,mam.mamPreStatus);
				rv=FALSE; /* break */
			}
		} else
			rv=FALSE; /* break */
			
		lfnr++;
	}
	/* Copy the tournr back */
	if (strcmp(akttour,current_tour)) {
		StrCpy(mak->makTourNr,current_tour);
		rv=1;
	} else
		rv=0;
	fprintf(stderr," New Tour '%s', rv=%d\n",
			mak->makTourNr, rv);
	
	return rv;	
}



/*
 * Parameter:
 *	tid		transaktions id fuer db-calls
 *	recid	record id des ma eintrags
 *	rec		zeiger auf entsprechendes record (kann auch NULL sein)
 *	status	der zu setzende status
 *	rectyp	der mit recid korrespondierende recordtyp. dieser parameter
 *			hat prioritaet! ggf. wird versucht, ueber die nicht typkonforme
 *			recid den entsprechende record zu ermitteln (z.b.: map->mak)
 *	user	user fuer log (optional)
 * Ergebnis:
 *	Ok:				TDB_MASTAT_OK		(0)
 *	allg. Fehler:	TDB_MASTAT_ERR		(-1)
 *  inputfehler:	TDB_MASTAT_NODATA	(-2)
 *  kontextfehler:	TDB_MASTAT_IMP		(-3)
 */
int TDB_SetMaStatus(const std::string & fac, void *tid, char *recid, void *record, int status, int rectyp, const char *usr, std::string *error_str)
{
	int 	rv	= TDB_MASTAT_NODATA;
	void	*rec	= NULL;
	MAK 	*mak,*tmpmak;
	MAP 	*map;
	MAM		*pmam;
	MAK		makrec;
	int		dbrv=FALSE,upd=FALSE;
	int		freerec	= FALSE;	/* indicates rec should be freed ... */
	LStick	l,l2,maklist,reclist	= NULL;
	KAK		kak;
	char	oldkakid[RECID_LEN+1];
	static char	sdummy[1024];
	
	if (!recid&&!record) {							
		return TDB_MASTAT_NODATA;		/* don't know how to handle */
	}


	if (rectyp!=RECTYP_TOUR) {

		if (!record) {									/* load & lock record for an update */
			rv	= _lock_rec(tid,recid,rectyp,TRUE,&rec,NULL);
			if (rv!=TDB_MASTAT_OK||!rec) {
				LogPrintf(fac, LT_ALERT, "_lock_rec fehlgeschlagen");
				return rv;
			}	
			freerec	= TRUE;
		} else {
			rec	= record;
			freerec	= FALSE;
		}
	} 		
	rv	= TDB_MASTAT_IMP;

	switch (rectyp) {

	case  RECTYP_MAP:
		map	= (MAP *)rec;
		memset(&makrec,0,sizeof(MAK));

		/*
		 * hole zugehoerigen mak
		 */

		if (TExecSql(tid,"select %MAK from mak where id=:a for update nowait",
					SELSTRUCT(TN_MAK,makrec),
					SQLSTRING(map->mapParentId),
					NULL)<=0) {
			rv	= TDB_MASTAT_ERR;
			break;
		}
		switch (status) {
		case  MASTAT_NEU:
			if (map->mapStatus<=MASTAT_NEU) {
				switch (makrec.makBwArt) {
				case BWART_ZPR:
				case BWART_ZLI:
				case BWART_ZKU:
				case BWART_ZSP:
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:

					/*
					 * errechne und initialisiere Gewichtdaten im MAP
					 */
					_calc_mapgew(tid,map);
					
					/*
					 * loesche zuerst alle bereits exisierenden mams -> mehrfach NEU-setzen moeglich
					 */
					TDB_DeleteMam(tid,recid,RECTYP_MAP);

					/*
					 * lege die MAP-MAMs an
					 */
					rv	= TDB_CreateMam(fac, tid, NULL, map, TDB_CREMAM_MAMMAP, usr);

					/*
					 * setze status und fertig...
					 */
					map->mapStatus	= MASTAT_NEU;
					upd	= TRUE;
					if( rv < 0 ) {
						LogPrintf(fac,LT_NOTIFY,"TDB_CreateMam fehlgeschlagen: %d", rv );
					}	
					break;
				}
			}
			break;
		case  MASTAT_FREIGABE:
			if (map->mapStatus<=MASTAT_FREIGABE) {
				StrCpy(map->mapLUnit.unitArtNr,map->mapBUnit.unitArtNr);/* for sure */
				switch (makrec.makBwArt) {
				case BWART_ZPR:
				case BWART_ZLI:
				case BWART_ZKU:
				case BWART_ZSP:
					rv	= TDB_CreateMam(fac, tid, NULL, map, TDB_CREMAM_MAE, usr);
					if( rv < 0 ) {
						LogPrintf(fac,LT_NOTIFY,"TDB_CreateMam fehlgeschlagen: %d", rv );
					}	
					if (rv==TDB_MASTAT_OK&&makrec.makBwArt==BWART_ZPR) {
					  rv	= TDB_SetMaStatus(fac,tid,NULL,&makrec,MASTAT_FREIGABE,RECTYP_MAK,usr,error_str);
						if( rv < 0 ) {
							LogPrintf(fac,LT_NOTIFY,"TDB_SetMaStatus fehlgeschlagen: %d", rv );
						}	
					}  
					map->mapStatus	= MASTAT_FREIGABE;
					upd	= TRUE;
					break;
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:
					if (map->mapStatus<=MASTAT_FREIGABE) {
						/*
						 * erzeuge MAEs fuer gegebenen MAP
						 */

						rv	= TDB_CreateMam(fac, tid, NULL, map, TDB_CREMAM_MAE, usr);

						if( rv != TDB_MASTAT_OK )
						  {
							LogPrintf( fac, LT_ALERT, "Fehler beim Anlegen der MAM Einträge für MAP %s MaNr %s Artikel %s",
									   map->mapId, map->mapMaNr, map->mapBUnit.unitArtNr );
							LogPrintf( FAC_LIB, LT_ALERT, "Fehler beim Anlegen der MAM Einträge für MAP %s MaNr %s Artikel %s",
									   map->mapId, map->mapMaNr, map->mapBUnit.unitArtNr );
						  }
						/*
						 * setze zugehoerigen MAK auf NEUF
						 */
  
						if (rv==TDB_MASTAT_OK&&makrec.makStatus==MASTAT_NEU) {
						  rv	= TDB_SetMaStatus(fac,tid,NULL,&makrec,MASTAT_NEUF,RECTYP_MAK,usr,error_str);
						}
						if (rv!=TDB_MASTAT_OK) break;	/* ... und tschuess */
						
						/*
						 * wenn es fuer diesen map noch keinen kak gibt, hole diesen, sonst erzeuge
						 * einen und trage ihn in MAP ein
						 */
						 
						if (!_find_kak(tid,&makrec,map,&kak)) {
							dbrv	= _mak2kak(tid,&kak,&makrec,usr)>0?1:-1;
							if (map->mapKom[0])
								StrCpy(kak.kakKomissionierer,map->mapKom);
					    	dbrv	= (dbrv>=0&&TExecStdSql(tid, StdNinsert, TN_KAK, &kak)>=0)?1:-1;
						} else dbrv	= 1;
						if (dbrv>=0) {
							StrCpy(map->mapKakId,kak.kakId);
						}
						rv	= (dbrv>=0&&_split_vermaps(tid,&makrec,map,usr)>=0)?TDB_MASTAT_OK:TDB_MASTAT_ERR;
						if (rv!=TDB_MASTAT_OK) break;	/* ... und tschuess */
					} else rv	= TDB_MASTAT_OK;
					map->mapStatus	= MASTAT_FREIGABE;
					map->mapMaFlags	= MAFLAG_O;
					upd	= TRUE;
					break;
				}
			}
			break;
		case  MASTAT_FERTIG:
 			if (map->mapStatus<=MASTAT_FERTIG) {
				switch (makrec.makBwArt) {
				case BWART_ZPR:
				  {
					int xrv = TExecSql(tid,"delete from lab where tep_mapidprod=:a"
							" or not exists (select id from map where id=tep_mapidprod)",
								SQLSTRING(map->mapId),NULL);

                    if( xrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					if (TExecSql(tid,"select id from map where parentid=:a and status<>:b and postyp="
								STR_MATYP_STD
								" and id<>:c",
								SELSTR(sdummy,RECID_LEN+1),
								SQLSTRING(map->mapParentId),
								SQLSTRING(l2sGetNameByValue(&l2s_MASTAT, MASTAT_FERTIG)),
								SQLSTRING(map->mapId),
								NULL)<0) {
						if (TSqlError(tid)==SqlNotFound) {
							/* Update von diesem MAP nicht vergessen */
							if (usr)
								SetHist(TN_MAP, map, HIST_UPDATE, usr);
							map->mapStatus	= MASTAT_FERTIG;
							dbrv=TExecStdSql(tid,StdNupdate,TN_MAP,map);
							if (dbrv<0)
								rv	= TDB_MASTAT_ERR;
							else
							  rv	= TDB_SetMaStatus(fac,tid,NULL,&makrec,MASTAT_FERTIG,RECTYP_MAK,usr,error_str);
						}
					} else {
						rv	= TDB_MASTAT_OK;
					}
					map->mapStatus	= MASTAT_FERTIG;
					dbrv	= TRUE;
						upd	= TRUE;
				  }
				  break;
				case BWART_ZLI:
				case BWART_ZKU:
				case BWART_ZSP:
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:
				  {


					/*
					 * setze MAEs auf fertig
					 */
					 
					int xrv = TExecSql(tid,
						"update mae set status="STR_MASTAT_FERTIG
						" where parentid=:a",
						SQLSTRING(map->mapId),
						NULL);

                    if( xrv <= 0 && TSqlError(tid) != SqlNotFound ) {
                        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
                    }

					/*
					 * setze KAKs auf fertig
					 */
					 
					xrv = TExecSql(tid,
						"update kak set status="STR_MASTAT_FERTIG
						" where id=:a",
						SQLSTRING(map->mapKakId),
						NULL);

					if( xrv <= 0 && TSqlError(tid) != SqlNotFound ) {
					    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
					}
					
					map->mapStatus	= MASTAT_FERTIG;
					upd	= TRUE;
					rv	= TDB_MASTAT_OK;
				  }
				  break;
				}
			} else
				rv	= TDB_MASTAT_OK;
			break;
		}
		if (upd) {
			if (dbrv>=0&&rv==TDB_MASTAT_OK) {
				if (usr)
					SetHist(TN_MAP, map, HIST_UPDATE, usr);
				if (TExecStdSql(tid,StdNupdate,TN_MAP,map)<0)
					rv	= TDB_MASTAT_ERR;
			}
		}
		break;

	case  RECTYP_MAK:
		mak	= (MAK *)rec;
		switch (status) {
		case  MASTAT_NEU:
			if (mak->makStatus<=MASTAT_NEU) {
				LogPrintf(fac, LT_ALERT, "HIER");
				switch (mak->makBwArt) {
				case BWART_ZPR:
				case BWART_ZLI:
				case BWART_ZKU:
				case BWART_ZSP:
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:

					/*
					 * do SetMaStatus(NEU) for all MAPs
					 */
					reclist	= _load_rec_list(tid, RECTYP_MAP, TRUE, mak->makId, "map.postyp='STD'");
					for (l=reclist;l&&l->data;l=l->next) {
					  rv	= TDB_SetMaStatus(fac,tid, NULL, l->data, MASTAT_NEU, RECTYP_MAP, usr,error_str);
						if (rv!=TDB_MASTAT_OK){
							LogPrintf(fac, LT_ALERT, "TDB_SetMaStatus fehlgeschlagen %d", rv);
							break;
						}
					}
					reclist	= _free_rec_list(reclist);
					if (rv!=TDB_MASTAT_OK) {
						LogPrintf(fac, LT_ALERT, "HIER");
						break;							/* error in any sub, bye */
						}
					if (mak->makBwArt!=BWART_ZPR) {
						rv	= TDB_CreateMam(fac, tid, NULL, mak, TDB_CREMAM_MAMMAK, usr);
						LogPrintf(fac, LT_ALERT, "Rv = %d", rv);	
					}
					if (rv==TDB_MASTAT_OK&&
						(mak->makBwArt==BWART_AKU||mak->makBwArt==BWART_ALI||mak->makBwArt==BWART_ASP)) {

						if (_getNewTourNr(tid, mak)==TRUE) {
							if (TExecStdSql(tid,StdNupdate,TN_MAK,mak)<0) {
								rv	= TDB_MASTAT_ERR;
								LogPrintf(fac, LT_ALERT, "HIER %ld", rv);
								break;
							}
						}
		
						rv	= TDB_CreateMam(fac, tid, NULL, mak, TDB_CREMAM_MAMTOUR, usr);	
						LogPrintf(fac, LT_ALERT, "HIER %ld", rv);
						mak->makStatus	= MASTAT_NEU;
						upd	= TRUE;
#if 1

						/*
						 * wenn tour bereits freigegeben, dann Auftrag
						 * sofort mit tour-tornr/Kommzeit freigeben.
						 */
						 
 						reclist	= _load_rec_list(tid, RECTYP_TOUR, TRUE, mak->makTourNr, NULL);
						if (reclist) {
							pmam	= (MAM *)reclist->data;
							if (pmam->mamStatus	== MASTAT_FREIGABE) {
								mak->makTorNr	= pmam->mamTorNr;
								mak->makKomZeit	= pmam->mamKomZeit;
								rv	= TDB_SetMaStatus(fac,tid, NULL, mak,
											MASTAT_FREIGABE, RECTYP_MAK, usr,error_str);
								LogPrintf(fac, LT_ALERT, "HIER %ld", rv);
								upd	= FALSE;
							}
						
						}	
						reclist	= _free_rec_list(reclist);


#endif
					} else {
						mak->makStatus	= MASTAT_NEU;
						upd	= TRUE;
					}
					dbrv	= TRUE;
					break;
				}
				break;
			}
		case  MASTAT_NEUF:
			if (mak->makStatus<=MASTAT_NEUF) {
				switch (mak->makBwArt) {
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:
				  rv	= TDB_SetMaStatus(fac,tid, mak->makTourNr, NULL, MASTAT_NEUF, RECTYP_TOUR, usr,error_str);
					mak->makStatus	= MASTAT_NEUF;
					upd	= TRUE;
					break;
				}
			}
			break;
		case  MASTAT_FREIGABE:
			if (mak->makStatus<=MASTAT_FREIGABE) {
				switch (mak->makBwArt) {
				case BWART_ZPR:
					mak->makStatus	= MASTAT_FREIGABE;
					upd	= TRUE;
					rv	= TDB_MASTAT_OK;
					break;
				case BWART_ZLI:
				case BWART_ZKU:
				case BWART_ZSP:
				case BWART_AKU:
				case BWART_ALI:
				case BWART_ASP:

					if (mak->makBatchNr&&mak->makMaTyp!=ARTTYP_STD) {
						rv	= TDB_MASTAT_OK;
						upd	= FALSE;
						break;
					}
					
					/*
					 * schreibe mak um ggf. die neuen daten (tor/komzeit)
					 * fuer die neuen kaks zugaenglich zu machen. der mak muss dazu aber
					 * als record uebergeben worden sein!
					 */

					if (usr) SetHist(TN_MAK, mak, HIST_UPDATE, usr);
					if (TExecStdSql(tid,StdNupdate,TN_MAK,mak)<0) {
						rv	= TDB_MASTAT_ERR;
						break;
					}
					
					/*
					 * do SetMaStatus(FREIGABE) for all MAPs
					 */

					rv	= TDB_MASTAT_OK;
					memset(oldkakid,0,sizeof(oldkakid));
					memset(&kak,0,sizeof(KAK));
					
					reclist	= _load_rec_list(tid, RECTYP_MAP, TRUE, mak->makId,
								"map.postyp="STR_ARTTYP_STD" and map.status="STR_MASTAT_NEU);
					for (l=reclist;l&&l->data;l=l->next) {
						((MAP *)l->data)->mapKomZeit	= mak->makKomZeit;
						rv	= TDB_SetMaStatus(fac,tid, NULL, l->data, MASTAT_FREIGABE, RECTYP_MAP, usr,error_str);
						if (rv!=TDB_MASTAT_OK) break;

						StrCpy(oldkakid,((MAP *)l->data)->mapKakId);
						if (strcmp(oldkakid,kak.kakId) && kak.kakId[0]!='\0') {
							if ((mak->makBwArt==BWART_AKU||mak->makBwArt==BWART_ALI||mak->makBwArt==BWART_ASP)
								&& mak->makBatchNr==0) {
								rv	= TDB_CreateMam(fac, tid, NULL, &kak, TDB_CREMAM_MAMKAK, usr);
								StrCpy(kak.kakId,oldkakid);
							}
						}
						else {
							StrCpy(kak.kakId,oldkakid);
						} 

					}

					if (rv==TDB_MASTAT_OK && (mak->makBwArt==BWART_AKU||mak->makBwArt==BWART_ALI||mak->makBwArt==BWART_ASP)
						&& mak->makBatchNr==0)
						rv	= TDB_CreateMam(fac, tid, NULL, &kak, TDB_CREMAM_MAMKAK, usr);

					reclist	= _free_rec_list(reclist);
					if (rv!=TDB_MASTAT_OK) break;

					if ((mak->makBwArt==BWART_AKU||mak->makBwArt==BWART_ALI||mak->makBwArt==BWART_ASP)&&
						rv==TDB_MASTAT_OK&&mak->makStatus<MASTAT_FREIGABE) {

						/*
						 * ist mak teil eines batchs, dann alle anderen maks mit dieser batchnr freigeben
						 */

						if (mak->makBatchNr) {
							
							/*
							 * hole alle maks der tour mit derselben batchnr (ausser mak selbst)
							 */

							sprintf(sdummy,"mak.batchnr=%ld and mak.id<>'%s' and mak.status<>"STR_MASTAT_FREIGABE,mak->makBatchNr,mak->makId);
							maklist	= _load_rec_list(tid, RECTYP_MAK, TRUE, mak->makTourNr, sdummy);
							
							/*
							 * setze jeden mak 'haendisch' auf FREIGABE (sonst endlosrekursion!)
							 */

							memset(&kak,0,sizeof(KAK));

							for (l=maklist;l&&l->data&&rv==TDB_MASTAT_OK;l=l->next) {
								tmpmak	= (MAK *)l->data;
								tmpmak->makKomZeit	= mak->makKomZeit;
								tmpmak->makTorNr	= mak->makTorNr;

								if (tmpmak->makMaTyp==ARTTYP_STD) {

									/*
									 * setze alle maps des aktuelle maks auf FREIGABE
									 */
									 
									reclist	= _load_rec_list(tid, RECTYP_MAP, TRUE, tmpmak->makId, "map.postyp='STD'");
									for (l2=reclist;l2&&l2->data&&rv==TDB_MASTAT_OK;l2=l2->next) {
										map	= (MAP *)l2->data;
										map->mapKomZeit	= mak->makKomZeit;
										rv	= TDB_SetMaStatus(fac,tid, NULL, l2->data, MASTAT_FREIGABE, RECTYP_MAP, usr,error_str);

										StrCpy(kak.kakId,((MAP *)l2->data)->mapKakId);
									}

									reclist	= _free_rec_list(reclist);

								} else {
								}
								
								/*
								 * setze aktuellen mak auf FREIGABE und update ...
								 */

								tmpmak->makStatus	= MASTAT_FREIGABE;
								if (usr)
									SetHist(TN_MAK, tmpmak, HIST_UPDATE, usr);
								if (rv==TDB_MASTAT_OK)
									rv	= TExecStdSql(tid,StdNupdate,TN_MAK,tmpmak)>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;
							}

							if (kak.kakId[0]!='\0')
								rv	= TDB_CreateMam(fac, tid, NULL, &kak, TDB_CREMAM_MAMKAK, usr);

							maklist	= _free_rec_list(maklist);
						}

						/*
						 * update kak batchanz - kak muss hier bereits existieren!
						 */

						if (rv==TDB_MASTAT_OK) {
							if (mak->makBatchNr) {
								sprintf(sdummy,"update kak set batchanz="
									"(select COUNT(id) from mak where tournr='%s' and batchnr=%ld and matyp="STR_ARTTYP_STD") "
									"where id=(select distinct kakid from map where parentid='%s' and postyp=%s)",
									mak->makTourNr,mak->makBatchNr,mak->makId,STR_ARTTYP_STD);
								rv 	= TExecSql(tid,sdummy,NULL)>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;
							}
						}	
					}  
					mak->makStatus	= MASTAT_FREIGABE;
					upd		= TRUE;
					break;
				}
			}
			break;
		case  MASTAT_FERTIG:
			switch (mak->makBwArt) {
			case BWART_ZPR: /* sis: Produktion */
			case BWART_ZLI: /* sis: Lieferung extern */
			case BWART_ZSP:
			case BWART_ZKU:
				if (mak->makStatus<MASTAT_FERTIG) {
					
					/*
					 * init hostcomm
					 */ 
					/* sis: Bei Zugängen Rückmeldung wie bisher !! */
				  std::vector<MAP> vMaps;

                    reclist = _load_rec_list(tid, RECTYP_MAP, FALSE, mak->makId, NULL);
                    for (l=reclist;l&&l->data;l=l->next) {
					  
						  if( mak->makBwArt == BWART_ZPR )
							{
							  try {
								rv=Sap::WriteWeMeldungFertig(fac,tid, *mak, *((MAP *)l->data), l->next == NULL );
							  } catch( const ReportException & err ) {
							  	 LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
								 if( error_str ) {
									*error_str += err.simple_what();
								 }
								 return (TDB_MASTAT_ERR);
							  } catch( std::exception & err ) {
								LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
								if( error_str ) {
									*error_str += err.what();
								}
								return (TDB_MASTAT_ERR);			
							  }
							  
							  if( !rv )
								{
								  LogPrintf( fac, LT_ALERT, 
											 "Fehler beim Schreiben der WE Meldung für Auftrag: "
											 "Auftrag: %s MakId: %s",
											 mak->makRefNr, mak->makId );
								  return (TDB_MASTAT_ERR);
								}
							} 
						  else 
							{
							  vMaps.push_back( *((MAP *)l->data) );
							}
                    }
                    reclist = _free_rec_list(reclist);

					if( UseSap() && mak->makBwArt != BWART_ZPR )
					  {
						try {
						  rv=Sap::WriteWeMeldungFertigZLI(fac,tid, *mak, vMaps );
						 } catch( const ReportException & err ) {
						  	 LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
							 if( error_str ) {
								*error_str += err.simple_what();
							 }
							 return (TDB_MASTAT_ERR);
						} catch( std::exception & err ) {
						  LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
						  if( error_str ) {
								*error_str += err.what();
						  }
						  return (TDB_MASTAT_ERR);			
						}
						
						if( !rv )
						  {
							LogPrintf( fac, LT_ALERT, 
									   "Fehler beim Schreiben der WE Meldung für Auftrag: "
									   "Auftrag: %s MakId: %s",
									   mak->makRefNr, mak->makId );
							return (TDB_MASTAT_ERR);
						  }
					  }
					
					/*
					 * aufraeumen: MAMs und MAEs fuer auftrag loeschen
					 */
					TDB_DeleteMam(tid,mak->makId,RECTYP_MAK);
					TDB_DeleteMae(tid,mak->makId,RECTYP_MAK);

					mak->makStatus	= status;
					upd	= TRUE;
					if (dbrv>=0) {
						rv	= TDB_MASTAT_OK;
					} else
						rv	= TDB_MASTAT_ERR;
				}
				break;
			case BWART_AKU:
			case BWART_ALI:
			case BWART_ASP:

				if (mak->makStatus<=MASTAT_FREIGABE) {
					
					/*
					 * init hostcomm
					 */
					 
					rv=TDB_MASTAT_OK;
					reclist	= _load_rec_list(tid, RECTYP_MAP, FALSE, mak->makId,
								"(map.postyp="STR_ARTTYP_STD" or map.splitnr=0)");

					LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"MAP-listload: %s %s %s",
					   	mak->makRefNr,
						mak->makId,
						reclist?"found":"not found");
			
					/* Status von allen MAP's updaten !! */
					for (l=reclist;l&&l->data;l=l->next) 
					{
					  rv	= TDB_SetMaStatus(fac,tid, NULL, l->data, MASTAT_FERTIG, RECTYP_MAP, usr,error_str);
						if (rv!=TDB_MASTAT_OK)
							break;
					}
					reclist	= _free_rec_list(reclist);
					
					try {
					    rv=Sap::WriteVerpMeldung(fac,tid, *mak );
					} catch( const ReportException & err ) {
					    LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
					    if( error_str ) {
					        *error_str += err.simple_what();
					    }
					    return (TDB_MASTAT_ERR);
					} catch( std::exception & err ) {
					    LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
					    if( error_str ) {
					        *error_str += err.what();
					    }
					    return (TDB_MASTAT_ERR);
					}

					if( !rv )
					  {
					    LogPrintf( fac, LT_ALERT,
					        "Fehler beim Schreiben der Verpackungsmeldung für Tour: "
					        "%s, Auftrag: %s MakId: %s Es wurden keine Datengefunden, dies muß nicht unbedingt ein Fehler sein, mache weiter.",
					        mak->makTourNr, mak->makRefNr, mak->makId );
					    rv  = TDB_MASTAT_OK;
					  }
					

 					mak->makStatus	= status;
					upd	= TRUE;
				} else
					rv=TDB_MASTAT_OK;
				break;
			}
			break;
		}

 		if (upd) {
			if (dbrv>=0&&rv==TDB_MASTAT_OK) {
				if (usr)
					SetHist(TN_MAK, mak, HIST_UPDATE, usr);

					if (mak->makStatus==MASTAT_FERTIG&&mak->makBwArt!=BWART_AKU&&
						mak->makBwArt!=BWART_ALI&&mak->makBwArt!=BWART_ASP) {
						/* MAK Update ! */
						if (TExecStdSql(tid,StdNupdate,TN_MAK,mak)<0) {
							rv	= TDB_MASTAT_ERR;
							LogPrintf(fac,LT_NOTIFY, "Update MAK fehlgeschlagen rv: %s", TSqlErrTxt(tid));
						} else {
						  rv = TDB_protMakMap(fac,tid,mak->makId,usr)==DB_UTIL_SQLFEHLER
								?TDB_MASTAT_ERR:TDB_MASTAT_OK;
						  if( rv != TDB_MASTAT_OK ) {
								 LogPrintf(fac,LT_NOTIFY, "TDB_protMakMap fehlgeschlagen rv: %d", rv);
						  }
						}		
							
					} else {
						if (TExecStdSql(tid,StdNupdate,TN_MAK,mak)<0) {
							rv	= TDB_MASTAT_ERR;
							LogPrintf(fac,LT_NOTIFY,"TDB_MASTAT_ERR" );
						}

						if (mak->makStatus == MASTAT_FREIGABE) {
							int xrv = TExecSql(tid,"UPDATE MAM SET FreigabeZeit=SYSDATE,"
										 " KomZeit=:a, LiefZeit=:b"
										 " WHERE RecTyp="STR_RECTYP_MAK
										 " AND ParentId=:c",
										 SQLTIMET(mak->makKomZeit),
										 SQLTIMET(mak->makLiefZeit),
										 SQLSTR(mak->makId, RECID_LEN+1),
										 NULL);

							if( xrv <= 0 && TSqlError(tid) != SqlNotFound ) {
							    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
							}
						}

						/*
						 * setze tour auf NEUF
						 */

						if (rv==TDB_MASTAT_OK && mak->makStatus==MASTAT_FREIGABE &&
							(mak->makBwArt==BWART_AKU||mak->makBwArt==BWART_ALI||mak->makBwArt==BWART_ASP)) {

						  rv	= TDB_SetMaStatus(fac,tid, mak->makTourNr, NULL, MASTAT_NEUF, RECTYP_TOUR, usr,error_str);
						  if( rv < 0 ) {
						  	LogPrintf(fac,LT_NOTIFY, "TDB_SetMaStatus fehlgeschlagen Tour %s rv: %d",  mak->makTourNr, rv );
						  }	
						}
					}
			}
		}
		LogPrintf(fac,LT_NOTIFY,
				  	"MAK-Status gesetzt: %s %s %s (bwart:%s upd:%d, rv:%d, usr:%s)",
				   	mak->makRefNr,
					mak->makId,
					l2sGetNameByValue(&l2s_MASTAT,status),
					l2sGetNameByValue(&l2s_BWART,mak->makBwArt),
					upd,
					rv,
					usr);

		LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"MAK-Status gesetzt: %s %s %s (bwart:%s upd:%d, rv:%d, usr:%s)",
				   	mak->makRefNr,
					mak->makId,
					l2sGetNameByValue(&l2s_MASTAT,status),
					l2sGetNameByValue(&l2s_BWART,mak->makBwArt),
					upd,
					rv,
					usr);

		break;

	case  RECTYP_TOUR:
	
		/*
		 * lade die tourmams-muessen hier bereits existieren.
		 */
		reclist	= _load_rec_list(tid, RECTYP_TOUR, TRUE, recid, NULL);
		if (reclist) {
			pmam	= (MAM *)reclist->data;
			switch (status) {
			case MASTAT_FREIGABE:

				/*
				 * lade alle maks der tour, welche noch nicht freigegeben sind
		 		 */
				maklist	= _load_rec_list(tid, RECTYP_MAK, TRUE, recid, "mak.status<>"STR_MASTAT_FREIGABE);
				rv=TDB_MASTAT_OK;

				/*
				 * setze alle noch nicht freigegebenen maks auf FREIGABE
				 */
				for (l=maklist;l&&l->data;l=l->next) {
					mak	= (MAK *)l->data;
					mak->makTorNr	= pmam->mamTorNr;
					mak->makKomZeit	= pmam->mamKomZeit;
					rv	= TDB_SetMaStatus(fac,tid, NULL, mak, status, RECTYP_MAK, usr,error_str);
 					if (rv!=TDB_MASTAT_OK)
 						break;
				}
				maklist	= _free_rec_list(maklist);
				if (rv==TDB_MASTAT_OK)
					upd	= TRUE;			
				break;
			case MASTAT_NEUF:
				maklist	= _load_rec_list(tid, RECTYP_MAK, FALSE, recid, "mak.status<>"STR_MASTAT_FREIGABE);
				if (!maklist)
					status=MASTAT_FREIGABE;
				maklist	= _free_rec_list(maklist);					
				upd	= TRUE;			
				rv	= TDB_MASTAT_OK;
				break;
			case MASTAT_FERTIG:

				/*
				 * lade alle maks der tour
		 		 */
				maklist	= _load_rec_list(tid, RECTYP_MAK, TRUE, recid, NULL);
				rv=TDB_MASTAT_OK;

				/*
				 * setze alle maks auf FERTIG
				 */

				rv	= TDB_MASTAT_OK;
				for (l=maklist;l&&l->data&&rv==TDB_MASTAT_OK;l=l->next) {

					mak	= (MAK *)l->data;
					rv	= TDB_SetMaStatus(fac,tid, NULL, mak, status, RECTYP_MAK, usr,error_str);
 					if (rv!=TDB_MASTAT_OK)
 						break;
 						


					/*
					 * aufraeumen: MAMs und MAEs fuer auftrag loeschen
					 */
					 
					TDB_DeleteMam(tid,mak->makId,RECTYP_MAK);
					TDB_DeleteMae(tid,mak->makId,RECTYP_MAK);

					rv	= rv==TDB_MASTAT_OK
					  ?TDB_DeleteTek(fac,tid,mak->makId,usr)
							:rv;

					if (rv==TDB_MASTAT_OK)
					  rv = TDB_protMakMap(fac,tid,mak->makId,usr)==DB_UTIL_SQLFEHLER
							?TDB_MASTAT_ERR:TDB_MASTAT_OK;


				}
				maklist	= _free_rec_list(maklist);

				
				if (rv==TDB_MASTAT_OK)
					/*
					 * entferne KAKs
					 */
/*	MAPs gibt's nicht mehr ...
					TExecSql(tid,"update map set kakid=NULL where parentid in "
							"(select id from mak where tournr=:a)",
							SQLSTRING(recid),
							NULL); 
*/
                    /* sis: Vorher noch Tourenprotokoll updaten !! */
                    WriteTourProtAll(tid,recid,0);

					int xrv = TExecSql(tid,"delete from kak where tournr=:a",
						SQLSTRING(recid),
						NULL);

				    if( xrv <= 0 && TSqlError(tid) != SqlNotFound ) {
				        LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));
				    }

					upd	= TRUE;			
				break;
			}
		} 
		if (upd) {
			if (rv==TDB_MASTAT_OK) {
				/*
				 * setze alle tour-mams auf den neuen status
				 */
				for (l=reclist;l&&l->data;l=l->next) {
					pmam	= (MAM *)l->data;
					pmam->mamStatus	= status;
					if (status==MASTAT_FREIGABE || status==MASTAT_NEUF) {
						pmam->mamFreigabeZeit=time(NULL);
					}					
					if (usr)
						SetHist(TN_MAM, (MAM *)l->data, HIST_UPDATE, usr);
					if (status==MASTAT_FERTIG) {
						if (TExecStdSql(tid,StdNdelete,TN_MAM,(MAM *)l->data)<0) {
							rv	= TDB_MASTAT_ERR;
							break;
						}
					} else {
						if (TExecStdSql(tid,StdNupdate,TN_MAM,(MAM *)l->data)<0) {
							rv	= TDB_MASTAT_ERR;
							break;
						}
					}
				}
			}
		}
		_free_rec_list(reclist);
		break;

	case  RECTYP_MAE:
		break;

	case  RECTYP_KAK:
		break;
	}
	if (freerec) 
		MemDeallocate(rec);
	return rv;
}

#define UPDBUF 4096

int TDB_UpdateTourMAM(void *tid, char *tournr, MAM *mambuf)
{
	int 	rv	= TDB_MASTAT_ERR;
	char 	mapid[UPDBUF][RECID_LEN+1];
	char 	maetyp[UPDBUF][STRVALUELEN_MAETYP+1];
	int		mapcnt,pkcnt;
	long	mpk[UPDBUF],mgp[UPDBUF];
	long	smpk[UPDBUF],sgpk[UPDBUF];
	long	smgp[UPDBUF],sggp[UPDBUF];
	MAM		mam[3];
	
	if (tournr) {

		/*
		 * hole LE2/Pick bzw. gp Mengen aus zugehoerigen eans
	 	 */

		memset(mpk,0,sizeof(mpk));	 
		memset(smpk,0,sizeof(smpk));	 
		memset(sgpk,0,sizeof(sgpk));	 
		memset(mapid,0,sizeof(mapid));	 

		pkcnt	= TExecSqlX(tid,NULL,
					"select map.id,ean.unit_menge,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from ean,map,mak,mae"
					" where mak.tournr=:a"
					" and mak.bwart="STR_BWART_AKU
					" and mak.status="STR_MASTAT_FREIGABE
					" and map.parentid=mak.id"
					" and map.postyp="STR_ARTTYP_STD
					" and ean.unit_artnr=map.bunit_artnr"
					" and ean.maetyp="STR_MAETYP_PK
					" and ean.typ="STR_EANTYP_LE2
					" and mae.parentid=map.id"
					" and mae.maetyp="STR_MAETYP_PK
					" group by map.id,ean.unit_menge"
					" union"
					" select map.id,1,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from map,mak,mae"
					" where mak.tournr=:b"
					" and mak.bwart="STR_BWART_AKU
					" and mak.status="STR_MASTAT_FREIGABE
					" and map.parentid=mak.id"
					" and map.postyp="STR_ARTTYP_STD
					" and mae.parentid=map.id"
					" and mae.maetyp="STR_MAETYP_PK
					" and not exists (select id from ean where"
					"  unit_artnr=map.bunit_artnr and"
					"  maetyp="STR_MAETYP_PK" and"
					"  typ="STR_EANTYP_LE2")"
					" group by map.id"
					" order by 1",
				UPDBUF,0,
				SQLSTRING(tournr),
				SQLSTRING(tournr),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mpk[0]),
				SELLONG(smpk[0]),
				SELLONG(sgpk[0]),
				NULL);

		/*
		 * hole GP/TT Mengen aus zugehoerigen eans
	 	*/

		memset(mgp,0,sizeof(mgp));	 
		memset(smgp,0,sizeof(smgp));	 
		memset(sggp,0,sizeof(sggp));	 
		memset(mapid,0,sizeof(mapid));	 
		memset(maetyp,0,sizeof(maetyp));	 

		mapcnt	= TExecSqlX(tid,NULL,
					"select map.id,ean.unit_menge,ean.maetyp,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from ean,map,mak,mae"
					" where mak.tournr=:a"
					" and mak.bwart="STR_BWART_AKU
					" and mak.status="STR_MASTAT_FREIGABE
					" and map.parentid=mak.id"
					" and map.postyp="STR_ARTTYP_STD
					" and ean.unit_artnr=map.bunit_artnr"
					" and (ean.maetyp="STR_MAETYP_GP" or ean.maetyp="STR_MAETYP_TT")"
					" and mae.parentid=map.id"
/*					" and (mae.maetyp="STR_MAETYP_GP" or mae.maetyp="STR_MAETYP_TT")" 
*/					" and mae.maetyp<>"STR_MAETYP_PK
					" group by map.id,ean.unit_menge,ean.maetyp"
					" union"
					" select map.id,1,'--',SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from map,mak,mae"
					" where mak.tournr=:b"
					" and mak.bwart="STR_BWART_AKU
					" and mak.status="STR_MASTAT_FREIGABE
					" and map.parentid=mak.id"
					" and map.postyp="STR_ARTTYP_STD
					" and mae.parentid=map.id"
					" and (mae.maetyp="STR_MAETYP_GP" or mae.maetyp="STR_MAETYP_TT")" 
					" and not exists (select id from ean where"
					"  unit_artnr=map.bunit_artnr"
					"  and (ean.maetyp="STR_MAETYP_GP" or ean.maetyp="STR_MAETYP_TT"))"
					" group by map.id"
					" order by 1",
				UPDBUF,0,
				SQLSTRING(tournr),
				SQLSTRING(tournr),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mgp[0]),
				SELSTR(maetyp[0],STRVALUELEN_MAETYP+1),
				SELLONG(smgp[0]),
				SELLONG(sggp[0]),
				NULL);

	
		if (pkcnt!=mapcnt||pkcnt<=0) {
			/*
			 * something's wrong ...
			 */
			return TDB_MASTAT_ERR;
		} 

		/*
		 * hole tour-MAM's
		 */

		memset(mam,0,sizeof(mam));	 
		if (!mambuf) {
			pkcnt	= TExecSqlX(tid,NULL,
						"select %MAM from mam"
						" where parentid=:a"
						" and rectyp="STR_RECTYP_TOUR
						" order by maetyp"
						" for update nowait",
						3,0,
						SQLSTRING(tournr),
						SELSTRUCT(TN_MAM,mam[0]),
						NULL);

			if (pkcnt!=3) {
				/*
				 * something's wrong ...
				 */
				return TDB_MASTAT_ERR;
			}
		} else {
			memcpy(mam,mambuf,sizeof(mam));
		}

		mam[0].mamEAnzahl = mam[0].mamEAnzEinzel = mam[0].mamEGewNetto = 0;
		mam[1].mamEAnzahl = mam[1].mamEAnzEinzel = mam[1].mamEGewNetto = 0;	
		mam[2].mamEAnzahl = mam[2].mamEAnzEinzel = mam[2].mamEGewNetto = 0;

		for (;mapcnt>0;mapcnt--) {

			/*
			 * PK ...
			 */

			mam[1].mamEGewNetto += sgpk[mapcnt-1];
			mam[1].mamEAnzEinzel += smpk[mapcnt-1];
			mam[1].mamEAnzahl += smpk[mapcnt-1]/mpk[mapcnt-1];

			/*
			 * GP/TT
			 */

			if (!strcmp(maetyp[mapcnt-1],strvalue_MAETYP[MAETYP_GP])) {
				mam[0].mamEGewNetto += sggp[mapcnt-1];
				mam[0].mamEAnzEinzel += smgp[mapcnt-1];
				mam[0].mamEAnzahl += smgp[mapcnt-1]/mgp[mapcnt-1];
			} else if (!strcmp(maetyp[mapcnt-1],strvalue_MAETYP[MAETYP_TT])) {
				mam[2].mamEGewNetto += sggp[mapcnt-1];
				mam[2].mamEAnzEinzel += smgp[mapcnt-1];
				mam[2].mamEAnzahl += smgp[mapcnt-1]/mgp[mapcnt-1];
			}			 
						
		}

		rv	= TExecStdSql(tid,StdNupdate,TN_MAM,&mam[0])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
		rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&mam[1])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
		rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&mam[2])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											

		if (mambuf) {
			memcpy(mambuf,mam,sizeof(mam));
		}
	}
	return rv;
}

int TDB_UpdateAuftrMAM(void *tid, char *makid, MAM *mambuf)
{
	int 	rv	= TDB_MASTAT_ERR;
	char 	mapid[UPDBUF][RECID_LEN+1];
	char 	maetyp[UPDBUF][STRVALUELEN_MAETYP+1];
	int		mapcnt,pkcnt;
	long	mpk[UPDBUF],mgp[UPDBUF];
	long	smpk[UPDBUF],sgpk[UPDBUF];
	long	smgp[UPDBUF],sggp[UPDBUF];
	MAM		mam[3];
	
	if (makid) {

		/*
		 * hole LE2/Pick bzw. gp Mengen aus zugehoerigen eans
	 	 */

		memset(mpk,0,sizeof(mpk));	 
		memset(smpk,0,sizeof(smpk));	 
		memset(sgpk,0,sizeof(sgpk));	 
		memset(mapid,0,sizeof(mapid));	 

		pkcnt	= TExecSqlX(tid,NULL,
					"select map.id,ean.unit_menge,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from ean,map,mae"
					" where map.parentid=:a"
					" and map.postyp="STR_ARTTYP_STD
					" and ean.unit_artnr=map.bunit_artnr"
					" and ean.maetyp="STR_MAETYP_PK
					" and ean.typ="STR_EANTYP_LE2
					" and mae.parentid=map.id"
					" and mae.maetyp="STR_MAETYP_PK
					" group by map.id,ean.unit_menge"
					" union"
					" select map.id,1,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from map,mae"
					" where map.parentid=:b"
					" and map.postyp="STR_ARTTYP_STD
					" and mae.parentid=map.id"
					" and mae.maetyp="STR_MAETYP_PK
					" and not exists (select id from ean where"
					"  unit_artnr=map.bunit_artnr and"
					"  maetyp="STR_MAETYP_PK" and"
					"  typ="STR_EANTYP_LE2")"
					" group by map.id"
					" order by 1",
				UPDBUF,0,
				SQLSTRING(makid),
				SQLSTRING(makid),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mpk[0]),
				SELLONG(smpk[0]),
				SELLONG(sgpk[0]),
				NULL);

		/*
		 * hole GP/TT Mengen aus zugehoerigen eans
	 	*/

		memset(mgp,0,sizeof(mgp));	 
		memset(smgp,0,sizeof(smgp));	 
		memset(sggp,0,sizeof(sggp));	 
		memset(mapid,0,sizeof(mapid));	 
		memset(maetyp,0,sizeof(maetyp));	 

		mapcnt	= TExecSqlX(tid,NULL,
					"select map.id,ean.unit_menge,ean.maetyp,SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from ean,map,mae"
					" where map.parentid=:a"
					" and map.postyp="STR_ARTTYP_STD
					" and ean.unit_artnr=map.bunit_artnr"
					" and (ean.maetyp="STR_MAETYP_GP" or ean.maetyp="STR_MAETYP_TT")"
					" and mae.parentid=map.id"
/*					" and (mae.maetyp="STR_MAETYP_GP" or mae.maetyp="STR_MAETYP_TT")" 
*/					" and mae.maetyp<>"STR_MAETYP_PK
					" group by map.id,ean.unit_menge,ean.maetyp"
					" union"
					" select map.id,1,'--',SUM(mae.lunit_menge),SUM(mae.lunit_gewnetto)"
					" from map,mae"
					" where map.parentid=:b"
					" and map.postyp="STR_ARTTYP_STD
					" and mae.parentid=map.id"
					" and (mae.maetyp="STR_MAETYP_GP" or mae.maetyp="STR_MAETYP_TT")" 
					" and not exists (select id from ean where"
					"  unit_artnr=map.bunit_artnr"
					"  and (ean.maetyp="STR_MAETYP_GP" or ean.maetyp="STR_MAETYP_TT"))"
					" group by map.id"
					" order by 1",
				UPDBUF,0,
				SQLSTRING(makid),
				SQLSTRING(makid),
				SELSTR(mapid[0],RECID_LEN+1),
				SELLONG(mgp[0]),
				SELSTR(maetyp[0],STRVALUELEN_MAETYP+1),
				SELLONG(smgp[0]),
				SELLONG(sggp[0]),
				NULL);

	
		if (pkcnt!=mapcnt||pkcnt<=0) {
			/*
			 * something's wrong ...
			 */
			return TDB_MASTAT_ERR;
		} 

		/*
		 * hole Auftrtag-MAM's
		 */

		memset(mam,0,sizeof(mam));	 
		if (!mambuf) {
			pkcnt	= TExecSqlX(tid,NULL,
						"select %MAM from mam"
						" where parentid=:a"
						" and rectyp="STR_RECTYP_MAK
						" order by maetyp"
						" for update nowait",
						3,0,
						SQLSTRING(makid),
						SELSTRUCT(TN_MAM,mam[0]),
						NULL);
		} else {
			memcpy(mam,mambuf,sizeof(mam));
		}

		if (pkcnt!=3) {
			/*
			 * something's wrong ...
			 */
			return TDB_MASTAT_ERR;
		}
		
		mam[0].mamEAnzahl = mam[0].mamEAnzEinzel = mam[0].mamEGewNetto = 0;
		mam[1].mamEAnzahl = mam[1].mamEAnzEinzel = mam[1].mamEGewNetto = 0;	
		mam[2].mamEAnzahl = mam[2].mamEAnzEinzel = mam[2].mamEGewNetto = 0;

		for (;mapcnt>0;mapcnt--) {

			/*
			 * PK ...
			 */

			mam[1].mamEGewNetto += sgpk[mapcnt-1];
			mam[1].mamEAnzEinzel += smpk[mapcnt-1];
			mam[1].mamEAnzahl += smpk[mapcnt-1]/mpk[mapcnt-1];

			/*
			 * GP/TT
			 */

			if (!strcmp(maetyp[mapcnt-1],strvalue_MAETYP[MAETYP_GP])) {
				mam[0].mamEGewNetto += sggp[mapcnt-1];
				mam[0].mamEAnzEinzel += smgp[mapcnt-1];
				mam[0].mamEAnzahl += smgp[mapcnt-1]/mgp[mapcnt-1];
			} else if (!strcmp(maetyp[mapcnt-1],strvalue_MAETYP[MAETYP_TT])) {
				mam[2].mamEGewNetto += sggp[mapcnt-1];
				mam[2].mamEAnzEinzel += smgp[mapcnt-1];
				mam[2].mamEAnzahl += smgp[mapcnt-1]/mgp[mapcnt-1];
			}			 
						
		}

		rv	= TExecStdSql(tid,StdNupdate,TN_MAM,&mam[0])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
		rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&mam[1])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											
		rv	= rv==TDB_MASTAT_OK&&TExecStdSql(tid,StdNupdate,TN_MAM,&mam[2])>0?TDB_MASTAT_OK:TDB_MASTAT_ERR;											

		if (mambuf) {
			memcpy(mambuf,mam,sizeof(mam));
		}
	}
	return rv;
}
 
/*
 * setzt uebergebene Tour fertig
 */
/* 
 * sis: Umbau für neue Schnittstelle:
 * Ich brauch hier nur die noch nicht fertig-gesetzten
 * MAK's über TDB_AuftrFertig rückmelden !!
 */
int TDB_TourFertig(const std::string & fac, void *tid,char *tournr,char *user)
{
	int     rv  = TDB_MASTAT_OK;
	int 	cnt=0,i=0;
	MAK		mak[UPDBUF];
	
	if (tournr) 
	{
		memset(mak,0,sizeof(MAK)*UPDBUF);

		cnt = TExecSqlX(tid,NULL,
					"select %MAK from mak "
					" where mak.tournr=:a"
					" and mak.status<>" STR_MASTAT_FERTIG
					" and " TCN_MAK_HostStatus " = " STR_HOSTSTATUS_FERTIG
					" order by mak.id",
					UPDBUF,0,
					SQLSTRING(tournr),
					SELSTRUCT(TN_MAK,mak[0]),
					NULL);

		/* Wenn 'echter' Fehler, dann zurückmelden !! */
		if (cnt<=0 && TSqlError(tid)!=SqlNotFound) {
			return (TDB_MASTAT_ERR);
		}

		if (cnt<=0 && TSqlError(tid)==SqlNotFound) {
			return (TDB_MASTAT_OK);
		}

		rv	= TDB_MASTAT_OK;
		for (i=0;i<cnt && (rv==TDB_MASTAT_OK);i++)
		{
			/* Einzelne Aufträge rückmelden */
		  LogPrintf(FAC_LIB,LT_NOTIFY,"%s", mak[i].makId );	
		  rv=TDB_AuftrFertig(fac,tid,mak[i].makId,user);
		}
		LogPrintf(FAC_LIB,LT_NOTIFY,"rv: %d cnt: %d", rv, cnt );

		/* Tour aufräumen */
		if (rv==TDB_MASTAT_OK)
		{
			rv=TDB_CleanUpTour(tid,tournr,user);	
		}
	
		/* Meldung schreiben !! */	
		LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Tour fertiggesetzt: %s von: %s (status:%d)",
				  	tournr,
				  	user,
					rv);

	}
	return rv;
}

/* 
 * sis: Umbau für Schnittstelle, dass LHId-Weise   
 *  zurückgemeldet wird
 */


/* 
 * sis: Tour 'aufräumen' 
 */
static int TDB_CleanUpTour(void *tid,const char *tournr,const char *user)
{
	/* Tour-Mam löschen */
	TDB_DeleteMam(tid,tournr,RECTYP_TOUR);

	/* Tourenprotokollierung schreiben */
    WriteTourProtAll(tid,tournr,0);
	int xrv = TExecSql(tid,"delete from kak where tournr=:a",
				SQLSTRING(tournr),
				NULL);

	if( xrv < 0 && TSqlError(tid) != SqlNotFound ) {
	    LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s", TSqlErrTxt(tid));


		FetchTable<MAP> vMAP( tid, NULL, TN_MAP,
						format( " where " TCN_MAP_KakId " in ( select " TCN_KAK_Id " from " TN_KAK 
						"     where " TCN_KAK_TourNr " = '%s' ) ", 
						tournr ) );

		if( !vMAP ) {
			LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s sql: %s", TSqlErrTxt(tid ), vMAP.get_sql() );
		} else {
			 for( const MAP & map : vMAP ) {
				MAK mak = {};
				StrCpy( mak.makId, map.mapParentId );
				if( TExecStdSql( tid, StdNselect, TN_MAK, &mak ) < 0 ) {
					 LogPrintf( FAC_LIB, LT_ALERT, "SqlError: %s ", TSqlErrTxt(tid ) );
				}
				else
				{
					LogPrintf(  FAC_LIB, LT_ALERT, "MAK: %s TourNr %s, RefNr: %s", mak.makId, mak.makTourNr, mak.makRefNr );
				}	
			 }
		}


		return TDB_MASTAT_ERR;
	}

	return TDB_MASTAT_OK;
}

/*
 * setzt uebergebenen Auftrag fertig
 */
int TDB_AuftrFertig(const std::string & fac, void *tid,const char *makid,const char *user)
{
	int 	rv	= TDB_MASTAT_ERR;
	int		ok=0;
	LStick  maklist=NULL;
	MAK		mak;

	LogPrintf( fac, LT_ALERT, "setzte Auftrag Fertig: %s", makid );

	/* Wenn makid gesetzt, ist's schon mal kein Fehler ... */	
	if (makid) 
	{
		/* sis: Den kompletten MAK rückmelden !! */
	  
		  try {
			ok=Sap::WriteMAK(fac,tid,makid );
		  } catch( std::exception & err ) {
			LogPrintf( fac, LT_ALERT, "%s Exception Caught: %s", makid, err.what() );
			return (TDB_MASTAT_ERR);			
		  }

		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d", ok );

		/* Auftrag nicht rückmelden, wenn Fehler aufgetreten !! */
		if (!ok) {
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d", ok );
			return (TDB_MASTAT_ERR);
		}

		/* Originalen MAK holen */
		memset(&mak,0,sizeof(MAK));
		StrCpyDestLen(mak.makId,makid);
		ok = TExecStdSql(tid,StdNselect,TN_MAK,&mak);	
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d", ok );

		/* sis: Mam's erst dann löschen, wenn Rückmeldung ok !! */
		if (ok)
			TDB_DeleteMam(tid,makid,RECTYP_MAK);

		if (ok) {
		  
		    try {
		        ok=Sap::WriteVerpMeldung(fac,tid,mak );
		    } catch( std::exception & err ) {
		        LogPrintf( fac, LT_ALERT, "Exception Caught: %s", err.what() );
		        return (TDB_MASTAT_ERR);
		    }

		    if( !ok )
		      {
		        LogPrintf( fac, LT_ALERT,
		            "Fehler beim Schreiben der Verpackungsmeldung für Tour: "
		            "%s, Auftrag: %s MakId: %s, keine Daten gefunden, dies muß nicht unbedingt ein Fehler sein, mache weiter.",
		            mak.makTourNr, mak.makRefNr, makid );
		        ok = TDB_MASTAT_OK;
		      }
		
		  ok = TDB_DeleteTek(fac,tid,makid,user)==TDB_MASTAT_OK?1:0;
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d SqlFehler: %s", ok, TSqlErrTxt(tid) );
			if (ok)
				ok = TExecSql(tid,"update mak set status="STR_MASTAT_FERTIG" where id=:a",
						SQLSTRING(makid),
						NULL)>0?1:0;
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d", ok );
			if (ok)
				ok = TExecSql(tid,"update map set status="STR_MASTAT_FERTIG" where parentid=:a",
						SQLSTRING(makid),
						NULL)>0?1:0;
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d", ok );
			if (ok>=0)
			  ok = TDB_protMakMap(fac,tid,makid,user)==DB_UTIL_SQLFEHLER?0:1;
		LogPrintf( FAC_LIB,LT_NOTIFY, "ok: %d; SqlError: %s", ok, TSqlErrTxt(tid) );
		}
		if (ok) 
		{
			/* Wenn dies der einzige Auftrag in Tour, dann Tour Fertig setzen !! */
			maklist	= _load_rec_list(tid, RECTYP_MAK, FALSE, mak.makTourNr, NULL);
			if (!maklist)
			{
				rv=TDB_CleanUpTour(tid,mak.makTourNr,user);
				/* Meldung schreiben !! */	
				LogPrintf(FAC_LIB,LT_NOTIFY,
					  	"Tour fertiggesetzt: %s von: %s (status:%d)",
				  		mak.makTourNr,
				  		user,
						rv);
			}
			else 
				rv=TDB_MASTAT_OK;
			maklist	= _free_rec_list(maklist);					
		}
		else if (ok)
			rv	= TDB_MASTAT_OK;

		LogPrintf(FAC_LIB,LT_NOTIFY,
				  	"Auftrag fertiggesetzt: %s von: %s (status:%d)",
				  	mak.makMaNr,
				  	user,
					rv);
	}
	return rv;
}

bool containsStrategieOr( const MAP & map, const std::string & strat_letters )
{
  std::string strat = map.mapStrat;

  for( unsigned i = 0; i < strat_letters.size(); i++ )
	{
	  char strat_char = strat_letters[i];

	  if( strat.find(strat_char) != std::string::npos )
		{
		  return true;
		}	  
	}

  return false;
}

bool verplaner_neu( const MAP & map )
{
  // nur bei Strategie A,B und C aktivieren
  if( !containsStrategieOr( map, "ABCT" ) ) {
	return false;
  }
  
  
  // am Testsystem immer den neuen Verplaner wahlen
  if( isTestSystem() ) {
	return true;
  }
  
  char *s = AppContextGetData( "VerplanerNeuArtikel" );
  
  if( is_empty_string( s ) )
	{
	  return true;
	}
  
  std::vector<std::string> sl = split_and_strip_simple( s, ", \t;");
  
  for( unsigned i = 0; i < sl.size(); i++ )
	{
	  if( sl[i] == map.mapBUnit.unitArtNr ) {
		return true;
	  }
	}
  
  return false;
}
