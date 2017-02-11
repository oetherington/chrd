/*
 * chordlet.js
 * The Chordlet Chord Editor
 *
 * Copyright 2015 Ollie Etherington.
 * All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

/*
 * TODO:
 * Images
 * Small labels over specific chords
 * Segnos/codas
 * Better chord parser
 * Rhythm notation
 */

'use strict';

var VERSION = "0.0.1";
var RELEASE_BUILD = "18";

var CHORLET_FONT_URL = "img/ChordletType.ttf";
var NEW_DOCUMENT_STRING = "#version 0.0.1\n#title\n#author\n#key\n#start";

var DIRTY = false;

// Multiple indexOf function
function getIndicesOf(searchStr, str, caseSensitive) {
    var startIndex = 0, searchStrLen = searchStr.length;
    var index, indices = [];
    if (!caseSensitive) {
        str = str.toLowerCase();
        searchStr = searchStr.toLowerCase();
    }
    while ((index = str.indexOf(searchStr, startIndex)) > -1) {
        indices.push(index);
        startIndex = index + searchStrLen;
    }
    return indices;
}

function unicode_to_surrogate_pair(code_point) {
	if (code_point < 0xffff)
		return code_point;

	var high = Math.floor((code_point - 0x10000) / 0x400) + 0xD800;
	var low = (code_point - 0x10000) % 0x400 + 0xDC00;
	return String.fromCharCode(high) + String.fromCharCode(low);
}

function unicode_from_surrogate_pair(pair_or_high, low) {
	var high = pair_or_high.charCodeAt(0);
	var low = low ? low.charCodeAt(0) : pair_or_high.charCodeAt(1);
	return (high - 0xD800) * 0x400 + low - 0xDC00 + 0x10000;
}

// Key to the special characters in the font
var font = {
	handle:		"\ue262",
	segno:		"\ue047",
	coda:		"\ue048",
	sharp:		"\ue10c",
	flat:		"\ue10d",
	natural:	"\ue10e",
	eleven:		"\ue182",
	thirteen:	"\ue183",
	dim:		"\ue184",
	sus:		"\ue185",
	aug:		"\ue186",
	superdim0:	"\ue187",
	superflat:	"\ue188",
	supersharp:	"\ue189",
	maj7:		"\ue18a",
	min7:		"-",
	add:		"\ue18b",
	add11:		"\ue18c",
	sus4:		"\ue18d",
	dim0:		"\ue18e",
	halfdim:	"\ue18f",
	super0:		"\ue190",
	super1:		"\ue191",
	super2:		"\ue192",
	super3:		"\ue193",
	super4:		"\ue194",
	super5:		"\ue195",
	super6:		"\ue196",
	super7:		"\ue197",
	super8:		"\ue198",
	super9:		"\ue199",
	simile:		"\u2673",
	simile2:	"\u2674",
	simile3:	"\u2675",
	crotchet:	"\u2669",
	quaver:		"\u266a",
	semibreve2:	"\ue1d2",
	minim2:		"\ue1d3",
	crotchet2:	"\ue1d5",
	quaver2:	"\ue1d7",
	semiquaver2:"\ue1d9",
	crotchetr:	unicode_to_surrogate_pair(0x1d13d),
}

// Chord parser - takes data and returns renderable unicode string.
var chord_parse = function(data) {
	var obj = {};

	var push = ">";
	var ring = "R";
	var choke = "K";
	var pause = "P";

	for (var i = data.indexOf(push); i >= 0; i = data.indexOf(push)) {
		data = data.slice(0, i) + data.slice(i + 1, data.length);
		obj.push = true;
	}

	for (var i = data.indexOf(ring); i >= 0; i = data.indexOf(ring)) {
		data = data.slice(0, i) + data.slice(i + 1, data.length);
		obj.ring = true;
	}

	for (var i = data.indexOf(choke); i >= 0; i = data.indexOf(choke)) {
		data = data.slice(0, i) + data.slice(i + 1, data.length);
		obj.choke = true;
	}

	for (var i = data.indexOf(pause); i >= 0; i = data.indexOf(pause)) {
		data = data.slice(0, i) + data.slice(i + 1, data.length);
		obj.pause = true;
	}

	var has_accidental = false;

	for (var i = 0; i < data.length; i++) {
		if (data[i] == 'b') {
			if (has_accidental) {
				data = data.slice(0, i) + font.superflat +
					data.slice(i + 1, data.length);
			} else {
				has_accidental = true;
				data = data.slice(0, i) + font.flat +
					data.slice(i + 1, data.length);
			}
		} else if (data[i] == '#') {
			if (has_accidental) {
				data = data.slice(0, i) + font.supersharp +
					data.slice(i + 1, data.length);
			} else {
				has_accidental = true;
				data = data.slice(0, i) + font.sharp +
					data.slice(i + 1, data.length);
			}
		} else if (data[i] == '/') {
			has_accidental = false;
		}
	}

	obj.string = data
		.replace("n", font.natural)
		.replace("11", font.eleven)
		.replace("13", font.thirteen)
		.replace("dim", font.dim)
		.replace("o", font.superdim0)
		.replace(/s(?!4)/g, font.sus)
		.replace("s4", font.sus4)
		.replace("+", font.aug)
		.replace("^", font.maj7)
		.replace("-", font.min7)
		.replace("add", font.add)
		.replace("add11", font.add11)
		.replace("@", font.halfdim)
		.replace("5", font.super5)
		.replace("6", font.super6)
		.replace("7", font.super7)
		.replace("9", font.super9)
		.replace("%%%", font.simile3)
		.replace("%%", font.simile2)
		.replace("%", font.simile)
		.replace("S", font.crotchetr);

	return obj;
}

