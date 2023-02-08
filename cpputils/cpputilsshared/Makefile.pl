
# it is required to install  to the prod directory
if( $main::uname ne 'WINNT') {
	if( $ENV{SRCDIR} || $ENV{EXTERNAL_CPPUTILS} ) {

	&Rulemore('install', 'HEADERFILES', '',
			"\@mkdir -p \$($standard::ENVROOTDIR)/include && cp -f *.h \$($standard::ENVROOTDIR)/include");

	&Rulemore('install', 'LIBS', '',
			"\@mkdir -p \$($standard::ENVROOTDIR)/lib && cp -f \$(LIBRARY) \$($standard::ENVROOTDIR)/lib");

	}

	
	if( !defined ( $ENV{TOOLSBOXDIR} ) &&  ( $ENV{EPREFIX} =~ /2015/ ) ) {

		# on TB-2015 / GCC-4.8: enable C++11 
		# <= TB-2010 : not possible
		# TB-2020 / GCC-8 or GCC-10 : use higher standard

        &main'Macromore('CFLAGSAUX', 'c++11', "-std=gnu++11");

    }
}		

$eof;
