#ifndef REMOVE_VERSID_CH_H
#define REMOVE_VERSID_CH_H

#include "remove_versid_class.h"

class RemoveVersidCh : public RemoveVersid
{
	bool noheader;
	std::wstring header_template;

public:
	RemoveVersidCh( bool noheader );

	std::wstring remove_versid(  const std::wstring & file );

	bool want_file( const FILE_TYPE & file_type ) override;

	std::wstring patch_file( const std::wstring & file ) override
	{
		return remove_versid( file );
	}

protected:

	std::wstring make_header_template() const;
	std::wstring cut_revision_history( const std::wstring & file ) const;
	std::wstring cut_versid_h( const std::wstring & file ) const;
	std::wstring cut_versid_ch_makro( const std::wstring & file ) const;
	std::wstring cut_the_easy_stuff( const std::wstring & file ) const;
	std::wstring cut_static_versid( const std::wstring & file ) const;
};


#endif
