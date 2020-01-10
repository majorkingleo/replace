/*
 * @brief 
 * @file: WamasSqlEditListSimple.h
 * @ author Copyright (c) 2011 Salomon Automation GmbH
 */

#ifndef __wamas_wms_WamasSqlEditListSimple_h
#define __wamas_wms_WamasSqlEditListSimple_h

//#include <logtool2.h>
//#include <if_opmsg_s.h>
//#include <if_frmwrk_s.h>
#include <hist_util.h>
#include <dbsqlstd.h>
#include <vector>
#include <string>
#include <wamasexception.h>

namespace wamas {
namespace wms {

  /**
   * for internal use only. Do not use directly
   */
  const int blocksize=500;
  template <typename thetype>
  void WamasSqlEditListSimple(const void* tid, const std::string &fac, const std::string &tname,
      std::vector<thetype> &inlist, SqlTstdStmtRes res)
  {
    WAMASSERT (!fac.empty(), fac.c_str(), "fac is empty");
    WAMASSERT (!tname.empty(), fac.c_str(), "clname is empty");

    SqlTableDesc    *tabledesc = _SqlFindTableDesc(tname.c_str());
    WAMASSERT (tabledesc != NULL, fac.c_str(), "SqlTableDesc not found");
    WAMASSERT (tabledesc->strucSize == sizeof(thetype), fac.c_str(), "struct size mismatch");

    for (size_t i = 0; i < inlist.size(); i += blocksize) {
        int cnt = blocksize;
        if (i + blocksize > inlist.size()) {
            cnt = inlist.size() - i;
        }
        thetype *tmp = &inlist[i];
        int rv = TExecStdSqlX(tid, NULL, res, tname.c_str(), tmp, cnt, NULL, NULL);
        WAMASSERT_DB (rv == cnt, tid, fac.c_str());
    }
  }

/**
 * Template function to insert the contents of an a std::vector which is modeled in pdl into the database
 * @param tid SQL transaction
 * @param fac Log facility
 * @param tname The table name
 * @param inlist The records to insert into database
 *
 * The function throws an exception if something went wrong.
 *
 * Usage examples:
 * @code
 *
 * #include <wamas/wms/WamasSqlEditListSimpleArgs.h>
 * #include <FES.h>
 *
 * std::vector<FES> locations;
 * for(int i=0;i<10000;i++) {
 *   locations.push_back(someData);
 * }
 *
 *  wamas::wms::WamasSqlInsertListSimple<FES> (tid, fac, "TN_FES", locations);
 *
 * @endcode
 */
template <typename thetype>
void WamasSqlInsertListSimple(const void* tid, const std::string &fac, const std::string &tname,
		std::vector<thetype> &inlist)
{
	typedef typename std::vector<thetype>::const_iterator thetypeit;
	int i=0;
    for(thetypeit it=inlist.begin();it!=inlist.end();it++, i++) {
//* msi    	int rv=FrmwrkSIf_SetHist(fac.c_str(), FrmwrkTSetHistTyp_Insert, tname.c_str(), &inlist[i]);
    	int rv=SetHist(tname.c_str(), &inlist[i], HIST_INSERT, GetUserOrTaskName());
    	WAMASSERT (rv >= 0, fac.c_str(), NULL);
    }
	WamasSqlEditListSimple(tid, fac, tname, inlist, StdNinsert);
}

/**
 * Template function to update the contents of an pdl modelled table in the database by providing a vector
 * including the content to be updated.
 * @param tid SQL transaction
 * @param fac Log facility
 * @param tname The table name
 * @param inlist The records to insert into database
 *
 * The function throws an exception if something went wrong.
 *
 * Usage examples:
 * @code
 *
 * #include <wamas/wms/WamasSqlEditListSimpleArgs.h>
 * #include <FES.h>
 *
 * std::vector<FES> locations;
 * for(int i=0;i<10000;i++) {
 *   locations.push_back(someData);
 * }
 *
 *  wamas::wms::WamasSqlUpdateListSimple<FES> (tid, fac, "TN_FES", locations);
 *
 * @endcode
 */
template <typename thetype>
void WamasSqlUpdateListSimple(const void* tid, const std::string &fac, const std::string &tname,
		std::vector<thetype> &inlist)
{
	typedef typename std::vector<thetype>::const_iterator thetypeit;
	int i=0;
    for(thetypeit it=inlist.begin();it!=inlist.end();it++, i++) {
//! *msi    	int rv=FrmwrkSIf_SetHist(fac.c_str(), FrmwrkTSetHistTyp_Update, tname.c_str(), &inlist[i]);
    	int rv=SetHist(tname.c_str(), &inlist[i], HIST_UPDATE, GetUserOrTaskName());
    	WAMASSERT (rv >= 0, fac.c_str(), NULL);
    }
	WamasSqlEditListSimple(tid, fac, tname, inlist, StdNupdate);
}

/**
 * Template function to delete the contents of an pdl modelled table from the database by providing a vector including
 * the entries to be deleted.
 * @param tid SQL transaction
 * @param fac Log facility
 * @param tname The table name
 * @param inlist The records to insert into database
 *
 * The function throws an exception if something went wrong.
 *
 * Usage examples:
 * @code
 *
 * #include <wamas/wms/WamasSqlEditListSimpleArgs.h>
 * #include <FES.h>
 *
 * std::vector<FES> locations;
 * for(int i=0;i<10000;i++) {
 *   locations.push_back(someData);
 * }
 *
 *  wamas::wms::WamasSqlDeleteListSimple<FES> (tid, fac, "TN_FES", locations);
 *
 * @endcode
 */
template <typename thetype>
void WamasSqlDeleteListSimple(const void* tid, const std::string &fac, const std::string &tname,
		std::vector<thetype> &inlist)
{
	WamasSqlEditListSimple(tid, fac, tname, inlist, StdNdelete);
}

/**
 * Prot and delete
 * attention: does not work for head/position tables. You have to make this manually, but you can use the
 * deleteListSimple and InsertListSimple Templates.
 *
 * @param tid
 * @param fac
 * @param tname tablename
 * @param p_tname tablename of the protocol table
 * @param inlist list of entries to delete
 * @param p_inlist list of entries to protocol
 */
template <typename thetype, typename p_thetype>
void WamasSqlProtandDeleteSimple(const void* tid, const std::string &fac, const std::string &tname,
		const std::string p_tname, std::vector<thetype> &inlist, std::vector<p_thetype> &p_inlist)
{
    WAMASSERT (!fac.empty(), fac.c_str(), "fac is empty");
    WAMASSERT (!tname.empty(), fac.c_str(), "tname is empty");
    WAMASSERT (!p_tname.empty(), fac.c_str(), "p_tname is empty");

    SqlTableDesc    *tabledesc = _SqlFindTableDesc(tname.c_str());
    WAMASSERT (tabledesc != NULL, fac.c_str(), "SqlTableDesc not found");
    WAMASSERT (tabledesc->strucSize == sizeof(thetype), fac.c_str(), "struct size mismatch");

    SqlTableDesc    *p_tabledesc = _SqlFindTableDesc(p_tname.c_str());
    WAMASSERT (p_tabledesc != NULL, fac.c_str(), "SqlTableDesc not found");
    WAMASSERT (p_tabledesc->strucSize == sizeof(p_thetype), fac.c_str(), "struct size mismatch");

    WAMASSERT (inlist.size()==p_inlist.size(), fac.c_str(), "List length isn't equal");

    for (size_t i = 0; i < inlist.size(); i += blocksize) {
        int cnt = blocksize;
        if (i + blocksize > inlist.size()) {
            cnt = inlist.size() - i;
        }
        thetype *tmp = &inlist[i];
        p_thetype *p_tmp = &p_inlist[i];

        //prot elements
        int rv=TExecStdSqlX(tid, NULL, StdNinsert, p_tname.c_str(), p_tmp, cnt, NULL, NULL);
        WAMASSERT_DB (rv == cnt, tid, fac.c_str());

        //delete elements
        rv = TExecStdSqlX(tid, NULL, StdNdelete, tname.c_str(), tmp, cnt, NULL, NULL);
        WAMASSERT_DB (rv == cnt, tid, fac.c_str());
    }
}

} // /namespace wms
} // /namespace wamas

#endif

