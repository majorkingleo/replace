/**
 * Implementation of CppGet
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */
#if __cplusplus >= 201103

#include "CppGet.h"
#include <cpp_util.h>
#include <dbdesc.h>
#include <CppSqlReader.h>
#include <string_utils.h>
#include <set>

#include <boost/version.hpp>

#if (BOOST_VERSION >= 107000 )
# include <boost/core/demangle.hpp>
#endif

#ifdef __GNUC__
#include <cxxabi.h>
#endif

namespace {
	class CppExecSqlArgs
	{
		SqlTexecArgsPtr execArgs;

	public:
		CppExecSqlArgs( SqlTexecArgsPtr execArgs_ )
		: execArgs( execArgs_ )
		{}

		CppExecSqlArgs( const CppExecSqlArgs & other ) = delete;
		CppExecSqlArgs & operator=( const CppExecSqlArgs & other ) = delete;

		operator SqlTexecArgsPtr() {
			return execArgs;
		}

		~CppExecSqlArgs() {
			SqlExecArgsDestroy(execArgs);
			execArgs = NULL;
		}
	};
}

namespace Tools {

const char *CppGetImpl::pcSourceFile = "";
int CppGetImpl::iSourceLine = 0;

void CppGetImpl::setSourceLocation( const char *pcSource, int iLine )
{
	pcSourceFile = pcSource;
	iSourceLine = iLine;
}

CppGetImpl::CppGetImpl( const std::string & fac_ )
: fac( fac_ ),
  desc_by_struct_name(),
  desc_by_table_name()
{
	initDbDesc();
}

CppGetImpl::~CppGetImpl()
{

}

CppGetImpl & CppGetImpl::instance() {
	static CppGetImpl cppget;

	return cppget;
}

void CppGetImpl::initDbDesc()
{
	for( SqlTableDesc *ptr = SqlDesc; ptr->strucName != NULL; ptr++ ) {
		desc_by_struct_name[ptr->strucName] = ptr;
		desc_by_table_name[ptr->tableName] = ptr;
	}
}

#ifdef __GNUC__
std::string CppGetImpl::gccDemangle(const char* name)
{
	char buf[1024] = {};
	size_t size = sizeof(buf);
	int status;
	// todo:
	char* res = abi::__cxa_demangle (name,
			buf,
			&size,
			&status);
	buf[sizeof(buf) - 1] = 0; // I'd hope __cxa_demangle does this when the name is huge, but just in case.
	return res;
}
#endif

std::string CppGetImpl::getDemangledName( const std::string & name )
{
	std::string table_name;

#if (BOOST_VERSION >= 107000 )
	table_name = boost::core::demangle( name.c_str() );
#elif __GNUC__
	table_name = gccDemangle( name.c_str() );
#else
#   error "unsupported compiler version, please fix me!!!"
#endif

#ifdef _MSC_VER
	// MSCV demangled name will be "struct _TPA"
	static const std::string STRUCT_PREFIX="struct ";
	if( table_name.find( STRUCT_PREFIX ) == 0 ) {
		table_name = table_name.substr( STRUCT_PREFIX.size() );
	}
#endif

	return table_name;
}

std::string CppGetImpl::getStructNameFromTypeId( const std::string & str_typeid )
{
	std::string table_name = getDemangledName( str_typeid );

	// pdl generated tables names 'struct _TPA', so remove the leading '_'
	if( table_name.find('_') == 0 && table_name.size() > 1 ) {
		table_name = table_name.substr(1);
	}

	return table_name;
}

SqlTableDesc* CppGetImpl::getSqlTableDescByStructName( const std::string & struct_name )
{
	auto it = desc_by_struct_name.find( struct_name );
	if( it == desc_by_struct_name.end() ) {
		return NULL;
	}

	return it->second;
}

SqlTableDesc* CppGetImpl::getSqlTableDescByTableName( const std::string & struct_name )
{
	auto it = desc_by_table_name.find( struct_name );
	if( it == desc_by_table_name.end() ) {
		return NULL;
	}

	return it->second;
}

void CppGetImpl::bindPk( std::string & sql, SqlTableDesc *descSource, const void *pvSource, SqlTexecArgsPtr execArgs, bool sql_with_and )
{
	std::vector<std::string> primary_key_list = split_and_strip_simple( descSource->primKey, ", \t" );
	std::set<std::string> primary_keys;

	if( !primary_key_list.empty() )
	{
		primary_keys.insert( primary_key_list.begin(), primary_key_list.end() );
	}

	BindList bindList;

	memset( &bindList, 0, sizeof( bindList ) );
	bindList.len = -1;
	bindList.list = descSource->strucBind;
	_SqlStartBindDescWalk( &bindList );
	BindDesc *pBd;
	unsigned pkPartsFound = 0;

	while( ((pBd = _TSqlGetNextBindDesc( NULL, &bindList, 1, 1, NULL ) ) != NULL ) &&
			( pkPartsFound < primary_key_list.size() ) )
	{
		if( primary_keys.find( pBd->name ) !=  primary_keys.end() ) {

			char *ptrPkValue = (char*)pvSource;
			ptrPkValue += pBd->location.offset;

			int rv = 0;

			switch( pBd->type->baseType )
			{
			    case BASE_STRING:
			    case BASE_TEXT:
			    case BASE_STRDATE:
			    case BASE_STRTIME:
			    case BASE_VARSTRING:
			    	// In case of an enum, we have to use the converstion function.
			    	// In that case a real string field is indicated, when baseCSize == -1
			    	if( pBd->type->baseCSize == -1 ) {
			    		rv = _SqlExecArgsAppend( execArgs,
			    				pBd->type,
			    				_BIND,
			    				ptrPkValue,
			    				(-1),
			    				NULL );
			    		break;
			    	}
			    	// fallthrough

			    default:
			    	rv = _SqlExecArgsAppend( execArgs,
			    						pBd->type,
			    						_BIND,
			    						ptrPkValue,
			    						NULL );
			    	break;
			}


			if( rv < 0 ) {
				THROW_CPPGET_REPORT_EXCEPTION( "Cannot create execArgs." );
			}

			std::string col_name = pBd->name;

			if( pBd->colName != NULL ) {
				col_name = pBd->colName;
			}

			if( descSource->tableName != NULL ) {
				col_name = std::string(descSource->tableName) + "." + col_name;
			}

			if( sql_with_and ||
			    pkPartsFound > 0 ) { // add an and if primary key is a combined one
				sql += " and ";
			}

			sql += format( " %s = :%s", col_name, pBd->name );

			pkPartsFound++;
		}
	}
}

void CppGetImpl::get( void *tid,
					  void *pvTarget,
					  const void *pvSource,
					  const std::string & source_struct_name,
					  const std::string & source_table_name,
					  const std::string & target_struct_name,
					  const std::string & target_table_name,
					  const std::string & join_query )
{
	std::string sql = format( "select %%%s from %s, %s where %s",
							  target_struct_name,
							  target_table_name,
							  source_table_name,
							  join_query );

	SqlTableDesc *descSource = getSqlTableDescByTableName( source_table_name );

	if( !descSource ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_name ) );
	}

	CppExecSqlArgs execArgs( DbSqlTidCreateSqlExecArgs(const_cast<void*>(tid)) );

	if( !execArgs ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "Cannot create execArgs. SqlError: %s",  TSqlErrTxt(tid) ) );
	}

	if( SqlExecArgsSetArrayLen(execArgs, static_cast<int>(1), 0) < 0 ) {
		THROW_CPPGET_REPORT_EXCEPTION(  format("Cannot set array length to %d", 1 ) );
	}

	int rv = _SqlExecArgsAppend( execArgs,
								 SELSTRUCT( target_struct_name.c_str(), *(char*)pvTarget ),
								 NULL );

	if( rv < 0 ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "Cannot create execArgs for '%s'. SqlError: %s",  target_struct_name, TSqlErrTxt(tid) ) );
	}

	bindPk( sql, descSource, pvSource, execArgs, true );

	rv = _TExecSqlArgsV(tid, NULL, sql.c_str(), execArgs );

	if( rv < 0 ) {
		LogPrintf( fac, LT_ALERT, "sql: %s %s", sql, TSqlErrTxt(tid) );
		throw CPPGET_SQL_EXCEPTION( tid );
	} else {
		LogPrintf( fac, LT_TRACE, "sql: %s", sql );
	}
}

