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

#include "font.hpp"

namespace Font {

#ifdef FONT_DEBUG
#define F(name, str) const char *const name = "[" #name "]"
#else
#define F(name, str) const char *const name = str
#endif

F(handle,		"\ue262");
F(segno,		"\ue047");
F(coda,			"\ue048");
F(sharp,		"\ue10c");
F(flat,			"\ue10d");
F(natural,		"\ue10e");
F(eleven,		"\ue182");
F(thirteen,		"\ue183");
F(dim,			"\ue184");
F(sus,			"\ue185");
F(aug,			"\ue186");
F(superdim0,	"\ue187");
F(superflat,	"\ue188");
F(supersharp,	"\ue189");
F(maj7,			"\ue18a");
F(min7,			"-");
F(add,			"\ue18b");
F(add11,		"\ue18c");
F(sus4,			"\ue18d");
F(dim0,			"\ue18e");
F(halfdim,		"\ue18f");
F(super0,		"\ue190");
F(super1,		"\ue191");
F(super2,		"\ue192");
F(super3,		"\ue193");
F(super4,		"\ue194");
F(super5,		"\ue195");
F(super6,		"\ue196");
F(super7,		"\ue197");
F(super8,		"\ue198");
F(super9,		"\ue199");
F(simile,		"\u2673");
F(simile2,		"\u2674");
F(simile3,		"\u2675");
F(crotchet,		"\u2669");
F(quaver,		"\u266a");
F(semibreve2,	"\ue1d2");
F(minim2,		"\ue1d3");
F(crotchet2,	"\ue1d5");
F(quaver2,		"\ue1d7");
F(semiquaver2,	"\ue1d9");
F(crotchetr,	"\u00a6"); // Also at 0x1d13d

};	// namespace Font
