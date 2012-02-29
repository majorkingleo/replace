#if !defined(__lint) && !defined(__LINT__) && !defined(lint)
/*****************************************************************************
+*/ static char VERSID []
#if defined(__GNUC__)
	__attribute__ ((unused))
#endif
	= "$Header: /cvsroot/PISTOR-KCL/llr/src/rbg/lrbgkernel/lrbg_bs.c,v 1.5 2001/11/30 09:33:27 matl Exp $$Locker:  $";
#endif 
/*
+* PROJECT:   WAMAS-A
+* PACKAGE:   RBG
+* FILE:      lrbg_bs.c
+* CONTENTS:  Functions for sending/evaluating block-control-data
+* COPYRIGHT NOTICE:
+*         (c) Copyright 2000 by
+*                 Salomon Automationstechnik Ges.m.b.H
+*                 Friesachstrasse 15
+*                 A-8114 Friesach bei Graz
+*                 Tel.: ++43 3127 200-0
+*                 Fax.: ++43 3127 200-22
+* REVISION HISTORY:
+*   01-10-2001  CREATED BY matl
+*
+*   $Log: lrbg_bs.c,v $
+*   Revision 1.5  2001/11/30 09:33:27  matl
+*   Documentation
+*
+*   Revision 1.4  2001/11/08 16:59:49  matl
+*   SunOs-port
+*
+*   Revision 1.3  2001/11/02 16:17:57  matl
+*   File renamed from rbg_xxx to lrbg_xxx and overworked
+*
+*   Revision 1.2  2001/10/31 16:14:01  matl
+*   SetBgAction combined with MlMpack
+*
+*   Revision 1.1  2001/10/05 14:37:10  matl
+*   Initial revision
+*
+****************************************************************************/

/* ===========================================================================
 * INCLUDES
 * =========================================================================*/

/* ------- System-Headers ------------------------------------------------- */

/* ------- Tools-Headers -------------------------------------------------- */
#include <logtool.h>

#include <sbgact.h>
#include <mlpack.h>

/* ------- Local-Headers -------------------------------------------------- */
#include "lrbg_func.h"
#include "lrbg_desc.h"

#define _LRBG_BS_C
#include "lrbg_bs.h"
#undef _LRBG_BS_C

