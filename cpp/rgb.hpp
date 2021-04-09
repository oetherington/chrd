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

#ifndef RGB_HPP
#define RGB_HPP

#include "csscolorparser/csscolorparser.hpp"
#include <string>

struct RGB {
	float r, g, b;

	RGB()
		: r(0.f)
		, g(0.f)
		, b(0.f)
	{}

	RGB(float r, float g, float b)
		: r(r)
		, g(g)
		, b(b)
	{}

	RGB(const std::string &col)
	{
		const CSSColorParser::Color c = CSSColorParser::parse(col);
		r = static_cast<float>(c.r) / 255.f;
		g = static_cast<float>(c.g) / 255.f;
		b = static_cast<float>(c.b) / 255.f;
	}
};

#endif
