/* globales Include */

#ifndef _tdb20_h
#define _tdb20_h

#include <disperr.h>
#include <svar20.h>

/* [[ defines ]] */

#define TDB_KEY "Tdb"
#define TDB_VERSION "20"
#define TDB_NAME "DB-Wartung - Basis"

#define PROVIDE_TDB Mod_Provide(TDB_KEY,TDB_VERSION,TDB_NAME,\
                                 Tdb_Init,Tdb_Exit)
#define REQUIRE_TDB Mod_Require(TDB_KEY,TDB_VERSION,TDB_NAME,\
                                 Tdb_Init,Tdb_Exit)

#define Tdb_DispErrPK TDB_DispErrPK

#ifdef __cplusplus
extern "C" {
#endif


char *Tdb_CreateAtomNameStr(char *mask_name,
                            char *el_name,
                            const char *tableName,
                            char *result);
int tdb_GetAtomClientData(MskDialog mask,
                              MskElement el,
                              char *atomName,
                              Value *data /* RET: ClientData oder NULL */
							);
char *Tdb_CreateAtomName(MskDialog mask,
                                MskElement el,
                                const char *tableName,
                                char *result);

                                 
#define TABLE_NAME_LEN		32

#define TDB_ATOM_NAME_LEN	64

#define TDB_TABLE_SIG		0x5F0001FF

#define TDB_FLAG_NONE			(0x00000000L)
#define TDB_FLAG_LOCAL_ATOM		(0x00000001L)
#define TDB_FLAG_GLOBAL_ATOM	(0x00000002L)
#define TDB_FLAG_UPDATE			(0x00000004L)

#define TDB_FLAG_DEFAULT		TDB_FLAG_LOCAL_ATOM

#define TDBA_TABLE_BASE		"TDBA_TABLE_"
#define TDBA_TABLES_ARRAY	"TDBA_TABLES_ARRAY"
#define TDBA_TABLES			"TdbTables"
#define TDBA_LISTS			"TdbLists"
#define TDBA_FILTERS		"TdbFilters"
#define TDBA_FACNAME		"TdbFacname"
#define TDBA_SETFOCUS		"TdbSetFocus"
#define TDBA_CB_CHECK		"TdbCbCheck"
#define TDBA_CB_CHECK_MATCH	"TdbCbCheckMatch"
#define TDBA_CHECK_MSG		"TdbCheckMsg"
#define TDBA_RECID_BASE		"TdbRecId_"

#define TDB_CTRL_NONE		(0x00000000L)
#define TDB_CTRL_MSGNONE	(0x00000001L)
#define TDB_CTRL_MSGIMP		(0x00000002L)
#define TDB_CTRL_MSGMOST	(0x00000004L)
#define TDB_CTRL_MSGALL		(0x00000008L)
#define TDB_CTRL_COMMIT		(0x00000010L)
#define TDB_CTRL_ROLL		(0x00000020L)

#define TDB_CTRL_MSGDEF		(TDB_CTRL_MSGIMP|TDB_CTRL_MSGMOST|TDB_CTRL_MSGALL)
#define TDB_CTRL_DEFAULT	(TDB_CTRL_ROLL|TDB_CTRL_COMMIT|TDB_CTRL_MSGIMP)	

#define TDB_ALL_TABLES		NULL

     /* compatibility defines so old codes does not break */
#define getDiffBetwTables tdb_DiffTables
#define getDiffBetwTablesX 	  tdb_DiffTablesX
#define getDiffBetwTablesXNew tdb_DiffTablesX_New 
#define getDiffBetwPK tdb_DiffPK
#define PkFilled tdb_PkFilled
#define PkCommitBox tdb_PkCommitBox
#define checkPrimKey tdb_CheckPK
#define LogDiffRec tdb_LogDiffRec


#define ReadRec LOC?0:tdb_ReadRec
#define LockRec LOC?0:tdb_LockRec
#define WriteRec LOC?0:tdb_WriteRec
#define DeleteRec LOC?0:tdb_DeleteRec

#define TdbCreateAtomNameStr Tdb_CreateAtomNameStr
#define TdbCreateAtomName Tdb_CreateAtomName     
#define TdbAtomClientData Tdb_GetAtomClientData
#define TdbCreateTableAtom Tdb_CreateTableAtoms
#define TdbStrAtom Tdb_GetStrAtom
#define TdbStrAtomEnum Tdb_GetStrAtomEnum
#define TdbNumAtom Tdb_GetNumAtom
#define Tdb_NumAtom Tdb_GetNumAtom
#define Tdb_BoolAtom Tdb_GetBoolAtom
#define Tdb_FctAtom Tdb_GetFctAtom

/* damit die Log-Messages bessere Positionen ausgeben: */
#include <logtool.h>
#define Tdb_ReadRec LOC?0:tdb_ReadRec
#define Tdb_WriteRec LOC?0:tdb_WriteRec
#define Tdb_LockRec LOC?0:tdb_LockRec
#define Tdb_DeleteRec LOC?0:tdb_DeleteRec

#define Tdb_GetStrAtom LOC?0:tdb_GetStrAtom
#define Tdb_GetStrAtomEnum LOC?0:tdb_GetStrAtomEnum
#define Tdb_GetNumAtom LOC?0:tdb_GetNumAtom
#define Tdb_GetBoolAtom LOC?0:tdb_GetBoolAtom
#define Tdb_GetFctAtom LOC?0:tdb_GetFctAtom
#define Tdb_GetAtomClientData LOC?0:tdb_GetAtomClientData

#define Tdb_CreateTableNameAtomGlobal LOC?0:tdb_CreateTableNameAtomGlobal
#define Tdb_FillTableAtom LOC?0:tdb_FillTableAtom
#define Tdb_CreateTableAtoms LOC?0:tdb_CreateTableAtoms

/* [endfold] */
/* [[ includes ]] */

/* [endfold] */
/* [[ typedefs ]] */

typedef enum _siNmsg {
	SiNsilent,		/* Keine Meldungen oder Commitbox */
	SiNshortDiff,	/* Bei Differenz (RecordBefore und After) nur Kurzmeldung */
	SiNmsg			/* Alle Meldungen und Commitbox */
}	siNmsg;

typedef enum _sqlNaction {
	SqlNrollCom,		/* Rollbacks und Commits werden durchgefœhrt*/
	SqlNroll,			/* Nur Rollbacks (keine Commits) werden durchgefœhrt */
	SqlNcom,			/* Nur Commits (keine Rollbacks) werden durchgefœhrt */
	SqlNwhithoutRollCom	/* Ohne Rollbacks und Commits */
}	sqlNaction;

typedef struct {
	DWord		signature;
	size_t		rec_size;
	char		tablename[TABLE_NAME_LEN+1];
	void 		*pRecBefore;
	void 		*pRecNow;
	void 		*pRecInDb;
	int	 		readBefore;
    int         readInit; /* Daten neu lesen beim Install des
                             Callbacks (Werte wurden gesetzt) */
}	REC_DB;

typedef struct {
	void			*ptr;
	MskTmaskRlPtr 	mask_rl;
	ListBuffer		lb, lb2;
	int				changed;
}	REC_LB;

/* [endfold] */

/* [[ protos ]] */

int Tdb_Init(void);
int Tdb_Exit(void);

int Tdb_TdbMaskCB(MskDialog mask, int reason);

REC_DB *Tdb_GetRecPtr(MskDialog mask,
                      const char *tableName /* kann auch NULL sein */
                      );

int tdb_CreateTableNameAtomGlobal(char *mask,
                                  char *tableName,
                                  void *data
                                  );
int tdb_FillTableAtom(MskDialog mask,
                      const char *tableName,
                      void *data, /* kann auch NULL sein, dann
                                     leerer REC_DB */
                      int read_before /* Setzt das readBefore
                                         Flag im REC_DB */
                      );
int tdb_CreateTableAtoms(MskDialog mask,
                         char *tables, 
                         long flags /* globale oder lokale Atome */
                         );
int Tdb_DestroyTableAtomsGlobal(void);
int Tdb_DestroyTableAtoms(MskDialog mask);

int tdb_GetStrAtom(MskDialog mask,
                   MskElement el,
                   char *atomName, /* Name des gesuchten
                                      Atoms, kompletter String */
                   char **data /* RET: Wert des Atoms
                                  (sonst NULL) */
                   );
int tdb_GetStrAtomEnum(MskDialog mask,
                       MskElement el,
                       char *atomName,
                       Value e,
                       char **data
                       );
int tdb_NumAtom(MskDialog mask,
                MskElement el,
                char *atomname,
                int *data /* RET: Integer oder 0 */
                );
int tdb_BoolAtom(MskDialog mask,
                 MskElement el,
                 char *atomname,
                 int *data);
int tdb_FctAtom(MskDialog mask,
                MskElement el,
                char *atomname,
                void **data);
int tdb_AtomClientData(MskDialog mask,
                       MskElement el,
                       char *atomName,
                       Value *data /* RET: ClientData oder NULL */
                       );
int tdb_DeleteRec(MskTmaskRlPtr  mask_rl,
                         REC_DB *pRec,
                         char *pTableName,
                         int sqlaction,
                         int silent,
                         char *log);
int tdb_WriteRec(MskTmaskRlPtr mask_rl,
                        REC_DB *pRec,
                        const char *pTableName,
                      int sqlaction,
                        int silent,
                        const char *log);

int tdb_ReadRec(MskDialog mask_rl,
                       REC_DB *pRec,
                       const char *pTableName,
                       int sqlaction,
                       int silent,
                       const char *log);

int tdb_GetNumAtom(MskDialog mask,
                       MskElement el,
                       char *atomName,
                       int *data /* RET: Integer oder 0 */
                       );

/* [endfold] */
#ifdef __cplusplus
} /* extern C */ 
#endif

#endif /* _tdb20_h */
