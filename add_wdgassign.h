/*
 * add_wdgassign.h
 *
 *  Created on: 04.07.2014
 *      Author: martin
 */

#ifndef ADD_WDGASSIGN_H_
#define ADD_WDGASSIGN_H_

#include "HandleFile.h"


class AddWamasWdgAssignMenu : public HandleFile
{
	const std::wstring SHELL_CREATE_FUNCTION;
public:
	AddWamasWdgAssignMenu( const std::wstring & assign_function );

	std::wstring patch_file( const std::wstring & file ) override;
	virtual bool want_file( const FILE_TYPE & file_type );
};



#endif /* ADD_WDGASSIGN_H_ */
