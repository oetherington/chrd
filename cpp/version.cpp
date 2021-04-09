/*
 * Part of the Chrd chord editor
 * http://www.github.com/oetherington/chrd
 *
 * Copyright 2016 Ollie Etherington.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include "version.hpp"
#include "util.hpp"

#ifdef DEBUG
#define DEBUGGING_ENABLED true
#else
#define DEBUGGING_ENABLED false
#endif

std::string GetVersionString()
{
	return Format("chrd - Version %s\nDebugging %s\nBuilt %s %s with %s",
			CHRD_VERSION_STRING, DEBUGGING_ENABLED ? "enabled" : "disabled",
			__TIME__, __DATE__, __VERSION__);
}

std::string GetAboutString()
{
	return Format("The Chrd Chord Chart Editor - Version %s\n"
			"http://github.com/oetherington/chrd\n"
			"Copyright \u00A9 2016 Ollie Etherington\n"
			"Free software under the GNU GPLv3\n"
			"Use the '-h' flag for help",
			CHRD_VERSION_STRING);
}

std::string GetHelpString()
{
	// TODO
	return "";
}
