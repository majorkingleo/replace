/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */

#ifndef _wamas_REPORT_EXCEPTION_H
#define _wamas_REPORT_EXCEPTION_H

#include <backtrace.h>
#include <ref.h>

#ifndef NOWAMAS
# include <dbsql.h>
#endif

class ReportException : public std::exception
{
  std::string err;
  std::string simple_err;

public:
  ReportException( const std::string & err_ ) : err( err_ ), simple_err(std::string()) { write_warning_message(); }
  ReportException( const std::string & err_, const std::string & simple_err_ ) : err( err_ ), simple_err(simple_err_) { write_warning_message(); }
  ~ReportException() throw() {}
  
  virtual const char *what() const throw() { return err.c_str(); }
  virtual const char *simple_what() const throw() { return simple_err.c_str(); }

protected:
  void write_warning_message();
};

#ifndef NOWAMAS

#define SQL_EXCEPTION( tid ) \
ReportException( Tools::format( "SqlError from: %s:%d:%s %s%s",\
								__FILE__, __LINE__, __FUNCTION__, \
								TSqlErrTxt(tid),					\
								Tools::BackTraceHelper::bt.bt() ), TSqlErrTxt(tid) )

#endif

#define REPORT_EXCEPTION( what ) \
ReportException( Tools::format( "Exception from: %s:%d:%s message: %s%s", \
								__FILE__, __LINE__, __FUNCTION__, what, Tools::BackTraceHelper::bt.bt() ), \
				 Tools::format("%s", what ) )


#endif  /* _wamas_REPORT_EXCEPTION_H */