var BARLINE_SINGLE = 0;
var BARLINE_DOUBLE = 1;
var BARLINE_START_REPEAT = 2;
var BARLINE_END_REPEAT = 3;
var BARLINE_DOUBLE_REPEAT = 4;
var BARLINE_FINAL = 5;

var REPEAT_TO_CUE = -1;

// Bar class
function Bar() {
	var THIS = this;

	this.chords = [];
	this.barline = BARLINE_SINGLE;
	this.num_repeats = 0;
	this.sig_numerator = 0;
	this.sig_denominator = 0;

	this.add = function(c) {
		THIS.chords[THIS.chords.length] = c;
	}
}

// Stack to store states
var state_stack = [];

// Global configuarable data
var comment_delim = "!";
var background_color = "#ffffff";
var draw_color = "#000000";
var draw_barlines = true;
var text_font = "Arial";
var chord_font = "ChordletType";
var lock_copyright = true;
var use_final_barline = true;
var key_box = true;
var title = null;
var subtitle = null;
var author = null;
var copyright = null;
var key = null;

// Render text to a canvas context
function render(ctx, data, scale) {
	// Start a new rendering context - delete any previous saved states
	state_stack = [];

	// Data specific to the scale
	var barline_padding = 8 * scale;
	var barline_spacing = 1 * scale;
	var barline_width = 1 * scale;
	var final_barline_width = 2 * scale;
	var repeat_radius = 1 * scale;

	var margin_top = 14 * scale;
	var margin_right = 22 * scale;
	var margin_bottom = 22 * scale;
	var margin_left = 25 * scale;

	var chord_size = 14 * scale;

	var label_size = 12 * scale;
	var repeat_num_size = 10 * scale;
	var title_size = 20 * scale;
	var subtitle_size = 14 * scale;
	var author_size = 12 * scale;
	var copyright_size = 8 * scale;

	var key_size = 18 * scale;
	var key_offset = 16 * scale;
	var key_padding = 2 * scale;
	var key_box_width = 1 * scale;

	var tempo_size = 12 * scale;
	var tempo_break = 6 * scale;

	var first_line_offset = 4 * scale;
	var system_offset = 14 * scale;
	var label_offset = 10 * scale;
	var repeat_num_offset = 1.5 * scale;

	var break_size = 10 * scale;

	var stroke_width = 1 * scale;

	var push_offset = 8 * scale;
	var push_width = 7 * scale;
	var push_height = 6 * scale;

	var ring_x_offset = 0 * scale;
	var ring_y_offset = 2 * scale;
	var ring_x_margin = 5 * scale;
	var ring_y_margin = 4 * scale;

	var choke_offset = 8 * scale;
	var choke_width = 8 * scale;
	var choke_height = 6 * scale;

	var pause_y_offset = 0 * scale;
	var pause_line_radius = 4 * scale;
	var pause_dot_radius = 1 * scale;

	// State pushing and poping
	function push_state() {
		state_stack[state_stack.length] = {
			draw_color: draw_color,
			draw_barlines: draw_barlines,
			text_font: text_font,
			chord_font: chord_font,
			barline_padding: barline_padding,
			barline_spacing: barline_spacing,
			barline_width: barline_width,
			final_barline_width: final_barline_width,
			repeat_radius: repeat_radius,
			margin_top: margin_top,
			margin_right: margin_right,
			margin_bottom: margin_bottom,
			margin_left: margin_left,
			chord_size: chord_size,
			label_size: label_size,
			repeat_num_size: repeat_num_size,
			tempo_size: tempo_size,
			tempo_break: tempo_break,
			system_offset: system_offset,
			label_offset: label_offset,
			repeat_num_offset: repeat_num_offset,
			break_size: break_size,
			stroke_width: stroke_width,
			push_offset: push_offset,
			push_width: push_width,
			push_height: push_height,
			ring_x_offset: ring_x_offset,
			ring_y_offset: ring_y_offset,
			choke_offset: choke_offset,
			choke_width: choke_width,
			choke_height: choke_height,
			pause_y_offset: pause_y_offset,
			pause_line_radius: pause_line_radius,
			pause_dot_radius: pause_dot_radius
		};
	}

	function pop_state() {
		if (state_stack.length < 1)
			return;

		var state = state_stack.pop();

		draw_color = state.draw_color;
		draw_barlines = state.draw_barlines;
		text_font = state.text_font;
		chord_font = state.chord_font;
		barline_padding = state.barline_padding;
		barline_spacing = state.barline_spacing;
		barline_width = state.barline_width;
		final_barline_width = state.final_barline_width;
		repeat_radius = state.repeat_radius;
		margin_top = state.margin_top;
		margin_right = state.margin_right;
		margin_bottom = state.margin_bottom;
		margin_left = state.margin_left;
		chord_size = state.chord_size;
		label_size = state.label_size;
		repeat_num_size = state.repeat_num_size;
		tempo_size = state.tempo_size;
		tempo_break = state.tempo_break;
		system_offset = state.system_offset;
		label_offset = state.label_offset;
		repeat_num_offset = state.repeat_num_offset;
		break_size = state.break_size;
		stroke_width = state.stroke_width;
		push_offset = state.push_offset;
		push_width = state.push_width;
		push_height = state.push_height;
		ring_x_offset = state.ring_x_offset;
		ring_y_offset = state.ring_y_offset;
		choke_offset = state.choke_offset;
		choke_width = state.choke_width;
		choke_height = state.choke_height;
		pause_y_offset = state.pause_y_offset;
		pause_line_radius = state.pause_line_radius;
		pause_dot_radius = state.pause_dot_radius;
	}

	// Track when we've drawn the first line
	var first_line = true;

	// Canvas size
	var width = ctx.canvas.clientWidth;
	var height = ctx.canvas.clientHeight;

	// Current y position
	var cur_y = margin_top;

	// Helper methods
	function set_font(size, font) {
		ctx.font = size + "px " + font;
	}

	function draw_text_at(text, size, font, x, y) {
		set_font(size, font);
		ctx.fillText(text, x, y + size);
	}

	function draw_text(text, size, font, x) {
		set_font(size, font);
		ctx.fillText(text, x, cur_y + size);
	}

	function draw_text_right_justified(text, size, font) {
		set_font(size, font);
		ctx.fillText(text,
				width - ctx.measureText(text).width - margin_right,
				cur_y + size);
	}

	function draw_text_centered(text, size, font) {
		set_font(size, font);
		ctx.fillText(text, width / 2 - ctx.measureText(text).width / 2,
				cur_y + size);
	}

	function draw_copyright_text() {
		var text = copyright.replace("(c)", "\u00a9").replace("(C)", "\u00a9");
		var y_offs = lock_copyright ? height - margin_bottom : cur_y;
		set_font(copyright_size, text_font);
		ctx.fillText(text, width / 2 - ctx.measureText(text).width / 2,
				y_offs + copyright_size);
	}

	function draw_key_text() {
		var x = key_offset;
		var y = key_offset;
		var text = key.replace("#", font.sharp).replace("b", font.flat);

		set_font(key_size, chord_font);

		if (key_box) {
			var text_width = ctx.measureText(text).width;
			var size = Math.max(text_width, key_size) + key_padding * 2;

			ctx.save();
			ctx.beginPath();
			ctx.lineWidth = "" + key_box_width;
			ctx.rect(x, y, size, size);
			ctx.stroke();
			ctx.restore();

			ctx.fillText(text, x + size / 2 - text_width / 2,
					y + size / 2 - key_size / 2 + key_size);
		} else {
			ctx.fillText(text, x + key_padding, y + key_padding + key_size);
		}
	}

	function draw_tempo_text(tempo) {
		cur_y += tempo_break;
		set_font(tempo_size, chord_font);
		ctx.fillText(font.crotchet + " = " + tempo, margin_left, cur_y);
		cur_y += tempo_break;
	}

	function draw_barline(x) {
		ctx.fillRect(x, cur_y, barline_width, chord_size + 2);
	}

	function draw_double_barline(x) {
		draw_barline(x - 2 * barline_width - barline_spacing);
		draw_barline(x - barline_width);
	}

	function draw_start_repeat_barline(x) {
		ctx.fillRect(x, cur_y, final_barline_width,
				chord_size + 2);
		ctx.fillRect(x + final_barline_width + barline_spacing,
				cur_y, barline_width, chord_size + 2);

		ctx.beginPath();
		ctx.arc(x + final_barline_width + barline_width + 3 * barline_spacing,
				1 + cur_y + chord_size / 3, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x + final_barline_width + barline_width + 3 * barline_spacing,
				1 + cur_y + chord_size / 3 * 2, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();
	}

	function draw_end_repeat_barline(x) {
		ctx.fillRect(x - final_barline_width - barline_width - barline_spacing,
				cur_y, barline_width, chord_size + 2);
		ctx.fillRect(x - final_barline_width, cur_y, final_barline_width,
				chord_size + 2);

		ctx.beginPath();
		ctx.arc(x - final_barline_width - barline_width - 3 * barline_spacing,
				1 + cur_y + chord_size / 3, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x - final_barline_width - barline_width - 3 * barline_spacing,
				1 + cur_y + chord_size / 3 * 2, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();
	}

	function draw_double_repeat_barline(x) {
		// Left thin line
		ctx.fillRect(x - final_barline_width - barline_width - barline_spacing,
				cur_y, barline_width, chord_size + 2);

		// Right thin line
		ctx.fillRect(x + barline_spacing, cur_y, barline_width,
				chord_size + 2);

		// Fat line
		ctx.fillRect(x - final_barline_width, cur_y, final_barline_width,
				chord_size + 2);

		// Left dots
		ctx.beginPath();
		ctx.arc(x - final_barline_width - barline_width - 3 * barline_spacing,
				1 + cur_y + chord_size / 3, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x - final_barline_width - barline_width - 3 * barline_spacing,
				1 + cur_y + chord_size / 3 * 2, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();

		// Right dots
		ctx.beginPath();
		ctx.arc(x + barline_width + 3 * barline_spacing,
				1 + cur_y + chord_size / 3, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();

		ctx.beginPath();
		ctx.arc(x + barline_width + 3 * barline_spacing,
				1 + cur_y + chord_size / 3 * 2, repeat_radius, 0, 2 * Math.PI);
		ctx.fill();
	}

	function draw_final_barline(x) {
		ctx.fillRect(x - final_barline_width - barline_width - barline_spacing,
				cur_y, barline_width, chord_size + 2);
		ctx.fillRect(x - final_barline_width, cur_y, final_barline_width,
				chord_size + 2);
	}

	function draw_num_repeats(x, num) {
		ctx.save();
		set_font(repeat_num_size, text_font);

		if (num == REPEAT_TO_CUE) {
			var text = "Until cue";
			var width = ctx.measureText(text).width
			ctx.fillText("Until cue", x - width * 1.2, cur_y);
		} else {
			var text = "x" + num;
			var width = ctx.measureText(text).width
			ctx.fillText(text, x - width * 1.4, cur_y);
		}

		ctx.restore();
	}

	function draw_push(x) {
		var pos_top = cur_y - chord_size + push_offset;
		var pos_bot = cur_y - chord_size + push_offset + push_height;
		var pos_mid = pos_bot - ((pos_bot - pos_top) / 2);

		ctx.save();
		ctx.beginPath();
		ctx.moveTo(x, pos_top);
		ctx.lineTo(x + push_width, pos_mid);
		ctx.lineTo(x, pos_bot);
		ctx.lineWidth = stroke_width;
		ctx.stroke();
		ctx.restore();
	}

	function draw_choke(x) {
		var left = x + scale;
		var right = x - scale + choke_width;
		var center = right - ((right - left) / 2);

		var top = cur_y - chord_size + choke_offset;
		var bot = cur_y - chord_size + choke_offset + choke_height;

		ctx.save();
		ctx.beginPath();
		ctx.moveTo(left, bot);
		ctx.lineTo(center, top);
		ctx.lineTo(right, bot);
		ctx.lineWidth = stroke_width;
		ctx.stroke();
		ctx.restore();
	}

	function draw_ring(x, text) {
		var chord_width = ctx.measureText(text).width;

		var left = x - ring_x_margin + ring_x_offset;
		var right = x + chord_width + ring_x_margin + ring_x_offset;
		var center_x = right - ((right - left) / 2);

		var top = cur_y - ring_y_margin + ring_y_offset;
		var bottom = cur_y + chord_size + ring_y_margin + ring_y_offset;
		var center_y = bottom - ((bottom - top) / 2);

		ctx.save();
		ctx.beginPath();
		ctx.moveTo(center_x, top);
		ctx.lineTo(right, center_y);
		ctx.lineTo(center_x, bottom);
		ctx.lineTo(left, center_y);
		ctx.lineTo(center_x, top);
		ctx.lineWidth = stroke_width;
		ctx.stroke();
		ctx.restore();
	}

	function draw_pause(x, text) {
		var text_width = ctx.measureText(text).width;
		var center = x + text_width / 2;

		ctx.save();
		ctx.lineWidth = stroke_width;

		ctx.beginPath();
		ctx.arc(center, cur_y + pause_y_offset, pause_line_radius, 0, Math.PI,
				true);
		ctx.stroke();

		ctx.beginPath();
		ctx.arc(center, cur_y + pause_y_offset, pause_dot_radius, 0,
				2 * Math.PI);
		ctx.fill();

		ctx.restore();
	}

	function draw_chord(text, x) {
		var obj = chord_parse(text);

		ctx.fillText(obj.string, x, cur_y + chord_size);

		if (obj.push && obj.choke) {
			draw_choke(x - 1 * scale);
			draw_push(x + 8 * scale);
		} else if (obj.push && obj.pause) {
			var offset = 2;
			cur_y -= push_height;
			draw_pause(x, obj.string);
			cur_y += push_height + offset * scale;
			draw_push(x);
			cur_y -= offset * scale;
		} else {
			if (obj.push)
				draw_push(x);

			if (obj.choke)
				draw_choke(x);

			if (obj.pause)
				draw_pause(x, obj.string);
		}

		if (obj.ring)
			draw_ring(x, obj.string);
	}

	function draw_signature(numerator, denominator, x) {
		var scaling = 1.8;
		ctx.save();
		draw_text_at(numerator, chord_size / scaling, chord_font,
				x - barline_padding, cur_y);
		draw_text_at(denominator, chord_size / scaling, chord_font,
				x - barline_padding, cur_y + chord_size / scaling);
		ctx.restore();
	}

	function pragma_get_data(str) {
		var index = str.indexOf(" ");
		return index > 0 ? str.substr(index + 1) : null;
	}

	function strip(str) {
		var indices = getIndicesOf(comment_delim, str, true);

		for (var i = 0; i < indices.length; i++)
			if (indices[i] > 0 && str[indices[i] - 1] == "\\")
				indices.splice(i, 1);

		return indices.length > 0
			? str.substr(0, indices[0]).trim()
			: str.trim();
	}

	function parsei(str) {
		return parseInt(str) * scale;
	}

	// Update data
	var process_pragma = function(str) {
		var tkns = str.split(" ");

		switch (tkns[0]) {
		case "#version":
			break;
		case "#push":
			push_state();
			break;
		case "#pop":
			pop_state();
			break;
		case "#title":
			title = pragma_get_data(str);
			if (title) {
				draw_text_centered(title, title_size, text_font);
				cur_y += title_size;
			}
			break;
		case "#subtitle":
			subtitle = pragma_get_data(str);
			if (subtitle) {
				draw_text_centered(subtitle, subtitle_size, text_font);
				cur_y += subtitle_size;
			}
			break;
		case "#author":
			author = pragma_get_data(str);
			if (author) {
				draw_text_right_justified(author, author_size, text_font);
				cur_y += author_size;
			}
			break;
		case "#copyright":
			copyright = pragma_get_data(str);
			if (copyright) {
				draw_copyright_text();

				if (!lock_copyright)
					cur_y += copyright_size;
			}
			break;
		case "#key":
			key = pragma_get_data(str);
			if (key) {
				draw_key_text();
			}
			break;
		case "#tempo":
			draw_tempo_text(parseInt(tkns[1]));
			break;
		case "#label":
			var label = pragma_get_data(str);
			if (label) {
				draw_text(label, label_size, text_font ,margin_left);
				cur_y += label_size + label_offset;
			}
			break;
		case "#label_at":
			var data = pragma_get_data(str)

			if (!data)
				break;

			data = data.split(" ");
			var lbl_x = parseInt(data[0]) * scale;
			var lbl_y = parseInt(data[1]) * scale;
			var lbl_msg = data.slice(2).join(" ");

			if (lbl_x >= 0 && lbl_y >= 0 && lbl_msg)
				draw_text_at(lbl_msg, label_size, text_font, lbl_x, lbl_y);

			break;
		case "#start":
			first_line = false;
			cur_y += first_line_offset;
			break;
		case "#break":
			cur_y += break_size;
			break;
		case "#include":
			console.log("Warning: Includes not yet implemented");
			break;
		case "#comment_delim":	comment_delim = tkns[1];				break;
		case "#background":		background_color = tkns[1];				break;
		case "#color":			draw_color = tkns[1];					break;
		case "#draw_barlines":	draw_barlines = (tkns[1] != "false");	break;
		case "#barline_padding":barline_padding = parsei(tkns[1]);		break;
		case "#barline_spacing":barline_spacing = parsei(tkns[1]);		break;
		case "#barline_width":	barline_width = parsei(tkns[1]);		break;
		case "#final_barline_width":
							final_barline_width = parsei(tkns[1]);		break;
		case "#repeat_radius":	repeat_radius = parsei(tkns[1]);		break;
		case "#margin_top":		margin_top = parsei(tkns[1]);			break;
		case "#margin_right":	margin_right = parsei(tkns[1]);			break;
		case "#margin_bottom":	margin_bottom = parsei(tkns[1]);		break;
		case "#margin_left":	margin_left = parsei(tkns[1]);			break;
		case "#text_font":		text_font = tkns[1];					break;
		case "#chord_font":		chord_font = tkns[1];					break;
		case "#chord_size":		chord_size = parsei(tkns[1]);			break;
		case "#label_size":		label_size = parsei(tkns[1]);			break;
		case "#repeat_num_size":repeat_num_size = parsei(tkns[1]);		break;
		case "#title_size":		title_size = parsei(tkns[1]);			break;
		case "#subtitle_size":	subtitle_size = parsei(tkns[1]);		break;
		case "#author_size":	author_size = parsei(tkns[1]);			break;
		case "#copyright_size":	copyright_size = parsei(tkns[1]);		break;
		case "#key_size":		key_size = parsei(tkns[1]);				break;
		case "#key_offset":		key_offset = parsei(tkns[1]);			break;
		case "#key_padding":	key_padding = parsei(tkns[1]);			break;
		case "#key_box":		key_box = (tkns[1] != "false");			break;
		case "#key_box_width":	key_box_width = parsei(tkns[1]);		break;
		case "#tempo_size":		tempo_size = parsei(tkns[1]);			break;
		case "#tempo_break":	tempo_break = parsei(tkns[1]);			break;
		case "#first_offset":	first_line_offset = parsei(tkns[1]);	break;
		case "#system_offset":	system_offset = parsei(tkns[1]);		break;
		case "#label_offset":	label_offset = parsei(tkns[1]);			break;
		case "#break_size":		break_size = parsei(tkns[1]);			break;
		case "#lock_copyright":	lock_copyright = (tkns[1] != "false");	break;
		case "#use_final_barline":
							use_final_barline = (tkns[1] != "false");	break;
		case "#stroke_width":	stroke_width = parsei(tkns[1]);			break;
		case "#push_offset":	push_offset = parsei(tkns[1]);			break;
		case "#push_width":		push_width = parsei(tkns[1]);			break;
		case "#push_height":	push_height = parsei(tkns[1]);			break;
		case "#ring_x_offset":	ring_x_offset = parsei(tkns[1]);		break;
		case "#ring_y_offset":	ring_y_offset = parsei(tkns[1]);		break;
		case "#ring_x_margin":	ring_x_margin = parsei(tkns[1]);		break;
		case "#ring_y_margin":	ring_y_margin = parsei(tkns[1]);		break;
		case "#choke_offset":	choke_offset = parsei(tkns[1]);			break;
		case "#choke_width":	choke_width = parsei(tkns[1]);			break;
		case "#choke_height":	choke_height = parsei(tkns[1]);			break;
		case "#pause_y_offset":	pause_y_offset = parsei(tkns[1]);		break;
		case "#pause_line_radius":pause_line_radius = parsei(tkns[1]);	break;
		case "#pause_dot_radius":pause_dot_radius = parsei(tkns[1]);	break;
		default: break;
		}
	}

	// Draw a line of chords
	var draw_chord_line = function(str, last_line) {
		if (str.length < 1)
			return;

		if (first_line) {
			cur_y += first_line_offset;
			first_line = false;
		}

		var tkns = str.split(" ");

		var first_barline = BARLINE_SINGLE;
		var bars = [];
		var in_a_bar = false;
		for (var i = 0; i < tkns.length; i++) {
			switch (tkns[i]) {
			case "|":
				if (!in_a_bar)
					in_a_bar = true;

				bars[bars.length] = new Bar();
				break;
			case "||":
				if (!in_a_bar)
					in_a_bar = true;

				if (bars.length > 0)
					bars[bars.length - 1].barline = BARLINE_DOUBLE;
				else
					first_barline = BARLINE_DOUBLE;

				bars[bars.length] = new Bar();
				break;
			case "||:":
				if (bars.length > 0) {
					if (bars[bars.length - 1].barline == BARLINE_END_REPEAT) {
						bars[bars.length - 1].barline = BARLINE_DOUBLE_REPEAT;
					} else {
						bars[bars.length - 1].barline = BARLINE_START_REPEAT;
					}
				} else {
					first_barline = BARLINE_START_REPEAT;
				}

				if (!in_a_bar)
					in_a_bar = true;

				bars[bars.length] = new Bar();
				break;
			case "|:":
				if (bars.length > 0) {
					if (bars[bars.length - 1].barline == BARLINE_END_REPEAT) {
						bars[bars.length - 1].barline = BARLINE_DOUBLE_REPEAT;
					} else {
						bars[bars.length - 1].barline = BARLINE_START_REPEAT;
					}
				} else {
					first_barline = BARLINE_START_REPEAT;
				}

				break;
			case "\\":
				in_a_bar = false;
				break;
			case "|\\":
				in_a_bar = false;

				if (bars.length > 0)
					bars[bars.length - 1].barline = BARLINE_DOUBLE;
				else
					first_barline = BARLINE_DOUBLE;

				break;
			case ":|":
				if (bars.length > 0)
					bars[bars.length - 1].barline = BARLINE_END_REPEAT;
				else
					console.log("Invalid token: :|");

				break;
			case ":||":
				if (bars.length > 0)
					bars[bars.length - 1].barline = BARLINE_END_REPEAT;
				else
					console.log("Invalid token: :||");

				if (!in_a_bar)
					in_a_bar = true;

				bars[bars.length] = new Bar();
				break;
			case ":|\\":
				if (bars.length > 0)
					bars[bars.length - 1].barline = BARLINE_END_REPEAT;
				else
					console.log("Invalid token: :|\\");

				in_a_bar = false;
				break;
			default:
				var num_str = false;

				if (tkns[i].indexOf(":||") == 0) {
					num_str = tkns[i].substr(3, tkns[i].length);
				} else if (tkns[i].indexOf(":|\\") == 0) {
					num_str = tkns[i].substr(3, tkns[i].length);
					in_a_bar = false;
				} else if (tkns[i].indexOf(":|") == 0) {
					num_str = tkns[i].substr(2, tkns[i].length);
				}

				if (num_str) {
					var num = (num_str == "c" || num_str == "C")
							? REPEAT_TO_CUE
							: parseInt(num_str);

					if (bars.length > 0) {
						bars[bars.length - 1].barline = BARLINE_END_REPEAT;

						if (num)
							bars[bars.length - 1].num_repeats = num;
					} else {
						console.log("Invalid token: " + tkns[i]);
					}

					if (tkns[i].indexOf(":||") == 0) {
						if (!in_a_bar)
							in_a_bar = true;

						bars[bars.length] = new Bar();
					}
				} else if(tkns[i].indexOf("T") == 0) {
					num_str = tkns[i].substr(1, tkns[i].length);
					var slash = num_str.indexOf("/");
					var numerator = parseInt(num_str.substr(0, slash));
					var denominator = parseInt(num_str.substr(slash + 1,
								num_str.length));

					if (numerator && denominator) {
						bars[bars.length - 1].sig_numerator = numerator;
						bars[bars.length - 1].sig_denominator = denominator;
					}
				} else {
					if (!in_a_bar)
						bars[bars.length] = new Bar();

					if (bars.length > 0)
						bars[bars.length - 1].add(tkns[i]);
				}

				break;
			}
		}

		var bar_width = (width - margin_left - margin_right) / bars.length;
		var cur_x = margin_left;

		set_font(chord_size, chord_font);

		var draw_barline_of_type = function(type, num_repeats) {
			if (draw_barlines) {
				switch (type) {
				case BARLINE_SINGLE:
					draw_barline(cur_x - barline_width);
					break;
				case BARLINE_DOUBLE:
					draw_double_barline(cur_x - barline_width);
					break;
				case BARLINE_START_REPEAT:
					draw_start_repeat_barline(cur_x - barline_width);
					break;
				case BARLINE_END_REPEAT:
					draw_end_repeat_barline(cur_x - barline_width);

					if (num_repeats)
						draw_num_repeats(cur_x, num_repeats);

					break;
				case BARLINE_DOUBLE_REPEAT:
					draw_double_repeat_barline(cur_x - barline_width);

					if (num_repeats)
						draw_num_repeats(cur_x, num_repeats);

					break;
				case BARLINE_FINAL:
					draw_final_barline(cur_x);
					break;
				}
			}
		}

		draw_barline_of_type(first_barline);

		for (var i = 0; i < bars.length; i++) {
			var chord_width = (bar_width - 2 * barline_padding) /
				bars[i].chords.length;

			for (var j = 0; j < bars[i].chords.length; j++)
				draw_chord(bars[i].chords[j],
						cur_x + barline_padding + j * chord_width);

			if (bars[i].sig_numerator > 0 && bars[i].sig_denominator > 0)
				draw_signature(bars[i].sig_numerator, bars[i].sig_denominator,
						cur_x);

			cur_x += bar_width;

			draw_barline_of_type(
					last_line && i + 1 == bars.length && use_final_barline &&
							bars[i].barline == BARLINE_SINGLE
						? BARLINE_FINAL
						: bars[i].barline,
					bars[i].num_repeats);
		}

		cur_y += chord_size + system_offset;
	}

	// Clear the canvas
	ctx.fillStyle = background_color;
	ctx.fillRect(0, 0, width, height);

	ctx.fillStyle = draw_color;

	// Process the text
	var lines = data.split("\n");

	for (var i = 0; i < lines.length; i++) {
		if (lines[i].length < 1)
			lines.splice(i, 1);
	}

	for (var i = 0; i < lines.length; i++) {
		var ln = strip(lines[i]);

		if (ln.length < 1)
			continue;

		if (ln.charAt(0) == "#")
			process_pragma(ln);
		else if(ln.charAt(0) != comment_delim)
			draw_chord_line(ln, i + 1 == lines.length);
	}
}

// Format a file title
function make_title(extension) {
	if (!title)
		return "chord_chart" + extension;

	return title
		.toLowerCase()
		.split(" ").join("_")
		.split(",").join("")
		.split(".").join("")
		.split("?").join("")
		.split("'").join("")
		.split("(").join("")
		.split(")").join("")
		+ extension
}

// Output a pdf
function make_pdf(canvas) {
	var img_data = canvas.toDataURL("image/jpeg", 1.0);

	var pdf = new jsPDF();

	pdf.addImage(img_data, 'JPEG', 0, 0, pdf.getWidth(),
			pdf.getWidth() * Math.SQRT2);

	pdf.save(make_title(".pdf"));
}

// Download a chrd file
function make_chrd(text) {
	var pom = document.createElement('a');
	pom.setAttribute('href',
			'data:text/plain;charset=utf-8,' + encodeURIComponent(text));
	pom.setAttribute('download', make_title(".chrd"));

	if (document.createEvent) {
		var event = document.createEvent('MouseEvents');
		event.initEvent('click', true, true);
		pom.dispatchEvent(event);
	} else {
		pom.click();
	}

	DIRTY = false;
}

// Run the program
function run() {
	// jsPDF plugins to get the page width and height
	(function(API) {
		API.getWidth = function() {
			return this.internal.pageSize.width;
		}
		API.getHeight = function() {
			return this.internal.pageSize.height;
		}
	})(jsPDF.API);

	// Setup the page
	$("#cl_container").html("<div id='cl_entry'>" + NEW_DOCUMENT_STRING +
		"</div>" +
		"<div id='cl_canvas_container'></div>" +
		"<div id='cl_buttons'>" +
			"<button id='cl_new_button'>New</button>" +
			"<button id='cl_chrd_button'>Save CHRD</button>" +
			"<button id='cl_pdf_button'>Export PDF</button>" +
			"<input type='file' id='cl_file_input' /> " +
		"</div>" +
		"<div id='cl_dialog'></div>");

	var create_canvas = function(w, h, ratio) {
		if (!ratio)
			ratio = (function () {
					var ctx =document.createElement("canvas").getContext("2d"),
						dpr = window.devicePixelRatio || 1,
						bsr = ctx.webkitBackingStorePixelRatio ||
							  ctx.mozBackingStorePixelRatio ||
							  ctx.msBackingStorePixelRatio ||
							  ctx.oBackingStorePixelRatio ||
							  ctx.backingStorePixelRatio || 1;
				return dpr / bsr;
			})();

		var can = document.createElement("canvas");
		can.width = w * ratio;
		can.height = h * ratio;
		can.style.width = w + "px";
		can.style.height = h + "px";
		can.getContext("2d").setTransform(ratio, 0, 0, ratio, 0, 0);

		return can;
	}

	var dpi = 300;
	var cm_per_inch = 2.54;
	var a4_width_cm = 21.0;

	var web_width = 425;
	var print_width = a4_width_cm * dpi / cm_per_inch;

	var web_height = web_width * Math.SQRT2;
	var print_height = print_width * Math.SQRT2;

	var web_scale = 1;
	var print_scale = print_width / web_width;

	var web_canvas = create_canvas(web_width, web_height);
	var print_canvas = create_canvas(print_width, print_height);

	web_canvas.id = "cl_canvas";
	print_canvas.id = "cl_print_canvas";
	print_canvas.style.display = "none";

	document.getElementById("cl_canvas_container").appendChild(web_canvas);
	document.getElementById("cl_canvas_container").appendChild(print_canvas);

	$("#cl_entry").css({
		"width": "49%",
		"height": web_height + "px",
		"font-size": "1em"
	});

	var editor = ace.edit("cl_entry");
	editor.setTheme("ace/theme/vibrant_ink");
	editor.setShowPrintMargin(false);
	editor.setHighlightActiveLine(false);
	editor.getSession().setMode("ace/mode/chrd");
	editor.getSession().setUseSoftTabs(false);
	editor.getSession().setTabSize(4);
	editor.$blockScrolling = Infinity;

	var update_web_canvas = function() {
		render(web_canvas.getContext("2d"), editor.getValue(), web_scale);
	}

	var update_print_canvas = function() {
		render(print_canvas.getContext("2d"), editor.getValue(), print_scale);
	}

	editor.getSession().on("change", function() {
		update_web_canvas();
		DIRTY = true;
	});

	document.getElementById("cl_file_input")
		.addEventListener('change', function(e) {
			var file = e.target.files[0];

			if (!file)
				return;

			var reader = new FileReader();

			reader.onload = function(e) {
				editor.setValue(e.target.result);
				DIRTY = false;
			}

			reader.readAsText(file);
		}, false);

	var new_document = function() {
		$("#cl_dialog")
			.html("Warning - This will clear the current document")
			.dialog({
				resizable: false,
				modal: true,
				buttons: {
					"Continue": function() {
						editor.setValue(NEW_DOCUMENT_STRING);
						DIRTY = false;

						$(this)
							.html("")
							.dialog("close");
					},
					Cancel: function() {
						$(this)
							.html("")
							.dialog("close");
					}
				}
			});
	}

	var save_document = function() {
		make_chrd(editor.getValue());
	}

	var export_document = function() {
		print_canvas.style.display = "block";
		update_print_canvas();
		make_pdf(print_canvas);
		print_canvas.style.display = "none";
	}

	var open_document = function() {
		$("#cl_file_input").trigger("click");
	}

	$("#cl_new_button").button().click(new_document);
	$("#cl_chrd_button").button().click(save_document);
	$("#cl_pdf_button").button().click(export_document);

	editor.commands.addCommand({
		name: 'new',
		bindKey: { win: 'Ctrl-N',  mac: 'Command-N' },
		exec: function(editor) {
			new_document();
		},
		readOnly: true
	});

	editor.commands.addCommand({
		name: 'save',
		bindKey: { win: 'Ctrl-S',  mac: 'Command-S' },
		exec: function(editor) {
			save_document();
		},
		readOnly: true
	});

	editor.commands.addCommand({
		name: 'export',
		bindKey: { win: 'Ctrl-E',  mac: 'Command-E' },
		exec: function(editor) {
			export_document();
		},
		readOnly: true
	});

	editor.commands.addCommand({
		name: 'open',
		bindKey: { win: 'Ctrl-O',  mac: 'Command-O' },
		exec: function(editor) {
			open_document();
		},
		readOnly: true
	});

	$(window).on("beforeunload", function() {
		if (DIRTY)
			return "Chordlet: You have unsaved changes that will be lost - " +
					"are you sure you want to leave?";
	});

	update_web_canvas();
	update_print_canvas();
}

// Entry point - load the font then run the program
$(document).ready(function() {
	$("head").append(
		"<style type='text/css'>@font-face{font-family:'ChordletType';" +
		"src:url('" + CHORLET_FONT_URL + "');}</style>");

	WebFont.load({
		custom: {
			families: ["ChordletType"]
		},
		active: function() {
			run();
		},
		inactive: function() {
			console.err("Chordlet: Couldn't load font - aborting");
		}
	});
});
