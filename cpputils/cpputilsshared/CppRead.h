/**
 * Wrapper around CppGet for easier handling
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 *
 * ************************ Example 1 * Fetch signle row data  *********************
 *
 * 	TPA tpa = {};
 * 	tpa.tpaTaNr = 1234;
 *
 * 	TEK tek1;
 * 	TEP tep1;
 *	MAP map1;
 *	MAK mak1;
 *
 *	CppRead<TPA> read(NULL, tpa);
 *
 *	read >> tek1 >> tep1 >> map1 >> mak1;
 *
 *	std::cout << "RefNr1: " << mak1.makRefNr << std::endl;
 *	std::cout << "MaPos1: " << map1.mapMaPos << std::endl;
 *	std::cout << "ArtNr1: " << tep1.tepUnit.unitArtNr << std::endl;
 *	std::cout << "TeId1:  " << tek1.tekTeId << std::endl;
 *
 *
 * ************************ Example 2 * Fetch multiple row data  ******************
 *
 * 	TPA tpa = {};
 * 	tpa.tpaTaNr = 1234;
 *
 * 	TEK tek1;
 * 	std::vector<TEP> teps;
 * 	std::vector<MAP> maps;
 *
 * 	CppRead<TPA> read(NULL, tpa);
 * 	read >> tek1 >> teps >> maps;
 *
 * 	std::cout << "TeId1:  " << tek1.tekTeId << std::endl;
 *
 * 	for( TEP & tep : teps ) {
 * 		std::cout << "ArtNr1: " << tep.tepUnit.unitArtNr << std::endl;
 * 	}
 *
 * 	for( MAP & map : maps ) {
 * 		std::cout << "Id: " << map.mapId << std::endl;
 * 	}
 *
 * ************************ Example 3 * Read with external join ******************
 *
 * 	TPA tpa = {};
 *	tpa.tpaTaNr = 57162;
 *
 *  FES fes1;
 *
 *	CppRead<TPA> read(NULL, tpa);
 *
 *	read >> TCN_TPA_Aktpos_FeldId " = " TCN_FES_FeldId >> fes1;
 *
 *	std::cout << "Aktpos: " << fes1.fesFeldId << "(" << fes1.fesLagId << ")" << std::endl;
 *
 * ************************ Example 4 * Read with SqlNotFound ******************
 *
 *  TPA tpa;
 *	tpa.tpaTaNr = 0;
 *
 *	TEK tek;
 *
 *	CppRead<TPA> read(NULL, tpa);
 *
 *  // no exception if tek was not found
 *	if( !(read >> tek) ) {
 *		std::cout << "leider nicht gefunden\n";
 *	}
 *
 * ************************ Example 5 * Read with SqlNotFound ******************
 *
 *	TPA tpa;
 *	tpa.tpaTaNr = 64216;
 *
 *	TEK tek;
 *
 *	CppRead<TPA> read(NULL, tpa);
 *
 *	if( read >> tek ) {
 *		std::cout << "TeId: " << tek.tekTeId << std::endl;
 *	} else {
 *		std::cout << "leider nicht gefunden\n";
 *	}
 *
 * ************************ Example 6 * Join a table with itself ******************
 *
 *  TPA tpa;
 *  tpa.tpaTaNr = 64216;
 *
 *	std::vector<TPA> vTpa;
 *
 *	CppRead<TPA> read(NULL, tpa);
 *
 *	read >> TCN_TPA_Ziel_FeldId " = " TCN_TPA_Ziel_FeldId >> vTpa;
 *
 *
 * ************************ Example 7 * User dummy objects for joining ************
 *
 * 	// This will generate one TExecSql() call
 * 	// %TEK, %TEP will be selected, but the data will not be stored anywhere
 *
 * 	ART art;
 *
 *	if( CppRead<TPA>(NULL, TPA{1})
 *				>> TEK{}
 *				>> TEP{}
 *				>> art ) {
 *		std::cout << "ArtNr: " << art.artArtNr << std::endl;
 *	}
 *
 * ************************ Example 8 * external additional where clause ************
 *
 * 	// you can append additional sql queries at the end
 *
 * 	ART art;
 *
 *	if( CppRead<TPA>(NULL, (TPA){1})
 *				>> TEK{}
 *				>> TEP{}
 *				>> art
 *				>> TCN_Art_ArtBez " like '%10'"  ) {
 *		std::cout << "ArtNr: " << art.artArtNr << std::endl;
 *	}
 *
 *
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */

