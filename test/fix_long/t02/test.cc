int
StackerNextVpl(int KpIdx, char *pAuftrNr, int *pAuftrIdx, int RNr)
{
	int			rv;

	LogPrintf(getAktFac(KpIdx), LT_ALERT, "---------------");

	rv = __StackerNextVpl(KpIdx, pAuftrNr, pAuftrIdx, RNr);

	if (pAuftrNr[0] != '\0')
		LogPrintf(getAktFac(KpIdx), LT_ALERT,
			"StackerNextVpl        (%s, %s/%02d, %d) rv = %d",
			KpIdx2Str(&KpIdx, NULL),
			pAuftrNr, *pAuftrIdx, RNr, rv);
	else
		LogPrintf(getAktFac(KpIdx), LT_ALERT,
			"StackerNextVpl        (%s, __________/__, %d) rv = %d",
			KpIdx2Str(&KpIdx, NULL), RNr, rv);

	return rv;
}
