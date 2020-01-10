/**
 * @file
 * Lifetime Controller for DBSQL's SqlContext.
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */
#ifndef __wamas_wms_WamasSqlContext_h
#define __wamas_wms_WamasSqlContext_h

#include <dbsql.h>
#include <wamasexception.h>

/**
 * Instantiates and initializes a local WamasSqlContext object.
 * 
 * WamasSqlContext throws an WamasException if something went wrong.
 * 
 * How to use:
 * @code
 * const void *tid = 4711;
 * const char *fac = "sepp";
 * WAMASSQLCONTEXT (ctx,tid,fac);
 * int rv = TExecSqlX (tid, ctx, "SELECT"...); 
 * @endcode
 */
#define WAMASSQLCONTEXT(handle, tid, fac) \
			wamas::wms::WamasSqlContext handle(tid, fac, __FILE__, __LINE__); \
			handle.init(__FILE__, __LINE__);

namespace wamas {
namespace wms {

/**
 * Lifetime controller for DBSQL's SqlContext.
 *
 * It's highly recommended to use the WAMASSQLCONTEXT() macro for handling this class.
 * 
 * The object does NOT initialize the SqlContext during it's construction, the init() method
 * is responsible for doing that.
 * 
 * If the SqlContext is invalid (not initialized yet or the creation of a new SqlContext has failed), 
 * a WamasException is thrown.
 * 
 * As soon as this 
 * object goes out of scope the encapsulated SqlContext will be destroyed automatically.
 */
class WamasSqlContext {
public:
    WamasSqlContext(const void* tid, char const *fac, char const *file, const int line)
        : sqlCtx_(NULL), tid_(tid), fac_(fac), file_(file), line_(line)
    { }

    ~WamasSqlContext() {
        if (sqlCtx_) TSqlDestroyContext (const_cast<void*>(tid_), sqlCtx_);
    }

	/**
	 * This method creates the actual SqlContext
	 * @note WAMASSQLCONTEXT() macro calls this method for you
	 */
	void init(char const *file, const int line) {
		if (sqlCtx_) TSqlDestroyContext (const_cast<void*>(tid_), sqlCtx_);

		sqlCtx_ = TSqlNewContext (const_cast<void*>(tid_), NULL);
        if (!sqlCtx_) {
			throw WamasException(file, line, fac_, MLM("Fehler bei Datenbank Zugriff"));
        }
	}

    operator SqlContext*() { 
        if (!sqlCtx_) {
			throw WamasException(file_, line_, fac_, MLM("Fehler bei Datenbank Zugriff"));
        }
		return sqlCtx_; 
	}

private:
	/// Copy not allowed
    WamasSqlContext(const WamasSqlContext& other);

    /// Assignment not  allowed
    WamasSqlContext& operator=(const WamasSqlContext& other);

	SqlContext *sqlCtx_;
	const void* tid_;
	char const *fac_;
	char const *file_;
	int line_;
};

} // /namespace wms
} // /namespace wamas

#endif
