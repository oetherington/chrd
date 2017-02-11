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
#include "font.hpp"
#include "data.hpp"
#include "version_0_0_1.hpp"
#include "util.hpp"
#include "stb_image.h"
#include <cstdio>
#include <clocale>
#include <cmath>
#include <cstring>
#include <cassert>
#include <stdexcept>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>
#include <gtksourceview/gtksource.h>
#include <evince-view.h>
#include <evince-document.h>

static const char *const g_new_file_string = R""(#version 0.0.1
#title
#author
#key
#start
)"";

static const size_t g_new_file_string_len = strlen(g_new_file_string);

struct Opts {
	Opts(int argc, char **argv)
	{
		std::vector<std::string> args;

		for (size_t i = 1; i < argc; ++i)
			args.push_back(argv[i]);

		bool malformed = false;

		for (const std::string &arg : args) {
			if (arg == "-v" || arg == "--version") {
				printf("%s\n", GetVersionString().c_str());
				exit(EXIT_SUCCESS);
			} else if (arg == "-a" || arg == "--about") {
				printf("%s\n", GetAboutString().c_str());
				exit(EXIT_SUCCESS);
			} else if (arg == "-h" || arg == "--help") {
				printf("%s\n", GetHelpString().c_str());
				exit(EXIT_SUCCESS);
			} else {
				printf("Unrecognized argument: '%s'\n", arg.c_str());
				malformed = true;
			}
		}

		if (malformed) {
			puts("Malformed arguments - aborting\n");
			exit(EXIT_FAILURE);
		}
	}
};

struct Context {
private:
	std::string path = "";
	std::string name = "";
	bool dirty = false;
	Hash last_hash = HashString("");

public:
	GtkWidget *win;
	GError *err;
	GtkSourceView *editor;
	GtkSourceBuffer *buffer;
	EvDocument *document;
	EvDocumentModel *docmodel;
	GtkWidget *docview;

	void SetPath(const char *const p)
	{
		path = p;
		name = NameFromPath(p);
	}

	std::string GetName()
	{
		return name.size() ? name : "untitled";
	}

	std::string GetBufferContents()
	{
		GtkTextIter start, end;
		gtk_text_buffer_get_start_iter(GTK_TEXT_BUFFER(buffer), &start);
		gtk_text_buffer_get_end_iter(GTK_TEXT_BUFFER(buffer), &end);
		return gtk_text_buffer_get_text(GTK_TEXT_BUFFER(buffer),
				&start, &end, FALSE);
	}

	void ErrorPopup(const std::string &msg)
	{
		GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(win),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_CLOSE,
				"%s", msg.c_str());

		gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}

	void ErrorPopup(const Error &e)
	{
		ErrorPopup(e.ToString());
	}

	bool PromptSave()
	{
		GtkWidget *dialog = gtk_dialog_new_with_buttons("Save changes?",
				GTK_WINDOW(win),
				GTK_DIALOG_MODAL,
				"Cancel", GTK_RESPONSE_CANCEL,
				"Discard changes", GTK_RESPONSE_REJECT,
				"Save", GTK_RESPONSE_ACCEPT,
				nullptr);

		const gint res = gtk_dialog_run(GTK_DIALOG(dialog));
		bool ret;

		switch (res) {
		case GTK_RESPONSE_CANCEL:
			ret = false;
			break;
		case GTK_RESPONSE_REJECT:
			ret = true;
			break;
		case GTK_RESPONSE_ACCEPT:
			Save();
			ret = true;
			break;
		default:
			assert(0);
		}

		gtk_widget_destroy(dialog);
		return ret;
	}

	void New()
	{
		if (dirty)
			if (!PromptSave())
				return;

		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), g_new_file_string,
				g_new_file_string_len);

		path = "";
		name = "";

		ev_document_model_set_sizing_mode(docmodel, EV_SIZING_FIT_WIDTH);

		dirty = false;
	}

	void Save()
	{
		if (path.size()) {
			try {
				WriteFile(path, GetBufferContents());
			} catch (const Error &e) {
				ErrorPopup(e);
			}
		} else {
			SaveAs();
		}
	}

	void SaveAs()
	{
		GtkWidget *dialog = gtk_file_chooser_dialog_new("Save File",
				GTK_WINDOW(win),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				"Cancel", GTK_RESPONSE_CANCEL,
				"Save", GTK_RESPONSE_ACCEPT,
				nullptr);

		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
		gtk_file_chooser_set_current_name(chooser,
				(GetName() + ".chrd").c_str());

		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(chooser);

			try {
				const std::string &contents = GetBufferContents();
				WriteFile(filename, contents);
				last_hash = HashString(contents);
			} catch (const Error &e) {
				ErrorPopup(e);
			}

			SetPath(filename);

			g_free(filename);

			dirty = false;
		}

		gtk_widget_destroy(dialog);
	}

	void OnBufferChange(const std::string &text)
	{
		dirty = true;

		Version_0_0_1_Result res = Version_0_0_1(text);

		char *data = static_cast<char *>(g_malloc(
					sizeof(char) * (res.pdf_data.size() + 1)));
		memcpy(data, res.pdf_data.c_str(), res.pdf_data.size());

		GInputStream *is = g_memory_input_stream_new_from_data(
				data, res.pdf_data.size(), g_free);

		err = nullptr;
		if (!(document = ev_document_factory_get_document_for_stream(is,
				"application/pdf", static_cast<EvDocumentLoadFlags>(0),
				nullptr, &err)))
			throw Error(std::string("Can't create document: ") + err->message);

		ev_document_model_set_document(docmodel, document);
	}

	void OpenFile(const char *const file)
	{
		if (dirty)
			if (!PromptSave())
				return;

		path = file;

		const std::string text = ReadFile(file);
		gtk_text_buffer_set_text(GTK_TEXT_BUFFER(buffer), text.c_str(),
				text.size());

		ev_document_model_set_sizing_mode(docmodel, EV_SIZING_FIT_WIDTH);

		dirty = false;
	}

	void Export()
	{
		Version_0_0_1_Result res = Version_0_0_1(GetBufferContents());

		GtkWidget *dialog = gtk_file_chooser_dialog_new("Export",
				GTK_WINDOW(win),
				GTK_FILE_CHOOSER_ACTION_SAVE,
				"Cancel", GTK_RESPONSE_CANCEL,
				"Save", GTK_RESPONSE_ACCEPT,
				nullptr);

		GtkFileChooser *chooser = GTK_FILE_CHOOSER(dialog);
		gtk_file_chooser_set_do_overwrite_confirmation(chooser, TRUE);
		gtk_file_chooser_set_current_name(chooser, res.pdf_file.c_str());

		if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
			char *filename = gtk_file_chooser_get_filename(chooser);

			try {
				WriteFile(filename, res.pdf_data);
			} catch (const Error &e) {
				ErrorPopup(e);
			}

			g_free(filename);
		}

		gtk_widget_destroy(dialog);
	}

	void Print()
	{
		GtkWidget *dialog = gtk_print_unix_dialog_new("Print...",
				GTK_WINDOW(win));

		const gint res = gtk_dialog_run(GTK_DIALOG(dialog));

		switch (res) {
		case GTK_RESPONSE_OK:
			{
				// TODO

				GtkPrintUnixDialog *d = GTK_PRINT_UNIX_DIALOG(dialog);

				/* GtkPrintJob *job = gtk_print_job_new("Chrd print job", */
						/* gtk_print_unix_dialog_get_selected_printer(d), */
						/* gtk_print_unix_dialog_get_settings(d), */
						/* gtk_print_unix_dialog_get_page_setup(d)); */


				EvPrintOperation *op = ev_print_operation_new(document);
				ev_print_operation_set_job_name(op, "Chrd Print Job");
				ev_print_operation_set_print_settings(op,
						gtk_print_unix_dialog_get_settings(d));
				ev_print_operation_set_default_page_setup(op,
						gtk_print_unix_dialog_get_page_setup(d));
				ev_print_operation_run(op, GTK_WINDOW(win));
			}
			break;
		case GTK_RESPONSE_APPLY:
			// TODO Print Preview
			break;
		case GTK_RESPONSE_CANCEL:
			break;
		}

		gtk_widget_destroy(dialog);
	}

	void Quit()
	{
		if (dirty || last_hash != HashString(GetBufferContents()))
			if (!PromptSave())
				return;

		gtk_main_quit();
	}
};

