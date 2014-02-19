/**
* @file
* @todo describe file content
* @author Copyright (c) 2013 Salomon Automation GmbH
*/
#include "tdbint.h"

/* [[ Tdb_CreateAtomNameStr ]] */

/* create an atom name from the given table name, mask name and
   element name using standard conventions. if a ptr is passed as
   result then the string is returned in that pointer, otherwise an
   internal static buffer is used (which is overwritten by the next
   call...) */
PUBLIC char *Tdb_CreateAtomNameStr(char *mask_name,
                                   char *el_name,
                                   const char *tableName,
                                   char *result)
{
	static char buffer[TDB_ATOM_NAME_LEN+1];
	char buf1[TDB_ATOM_NAME_LEN+1];
	char *maskName;

	maskName=mask_name;
	if (!result) {
		result=buffer;
    }
	sprintf(buf1,"%s%s",TDBA_TABLE_BASE,tableName);	
	StrUpper(buf1);
    /* create a name containing mask name, element name and table name
       (if present) */
	if (maskName) {
		if (el_name) {
			sprintf(result,"%s.%s.%s",maskName,el_name,buf1);
		} else {
			sprintf(result,"%s.%s",maskName,buf1);
        }
	} else {
		if (el_name) {
			sprintf(result,"%s.%s",el_name,buf1);
		} else {
			strcpy(result,buf1);
        }
        
	}
    /* printf("CREATE ATOM: %s (for m=%s, e=%s)\n",result,
           mask_name?mask_name:"null",
           el_name?el_name:"nix"); */
	return result;	
}

/* [endfold] */
/* [[ Tdb_CreateAtomName ]] */

/* Erzeugen eines Atomnames, wobei die Namen des uebergebenen Dialoges
   und des uebergebenen Elements sowie der Tabellenname verwendet
   werden. Alle diese Werte koennen auch NULL sein */
PUBLIC char *Tdb_CreateAtomName(MskDialog mask,
                                MskElement el,
                                const char *tableName,
                                char *result)
{
	char *name=NULL,*el_name=NULL;
	
	if (el) {
		el_name=(char *)MskElementGet(el,MskNname);
    }
	if (mask) {
		name = (char *)MskDialogGet(mask,MskNmaskName);
	}
	return Tdb_CreateAtomNameStr(name,el_name,tableName,result);
}

/* [endfold] */
/* [[ tdb_GetStrAtom ]] */

/* Liefert fuer den angegebenen Atom-Namen den Wert des Atoms als
   String in data zurueck. Wenn das Atom nicht gefunden wurde, dann
   ist data NULL und der Rueckgabewert RETURN_ERROR */
PUBLIC int tdb_GetStrAtom(MskDialog mask,
                       MskElement el,
                       char *atomName, /* Name des gesuchten
                                          Atoms, kompletter String */
                       char **data /* RET: Wert des Atoms
                                      (sonst NULL) */
                       )
{
	ContainerAtom ca;

    if((data==NULL)||(atomName==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"NULL ptr");
        return RETURN_ERROR;
    }
        
	*data=NULL;
	ca=MskAtomInquire(mask,el,atomName);
	if(ca==NULL) {
        _LogPrintf(FAC_TDB,LT_ERROR,
                  "Query des StrAtoms %s (m=%s, e=%s) ohne Ergebnis",
                  atomName,MskDialogGet(mask,MskNmaskName),
                  (char*)((MskRlGet(el,MskNname)==0)?"NULL":
                  (char *)MskRlGet(el,MskNname)));
        return RETURN_ERROR;
    }
    
    ApStrTxt2Str(ca->data,data);
    /* printf("QUERY ATOM: %s: %s\n",atomName,*data); */
	return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ tdb_GetStrAtomEnum ]] */

PUBLIC int tdb_GetStrAtomEnum(MskDialog mask,
                           MskElement el,
                           char *atomName,
                           Value e,
                           char **data
                           )
{
	ContainerAtom ca;

    /* FIXME not yet finished: risc */
	*data=NULL;
	ca=MskAtomEnumInquire(mask,el,atomName,e);
	if (ca) {
		ApStrTxt2Str(ca->data,data);
	}
	return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ tdb_GetNumAtom ]] */

/* liefert fuer das uebergebene Element den Int-Wert des Atoms oder
   einen Fehler, falls es das Atom nicht gibt */
