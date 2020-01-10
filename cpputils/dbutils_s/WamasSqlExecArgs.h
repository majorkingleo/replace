#ifndef __wamas_wms_WamasSqlExecArgs_h
#define __wamas_wms_WamasSqlExecArgs_h

#include <sqlargs.h>
#include <dbsql.h>
#include <wamasexception.h>

/**
 * Instantiates and initialices a local WamasSqlExecArgs object.
 */
#define WAMASSQLEXECARGS(handle, tid, fac) \
			wamas::wms::WamasSqlExecArgs handle(tid, fac, __FILE__, __LINE__); \
			handle.init(__FILE__, __LINE__);

namespace wamas {
namespace wms {

/**
 * Lifetime controller for a SqlTexecArgsPtr.
 *
 * Upon calling init() a new SqlTexecArgsPtr will be created. As soon as this 
 * object goes out of scope the captured SqlTexecArgsPtr will be destroyed.
 *
 * It's highly recommanded not to use this class directly. 
 * Use the WAMASSQLEXECARGS() macro instead.
 */
class WamasSqlExecArgs
{
public:
    WamasSqlExecArgs(const void* tid, char const *fac, char const *file, const int line)
        : execArg_(NULL), tid_(tid), fac_(fac), file_(file), line_(line)
    {
    }

    ~WamasSqlExecArgs()
    {
        if (execArg_) execArg_ = SqlExecArgsDestroy(execArg_);
    }

	/**
	 * This method creates the actual SqlTexecArgsPtr
	 * @note WAMASSQLEXECARGS() macro calls this method for you
	 */
	void init(char const *file, const int line)
	{
		if (execArg_) execArg_ = SqlExecArgsDestroy(execArg_);

		execArg_ = DbSqlTidCreateSqlExecArgs(const_cast<void*>(tid_));
        if (!execArg_) {
			throw WamasException(file, line, fac_, 
					MLM("Fehler bei Datenbank Zugriff"));
        }
	}

    operator SqlTexecArgsPtr()
	{ 
        if (!execArg_) {
			throw WamasException(file_, line_, fac_,
					MLM("Argumente nicht initialisiert"));
        }

		return execArg_; 
	}

private:
	/**
	 * Copy-Ctor is private as it should never be called.
	 * If we would copy execArg_ to a new object it would be destroyed
	 * by the other object's dtor.
	 * If you really need to pass WamasSqlExecArgs to an other function (or so)
	 * pass it by reference and not by value.
	 */
    WamasSqlExecArgs(const WamasSqlExecArgs& other)
	: execArg_(),
	tid_(),
	fac_(),
	file_(),
	line_()
    {
		abort(); // just to make sure ;)
	}

	/**
	 * We don't want WamasSqlExecArgs objects to be copied as we can't create
	 * a deep-copy of a SqlTexecArgsPtr. If we only copy the pointer both 
	 * object would free the same pointer.
	 */
    WamasSqlExecArgs& operator=(const WamasSqlExecArgs& other)
    {
		abort(); // just to make sure ;)
		return *this;
	}

	SqlTexecArgsPtr execArg_;
	const void* tid_;
	char const *fac_;
	char const *file_;
	int line_;
};

} // /namespace wms
} // /namespace wamas

#endif

