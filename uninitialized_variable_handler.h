/*
 * uninitialized_variable_handler.h
 *
 *  Created on: 17.02.2014
 *      Author: martin
 */

#ifndef UNINITIALIZED_VARIABLE_HANDLER_H_
#define UNINITIALIZED_VARIABLE_HANDLER_H_

#include "unused_variable_handler.h"

class UninitializedVariableHandler : public UnusedVariableHandler
{
public:
	UninitializedVariableHandler();

	void read_compile_log_line( const std::string & line );

	void fix_warning( UnusedVarWarnigs & warning, std::string & content );
};


#endif /* UNINITIALIZED_VARIABLE_HANDLER_H_ */
