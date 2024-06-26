bin_PROGRAMS=replace

replace_SOURCES=\
		add_wdgassign.cc\
		argenumtype.cc\
		correct_va_multiple_malloc.cc\
		find_decl.cc\
		find_files.cc\
		find_first_of.cc\
		fix_from_compile_log.cc\
		fix_mlm.cc\
		fix_prmget.cc\
		fix_sprintf.cc\
		fix_scoped_c_str.cc\
		fix_StrForm.cc\
		format_string_handler.cc\
		format_string_handler_gcc48.cc\
		get_file_type.cc\
		getline.cc\
		HandleFile.cc\
		implicit_handler2.cc\
		implicit_handler.cc\
		main.cc\
		OwCallback1.cc\
		PrimanList.cc\
		remove_generic_cast.cc\
		remove_versid_ch.cc\
		remove_versid_class.cc\
		remove_versid_pdl.cc\
		remove_versid_pl.cc\
		remove_versid_rc.cc\
		RestoreShell.cc\
		space_between_literal_handler.cc\
		uninitialized_variable_handler.cc\
		unused_variable_handler.cc\
		utils.cc \
		fix_c_headers.cc \
		add_cast.cc \
		fix_conversion_null.cc \
		add_mlm.cc \
		add_mlm_wbox.cc \
		read_file.h \
		read_file.cc \
		regex_match.h \
		regex_match.cc \
		reset_versid.cc \
		reset_versid.h

AM_CPPFLAGS = -I$(top_srcdir)/cpputils/io \
	-I$(top_srcdir)/cpputils/cpputilsshared  \
	-I$(top_srcdir)/cpputils/cpputilsshared/cpputilsformat \
	-DNOWAMAS \
	-std=c++20 \
	-D_XOPEN_SOURCE=700 # for cyggwin fileno()

noinst_LIBRARIES=cpputils/io/libio.a \
	cpputils/cpputilsshared/libcpputilsshared.a \
	cpputils/cpputilsshared/cpputilsformat/libcpputilsformat.a


cpputils_cpputilsshared_libcpputilsshared_a_SOURCES= \
		cpputils/cpputilsshared/arg.cc \
		cpputils/cpputilsshared/arg.h \
		cpputils/cpputilsshared/stderr_exception.h \
		cpputils/cpputilsshared/stderr_exception.cc \
		cpputils/cpputilsshared/backtrace.cc \
		cpputils/cpputilsshared/cppdir.cc \
		cpputils/cpputilsshared/leoini.cc \
		cpputils/cpputilsshared/string_utils.cc \
		cpputils/cpputilsshared/xml.h \
		cpputils/cpputilsshared/xml.cc \
		cpputils/cpputilsshared/pairs.h \
		cpputils/cpputilsshared/utf8.h \
		cpputils/cpputilsshared/utf8/core.h \
		cpputils/cpputilsshared/utf8/checked.h \
		cpputils/cpputilsshared/utf8/unchecked.h \
		cpputils/cpputilsshared/utf8/cpp11.h \
		cpputils/cpputilsshared/utf8/cpp17.h \
		cpputils/cpputilsshared/utf8_util.h \
		cpputils/cpputilsshared/utf8_util.cc \
		cpputils/cpputilsshared/file_option.cc \
		cpputils/cpputilsshared/file_option.h


cpputils_cpputilsshared_cpputilsformat_libcpputilsformat_a_SOURCES= \
	cpputils/cpputilsshared/cpputilsformat/format1.cc \
	cpputils/cpputilsshared/cpputilsformat/format2.cc \
	cpputils/cpputilsshared/cpputilsformat/format2.h \
	cpputils/cpputilsshared/cpputilsformat/format1.h \
	cpputils/cpputilsshared/cpputilsformat/cformat.h \
	cpputils/cpputilsshared/cpputilsformat/wformat2.cc \
	cpputils/cpputilsshared/cpputilsformat/wformat2.h \
	cpputils/cpputilsshared/cpputilsformat/cwformat.h \
	cpputils/cpputilsshared/cpputilsformat/cwformat.cc
	
cpputils_io_libio_a_SOURCES= \
		cpputils/io/ColoredOutput.cc \
		cpputils/io/ColoredOutput.h \
		cpputils/io/ConvertEncoding.cc \
		cpputils/io/ConvertEncoding.h \
		cpputils/io/CpputilsDebug.cc \
		cpputils/io/CpputilsDebug.h \
		cpputils/io/DetectLocale.cc \
		cpputils/io/DetectLocale.h \
		cpputils/io/OutDebug.cc \
		cpputils/io/OutDebug.h \
		cpputils/io/read_file.h \
		cpputils/io/read_file.cc

replace_LDADD = cpputils/io/libio.a cpputils/cpputilsshared/libcpputilsshared.a cpputils/cpputilsshared/cpputilsformat/libcpputilsformat.a