std::string CppGetImpl::getJoin( const std::string & table_a,
								 const std::string & table_a_alias,
								 const std::string & table_b,
								 const std::string & table_b_alias,
								 std::vector<std::string> & table_a_fields,
								 std::vector<std::string> & table_b_fields,
								 bool no_exception )
{
	SqlTableDesc *descTableA = getSqlTableDescByTableName( table_a );

	if( !descTableA ) {
		throw REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", table_a ));
	}

	std::string join;

	std::string fk_table_a;
	std::string fk_or_pk_table_b;

/*
	if( !table_a_fields.empty() ) {
		LogPrintf( fac, LT_TRACE, "%s: %s table_a_fields: %s",
				   __FUNCTION__,
				   table_a, IterableToCommaSeparatedString( table_a_fields ) );

		LogPrintf( fac, LT_TRACE, "%s: %s table_b_fields: %s",
				   __FUNCTION__,
				   table_b, IterableToCommaSeparatedString( table_b_fields ) );
	}
*/

	for( SqlTforkeyPtr fk = descTableA->foreignKeyConstraints; fk != NULL && fk->fkConstraintName != NULL; fk++ ) {
		if( fk->fkForeignTableName == table_b ) {
			fk_table_a = fk->fkFields;
			auto table_a_fields_data = split_and_strip_simple( fk_table_a, ", \t" );

			if( table_a_fields.empty() ) {
				table_a_fields = table_a_fields_data;
			} else {
				// wrong FK, search for next one
				if( table_a_fields_data != table_a_fields ) {
					continue;
				}
			}

			if( fk->fkForeignFields != NULL ) {
				fk_or_pk_table_b = fk->fkForeignFields;
				table_b_fields = split_and_strip_simple( fk_or_pk_table_b, ", \t" );
			}

			break;
		}
	}

	// fill with PK
	if( table_b_fields.empty() ) {
		SqlTableDesc *descTableB = getSqlTableDescByTableName( table_b );

		if( !descTableB ) {
			THROW_CPPGET_REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", table_b ));
		}

		fk_or_pk_table_b = descTableB->primKey;
		table_b_fields = split_and_strip_simple( fk_or_pk_table_b, ", \t" );
	}

	if( table_a_fields.empty() ) {
		if( no_exception ) {
			LogPrintf( fac, LT_DEBUG, "%s: couldn't find a foreign key to join table '%s' with table '%s'",
					   __FUNCTION__,
					   table_a,
					   table_b );

			return std::string();
		} else {
			THROW_CPPGET_REPORT_EXCEPTION( format( "couldn't find a foreign key to join table '%s' with table '%s'",
													table_a, table_b ) );
		}
	}

	if( table_b_fields.empty() ) {
		if( no_exception ) {
			LogPrintf( fac, LT_DEBUG, "%s: couldn't find a primary key for table '%s'",
					   __FUNCTION__,
					   table_b );

			return std::string();
		} else {
			THROW_CPPGET_REPORT_EXCEPTION( format( "couldn't find a primary key for table '%s'", table_b ) );
		}
	}

	if( table_a_fields.size() != table_b_fields.size() ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "invalid FK sizes %d != %d ( '%s' != '%s' )",
											   table_a_fields.size(),
											   table_b_fields.size(),
											   fk_table_a,
											   fk_or_pk_table_b ));
	}

	for( unsigned i = 0; i < table_a_fields.size(); i++ )  {
		if( i > 0 ) {
			join += " and ";
		}

		join += table_a_alias + "." + table_a_fields[i] + " = " + table_b_alias + "." +  table_b_fields[i] + " ";
	}

	LogPrintf( fac, LT_DEBUG, "%s: found join: %s", __FUNCTION__, join );

	return join;
}

