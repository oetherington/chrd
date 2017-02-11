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

#include "version_0_0_1.hpp"
#include "rgb.hpp"
#include "util.hpp"
#include "font.hpp"
#include "data.hpp"
#include <vector>
#include <stack>
#include <cctype>
#include <cstring>
#include <cassert>
#include <hpdf.h>
#include "hpdf_contrib.hpp"

struct Vec2 {
	HPDF_REAL x;
	HPDF_REAL y;

	Vec2(HPDF_REAL x, HPDF_REAL y)
		: x(x)
		, y(y)
	{}
};

enum class Barline {
	Single,
	Double,
	StartRepeat,
	EndRepeat,
	DoubleRepeat,
	Final,
};

#define REPEAT_TO_CUE (-1)

struct Bar {
	std::vector<std::string> chords;
	Barline barline = Barline::Single;
	int num_repeats = 0;
	struct {
		int top = 0;
		int bot = 0;
	} sig;

	Bar() {}

	inline void push_back(std::string c) { chords.push_back(c); }
};

#define PUSH_CH '>'
#define RING_CH 'R'
#define CHOKE_CH 'K'
#define PAUSE_CH 'P'

struct Chord {
	std::string data = "";
	bool push = false;
	bool ring = false;
	bool choke = false;
	bool pause = false;

	Chord(const std::string &c)
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
};

enum class Param {
	BarlinePadding,
	BarlineSpacing,
	BarlineWidth,
	FinalBarlineWidth,
	RepeatRadius,
	RepeatDotOffset,
	MarginTop,
	MarginRight,
	MarginBottom,
	MarginLeft,
	ChordSize,
	LabelSize,
	RepeatNumSize,
	TitleSize,
	SubtitleSize,
	AuthorSize,
	CopyrightSize,
	KeySize,
	KeyOffset,
	KeyPadding,
	KeyBoxWidth,
	TempoSize,
	TempoBreak,
	FirstLineOffset,
	SystemOffset,
	LabelOffset,
	RepeatNumOffset,
	BreakSize,
	StrokeWidth,
	PushOffset,
	PushWidth,
	PushHeight,
	RingXOffset,
	RingYOffset,
	RingXMargin,
	RingYMargin,
	ChokeOffset,
	ChokeWidth,
	ChokeHeight,
	PauseYOffset,
	PauseLineRadius,
	PauseDotRadius,
	COUNT
};

static void ErrorHandler(HPDF_STATUS error, HPDF_STATUS detail, void *r);

class Renderer {
private:
	typedef HPDF_REAL state_t;

	class State : public std::vector<state_t> {
	public:
		State()
		{
			resize(static_cast<state_t>(Param::COUNT));
		}

		reference operator[](Param p)
		{
			return at(static_cast<state_t>(p));
		}

		const_reference operator[](Param p) const
		{
			return at(static_cast<state_t>(p));
		}
	};

	const char *m_arial = nullptr;
	const char *m_chordlet_type = nullptr;

	state_t m_scale;
	bool m_on_first_line;
	size_t m_width;
	size_t m_height;
	size_t m_cur_y;
	std::stack<State> m_states;

	char m_comment_delim = '!';
	RGB m_background_color = RGB("#ffffff");
	RGB m_draw_color = RGB("#000000");
	bool m_draw_barlines = true;
	std::string m_text_font = "";
	std::string m_chord_font = "";
	bool m_lock_copyright = true;
	bool m_use_final_barline = true;
	bool m_key_box = true;

	std::string m_title = "";

	HPDF_Doc m_pdf;
	HPDF_Page m_page;

	std::string m_pdf_file = "";
	std::string m_pdf_string = "";

	inline size_t Width() const { return m_width; }
	inline size_t Height() const { return m_height; }

	inline void PushState() { m_states.push(m_states.top()); }
	inline void PopState() { m_states.pop(); }

	inline state_t GetParam(Param p) { return m_states.top()[p]; }
	inline void SetParam(Param p, state_t val) { m_states.top()[p] = val; }

	inline void Rect(HPDF_REAL x, HPDF_REAL y, HPDF_REAL w, HPDF_REAL h,
			bool fill = true)
	{
		HPDF_Page_SetLineWidth(m_page, GetParam(Param::StrokeWidth));
		HPDF_Page_Rectangle(m_page, x, m_height - y, w, h);
		fill ? HPDF_Page_Fill(m_page) : HPDF_Page_Stroke(m_page);
	}

