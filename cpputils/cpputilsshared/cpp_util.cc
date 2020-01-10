/**
 * @file
 * @todo describe file content
 * @author Copyright (c) 2009 Salomon Automation GmbH
 */
#include <iostream>
#include "cpp_util.h"

void ReportException::write_warning_message()
{
  std::cerr << "Exception thrown! Message: " << err << std::endl;
}
