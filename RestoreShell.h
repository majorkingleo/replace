/*
 * RestoreShell.h
 *
 *  Created on: 21.06.2012
 *      Author: martin
 */

#ifndef RESTORESHELL_H_
#define RESTORESHELL_H_

#include "HandleFile.h"

class RestoreShell : public HandleFile
{
public:
	RestoreShell();


	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;
};


#endif /* RESTORESHELL_H_ */
