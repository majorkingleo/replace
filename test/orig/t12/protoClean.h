
/**********************************************************************
+*
+*	static char SCCSID [] = "@(#)protoClean.h	1.4 11/29/95 09:34:07"; 
+*
+*  PACKAGE:   MILEX
+*
+*  FILE:      protoClean.h
+*
+*  CONTENTS:  
+*
+*  PURPOSE:  
+*
+*  NOTES:      
+*
+*
+*  COPYRIGHT NOTICE:
+*
+*  		(c) Copyright 1995 by
+*
+*                  Salomon Automationstechnik Ges.m.b.H
+*                  Friesach 67
+*                  A-8114 Stuebing
+*                  Tel.: (++43) (3127) 2211-0
+*                  Fax.: (++43) (3127) 2211-22
+*
+*
+*
+*
+*  REVISION HISTORY:
+*
+*  Rev  Date       Comment                                         By
+*  ---  ---------  ----------------------------------------------  ---
+*    0  04-Jul-95  created (from)                                  cat
+*
+**********************************************************************/


#ifndef PROTOCLEAN_H_INCLUDED
#define PROTOCLEAN_H_INCLUDED



#  if defined(__STDC__) || defined(__cplusplus)

int ProtoClean(int DbKey,void *Record,int DbMode,
				unsigned timeroffset,unsigned age_days,char * log);

#else
int ProtoClean();


#endif


#endif 

