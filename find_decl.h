/*
 * find_decl.h
 *
 *  Created on: 10.01.2020
 *      Author: martin
 */

#ifndef FIND_DECL_H_
#define FIND_DECL_H_

#include <string>


std::wstring find_decl( const std::wstring &s,
				      std::wstring::size_type start_,
				      const std::wstring & name,
				      std::wstring & decl,
				      std::wstring::size_type & at_pos );


#endif /* FIND_DECL_H_ */
