/**
 * CppGet
 *
 * Simple get PDL generated structs from DB
 *
 * Basic usage:
 *
 * 		// ########## CppGet functions (load a single object) ##########
 *
 *		TPA tpa = {};
 *		tpa.tpaTaNr = 57162;
 *
 *		// Loads tpa via primary key into tpa structure.
 *		// The resulting query in this example will be:
 *		//     select %TPA from TPA where TaNr = :tanr
 *		CppGet(TPA).load( tid, tpa );
 *
 *
 *
 *		// Loads TEK from TPA. The foreign key will be automatically detected
 *		// will throw an exaption if fereign key not found, or TEK does not exists.
 *		// The resulting query in this example will be:
 *		//     select %TEK from TEK, TPA where Tek.TeId = TPA.TeId and TPA.TaNr = :tanr
 * 		TEK tek = CppGet(TEK).from( tid, tpa );
 *
 *
 *
 * 		// Loads FES from TPA, by a user defined join
 * 		FES fes = CppGet(FES).from( tid, tpa, TCN_TPA_Aktpos_FeldId " = " TCN_FES_FeldId );
 *
 *
 *		// Loads ART from TEP not by navigating over the primary key. The function
 *		// directly joins the values of the art table.
 *		// The resulting query in this example will be
 *		//     select %ART from ART where Mandant = :mandant and ArtNr = :artnr
 *
 *		TEP tep = {};
 *		StrCpy( tep.tepMandant, "001" );
 * 		StrCpy( tep.tepArtNr, "20" );
 *
 *		ART art = CppGet(ART).fromData( tid, tep );
 *
 *
 *
 *		// Loads FES from TPA by specifiying a specific field to join
 *		TPA tpa = {};
 *		StrCpy( tpa.tpaZiel.FeldId, "KL-POOL" );
 *
 *		FES fes = CppGet(FES).fromData( tid, tpa, TCN_TPA_Ziel_FeldId );
 *
 *
 *
 *		// Loads TPA from FES by using a specific Foreign key and an extra sql
 *		FES fes = {};
 *		StrCpy( fes.fesFeldId, "KL-POOL" );
 *
 *		TPA tpa = CppGet(TPA).fromData( tid, fes, TCN_TPA_Ziel_FeldId, TCN_TPA_TaNr " = 1 " );
 *
 *		// ##########  CppGets functions (load a vector) ##########
 *
 * 		// load TEPs from TEK
 * 		std::vector<TEP> vTep = CppGets(TEP).from( tid, tek );
 *
 *
 *		// ########## CppTGet functions (load a tuple) ##########
 *
 *		// load TEK and FES from TPA.
 *		TEK tek = {};
 * 		TEK fes = {};
 *
 *		std::tie( fes, tek ) = CppTGet(FES,TEK).from( tid, tpa );
 *
 *		std::cout << "TEK: " << tek.tekTeId << std::endl;
 *		std::cout << "FES: " << fes.fesFeldId << std::endl;
 *
 *
 *		// load more data c++11 style
 *		auto data = CppTGet(MAK,MAP,TEP,TEK).from( tid, tpa );
 *
 *		std::cout << "RefNr: " << std::get<0>(data).makRefNr << std::endl;
 *		std::cout << "MaPos: " << std::get<1>(data).mapMaPos << std::endl;
 *		std::cout << "ArtNr: " << std::get<2>(data).tepUnit.unitArtNr << std::endl;
 *		std::cout << "TeId:  " << std::get<3>(data).tekTeId << std::endl;
 *
 *		// load more data c++17 style
 *		auto data = CppTGet(MAK,MAP,TEP,TEK).from( tid, tpa );
 *
 *		std::cout << "RefNr: " << std::get<MAK>(data).makRefNr << std::endl;
 *		std::cout << "MaPos: " << std::get<MAP>(data).mapMaPos << std::endl;
 *		std::cout << "ArtNr: " << std::get<TEP>(data).tepUnit.unitArtNr << std::endl;
 *		std::cout << "TeId:  " << std::get<TEK>(data).tekTeId << std::endl;
 *
 *
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */

