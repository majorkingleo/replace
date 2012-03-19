#include "PrimanList.h"
#include "find_first_of.h"
#include "utils.h"
#include <string_utils.h>
#include <ostream>
#include <format.h>
#include <debug.h>
#include <ctype.h>

using namespace Tools;

namespace {
	std::ostream & operator<<( std::ostream & out, const PrimanList::SEL_CALLBACK & type )
	{
		switch( type )
		{
		case PrimanList::SEL_CALLBACK::AT_POS:    out << "AT_POS";    break;
		case PrimanList::SEL_CALLBACK::IS_NULL:   out << "IS_NULL";   break;
		case PrimanList::SEL_CALLBACK::NOT_FOUND: out << "NOT_FOUND"; break;
		case PrimanList::SEL_CALLBACK::FIRST__: break;
		case PrimanList::SEL_CALLBACK::LAST__: break;
		}

		return out;
	}
} // namespace

PrimanList::PrimanList()
{
	keywords.push_back( "ListTXdialog" );
}

std::string PrimanList::toString( const SEL_CALLBACK & type )
{
	switch( type )
	{
	case PrimanList::SEL_CALLBACK::AT_POS:   return "AT_POS";    break;
	case PrimanList::SEL_CALLBACK::IS_NULL:  return"IS_NULL";    break;
	case PrimanList::SEL_CALLBACK::NOT_FOUND: return "NOT_FOUND"; break;
	case PrimanList::SEL_CALLBACK::FIRST__: break;
	case PrimanList::SEL_CALLBACK::LAST__: break;
	}

	return std::string();
}

std::string PrimanList::include_primanlist( const std::string & file )
{
	std::string::size_type pos = 0;

	while (pos != std::string::npos) {
		pos = file.find( "#include", pos );

		if( pos == std::string::npos )
			return file;

		if( is_in_string( file, pos ) )
			return file;

		std::string::size_type begin_of_line = file.rfind('\n',pos);

		DEBUG( get_whole_line(file,begin_of_line-1) );

		if( get_whole_line(file,begin_of_line-1).find("#ifdef") != std::string::npos )
		{
			DEBUG(format("#ifdef found at line %d",get_linenum(file,pos-1)));
			pos = file.find("#endif", pos+1)+6;
			continue;
		}

		if( begin_of_line == std::string::npos )
			begin_of_line = 0;

		std::string result = file.substr( 0, begin_of_line +1);
		result += "#include <primanlist.h>";
		result += file.substr(begin_of_line);

		return result;
	}

	return file;
}

PrimanList::SEL_CALLBACK  PrimanList::detect_sel_callback(
		const std::string & file,
		std::string::size_type & pos)
{
	static const std::string function = "sel_callback";

	pos = 0;
	std::string::size_type start = pos;

	while( pos != std::string::npos )
	{
		pos = file.find(function, start);

		if( pos == std::string::npos ) {
			DEBUG( "sel_callback not found");
			return SEL_CALLBACK::NOT_FOUND;
		}

		if( is_in_string( file, pos ) ) {
			DEBUG( "sel_callback is in string");
			return SEL_CALLBACK::NOT_FOUND;
		}

		if( isalpha( file[pos-1] ) ||
			isalpha( file[pos+function.size()] ) ||
			file[pos-1] == '_' ||
			file[pos-1] == '$' ||
			file[pos+function.size()] == '_')
		{
			start = pos + function.size();
			continue;
		}

		std::string line = get_whole_line(file,pos);

		if( line.find("NULL") != std::string::npos )
			return SEL_CALLBACK::IS_NULL;

		std::vector<std::string> sl = split_simple( line, "=");

		std::string callback_name = strip(*sl.rbegin(),";\t ");

		DEBUG(format("try to find function: %s",callback_name));
		pos = find_function(callback_name,file,0);

		if( pos != std::string::npos )
			return SEL_CALLBACK::AT_POS;
	}

	return SEL_CALLBACK::NOT_FOUND;
}

std::string PrimanList::patch_null_selcallback( const std::string & file, std::string::size_type pos )
{
	std::string::size_type begin = file.find('=', pos);
	std::string::size_type end = file.find(';', begin);

	std::string result = file.substr(0,begin+1);
	result += " PrimanListSelCallbackLocal;";
	result += file.substr(end+1);

	return result;
}

std::string PrimanList::get_varname( const std::string & var )
{
	for(int pos = var.length() - 1; pos >= 0; pos-- )
	{
		if( !isalnum(var[pos]) && var[pos] != '_' )
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

std::string PrimanList::add_selcallback_to_reasons( const std::string & file, std::string::size_type pos )
{
	Function func;
	std::string::size_type start, end;

	if( !get_function(file,pos,start,end,&func) ) {
		DEBUG("unable to load callback function");
		return file;
	}

	strip_argtypes(func);

	DEBUG(format("found function %s at %d up to %d", func.name, pos, start, end));

	pos = file.find( "case", start);

	if( pos == std::string::npos ) {
		DEBUG("cannot find position for inserting Priman Callback reason");
		return file;
	}

	std::string line = get_whole_line( file, pos);

	// das machen wird dazu damit wir wissen
	// wieviel wir einr�cken sollen
	// damits auch besser aussieht
	std::string::size_type start_of_case = find_first_of( line, 0, "case", "default" );
	std::string indent = line.substr(0,start_of_case);

	pos = file.rfind('\n', pos);

	std::stringstream str;
	str << file.substr(0,pos);

	str << '\n';
	str << '\n';
	str << indent;
	str << "case SEL_REASON_PRINT:\n";
	str << indent;
	str << "\treturn PrimanListSelCallbackLocal(";

	for( unsigned i = 0; i < func.args.size(); i++ )
	{
		if( i > 0 )
			str << ", ";

		str << func.args[i];
	}

	str << ");\n";

	str << file.substr(pos);

	return str.str();
}

std::string PrimanList::insert_selcallback( const std::string & file )
{
	static const std::string action_type = "ListTaction";
	std::string::size_type pos = 0;

	while( pos != std::string::npos )
	{
		pos = file.find(action_type, pos);

		if( is_in_string(file,pos) ) {
			pos += action_type.length();
			continue;
		}

		std::string line = get_whole_line( file, pos );

		if( line.find("memset") == std::string::npos ) {
			pos += action_type.length();
			continue;
		}

		std::string::size_type memset_pos = file.rfind("memset", pos );
		Function func_memset;
		std::string::size_type start, end;

		if( !get_function(file, memset_pos, start, end, &func_memset) )
		{
			DEBUG("cannot load memset line");
			pos += action_type.length();
			continue;
		}

		// in der n�chsten Zeile
		// pListAction->sel_callback = PrimanListSelCallbackLocal
		// hinzuf�gen

		std::string::size_type next_line = file.find('\n', pos);

		std::stringstream str;

		str << file.substr( 0, next_line );

		std::string indent = line.substr(0,line.find("memset"));

		str << '\n';
		str << indent;
		str << *func_memset.args.begin();
		str << "->sel_callback = PrimanListSelCallbackLocal;";

		str << file.substr( next_line );

		return str.str();
	}

	return file;
}

std::string PrimanList::patch_file( const std::string & file )
{
	if( should_skip_file( file ))
		return file;

	// file already patched ?
	if( find_first_of(file, 0,
				  "primanlist.h",
				  "SEL_REASON_PRINT:") != std::string::npos ) {
		return file;
	}

	std::string result = include_primanlist( file );

	std::string::size_type pos;

	SEL_CALLBACK sc_type = detect_sel_callback(file,pos);

	DEBUG( format("ret: %s pos: %d", toString(sc_type), pos) );

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
