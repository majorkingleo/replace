
#$Locker:  $

if ($uname eq 'WINNT') {
	&global::AddScript($global::path, 'ap.pl'); 
}else{
  &Rulemore('install', 'DBCHECKCFG', '',
	"\@cp -f dbcheck.cfg \$($standard::ENVROOTDIR)/data/sql");
  &Rulemore('install', 'DBCHECK2CFG', '',
	"\@chmod 777 dbcheck.pl; cp -f dbcheck.pl \$($standard::ENVROOTDIR)/bin");
  &Rulemore('install', 'DBCHECK3CFG', '',
	"\@chmod 777 dbcheck.csh; cp -f dbcheck.csh \$($standard::ENVROOTDIR)/bin");
}

$eof;