#ifndef _TOOLS_CPPREAD_H
#define _TOOLS_CPPREAD_H

#if __cplusplus >= 201103

#include "CppGet.h"
#include <sstream>
#include <set>

#define FAC_CPPREAD "CppRead"

namespace Tools {

class CppReadTmpImpl
{
protected:
	class CppTmpReadExec
	{
	public:
		class SelectionEntryBase
		{
		public:
			virtual ~SelectionEntryBase()
			{}

			virtual void assignResult() = 0;
		};

		template<class TargetType> class OneSelectionEntry : public SelectionEntryBase
		{
			std::vector<TargetType> data;
			TargetType *ptrTarget;

		public:
			OneSelectionEntry( TargetType & target )
			: data(),
			  ptrTarget( &target )
			{}

			OneSelectionEntry( OneSelectionEntry<TargetType> & other ) = delete;
			OneSelectionEntry & operator=( OneSelectionEntry<TargetType> & other ) = delete;

			void assignResult() override {

				if( data.empty() ) {
					throw REPORT_EXCEPTION( "no result" );
				}

				*ptrTarget = data.at(0);
			}

			std::vector<TargetType> & getData() {
				return data;
			}
		};

		template<class TargetType> class DirectSelectionEntry : public SelectionEntryBase
		{
			std::vector<TargetType> & data;

		public:
			DirectSelectionEntry( std::vector<TargetType> & target )
			: data( target )
			{}

			// CppSqlReader directly loads into data vector, so there is nothing to do
			void assignResult() override {
			}

			std::vector<TargetType> & getData() {
				return data;
			}
		};

		template<class TargetType> class DummySelectionEntry : public SelectionEntryBase
		{
			std::vector<TargetType> data;

		public:
			DummySelectionEntry()
			: data()
			{}

			DummySelectionEntry( DummySelectionEntry<TargetType> & other ) = delete;
			DummySelectionEntry & operator=( DummySelectionEntry<TargetType> & other ) = delete;

			void assignResult() override {
			}

			std::vector<TargetType> & getData() {
				return data;
			}
		};

	private:
		CppSqlReaderImpl *sql_reader;


		// Every CppReadTmpImpl() container appends a given join in constructor
		// This is required to append an sql query at last;
		// And this join has to be discarded if something else is todo. (another >> will be appended)
		// So to know what has to be discarded the container stores the address of the join string
		// eg:
		//  CppRead<TEK> read(NULL, (TEK){{"10000000000000001"}})
		// 					  >> tep
		// 					  >> TCN_TEP_PosNr " = 2 ";

		typedef std::vector<std::pair<std::string,std::string*>> JOIN_CONTAINER;
		JOIN_CONTAINER joins;

		std::vector<std::string> selection_table_names;
		std::vector<std::string> from_table_names;
		std::vector<std::string> from_table_names_alias;
		std::set<std::string> used_table_alias;
		std::list<std::shared_ptr<SelectionEntryBase>> selection_entries;
		std::string last_selection_table_name;
		std::string last_selection_table_name_alias;

	public:
		CppTmpReadExec( void *tid );
		~CppTmpReadExec();

		CppTmpReadExec( const CppTmpReadExec & other ) = delete;
		CppTmpReadExec & operator=( const CppTmpReadExec & other ) = delete;

		bool execute( bool exception_on_sqlnot_found = true );

		void appendJoin( const std::string & join ) {
			// store the address of this query, we use it as an index
			joins.push_back(std::pair<std::string,std::string*>(join,const_cast<std::string*>(&join)));
		}

