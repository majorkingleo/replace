SUBDIRS=tools cpputils cpputils/cpputilsshared cpputils/cpputilsshared/cpputilsformat
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
		utils.cc 

AM_CPPFLAGS = -I$(top_srcdir)/tools \
	-I$(top_srcdir)/cpputils/cpputilsshared  \
	-I$(top_srcdir)/cpputils/cpputilsshared/cpputilsformat \
	-DNOWAMAS
target_CPPFLAGS = $(FOO_CPPFLAGS) 

replace_LDADD = tools/libtools.a cpputils/cpputilsshared/libcpputilsshared.a cpputils/cpputilsshared/cpputilsformat/libcpputilsformat.a


