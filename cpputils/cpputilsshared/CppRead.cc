/**
 * Wrapper around CppGet for easier handling
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */

#if __cplusplus >= 201103

#include "CppRead.h"
#include "string_utils.h"

namespace Tools {

CppReadTmpImpl::CppTmpReadExec::CppTmpReadExec( void *tid )
: sql_reader( new CppSqlReaderImpl( FAC_CPPREAD, tid, "" )),
  joins(),
  selection_table_names(),
  from_table_names(),
  from_table_names_alias(),
  used_table_alias(),
  selection_entries(),
  last_selection_table_name(),
  last_selection_table_name_alias()
{
}

CppReadTmpImpl::CppTmpReadExec::~CppTmpReadExec()
{
	/**
	 * should never happen, since execute is called from ~CppReadTmpImpl()
	 */
	if( sql_reader ) {
		delete sql_reader;
		sql_reader = 0;
	}
}

std::string CppReadTmpImpl::CppTmpReadExec::getNewAlias4Table( const std::string & table ) {

	if( used_table_alias.count( table ) == 0 ) {
		return table;
	}

	for( unsigned count = 1; ; count++ ) {
		std::string alias_name = format( "%s_%d", table, count );
		if( used_table_alias.count( alias_name ) == 0 ) {
			return alias_name;
		}
	}
}

CppReadTmpImpl::~CppReadTmpImpl() noexcept(false) {

	/**
	 * if this is the last instance to temp_exec, call execute() if not already called
	 * In case of an error, or SqlNotefound this will throw an exception
	 */
	auto tmp_e = tmp_exec.get();

	if( tmp_exec.use_count() == 1 ) {
		if( tmp_e->isInCharge() ) {
			tmp_e->execute();
		}
	}
}

std::string CppReadTmpImpl::fix_last_join( const std::string & last_join,
										   const std::string & left_table_name,
										   const std::string & left_table_alias,
										   const std::string & right_table_name,
										   const std::string & right_table_alias )
{
	if( last_join.find( " and " ) == std::string::npos ) {
		return fix_simple_join( last_join, left_table_name, left_table_alias, right_table_name, right_table_alias );
	}

	std::vector<std::string> sl = split_and_strip_simple( last_join, " and " );

	std::stringstream str;

	for( unsigned i = 0; i < sl.size(); i++ ) {
		str << fix_simple_join( last_join, left_table_name, left_table_alias, right_table_name, right_table_alias );
	}

	std::string result = str.str();

	LogPrintf( FAC_CPPREAD, LT_DEBUG, "modified join from '%s' to '%s'", last_join, result );

	return result;
}


std::string CppReadTmpImpl::fix_simple_join( const std::string & last_join,
											 const std::string & left_table_name,
											 const std::string & left_table_alias,
											 const std::string & right_table_name,
											 const std::string & right_table_alias )

{
	std::vector<std::string> sl = split_and_strip_simple( last_join, "=", 2 );

	if( sl.size() != 2 ) {
		throw REPORT_EXCEPTION( format( "cannot fix join: '%s'", last_join ) );
	}

	sl[0] = substitude( sl[0], left_table_name, left_table_alias );
	sl[1] = substitude( sl[1], right_table_name, right_table_alias );

	return sl[0] + " = " + sl[1];
}

bool CppReadTmpImpl::CppTmpReadExec::execute( bool exception_on_sqlnot_found )
{
	std::stringstream str;
	str << "select ";

	for( unsigned i = 0; i < selection_table_names.size(); i++  ) {
		if( i > 0 ) {
			str << ", ";
		}

		str << "%" << selection_table_names[i];
	}

	str << " from ";


	for( unsigned i = 0; i < from_table_names.size(); i++  ) {
		if( i > 0 ) {
			str << ", ";
		}

		const std::string & table_name = from_table_names[i];
		const std::string & table_name_alias = from_table_names_alias[i];

		str << table_name;

		if( table_name != table_name_alias ) {
			str << " " << table_name_alias;
		}
	}

	str << "\nwhere ";

	for( unsigned i = 0; i < joins.size(); i++ ) {
		if( i > 0 ) {
			str << " and ";
		}
		str << joins[i].first << "\n";
	}

	sql_reader->appendQuery( str.str() );

	size_t ret;

	try {

		ret = sql_reader->execAll();

		if( ret > 0 ) {
			for( auto one_selection_entry : selection_entries ) {
				one_selection_entry->assignResult();
			}
		}

	} catch( const std::exception & err ) {

		LogPrintf( FAC_CPPREAD, LT_ALERT, "Error: %s", err.what() );

		delete sql_reader;
		sql_reader = 0;

		throw( err );
	}


	delete sql_reader;
	sql_reader = 0;

	if( ret == 0 && exception_on_sqlnot_found ) {
		throw REPORT_EXCEPTION( "no data found" );
	}

	return ret > 0;
}

} // namespace Tools


#endif
