#pragma once

#include <core/core.hpp>

#include <cstdint>

namespace ENGINE_NAMESPACE
{
	namespace log
	{
		using setting_t = uint16_t;

		static const char ESC = '\x1B'; //can also be '\033'
		/*static const setting_t background_offset = 10U;

		//Text colors
		static const setting_t black =	 30U;
		static const setting_t red =	 31U;
		static const setting_t green =	 32U;
		static const setting_t yellow =	 33U;
		static const setting_t blue =	 34U;
		static const setting_t magenta = 35U;
		static const setting_t cyan =	 36U;
		static const setting_t white =	 37U;

		static const setting_t bright_black =	90U;
		static const setting_t bright_red =		91U;
		static const setting_t bright_green =	92U;
		static const setting_t bright_yellow =	93U;
		static const setting_t bright_blue =	94U;
		static const setting_t bright_magenta =	95U;
		static const setting_t bright_cyan =	96U;
		static const setting_t bright_white =	97U;

		static const setting_t default_fg =		39U;

		//Background colors
		static const setting_t black_bg =	black + background_offset;
		static const setting_t red_bg =		red + background_offset;
		static const setting_t green_bg =	green + background_offset;
		static const setting_t yellow_bg =	yellow + background_offset;
		static const setting_t blue_bg =	blue + background_offset;
		static const setting_t magenta_bg =	magenta + background_offset;
		static const setting_t cyan_bg =	cyan + background_offset;
		static const setting_t white_bg =	white + background_offset;

		static const setting_t bright_black_bg =	bright_black + background_offset;
		static const setting_t bright_red_bg =		bright_red + background_offset;
		static const setting_t bright_green_bg =	bright_green + background_offset;
		static const setting_t bright_yellow_bg =	bright_yellow + background_offset;
		static const setting_t bright_blue_bg =		bright_blue + background_offset;
		static const setting_t bright_magenta_bg =	bright_magenta + background_offset;
		static const setting_t bright_cyan_bg =		bright_cyan + background_offset;
		static const setting_t bright_white_bg =	bright_white + background_offset;

		static const setting_t default_bg =			default_fg + background_offset;

		//Text modifiers
		static const setting_t bold =		1U;
		static const setting_t faint =		2U;
		static const setting_t italic =		3U;
		static const setting_t underline =	4U;
		static const setting_t strike =		9U;

		static const setting_t slow_blink =		5U;
		static const setting_t rapid_blink =	6U;

		static const setting_t no_font = 10U;
		*/
		enum ANSI_settings
		{
			RESET = 0,
			BOLD, FAINT, ITALIC, UNDERLINE,
			SLOW_BLINK, RAPID_BLINK,
			STRIKE = 9,
			DEFAULT_FONT,

			BLACK = 30,
			RED, GREEN, YELLOW, BLUE, MAGENTA, CYAN, WHITE,
			DEFAULT_COLOR,

			BLACK_BG = 40,
			RED_BG, GREEN_BG, YELLOW_BG, BLUE_BG, MAGENTA_BG, CYAN_BG, WHITE_BG,
			DEFAULT_BG_COLOR,

			BRIGHT_BLACK = 90,
			BRIGHT_RED, BRIGHT_GREEN, BRIGHT_YELLOW, BRIGHT_BLUE, BRIGHT_MAGENTA, BRIGHT_CYAN, BRIGHT_WHITE,

			BRIGHT_BLACK_BG = 100,
			BRIGHT_RED_BG, BRIGHT_GREEN_BG, BRIGHT_YELLOW_BG, BRIGHT_BLUE_BG, BRIGHT_MAGENTA_BG, BRIGHT_CYAN_BG, BRIGHT_WHITE_BG
		};
	}
}

#define ANSI_SETTING(arg) ENGINE_NAMESPACE::log::ESC << '[' << arg << 'm'
#define ANSI_SETTINGS(arg_0, arg_1, arg_2) ENGINE_NAMESPACE::log::ESC << '[' << arg_0 << ';' << arg_1 << ';' << arg_2 << 'm'

#define ANSI_RESET ENGINE_NAMESPACE::log::ESC << "[0m"
