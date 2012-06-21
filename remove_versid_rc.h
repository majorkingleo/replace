#ifndef REMOVE_VERSID_RC_H
#define REMOVE_VERSID_RC_H

#include "remove_versid_pdl.h"

class RemoveVersidRc : public RemoveVersidPdl
{
public:
	RemoveVersidRc();

	std::string remove_versid(  const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );

	virtual std::string patch_file( const std::string & file )
	{
		return remove_versid( file );
	}

protected:

	std::string cut_versid_atom( const std::string & file ) const;
};

#endif