PUBLIC int tdb_GetNumAtom(MskDialog mask,
                       MskElement el,
                       char *atomName,
                       int *data /* RET: Integer oder 0 */
                       )
{
	ContainerAtom ca;

    if((data==NULL)||(atomName==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"NULL ptr");
        return RETURN_ERROR;
    }
        
	*data=0;
	ca=MskAtomInquire(mask,el,atomName);
	if(ca==NULL) {
        _LogPrintf(FAC_TDB,LT_ERROR,
                  "Query des StrAtoms %s (m=%s, e=%s) ohne Ergebnis",
                  atomName,MskDialogGet(mask,MskNmaskName),
                  (MskRlGet(el,MskNname)==0)?"NULL":
                  (char *)MskRlGet(el,MskNname));
        return RETURN_ERROR;
    }
    
    ApStrTxt2Num(ca->data,(Value *)data);
	return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ tdb_BoolAtom ]] */

PUBLIC int tdb_BoolAtom(MskDialog mask,
                        MskElement el,
                        char *atomname,
                        int *data)
{
	ContainerAtom ca;

    /* FIXME: has to be finished: risc */
	*data=0;
	ca=MskAtomInquire(mask,el,atomname);
	if (ca) {
		ApStrTxt2Bool(ca->data, data);
	}
	return RETURN_ERROR;
}

/* [endfold] */
/* [[ tdb_FctAtom ]] */

PUBLIC int tdb_FctAtom(MskDialog mask,
                       MskElement el,
                       char *atomname,
                       void **data)
{
	ContainerAtom ca;
	Dictionary dictfunc=BaseDictGet(OWDICT_MSKCALLBACK);

    /* FIXME: has to be finished: risc */

	*data=NULL;
	ca=MskAtomInquire(mask,el,atomname);
	if (ca) {
		*data=(void *)DdGet(dictfunc,ca->data);
	}
	return RETURN_ERROR;
}

/* [endfold] */
/* [[ tdb_GetAtomClientData ]] */

/* Liefert fuer das angefragte Atom den Client-Data Zeiger oder einen
   Fehler */
PUBLIC int tdb_GetAtomClientData(MskDialog mask,
                              MskElement el,
                              char *atomName,
                              Value *data /* RET: ClientData oder NULL */
)
{
	ContainerAtom ca;

    if((data==NULL)||(atomName==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"NULL ptr");
        return RETURN_ERROR;
    }

	*data=0;
	ca=MskAtomInquire(mask,el,atomName);
	if(ca==NULL) {
        _LogPrintf(FAC_TDB,LT_ERROR,
                  "Query des ClientAtoms %s (m=%p, e=%p) ohne Ergebnis",
                  atomName,mask,el);
        return RETURN_ERROR;
    }
    /* printf("QUERY CLIENT: %s: %p\n",atomName,cl);    */
    *data=ca->client_data;
	return RETURN_ACCEPTED;
}

/* [endfold] */

/* [[ tdb_AllocRECDB ]] */

/* Allozieren des Speichers fuer einen REC_DB Datensatz, der die Daten
   fuer einen Eintrag in eine beliebige Tabelle dem Fenstersystem zur
   Verfuegung stellt.
   Achtung: Der REC_DB wird mit malloc alloziert und kann mitsamt
   seinen Records durch ein einzelnes free() freigegeben werden. */
PRIVATE REC_DB *tdb_AllocRECDB(const char *tableName,
                               size_t *recSize /* wenn nicht NULL, liefert
                                                  die Groesse eines Records */
)
{
    int rv;
	size_t size;
	long r_size;
	long t_size;
	long all_size;
	REC_DB *r;
	char *ptr;

    /* Achtung: der TableName muss wohl Uppercase sein, sonst geht's
       hier schon in die Hose. Da hilft das StrUpper spaeter nichts
       mehr */
    rv=Misc_SqlGetRecordSize(tableName,&size);
    if(rv!=RETURN_ACCEPTED) {
        return NULL;
    }

    if(recSize!=NULL) {
        *recSize=size;
    }
    
    /* und das alles nur, damit ein einzelnes free() reicht. Ob das
       nicht mehr Probleme macht als es loest? */
	t_size=MemAlignSize(size);
	r_size=MemAlignSize(sizeof(REC_DB));
	all_size=r_size+3*t_size;
	r= MemAlloc(all_size);
	if(r==NULL) {
        _LogPrintf(FAC_TDB,LT_ERROR,"Kein Speicher fuer REC_DB(%s)",
                  tableName);
        return NULL;
    }
    
    ptr = (char *)r;
    memset(r, 0, all_size);
    r->signature = TDB_TABLE_SIG;
    r->rec_size = size;
    strcpy(r->tablename, tableName);
    StrUpper(r->tablename);
    ptr += r_size;
    r->pRecBefore = (void *)ptr;

    ptr += t_size;
    r->pRecNow = (void *)ptr;

    ptr += t_size;
    r->pRecInDb = (void *)ptr;

	return r;
}