	inline void Circ(HPDF_REAL x, HPDF_REAL y, HPDF_REAL radius,
			bool fill = true)
	{
		HPDF_Page_SetLineWidth(m_page, GetParam(Param::StrokeWidth));
		HPDF_Page_Circle(m_page, x, m_height - y, radius);
		fill ? HPDF_Page_Fill(m_page) : HPDF_Page_Stroke(m_page);
	}

	inline void Arc(HPDF_REAL x, HPDF_REAL y, HPDF_REAL radius,
			HPDF_REAL theta0, HPDF_REAL theta1)
	{
		HPDF_Page_SetLineWidth(m_page, GetParam(Param::StrokeWidth));
		HPDF_Page_Arc(m_page, x, m_height - y, radius, theta0, theta1);
		HPDF_Page_Stroke(m_page);
	}

	inline void Lines(const Vec2 *vertices, const size_t num)
	{
		assert(num >= 2);

		HPDF_Page_SetLineWidth(m_page, GetParam(Param::StrokeWidth));
		HPDF_Page_SetRGBStroke(m_page, m_draw_color.r, m_draw_color.g,
				m_draw_color.b);

		HPDF_Page_MoveTo(m_page, vertices[0].x, vertices[0].y);

		for (size_t i = 1; i < num; ++i)
			HPDF_Page_LineTo(m_page, vertices[i].x, vertices[i].y);

		HPDF_Page_Stroke(m_page);
	}

	class TextRenderer {
	private:
		Renderer *m_r;

	public:
		TextRenderer(Renderer *r, state_t size, const std::string &font_name)
			: m_r(r)
		{
			HPDF_Page_BeginText(m_r->m_page);
			HPDF_Page_SetRGBFill(m_r->m_page, m_r->m_draw_color.r,
					m_r->m_draw_color.g, m_r->m_draw_color.b);
			HPDF_Page_SetTextRenderingMode(m_r->m_page, HPDF_FILL);
			const HPDF_Font font = HPDF_GetFont(m_r->m_pdf, font_name.c_str(),
					"UTF-8");
			HPDF_Page_SetFontAndSize(m_r->m_page, font, size);
		}

		~TextRenderer()
		{
			HPDF_Page_EndText(m_r->m_page);
		}

		inline void Render(const std::string &text, HPDF_REAL x, HPDF_REAL y)
		{
			HPDF_Page_TextOut(m_r->m_page, x, m_r->m_height - y, text.c_str());
		}

		inline HPDF_REAL TextWidth(const std::string &text)
		{
			return HPDF_Page_TextWidth(m_r->m_page, text.c_str());
		}
	};

	void DrawTextAt(const std::string &text, state_t size,
			const std::string &font_name, HPDF_REAL x, HPDF_REAL y)
	{
		TextRenderer r(this, size, font_name);
		r.Render(text, x, y);
	}

	inline void DrawText(const std::string &text, state_t size,
			const std::string &font_name, HPDF_REAL x)
	{
		DrawTextAt(text, size, font_name, x, m_cur_y);
	}

	void DrawTextCentered(const std::string &text, state_t size,
			const std::string &font_name)
	{
		TextRenderer r(this, size, font_name);
		const HPDF_REAL width = r.TextWidth(text);
		r.Render(text, m_width / 2 - width / 2, m_cur_y + size);
	}

	void DrawTextRightJustified(const std::string &text, state_t size,
			const std::string &font_name)
	{
		TextRenderer r(this, size, font_name);
		const HPDF_REAL width = r.TextWidth(text);
		r.Render(text, m_width - width - GetParam(Param::MarginRight),
				m_cur_y + size);
	}

	void DrawCopyrightText(std::string &text)
	{
		ReplaceAll(text, "(c)", "\u00a9");
		ReplaceAll(text, "(C)", "\u00a9");

		const state_t size = GetParam(Param::CopyrightSize);

		const HPDF_REAL y = m_lock_copyright
			? m_height - GetParam(Param::MarginBottom)
			: m_cur_y + size;

		TextRenderer r(this, size, m_text_font);
		const HPDF_REAL width = r.TextWidth(text);
		r.Render(text, m_width / 2 - width / 2, y);
	}

