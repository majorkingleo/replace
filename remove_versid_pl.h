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

	std::string remove_versid(  const std::string & file );

private:
	std::string  cut_Header( const std::string & file ) const;

};

#endif /* REMOVE_VERSID_PL_H_ */
