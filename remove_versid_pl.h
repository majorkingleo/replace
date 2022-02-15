/*
 * remove_versid_pl.h
 *
 *  Created on: 09.12.2011
 *      Author: martin
 */

#ifndef REMOVE_VERSID_PL_H_
#define REMOVE_VERSID_PL_H_

#include "remove_versid_pdl.h"

class RemoveVersidPl : public RemoveVersidPdl
{
public:
	RemoveVersidPl();

	std::wstring remove_versid(  const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;

	std::wstring patch_file( const std::wstring & file ) override
	{
		return remove_versid( file );
	}

private:
	std::wstring  cut_Header( const std::wstring & file ) const;

};

#endif /* REMOVE_VERSID_PL_H_ */
