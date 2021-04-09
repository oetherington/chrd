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

#pragma once

#ifndef CHORD_HPP
#define CHORD_HPP

#include <string>

struct Chord {
	std::string data = "";
	bool push = false;
	bool ring = false;
	bool choke = false;
	bool pause = false;

	Chord(const std::string &c);
};

#endif