	void DrawKeyText(std::string &text)
	{
		ReplaceAll(text, "#", Font::sharp);
		ReplaceAll(text, "b", Font::flat);

		const HPDF_REAL ks = GetParam(Param::KeySize);
		const HPDF_REAL kp = GetParam(Param::KeyPadding);
		const HPDF_REAL ko = GetParam(Param::KeyOffset);

		HPDF_REAL size, x, y;

		{
			TextRenderer r(this, ks, m_chord_font);
			const HPDF_REAL width = r.TextWidth(text);
			size = kp * 2 + std::max(width, ks);
			x = ko;
			y = ko + size;
			r.Render(text, x + kp, y - kp);
		}

		if (m_key_box)
			Rect(x, y, size, size, false);
	}

	void DrawBarline(Barline type, HPDF_REAL x, int num_repeats = 0)
	{
		if (!m_draw_barlines)
			return;

		const HPDF_REAL bw = GetParam(Param::BarlineWidth);
		const HPDF_REAL fbw = GetParam(Param::FinalBarlineWidth);
		const HPDF_REAL bs = GetParam(Param::BarlineSpacing);
		const HPDF_REAL cs = GetParam(Param::ChordSize);
		const HPDF_REAL rr = GetParam(Param::RepeatRadius);
		const HPDF_REAL rdo = GetParam(Param::RepeatDotOffset);

		switch (type) {
		case Barline::Single:
			Rect(x, m_cur_y, bw, cs + 2);
			break;
		case Barline::Double:
			Rect(x - 2 * bw - bs, m_cur_y, bw, cs + 2);
			Rect(x - bw, m_cur_y, bw, cs + 2);
			break;
		case Barline::StartRepeat:
			Rect(x, m_cur_y, fbw, cs + 2);
			Rect(x + fbw + bs, m_cur_y, bw, cs + 2);
			Circ(x + fbw + bw + 3 * bs, m_cur_y - cs / 3 - rdo, rr);
			Circ(x + fbw + bw + 3 * bs, m_cur_y - cs / 3 * 2 - rdo, rr);
			break;
		case Barline::EndRepeat:
			Rect(x - fbw - bw - bs, m_cur_y, bw, cs + 2);
			Rect(x - fbw, m_cur_y, fbw, cs + 2);
			Circ(x - fbw - bw - 3 * bs, m_cur_y - cs / 3 - rdo, rr);
			Circ(x - fbw - bw - 3 * bs, m_cur_y - cs / 3 * 2 - rdo, rr);
			break;
		case Barline::DoubleRepeat:
			Rect(x - fbw - bw - bs, m_cur_y, bw, cs + 2);
			Rect(x + bs, m_cur_y, bw, cs + 2);
			Rect(x - fbw, m_cur_y, fbw, cs + 2);
			Circ(x - fbw - bw - 3 * bs, m_cur_y - cs / 3 - rdo, rr);
			Circ(x - fbw - bw - 3 * bs, m_cur_y - cs / 3 * 2 - rdo, rr);
			Circ(x + bw + 3 * bs, m_cur_y - cs / 3 - rdo, rr);
			Circ(x + bw + 3 * bs, m_cur_y - cs / 3 * 2 - rdo, rr);
			break;
		case Barline::Final:
			Rect(x - fbw - bw - bs, m_cur_y, bw, cs + 2);
			Rect(x - fbw, m_cur_y, fbw, cs + 2);
			break;
		}
	}

	void DrawPush(HPDF_REAL x)
	{
		const HPDF_REAL cs = GetParam(Param::ChordSize);
		const HPDF_REAL pw = GetParam(Param::PushWidth);
		const HPDF_REAL ph = GetParam(Param::PushHeight);
		const HPDF_REAL po = GetParam(Param::PushOffset);
		const HPDF_REAL top = m_height - m_cur_y + cs + po;
		const HPDF_REAL bot = top + ph;
		const HPDF_REAL mid = bot - ((bot - top) / 2);

		const Vec2 vertices[] = {
			Vec2(x, top),
			Vec2(x + pw, mid),
			Vec2(x, bot)
		};

		Lines(vertices, sizeof(vertices) / sizeof(Vec2));
	}

	void DrawChoke(HPDF_REAL x)
	{
		const HPDF_REAL cs = GetParam(Param::ChordSize);
		const HPDF_REAL co = GetParam(Param::ChokeOffset);
		const HPDF_REAL ch = GetParam(Param::ChokeHeight);

		// TODO: Remove magic numbers
		const HPDF_REAL left = x + 1 * m_scale;
		const HPDF_REAL right = x + 1 * m_scale + GetParam(Param::ChokeWidth);
		const HPDF_REAL center = right - ((right - left) / 2);
		const HPDF_REAL top = m_height - m_cur_y + cs + co;
		const HPDF_REAL bot = m_height - m_cur_y + cs + co - ch;

		const Vec2 vertices[] = {
			Vec2(left, bot),
			Vec2(center, top),
			Vec2(right, bot)
		};

		Lines(vertices, sizeof(vertices) / sizeof(Vec2));
	}

