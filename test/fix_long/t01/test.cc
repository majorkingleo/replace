
int StackerNextAuftr(int KpIdx, char *pAuftrNr, int *pAuftrIdx)
{
	int			rv;
	int			doLog;
	static int	IdleCnt = 1;
	SqlContext	*LocalContext;

	if (IdleCnt < 3 || IdleCnt % 30 == 0)
		doLog = 1;
	else
		doLog = 0;

	if (doLog != 0)
		LogPrintf(getAktFac(KpIdx), LT_ALERT, "---------------");

	LocalContext = _SqlNewContext(NULL);
	if (LocalContext == NULL) {
		logPrintf(9, "SqlNewContext: %d", SqlError);
		return -1;
	}
	rv = _StackerNextAuftr(KpIdx, pAuftrNr, pAuftrIdx, LocalContext);

	_SqlDestroyContext(LocalContext);

	if (pAuftrNr[0] != '\0') {
		IdleCnt = 0;
		LogPrintf(getAktFac(KpIdx), LT_ALERT,
			"StackerNextAuftr      (%s, %s/%02d) rv = %d",
			KpIdx2Str(&KpIdx, NULL),
			pAuftrNr, *pAuftrIdx, rv);
	}
	else {
		if (doLog != 0) {
			if (IdleCnt < 2) {
				LogPrintf(getAktFac(KpIdx), LT_ALERT,
					"StackerNextAuftr      (%s, __________/__) rv = %d",
					KpIdx2Str(&KpIdx, NULL), rv);
			}
			else {
				LogPrintf(getAktFac(KpIdx), LT_ALERT,
					"StackerNextAuftr       IDLE ...");
			}
		}
		++IdleCnt;
	}
	return rv;
}