		void removeMyJoin( const std::string *join ) {
			for( JOIN_CONTAINER::iterator it = joins.begin(); it != joins.end(); it++ ) {
				if( it->second == join ) {
					joins.erase( it );
					return;
				}
			}
		}

		template <class TargetTable> void appendOneSelectTableEntry( const std::string & table_name,
																     const std::string & table_name_alias,
																	 TargetTable & target_table,
																	 bool bind_target_table = true ) {

			selection_table_names.push_back( table_name );

			from_table_names.push_back( table_name );
			from_table_names_alias.push_back( table_name_alias );

			if( bind_target_table ) {
				LogPrintf( FAC_CPPREAD, LT_TRACE, "Bind target table: %s", CppGetImpl::getStructNameFromTypeId(typeid(target_table).name()) );
				auto selection_entry = std::make_shared<OneSelectionEntry<TargetTable>>( target_table  );
				sql_reader->selStruct( table_name, selection_entry->getData() );
				selection_entries.push_back( selection_entry );

			} else {
				LogPrintf( FAC_CPPREAD, LT_TRACE, "Bind dummy table: %s", CppGetImpl::getStructNameFromTypeId(typeid(target_table).name()) );
				auto selection_entry = std::make_shared<DummySelectionEntry<TargetTable>>();
				sql_reader->selStruct( table_name, selection_entry->getData() );
				selection_entries.push_back( selection_entry );
			}

			last_selection_table_name = table_name;
			last_selection_table_name_alias = table_name_alias;
		}

		template <class TargetTable> void appendOneSelectTableEntry( const std::string & table_name,
																	 const std::string & table_name_alias,
																	 std::vector<TargetTable> & target_table,
																	 bool dummy_value = true ) {

			LogPrintf( FAC_CPPREAD, LT_TRACE, "Bind vector: %s", CppGetImpl::getStructNameFromTypeId(typeid(target_table).name()) );

			selection_table_names.push_back( table_name );
			from_table_names.push_back( table_name );
			from_table_names_alias.push_back( table_name_alias );

			auto selection_entry = std::make_shared<DirectSelectionEntry<TargetTable>>( target_table );
			sql_reader->selStruct( table_name, selection_entry->getData() );
			selection_entries.push_back( selection_entry );

			last_selection_table_name = table_name;
			last_selection_table_name_alias = table_name_alias;
		}

		void appendFromTableEntry( const std::string & table_name ) {
			from_table_names.push_back( table_name );
			from_table_names_alias.push_back( table_name );
			used_table_alias.insert( table_name );
		}

		const std::string & getLastSelectionTableName() const {
			return last_selection_table_name;
		}

		const std::string & getLastSelectionTableNameAlias() const {
			return last_selection_table_name_alias;
		}

		CppSqlReaderImpl & getSqlReader() {
			return *sql_reader;
		}

		std::string getNewAlias4Table( const std::string & table );

		/*
		 * after calling execute() sql_reader is set to 0
		 */
		bool isInCharge() const {
			return sql_reader != NULL;
		}
	};

	std::shared_ptr<CppTmpReadExec> tmp_exec;
	std::string join;

private:
	CppReadTmpImpl( const CppReadTmpImpl & other )
	: tmp_exec( other.tmp_exec ), // yes copy the "pointer"
	  join( other.join )
	{}

	CppReadTmpImpl( const CppReadTmpImpl & other, const std::string & join_ )
	: tmp_exec( other.tmp_exec ), // yes copy the "pointer"
	  join( join_ )
	{
		appendJoin( join );
	}

	CppReadTmpImpl & operator=( const CppReadTmpImpl & other ) {
		tmp_exec = other.tmp_exec; // yes copy the pointer
		// but do not copy the join
		return *this;
	}

	CppReadTmpImpl( void *tid )
	: tmp_exec( std::make_shared<CppTmpReadExec>(tid) ),
	  join()
	{}

