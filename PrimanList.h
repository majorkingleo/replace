#ifndef PRIMANLIST_H_
#define PRIMANLIST_H_

#include "HandleFile.h"
#include <range.h>

class Function;

class PrimanList : public HandleFile
{
public:
	struct SEL_CALLBACK_STRUCT {
		typedef enum {
			FIRST__,
			AT_POS,
			IS_NULL,
			NOT_FOUND,
			LAST__
		} ETYPE;
	};

	typedef Tools::EnumRange<SEL_CALLBACK_STRUCT> SEL_CALLBACK;

public:
	PrimanList();

	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

public:
	static std::wstring toString( const SEL_CALLBACK & callback );

protected:
	std::wstring include_primanlist( const std::wstring & file );

	static SEL_CALLBACK detect_sel_callback( const std::wstring & file, std::wstring::size_type & pos );

	static std::wstring patch_sel_callback( const std::wstring & file );

	static std::wstring patch_null_selcallback( const std::wstring & file, std::wstring::size_type pos );

	static std::wstring add_selcallback_to_reasons( const std::wstring & file, std::wstring::size_type pos );

	static void strip_argtypes( Function & f );

	static std::wstring get_varname( const std::wstring & var );

	static std::wstring insert_selcallback( const std::wstring & file );

	static bool is_in_comment( const std::wstring & file, std::wstring::size_type pos );
};

#endif /* PRIMANLIST_H_ */
