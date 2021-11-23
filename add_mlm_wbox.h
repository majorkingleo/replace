/*
 * add_mlm.h
 *
 *  Created on: 18.10.2021
 *      Author: martin
 */

#ifndef ADD_MLM_WBOX_H_
#define ADD_MLM_WBOX_H_

#include "HandleFile.h"
#include <set>

class AddMlMWBox : public HandleFile
{
	const std::string FUNCTION_NAME;
	std::set<std::string> key_args;
	static const std::string STRFORM;

public:
	AddMlMWBox(const std::string & FUNCTION_NAME_ = "WamasBox" );


	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

	static bool isNotTranslatable( const std::string & s );
};


#endif /* ADD_MLM_H_ */
