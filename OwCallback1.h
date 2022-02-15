/*
 * OwCallback1.h
 *
 *  Created on: 19.04.2012
 *      Author: martin
 */

#ifndef OWCALLBACK1_H_
#define OWCALLBACK1_H_

#include "HandleFile.h"


class OwCallback1 : public HandleFile
{
public:
	OwCallback1();

	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

protected:
	void fix_owcallbackx( std::wstring & file_content, int num );
};


#endif /* OWCALLBACK1_H_ */
