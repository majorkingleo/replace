#ifndef WAMASEXCEPTION_H_
#define WAMASEXCEPTION_H_
/**
 * @file
 * Exceptions for WAMAS
 * @author Copyright (c) 2008 Salomon Automation GmbH
 */

#include <string>
#include <sstring.h>
#include <exception>
/* MSI UNCOMMENT or REPLACE or DELETE 

#include <if_opmsg_s.h>

*/

namespace wamas {
namespace wms {

/**
 * This is the baseclass for all Wamas execptions.
 * 
 * As far as all other WamasExceptions are derived from this class you just
 * need to catch this one regardless of which type the businesscode trows.
 */
class WamasExceptionBase : public std::exception {
public:
	/**
	 * Construct an exception.
	 * Upon construction, the passed message msg is written into a logfile, specified by fac. 
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] fac Log facility (may be NULL)
	 * @param[in] msg Log message (may be NULL)
	 * @param[in] errorCode opmsg error code
	 */

	/* 	MSI FESTO
	WamasExceptionBase (char const *file, const int line, char const *fac, char const *msg,
			const long errorCode);
	*/
	WamasExceptionBase (char const *file, const int line, char const *fac, char const *msg)
      : what_(),
        where_(),
        errorCode_(0)
    {
	  init( file, line, fac, msg );
    }

	WamasExceptionBase ( char const *file, const int line, const std::string & fac, const std::string & msg )
      : what_(),
        where_(),
        errorCode_(0)
    {
      init( file, line, fac, msg );
    }

	WamasExceptionBase ( char const *file, const int line, const std::string & fac, char const *msg )
      : what_(),
        where_(),
        errorCode_(0)
	{
	  init( file, line, fac, msg );
	}

	virtual ~WamasExceptionBase() throw () {}

	virtual void log(const char* fac) =0;
	
	/// @return the exception message
	const char* what() const throw();
	/// @return the location where the exception was thrown
	std::string where();
	/// @return the opmsg error code
	long errorCode();

protected:
	std::string what_; ///< Exception message
	std::string where_; ///< Exception location
	long errorCode_; ///< Exception error code

	void init( char const *file, const int line, const std::string & fac, const std::string & msg );
};


/**
 * A class the be thrown as exception within any wamas module.
 * It is recommended to use the wrapper #WAMASSERT for convenience.
 */
class WamasException : public WamasExceptionBase {
public:
	/**
	 * Construct an exception.
	 * Upon construction, the passed message msg is written into a logfile, specified by fac. 
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] fac Log facility (may be NULL)
	 * @param[in] msg Log message (may be NULL)
	 */
	WamasException (char const *file, const int line, char const *fac, char const *msg);
	/**
	 * Construct an exception.
	 * Upon construction, the passed message msg is written into a logfile, specified by fac. 
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] fac Log facility (may be NULL)
	 * @param[in] msg Log message (may be NULL)
	 */
	WamasException (char const *file, const int line, std::string const &fac, char const *msg);

    /**
     * Construct an exception.
     * Upon construction, the passed message msg is written into a logfile, specified by fac.
     * @param[in] file Sourcefilename, typically passed __FILE__
     * @param[in] line Line ins Sourcefilename, typically passed __LINE__
     * @param[in] fac Log facility (may be NULL)
     * @param[in] msg Log message (may be NULL)
     */
    WamasException (char const *file, const int line, std::string const &fac, const std::string & msg);

	/**
	 * Construct an exception.
	 * Upon construction, the passed message msg is written into a logfile, specified by fac.
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] fac Log facility (may be NULL)
	 * @param[in] msg Log message (may be NULL)
	 * @param[in] errorCode opmsg error code
	 */
	WamasException (char const *file, const int line, char const *fac, char const *msg,
			const long errorCode);
	~WamasException() throw() {}

	virtual void log(const char* fac);
};


/**
 * A class the be thrown as database access exception within any wamas module.
 * It is recommended to use the wrapper #WAMASSERT_DB for convenience.
 */
class WamasExceptionDb : public WamasExceptionBase {
public:
	/**
	 * Construct an exception.
	 * Upon construction, the error text of the sql transaction is written into a logfile, 
	 * specified by fac. 
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] tid SQL transaction
	 * @param[in] fac Log facility (may be NULL)
	 */
	WamasExceptionDb (char const *file, const int line, const void *tid, char const *fac);
	WamasExceptionDb (char const *file, const int line, const void *tid, std::string const &fac);
	~WamasExceptionDb() throw () {}