static void Destroy(GtkWidget *window, gpointer data)
{
	gtk_main_quit();
}

static gboolean DeleteEvent(GtkWidget *window, GdkEvent *event, gpointer data)
{
	// Return FALSE to destroy the widget. By returning TRUE, you can cancel a
	// delete-event. This can be used to confirm quitting the application.
	return FALSE;
}

static void CallbackNew(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->New();
}

static void CallbackOpen(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	GtkWidget *dialog = gtk_file_chooser_dialog_new("Open file...",
			GTK_WINDOW(ctx->win),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			"Cancel", GTK_RESPONSE_CANCEL,
			"Open", GTK_RESPONSE_ACCEPT,
			nullptr);

	{
		GtkFileFilter *filter;

		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "Chrd files");
		gtk_file_filter_add_pattern(filter, "*.chrd");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

		filter = gtk_file_filter_new();
		gtk_file_filter_set_name(filter, "All files");
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
	}

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

		try {
			ctx->OpenFile(file);
		} catch (const Error &e) {
			g_print("Error opening file '%s': %s\n",
					file, e.ToString().c_str());
		}

		g_free(file);
	}

	gtk_widget_destroy(dialog);
}

static void CallbackSave(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->Save();
}

static void CallbackSaveAs(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->SaveAs();
}

static void CallbackExport(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->Export();
}

static void CallbackPrint(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->Print();
}

static void CallbackQuit(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ctx->Quit();
}

static void CallbackTranspose(GtkWidget *widget, gpointer data)
{
	// TODO
	g_print("Transpose\n");
}

static void CallbackPreferences(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	GtkWidget *dialog = gtk_dialog_new_with_buttons("Preferences",
			GTK_WINDOW(ctx->win),
			GTK_DIALOG_MODAL,
			"Cancel", GTK_RESPONSE_REJECT,
			"OK", GTK_RESPONSE_ACCEPT,
			nullptr);

	GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

	GtkWidget *scm_chooser = gtk_source_style_scheme_chooser_widget_new();
	gtk_source_style_scheme_chooser_set_style_scheme(
			GTK_SOURCE_STYLE_SCHEME_CHOOSER(scm_chooser),
			gtk_source_buffer_get_style_scheme(ctx->buffer));
	gtk_box_pack_start(GTK_BOX(box), scm_chooser, TRUE, TRUE, 0);

	GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));
	gtk_container_add(GTK_CONTAINER(content_area), box);
	gtk_widget_show_all(dialog);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		gtk_source_buffer_set_style_scheme(ctx->buffer,
				gtk_source_style_scheme_chooser_get_style_scheme(
					GTK_SOURCE_STYLE_SCHEME_CHOOSER(scm_chooser)));
	}

	gtk_widget_destroy(dialog);
}

static void CallbackZoomIn(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ev_document_model_set_sizing_mode(ctx->docmodel, EV_SIZING_FREE);
	if (ev_view_can_zoom_in(EV_VIEW(ctx->docview)))
		ev_view_zoom_in(EV_VIEW(ctx->docview));
}

static void CallbackZoomOut(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ev_document_model_set_sizing_mode(ctx->docmodel, EV_SIZING_FREE);
	if (ev_view_can_zoom_out(EV_VIEW(ctx->docview)))
		ev_view_zoom_out(EV_VIEW(ctx->docview));
}

static void CallbackFitPage(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ev_document_model_set_sizing_mode(ctx->docmodel, EV_SIZING_FIT_PAGE);
}

