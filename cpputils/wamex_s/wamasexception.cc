/**
 * @file
 * Exceptions for wamas
 * @author Copyright (c) 2008 Salomon Automation GmbH
 */

#include "wamasexception.h"

#include <dbsql.h>
#include <logtool2.h>
#include <dbsqlstd.h>
#include <cpp_util.h>

namespace wamas {
namespace wms {


void WamasExceptionBase::init (char const *file, const int line, const std::string & fac, const std::string & msg)
{
  if( what_.empty() ) {
	what_ = MLM("Unbekannter Fehler");
  }

  what_ = msg;

  where_ = Tools::format("%s:%d", file, line);

  std::string facility = "undef";
  if ( !fac.empty() ) {
      facility = fac;
  }

  _LogSetLocation (file, line);
  _LogPrintf (facility.c_str(), LT_ALERT, "EXCEPTION: %s", msg.c_str());
}

const char*  WamasExceptionBase::what() const throw()
{
	return what_.c_str();
}

std::string WamasExceptionBase::where() 
{
	return where_;
}

long WamasExceptionBase::errorCode()
{
	return errorCode_;
}

WamasException::WamasException (char const *file, const int line, char const *fac, char const *msg)
 : WamasExceptionBase (file, line, fac, msg)
{
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush(GeneralTerrException, msg);
}

WamasException::WamasException (char const *file, const int line, std::string const &fac, char const *msg)
 : WamasExceptionBase (file, line, fac.c_str(), msg)
{
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush(GeneralTerrException, msg);
}

WamasException::WamasException (char const *file, const int line, std::string const &fac, const std::string & msg)
 : WamasExceptionBase (file, line, fac.c_str(), msg)
{
//  _OpmsgSIf_SetLocation (file,line);
//  _OpmsgSIf_ErrPush(GeneralTerrException, msg);
}

/*
WamasException::WamasException (char const *file, const int line, char const *fac, char const *msg)
 : WamasExceptionBase (file,line,fac, msg)
{
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush(errorCode, what());
}
*/
void WamasException::log(const char* fac)
{

	LogPrintf (fac, LT_ALERT, 
		"Caught WamasException: %s at %s", what_.c_str(), where().c_str());

}

WamasExceptionDb::WamasExceptionDb (char const *file, const int line, const void *tid, char const *fac)
 : WamasExceptionBase (file,line,fac, TSqlPrettyErrTxt(tid)), dberror_(TSqlPrettyErrTxt(tid))
{
	what_ = MLM ("Datenbankfehler!");

	//! TODO: implement code

	// First we push the general errortext which might be shown to the user
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush(GeneralTerrDb, what_.c_str());
	// next push the conrete DB failure
//	_OpmsgSIf_ErrPush(GeneralTerrDb, dberror_.c_str());
}
#ifdef __NOT_USED_BY_FESTO
WamasExceptionDb::WamasExceptionDb (char const *file, const int line, const void *tid, std::string const &fac)
 : WamasExceptionBase (file,line,fac.c_str(), TSqlPrettyErrTxt(tid), GeneralTerrDb), dberror_(TSqlPrettyErrTxt(tid))
{
	what_ = MLM ("Datenbankfehler!");

	// First we push the general errortext which might be shown to the user
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush(GeneralTerrDb, what_.c_str());
	// next push the conrete DB failure
//	_OpmsgSIf_ErrPush(GeneralTerrDb, dberror_.c_str());
}
#endif
void WamasExceptionDb::log(const char* fac)
{
/*
	LoggingSIf_LogPrintf (fac, LOGGING_SIF_ALERT, 
		"Caught WamasExceptionDb: %s at %s", dberror_.c_str(), where().c_str());
*/
	LogPrintf (fac, LT_ALERT, 
		"Caught WamasExceptionDb: %s at %s", dberror_.c_str(), where().c_str());
}

/*
WamasExceptionAlreadyHandled::WamasExceptionAlreadyHandled(char const *file, const int line, char const *fac)
 : WamasExceptionBase (file,line,fac, MLM("Fehler wurde bereits bearbeitet"), GeneralTerrHasAlreadyBeenHandled)
{
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush (GeneralTerrHasAlreadyBeenHandled, MLM("Fehler wurde bereits bearbeitet"));
}

WamasExceptionAlreadyHandled::WamasExceptionAlreadyHandled(char const *file, const int line, std::string const &fac)
 : WamasExceptionBase (file,line,fac.c_str(), MLM("Fehler wurde bereits bearbeitet"), GeneralTerrHasAlreadyBeenHandled)
{
//	_OpmsgSIf_SetLocation (file,line);
//	_OpmsgSIf_ErrPush (GeneralTerrHasAlreadyBeenHandled, MLM("Fehler wurde bereits bearbeitet"));
}
*/
/*
void WamasExceptionAlreadyHandled::log(const char* fac)
{

	LoggingSIf_LogPrintf (fac, LOGGING_SIF_ALERT, 
		"Caught WamasExceptionAlreadyHandled at %s", where().c_str());

	LogPrintf (fac, LOGGING_SIF_ALERT, 
		"Caught WamasExceptionAlreadyHandled at %s", where().c_str());

}
*/
} // /namespace wms
} // /namespace wamas