/* [endfold] */
/* [[ tdb_FreeRECDB ]] */

PRIVATE void tdb_FreeRECDB(REC_DB *r)
{
    /* damit die Details (siehe Alloc) nicht jeder wissen muss */
    MemDealloc((char *)r);
}

/* [endfold] */

/* [[ tdb_CreateSingleAtom ]] */

/* Erzeugen eines REC_DB Atomes fuer den angegebenen Dialog (falls es
   dieses nicht bereit gibt). Gibt es ein passendes globales Atom,
   dann werden dessen Daten in das lokale Atom uebernommen, gibt es
   das globale Atom nicht, dann wird es bei entsprechendem Flag
   angelegt. */
PRIVATE int tdb_CreateSingleAtom(MskDialog mask,
                                const char *tableName,
                                long flags)
{
	char tatom[TDB_ATOM_NAME_LEN+1];
	ContainerAtom ca;
	REC_DB *r,*gr;
	
    if((mask==NULL)||(tableName==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"NullPtr");
        return RETURN_ERROR;
    }
    
    Tdb_CreateAtomName(mask,NULL,tableName,tatom);
    ca	= MskAtomInquire(mask,NULL,tatom);
    if(!ca) {
        /* record not yet allocated, do it now */
        r=tdb_AllocRECDB(tableName,NULL);
        if(r==NULL) {
            /* errors reported */
            return RETURN_ERROR;
        }
        if (flags & TDB_FLAG_LOCAL_ATOM) {
            MskAtomAdd(mask,NULL,tatom,NULL,(Value )r);
        }
    } else {
        /* already present, get it/check it */
        r=(REC_DB *)ca->client_data;
        if((r==NULL)||(r->signature!=TDB_TABLE_SIG)) {
            _LogPrintf(FAC_TDB,LT_ERROR,"No table signature for %s:%s",
                      tableName,tatom);
            return RETURN_ERROR;
        }
    }
    
    /* gibt es ein globales Atom mit diesem Namen, dann kopieren wir
       dessen Inhalt in das lokale Atom, ansonsten legen wir das
       globale Atom an (vorausgesetzt das entsprechende Flag ist
       gesetzt) */
    ca=ApAtomGet(tatom);
    if((ca!=NULL)&&(ca->client_data)) {
        gr=(REC_DB *)ca->client_data;
        if(gr->signature!=TDB_TABLE_SIG) {
            _LogPrintf(FAC_TDB,LT_ERROR,
                      "No table signature for global Atom %s:%s",
                      tableName,tatom);
            return RETURN_ERROR;
        }
            
        r->readBefore=gr->readBefore;
        if (r->rec_size!=gr->rec_size) {
            _LogPrintf(FAC_TDB,LT_ERROR,
                      "Recsize differs for global Atom %s:%s",
                      tableName,tatom);
            return RETURN_ERROR;
        }
            
        if (gr->pRecBefore) {
            memcpy(r->pRecBefore,gr->pRecBefore,r->rec_size);
        }
        if (gr->pRecNow) {
            memcpy(r->pRecNow,gr->pRecNow,r->rec_size);
        }
        if (gr->pRecInDb) {
            memcpy(r->pRecInDb,gr->pRecInDb,r->rec_size);
        }
    } else {
        /* neu anlegen */
        if (flags & TDB_FLAG_GLOBAL_ATOM) {
            ApAtomAdd(tatom,NULL,(Value )r);
        }
    }
	return RETURN_ACCEPTED;	
}

/* [endfold] */
/* [[ Tdb_GetRecPtr]] */

/* holt ueber den angegebenen Dialog und den optionalen Tabellennamen
   den zugehoerigen REC_DB Zeiger (fuer den angebundenen
   Datensatz). Wird tableName nicht angegeben, dann wird die erste
   Tabelle verwendet, die im TdbTables Atom vorkommt. Liefert NULL bei
   einem Fehler. */
