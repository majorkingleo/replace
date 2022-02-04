#ifndef WamasSqlReadListSimple_h_
#define WamasSqlReadListSimple_h_
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
#include <format.h>

namespace wamas {
namespace wms {

/**
 * Template function to read the contents of an xmi2pdl modeled table into a std::vector.
 * @param tid SQL transaction
 * @param fac Log facility
 * @param clname The table name
 * @param where An optional where clause (without the 'WHERE')
 * @param orderby An optional order-by clause (without the 'ORDER BY')
 * @param outlist The records read from database
 * 
 * The function throws an exception if something went wrong.
 * 
 * Usage examples:
 * @code
 * 
 * #include <wamas/wms/WamasSqlReadListSimple.h>
 * #include <FES.h>
 * 
 * std::vector<FES>	locations;
 * 
 * wamas::wms::WamasSqlReadListSimple<FES> (tid, fac, "FES", locations);
 * 
 * wamas::wms::WamasSqlReadListSimple<FES> (tid, fac, "FES", locations, 
 * 		"REG_LAGIDPHY='HRL'");
 * 
 * wamas::wms::WamasSqlReadListSimple<FES> (tid, fac, "FES", locations, 
 * 		"", 
 * 		"REG_REGAL,REG_REGALTEIL,FELDID");
 * 
 * wamas::wms::WamasSqlReadListSimple<FES> (tid, fac, "FES", locations, 
 * 		"REG_LAGIDPHY='HRL'", 
 * 		"REG_REGAL,REG_REGALTEIL,FELDID");
 * 
 * wamas::wms::WamasSqlReadListSimple<FES> (tid, fac, TN_FES, locations, 
 * 		scoped_cstr::form("%s='%s'", TCN_FES_REG_LagIdPhy, "HRL"),
 * 		TCN_FES_REG_Regal","TCN_FES_REG_Regalteil","TCN_FES_FeldId);
 * @endcode
 */
template <typename thetype> 
void WamasSqlReadListSimple (const void *tid, const char *fac, const char *clname, 
		std::vector<thetype>& outlist, const char *where=NULL, const char *orderby=NULL)
{
	WAMASSERT (fac != NULL, fac, "fac is NULL");
	WAMASSERT (clname != NULL, fac, "clname is NULL");
	
	SqlTableDesc    *tabledesc = _SqlFindTableDesc(clname);
	WAMASSERT (tabledesc != NULL, fac, "SqlTableDesc not found");
	WAMASSERT (tabledesc->strucSize == sizeof(thetype), fac, "struct size mismatch");

	int rv = 0;
	const int blocksize = 100;
	std::string	stmt(Tools::format("SELECT %%%s from %s", clname,clname));
	if (IsEmptyStrg (where) == 0) {
		stmt += Tools::format(" WHERE %s", where);
	}
	if (IsEmptyStrg (orderby) == 0) {
		stmt += Tools::format(" ORDER BY %s", orderby);
	}
	do {
		thetype	block[blocksize];
		memset(block, 0, sizeof(block));
		rv = TExecSqlX (tid, NULL, 
				rv == 0 ? stmt.c_str(): NULL,
						blocksize, 0,
						SELSTRUCT (clname, block[0]),
						NULL);
		WAMASSERT_DB (rv > 0 || TSqlError(tid) == SqlNotFound, tid, fac);

		for (int i = 0; i < rv; ++i) {
			outlist.push_back (block[i]);
		}
	}while (rv == blocksize);
}

} // /namespace wms
} // /namespace wamas

#endif
