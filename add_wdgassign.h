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
	const std::string SHELL_CREATE_FUNCTION;
public:
	AddWamasWdgAssignMenu( const std::string & assign_function );

	virtual std::string patch_file( const std::string & file );
	virtual bool want_file( const FILE_TYPE & file_type );
};



#endif /* ADD_WDGASSIGN_H_ */