	virtual void log(const char* fac);

private:
	std::string dberror_;
};


/**
 * A class the be thrown when a rollback should be forced but the error itself has already been handled.
 * It is recommended to use the wrapper #WAMTHROW_ALREADY_HANDLED for convenience.
 */
/*
class WamasExceptionAlreadyHandled : public WamasExceptionBase {
public:
	**
	 * Construct an exception.
	 * Upon construction, the message text "Fehler wurde bereits bearbeitet" is written into a logfile,
	 * specified by fac. 
	 * @param[in] file Sourcefilename, typically passed __FILE__
	 * @param[in] line Line ins Sourcefilename, typically passed __LINE__
	 * @param[in] fac Log facility (may be NULL)
	
	WamasExceptionAlreadyHandled (char const *file, const int line, char const *fac);
	WamasExceptionAlreadyHandled (char const *file, const int line, std::string const &fac);
	~WamasExceptionAlreadyHandled() throw () {}

	virtual void log(const char* fac);
};
*/

} // /namespace wms
} // /namespace wamas

/**
 * Assertion / Exception Macro.
 * Use this macro to test for an expression to be true. If <expr> evaluates
 * to false, an exception of type WamasException is thrown.
 *
 * @param[in] expr Expression to be tested
 * @param[in] fac Log facility (may be NULL)
 * @param[in] msg Log message (may be NULL)
 *
 * @code
 * Typical use:
 * bool tekExists = Transportunit(tek,teid);
 * WAMASSERT (tekExists == true, fac, MLM("Keine Transporteinheit gefunden"));
 *
 * ... no further error handling necessary
 * @endcode
 */
#define WAMASSERT(expr,fac,msg) \
	    if (!(expr)) throw wamas::wms::WamasException (__FILE__, __LINE__, fac, msg)

/**
 * Assertion / Exception Macro.
 * Use this macro to test for an expression to be true. If <expr> evaluates
 * to false, an exception of type WamasExceptionDb is thrown.
 * Prefer this macro over WAMASSERT for handling database access errors.
 * @param[in] expr Expression to be tested
 * @param[in] tid SQL transaction
 * @param[in] fac Log facility (may be NULL)
 *
 * @code
 * Typical use:
 * int rv = TExecSqlX (tid, blah);
 * WAMASSERT_DB (rv >= 0 || TSqlError(tid) == SqlNotFound, tid, fac);
 *
 * ... no further error handling necessary
 * @endcode
 */
#define WAMASSERT_DB(expr,tid,fac) \
	    if (!(expr)) throw wamas::wms::WamasExceptionDb(__FILE__, __LINE__, tid, fac)

/**
 * Assertion / Exception Macro.
 * Use this macro to test for an expression to be true. If <expr> evaluates
 * to false, an exception of type WamasException is thrown.
 * Prefer this macro over WAMASSERT for testing calls to functions which do not throw
 * exceptions, but return error codes and put error messages onto the OPMSG stack.
 *
 * @param[in] expr Expression to be tested
 * @param[in] rv Return value of failed function
 * @param[in] fac Log facility (may be NULL)
 *
 * @code
 * Typical use:
 * int rv = Transportunit(tek,teid);
 * WAMASSERT_ERR (rv >= 0, rv, fac);
 *
 * ... no further error handling necessary
 * @endcode
 */
/*
#define WAMASSERT_ERR(expr,rv,fac)												\
		if (!(expr)) {															\
			const char *msg = NULL;												\
			long error = 0;														\
			OpmsgSIf_Get1stMessageOnly(&error, &msg);							\
			throw wamas::wms::WamasException(__FILE__, __LINE__, fac, msg, rv);	\
		}
*/

/**
 * WAMASSERT for technical exception with no MLM text
 */
#define WAMASSERT_TECH(expr,fac,msg)                                            \
        if (!(expr)) {                                                          \
            _LoggingSIf_LogSetLocation (__FILE__, __LINE__);                    \
            LoggingSIf_LogPrintf(fac, LOGGING_SIF_ALERT, "%s", msg);                    \
            char *errshow=OpmsgSIf_ErrGetErrMsg(GeneralTerrInternal);           \
            throw wamas::wms::WamasException (__FILE__, __LINE__, fac, errshow); \
        }



/**
 * Macro to throw a WamasExceptionAlreadyHandled.
 * Use this macro when you don't want any further error handling (AlertBox, SendError...) but
 * force a rollback.
 * This is the exception-style equivalent to the classic GeneralTerrHasAlreadyBeenHandled error code.
 * 
 * @param[in] fac Log facility (may be NULL)
 * 
 * @code
 * Typical use:
 * if (specialCase) {
 *     DoSpecialHandling(tid, fac);		// attention: any database updates done inside won't be committed!
 *     WAMTHROW_ALREADY_HANDLED (fac);	// force a rollback
 * }
 * @endcode
 */
/*
#define WAMTHROW_ALREADY_HANDLED(fac)	\
		throw wamas::wms::WamasExceptionAlreadyHandled(__FILE__, __LINE__, fac)
*/
#endif

