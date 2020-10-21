bin_PROGRAMS=replace

replace_SOURCES=\
		add_wdgassign.cc\
		argenumtype.cc\
		colored_output.cc\
		correct_va_multiple_malloc.cc\
		file_option.cc\
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
		OutDebug.cc\
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
		fix_c_headers.cc

AM_CPPFLAGS = -I$(top_srcdir)/tools \
	-I$(top_srcdir)/cpputils/cpputilsshared  \
	-I$(top_srcdir)/cpputils/cpputilsshared/cpputilsformat \
	-DNOWAMAS \
	-std=c++11

noinst_LIBRARIES=tools/libtools.a \
	cpputils/cpputilsshared/libcpputilsshared.a \
	cpputils/cpputilsshared/cpputilsformat/libcpputilsformat.a

tools_libtools_a_SOURCES= \
	tools/arg.cpp \
	tools/database.cpp \
	tools/db.cpp \
	tools/dbi.cpp \
	tools/debug.cpp \
	tools/mysql_db.cpp \
	tools/odbc_db.cpp \
	tools/oracle_db.cpp \
	tools/pairs.cpp \
	tools/theme.cpp \
	tools/thread.cpp


cpputils_cpputilsshared_libcpputilsshared_a_SOURCES= \
		cpputils/cpputilsshared/aes_crypt_util.cc \
		cpputils/cpputilsshared/backtrace.cc \
		cpputils/cpputilsshared/blockupdate.cc \
		cpputils/cpputilsshared/cppdir.cc \
		cpputils/cpputilsshared/cpp_util.cc \
		cpputils/cpputilsshared/leoini.cc \
		cpputils/cpputilsshared/logtool2.cc \
		cpputils/cpputilsshared/string_utils.cc \
		cpputils/cpputilsshared/vec_utils.cc \
		cpputils/cpputilsshared/xml.cc


cpputils_cpputilsshared_cpputilsformat_libcpputilsformat_a_SOURCES= \
	cpputils/cpputilsshared/cpputilsformat/format1.cc \
	cpputils/cpputilsshared/cpputilsformat/format2.cc

replace_LDADD = tools/libtools.a cpputils/cpputilsshared/libcpputilsshared.a cpputils/cpputilsshared/cpputilsformat/libcpputilsformat.a