	void DrawPause(HPDF_REAL x, const std::string &str)
	{
		const HPDF_REAL cs = GetParam(Param::ChordSize);

		HPDF_REAL w;

		{
			TextRenderer r(this, cs, m_chord_font);
			w = r.TextWidth(str);
		}

		const HPDF_REAL center = x + w / 2;

		const HPDF_REAL pyo = GetParam(Param::PauseYOffset);
		const HPDF_REAL plr = GetParam(Param::PauseLineRadius);
		const HPDF_REAL pdr = GetParam(Param::PauseDotRadius);

		Circ(center, m_cur_y + pyo - cs, pdr);
		Arc(center, m_cur_y + pyo - cs, plr, -90, 90);
	}

	void DrawRing(HPDF_REAL x, const std::string &str)
	{
		const HPDF_REAL cs = GetParam(Param::ChordSize);

		HPDF_REAL cw;

		{
			TextRenderer r(this, cs, m_chord_font);
			cw = r.TextWidth(str);
		}

		const HPDF_REAL rxm = GetParam(Param::RingXMargin);
		const HPDF_REAL rym = GetParam(Param::RingYMargin);
		const HPDF_REAL rxo = GetParam(Param::RingXOffset);
		const HPDF_REAL ryo = GetParam(Param::RingYOffset);

		const HPDF_REAL left = x - rxm + rxo;
		const HPDF_REAL right = x + rxm + rxo + cw;
		const HPDF_REAL center_x = right - ((right - left) / 2);

		const HPDF_REAL top = m_height - m_cur_y + cs + rym - ryo;
		const HPDF_REAL bottom = top - cw - rym - ryo;
		const HPDF_REAL center_y = bottom - ((bottom - top) / 2);

		const Vec2 vertices[] = {
			Vec2(center_x, top),
			Vec2(right, center_y),
			Vec2(center_x, bottom),
			Vec2(left, center_y),
			Vec2(center_x, top)
		};

		Lines(vertices, sizeof(vertices) / sizeof(Vec2));
	}

	void DrawChord(const std::string &data, HPDF_REAL x)
	{
		Chord c(data);
		const HPDF_REAL size = GetParam(Param::ChordSize);
		DrawTextAt(c.data, size, m_chordlet_type, x, m_cur_y);

		// TODO: Remove magic numbers

		if (c.push && c.choke) {
			DrawChoke(x - 1 * m_scale);
			DrawPush(x + 8 * m_scale);
		} else if (c.push && c.pause) {
			const HPDF_REAL offset = 2;
			const HPDF_REAL ph = GetParam(Param::PushHeight);
			m_cur_y -= ph;
			DrawPause(x, c.data);
			m_cur_y += ph + offset * m_scale;
			DrawPush(x);
			m_cur_y -= ph * m_scale;
		} else {
			if (c.push)
				DrawPush(x);

			if (c.choke)
				DrawChoke(x);

			if (c.pause)
				DrawPause(x, c.data);
		}

		if (c.ring)
			DrawRing(x, c.data);
	}

	void DrawSignature(int top, int bot, HPDF_REAL x)
	{
		// TODO
	}

	void DrawNumRepeats(HPDF_REAL x, int num)
	{
		// TODO
	}

