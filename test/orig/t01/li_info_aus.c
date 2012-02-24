#if defined _LI_INFO_AUS_C && ! defined _LI_INFO_AUS_C_H
#define _LI_INFO_AUS_C_H
#include <versid.h>
VERSIDH(li_info_aus, "$Header: /cvsrootb/ALPLA/llr/src/prewmp/src/menus/li_info_aus.h,v 1.1.1.1 2004/05/28 08:15:12 mwirnsp Exp $$Locker:  $")
#endif
#ifndef _LI_INFO_AUS_H
#define _LI_INFO_AUS_H
/****************************************************************************
+* PROJECT:   ALPLA/
+* PACKAGE:   
+* FILE:      li_info_aus.h
+* CONTENTS:  INFOUSER: Header Liste Auslagerauftrag
+* COPYRIGHT NOTICE:
+* 		(c) Copyright 2001 by 
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Stuebing
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+* 
+****************************************************************************/

/* ==========================================================================
 * INCLUDES
 * =========================================================================*/

#include "menufct.h"

/* ==========================================================================
 * GLOBAL variables and Function-Prototypes
 * =========================================================================*/

int _li_info_aus (OWidget parent, Value ud);
MENUFCT li_info_aus (MskDialog mask, MskStatic ef, MskElement el,
				int reason, void *cbc);

#endif  /* _LI_INFO_AUS_H */