void CppGetImpl::bindKey( std::string & sql,
						 SqlTableDesc *descSource,
						 const void *pvSource,
						 const std::string & target_table_name,
						 const std::vector<std::string> & table_source_fields,
						 const std::vector<std::string> & table_target_fields,
						 SqlTexecArgsPtr execArgs,
						 bool sql_with_and )
{
	std::map<std::string,std::pair<std::string,std::string>> keys;

	for( unsigned i = 0; i < table_source_fields.size(); i++ ) {
		keys[table_source_fields[i]] = std::make_pair( table_source_fields[i], table_target_fields[i] );
	}

	BindList bindList;

	memset( &bindList, 0, sizeof( bindList ) );
	bindList.len = -1;
	bindList.list = descSource->strucBind;
	_SqlStartBindDescWalk( &bindList );
	BindDesc *pBd;
	unsigned keyPartsFound = 0;

	while( ((pBd = _TSqlGetNextBindDesc( NULL, &bindList, 1, 1, NULL ) ) != NULL ) &&
			( keyPartsFound < keys.size() ) )
	{
		if( keys.find( pBd->name ) !=  keys.end() ) {

			char *ptrPkValue = (char*)pvSource;
			ptrPkValue += pBd->location.offset;

			int rv = 0;

			switch( pBd->type->baseType )
			{
			    case BASE_STRING:
			    case BASE_TEXT:
			    case BASE_STRDATE:
			    case BASE_STRTIME:
			    case BASE_VARSTRING:

			    	// In case of an enum, we have to use the converstion function.
			    	// In that case a real string field is indicated, when baseCSize == -1
			    	if( pBd->type->baseCSize == -1 ) {
			    		rv = _SqlExecArgsAppend( execArgs,
			    				pBd->type,
			    				_BIND,
			    				ptrPkValue,
			    				(-1),
			    				NULL );
				    	break;
			    	}
			    	// fallthrough

			    default:
			    	rv = _SqlExecArgsAppend( execArgs,
			    						pBd->type,
			    						_BIND,
			    						ptrPkValue,
			    						NULL );
			    	break;
			}


			if( rv < 0 ) {
				THROW_CPPGET_REPORT_EXCEPTION( "Cannot create execArgs." );
			}

			std::string col_name = target_table_name + "." + keys[pBd->name].second;

			if( sql_with_and ||
			    keyPartsFound > 0 ) { // add an and if primary key is a combined one
				sql += " and ";
			}

			sql += format( " %s = :%s", col_name, pBd->name );

			keyPartsFound++;
		}
	}
}

