#ifndef OUTDEBUG_H_
#define OUTDEBUG_H_

#include "debug.h"
#include "colored_output.h"

class OutDebug : public Tools::Debug, ColoredOutput
{
private:
	ColoredOutput::Color color;
	std::wstring prefix;
	bool print_line_and_file_info;

public:
	OutDebug( ColoredOutput::Color color = ColoredOutput::BRIGHT_YELLOW );

	void setColor( ColoredOutput::Color color_ ) { color = color_; }
	void setPrefix( const std::wstring & prefix_ ) { prefix = prefix_; }
	void setPrintLineAndFileInfo( bool print_line_and_file_info_ ) {
		print_line_and_file_info = print_line_and_file_info_;
	}

	void add( const char *file, unsigned line, const char *function, const std::string & s ) override;
	void add( const char *file, unsigned line, const char *function, const std::wstring & s ) override;
};

#endif /* OUTDEBUG_H_ */