PUBLIC REC_DB *Tdb_GetRecPtr(MskDialog mask,
                             const char *tableName /* kann auch NULL sein */
                             )
{
    REC_DB *dbPtr;
	char atomName[TDB_ATOM_NAME_LEN+1];
	ContainerAtom ca;
    char *tables;
    char **tableNames;
	
    if((mask==NULL)) {
        LogPrintf(FAC_TDB,LT_ERROR,"NullPtr");
        return NULL;
    }

    if(tableName==NULL) {
		ca=MskAtomInquire(mask,NULL,TDBA_TABLES);
		if(ca!=NULL) {
			ApStrTxt2Str(ca->data,&tables);
		}
        if(tables==NULL) {
            LogPrintf(FAC_TDB,LT_ERROR,
                      "No tables for mask %s",
                      (char*)MskDialogGet(mask,MskNmaskName));
			return NULL;
        }
        tableNames=StrToStrArray(tables,',');
        /* wir benutzen die erste angefundene Tabelle */
        Tdb_CreateAtomName(mask,NULL,tableNames[0],atomName);
        MemDealloc(tableNames);
    } else {
        Tdb_CreateAtomName(mask,NULL,tableName,atomName);
    }
    
    ca=MskAtomInquire(mask,NULL,atomName);
    if((ca==0)||(ca->client_data==0)) {
        LogPrintf(FAC_TDB,LT_ERROR,
                  "No REC_DB for mask %s, atom %s",
                  (char*)MskDialogGet(mask,MskNmaskName),atomName);
        return NULL;
    }
    dbPtr=(REC_DB *)ca->client_data;
    if (dbPtr->signature!=TDB_TABLE_SIG) {
        LogPrintf(FAC_TDB,LT_ERROR,
                  "No tableSig for mask %s, atom %s",
                  (char*)MskDialogGet(mask,MskNmaskName),atomName);
        return NULL;
    }
	return dbPtr;
    
}

/* [endfold] */
/* [[ tdb_CreateTableAtomGlobal ]] */

/* Anlegen eines globalen REC_DB Atoms fuer die uebergebene Tabelle,
   falls dieses Atom noch nicht existiert. Auffuellen der Werte fuer
   Before,Now und inDb mit den in data uebergebenen Werten. */
PUBLIC int tdb_CreateTableNameAtomGlobal(char *mask,
                                         char *tableName,
                                         void *data /* Daten, mit
                                                       denen alle
                                                       Datensaetze des
                                                       REC_DB gefuellt
                                                       werden */
                                         )
{
	char tatom[TDB_ATOM_NAME_LEN+1];
	ContainerAtom ca;
	REC_DB *r=0;

    if((tableName==NULL)||(mask==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"tableName/mask=NULL");
        return RETURN_ERROR;
    }

    Tdb_CreateAtomNameStr(mask,NULL,tableName,tatom);
    ca = ApAtomGet(tatom);
    if (ca && ca->client_data) {
        r = (REC_DB *)ca->client_data;
        if((r==NULL)||(r->signature==TDB_TABLE_SIG)) {
            _LogPrintf(FAC_TDB,LT_ERROR,
                      "No table signature for global Atom %s:%s",
                      tableName,tatom);
            return RETURN_ERROR;
        }
    }
    if(r==NULL) {
        r=tdb_AllocRECDB(tableName,NULL);
        if(r==NULL) {
            /* errors reported */
            return RETURN_ERROR;
        }
    }

    if(StrCaseCmp(r->tablename,tableName)!=0) {
        _LogPrintf(FAC_TDB,LT_ERROR,
                  "Tables differ for global Atom %s:%s (%s:%s)",
                  tableName,tatom,
                  r->tablename,tableName);
        return RETURN_ERROR;
    }
        
    if (r->pRecBefore) {
        memcpy(r->pRecBefore,data,r->rec_size);
    }
    if (r->pRecNow) {
        memcpy(r->pRecNow,data,r->rec_size);
    }
    if (r->pRecInDb) {
        memcpy(r->pRecInDb,data,r->rec_size);
    }

    /* falls noch nicht vorhanden, jetzt Atom hinzufuegen */
    if(ca==NULL) {
        ApAtomAdd(tatom,NULL,(Value )r);
    }
	return RETURN_ACCEPTED;	
}

/* [endfold] */
/* [[ tdb_FillTableAtom ]] */

/* fuer den angegebenen Dialog und die uebergebene Tabelle
   wird der entsprechende REC_DB angelegt (falls es ihn noch nicht gibt)
   und mit den uebergebenen Daten aufgefuellt. */
