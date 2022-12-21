/*
 * reset_versid.h
 *
 *  Created on: 21.12.2022
 *      Author: martin
 */

#ifndef RESET_VERSID_H_
#define RESET_VERSID_H_

#include "HandleFile.h"

class ResetVersid : public HandleFile
{
public:
	ResetVersid();

	std::wstring reset_versid(  const std::wstring & file, const std::wstring & KEYWORD );

	bool want_file( const FILE_TYPE & file_type ) override;

	std::wstring patch_file( const std::wstring & file ) override
	{
		std::wstring res = file;

		res = reset_versid( res, L"$Header:" );
		res = reset_versid( res, L"$Locker:" );
		res = reset_versid( res, L"$Id:" );
		res = reset_versid( res, L"$Log:" );
		res = reset_versid( res, L"$Revision:" );
		res = reset_versid_log( res );

		return res;
	}

protected:
	// removes last log entry
	std::wstring reset_versid_log( const std::wstring & file );
	std::wstring reset_versid_once(  const std::wstring & file, const std::wstring & KEYWORD );
};


#endif /* RESET_VERSID_H_ */