	void DrawChordLine(std::string &s, bool is_final_line)
	{
		if (!s.size())
			return;

		if (m_on_first_line) {
			m_cur_y += GetParam(Param::FirstLineOffset);
			m_on_first_line = false;
		}

		std::vector<std::string> tkns = Split(s, ' ');
		const size_t tnum = tkns.size();

		Barline first_barline = Barline::Single;
		std::vector<Bar> bars;
		bool in_a_bar = false;

		for (size_t i = 0; i < tnum; ++i) {
			if (tkns[i] == "|") {
				if (!in_a_bar)
					in_a_bar = true;

				bars.push_back(Bar());
			} else if (tkns[i] == "||") {
				if (!in_a_bar)
					in_a_bar = true;

				if (bars.size())
					bars[bars.size() - 1].barline = Barline::Double;
				else
					first_barline = Barline::Double;

				bars.push_back(Bar());
			} else if (tkns[i] == "||:") {
				if (bars.size()) {
					if (bars[bars.size() - 1].barline == Barline::EndRepeat) {
						bars[bars.size() - 1].barline = Barline::DoubleRepeat;
					} else {
						bars[bars.size() - 1].barline = Barline::StartRepeat;
					}
				} else {
					first_barline = Barline::StartRepeat;
				}

				if (!in_a_bar)
					in_a_bar = true;

				bars.push_back(Bar());
			} else if (tkns[i] == "|:") {
				if (bars.size()) {
					if (bars[bars.size() - 1].barline == Barline::EndRepeat) {
						bars[bars.size() - 1].barline = Barline::DoubleRepeat;
					} else {
						bars[bars.size() - 1].barline = Barline::StartRepeat;
					}
				} else {
					first_barline = Barline::StartRepeat;
				}
			} else if (tkns[i] == "\\") {
				in_a_bar = false;
			} else if (tkns[i] == "|\\") {
				in_a_bar = false;

				if (bars.size())
					bars[bars.size() - 1].barline = Barline::Double;
				else
					first_barline = Barline::Double;
			} else if (tkns[i] == ":|") {
				if (bars.size())
					bars[bars.size() - 1].barline = Barline::EndRepeat;
				else
					puts("Invalid token: ':|'");
			} else if (tkns[i] == ":||") {
				if (bars.size())
					bars[bars.size() - 1].barline = Barline::EndRepeat;
				else
					puts("Invalid token: ':||'");

				if (!in_a_bar)
					in_a_bar = true;

				bars.push_back(Bar());
			} else if (tkns[i] == ":|\\") {
				if (bars.size())
					bars[bars.size() - 1].barline = Barline::EndRepeat;
				else
					puts("Invalid token: ':|\\'");

				in_a_bar = false;
			} else {
				std::string num_str = "";

				if (!tkns[i].compare(0, 3, ":||")) {
					num_str = tkns[i].substr(3);
				} else if (!tkns[i].compare(0, 3, ":|\\")) {
					num_str = tkns[i].substr(3);
					in_a_bar = false;
				} else if (!tkns[i].compare(0, 2, ":|")) {
					num_str = tkns[i].substr(2);
				}

				if (num_str.size()) {
					int num = (num_str == "c" || num_str == "C")
						? REPEAT_TO_CUE
						: static_cast<int>(strtol(num_str.c_str(),
									nullptr, 10));

					if (bars.size()) {
						bars[bars.size() - 1].barline = Barline::EndRepeat;

						if (num)
							bars[bars.size() - 1].num_repeats = num;
					} else {
						printf("Invalid token: '%s'\n", tkns[i].c_str());
					}

					if (!tkns[i].compare(0, 3, ":||")) {
						in_a_bar = true;
						bars.push_back(Bar());
					}
				} else if (tkns[i][0] == 'T') {
					num_str = tkns[i].substr(1);
					const size_t slash = num_str.find('/');
					const long top = strtol(num_str.substr(0, slash).c_str(),
							nullptr, 10);
					const long bot = strtol(num_str.substr(slash + 1).c_str(),
							nullptr, 10);

					if (top && bot) {
						bars[bars.size() - 1].sig.top = top;
						bars[bars.size() - 1].sig.bot = bot;
					}
				} else {
					if (!in_a_bar)
						bars.push_back(Bar());

					if (bars.size())
						bars[bars.size() - 1].push_back(tkns[i]);
				}
			}
		}

		const HPDF_REAL margin_left = GetParam(Param::MarginLeft);
		const HPDF_REAL margin_right = GetParam(Param::MarginRight);
		const HPDF_REAL bar_width = (m_width - margin_left - margin_right) /
			bars.size();

		HPDF_REAL cur_x = margin_left;

		DrawBarline(first_barline, cur_x);

		const size_t bnum = bars.size();

		for (size_t i = 0; i < bnum; ++i) {
			const size_t cnum = bars[i].chords.size();

			const HPDF_REAL bp = GetParam(Param::BarlinePadding);
			const HPDF_REAL chord_width = (bar_width - 2 * bp) / cnum;

			for (size_t j = 0; j < cnum; ++j)
				DrawChord(bars[i].chords[j], cur_x + bp + j * chord_width);

			if (bars[i].sig.top && bars[i].sig.bot)
				DrawSignature(bars[i].sig.top, bars[i].sig.bot, cur_x);

			cur_x += bar_width;

			DrawBarline(
					is_final_line && i + 1 == bnum && m_use_final_barline &&
							bars[i].barline == Barline::Single
						? Barline::Final
						: bars[i].barline,
					cur_x, bars[i].num_repeats);
		}

		m_cur_y += GetParam(Param::ChordSize) + GetParam(Param::SystemOffset);
	}