PUBLIC int tdb_FillTableAtom(MskDialog mask,
                             const char *tableName,
                             void *data, /* kann auch NULL sein, dann
                                            leerer REC_DB */
                             int read_before /* Setzt das readBefore
                                                Flag im REC_DB */
                             )
{
    REC_DB *recPtr;
	int rv;

    if((mask==NULL)||(tableName==NULL)) {
        _LogPrintf(FAC_TDB,LT_ERROR,"mask/table=NULL");
        return RETURN_ERROR;
    }

    rv=tdb_CreateSingleAtom(mask,tableName,TDB_FLAG_LOCAL_ATOM);
    if (rv!=RETURN_ACCEPTED) {
        return RETURN_ERROR;
    }
    recPtr=Tdb_GetRecPtr(mask,tableName);
    if(recPtr==NULL) {
        return RETURN_ERROR;
    }

    if(data==NULL) {
        memset(recPtr->pRecBefore,0,recPtr->rec_size);
        memset(recPtr->pRecNow,0,recPtr->rec_size);
        memset(recPtr->pRecInDb,0,recPtr->rec_size);
    } else {
        memcpy(recPtr->pRecBefore,data,recPtr->rec_size);
        memcpy(recPtr->pRecNow,data,recPtr->rec_size);
        memcpy(recPtr->pRecInDb,data,recPtr->rec_size);
    }
    
    recPtr->readBefore=read_before;
    return RETURN_ACCEPTED;
}

/* [endfold] */
/* [[ tdb_CreateTableAtoms ]] */

/* fuer den angegebenen Dialog und die eventuell uebergebenen Tabellen
   (sonst werden die Tabellennamen aus dem Atom 'TdbTables' gelesen),
   wird der entsprechende REC_DB fuer die Tabelleneintraege angelegt,
   falls es ihn noch nicht gibt.
   Tabellennamen sind durch ',' getrennt. */
PUBLIC int tdb_CreateTableAtoms(MskDialog mask,
                               char *tables, /* Liste der Tabellen,
                                                fuer die REC_DB Atome
                                                erstellt werden
                                                sollen,
                                                kann NULL sein */
                               long flags /* globale oder lokale Atome */
                               )
{
	char **p;
	int rv,err=RETURN_ACCEPTED;

    if(mask==NULL) {
        _LogPrintf(FAC_TDB,LT_ERROR,"mask=NULL");
        return RETURN_ERROR;
    }

    if(!tables) {
        /* hat die Maske ein enstprechendes Atom? */
        Tdb_GetStrAtom(mask,NULL,TDBA_TABLES,&tables);
    }

    if(tables==NULL) {
        /* immer noch keine Tables */
        _LogPrintf(FAC_TDB,LT_ERROR,
                  "keine Tabellennamen fuer %p",mask);
        return RETURN_ERROR;
    }
        
    p=StrToStrArray(tables,',');
    MskAtomAdd(mask,NULL,TDBA_TABLES_ARRAY,NULL,(Value )p);
    for(; *p; p++) {
        rv=tdb_CreateSingleAtom(mask,*p,flags);
        if (rv<0) {
            err=rv;
        }
    }
	return err;	
}

/* [endfold] */
/* [[ Tdb_DestroyTableAtomsGlobal ]] */

PUBLIC int Tdb_DestroyTableAtomsGlobal(void)
{
	ContainerAtom ca;
	char tatom[TDB_ATOM_NAME_LEN+1];
	REC_DB *r;

	sprintf(tatom,"*.%s*",TDBA_TABLE_BASE);
	for (ca=ApAtomFind(NULL,tatom,1); ca; ca=ApAtomFind(ca,tatom,1)) {
		if (ca->client_data) {
			r = (REC_DB *)ca->client_data;
			if (r->signature == TDB_TABLE_SIG) {
				tdb_FreeRECDB(r);
				ca->client_data=0;
			}
		}
	}
	ApAtomRemove(tatom,1);
	return TDB_OK;
}

/* [endfold] */
/* [[ Tdb_DestroyTableAtoms ]] */