#ifndef _TOOLS_CPPGETFROM_H
#define _TOOLS_CPPGETFROM_H

#if __cplusplus >= 201103

#include <dbsqlstd.h>
#include <typeinfo>
#include <string>
#include <map>
#include <cpp_util.h>
#include "TupleReverse.h"

#define FAC_CPPGET "CppGet"

namespace Tools {


class CppGetImpl
{
protected:
	static const char *pcSourceFile;
	static int iSourceLine;

	std::string fac;
	std::map<std::string,SqlTableDesc*> desc_by_struct_name;
	std::map<std::string,SqlTableDesc*> desc_by_table_name;

public:
	CppGetImpl( const std::string & fac = FAC_CPPGET );
	virtual ~CppGetImpl();

	static std::string getDemangledName( const std::string & name );
	static std::string getStructNameFromTypeId( const std::string & str_typeid );

	SqlTableDesc* getSqlTableDescByStructName( const std::string & struct_name );
	SqlTableDesc* getSqlTableDescByTableName( const std::string & table_name );

#if __GNUC__
	static std::string gccDemangle(const char* name);
#endif

	void get( void *tid,
			void *pvTarget,
			const void *pvSource,
			const std::string & source_struct_name,
			const std::string & source_table_name,
			const std::string & target_struct_name,
			const std::string & target_table_name,
			const std::string & join_query );

	void getFromData( void *tid,
					  void *pvTarget,
					  const void *pvSource,
					  const std::string & source_struct_name,
					  const std::string & source_table_name,
					  const std::string & target_struct_name,
					  const std::string & target_table_name,
					  const std::vector<std::string> & table_source_fields,
					  const std::vector<std::string> & table_target_fields,
					  const std::string & join_query );

	std::string getJoin( const std::string & table_a, const std::string & table_b, bool no_exception = false ) {
		return getJoin( table_a, table_a, table_b, table_b, no_exception );
	}

	std::string getJoin( const std::string & table_a,
						 const std::string & table_a_alias,
						 const std::string & table_b,
						 const std::string & table_b_alias,
						 bool no_exception = false ) {
		std::vector<std::string> table_a_fields;
		std::vector<std::string> table_b_fields;

		return getJoin( table_a,
						table_a_alias,
						table_b,
						table_b_alias,
						table_a_fields,
						table_b_fields,
						no_exception );
	}

	std::string getJoin( const std::string & table_a,
						 const std::string & table_a_alias,
						 const std::string & table_b,
						 const std::string & table_b_alias,
						 std::vector<std::string> & table_a_fields,
						 std::vector<std::string> & table_b_fields,
						 bool no_exception = false );

	std::string getJoinTryBothDirections( const std::string & table_a, const std::string & table_b ) {
		return getJoinTryBothDirections( table_a, table_a, table_b, table_b );
	}

	std::string getJoinTryBothDirections( const std::string & table_a,
										  const std::string & table_a_alias,
										  const std::string & table_b,
										  const std::string & table_b_alias ) {
		std::string res = getJoin( table_a, table_a_alias, table_b, table_b_alias, true );

		if( res.empty() ) {
			res = getJoin( table_b, table_b_alias, table_a, table_a_alias );
		}

		return res;
	}

	static CppGetImpl & instance();

	static void setSourceLocation( const char *pcSource, int iLine );

	const char * getSourceFile() const {
		return pcSourceFile;
	}

	int getSourceLine() const {
		return iSourceLine;
	}

	void bindPk( std::string & sql,
				 SqlTableDesc *descSource,
				 const void *pvSource,
				 SqlTexecArgsPtr execArgs,
				 bool sql_with_and = false );

	void bindKey( std::string & sql,
				 SqlTableDesc *descSource,
				 const void *pvSource,
				 const std::string & target_table_name,
				 const std::vector<std::string> & table_source_fields,
				 const std::vector<std::string> & table_target_fields,
				 SqlTexecArgsPtr execArgs,
				 bool sql_with_and = false );

