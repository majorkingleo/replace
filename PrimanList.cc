#include "PrimanList.h"
#include "find_first_of.h"
#include "utils.h"
#include <string_utils.h>
#include <ostream>
#include <format.h>
#include <debug.h>
#include <ctype.h>
#include <cpp_util.h>

using namespace Tools;

namespace {
	std::wostream & operator<<( std::wostream & out, const PrimanList::SEL_CALLBACK & type )
	{
		switch( type )
		{
		case PrimanList::SEL_CALLBACK::AT_POS:    out << L"AT_POS";    break;
		case PrimanList::SEL_CALLBACK::IS_NULL:   out << L"IS_NULL";   break;
		case PrimanList::SEL_CALLBACK::NOT_FOUND: out << L"NOT_FOUND"; break;
		case PrimanList::SEL_CALLBACK::FIRST__: break;
		case PrimanList::SEL_CALLBACK::LAST__: break;
		}

		return out;
	}
} // namespace

PrimanList::PrimanList()
{
	keywords.push_back( L"ListTXdialog" );
	// test
	// test 2
}

std::wstring PrimanList::toString( const SEL_CALLBACK & type )
{
	switch( type )
	{
	case PrimanList::SEL_CALLBACK::AT_POS:   return  L"AT_POS";    break;
	case PrimanList::SEL_CALLBACK::IS_NULL:  return  L"IS_NULL";    break;
	case PrimanList::SEL_CALLBACK::NOT_FOUND: return L"NOT_FOUND"; break;
	case PrimanList::SEL_CALLBACK::FIRST__: break;
	case PrimanList::SEL_CALLBACK::LAST__: break;
	}

	return std::wstring();
}

std::wstring PrimanList::include_primanlist( const std::wstring & file )
{
	std::wstring::size_type pos = 0;

	while (pos != std::wstring::npos) {
		pos = file.find( L"#include", pos );

		if( pos == std::wstring::npos )
			return file;

		if( is_in_string( file, pos ) )
			return file;

		std::wstring::size_type begin_of_line = file.rfind(L'\n',pos);

		if( pos == 0 )
			begin_of_line =  0;

		DEBUG( get_whole_line(file,begin_of_line-1) );

		if( get_whole_line(file,begin_of_line-1).find(L"#ifdef") != std::wstring::npos )
		{
			DEBUG(format("#ifdef found at line %d",get_linenum(file,pos-1)));
			pos = file.find(L"#endif", pos+1)+6;
			continue;
		}

		if( begin_of_line == std::wstring::npos )
			begin_of_line = 0;

		std::wstring::size_type cut_pos;
		std::wstring extra;

		if( begin_of_line > 0 ) {
			cut_pos = begin_of_line+1;
		} else {
			cut_pos = 0;
			extra = L"\n";
		}

		std::wstring result = file.substr( 0, cut_pos);
		result += L"#include <primanlist.h>" + extra;
		result += file.substr(begin_of_line);

		return result;
	}

	return file;
}

PrimanList::SEL_CALLBACK  PrimanList::detect_sel_callback(
		const std::wstring & file,
		std::wstring::size_type & pos)
{
	static const std::wstring function = L"sel_callback";

	pos = 0;
	std::wstring::size_type start = pos;

	while( pos != std::wstring::npos )
	{
		pos = file.find(function, start);

		if( pos == std::wstring::npos ) {
			DEBUG( "sel_callback not found");
			return SEL_CALLBACK::NOT_FOUND;
		}

		if( is_in_comment( file, pos ) ) {
			DEBUG( "sel_callback is in comment");
			start = pos + function.size();
			continue;
		}

		if( is_in_string( file, pos ) ) {
			DEBUG( "sel_callback is in string");
			start = pos + function.size();
			continue;
		}

		if( isalpha( file[pos-1] ) ||
			isalpha( file[pos+function.size()] ) ||
			file[pos-1] == L'_' ||
			file[pos-1] == L'$' ||
			file[pos+function.size()] == L'_')
		{
			start = pos + function.size();
			continue;
		}

		std::wstring line = get_whole_line(file,pos);

		if( line.find(L"=") == std::wstring::npos ) {			;
			start = pos + function.size();
			continue;
		}

		if( line.find(L"NULL") != std::wstring::npos )
			return SEL_CALLBACK::IS_NULL;

		std::vector<std::wstring> sl = split_simple( line, L"=");

		std::wstring callback_name = strip(*sl.rbegin(), L";\t\r ");

		DEBUG(wformat(L"try to find function: %s",callback_name));

		std::wstring::size_type pos2 = 0;

		while( pos2 != std::wstring::npos )
		{
			pos2 = find_function(callback_name,file,pos2);

			if( pos2 != std::wstring::npos )
			{
				std::wstring func_line = get_whole_line(file,pos2);

				if( is_in_comment( file, pos2 ) ) {
					DEBUG( wformat(L"line: >%s< is a comment", func_line));
					// comment line
					pos2 += callback_name.size();
				} else {
					DEBUG( wformat(L"found %s call at line %d", func_line, get_linenum(file,pos2)));
					break;
				}
			}
		}

		pos = pos2;

		if( pos2 != std::wstring::npos )
		{
			return SEL_CALLBACK::AT_POS;
		}
	}

	return SEL_CALLBACK::NOT_FOUND;
}

