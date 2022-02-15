#ifndef REMOVE_VERSID_PDL_H
#define REMOVE_VERSID_PDL_H

#include "remove_versid_class.h"

class RemoveVersidPdl : public RemoveVersid
{
public:
	RemoveVersidPdl();

	std::wstring remove_versid(  const std::wstring & file );

	bool want_file( const FILE_TYPE & file_type ) override;

	std::wstring patch_file( const std::wstring & file ) override
	{
		return remove_versid( file );
	}

protected:
	std::wstring cut_revision_history( const std::wstring & file ) const;
	std::wstring cut_VERSID( const std::wstring & file ) const;
	std::wstring cut_versid_function( const std::wstring & file ) const;
	std::wstring add_eof( const std::wstring & file ) const;
};

#endif