	std::vector<std::string> getFkFromTCNNames( const std::string & tcn_name );

public:
	void initDbDesc();
};

#define CppGet( TYPE ) Tools::CppGetClassTemplate<TYPE>()._setSourceLocation( __FILE__, __LINE__ )


#define THROW_CPPGET_REPORT_EXCEPTION( what ) \
	{ \
		LogPrintf( fac, LT_ALERT, Tools::format( "from %s:%d %s", pcSourceFile, iSourceLine, what ) ); \
		throw ReportException( Tools::format( "Exception from: %s:%d:%s message: %s%s\ninitial at: %s:%d", \
				pcSourceFile, iSourceLine, __FUNCTION__, what, Tools::BackTraceHelper::bt.bt(),  __FILE__, __LINE__ ), \
				Tools::format("%s", what ) ); \
	}

#define CPPGET_SQL_EXCEPTION( tid ) \
	Tools::SqlException( Tools::format( "SqlError from: %s:%d:%s %s%s\ninitial at: %s:%d",\
		pcSourceFile, iSourceLine, __FUNCTION__, \
		TSqlErrTxt(tid),					\
		Tools::BackTraceHelper::bt.bt(), \
		__FILE__, __LINE__ ), TSqlErrTxt(tid), TSqlError(tid) )

template <class TARGET_TABLE> class CppGetClassTemplate : public CppGetImpl
{
public:
	CppGetClassTemplate<TARGET_TABLE> & _setSourceLocation( const char *pcSource, int iLine )
	{
		CppGetImpl::setSourceLocation( pcSource, iLine );
		return *this;
	}

	template <class SOURCE_TABLE>
	TARGET_TABLE from( void *tid,
					   const SOURCE_TABLE & table,
					   const std::string & external_join = std::string(),
					   const std::string & extra_join = std::string() )
	{
		TARGET_TABLE tTarget = {};

		CppGetImpl & cppget = CppGetImpl::instance();

		const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(table).name());
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(tTarget).name());

		std::string join = external_join;

		if( join.empty() ) {
			join = cppget.getJoinTryBothDirections( source_table_and_struct_name, target_table_and_struct_name );
		}

		if( !extra_join.empty() ) {
			join += " and " + extra_join;
		}

		cppget.get( tid, &tTarget, &table,
					source_table_and_struct_name,
					source_table_and_struct_name,
					target_table_and_struct_name,
					target_table_and_struct_name,
					join );