#ifdef __cplusplus
extern "C" {
#endif


/* ===========================================================================
 * GLOBAL (PUBLIC) Functions
 * =========================================================================*/


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Blocksteuerung initialisieren
-*     (Speicheranforderung, Kommunikationsaufbau)
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsInit (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG
	if (gptBsDesc->iBsConnected == 0 &&
		gptBsDesc->iBsCloseConn == 0) {

		BgActSend( gptRbgDesc->pcBgActName,
					MlMpack("Connecting to BS...", 0, ""));

		if (BsInit() == 0) {
			gptRbgDesc->xBsReset();
			gptBsDesc->iBsConnected = 1;
		} else {
			gptRbgDesc->xBsExit();
			BgActSend( gptRbgDesc->pcBgActName,
						MlMpack("BS not available", 0, ""));
		}
	}
#endif /* BLOCKSTEUERUNG */

	return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Blocksteuerung beenden (Speicherfreigabe, Kommunikationsabbruch)
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsExit (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG
	if (gptBsDesc->iBsCloseConn == 1) {

		if (gptBsDesc->iBsConnected == 1) {
			BsExit();
		}
		gptBsDesc->iBsConnected = 0;
		gptBsDesc->iBsCloseConn = 0;
		BgActSend( gptRbgDesc->pcBgActName,
					MlMpack("BS-connection closed", 0, ""));
	}
#endif /* BLOCKSTEUERUNG */

	return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Blocksteuerung rücksetzen (beim TR-Start bzw. BS-Reconnect)
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsReset (void)
{
	int		nI;

	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

	for (nI = 0; nI < gptRbgDesc->nMaxOrder; nI++) {
		gptRbgCtxt->rcAb[nI].abBsStime = 0;
		gptRbgCtxt->rcAb[nI].abBsQtime = 0;
	}

	/* miFirstSetState = 1; */
	CF_TRIGGER( gptRbgDesc->pcProcName, FUNC_RBG_BSSTATE, 0, "RbgBsReset" );
	return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Statusinfo (TPM- und Gassensperre) an die Blocksteuerung senden
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsSendState (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG

	static long	slSperre;

	if (gptBsDesc->iBsConnected == 0) {
		return (0)
	}

	/*
	 * Wenn sich nichts geändert hat, dann gleich wieder aussteigen
	 */
	if (miFirstSetState == 0 &&
		slSperre == gptRbgCtxt->rcSperre) {
		return (0);
	}

	/*
	 * Aktuellen Zustand speichern
	 */
	miFirstSetState = 0;
	slSperre = gptRbgCtxt->rcSperre;

	/*
	 * Transportmittel-Status senden
	 */
	BsSendTpmState(	gptBsDesc,				/* BS-Descriptor */
					gptRbgCtxt->rcSperre,	/* Sperrflag 1/0 */
					gptBsDesc->ucTpmNr,		/* Tpm-Nummer    */
					1,						/* Von Tpm-Block */
					1 );					/* Bis Tpm-Block */

#endif /* BLOCKSTEUERUNG */

	return(0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Istposition(en) aller LAM an die Blocksteuerung melden
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsSendIstPos (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG
	int			nI;
	BS_POS		tBsPos;

	if (gptBsDesc->iBsConnected == 0) {
		return (0)
	}

	TrPos2BsPos(gptBsDesc,
				&gptRbgCtxt->rcLam[RBG_LAM1].lamAktpos,
				&gptRbgCtxt->rcLam[RBG_LAM1].lamTet,
				&tBsPos);

	LogPrintf (gptBsDesc->pcLogFacBs, LT_DEBUG,
		"IstPos LAM1: %d-%d", tBsPos.ucGasse, tBsPos.urSpalte);

	/*
	 * Ausgehend von LAM1 werden alle Blöcke geschickt
	 */
	for (nI = 0; nI < gptBsDesc->nTpmBlocks; nI ++) {

		BsSetPos( gptBsDesc, nI, &tBsPos );
		tBsPos.urSpalte ++;
	}

	BsSendPos( gptBsDesc );
#endif /* BLOCKSTEUERUNG */

	return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Transportaufträge (Anfrage für Wegfreigabe oder Löschtelegramm)
-*     an die Blocksteuerung senden
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsSendTpa (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG
	BS_AUFTRAG		atBsAuftr[gptRbgDesc->nMaxOrder];
	int				nBsAuftr;

	if (gptBsDesc->iBsConnected == 0) {
		return (0)
	}

	nBsAuftr = Auftr2DelExist( atBsAuftr );
	if (nBsAuftr > 0) {
		BsSendTpaDel( &gptBsDesc-> nBsAuftr, atBsAuftr );
		return (0);
	}

	nBsAuftr = Auftr2AskExist( atBsAuftr );
	if (nBsAuftr > 0) {
		BsSendTpaReq( &gptBsDesc-> nBsAuftr, atBsAuftr );
	}
#endif /* BLOCKSTEUERUNG */

	return (0);
}


/*----------------------------------------------------------------------------
-* SYNOPSIS
-*     $$
-* DESCRIPTION
-*     Transportauftrags-Quittierung von der Blocksteuerung auswerten
-* RETURNS
-*      0 ... OK
-*     -1 ... Fehler
-*--------------------------------------------------------------------------*/

int _RbgBsReceiveTpa (void)
{
	if (gptRbgDesc->eUseBs == RBGJN_NEIN) {
		return (0);
	}

#ifdef BLOCKSTEUERUNG
	BS_AUFTRAG		tBsAuftr;
	BS_STATUS		eBsStatus;
	int				iRv;

	if (gptBsDesc->iBsConnected == 0) {
		return (0)
	}

	iRv = BsReceive( gptBsDesc, &tBsAuftr, &eBsStatus );

	if (iRv < 0) {
		if (gptBsDesc->iBsCloseConn == 1) {
			CF_TRIGGER(gptRbgDesc->pcProcName, FUNC_RBG_BSEXIT, 0, "BsReceive");
		}
		return(-1);
	}

	if (gptBsDesc->iBsReconnect == 1) {
		gptBsDesc->iBsReconnect = 0;
		gptRbgDesc->xBsReset();
	}

	switch( eBsStatus ) {

	case BS_STATUS_NONE:
		/*
		 * Entweder keine TPA-Quittierung, oder alles bleibt beim alten
		 */
		break;

	case BS_STATUS_STOP:
		/*
		 * Falls das RBG fährt, dann die Schritte rausholzen
		 */
		HandleStatusStop( &tBsAuftr );
		break;

	case BS_STATUS_NOTSTOP:
		/*
		 * Hut brennt: alle Schritte rausholzen !
		 */
		HandleStatusNotstop( &tBsAuftr );
		break;

	case BS_STATUS_OK:
		/*
		 * Wegstrecke wurde von der BS freigegeben
		 */
		HandleStatusOk( &tBsAuftr );
		break;
	}
#endif /* BLOCKSTEUERUNG */

	return (0);
}


#ifdef __cplusplus
}
#endif