	CppReadTmpImpl( void *tid, const std::string & join_ )
	: tmp_exec( std::make_shared<CppTmpReadExec>(tid) ),
	  join( join_ )
	{}

public:

	~CppReadTmpImpl() noexcept(false);

	// only CppRead is allowed to copy CppReadTmpImpl
	template <class Table> friend class CppRead;

	/**
	 * if this operator is called, the query will be executed immediate,
	 * but it does not throw an exception in case of an SqlNotFound
	 *
	 * So you can implement:
	 *
	 * TEK tek;
	 * CppRead read(NULL, tpa);
	 *
	 * if( !(read >> tek ) ) {
	 *    std::cout << "nothing found";
	 * }
	 *
	 */
	bool operator!() {
		return !tmp_exec->execute(false);
	}

	/**
	 * if this operator is called, the query will be executed immediate,
	 * but it does not throw an exception in case of an SqlNotFound
	 *
	 * So you can implement:
	 *
	 * TEK tek;
	 * CppRead read(NULL, tpa);
	 *
	 * if( read >> tek ) {
	 *    std::cout << "found";
	 * } else {
	 *    std::cout << "not found";
	 * }
	 *
	 */
	operator bool() {
		return tmp_exec->execute(false);
	}

	template <class TargetTable>
	CppReadTmpImpl operator>>( TargetTable & in )
	{
		CppReadTmpImpl tmp_impl( *this );

		auto cppget = CppGet( TargetTable );

		const std::string source_table_and_struct_name = tmp_exec->getLastSelectionTableName();
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(in).name());

		const std::string source_table_alias = tmp_exec->getLastSelectionTableNameAlias();
		const std::string target_table_alias = tmp_exec->getNewAlias4Table( target_table_and_struct_name );

		if( join.empty() ) {
			join = cppget.getJoinTryBothDirections( source_table_and_struct_name,
													source_table_alias,
													target_table_and_struct_name,
													target_table_alias );

		} else if( source_table_alias != source_table_and_struct_name ||
				   target_table_alias != target_table_and_struct_name ) {
			join = fix_last_join( join,
								  source_table_and_struct_name,
								  source_table_alias,
								  target_table_and_struct_name,
								  target_table_alias );
		}

		removeMyJoin( &join );

		tmp_impl.appendJoin( join );
		tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_alias, in );

		tmp_impl.join.clear();
		join.clear();

		return tmp_impl;
	}

	/**
	 * Temporary object implementation. We are getting a dummy object here, so no
	 * reading will happen but the joins will be appended to the sql statements.
	 */
	template <class TargetTable>
	CppReadTmpImpl operator>>( const TargetTable & in )
	{
		CppReadTmpImpl tmp_impl( *this );

		auto cppget = CppGet( TargetTable );

		const std::string source_table_and_struct_name = tmp_exec->getLastSelectionTableName();
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(in).name());

		const std::string source_table_alias = tmp_exec->getLastSelectionTableNameAlias();
		const std::string target_table_alias = tmp_exec->getNewAlias4Table( target_table_and_struct_name );

		if( join.empty() ) {
			join = cppget.getJoinTryBothDirections( source_table_and_struct_name,
					source_table_alias,
					target_table_and_struct_name,
					target_table_alias );

		} else if( source_table_alias != source_table_and_struct_name ||
				target_table_alias != target_table_and_struct_name ) {
			join = fix_last_join( join,
					source_table_and_struct_name,
					source_table_alias,
					target_table_and_struct_name,
					target_table_alias );
		}

		removeMyJoin( &join );

		tmp_impl.appendJoin( join );

		TargetTable tmpObj;
		tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_alias, tmpObj, false );

		tmp_impl.join.clear();
		join.clear();

		return tmp_impl;
	}

	template <class TargetTable>
	CppReadTmpImpl operator>>( std::vector<TargetTable> & in )
	{
		CppReadTmpImpl tmp_impl( *this );

		auto cppget = CppGet( TargetTable );

		const std::string source_table_and_struct_name = tmp_exec->getLastSelectionTableName();
		const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(TargetTable).name());

		const std::string source_table_alias = tmp_exec->getLastSelectionTableNameAlias();
		const std::string target_table_alias = tmp_exec->getNewAlias4Table( target_table_and_struct_name );

		if( join.empty() ) {
			join = cppget.getJoinTryBothDirections( source_table_and_struct_name,
													source_table_alias,
													target_table_and_struct_name,
													target_table_alias );
		} else if( source_table_alias != source_table_and_struct_name ||
				   target_table_alias != target_table_and_struct_name ) {
			join = fix_last_join( join,
								  source_table_and_struct_name,
								  source_table_alias,
								  target_table_and_struct_name,
								  target_table_alias );
		}

		removeMyJoin( &join );

		tmp_impl.appendJoin( join );
		tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_alias, in );

		tmp_impl.join.clear();
		join.clear();

		return tmp_impl;
	}

	CppReadTmpImpl operator>>( const std::string & join ) {
		CppReadTmpImpl tmp_impl( *this, join );
		return tmp_impl;
	}

	CppReadTmpImpl operator>>( const char* join ) {
		CppReadTmpImpl tmp_impl( *this, std::string(join) );
		return tmp_impl;
	}

	CppSqlReaderImpl & getSqlReader() {
		return tmp_exec->getSqlReader();
	}