		return tTarget;
	}

	template <class SOURCE_TABLE>
	TARGET_TABLE fromData( void *tid,
					   const SOURCE_TABLE & table,
					   const std::string & external_foreign_key = std::string(), // use TCN_Names here
					   const std::string & extra_join = std::string() )
	{
		TARGET_TABLE tTarget = {};

		CppGetImpl & cppget = CppGetImpl::instance();

		const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(table).name());
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(tTarget).name());

		std::string join;
		std::vector<std::string> table_a_fields;
		std::vector<std::string> table_b_fields;

		if( !external_foreign_key.empty() ) {
			// eg TCN_TPA_Aktpos_FeldId  "TPA.Aktpos_FeldId"
			if( external_foreign_key.find( source_table_and_struct_name + "." ) == 0 ) {

				table_a_fields = cppget.getFkFromTCNNames( external_foreign_key );

				join = cppget.getJoin( source_table_and_struct_name,
						source_table_and_struct_name,
						target_table_and_struct_name,
						target_table_and_struct_name,
						table_a_fields,
						table_b_fields,
						true );

			} else if( external_foreign_key.find( target_table_and_struct_name + "." ) == 0 ) {

				table_b_fields = cppget.getFkFromTCNNames( external_foreign_key );

				join = cppget.getJoin( target_table_and_struct_name,
							target_table_and_struct_name,
							source_table_and_struct_name,
							source_table_and_struct_name,
							table_b_fields,
							table_a_fields,
							true );
			}
		} else {

			join = cppget.getJoin( source_table_and_struct_name,
					source_table_and_struct_name,
					target_table_and_struct_name,
					target_table_and_struct_name,
					table_a_fields,
					table_b_fields,
					false );

			// try swaped version, throw exception if not found
			if( join.empty() ) {
				join = cppget.getJoin( target_table_and_struct_name,
						target_table_and_struct_name,
						source_table_and_struct_name,
						source_table_and_struct_name,
						table_b_fields,
						table_a_fields,
						true );
			}
		}

		cppget.getFromData( tid, &tTarget, &table,
					source_table_and_struct_name,
					source_table_and_struct_name,
					target_table_and_struct_name,
					target_table_and_struct_name,
					table_a_fields,
					table_b_fields,
					extra_join );

		return tTarget;
	}

	// loads table via primary key into target structure
	void load( void *tid, TARGET_TABLE & target )
	{
		CppGetImpl & cppget = CppGetImpl::instance();

		const std::string table_name = cppget.getStructNameFromTypeId(typeid(target).name());
		int rv = _TExecStdSql( tid, StdNselect, table_name.c_str(), &target );

		if( rv != 1 ) {
			throw CPPGET_SQL_EXCEPTION( tid );
		}
	}
};

} // namespace Tools

#if __cplusplus >= 201103

#include "CppSqlReader.h"

namespace Tools {

#define CppGets( TYPE ) Tools::CppGetsClassTemplate<TYPE>()._setSourceLocation( __FILE__, __LINE__ )

template <class TARGET_TABLE> class CppGetsClassTemplate : public CppGetImpl
{
public:
	CppGetsClassTemplate<TARGET_TABLE> & _setSourceLocation( const char *pcSource, int iLine )
	{
		CppGetImpl::setSourceLocation( pcSource, iLine );
		return *this;
	}

	template <class SOURCE_TABLE>
	std::vector<TARGET_TABLE> from( void *tid,
									const SOURCE_TABLE & table,
									const std::string & external_join = std::string(),
									const std::string & extra_join = std::string() )
	{
		std::vector<TARGET_TABLE> res;

		CppGetImpl & cppget = CppGetImpl::instance();

		const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(table).name());
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(TARGET_TABLE).name());

		std::string join = external_join;

		if( join.empty() ) {
			join = cppget.getJoinTryBothDirections( source_table_and_struct_name, target_table_and_struct_name );
		}

		std::string sql = "select %" + target_table_and_struct_name +
						  " from " + target_table_and_struct_name +
						  " , " + source_table_and_struct_name +
						  " where " + join;

		if( !extra_join.empty() ) {
			sql += " and " + extra_join;
		}

		CppSqlReaderBase::setSourceLocation( pcSourceFile, iSourceLine );
		Tools::CppSqlReaderImpl sqlReader(fac, tid, sql );
		sqlReader.selStruct( target_table_and_struct_name, res );

		SqlTableDesc *descSource = getSqlTableDescByTableName( source_table_and_struct_name );

		if( !descSource ) {
			THROW_CPPGET_REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_and_struct_name ) );
		}

		std::string pk_sql;
		cppget.bindPk( pk_sql, descSource, &table, sqlReader.getExecArgs(), true );

		sqlReader.appendQuery( pk_sql );
		sqlReader.execAll();

		return res;
	}
};

} // namespace Tools
#endif


// #if __cplusplus > 201402
#include <boost/preprocessor.hpp>