void CppGetImpl::getFromData( void *tid,
							  void *pvTarget,
							  const void *pvSource,
							  const std::string & source_struct_name,
							  const std::string & source_table_name,
							  const std::string & target_struct_name,
							  const std::string & target_table_name,
							  const std::vector<std::string> & table_source_fields,
							  const std::vector<std::string> & table_target_fields,
							  const std::string & join_query )
{
	std::string sql = format( "select %%%s from %s where %s",
							  target_struct_name,
							  target_table_name,
							  join_query );

	SqlTableDesc *descSource = getSqlTableDescByTableName( source_table_name );

	if( !descSource ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_name ) );
	}

	CppExecSqlArgs execArgs( DbSqlTidCreateSqlExecArgs(const_cast<void*>(tid)) );

	if( !execArgs ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "Cannot create execArgs. SqlError: %s",  TSqlErrTxt(tid) ) );
	}

	if( SqlExecArgsSetArrayLen(execArgs, static_cast<int>(1), 0) < 0 ) {
		THROW_CPPGET_REPORT_EXCEPTION(  format("Cannot set array length to %d", 1 ) );
	}

	int rv = _SqlExecArgsAppend( execArgs,
								 SELSTRUCT( target_struct_name.c_str(), *(char*)pvTarget ),
								 NULL );

	if( rv < 0 ) {
		THROW_CPPGET_REPORT_EXCEPTION( format( "Cannot create execArgs for '%s'. SqlError: %s",  target_struct_name, TSqlErrTxt(tid) ) );
	}

	bool add_and = false;

	if( !join_query.empty() ) {
		add_and = true;
	}

	bindKey( sql, descSource, pvSource, target_table_name, table_source_fields, table_target_fields, execArgs, add_and );

	rv = _TExecSqlArgsV(tid, NULL, sql.c_str(), execArgs );

	if( rv < 0 ) {
		LogPrintf( fac, LT_ALERT, "%s: sql: %s %s", __FUNCTION__, sql, TSqlErrTxt(tid) );
		throw CPPGET_SQL_EXCEPTION( tid );
	} else {
		LogPrintf( fac, LT_TRACE, "%s: sql: %s", __FUNCTION__, sql );
	}
}

std::vector<std::string> CppGetImpl::getFkFromTCNNames( const std::string & tcn_name )
{
	std::vector<std::string> ret;

	// seperate combined foreign key
	std::vector<std::string> sl = split_and_strip_simple( tcn_name, ", \t" );

	for( const std::string & s : sl ) {
		std::string::size_type pos = s.find( '.' );

		if( pos == std::string::npos ) {
			THROW_CPPGET_REPORT_EXCEPTION(  format( "Cannot find . in '%s' at hunk '%s'", tcn_name, s ) );
		}

		ret.push_back( s.substr( pos+1 ) );
	}

	return ret;
}

} // /namespace wamas

#endif