static void CallbackFitWidth(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);
	ev_document_model_set_sizing_mode(ctx->docmodel, EV_SIZING_FIT_WIDTH);
}

static void CallbackFullscreenOff(GtkWidget *widget, gpointer data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
		Context *ctx = static_cast<Context *>(data);
		gtk_window_unfullscreen(GTK_WINDOW(ctx->win));
		ev_document_model_set_fullscreen(ctx->docmodel, FALSE);
	}
}

static void CallbackFullscreenEditor(GtkWidget *widget, gpointer data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
		Context *ctx = static_cast<Context *>(data);
		ev_document_model_set_fullscreen(ctx->docmodel, FALSE);
		gtk_window_fullscreen(GTK_WINDOW(ctx->win));
	}
}

static void CallbackFullscreenDisplay(GtkWidget *widget, gpointer data)
{
	if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(widget))) {
		Context *ctx = static_cast<Context *>(data);
		gtk_window_unfullscreen(GTK_WINDOW(ctx->win));
		ev_document_model_set_fullscreen(ctx->docmodel, TRUE);
	}
}

static void CallbackToggleLineNumbers(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	gtk_source_view_set_show_line_numbers(ctx->editor,
			!gtk_source_view_get_show_line_numbers(ctx->editor));
}

static void CallbackToggleRightMargin(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	gtk_source_view_set_show_right_margin(ctx->editor,
			!gtk_source_view_get_show_right_margin(ctx->editor));
}

static void CallbackToggleShowWhitespace(GtkWidget *widget,gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	if (gtk_source_view_get_draw_spaces(ctx->editor) ==
			static_cast<GtkSourceDrawSpacesFlags>(0)) {
		gtk_source_view_set_draw_spaces(ctx->editor,
				GTK_SOURCE_DRAW_SPACES_ALL);
	} else {
		gtk_source_view_set_draw_spaces(ctx->editor,
				static_cast<GtkSourceDrawSpacesFlags>(0));
	}
}

static void CallbackToggleGrid(GtkWidget *widget,gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	if (gtk_source_view_get_background_pattern(ctx->editor) ==
			GTK_SOURCE_BACKGROUND_PATTERN_TYPE_NONE) {
		gtk_source_view_set_background_pattern(ctx->editor,
				GTK_SOURCE_BACKGROUND_PATTERN_TYPE_GRID);
	} else {
		gtk_source_view_set_background_pattern(ctx->editor,
				GTK_SOURCE_BACKGROUND_PATTERN_TYPE_NONE);
	}
}

static void CallbackToggleHighlightCurrentLine(GtkWidget *widget,gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	gtk_source_view_set_highlight_current_line(ctx->editor,
			!gtk_source_view_get_highlight_current_line(ctx->editor));
}

static void CallbackAbout(GtkWidget *widget, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(ctx->win),
			GTK_DIALOG_MODAL,
			GTK_MESSAGE_INFO,
			GTK_BUTTONS_CLOSE,
			"%s", GetAboutString().c_str());

	gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

static void CallbackHelp(GtkWidget *widget, gpointer data)
{
	g_print("Help\n");
}

static void CallbackBufferChanged(GtkTextBuffer *buf, gpointer data)
{
	Context *ctx = static_cast<Context *>(data);

	try {
		ctx->OnBufferChange(ctx->GetBufferContents());
	} catch (const Error &e) {
		g_print("Error updating view from buffer\n");
	}
}

static void CallbackIconDestroy(unsigned char *data, void *userdata)
{
	stbi_image_free(data);
}

static void UseOrUpdateFile(const std::string &path,
		const unsigned char *const data, const size_t len)
{
	if (FileExists(path.c_str()))
		if (HashFile(path.c_str()) == HashData(data, len))
			return;

	WriteFile(path, data, len);
}