protected:

	void appendJoin( const std::string & join ) {
		tmp_exec->appendJoin( join );
	}

	void removeMyJoin( const std::string *join ) {
		tmp_exec->removeMyJoin( join );
	}

	template <class TargetTable> void appendOneSelectTableEntry( const std::string & table_name,
																 const std::string & table_name_alias,
																 TargetTable & target_table,
																 bool bind_target_table = true ) {
		tmp_exec->appendOneSelectTableEntry( table_name, table_name_alias, target_table, bind_target_table );
	}

	void appendFromTableEntry( const std::string & table_name ) {
		tmp_exec->appendFromTableEntry( table_name );
	}

	std::string getNewAlias4Table( const std::string & table ) {
		return tmp_exec->getNewAlias4Table( table );
	}

	std::string fix_last_join( const std::string & last_join,
							   const std::string & left_table_name,
							   const std::string & left_table_alias,
							   const std::string & right_table_name,
							   const std::string & right_table_alias );

	std::string fix_simple_join( const std::string & last_join,
								 const std::string & left_table_name,
								 const std::string & left_table_alias,
								 const std::string & right_table_name,
								 const std::string & right_table_alias );

};

template <class SourceTable> class CppRead
{
protected:
	void *tid;
	SourceTable start;
	std::string external_join;

public:
	CppRead( void *tid_, const SourceTable & start_ )
	: tid( tid_ ),
	  start( start_ ),
	  external_join()
	{}

	CppRead( const CppRead<SourceTable> & other )
	: tid( other.tid ),
	  start( other.start ),
	  external_join( other.external_join )
	{

	}

	CppRead<SourceTable> & operator=( const CppRead<SourceTable> & other )
	{
		tid = other.tid;
		start = other.start;
		external_join = other.external_join;
		return *this;
	}

	template <class TargetTable>
	CppReadTmpImpl operator>>( TargetTable & in );

	template <class TargetTable>
	CppReadTmpImpl operator>>( const TargetTable & in );

	template <class TargetTable>
	CppReadTmpImpl operator>>( std::vector<TargetTable> & in );

	CppReadTmpImpl operator>>( const std::string & join );

	CppReadTmpImpl operator>>( const char* join ) {
		return operator>>(std::string(join));
	}
};


