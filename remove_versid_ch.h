#ifndef REMOVE_VERSID_CH_H
#define REMOVE_VERSID_CH_H

#include "remove_versid_class.h"

class RemoveVersidCh : public RemoveVersid
{
	bool noheader;
	std::string header_template;

public:
	RemoveVersidCh( bool noheader );

	std::string remove_versid(  const std::string & file );

protected:

	std::string make_header_template() const;
	std::string cut_revision_history( const std::string & file ) const;
	std::string cut_versid_h( const std::string & file ) const;
	std::string cut_versid_ch_makro( const std::string & file ) const;
	std::string cut_the_easy_stuff( const std::string & file ) const;
	std::string cut_static_versid( const std::string & file ) const;
};


#endif