int main(int argc, char **argv)
{
	// Init
	setlocale(LC_ALL, "");
	Opts opts(argc, argv);
	gtk_init(&argc, &argv);

	if (!ev_init()) {
		fprintf(stderr, "Couldn't init libevview");
		return EXIT_FAILURE;
	}

	Context ctx;

	// Setup the .chrd directory
	const std::string chrd_dir = std::string(getenv("HOME")) + "/.chrd";

	try {
		EnsureDirectoryExists(chrd_dir.c_str());
	} catch (const Error &e) {
		fprintf(stderr, "%s\n", e.ToString().c_str());
		return 1;
	}

	const auto PathFor = [=](const char *const file) {
		return chrd_dir + '/' + file;
	};

	UseOrUpdateFile(PathFor("language2.rng"),
			syntax_language2_rng, syntax_language2_rng_len);
	UseOrUpdateFile(PathFor("def.lang"),
			syntax_def_lang, syntax_def_lang_len);
	UseOrUpdateFile(PathFor("chrd.lang"),
			syntax_chrd_lang, syntax_chrd_lang_len);
	UseOrUpdateFile(PathFor("dummy.pdf"),
			dummy_dummy_pdf, dummy_dummy_pdf_len);

	// Create window
	ctx.win = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(ctx.win), "Chrd");
	gtk_window_set_default_size(GTK_WINDOW(ctx.win), 800, 600);
	gtk_window_maximize(GTK_WINDOW(ctx.win));
	g_signal_connect(G_OBJECT(ctx.win), "destroy",
			G_CALLBACK(Destroy), &ctx);
	g_signal_connect(G_OBJECT(ctx.win), "delete_event",
			G_CALLBACK(DeleteEvent), &ctx);

	// Create the window icon
	{
		int w, h, n;
		unsigned char *data = stbi_load_from_memory(icon_icon_png,
				icon_icon_png_len, &w, &h, &n, 0);

		if (data) {
			GdkPixbuf *pb = gdk_pixbuf_new_from_data(data,
					GDK_COLORSPACE_RGB,
					n == 4,	// has alpha channel
					8,		// bits per sample
					w,		// width
					h,		// height
					w * n,	// stride
					CallbackIconDestroy,
					data);

			if (pb)
				gtk_window_set_icon(GTK_WINDOW(ctx.win), pb);
			else
				fprintf(stderr, "Couldn't create GdkPixbuf for icon\n");
		} else {
			fprintf(stderr, "Couldn't decode icon PNG data\n");
		}
	}

	// Create the menu
	GtkWidget *menu_bar = gtk_menu_bar_new();

	{
		GtkWidget *menu;
		GtkWidget *mitem;

		menu = gtk_menu_new();

		mitem = gtk_menu_item_new_with_label("New");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackNew), &ctx);

		mitem = gtk_menu_item_new_with_label("Open...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackOpen), &ctx);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				gtk_separator_menu_item_new());

		mitem = gtk_menu_item_new_with_label("Save");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackSave), &ctx);

		mitem = gtk_menu_item_new_with_label("Save as...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackSaveAs), &ctx);

		mitem = gtk_menu_item_new_with_label("Export PDF...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackExport), &ctx);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				gtk_separator_menu_item_new());

		mitem = gtk_menu_item_new_with_label("Print...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackPrint), &ctx);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				gtk_separator_menu_item_new());

		mitem = gtk_menu_item_new_with_label("Quit");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackQuit), &ctx);

		mitem = gtk_menu_item_new_with_label("File");
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mitem);

		menu = gtk_menu_new();

		mitem = gtk_menu_item_new_with_label("Transpose...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackTranspose), &ctx);

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				gtk_separator_menu_item_new());

		mitem = gtk_menu_item_new_with_label("Preferences");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackPreferences), &ctx);

		mitem = gtk_menu_item_new_with_label("Edit");
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mitem);

		menu = gtk_menu_new();

		mitem = gtk_menu_item_new_with_label("Zoom in");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackZoomIn), &ctx);

		mitem = gtk_menu_item_new_with_label("Zoom out");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackZoomOut), &ctx);

		mitem = gtk_menu_item_new_with_label("Fit Page");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackFitPage), &ctx);

		mitem = gtk_menu_item_new_with_label("Fit Width");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackFitWidth), &ctx);

		mitem = gtk_menu_item_new_with_label("Fullscreen mode...");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);

		{
			GtkWidget *zmenu = gtk_menu_new();
			GSList *group = nullptr;
			GtkWidget *zitem;

			zitem = gtk_radio_menu_item_new_with_label(group, "Off");
			gtk_menu_shell_append(GTK_MENU_SHELL(zmenu), zitem);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(zitem));
			gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(zitem), TRUE);
			g_signal_connect(G_OBJECT(zitem), "activate",
					G_CALLBACK(CallbackFullscreenOff), &ctx);

			zitem = gtk_radio_menu_item_new_with_label(group, "Editor");
			gtk_menu_shell_append(GTK_MENU_SHELL(zmenu), zitem);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(zitem));
			g_signal_connect(G_OBJECT(zitem), "activate",
					G_CALLBACK(CallbackFullscreenEditor), &ctx);

			zitem = gtk_radio_menu_item_new_with_label(group, "Display");
			gtk_menu_shell_append(GTK_MENU_SHELL(zmenu), zitem);
			group = gtk_radio_menu_item_get_group(GTK_RADIO_MENU_ITEM(zitem));
			g_signal_connect(G_OBJECT(zitem), "activate",
					G_CALLBACK(CallbackFullscreenDisplay), &ctx);

			gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), zmenu);
		}

		/* gtk_menu_shell_append(GTK_MENU_SHELL(menu), */
				/* gtk_separator_menu_item_new()); */

		/* mitem = gtk_menu_item_new_with_label("Toggle fullscreen"); */
		/* gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem); */
		/* g_signal_connect(G_OBJECT(mitem), "activate", */
				/* G_CALLBACK(CallbackToggleFullscreen), &ctx); */

		gtk_menu_shell_append(GTK_MENU_SHELL(menu),
				gtk_separator_menu_item_new());

		mitem = gtk_menu_item_new_with_label("Toggle line numbers");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackToggleLineNumbers), &ctx);

		mitem = gtk_menu_item_new_with_label("Toggle right margin");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackToggleRightMargin), &ctx);

		mitem = gtk_menu_item_new_with_label("Toggle show whitespace");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackToggleShowWhitespace), &ctx);

		mitem = gtk_menu_item_new_with_label("Toggle grid");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackToggleGrid), &ctx);

		mitem = gtk_menu_item_new_with_label("Toggle highlight current line");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackToggleHighlightCurrentLine), &ctx);

		mitem = gtk_menu_item_new_with_label("View");
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mitem);

		menu = gtk_menu_new();

		mitem = gtk_menu_item_new_with_label("About");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackAbout), &ctx);

		mitem = gtk_menu_item_new_with_label("Help");
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), mitem);
		g_signal_connect(G_OBJECT(mitem), "activate",
				G_CALLBACK(CallbackHelp), &ctx);

		mitem = gtk_menu_item_new_with_label("Help");
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(mitem), menu);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu_bar), mitem);
	}

	// Init the source view
	if (!(ctx.editor = GTK_SOURCE_VIEW(gtk_source_view_new()))) {
		fprintf(stderr, "Couldn't create GtkSourceView");
		return EXIT_FAILURE;
	}

	gtk_source_view_set_show_line_numbers(ctx.editor, TRUE);
	gtk_source_view_set_show_right_margin(ctx.editor, TRUE);
	gtk_source_view_set_right_margin_position(ctx.editor, 80);
	gtk_source_view_set_highlight_current_line(ctx.editor, TRUE);
	gtk_source_view_set_auto_indent(ctx.editor, TRUE);
	gtk_source_view_set_indent_on_tab(ctx.editor, TRUE);
	gtk_source_view_set_tab_width(ctx.editor, 4);
	gtk_source_view_set_indent_width(ctx.editor, -1);
	gtk_source_view_set_smart_home_end(ctx.editor,
			GTK_SOURCE_SMART_HOME_END_BEFORE);
	gtk_source_view_set_show_line_marks(ctx.editor, FALSE);
	gtk_source_view_set_draw_spaces(ctx.editor,
			static_cast<GtkSourceDrawSpacesFlags>(0));

	ctx.buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(
				GTK_TEXT_VIEW(ctx.editor)));

	if (!(ctx.buffer)) {
		fprintf(stderr, "Couldn't get GtkSourceBuffer");
		return EXIT_FAILURE;
	}

	{
		GtkSourceStyleSchemeManager *man =
			gtk_source_style_scheme_manager_get_default();

		GtkSourceStyleScheme *scm =
			gtk_source_style_scheme_manager_get_scheme(man, "cobalt");

		if (scm)
			gtk_source_buffer_set_style_scheme(ctx.buffer, scm);
	}

	{
		GtkSourceLanguageManager *man = gtk_source_language_manager_new();

		const gchar *const paths[] = { chrd_dir.c_str(), nullptr };

		gtk_source_language_manager_set_search_path(man, (gchar**)paths);

		GtkSourceLanguage *lang =
			gtk_source_language_manager_get_language(man, "chrd");
		gtk_source_buffer_set_language(ctx.buffer, lang);
	}

	gtk_source_buffer_set_highlight_syntax(ctx.buffer, TRUE);
	gtk_source_buffer_set_highlight_matching_brackets(ctx.buffer, TRUE);
	gtk_source_buffer_set_implicit_trailing_newline(ctx.buffer, FALSE);

	g_signal_connect(G_OBJECT(ctx.buffer), "changed",
			G_CALLBACK(CallbackBufferChanged), &ctx);

	// Create the evince pdf viewer
	ctx.err = nullptr;
	if (!(ctx.document = EV_DOCUMENT(ev_document_factory_get_document(
			(std::string("file://") + PathFor("dummy.pdf")).c_str(),
			&ctx.err)))) {
		fprintf(stderr, "Couldn't create document: %s\n", ctx.err->message);
		return EXIT_FAILURE;
	}

	ctx.docmodel = EV_DOCUMENT_MODEL(ev_document_model_new_with_document(
				EV_DOCUMENT(ctx.document)));
	ev_document_model_set_sizing_mode(ctx.docmodel, EV_SIZING_FIT_WIDTH);

	ctx.docview = ev_view_new();
	ev_view_set_model(EV_VIEW(ctx.docview), EV_DOCUMENT_MODEL(ctx.docmodel));

	GtkWidget *scrolled = gtk_scrolled_window_new(nullptr, nullptr);

	gtk_container_add(GTK_CONTAINER(scrolled), GTK_WIDGET(ctx.docview));

	// Set the editor text to the new document string - this automatically
	// calls the callback to render the pdf
	ctx.New();

	// Create the layout
	GtkWidget *topbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(topbox), FALSE);

	GtkWidget *mainbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_set_homogeneous(GTK_BOX(mainbox), TRUE);

	/* GtkWidget *mainbox = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL); */

	gtk_box_pack_start(GTK_BOX(topbox), GTK_WIDGET(menu_bar), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(topbox), GTK_WIDGET(mainbox), TRUE, TRUE, 0);

	gtk_box_pack_start(GTK_BOX(mainbox), GTK_WIDGET(ctx.editor), TRUE, TRUE,0);
	gtk_box_pack_start(GTK_BOX(mainbox), scrolled, TRUE, TRUE, 0);

	/* gtk_paned_add1(GTK_PANED(mainbox), GTK_WIDGET(ctx.editor)); */
	/* gtk_paned_add2(GTK_PANED(mainbox), scrolled); */

	/* { */
		/* gint w, h; */
		/* gtk_window_get_size(GTK_WINDOW(ctx.win), &w, &h); */
		/* gtk_paned_set_position(GTK_PANED(mainbox), w / 2); */
	/* } */

	gtk_container_add(GTK_CONTAINER(ctx.win), topbox);

	// Run!
	gtk_widget_show_all(ctx.win);
	gtk_main();

	return EXIT_SUCCESS;
}
