/*
 * format_string_handler_gcc48.h
 *
 *  Created on: 04.07.2016
 *      Author: martin
 */

#ifndef FORMAT_STRING_HANDLER_GCC48_H_
#define FORMAT_STRING_HANDLER_GCC48_H_

#include "format_string_handler.h"


class FormatStringHandlerGcc48 : public FormatStringHandler
{
public:
	FormatStringHandlerGcc48();

	bool is_interested_in_line( const std::wstring & line ) override;

	void strip_target_type( FormatWarnigs & location ) override;
};


#endif /* FORMAT_STRING_HANDLER_GCC48_H_ */
