/*
 * add_mlm.h
 *
 *  Created on: 18.10.2021
 *      Author: martin
 */

#ifndef ADD_MLM_H_
#define ADD_MLM_H_

#include "HandleFile.h"

class Function;

class AddMlM : public HandleFile
{
public:
	class ShouldIPatchThisFunction
	{
	public:
		virtual ~ShouldIPatchThisFunction() {}

		virtual bool should_i_patch_this_function( const Function & func ) = 0;
	};

	class PatchThisFunctionIfArgXequals : public ShouldIPatchThisFunction
	{
		const unsigned ARG_NUM;
		const std::string VAR_NAME;

	public:
		PatchThisFunctionIfArgXequals( unsigned arg_num_, const std::string & var_name_ )
		: ARG_NUM( arg_num_ ),
		  VAR_NAME( var_name_ )
		{}


		virtual bool should_i_patch_this_function( const Function & func );
	};

protected:
	const std::string FUNCTION_NAME;
	const unsigned    FUNCTION_ARG_NUM;
	const std::string FUNCTION_CALL;
	std::vector<ShouldIPatchThisFunction*> vChecker;

public:
	AddMlM(const std::string & FUNCTION_NAME_,
		   const unsigned      FUNCTION_ARG_NUM_,
		   const std::string & FUNCTION_CALL_ );

	~AddMlM();

	/*
	 * allocate checker with new, the will be deleted automatically
	 */
	void addChecker( ShouldIPatchThisFunction * checker ) {
		vChecker.push_back( checker );
	}

	virtual std::string patch_file( const std::string & file );

	virtual bool want_file( const FILE_TYPE & file_type );
};


#endif /* ADD_MLM_H_ */
