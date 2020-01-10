/*
 * find_decl.h
 *
 *  Created on: 10.01.2020
 *      Author: martin
 */

#ifndef FIND_DECL_H_
#define FIND_DECL_H_

#include <string>


std::string find_decl( const std::string &s,
				      std::string::size_type start_,
				      const std::string & name,
				      std::string & decl,
				      std::string::size_type & at_pos );


#endif /* FIND_DECL_H_ */
