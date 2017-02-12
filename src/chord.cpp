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

#include "chord.hpp"
#include "font.hpp"

#define PUSH_CH '>'
#define RING_CH 'R'
#define CHOKE_CH 'K'
#define PAUSE_CH 'P'

Chord::Chord(const std::string &c)
{
	data.reserve(c.size());

	bool has_accidental = false;

	for (size_t i = 0; i < c.size(); ++i) {
		switch (c[i]) {
		case PUSH_CH:		push = true;		break;
		case RING_CH:		ring = true;		break;
		case CHOKE_CH:		choke = true;		break;
		case PAUSE_CH:		pause = true;		break;

		case 'b':
			if (has_accidental) {
				data += Font::superflat;
			} else {
				has_accidental = true;
				data += Font::flat;
			}
			break;

		case '#':
			if (has_accidental) {
				data += Font::supersharp;
			} else {
				has_accidental = true;
				data += Font::sharp;
			}
			break;

		case '/':
			has_accidental = false;
			data += '/';
			break;

		case 'n':	data += Font::natural;		break;
		case 'o':	data += Font::superdim0;	break;
		case '+':	data += Font::aug;			break;
		case '^':	data += Font::maj7;			break;
		case '-':	data += Font::min7;			break;
		case '@':	data += Font::halfdim;		break;
		case '5':	data += Font::super5;		break;
		case '6':	data += Font::super6;		break;
		case '7':	data += Font::super7;		break;
		case '9':	data += Font::super9;		break;
		case 'S':	data += Font::crotchetr;	break;

		case 's':
			if (i + 1 < c.size() && c[i + 1] == '4') {
				data += Font::sus4;
				++i;
			} else {
				data += Font::sus;
			}
			break;

		default:
			if (!c.compare(i, 2, "11")) {
				data += Font::eleven;
				++i;
			} else if (!c.compare(i, 2, "13")) {
				data += Font::thirteen;
				++i;
			} else if (!c.compare(i, 3, "dim")) {
				data += Font::dim;
				i += 2;
			} else if (!c.compare(i, 5, "add11")) {
				data += Font::add11;
				i += 4;
			} else if (!c.compare(i, 3, "add")) {
				data += Font::add;
				i += 2;
			} else if (!c.compare(i, 3, "%%%")) {
				data += Font::simile3;
				i += 2;
			} else if (!c.compare(i, 2, "%%")) {
				data += Font::simile2;
				++i;
			} else if (c[i] == '%') {
				data += Font::simile;
			} else {
				data += c[i];
			}

			break;
		}
	}
}
