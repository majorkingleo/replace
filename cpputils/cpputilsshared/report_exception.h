/**
 * exception base class, writes the message also to stderr
 * @author Copyright (c) 2022 SSI Schaefer IT Solutions
 */

#ifndef _TOOLS_REPORT_EXCEPTION_H
#define _TOOLS_REPORT_EXCEPTION_H

#include <iostream>

#include <backtrace.h>
#include <ref.h>

#ifndef NOWAMAS
# include <dbsql.h>
#endif

class ReportException : public std::exception
{

protected:
  std::string err;
  std::string simple_err;
  bool		  bSqlException;
  int         iSqlError;

public:

  ReportException( const std::string & err_ ) :
	  err( err_ ),
	  simple_err(std::string()),
	  bSqlException(false),
	  iSqlError(0)
  {
	  write_warning_message();
  }

  ReportException( const std::string & err_, const std::string & simple_err_ ) :
	  err( err_ ),
	  simple_err(simple_err_),
	  bSqlException(false),
	  iSqlError(0)
  {
	  write_warning_message();
  }

  // Required for gcc 4.8.4
  // error: looser throw specifier for 'virtual ReportException::~ReportException()
  virtual ~ReportException() throw() {
  }

  virtual bool isSqlException () const {
	  return bSqlException;
  }
  
  virtual int getSqlError() const {
    return iSqlError;
  }

  virtual const char *what() const throw() {
	  return err.c_str();
  }
  virtual const char *simple_what() const throw() {
	  if( simple_err.empty() ) {
		  return what();
	  }
	  return simple_err.c_str();
  }

protected:
  ReportException( const std::string & err_,
		  const std::string & simple_err_,
		  bool bSqlException_,
		  int sql_error ) :
			  err( err_ ), simple_err(simple_err_),
			  bSqlException(bSqlException_),
			  iSqlError(sql_error){
	  write_warning_message();
  }

  void write_warning_message() {

	  if (bSqlException == true) {
		  std::cerr << "SqlException thrown! Message: " << err << std::endl;
	  } else {
		  std::cerr << "ReportException thrown! Message: " << err << std::endl;
	  }
  }

};

/**
 * usage:
 * throw REPORT_EXCEPTION( "unknown error" )
 *
 * with C++11 you can optional add the simple error message by yourself
 * throw REPORT_EXCEPTION( "unknown error", MlM("unknown error") )
 *
 * instantiates a ReportException
 * err ... 		the user-text with back-trace
 * simple_err.. the user-text without back-trace
 */
#if __cplusplus < 201103
# define REPORT_EXCEPTION( what ) \
	ReportException( Tools::format( "Exception from: %s:%d:%s message: %s%s", \
		__FILE__, __LINE__, __FUNCTION__, what, Tools::BackTraceHelper::bt.bt() ), \
		Tools::format("%s", what ) )
#else
# define REPORT_EXCEPTION1( what ) \
ReportException( Tools::format( "Exception from: %s:%d:%s message: %s%s", \
	__FILE__, __LINE__, __FUNCTION__, what, Tools::BackTraceHelper::bt.bt() ), \
	Tools::format("%s", what ) )

# define REPORT_EXCEPTION2( what, simple_what ) \
ReportException( Tools::format( "Exception from: %s:%d:%s message: %s (%s)%s", \
	__FILE__, __LINE__, __FUNCTION__, what, simple_what, Tools::BackTraceHelper::bt.bt() ), \
	Tools::format("%s", simple_what ) )

// https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments
#define REPORT_EXCEPTION_EXPAND(x) x
#define REPORT_EXCEPTION_GET_MACRO(_1, _2, NAME, ...) NAME
#define REPORT_EXCEPTION(...) REPORT_EXCEPTION_EXPAND(REPORT_EXCEPTION_GET_MACRO(__VA_ARGS__, REPORT_EXCEPTION2, REPORT_EXCEPTION1)(__VA_ARGS__))
#endif

#endif  /* _TOOLS_REPORT_EXCEPTION_H */
