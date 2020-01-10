#ifndef _BLOCKUPDATE_H
#define _BLOCKUPDATE_H
#ifndef NOWAMAS
/*****************************************************************************
+* PROJECT:   projectname
+* PACKAGE:   package name
+* FILE:      blockupdate.h
+* CONTENTS:  overview description, list of functions, ...
+* COPYRIGHT NOTICE:
+* 		(c) Copyright 2002 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Friesach bei Graz
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+****************************************************************************/

/* ==========================================================================
 * INCLUDES
 * =========================================================================*/

#ifdef __cplusplus

#include <string>
#include <vector>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <dbsql.h>

/* ==========================================================================
 * DEFINES AND MACROS
 * =========================================================================*/

/* ==========================================================================
 * TYPEDEFS (ENUMS, STRUCTURES, ...)
 * =========================================================================*/

 /* ------- Enums --------------------------------------------------------- */

 /* ------- Structures ---------------------------------------------------- */

/* ==========================================================================
 * GLOBAL variables and Function-Prototypes
 * =========================================================================*/

 /* ------- Variables ----------------------------------------------------- */

 /* ------- Function-Prototypes ------------------------------------------- */

#if (defined TOOLS_VERSION && TOOLS_VERSION <= 33)
#define SqlTstdStmtRes sqlNstmtRes
#endif

int BlockTExecStdSqlX(void *pvTid, const char *pcFac, SqlTstdStmtRes iReason,
					  const char *pcTableName, void *pvData, long lEle);

#if (defined TOOLS_VERSION && TOOLS_VERSION <= 33)
int getStructSize(char *pcTable);
#endif

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus

/* Absicht, dies soll einen Compilerfehler provozieren! */
template<class T> int BlockTExecStdSqlX(void *pvTid, const void*, SqlTstdStmtRes iReason,
										const std::string & pcTableName, std::vector<T> & ptData );

template<class T> int BlockTExecStdSqlX(void *pvTid, const std::string & pcFac, SqlTstdStmtRes iReason,
										const std::string & pcTableName, std::vector<T> & ptData )
{
  return BlockTExecStdSqlX( pvTid, pcFac.c_str(), iReason, pcTableName.c_str(), (void*) &ptData[0], ptData.size() );
}

template<class T> int BlockTExecStdSqlX(void *pvTid, const std::string & pcFac, SqlTstdStmtRes iReason,

										const std::string & pcTableName, Tools::FetchTable<T> & ptData )
{
  return BlockTExecStdSqlX( pvTid, pcFac.c_str(), iReason, pcTableName.c_str(), (void*) &ptData[0], ptData.size() );
}

template<class T> int BlockTExecStdSqlX(void *pvTid, const char *pcFac, SqlTstdStmtRes iReason,
										 const char *pcTableName, std::vector<T> & ptData )
{
  return BlockTExecStdSqlX( pvTid, pcFac, iReason, pcTableName, (void*) &ptData[0], ptData.size() );
}

template<class T> int BlockTExecStdSqlX(void *pvTid, const char *pcFac, SqlTstdStmtRes iReason,
										const std::string & pcTableName, std::vector<T> & ptData )
{
  return BlockTExecStdSqlX( pvTid, pcFac, iReason, pcTableName.c_str(), (void*) &ptData[0], ptData.size() );
}


#endif

#endif /* NOWAMAS */
#endif  /* _BLOCKUPDATE_H */