PUBLIC int Tdb_DestroyTableAtoms(MskDialog mask)
{
	int rv=TDB_OK;
	ContainerAtom ca;
	char tatom[TDB_ATOM_NAME_LEN+1];
	REC_DB *r;
	void *p;

	sprintf(tatom,"%s*",TDBA_TABLE_BASE);
	for (ca=MskAtomFind(mask,NULL,tatom,NULL,1); ca;
         ca=MskAtomFind(mask,NULL,tatom,ca,1)) {
		if (ca->client_data) {
			r = (REC_DB *)ca->client_data;
			if (r->signature == TDB_TABLE_SIG) {
				tdb_FreeRECDB(r);
				ca->client_data=0;
			}
		}
	}
	MskAtomRemove(mask,NULL,tatom,1);	
	ca = MskAtomGet(mask,NULL,TDBA_TABLES_ARRAY);
	if (ca) {
		p=(void *)ca->client_data;
		if (p)
			MemDealloc(p);
		ApAtomRemove(TDBA_TABLES_ARRAY,0);
	}
	return rv;	
}

/* [endfold] */

/* [[ tdb_TransferMaskToRec ]] */

/* holt anhand der uebergebenen 'fields' (z.B.
   'ART_ArtNr_t,Unit_ArtNr;ART_ArtBez_t,Unit_ArtBez;') die
   entsprechenden Werte aus der Maske (jeweils erster Eintrag ist Name
   des Maskenelementes) und schreibt diese in den uebergebenen Eintrag
   an der Position, die durch den zweiten Eintrag (Tabellenname)
   angegeben wird. Liefert ausserdem einen Filterstring, der fuer
   TexecStdSql(StdNSelect...) verwendet werden kann (statisch
   alloziert...) */
LOCAL int tdb_TransferMaskToRec(
                                MskDialog mask,
                                char *tableName, /* Name der Tabelle */
                                void *recPtr, /* ptr auf die
                                                Tabellen-Struktur */
                                char *fields, /* was transferiert wird
                                                (als Liste mit
                                                xx,yy;)*/
                                char **filter /* RET: kann NULL sein */
                                )
{
    char **bPtr,**transName;
    char *srcName;
    char *dstName;
    MskElement el;
    void *srcPtr;
    void *dstPtr;
    size_t size;
    static char filterStr[200];
    int first;
    
    if((mask==NULL)||(tableName==NULL)
       ||(recPtr==NULL)||(fields==NULL)) {
        LogPrintf(FAC_TDB,LT_ERROR,"null ptr");
        return RETURN_ERROR;
    }

    first=1;
    filterStr[0]='\0';
    
    
    for(bPtr=transName=StrToStrArray(fields,';');*transName;transName++) {
        printf("processing %s\n",*transName);
        if(Util_IsEmptyStr(*transName)) {
            /* kein Fehler, kann "xxxx;" sein */
            break;
        }
        srcName=strtok(*transName,",");
        dstName=strtok(NULL,",");
        if(Util_IsEmptyStr(srcName)||Util_IsEmptyStr(dstName)) {
            LogPrintf(FAC_TDB,LT_ERROR,"transName falsch '%s'",*transName);
            MemDealloc(bPtr);
            return RETURN_ERROR;
        }

        el=MskQueryRl(mask,MskGetElement(srcName),KEY_DEF);
        srcPtr=(void *)MskElementGet(el,MskNduplicate);

        if(srcPtr==NULL) {
            LogPrintf(FAC_TDB,LT_ERROR,
                      "Feldname ungueltig %s, Rec(%s)",
                      srcName,tableName);
            continue;
        }
        
        size=Misc_SqlGetFieldSize(tableName,dstName);
        dstPtr=Misc_SqlGetFieldPtr(tableName,dstName,recPtr);
        if((size<=0)||(dstPtr==NULL)) {
            LogPrintf(FAC_TDB,LT_ERROR,
                      "Feldgroesse/Ptr fuer %s, Rec(%s)",
                      dstName,tableName);
            MemDealloc(bPtr);
            return RETURN_ERROR;
        }
        if(first) {
            strcpy(filterStr,dstName);
            first=0;
        } else {
            strcat(filterStr,StrForm(",%s",dstName));
        }
        /* printf("trans: %s -> %s (%d) '%s'\n",
               srcName,dstName,size,srcPtr); */
        memcpy(dstPtr,srcPtr,size);
    }
    MemDealloc(bPtr);
    if(filter!=NULL) {
        *filter=filterStr;
    }
    
    /* printf("filter=%s\n",filterStr);*/
    return RETURN_ACCEPTED;
    
}

/* [endfold] */

/* [[ tdb_ListOp ]] */

