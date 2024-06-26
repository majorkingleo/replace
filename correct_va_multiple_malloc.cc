/*
 * correct_va_multiple_malloc.cc
 *
 *  Created on: 04.06.2013
 *      Author: martin
 */

#include "correct_va_multiple_malloc.h"
#include <format.h>
#include "utils.h"
#include "CpputilsDebug.h"
#include <sstream>
#include <regex_match.h>

using namespace Tools;

const std::wstring CorrectVaMultipleMalloc::KEY_WORD = L"(void**)";
const std::wstring CorrectVaMultipleMalloc::VA_MALLOC = L"VaMultipleMalloc";

CorrectVaMultipleMalloc::CorrectVaMultipleMalloc(bool isTB2020_)
: isTB2020(isTB2020_)
{
	keywords.push_back(L"VaMultipleMalloc");
}

static bool starts_with(const std::wstring &line, const std::wstring &match)
{
	return line.find(match) == 0;
}

std::wstring CorrectVaMultipleMalloc::patch_file(const std::wstring &file)
{
	if (should_skip_file(file)) {
		return file;
	}

	std::wstring res(file);
	std::wstring::size_type start_in_file = 0;
	std::wstring::size_type pos = 0;

	do {
		// file already patched ?
		if (res.find(KEY_WORD, start_in_file) == std::wstring::npos) {
			return res;
		}

		pos = res.find(VA_MALLOC, start_in_file);

		if (pos == std::wstring::npos) {
			return res;
		}

		CPPDEBUG(format("%s at line %d", w2out(VA_MALLOC), get_linenum(res, pos)));

		Function func;
		std::wstring::size_type start, end;

		if (!get_function(res, pos, start, end, &func, false)) {
			CPPDEBUG("unable to load VaMultipleMalloc function");
			start_in_file = pos + VA_MALLOC.size();
			continue;
		}

		bool changed_something = false;

		if (isTB2020) {
			for (unsigned i = 0; i < func.args.size(); i += 2) {
				std::wstring &arg = func.args[i];
				std::wstring int_cast = L"(int)";

				std::wstring::size_type int_pos = arg.find(int_cast);

				if (int_pos != std::wstring::npos) {
					std::wstring new_arg;

					if (int_pos > 0) {
						new_arg = arg.substr(0, int_pos);
					}

					if (!starts_with(arg.substr(int_pos + int_cast.length()), L"sizeof")) {
						if (starts_with(arg.substr(int_pos + int_cast.length()), L"(-1)")) {
							new_arg += L"(ssize_t)";
						} else {
							new_arg += L"(size_t)";
						}
					}

					new_arg += arg.substr(int_pos + int_cast.size());

					CPPDEBUG( format("arg %d '%s' => '%s'", i, w2out(arg),	w2out(new_arg)));

					arg = new_arg;

					changed_something = true;
				}
			}
		}

		// wir dürfen nur ab dem 3. Argument das void** wegreissen, denn
		// sonst kommt es zu einem Compilerfehler.
		for (unsigned i = (isTB2020 ? 1 : 3); i < func.args.size(); i += 2) {
			std::wstring &arg = func.args[i];

			std::wstring::size_type arg_pos = arg.find(KEY_WORD);

			if (arg_pos != std::wstring::npos) {
				std::wstring new_arg;

				if (arg_pos > 0) {
					new_arg = arg.substr(0, arg_pos);
				}

				new_arg += arg.substr(arg_pos + KEY_WORD.size());

				CPPDEBUG( format("arg %d '%s' => '%s'", i, w2out(arg), w2out(new_arg)) );

				arg = new_arg;

				changed_something = true;
			}
		}

		if (changed_something) {
			std::wstring first_part_of_file = res.substr(0, pos);

			std::wstringstream str;

			str << func.name << L"(";

			for (unsigned i = 0; i < func.args.size(); i++) {
				if (i > 0) {
					str << L",";

					if( func.args[i][0] == L'\n' ) {

					} else if( !regex_match( L"\\s.*", func.args[i] ) ) {
						str << L" ";
					}

				}

				str << func.args[i];
			}

			CPPDEBUG(str.str());

			std::wstring second_part_of_file = res.substr(end);

			res = first_part_of_file + str.str() + second_part_of_file;
		}

		start_in_file = end;

	} while (pos != std::wstring::npos && start_in_file < res.size());

	return res;
}

bool CorrectVaMultipleMalloc::want_file(const FILE_TYPE &file_type)
{
	switch (file_type) {
	case FILE_TYPE::C_FILE:
		return true;
	default:
		return false;
	}
}
