#ifndef REMOVE_VERSID_RC_H
#define REMOVE_VERSID_RC_H

#include "remove_versid_pdl.h"

class RemoveVersidRc : public RemoveVersidPdl
{
public:
	RemoveVersidRc();

	std::wstring remove_versid(  const std::wstring & file );

	bool want_file( const FILE_TYPE & file_type ) override;

	std::wstring patch_file( const std::wstring & file ) override
	{
		return remove_versid( file );
	}

protected:

	std::wstring cut_versid_atom( const std::wstring & file ) const;
};

#endif