	inline std::string PragmaGetData(const std::string &s)
	{
		const size_t first = s.find(" ");
		std::string res = first == std::string::npos ? "" : s.substr(first);
		return Trim(res);
	}

	inline state_t ParseFlt(const std::vector<std::string> &ss, size_t idx = 1)
	{
		return ss.size() <= idx ? 0
			: strtof(ss[idx].c_str(), nullptr) * m_scale;
	}

	void ProcessPragma(std::string &s)
	{
		std::vector<std::string> tkns = Split(s, ' ');

		if (!tkns.size())
			return;

		const std::string &p = tkns[0];
		const std::string first = tkns.size() > 1 ? tkns[1] : "";

		if (p == "#version") {
		} else if (p == "#push") {
			PushState();
		} else if (p == "#pop") {
			PopState();
		} else if (p == "#title") {
			std::string data = PragmaGetData(s);

			if (!data.size())
				return;

			if (m_title.size())
				m_title += ' ' + data;
			else
				m_title = data;

			const state_t size = GetParam(Param::TitleSize);
			DrawTextCentered(data, size, m_text_font);
			m_cur_y += size;
		} else if (p == "#subtitle") {
			std::string data = PragmaGetData(s);
			if (!data.size())
				return;
			const state_t size = GetParam(Param::SubtitleSize);
			DrawTextCentered(data, size, m_text_font);
			m_cur_y += size;
		} else if (p == "#author") {
			std::string data = PragmaGetData(s);
			if (!data.size())
				return;
			const state_t size = GetParam(Param::AuthorSize);
			DrawTextRightJustified(data, size, m_text_font);
			m_cur_y += size;
		} else if (p == "#copyright") {
			std::string data = PragmaGetData(s);
			if (!data.size())
				return;
			DrawCopyrightText(data);
			if (!m_lock_copyright)
				m_cur_y += GetParam(Param::CopyrightSize);
		} else if (p == "#key") {
			std::string data = PragmaGetData(s);
			if (!data.size())
				return;
			DrawKeyText(data);
		} else if (p == "#tempo") {
			const size_t tempo = static_cast<size_t>(ParseFlt(tkns));
			/* m_cur_y += GetParam(Param::TempoBreak); */
			DrawText(std::string(Font::crotchet) + " = " +
					std::to_string(tempo), GetParam(Param::TempoSize),
					m_chord_font, GetParam(Param::MarginLeft));
			m_cur_y += GetParam(Param::TempoBreak);
		} else if (p == "#label") {
			std::string data = PragmaGetData(s);
			if (!data.size())
				return;
			DrawText(data, GetParam(Param::LabelSize), m_text_font,
					GetParam(Param::MarginLeft));
			m_cur_y += GetParam(Param::LabelSize) +
				GetParam(Param::LabelOffset);
		} else if (p == "#label_at") {
			if (tkns.size() < 4)
				return;

			std::string data = PragmaGetData(s);
			if (!data.size())
				return;

			HPDF_REAL x = ParseFlt(tkns, 1);
			HPDF_REAL y = ParseFlt(tkns, 2);
			std::string lbl = tkns[3];

			for (size_t i = 4; i < tkns.size(); ++i)
				lbl += ' ' + tkns[i];

			DrawTextAt(lbl, GetParam(Param::LabelSize), m_text_font, x, y);
		} else if (p == "#start") {
			m_on_first_line = false;
			m_cur_y += GetParam(Param::FirstLineOffset);
		} else if (p == "#break") {
			m_cur_y += GetParam(Param::BreakSize);
		} else if (p == "#include") {
			// TODO
			printf("Error: Includes not yet implemented!\n");
		} else if (p == "#comment_delim") {
			if (first.size())
				m_comment_delim = first[0];
		} else if (p == "#background") {
			m_background_color = RGB(first);
		} else if (p == "#color") {
			m_draw_color = RGB(first);
		} else if (p == "#draw_barlines") {
			m_draw_barlines = first != "false";
		} else if (p == "#lock_copyright") {
			m_lock_copyright = first != "false";
		} else if (p == "#use_final_barline") {
			m_use_final_barline = first != "false";
		} else if (p == "#key_box") {
			m_key_box = first != "false";
		} else if (p == "#text_font") {
			m_text_font = first;
		} else if (p == "#chord_font") {
			m_chord_font = first;
		}

#define PARAM_PRAGMA(name, param) else if (p == name) { \
	SetParam(Param::param, ParseFlt(tkns)); }