/* Lesen/Loeschen von Datensaetzen aus einem Listbuffer. Aehnlich wie
   Tdb_Operation, aber fuer die Listbuffer in Dialogen wie z.B. die
   EAN-Werte im Artikel-Dialog.
   */
LOCAL int tdb_ListOp(MskDialog mask, /* Dialog */
                     char *lists, /* Tabellen, die von der Op
                                     betroffen sind, wenn NULL,
                                     dann holen aus Atom
                                     'TdbLists' des Dialoges  */
                     int operation, /* lesen/schreiben/locken... */
                     long control /* Flags, beschreiben ob
                                     Commit/Rollback gemacht werden
                                     soll, ob Benutzer Anzeige
                                     eventueller Fehler erhaelt etc.*/
                     ) 
{
    MskElement el;
	char **p,**px;
	char *log;
    ListBuffer lb;
    char *listName;
	int rv;
	int sqlaction=SqlNwhithoutRollCom,silent=SiNsilent;

    if(mask==NULL) {
        LogPrintf(FAC_TDB,LT_ERROR,"mask=NULL");
        return RETURN_ERROR;
    }
    
    /* hat die Maske eine eigene Log-Facility? */
    Tdb_GetStrAtom(mask,NULL,TDBA_FACNAME,&log);
    if(log==NULL) {
        log=FAC_TDB;        /* just in case */
    }
    
	if(lists==NULL) {
        Tdb_GetStrAtom(mask,NULL,TDBA_LISTS,&lists);
	}
    if(lists==NULL) {
        /* kein Fehler, nur keine Listen */
        return RETURN_ACCEPTED;
    }

    if (control & TDB_CTRL_MSGNONE) {
        silent=SiNsilent;
    }
    if (control & TDB_CTRL_MSGIMP) {
        silent=SiNshortDiff;
    }
    if (control & TDB_CTRL_MSGALL) {
        silent=SiNmsg;
    } 

    for (px=p=StrToStrArray(lists,','); *p; p++) {
        listName=StrForm("List%s",*p);
        lb=ListBufferInquireName(mask,listName,KEY_DEF);
        if(lb==NULL) {
            LogPrintf(log,LT_ERROR,
                      "No Listbuf %s for mask %s",
                      listName,(char*)MskDialogGet(mask,MskNmaskName));
            MemDealloc(px);
            return RETURN_ERROR;
        }

        /* get element containing listbuffer and read atoms from it */
        el=MskQueryRl(mask,MskGetElement(listName),KEY_DEF);
        if((operation==TDB_OP_READ)||(operation==TDB_OP_LOCK)) {
            rv=tdb_ReadList(mask,el,lb,*p,sqlaction,silent,log);
            if(rv!=RETURN_ACCEPTED) {
                if (control&(TDB_CTRL_MSGDEF)) {
                    ApBoxAlert (SHELL_OF (mask), HSL_NI,
                                "Fehler beim Lesen der Liste für die "
                                " Maske %s",
                                (char*)MskDialogGet(mask,MskNmaskName));
                }
                MemDealloc(px);
                return RETURN_ERROR;
            }
        } else if(operation==TDB_OP_WRITE) {
        } else if(operation==TDB_OP_DELETE) {
            /* delete all the available list records */
            rv=tdb_DeleteList(mask,el,lb,*p,sqlaction,silent,log);
            if(rv!=RETURN_ACCEPTED) {
                if (control&(TDB_CTRL_MSGDEF)) {
                    ApBoxAlert (SHELL_OF (mask), HSL_NI,
                                "Fehler beim Loeschen der Liste für die "
                                " Maske %s",
                                (char*)MskDialogGet(mask,MskNmaskName));
                }
                MemDealloc(px);
                return RETURN_ERROR;
            }
        }
    }
    
    MemDealloc(px);
	return RETURN_ACCEPTED;	
}

/* [endfold] */
/* [[ tdb_Operation ]] */

/* Schreiben/Lesen/Loeschen/Sperren von Datensaetzen aus beliebigen
   Tabellen anhand eines Dialoges. Holt sich aus den Tabellen-Atomen
   des Dialoges die entsprechenden REC_DB's und fuehrt die angegebene
   Datenbankoperation damit aus. */
