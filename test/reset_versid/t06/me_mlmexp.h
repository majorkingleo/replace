#if defined _MLMEXP_C && ! defined __LINT__ && ! defined _MLMEXP_C_H
#define _MLMEXP_C_H
#endif
#ifndef _MLMEXP_H
#define _MLMEXP_H
/*****************************************************************************
+* PROJECT:   WAMAS - P
+* PACKAGE:   package name
+* FILE:      me_mlmexp.h
+* CONTENTS:  overview description, list of functions, ...
+* COPYRIGHT NOTICE:
+*      (c) Copyright 2001 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+* REVISION HISTORY:
+*   $Log: me_mlmexp.h,v $
+*   Revision 1.1  2017/12/06 13:09:42  wamas
+*   MLM-Menus integrated into WAMAS-M; user:ghoer
+*
+*   Revision 1.2  2001/12/03 15:30:36  rschick
+*   Bugfix
+*   Neue Funktionen Import/Export (Wamas-K Version)
+*
+*   Revision 1.1.1.1  2001/08/27 09:48:06  mmovia
+*
+*
+*   Revision 1.1.1.1  2001/04/20 12:45:59  mmovia
+*   Multilanguage System
+*
+*   Revision 1.1  2001/02/23 13:31:44  mmovia
+*   *** empty log message ***
+*
+*
+*
+****************************************************************************/

#define ENVPRODUCT     "PRODUCT"
#define PRODUCT_WAMASK "WAMAS-K"

int me_mlmexp(OWidget w, MLMINOUTCTX *pData, long Anzahl, int Modus);

#endif /* ME_MLMEXP_H_INCLUDED */