std::wstring PrimanList::patch_null_selcallback( const std::wstring & file, std::wstring::size_type pos )
{
	std::wstring::size_type begin = file.find(L'=', pos);
	std::wstring::size_type end = file.find(L';', begin);

	std::wstring result = file.substr(0,begin+1);
	result += L" PrimanListSelCallbackLocal;";
	result += file.substr(end+1);

	return result;
}

std::wstring PrimanList::get_varname( const std::wstring & var )
{
	for(int pos = var.length() - 1; pos >= 0; pos-- )
	{
		if( !isalnum(var[pos]) && var[pos] != L'_' )
			return var.substr(pos+1);
	}

	return var;
}

void PrimanList::strip_argtypes( Function & f )
{
	for( unsigned i = 0; i < f.args.size(); i++ )
	{
		f.args[i] = get_varname( f.args[i] );
	}
}

std::wstring PrimanList::add_selcallback_to_reasons( const std::wstring & file, std::wstring::size_type pos )
{
	Function func;
	std::wstring::size_type start, end;

	DEBUG( wformat(L"line %d: %s", get_linenum(file, pos ),get_whole_line(file,pos) ));


	if( !get_function(file,pos,start,end,&func) ) {
		DEBUG("unable to load callback function");
		return file;
	}

	DEBUG( wformat(L"function args: %d", func.args.size() ) );

	if( func.args.size() != 5 ) {
		throw REPORT_EXCEPTION( format("cannot find the correct number of arguments for function %s", w2out(func.name) ) );
	}

	strip_argtypes(func);

	DEBUG(wformat(L"found function %s at %d up to %d", func.name, pos, start, end));

	pos = file.find( L"case", start);

	if( pos == std::string::npos ) {
		DEBUG("cannot find position for inserting Priman Callback reason");
		return file;
	}

	std::wstring line = get_whole_line( file, pos);

	// das machen wird dazu damit wir wissen
	// wieviel wir einrücken sollen
	// damits auch besser aussieht
	std::wstring::size_type start_of_case = find_first_of( line, 0, L"case", L"default" );
	std::wstring indent = line.substr(0,start_of_case);

	pos = file.rfind(L'\n', pos);

	std::wstringstream str;
	str << file.substr(0,pos);

	str << L'\n';
	str << L'\n';
	str << indent;
	str << L"case SEL_REASON_PRINT:\n";
	str << indent;
	str << L"\treturn PrimanListSelCallbackLocal(";

	for( unsigned i = 0; i < func.args.size(); i++ )
	{
		if( i > 0 )
			str << L", ";

		str << func.args[i];
	}

	str << L");\n";

	str << file.substr(pos);

	return str.str();
}

std::wstring PrimanList::insert_selcallback( const std::wstring & file )
{
	static const std::wstring action_type = L"ListTaction";
	std::wstring::size_type pos = 0;

	while( pos != std::wstring::npos )
	{
		pos = file.find(action_type, pos);

		if( pos == std::wstring::npos ) {
			break;
		}

		if( is_in_string(file,pos) ) {
			pos += action_type.length();
			continue;
		}

		std::wstring line = get_whole_line( file, pos );

		if( line.find(L"memset") == std::wstring::npos ) {
			pos += action_type.length();
			continue;
		}

		std::wstring::size_type memset_pos = file.rfind(L"memset", pos );
		Function func_memset;
		std::wstring::size_type start, end;

		if( !get_function(file, memset_pos, start, end, &func_memset) )
		{
			DEBUG("cannot load memset line");
			pos += action_type.length();
			continue;
		}

		// in der nächsten Zeile
		// pListAction->sel_callback = PrimanListSelCallbackLocal
		// hinzufügen

		std::wstring::size_type next_line = file.find(L'\n', pos);

		std::wstringstream str;

		str << file.substr( 0, next_line );

		std::wstring indent = line.substr(0,line.find(L"memset"));

		str << L'\n';
		str << indent;
		str << *func_memset.args.begin();
		str << L"->sel_callback = PrimanListSelCallbackLocal;";

		str << file.substr( next_line );

		return str.str();
	}

	return file;
}

std::wstring PrimanList::patch_file( const std::wstring & file )
{
	if( should_skip_file( file ))
		return file;

	// file already patched ?
	if( find_first_of(file, 0,
				  L"primanlist.h",
				  L"SEL_REASON_PRINT:") != std::wstring::npos ) {
		return file;
	}

	std::wstring result = include_primanlist( file );

	std::wstring::size_type pos = 0;

	SEL_CALLBACK sc_type = detect_sel_callback(file,pos);

	DEBUG( wformat(L"ret: %s pos: %d", toString(sc_type), pos) );

	switch( sc_type )
	{
		case SEL_CALLBACK::FIRST__:   break;
		case SEL_CALLBACK::LAST__:    break;
		case SEL_CALLBACK::IS_NULL:   result = patch_null_selcallback( result, pos ); break;
		case SEL_CALLBACK::AT_POS:    result = add_selcallback_to_reasons( result, pos ); break;
		case SEL_CALLBACK::NOT_FOUND: result = insert_selcallback( result ); break;
	}

	return result;
}

bool PrimanList::want_file( const FILE_TYPE & file_type )
{
	switch( file_type )
	{
	  case FILE_TYPE::C_FILE:
		  return true;
	  default:
		  return false;
	}
}

bool PrimanList::is_in_comment( const std::wstring & file, std::wstring::size_type pos )
{
	std::wstring func_line = get_whole_line(file,pos);

	if( func_line.find(L"-*") == 0 ) {
		return true;
	}

	return false;
}
