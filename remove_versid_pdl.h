#ifndef REMOVE_VERSID_PDL_H
#define REMOVE_VERSID_PDL_H

#include "remove_versid_class.h"

class RemoveVersidPdl : public RemoveVersid
{
public:
	RemoveVersidPdl();

	std::string remove_versid(  const std::string & file );

protected:
	std::string cut_revision_history( const std::string & file ) const;
	std::string cut_VERSID( const std::string & file ) const;
	std::string cut_versid_function( const std::string & file ) const;
};

#endif
