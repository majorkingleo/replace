/*
 * DetectLocale.h
 *
 *  Created on: 08.02.2022
 *      Author: martin
 */

#ifndef DETECTLOCALE_H_
#define DETECTLOCALE_H_


class DetectLocale
{
	bool is_utf8;

public:
	DetectLocale();

	void init();

	bool isUtf8() const {
		return is_utf8;
	}
};



#endif /* DETECTLOCALE_H_ */
