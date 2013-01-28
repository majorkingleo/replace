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

	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

public:
	static std::string toString( const SEL_CALLBACK & callback );

protected:
	std::string include_primanlist( const std::string & file );

	static SEL_CALLBACK detect_sel_callback( const std::string & file, std::string::size_type & pos );

	static std::string patch_sel_callback( const std::string & file );

	static std::string patch_null_selcallback( const std::string & file, std::string::size_type pos );

	static std::string add_selcallback_to_reasons( const std::string & file, std::string::size_type pos );

	static void strip_argtypes( Function & f );

	static std::string get_varname( const std::string & var );

	static std::string insert_selcallback( const std::string & file );

	static bool is_in_comment( const std::string & file, std::string::size_type pos );
};

#endif /* PRIMANLIST_H_ */
