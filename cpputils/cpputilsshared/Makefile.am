SUBDIRS=cpputilsformat

noinst_LIBRARIES=libcpputilsshared.a
libcpputilsshared_a_SOURCES= \
		aes_crypt_util.cc \
		backtrace.cc \
		blockupdate.cc \
		cppdir.cc \
		cpp_util.cc \
		leoini.cc \
		logtool2.cc \
		string_utils.cc \
		vec_utils.cc \
		xml.cc

AM_CPPFLAGS = \
    -I$(top_srcdir)/cpputils/cpputilsshared  \
    -I$(top_srcdir)/cpputils/cpputilsshared/cpputilsformat \
    -DNOWAMAS \
    -I$(top_srcdir)
target_CPPFLAGS = $(FOO_CPPFLAGS)

