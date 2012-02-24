#if defined _LI_ZAP_C && ! defined __LINT__ && ! defined _LI_ZAP_C_H
#define _LI_ZAP_C_H
	static char VERSID_H [] 
#if defined(__GNUC__)
	__attribute__ ((unused))
#endif
	= "$Header: /cvsrootb/ALPLA/llr/src/term/li_zap.h,v 1.1.1.1 2004/05/28 08:15:13 mwirnsp Exp $$Locker:  $";
#endif
#ifndef _LI_ZAP_H
#define _LI_ZAP_H
/*****************************************************************************
+* PROJECT:   ALPLA/
+* PACKAGE:   Protokolle 
+* FILE:      li_zap.h
+* CONTENTS:  Protokoll Zu- und Abgang 
+* COPYRIGHT NOTICE:
+* 		(c) Copyright 2001 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+* REVISION HISTORY:
+*   $Log: li_zap.h,v $
+*   Revision 1.1.1.1  2004/05/28 08:15:13  mwirnsp
+*   COPIED FROM WAMAS-A/llr/src WK32_LVSWA_005
+*
+****************************************************************************/

/* ==========================================================================
 * INCLUDES
 * =========================================================================*/

#ifdef __cplusplus
extern "C" {
#endif

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

int _li_zap (OWidget parent, Value ud);
void li_zap (MskDialog mask, MskStatic ef, MskElement el,
				int reason, void *cbc);

#ifdef __cplusplus
}
#endif

#endif  /* _LI_ZAP_H */