namespace Tools {

#if __cplusplus > 201402

#define CPPGET_REVERSE(...) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_REVERSE(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))

#define CppTGet( ... ) Tools::CppTGetClassTemplate< CPPGET_REVERSE(__VA_ARGS__) >()._setSourceLocation( __FILE__, __LINE__ )

template <class ...TargetTypes> class CppTGetClassTemplate : public CppGetImpl
{
	template<int element, class TargetTable, class ...XTargetTypes> struct CppTGetApply
	{
		template <class SourceTable, class Tuple> void apply( void *tid, const SourceTable & table, Tuple & tuple )
		{
			auto & ele = std::get<element>( tuple );

			// std::cout << "From " <<  typeid(table).name() << " to " <<  typeid(ele).name() << std::endl;
			ele = CppGetClassTemplate<TargetTable>().from(tid, table );

			if constexpr ( element + 1 < sizeof...(TargetTypes) ) {
				CppTGetApply<element+1,XTargetTypes...>().apply( tid, ele, tuple );
			}
		}
	};

public:
	CppTGetClassTemplate<TargetTypes...> & _setSourceLocation( const char *pcSource, int iLine )
	{
		CppGetImpl::setSourceLocation( pcSource, iLine );
		return *this;
	}

	template <class SourceType>
	typename TupleHelper::tuple_reverse<std::tuple<TargetTypes...>>::type from( void *tid, const SourceType & table )
	{
		std::tuple<TargetTypes...> ret;
		fromPart<0,TargetTypes...,SourceType>( tid, table, ret );

		// reverse the tuple order, to be again in the same order as
		// typed in source code
		return TupleHelper::reverseTuple( ret );
	}

	template <int element, class ...SubTargetTypes, class SourceType, class Tuple>
	void fromPart( void *tid, const SourceType & table, Tuple & tuple )
	{
		CppTGetApply<element, SubTargetTypes...>().apply( tid, table, tuple );
	}
};

#else

#define CPPGET_REVERSE(...) BOOST_PP_SEQ_ENUM(BOOST_PP_SEQ_REVERSE(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__)))
#define CppTGet( ... ) Tools::CppTGetClassTemplate< CPPGET_REVERSE(__VA_ARGS__) >()._setSourceLocation( __FILE__, __LINE__ )

/*
 * This apprach creates a tuple, unpacks it to a variadic template call
 *
 * 		CppTGet<TEP, TEK>( tid, TPA )
 *  will be unpacked to
 * 		CppTGetApply( tid, TPA, TEK, TEP )
 */

template <class ...TargetTypes> class CppTGetClassTemplate : public CppGetImpl
{
	void CppTGetApply( void *tid ) {}
	template <class T> void CppTGetApply( void *tid, T * last ) {}

	template<class SourceTable, class TargetTable, class ...Args>
	void CppTGetApply( void *tid, const SourceTable * source, TargetTable * target, Args... args )
	{
		// std::cout << "From " <<  typeid(source).name()
		//		  << " to " << typeid(target).name()
		//		  << std::endl;

		*target = CppGetClassTemplate<TargetTable>().from(tid, *source );
		CppTGetApply( tid, target, args... );
	}

public:
	CppTGetClassTemplate<TargetTypes...> & _setSourceLocation( const char *pcSource, int iLine )
	{
		CppGetImpl::setSourceLocation( pcSource, iLine );
		return *this;
	}

	template <class SourceType>
	typename TupleHelper::tuple_reverse<std::tuple<TargetTypes...>>::type from( void *tid, const SourceType & sourceTable )
	{
		std::tuple<TargetTypes...> ret;
		applyAllArguments( tid, sourceTable, ret, typename TupleHelper::SequenceHelper::gens<sizeof...(TargetTypes)>::type());

		// reverse the tuple order, to be again in the same order as
		// typed in source code
		return TupleHelper::reverseTuple( ret );
	}

	template <class Tuple, class SourceType, int ...S>
	void applyAllArguments( void *tid, SourceType & sourceTable, Tuple & ret, TupleHelper::SequenceHelper::seq<S...> )
	{
		// expand the tuple to one function call
		CppTGetApply(tid, &sourceTable, &std::get<S>(ret) ... );
	}
};

#endif

} // /namespace Tools

#endif // #if __cplusplus >= 201103
#endif  /* _TOOLS_CPPGETFROM_H */