		PARAM_PRAGMA("#barline_spacing", BarlineSpacing)
		PARAM_PRAGMA("#barline_width", BarlineWidth)
		PARAM_PRAGMA("#final_barline_width", FinalBarlineWidth)
		PARAM_PRAGMA("#repeat_radius", RepeatRadius)
		PARAM_PRAGMA("#repeat_dot_offset", RepeatDotOffset)
		PARAM_PRAGMA("#margin_top", MarginTop)
		PARAM_PRAGMA("#margin_right", MarginRight)
		PARAM_PRAGMA("#margin_bottom", MarginBottom)
		PARAM_PRAGMA("#margin_left", MarginLeft)
		PARAM_PRAGMA("#chord_size", ChordSize)
		PARAM_PRAGMA("#label_size", LabelSize)
		PARAM_PRAGMA("#repeat_num_size", RepeatNumSize)
		PARAM_PRAGMA("#title_size", TitleSize)
		PARAM_PRAGMA("#subtitle_size", SubtitleSize)
		PARAM_PRAGMA("#author_size", AuthorSize)
		PARAM_PRAGMA("#copyright_size", CopyrightSize)
		PARAM_PRAGMA("#key_size", KeySize)
		PARAM_PRAGMA("#key_offset", KeyOffset)
		PARAM_PRAGMA("#key_padding", KeyPadding)
		PARAM_PRAGMA("#key_box_width", KeyBoxWidth)
		PARAM_PRAGMA("#tempo_size", TempoSize)
		PARAM_PRAGMA("#tempo_break", TempoBreak)
		PARAM_PRAGMA("#first_offset", FirstLineOffset)
		PARAM_PRAGMA("#system_offset", SystemOffset)
		PARAM_PRAGMA("#label_offset", LabelOffset)
		PARAM_PRAGMA("#break_size", BreakSize)
		PARAM_PRAGMA("#stroke_width", StrokeWidth)
		PARAM_PRAGMA("#push_offset", PushOffset)
		PARAM_PRAGMA("#push_width", PushWidth)
		PARAM_PRAGMA("#push_height", PushHeight)
		PARAM_PRAGMA("#ring_x_offset", RingXOffset)
		PARAM_PRAGMA("#ring_y_offset", RingYOffset)
		PARAM_PRAGMA("#ring_x_margin", RingXMargin)
		PARAM_PRAGMA("#ring_y_margin", RingYMargin)
		PARAM_PRAGMA("#choke_offset", ChokeOffset)
		PARAM_PRAGMA("#choke_width", ChokeWidth)
		PARAM_PRAGMA("#choke_height", ChokeHeight)
		PARAM_PRAGMA("#pause_y_offset", PauseYOffset)
		PARAM_PRAGMA("#pause_line_radius", PauseLineRadius)
		PARAM_PRAGMA("#pause_dot_radius", PauseDotRadius)

#undef PARAM_PRAGMA

		else {
			printf("Unrecognised pragma: '%s'\n", tkns[0].c_str());
		}
	}

	std::string MakeFileTitle(const char *const extension)
	{
		std::string t;
		t.reserve(m_title.size() + strlen(extension));

		for (char &c : m_title) {
			if (isspace(c))
				t += '_';
			else if (isalnum(c))
				t += tolower(c);
		}

		if (!t.size())
			t = "chrd_output";

		return t + extension;
	}

