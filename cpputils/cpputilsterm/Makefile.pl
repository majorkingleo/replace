
# this way we can detect if it is a product and only in that case 
# it is required to install cpputils to the prod directory
if( $main::uname ne 'WINNT') {
	if( $ENV{SRCDIR} || $ENV{EXTERNAL_CPPUTILS} ) {

	&Rulemore('install', 'HEADERFILES', '',
			"\@mkdir -p \$($standard::ENVROOTDIR)/include && cp -f *.h \$($standard::ENVROOTDIR)/include");

	&Rulemore('install', 'LIBS', '',
			"\@mkdir -p \$($standard::ENVROOTDIR)/lib && cp -f \$(LIBRARY) \$($standard::ENVROOTDIR)/lib");

	}
}

$eof;
