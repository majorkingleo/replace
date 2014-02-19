/**********************************************************************
+*  VERSID:    "%Z% @(#)$Header: /big2/cvsroot/tools/src/errmsg/errmsg.h,v 40.5 2007/09/08 20:24:26 haubi Exp $";
+*  PACKAGE:   Error-Message-Handling
+*  FILE:      error.h
+*  CONTENTS:
+*  PURPOSE:  
+*  NOTES:      
+*  COPYRIGHT NOTICE:
+*  		(c) Copyright 1996 by
+*                  Salomon Automationstechnik Ges.m.b.H
+*                  Friesachstrasse 15
+*                  A-8114 Stuebing
+*                  Tel.: ++43 3127 2211-0
+*                  Fax.: ++43 3127 2211-22
+*  REVISION HISTORY:
+*
+*  Rev  Date       Comment                                         By
+*  ---  ---------  ----------------------------------------------  ---
+*    0  13-Jan-97  created (from)                                  haubi
+*
+**********************************************************************/

#ifdef _ERRMSG_C
# if !defined(__lint) && !defined(__LINT__) && !defined(lint)
	static char VERSID_H []
#if defined(__GNUC__)
	__attribute__ ((unused))
#endif
	= "%Z% @(#)$Header: /big2/cvsroot/tools/src/errmsg/errmsg.h,v 40.5 2007/09/08 20:24:26 haubi Exp $";
# endif 
#endif

#ifndef _ERRMSG_H_INCLUDED
#define _ERRMSG_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

enum {
	SUB_OK     =  1,
	SUB_IGNORE =  0,
	SUB_ABORT  = -1,
	SUB_ERR    = -2,
	SUB_DBERR  = -3,
	SUB_USERERR = -4
};


extern void		_SetError	( int err, char const *file, long line );
extern int		SetErrmsg1	( char const *fmt, ...)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((__format__ (__printf__, 1, 2)))
#endif
;
extern int		SetErrmsg2	( char const *fmt, ...)
#if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
  __attribute__((__format__ (__printf__, 1, 2)))
#endif
;

extern int         GetError   ( void );
extern char const *GetErrFile ( void );
extern int         GetErrLine ( void );
extern char const *GetErrmsg1 ( void );
extern char const *GetErrmsg2 ( void );

extern void		ResetError	( void );

#define SetError(err) _SetError(err, __FILE__, __LINE__)

#ifdef __cplusplus
}
#endif

#endif /* _ERRMSG_H_INCLUDED */
