/*
 * add_mlm.cc
 *
 *  Created on: 18.10.2021
 *      Author: martin
 */
#include "add_mlm_wbox.h"
#include <format.h>
#include "utils.h"
#include "find_decl.h"
#include "CpputilsDebug.h"
#include <sstream>
#include <getline.h>
#include "find_decl.h"
#include <string_utils.h>
#include "unused_variable_handler.h"
#include "add_mlm.h"

using namespace Tools;

const std::wstring AddMlMWBox::STRFORM = L"StrForm";

AddMlMWBox::AddMlMWBox( const std::wstring & FUNCTION_NAME_ )
	: FUNCTION_NAME( FUNCTION_NAME_ )
{
	keywords.push_back( FUNCTION_NAME );

	key_args.insert( L"WboxNbuttonText" );
	key_args.insert( L"WboxNmwmTitle" );
	key_args.insert( L"WboxNtext" );
	key_args.insert( L"WboxNescButtonText" );
	key_args.insert( L"WboxNdefaultButtonText" );
	key_args.insert( L"WboxNfocusButtonText" );
}

std::wstring AddMlMWBox::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	std::wstring res( file );
	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	bool restart_file_patching = false;

	do
	{
		restart_file_patching = false;

		// file already patched ?
		if( res.find( FUNCTION_NAME, start_in_file) == std::string::npos ) {
			return res;
		}

		pos = res.find( FUNCTION_NAME, start_in_file );

		if( pos == std::wstring::npos )
			return res;

		CPPDEBUG( wformat( L"%s at line %d", FUNCTION_NAME, get_linenum(res,pos) ))

		Function func;
		std::wstring::size_type start, end;

		if( !get_function(res,pos,start,end,&func, false) ) {
			CPPDEBUG(wformat(L"unable to load %s function", FUNCTION_NAME) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		end += 1; // skip closing bracket

		if( func.name != FUNCTION_NAME )
		{
			CPPDEBUG( wformat(L"function name is '%s'", func.name) );
			start_in_file = pos + FUNCTION_NAME.size();
			continue;
		}

		bool changed_something = false;

		for( size_t idx = 0; idx < func.args.size(); idx++ ) {
			std::wstring var_name = strip( func.args[idx] );

			if( key_args.count( var_name ) == 0 ) {
				continue;
			}

			if( idx + 1 >= func.args.size() ) {
				continue;
			}

			std::wstring sstring_arg = strip( func.args[idx+1] );

			CPPDEBUG( wformat(L"processing: '%s'", sstring_arg ) );

			// we found a C string, just do normal replacement
			if( sstring_arg.find( L"\"" ) == 0 ) {

				const std::wstring c_string_arg = func.args[idx+1];

				std::wstring::size_type c_start = c_string_arg.find(L"\"");
				std::wstring::size_type c_end = c_string_arg.rfind(L"\"");

				std::wstring leading_spaces = c_string_arg.substr(0,c_start);
				std::wstring trailing_spaces = c_string_arg.substr(c_end+1);

				std::wstring string_arg = wformat( L"%sMlM(%s)%s", leading_spaces, sstring_arg, trailing_spaces );
				func.args[idx+1] = string_arg;

				/*
				if( string_arg.size() > c_string_arg.size() ) {
					DEBUG( format( "increasing end by: %d",  string_arg.size() - c_string_arg.size()));
					end += string_arg.size() - c_string_arg.size();
				}*/

				changed_something = true;
				idx++;
				continue;

			} else if( sstring_arg.find( STRFORM ) == 0 ) {

				CPPDEBUG(wformat(L"%s found", STRFORM ));

				std::wstring::size_type strform_start = 0;
				std::wstring::size_type strform_end = 0;
				std::wstring::size_type strform_pos = 0;

				Function f_strform;

				if( !get_function(sstring_arg,strform_pos,strform_start,strform_end,&f_strform, false) ) {
					CPPDEBUG(wformat(L"unable to load %s function. get_function() failed", STRFORM) );
					start_in_file = pos + FUNCTION_NAME.size();
					continue;
				}

				// skip end bracket
				strform_end += 1;

				if( f_strform.name != STRFORM ) {
					CPPDEBUG(wformat(L"unable to load %s function. Name is: '%s' instead of '%s'", STRFORM, f_strform.name, STRFORM ) );
					start_in_file = pos + FUNCTION_NAME.size();
					continue;
				}

				if( f_strform.args.size() == 0 ) {
					CPPDEBUG(wformat(L"unable to load %s function. Invalid number of arguments: %d", STRFORM, f_strform.args.size() ) );
					start_in_file = pos + FUNCTION_NAME.size();
					continue;
				}

				std::wstring strform_sstring_arg = strip( f_strform.args[0] );

				if( strform_sstring_arg.find( L"\"" ) == 0 && !isNotTranslatable(strform_sstring_arg) ) {

					CPPDEBUG( wformat( L"pure C-string processing for '%s'", strform_sstring_arg ));

					std::wstring & format_arg = f_strform.args[0];

					std::wstring::size_type c_start = format_arg.find(L"\"");
					std::wstring::size_type c_end = format_arg.rfind(L"\"");

					std::wstring leading_spaces = format_arg.substr(0,c_start);
					std::wstring trailing_spaces = format_arg.substr(c_end+1);

					std::wstring string_arg = wformat( L"%sMlMsg(%s)%s", leading_spaces, strform_sstring_arg, trailing_spaces );
					format_arg = string_arg;


					const std::wstring arg_before = func.args[idx+1];
					std::wstring & s_res = func.args[idx+1];

					strform_pos = arg_before.find( STRFORM );

					CPPDEBUG( format( "strform_pos: %d", strform_pos ));

					s_res = function_to_string( arg_before, f_strform,  strform_pos, strform_end + strform_pos );

					CPPDEBUG( wformat( L"arg_before: '%s'", arg_before ));
					CPPDEBUG( wformat( L"s_res: '%s'", s_res ));

					/*
					if( s_res.size() > arg_before.size() ) {
						DEBUG( format( "increasing end by: %d", s_res.size() - arg_before.size()));
						end += s_res.size() - arg_before.size();
					}*/

					changed_something = true;

				} // if
			} else if( sstring_arg.find( L"(" ) == std::wstring::npos ) {

				std::wstring::size_type decl_pos = 0;
				std::wstring decl;

				find_decl( res, pos, sstring_arg, decl, decl_pos );

				if( decl_pos == std::wstring::npos || decl_pos == 0 ) {
					// write a warning message
					func.args[idx+1] += L" /* replace: add MlMsg() */";
					continue;
				}

				if( decl_pos >= pos ) {
					continue;
				}

				CPPDEBUG( wformat( L"'%s' decl: '%s' line: %d", sstring_arg, decl, get_linenum(res,decl_pos)) );

				std::wstring sub_file_start_till_decl = res.substr( 0, decl_pos );
				std::wstring sub_file_middle = res.substr( decl_pos, pos - decl_pos );
				std::wstring sub_file_end = res.substr( pos );

				AddMlM add_mlm_sprintf( L"sprintf", 2, L"MlMsg" );
				add_mlm_sprintf.set_file_name( file_name );
				add_mlm_sprintf.addChecker( new AddMlM::PatchThisFunctionIfArgXequals( 0, sstring_arg ));
				std::wstring new_sub_file_middle = add_mlm_sprintf.patch_file( sub_file_middle );

				AddMlM add_mlm_snprintf( L"snprintf", 3, L"MlMsg" );
				add_mlm_snprintf.addChecker( new AddMlM::PatchThisFunctionIfArgXequals( 0, sstring_arg ));
				add_mlm_snprintf.set_file_name( file_name );
				new_sub_file_middle = add_mlm_snprintf.patch_file( new_sub_file_middle );

				// alle unsere offsets stimmen nicht mehr, daher zueruck zum Start
				if( new_sub_file_middle != sub_file_middle ) {
					CPPDEBUG( "restart" );
					res = sub_file_start_till_decl + new_sub_file_middle + sub_file_end;
					restart_file_patching = true;
					break;
				}
			} // else if
		} // for


		if( restart_file_patching ) {
			start_in_file = 0;
			pos = 0;
			continue;
		}

		if( changed_something )
		{
			res = function_to_string( res, func, pos, end );
			start_in_file = end;
		}


		if( !changed_something ) {
			start_in_file = pos + FUNCTION_NAME.size();
		}

	} while( restart_file_patching || (pos != std::string::npos && start_in_file < res.size()) );


	return res;
}

bool AddMlMWBox::isNotTranslatable( const std::wstring & s )
{
	static std::set<std::wstring> EMPTY_STRINGS;

	if( EMPTY_STRINGS.empty() ) {
		EMPTY_STRINGS.insert( L"\"\"" );
		EMPTY_STRINGS.insert( L"\"%s\"" );
		EMPTY_STRINGS.insert( L"\"%s\\n%s\"" );
		EMPTY_STRINGS.insert( L"\"%s%s\"" );
		EMPTY_STRINGS.insert( L"\" \"" );
	}

	if( EMPTY_STRINGS.find( s ) != EMPTY_STRINGS.end() ) {
		return true;
	}

	return false;
}

bool AddMlMWBox::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

