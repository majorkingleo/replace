#ifndef WamasSqlReadListSimpleArgs_h_
#define WamasSqlReadListSimpleArgs_h_
/**
 * @file
 * A template for easily reading records from an XMI modeled table.
 * @author Copyright (c) 2010 Salomon Automation GmbH
 */

#include <string>
#include <vector>
#include <dbsqlstd.h>
#include <sstring.h>
#include <wamasexception.h>
#include <WamasSqlContext.h>
#include <WamasSqlExecArgs.h>
#include <string_utils.h>

namespace wamas {
namespace wms {

/**
 * Template function to read the contents of an pdl modeled table into a std::vector.
 * @param tid SQL transaction
 * @param fac Log facility
 * @param clname The table name
 * @param outlist The records read from database
 * @param where An optional where clause (without the 'WHERE')
 * @param orderby An optional order-by clause (without the 'ORDER BY', could be NULL)
 * @param ... variable argument list for bindvariables - terminated with NULL (SQLSTRING(xx))
 *
 * The function throws an exception if something went wrong.
 *
 * Usage examples:
 * @code
 *
 * #include <wamas/wms/WamasSqlReadListSimpleArgs.h>
 * #include <FES.h>
 *
 * std::vector<FES> locations;
 *
 *  wamas::wms::WamasSqlReadListSimpleArgs<FES> (tid, fac, "FES", locations,
 *      "REG_LAGIDPHY = 'HRL'",
 *      "REG_REGAL, REG_REGALTEIL, FELDID");
 *
 *  wamas::wms::WamasSqlReadListSimpleArgs<FES> (tid, fac, TN_FES, locations,
 *      scoped_cstr::form("%s = :lagid", TCN_FES_REG_LagIdPhy),
 *      TCN_FES_REG_Regal","TCN_FES_REG_Regalteil","TCN_FES_FeldId,
 *      SQLSTRING(lagid), NULL);
 * @endcode
 */

#if __cplusplus - 0 <= 1 // jurassicly old gnu gcc 3.x (for lutz)
  // gcc 3.x cannot use va_arg lists starting with a c++ object,
  // so order by has to be a const char* pointer	
  typedef char* WamasSqlReadListSimpleArgs_OrderByType ;
#else
  typedef std::string WamasSqlReadListSimpleArgs_OrderByType ;
#endif
  
template <typename thetype>
void WamasSqlReadListSimpleArgs (const void *tid, const char *fac,
        const std::string& clname, std::vector<thetype>& outlist,
        const std::string& where = "",
		// Must not use a reference here (-> va_start)
		// unfortunately, old VC9 allowed compilation and 
		// it led to the following runtime issue:
		// "_SqlExecArgsVaAppend() failed"
		const WamasSqlReadListSimpleArgs_OrderByType orderby="", ...)
{
    WAMASSERT (fac != NULL, fac, "fac is NULL");
    WAMASSERT (!clname.empty(), fac, "clname is NULL");

    SqlTableDesc    *tabledesc = _SqlFindTableDesc(clname.c_str());
    WAMASSERT (tabledesc != NULL, fac, "SqlTableDesc not found");
    WAMASSERT (tabledesc->strucSize == sizeof(thetype), fac, "struct size mismatch");

    WAMASSQLEXECARGS(hSqlExecArgs, const_cast<void *>(tid), fac);

    int rv = 0;
    const int blocksize = 100;
	std::string	stmt = "SELECT %" + clname + " from " + clname;
	if (!IsEmptyStrg (where.c_str())) {
		stmt += " WHERE " + where;
	}

    // this can handle std::string and char* // riequired for gcc 3.x compatibility
	if (!Tools::is_empty_string(orderby)) {
		stmt += " ORDER BY " + std::string(orderby); // order by can by a char*
	}
	
    thetype block[blocksize];
    rv = SqlExecArgsAppend(hSqlExecArgs, SELSTRUCT(clname.c_str(), block[0]), NULL);
    WAMASSERT (rv >= 0, fac, "SqlExecArgsAppend() failed");

    if (!where.empty()) {
    	va_list hArgList;
    	_SqlObjectCatchCaller(SqlExecArgsGetObject(hSqlExecArgs));
    	va_start(hArgList, orderby);
    	rv = _SqlExecArgsVaAppend(hSqlExecArgs, hArgList);
    	va_end(hArgList);
    	WAMASSERT (rv >= 0, fac, "_SqlExecArgsVaAppend() failed");
    }

    rv = SqlExecArgsSetArrayLen(hSqlExecArgs, blocksize, 0);
    WAMASSERT (rv >= 0, fac, "SqlExecArgsSetArrayLen() failed");

    rv = 0;
    do {
        memset(block, 0, sizeof(block));

        rv = rv==0 ?
          TExecSqlArgsV(const_cast<void *>(tid), NULL, stmt.c_str(), hSqlExecArgs):
          TExecSqlArgsV(const_cast<void *>(tid), NULL, NULL, hSqlExecArgs);

         WAMASSERT_DB (rv > 0 || TSqlError(tid) == SqlNotFound, tid, fac);

        for (int cnt=0; cnt < rv; ++cnt) {
            outlist.push_back (block[cnt]);
        }

    } while (rv == blocksize);
}



} // /namespace wms
} // /namespace wamas

#endif