public:
	Renderer(state_t scale, const std::string &data)
		: m_scale(scale)
		, m_on_first_line(true)
		, m_width(0)
		, m_height(0)
		, m_cur_y(0)
	{
		// Setup the state
		m_states.push(State());

		State &s = m_states.top();

		s[Param::BarlinePadding]	= 8;
		s[Param::BarlineSpacing]	= 1;
		s[Param::BarlineWidth]		= 1;
		s[Param::FinalBarlineWidth]	= 2;
		s[Param::RepeatRadius]		= 1;
		s[Param::RepeatDotOffset]	= 1;
		s[Param::MarginTop]			= 14;
		s[Param::MarginRight]		= 22;
		s[Param::MarginBottom]		= 22;
		s[Param::MarginLeft]		= 25;
		s[Param::ChordSize]			= 14;
		s[Param::LabelSize]			= 12;
		s[Param::RepeatNumSize]		= 10;
		s[Param::TitleSize]			= 20;
		s[Param::SubtitleSize]		= 14;
		s[Param::AuthorSize]		= 12;
		s[Param::CopyrightSize]		= 8;
		s[Param::KeySize]			= 18;
		s[Param::KeyOffset]			= 16;
		s[Param::KeyPadding]		= 2;
		s[Param::KeyBoxWidth]		= 1;
		s[Param::TempoSize]			= 12;
		s[Param::TempoBreak]		= 12;
		s[Param::FirstLineOffset]	= 20;
		s[Param::SystemOffset]		= 14;
		s[Param::LabelOffset]		= 10;
		s[Param::RepeatNumOffset]	= 1.5;
		s[Param::BreakSize]			= 10;
		s[Param::StrokeWidth]		= 1;
		s[Param::PushOffset]		= -1;
		s[Param::PushWidth]			= 7;
		s[Param::PushHeight]		= 6;
		s[Param::RingXOffset]		= 0;
		s[Param::RingYOffset]		= 2;
		s[Param::RingXMargin]		= 5;
		s[Param::RingYMargin]		= 4;
		s[Param::ChokeOffset]		= 4;
		s[Param::ChokeWidth]		= 8;
		s[Param::ChokeHeight]		= 6;
		s[Param::PauseYOffset]		= 0;
		s[Param::PauseLineRadius]	= 4;
		s[Param::PauseDotRadius]	= 1;

		for (state_t &p : s)
			p *= m_scale;

		m_cur_y = s[Param::MarginTop];

		// Create the pdf
		m_pdf = HPDF_New(static_cast<HPDF_Error_Handler>(ErrorHandler),
				static_cast<void *>(this));

		if (!m_pdf)
			throw Error("Couldn't create PDF document");

		HPDF_UseUTFEncodings(m_pdf);

		/* m_arial = HPDF_LoadTTFontFromBuffer(m_pdf, */
				/* fonts_Arial_ttf, fonts_Arial_ttf_len, */
				/* HPDF_TRUE); */
		m_arial = HPDF_LoadTTFontFromBuffer(m_pdf,
				fonts_Roboto_Regular_ttf, fonts_Roboto_Regular_ttf_len,
				HPDF_TRUE);
		m_chordlet_type = HPDF_LoadTTFontFromBuffer(m_pdf,
				fonts_ChordletType_ttf, fonts_ChordletType_ttf_len, HPDF_TRUE);

		m_text_font = m_arial;
		m_chord_font = m_chordlet_type;

		m_page = HPDF_AddPage(m_pdf);
		HPDF_Page_SetSize(m_page, HPDF_PAGE_SIZE_A4, HPDF_PAGE_PORTRAIT);

		m_width = HPDF_Page_GetWidth(m_page);
		m_height = HPDF_Page_GetHeight(m_page);

		// Render
		std::vector<std::string> lines = Split(data, '\n');

		for (size_t i = 0; i < lines.size(); ++i) {
			std::string &l = Trim(lines[i]);

			if (!l.size())
				continue;

			if (l[0] == '#')
				ProcessPragma(l);
			else if (l[0] != m_comment_delim)
				DrawChordLine(l, i + 1 == lines.size());
		}

		// Save
		m_pdf_file = MakeFileTitle(".pdf");
		HPDF_SaveToStream(m_pdf);
		HPDF_UINT32 stream_size = HPDF_Stream_Size(m_pdf->stream);
		m_pdf_string.resize(stream_size);
		if (HPDF_Stream_Read(m_pdf->stream, (HPDF_BYTE *)&m_pdf_string[0],
					&stream_size) != HPDF_OK)
			throw Error("Couldn't read HPDF stream");
	}

	~Renderer()
	{
		if (m_pdf)
			HPDF_Free(m_pdf);
	}

	inline const std::string &GetPDFFile() const
	{
		return m_pdf_file;
	}

	inline const std::string &GetPDFString() const
	{
		return m_pdf_string;
	}
};

static void ErrorHandler(HPDF_STATUS error, HPDF_STATUS detail, void *r)
{
	throw Error(Format("HPDF Error: %lx %lx\n", error, detail));
}

Version_0_0_1_Result Version_0_0_1(const std::string &data)
{
	Renderer r(1.f, data);
	return Version_0_0_1_Result {
		.pdf_file = r.GetPDFFile(),
		.pdf_data = r.GetPDFString(),
	};
}
