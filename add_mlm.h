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
		const std::wstring VAR_NAME;

	public:
		PatchThisFunctionIfArgXequals( unsigned arg_num_, const std::wstring & var_name_ )
		: ARG_NUM( arg_num_ ),
		  VAR_NAME( var_name_ )
		{}


		virtual bool should_i_patch_this_function( const Function & func );
	};

protected:
	const std::wstring FUNCTION_NAME;
	const unsigned    FUNCTION_ARG_NUM;
	const std::wstring FUNCTION_CALL;
	std::vector<ShouldIPatchThisFunction*> vChecker;

public:
	AddMlM(const std::wstring & FUNCTION_NAME_,
		   const unsigned      FUNCTION_ARG_NUM_,
		   const std::wstring & FUNCTION_CALL_ );

	~AddMlM();

	/*
	 * allocate checker with new, the will be deleted automatically
	 */
	void addChecker( ShouldIPatchThisFunction * checker ) {
		vChecker.push_back( checker );
	}

	std::wstring patch_file( const std::wstring & file ) override;

	bool want_file( const FILE_TYPE & file_type ) override;
};


#endif /* ADD_MLM_H_ */