LOCAL int tdb_Operation(MskDialog mask, /* Dialog */
                        char *tables, /* Tabellen, die von der Op
                                         betroffen sind, wenn NULL,
                                         dann holen aus Atom
                                         'TdbTables' des Dialoges  */
                        int operation, /* lesen/schreiben/locken... */
                        long control /* Flags, beschreiben ob
                                        Commit/Rollback gemacht werden
                                        soll, ob Benutzer Anzeige
                                        eventueller Fehler erhaelt etc.*/
                        ) 
{
	char **p,**px;
	char *log;
	REC_DB *pRec;
	int err=0,rv=1;
	int sqlaction=SqlNwhithoutRollCom,silent=SiNsilent;
	
    if(mask==NULL) {
        LogPrintf(FAC_TDB,LT_ERROR,"mask=NULL");
        return RETURN_ERROR;
    }
    
    /* hat die Maske eine eigene Log-Facility? */
    Tdb_GetStrAtom(mask,NULL,TDBA_FACNAME,&log);
    if(log==NULL) {
        log=FAC_TDB;        /* just in case */
    }
    
	if(tables==NULL) {
        Tdb_GetStrAtom(mask,NULL,TDBA_TABLES,&tables);
	}
    if(tables==NULL) {
        LogPrintf(log,LT_ERROR,
                  "No tables for mask %s",
                  (char*)MskDialogGet(mask,MskNmaskName));
		if (control&(TDB_CTRL_MSGDEF)) {
			ApBoxAlert (SHELL_OF (mask), HSL_NI,
                        "Keine Tabellen für die Maske %s angegeben",
                        (char*)MskDialogGet(mask,MskNmaskName));
			return RETURN_ERROR;
        }
    }

    if (control & TDB_CTRL_MSGNONE) {
        silent=SiNsilent;
    }
    if (control & TDB_CTRL_MSGIMP) {
        silent=SiNshortDiff;
    }
    if (control & TDB_CTRL_MSGALL) {
        silent=SiNmsg;
    } 

    for (px=p=StrToStrArray(tables,','); *p && rv>=0; p++) {
        pRec=Tdb_GetRecPtr(mask,*p);
        if((pRec==NULL)||(pRec->signature!=TDB_TABLE_SIG)) {
            LogPrintf(log,LT_ERROR,
                      "No tableSig for mask %s, p= %p",
                      (char*)MskDialogGet(mask,MskNmaskName),pRec);
            if (control&(TDB_CTRL_MSGDEF))
                ApBoxAlert (SHELL_OF (mask), HSL_NI,
                            "Keine gueltige Signatur in Db fuer Maske %s",
                            (char*)MskDialogGet(mask,MskNmaskName));
            MemDealloc(px);
            return RETURN_ERROR;
        }
        switch (operation) {
        case TDB_OP_READ:
            /* printf("readrec: %s before=%d init=%d\n",
                   pRec->tablename,pRec->readBefore,pRec->readInit);*/
            rv=ReadRec(mask,pRec,*p,sqlaction, silent,log);
            if(rv==RETURN_ACCEPTED) {
                rv=tdb_ListOp(mask,NULL,operation,control);
            }
            break;
        case TDB_OP_WRITE:
            rv=WriteRec(mask,pRec,*p,sqlaction, silent,log);
            /* reread records, the connection may have changed */
            if(rv==RETURN_ACCEPTED) {
                rv=tdb_ListOp(mask,NULL,operation,control);
            }
            break;
        case TDB_OP_DELETE:
            rv=tdb_ListOp(mask,NULL,operation,control);
            if(rv==RETURN_ACCEPTED) {
                rv=DeleteRec(mask,pRec,*p,sqlaction, silent,log);
            }
            break;
        case TDB_OP_LOCK:
            rv=LockRec(mask,pRec,*p,sqlaction, silent,log);
            if(rv==RETURN_ACCEPTED) {
                rv=tdb_ListOp(mask,NULL,operation,control);
            }
            break;
        }
    }

    if (rv==RETURN_ERROR) {
        if (control & TDB_CTRL_ROLL) {
            TSqlRollback(mask);
        }
        err=RETURN_ERROR; 
    } else if (rv==RETURN_ACCEPTED) {
        err=RETURN_ACCEPTED;
        if((operation==TDB_OP_WRITE)||(operation==TDB_OP_DELETE)) {
            if (control & TDB_CTRL_COMMIT) {
                TSqlCommit(mask);
            } else {
                if (control & TDB_CTRL_ROLL) {
                    TSqlRollback(mask);
                }
                
            }
        }
    }
    
    MemDealloc(px);
	return err;	
}

/* [endfold] */

