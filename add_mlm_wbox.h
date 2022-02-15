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
	const std::wstring FUNCTION_NAME;
	std::set<std::wstring> key_args;
	static const std::wstring STRFORM;

public:
	AddMlMWBox(const std::wstring & FUNCTION_NAME_ = L"WamasBox" );


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

	static bool isNotTranslatable( const std::wstring & s );
};


#endif /* ADD_MLM_H_ */
