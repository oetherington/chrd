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

#include "hpdf_contrib.hpp"
#include <cstring>

static const char *LoadTTFontFromStream(HPDF_Doc pdf, HPDF_Stream font_data,
		HPDF_BOOL embedding)
{
	HPDF_FontDef def = HPDF_TTFontDef_Load(pdf->mmgr, font_data, embedding);

	if (!def)
		return nullptr;

	HPDF_FontDef tmpdef = HPDF_Doc_FindFontDef(pdf, def->base_font);

	if (tmpdef) {
		HPDF_FontDef_Free(def);
		return tmpdef->base_font;
	}

	if (HPDF_List_Add(pdf->fontdef_list, def) != HPDF_OK) {
		HPDF_FontDef_Free(def);
		return nullptr;
	}

	if (embedding) {
		if (pdf->ttfont_tag[0] == 0) {
			memcpy(pdf->ttfont_tag, (HPDF_BYTE *)"HPDFAA", 6);
		} else {
			for (HPDF_INT i = 5; i >= 0; i--) {
				pdf->ttfont_tag[i] += 1;
				if (pdf->ttfont_tag[i] > 'Z')
					pdf->ttfont_tag[i] = 'A';
				else
					break;
			}
		}

		HPDF_TTFontDef_SetTagName(def, (char *)pdf->ttfont_tag);
	}

	return def->base_font;
}

const char *HPDF_LoadTTFontFromBuffer(HPDF_Doc pdf,
		const unsigned char *const buffer, size_t len, HPDF_BOOL embedding)
{
	if (!HPDF_HasDoc(pdf))
		return nullptr;

	HPDF_Stream font_data = HPDF_MemStream_New(pdf->mmgr, len);
	HPDF_Stream_Write(font_data, (HPDF_BYTE *)buffer, len);

	const char *ret = HPDF_Stream_Validate(font_data)
		? LoadTTFontFromStream(pdf, font_data, embedding)
		: nullptr;

	if (!ret)
		HPDF_CheckError(&pdf->error);

	return ret;
}
