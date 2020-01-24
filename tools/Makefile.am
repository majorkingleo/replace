noinst_LIBRARIES=libtools.a
libtools_a_SOURCES= \
	arg.cpp \
	database.cpp \
	db.cpp \
	dbi.cpp \
	debug.cpp \
	mysql_db.cpp \
	odbc_db.cpp \
	oracle_db.cpp \
	pairs.cpp \
	theme.cpp \
	thread.cpp

AM_CPPFLAGS = -I$(top_srcdir)/tools \
    -I$(top_srcdir)/cpputils/cpputilsshared  \
    -I$(top_srcdir)/cpputils/cpputilsshared/cpputilsformat \
    -DNOWAMAS \
	-I$(top_srcdir)
target_CPPFLAGS = $(FOO_CPPFLAGS)