template <class SourceTable>
template <class TargetTable>
CppReadTmpImpl CppRead<SourceTable>::operator>>( TargetTable & in )
{
	CppReadTmpImpl tmp_impl( tid );

	auto cppget = CppGet( TargetTable );

	const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(start).name());
	const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(in).name());

	std::string join = external_join;

	if( join.empty() ) {
		join = cppget.getJoinTryBothDirections( source_table_and_struct_name, target_table_and_struct_name );
	}

	tmp_impl.appendJoin( join );
	tmp_impl.appendFromTableEntry( source_table_and_struct_name );
	tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_and_struct_name, in );

	SqlTableDesc *descSource = cppget.getSqlTableDescByTableName( source_table_and_struct_name );

	if( !descSource ) {
		throw REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_and_struct_name ) );
	}

	std::string pk_sql;
	cppget.bindPk( pk_sql, descSource, &start, tmp_impl.getSqlReader().getExecArgs(), false );

	tmp_impl.appendJoin( pk_sql );

	return tmp_impl;
}


template <class SourceTable>
template <class TargetTable>
CppReadTmpImpl CppRead<SourceTable>::operator>>( const TargetTable & in )
{
	CppReadTmpImpl tmp_impl( tid );

	auto cppget = CppGet( TargetTable );

	const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(start).name());
	const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(in).name());

	std::string join = external_join;

	if( join.empty() ) {
		join = cppget.getJoinTryBothDirections( source_table_and_struct_name, target_table_and_struct_name );
	}

	tmp_impl.appendJoin( join );
	tmp_impl.appendFromTableEntry( source_table_and_struct_name );

	TargetTable tmp_obj;
	tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_and_struct_name, tmp_obj, false );

	SqlTableDesc *descSource = cppget.getSqlTableDescByTableName( source_table_and_struct_name );

	if( !descSource ) {
		throw REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_and_struct_name ) );
	}

	std::string pk_sql;
	cppget.bindPk( pk_sql, descSource, &start, tmp_impl.getSqlReader().getExecArgs(), false );

	tmp_impl.appendJoin( pk_sql );

	return tmp_impl;
}

template <class SourceTable>
template <class TargetTable>
CppReadTmpImpl CppRead<SourceTable>::operator>>( std::vector<TargetTable> & in )
{
	CppReadTmpImpl tmp_impl( tid );

	auto cppget = CppGet( TargetTable );

	const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(start).name());
	const std::string target_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(TargetTable).name());

	std::string join = external_join;

	if( join.empty() ) {
		join = cppget.getJoinTryBothDirections( source_table_and_struct_name, target_table_and_struct_name );
	}

	tmp_impl.appendJoin( join );
	tmp_impl.appendFromTableEntry( source_table_and_struct_name );
	tmp_impl.appendOneSelectTableEntry( target_table_and_struct_name, target_table_and_struct_name, in );

	SqlTableDesc *descSource = cppget.getSqlTableDescByTableName( source_table_and_struct_name );

	if( !descSource ) {
		throw REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_and_struct_name ) );
	}

	std::string pk_sql;
	cppget.bindPk( pk_sql, descSource, &start, tmp_impl.getSqlReader().getExecArgs(), false );

	tmp_impl.appendJoin( pk_sql );

	return tmp_impl;
}


template <class SourceTable>
CppReadTmpImpl CppRead<SourceTable>::operator>>( const std::string & join_next )
{
	CppReadTmpImpl tmp_impl( tid, join_next );

	auto cppget = CppGet( SourceTable );

	const std::string source_table_and_struct_name = cppget.getStructNameFromTypeId(typeid(start).name());

	tmp_impl.appendFromTableEntry( source_table_and_struct_name );

	SqlTableDesc *descSource = cppget.getSqlTableDescByTableName( source_table_and_struct_name );

	if( !descSource ) {
		throw REPORT_EXCEPTION( format( "cannot find TableDesc for table: '%s'", source_table_and_struct_name ) );
	}

	std::string pk_sql;
	cppget.bindPk( pk_sql, descSource, &start, tmp_impl.getSqlReader().getExecArgs(), false );

	tmp_impl.appendJoin( pk_sql );

	return tmp_impl;
}

} // namespace TOOLS

#endif
#endif  /* _TOOLS_CPPREAD_H */
