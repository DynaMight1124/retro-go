/*
 *  Prefs_gtk.h - Global preferences, GTK specific stuff
 *
 *  Frodo Copyright (C) Christian Bauer
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "Version.h"

#include "1541d64.h"
#include "main.h"
#include "SAM.h"

#include <gtk/gtk.h>

#include <SDL.h>

#include <filesystem>
#include <format>
#include <set>
namespace fs = std::filesystem;


// GTK builder object
static GtkBuilder *builder = nullptr;

// Whether ShowEditor() was called for the first time
static bool first_time = true;

// Parameter of ShowEditor()
static bool in_startup = true;

// Result of ShowEditor()
static bool result = false;

// Pointer to preferences being edited
static Prefs * prefs = nullptr;

// Application base directory (may be empty)
static std::string app_base_dir;

// Main settings window
static GtkWindow * prefs_win = nullptr;

// Dialog for selecting snapshot file
static GtkWidget * snapshot_dialog = nullptr;
static GtkWidget * snapshot_accept_button = nullptr;

// Dialogs for creating disk or tape image files
static GtkWidget * create_disk_image_dialog = nullptr;
static GtkWidget * create_tape_image_dialog = nullptr;

// SAM text view and buffer
static GtkTextView * sam_view = nullptr;
static GtkTextBuffer * sam_buffer = nullptr;
static GtkTextMark * sam_input_start = nullptr;

// Prototypes
static void set_tape_controls();
static void set_values();
static void get_values();
static void ghost_widgets();
static void write_sam_output(std::string s, bool error = false);
extern "C" G_MODULE_EXPORT void on_tape_play_toggled(GtkToggleButton *button, gpointer user_data);
extern "C" G_MODULE_EXPORT void on_tape_record_toggled(GtkToggleButton *button, gpointer user_data);
extern "C" G_MODULE_EXPORT void on_tape_stop_toggled(GtkToggleButton *button, gpointer user_data);
extern "C" G_MODULE_EXPORT void on_cartridge_eject_clicked(GtkButton *button, gpointer user_data);

// Shortcuts window definition
static const char * shortcuts_win_ui =
	"<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
	"<interface domain=\"marker\">"
	"<object class=\"GtkShortcutsWindow\" id=\"shortcuts_win\">"
      "<property name=\"modal\">1</property>"
	  "<child>"
	    "<object class=\"GtkShortcutsSection\">"
	      "<property name=\"visible\">1</property>"
	      "<property name=\"section-name\">Keyboard Shortcuts</property>"
	      "<child>"
	        "<object class=\"GtkShortcutsGroup\">"
	          "<property name=\"visible\">1</property>"
	          "<property name=\"title\">Emulator Window</property>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">F10</property>"
	              "<property name=\"title\">Open settings window</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">F12</property>"
	              "<property name=\"title\">Reset C64</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">&lt;shift&gt;F12</property>"
	              "<property name=\"title\">Reset C64 and clear memory</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">&lt;ctrl&gt;F12</property>"
	              "<property name=\"title\">Reset C64 and auto-start from drive 1 or 8</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">KP_Enter</property>"
	              "<property name=\"title\">Toggle fullscreen mode</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">KP_Subtract</property>"
	              "<property name=\"title\">Rewind</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">KP_Add</property>"
	              "<property name=\"title\">Fast-forward</property>"
	            "</object>"
	          "</child>"
	        "</object>"
	      "</child>"
	      "<child>"
	        "<object class=\"GtkShortcutsGroup\">"
	          "<property name=\"visible\">1</property>"
	          "<property name=\"title\">C64 Keyboard</property>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">Escape</property>"
	              "<property name=\"title\">RUN/STOP</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">F11</property>"
	              "<property name=\"title\">RESTORE</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">&lt;alt&gt;</property>"
	              "<property name=\"title\">C=</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">BackSpace</property>"
	              "<property name=\"title\">INS/DEL</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">Home</property>"
	              "<property name=\"title\">CLR/HOME</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">End</property>"
	              "<property name=\"title\">£</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">Page_Up</property>"
	              "<property name=\"title\">↑</property>"
	            "</object>"
	          "</child>"
	          "<child>"
	            "<object class=\"GtkShortcutsShortcut\">"
	              "<property name=\"visible\">1</property>"
	              "<property name=\"accelerator\">Page_Down</property>"
	              "<property name=\"title\">=</property>"
	            "</object>"
	          "</child>"
	        "</object>"
	      "</child>"
	    "</object>"
	  "</child>"
	"</object>"
	"</interface>";


// Assiciation of SDL controller buttons with widgets
static const struct {
	SDL_GameControllerButton button;
	const char * widget_name;
} button_widgets[] = {
	{ SDL_CONTROLLER_BUTTON_B, "cmap_b" },
	{ SDL_CONTROLLER_BUTTON_X, "cmap_x" },
	{ SDL_CONTROLLER_BUTTON_Y, "cmap_y" },
	{ SDL_CONTROLLER_BUTTON_BACK, "cmap_back" },
	{ SDL_CONTROLLER_BUTTON_GUIDE, "cmap_guide" },
	{ SDL_CONTROLLER_BUTTON_START, "cmap_start" },
	{ SDL_CONTROLLER_BUTTON_LEFTSTICK, "cmap_leftstick" },
	{ SDL_CONTROLLER_BUTTON_RIGHTSTICK, "cmap_rightstick" },
	{ SDL_CONTROLLER_BUTTON_LEFTSHOULDER, "cmap_leftshoulder" },
	{ SDL_CONTROLLER_BUTTON_RIGHTSHOULDER, "cmap_rightshoulder" },
	{ SDL_CONTROLLER_BUTTON_MISC1, "cmap_misc1" },
	{ SDL_CONTROLLER_BUTTON_PADDLE1, "cmap_paddle1" },
	{ SDL_CONTROLLER_BUTTON_PADDLE2, "cmap_paddle2" },
	{ SDL_CONTROLLER_BUTTON_PADDLE3, "cmap_paddle3" },
	{ SDL_CONTROLLER_BUTTON_PADDLE4, "cmap_paddle4" },
	{ SDL_CONTROLLER_BUTTON_TOUCHPAD, "cmap_touchpad" },
};


// Mainloop idler to keep SDL events going
static guint idle_source_id = 0;

static gboolean pump_sdl_events(gpointer user_data)
{
	SDL_PumpEvents();
	return G_SOURCE_CONTINUE;
}


/*
 *  Show preferences editor (synchronously)
 *  prefs_path is the preferences file name
 *  snapshot_path is the default directory for snapshots
 */

bool Prefs::ShowEditor(bool startup, fs::path prefs_path, fs::path snapshot_path)
{
	in_startup = startup;
	prefs = this;

	if (first_time) {
		first_time = false;

		// Load user interface file if called the first time
		builder = gtk_builder_new();

		auto base_path = SDL_GetBasePath();
		if (base_path) {
			app_base_dir = base_path;
			SDL_free(base_path);
		}

		GError * error = nullptr;
		bool success = false;

		// Look in base dir first, then in DATADIR
		if (! app_base_dir.empty()) {
			success = gtk_builder_add_from_file(builder, (app_base_dir + "Frodo.ui").c_str(), &error) > 0;
		}
		if (! success) {
			if (error) {
				g_free(error);
				error = nullptr;
			}
			success = gtk_builder_add_from_file(builder, DATADIR "Frodo.ui", &error) > 0;
		}

		if (! success) {

			// No UI means no prefs editor
			g_warning("Couldn't load preferences UI definition: %s\nPreferences editor not available.\n", error->message);
			g_free(error);
			g_object_unref(builder);
			builder = nullptr;
			return startup;
		}

		gtk_builder_connect_signals(builder, nullptr);
		prefs_win = GTK_WINDOW(gtk_builder_get_object(builder, "prefs_win"));

		if (IsFrodoSC) {

			// Remove "Advanced" page in regular Frodo
			gtk_notebook_remove_page(GTK_NOTEBOOK(gtk_builder_get_object(builder, "tabs")), -1);

		} else {

			// Hide "Tape Drive Path" frame in Frodo Lite
			gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_path_frame")));
		}

		// Create dialog for loading/saving snapshot files
		snapshot_dialog = gtk_file_chooser_dialog_new("", prefs_win,
			GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", GTK_RESPONSE_CANCEL, nullptr
		);
		snapshot_accept_button = gtk_dialog_add_button(GTK_DIALOG(snapshot_dialog), "Save", GTK_RESPONSE_ACCEPT);
		gtk_dialog_set_default_response(GTK_DIALOG(snapshot_dialog), GTK_RESPONSE_ACCEPT);

		gtk_file_chooser_add_shortcut_folder(GTK_FILE_CHOOSER(snapshot_dialog), snapshot_path.string().c_str(), nullptr);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(snapshot_dialog), snapshot_path.string().c_str());
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(snapshot_dialog), "Untitled.snap");
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(snapshot_dialog), true);

		GtkFileFilter * filter;
		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*.snap");
		gtk_file_filter_set_name(filter, "Snapshot Files (*.snap)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(snapshot_dialog), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_filter_set_name(filter, "All Files (*.*)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(snapshot_dialog), filter);

		// Create dialog for creating blank disk image file
		create_disk_image_dialog = gtk_file_chooser_dialog_new("Create Disk Image File", prefs_win,
			GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, nullptr
		);

		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(create_disk_image_dialog), "Untitled.d64");
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(create_disk_image_dialog), true);

		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*.d64");
		gtk_file_filter_set_name(filter, "C64 Disk Image Files (*.d64)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(create_disk_image_dialog), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_filter_set_name(filter, "All Files (*.*)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(create_disk_image_dialog), filter);

		// Create dialog for creating blank tape image file
		create_tape_image_dialog = gtk_file_chooser_dialog_new("Create Tape Image File", prefs_win,
			GTK_FILE_CHOOSER_ACTION_SAVE, "Cancel", GTK_RESPONSE_CANCEL, "Save", GTK_RESPONSE_ACCEPT, nullptr
		);

		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(create_tape_image_dialog), "Untitled.tap");
		gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(create_tape_image_dialog), true);

		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*.tap");
		gtk_file_filter_set_name(filter, "C64 Tape Image Files (*.tap)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(create_tape_image_dialog), filter);
		filter = gtk_file_filter_new();
		gtk_file_filter_add_pattern(filter, "*");
		gtk_file_filter_set_name(filter, "All Files (*.*)");
		gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(create_tape_image_dialog), filter);

		// Set up SAM view
		sam_view = GTK_TEXT_VIEW(gtk_builder_get_object(builder, "sam_view"));
		sam_buffer = gtk_text_view_get_buffer(sam_view);

		gtk_text_buffer_create_tag(sam_buffer, "protected", "editable", false, nullptr);
		gtk_text_buffer_create_tag(sam_buffer, "error", "foreground", "red", nullptr);

		GtkTextIter start;
		gtk_text_buffer_get_start_iter(sam_buffer, &start);
		sam_input_start = gtk_text_buffer_create_mark(sam_buffer, "input_start", &start, true);

		write_sam_output(SAM_GetStartupMessage());
		write_sam_output(SAM_GetPrompt());

		// Set up C64 keyboard combo boxes
		std::set<std::string> c64_key_names;
		for (unsigned keycode = 0; keycode < NUM_C64_KEYCODES; ++keycode) {
			c64_key_names.insert(StringForKeycode(keycode));
		}

		for (const auto & [_, widget_name] : button_widgets) {
			GtkComboBoxText * box = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, widget_name));
			for (const auto & key_name : c64_key_names) {
				gtk_combo_box_text_append(box, key_name.c_str(), key_name.c_str());	// ID = key name
			}
		}

		// Create file filters for ROM file choosers
		auto allow_files_with_size = [](const GtkFileFilterInfo * filter_info, gpointer user_data) -> gboolean {
			if (fs::is_regular_file(filter_info->filename)) {
				return fs::file_size(filter_info->filename) == (size_t) user_data && strchr(filter_info->filename, ';') == nullptr;
			}
			return true;
		};

		GtkFileChooser * rom_chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "basic_rom_path"));
		filter = gtk_file_filter_new();
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, allow_files_with_size, (gpointer) BASIC_ROM_SIZE, nullptr);
		gtk_file_chooser_add_filter(rom_chooser, filter);

		rom_chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "kernal_rom_path"));
		filter = gtk_file_filter_new();
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, allow_files_with_size, (gpointer) KERNAL_ROM_SIZE, nullptr);
		gtk_file_chooser_add_filter(rom_chooser, filter);

		rom_chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "char_rom_path"));
		filter = gtk_file_filter_new();
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, allow_files_with_size, (gpointer) CHAR_ROM_SIZE, nullptr);
		gtk_file_chooser_add_filter(rom_chooser, filter);

		rom_chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive_rom_path"));
		filter = gtk_file_filter_new();
		gtk_file_filter_add_custom(filter, GTK_FILE_FILTER_FILENAME, allow_files_with_size, (gpointer) DRIVE_ROM_SIZE, nullptr);
		gtk_file_chooser_add_filter(rom_chooser, filter);
	}

	// No UI means no prefs editor
	if (builder == nullptr)
		return startup;

	if (startup) {

		// Adjust menus and widgets for startup
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(builder, "ok_menu")), "Start");
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "ok_button")), "Start");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "load_snapshot_menu")), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "save_snapshot_menu")), false);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "sam_menu")), false);

		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_position")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_play")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_record")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_stop")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_rewind")));
		gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "tape_forward")));

	} else {

		// Adjust menus and widgets for running emulation
		gtk_menu_item_set_label(GTK_MENU_ITEM(gtk_builder_get_object(builder, "ok_menu")), "Continue");
		gtk_button_set_label(GTK_BUTTON(gtk_builder_get_object(builder, "ok_button")), "Continue");
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "load_snapshot_menu")), true);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "save_snapshot_menu")), true);
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "sam_menu")), true);

		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_position")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_play")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_record")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_stop")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_rewind")));
		gtk_widget_show(GTK_WIDGET(gtk_builder_get_object(builder, "tape_forward")));

		set_tape_controls();

#ifndef __APPLE__
		// Keep SDL event loop running while preferences editor is open
		// to prevent "Application is not responding" messages from the WM
		idle_source_id = g_timeout_add(10, pump_sdl_events, nullptr);
#endif

		SAM_GetState(TheC64);
	}

	// Run editor
	result = false;
	set_values();

	gtk_window_present(prefs_win);
	gtk_main();

	if (idle_source_id > 0) {
		g_source_remove(idle_source_id);
		idle_source_id = 0;
	}

	// Save preferences if "Start"/"Continue" clicked
	if (result) {
		get_values();
		prefs->Save(prefs_path);

		if (! startup) {
			SAM_SetState(TheC64);
		}
	}

	return result;
}


/*
 *  Set the tape control buttons to the current tape state
 */

static void set_tape_controls()
{
	GtkWidget * tape_play = GTK_WIDGET(gtk_builder_get_object(builder, "tape_play"));
	GtkWidget * tape_record = GTK_WIDGET(gtk_builder_get_object(builder, "tape_record"));
	GtkWidget * tape_stop = GTK_WIDGET(gtk_builder_get_object(builder, "tape_stop"));

	TapeState tape_buttons = TheC64->TapeButtonState();

	g_signal_handlers_block_by_func(G_OBJECT(tape_play), (void *) on_tape_play_toggled, nullptr);
	g_signal_handlers_block_by_func(G_OBJECT(tape_record), (void *) on_tape_record_toggled, nullptr);
	g_signal_handlers_block_by_func(G_OBJECT(tape_stop), (void *) on_tape_stop_toggled, nullptr);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tape_play), tape_buttons == TapeState::Play);
	gtk_widget_set_sensitive(tape_play, tape_buttons != TapeState::Play);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tape_record), tape_buttons == TapeState::Record);
	gtk_widget_set_sensitive(tape_record, tape_buttons != TapeState::Record);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tape_stop), tape_buttons == TapeState::Stop);
	gtk_widget_set_sensitive(tape_stop, tape_buttons != TapeState::Stop);

	g_signal_handlers_unblock_by_func(G_OBJECT(tape_play), (void *) on_tape_play_toggled, nullptr);
	g_signal_handlers_unblock_by_func(G_OBJECT(tape_record), (void *) on_tape_record_toggled, nullptr);
	g_signal_handlers_unblock_by_func(G_OBJECT(tape_stop), (void *) on_tape_stop_toggled, nullptr);

	gtk_label_set_text(GTK_LABEL(gtk_builder_get_object(builder, "tape_position")), std::format("Position: {}%", TheC64->TapePosition()).c_str());
}


/*
 *  Set the values of the widgets
 */

static void create_joystick_menu(const char * widget_name)
{
	GtkComboBoxText * w = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, widget_name));

	gtk_combo_box_text_remove_all(w);
	gtk_combo_box_text_append_text(w, "None");

	for (int i = 0; i < SDL_NumJoysticks(); ++i) {
		gtk_combo_box_text_append_text(w, SDL_JoystickNameForIndex(i));
	}
}

static void create_button_map_menu(const std::string & selected_button_map)
{
	GtkComboBoxText * button_map = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "button_map"));

	gtk_combo_box_text_remove_all(button_map);
	gtk_combo_box_text_append_text(button_map, "Default");

	for (const auto & [name, _] : prefs->ButtonMapDefs) {
		gtk_combo_box_text_append(button_map, name.c_str(), name.c_str());	// ID = name
	}

	if (selected_button_map.empty()) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(button_map), 0);	// Built-In
	} else {
		if (! gtk_combo_box_set_active_id(GTK_COMBO_BOX(button_map), selected_button_map.c_str())) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(button_map), 0);
		}
	}
}

static void create_rom_set_menu(const std::string & selected_rom_set)
{
	GtkComboBoxText * rom_set = GTK_COMBO_BOX_TEXT(gtk_builder_get_object(builder, "rom_set"));

	gtk_combo_box_text_remove_all(rom_set);
	gtk_combo_box_text_append_text(rom_set, "Built-In");

	for (const auto & [name, _] : prefs->ROMSetDefs) {
		gtk_combo_box_text_append(rom_set, name.c_str(), name.c_str());	// ID = name
	}

	if (selected_rom_set.empty()) {
		gtk_combo_box_set_active(GTK_COMBO_BOX(rom_set), 0);	// Built-In
	} else {
		if (! gtk_combo_box_set_active_id(GTK_COMBO_BOX(rom_set), selected_rom_set.c_str())) {
			gtk_combo_box_set_active(GTK_COMBO_BOX(rom_set), 0);
		}
	}
}

static void set_values()
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "emul1541_proc")), prefs->Emul1541Proc);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "map_slash")), prefs->MapSlash);

	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive8_path")), prefs->DrivePath[0].c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive9_path")), prefs->DrivePath[1].c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive10_path")), prefs->DrivePath[2].c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive11_path")), prefs->DrivePath[3].c_str());

	if (IsFrodoSC) {
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "tape_path")), prefs->TapePath.c_str());
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "display_type")), prefs->DisplayType);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "scaling_numerator")), prefs->ScalingNumerator - 1);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "palette")), prefs->Palette);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "sprite_collisions")), prefs->SpriteCollisions);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "show_leds")), prefs->ShowLEDs);

	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "sid_type")), prefs->SIDType);

	create_joystick_menu("joystick1_port");
	create_joystick_menu("joystick2_port");
	create_button_map_menu(prefs->ButtonMap);

	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "joystick1_port")), prefs->Joystick1Port);
	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "joystick2_port")), prefs->Joystick2Port);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "joystick_swap")), prefs->JoystickSwap);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "twin_stick")), prefs->TwinStick);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "tape_rumble")), prefs->TapeRumble);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "limit_speed")), prefs->LimitSpeed);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "fast_reset")), prefs->FastReset);

	gtk_combo_box_set_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "reu_type")), prefs->REUType);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "cartridge_path")), prefs->CartridgePath.c_str());
	create_rom_set_menu(prefs->ROMSet);

	if (! IsFrodoSC) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "normal_cycles")), prefs->NormalCycles);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "bad_line_cycles")), prefs->BadLineCycles);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "cia_cycles")), prefs->CIACycles);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "floppy_cycles")), prefs->FloppyCycles);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "cia_irq_hack")), prefs->CIAIRQHack);
	}

	ghost_widgets();
}


/*
 *  Get the values of the widgets
 */

static void get_drive_path(int num, const char *widget_name)
{
	gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, widget_name)));
	if (path) {
		prefs->DrivePath[num] = path;
		g_free(path);
	} else {
		prefs->DrivePath[num].clear();
	}
}

static std::string get_selected_button_map()
{
	GtkComboBox * button_map = GTK_COMBO_BOX(gtk_builder_get_object(builder, "button_map"));
	if (gtk_combo_box_get_active(button_map) == 0) {
		return {};	// Built-In
	} else {
		return gtk_combo_box_get_active_id(button_map);
	}
}

static std::string get_selected_rom_set()
{
	GtkComboBox * rom_set = GTK_COMBO_BOX(gtk_builder_get_object(builder, "rom_set"));
	if (gtk_combo_box_get_active(rom_set) == 0) {
		return {};	// Built-In
	} else {
		return gtk_combo_box_get_active_id(rom_set);
	}
}

static void get_values()
{
	gchar * path;

	prefs->Emul1541Proc = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "emul1541_proc")));
	prefs->MapSlash = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "map_slash")));

	get_drive_path(0, "drive8_path");
	get_drive_path(1, "drive9_path");
	get_drive_path(2, "drive10_path");
	get_drive_path(3, "drive11_path");

	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "tape_path")));
	if (path) {
		prefs->TapePath = path;
		g_free(path);
	} else {
		prefs->TapePath.clear();
	}

	prefs->DisplayType = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "display_type")));
	prefs->ScalingNumerator = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "scaling_numerator"))) + 1;
	prefs->ScalingDenominator = 1;  // for now...
	prefs->Palette = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "palette")));
	prefs->SpriteCollisions = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "sprite_collisions")));
	prefs->ShowLEDs = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "show_leds")));

	prefs->SIDType = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "sid_type")));

	prefs->Joystick1Port = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "joystick1_port")));
	prefs->Joystick2Port = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "joystick2_port")));
	prefs->JoystickSwap = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "joystick_swap")));
	prefs->TwinStick = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "twin_stick")));
	prefs->TapeRumble = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "tape_rumble")));
	prefs->ButtonMap = get_selected_button_map();

	prefs->LimitSpeed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "limit_speed")));
	prefs->FastReset = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "fast_reset")));

	prefs->REUType = gtk_combo_box_get_active(GTK_COMBO_BOX(gtk_builder_get_object(builder, "reu_type")));
	path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "cartridge_path")));
	if (path) {
		prefs->CartridgePath = path;
		g_free(path);
	} else {
		prefs->CartridgePath.clear();
	}
	prefs->ROMSet = get_selected_rom_set();

	if (! IsFrodoSC) {
		prefs->NormalCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "normal_cycles")));
		prefs->BadLineCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "bad_line_cycles")));
		prefs->CIACycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "cia_cycles")));
		prefs->FloppyCycles = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "floppy_cycles")));

		prefs->CIAIRQHack = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(builder, "cia_irq_hack")));
	}
}


/*
 *  Ghost/unghost widgets
 */

static void ghost_widget(const char *name, bool ghosted)
{
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, name)), !ghosted);
}

static void ghost_widgets()
{
	ghost_widget("drive9_path", prefs->Emul1541Proc);
	ghost_widget("drive10_path", prefs->Emul1541Proc);
	ghost_widget("drive11_path", prefs->Emul1541Proc);
	ghost_widget("drive9_eject", prefs->Emul1541Proc);
	ghost_widget("drive10_eject", prefs->Emul1541Proc);
	ghost_widget("drive11_eject", prefs->Emul1541Proc);
	ghost_widget("drive9_next_disk", prefs->Emul1541Proc);
	ghost_widget("drive10_next_disk", prefs->Emul1541Proc);
	ghost_widget("drive11_next_disk", prefs->Emul1541Proc);
	ghost_widget("map_slash", prefs->Emul1541Proc);

	ghost_widget("scaling_numerator", prefs->DisplayType == DISPTYPE_SCREEN);

	ghost_widget("cartridge_path", prefs->REUType != REU_NONE);

	// Auto-start is available if drive 8 or 1 has a path
	GtkButton * auto_start = GTK_BUTTON(gtk_builder_get_object(builder, "auto_start_button"));
	gchar * drive8_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive8_path")));
	gchar * tape_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "tape_path")));
	if (drive8_path) {
		gtk_button_set_label(auto_start, "Auto-Start Drive 8");
		gtk_widget_set_sensitive(GTK_WIDGET(auto_start), true);
	} else if (tape_path) {
		gtk_button_set_label(auto_start, "Auto-Start Drive 1");
		gtk_widget_set_sensitive(GTK_WIDGET(auto_start), true);
	} else {
		gtk_widget_set_sensitive(GTK_WIDGET(auto_start), false);
	}
	g_free(drive8_path);
	g_free(tape_path);
}


/*
 *  SAM window handling
 *  (inspired by https://github.com/SvenFestersen/GtkPyInterpreter)
 */

static void write_sam_output(std::string s, bool error)
{
	// Append non-editable text
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(sam_buffer, &end);
	if (error) {
		gtk_text_buffer_insert_with_tags_by_name(sam_buffer, &end, s.c_str(), -1, "protected", "error", nullptr);
	} else {
		gtk_text_buffer_insert_with_tags_by_name(sam_buffer, &end, s.c_str(), -1, "protected", nullptr);
	}

	// Make cursor visible
	gtk_text_view_scroll_mark_onscreen(sam_view, gtk_text_buffer_get_insert(sam_buffer));

	// Move "start of input" mark to end of text
	gtk_text_buffer_get_end_iter(sam_buffer, &end);
	gtk_text_buffer_move_mark(sam_buffer, sam_input_start, &end);
}

extern "C" G_MODULE_EXPORT gboolean on_sam_view_key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
	const char * key = gdk_keyval_name(event->keyval);
	if (strcmp(key, "Return") == 0) {

		// Return pressed, fetch text input since last mark set
		GtkTextIter start, end;
		gtk_text_buffer_get_iter_at_mark(sam_buffer, &start, sam_input_start);
		gtk_text_buffer_get_end_iter(sam_buffer, &end);
		const char * input = gtk_text_buffer_get_text(sam_buffer, &start, &end, true);

		// Append newline and make input non-editable
		gtk_text_buffer_insert(sam_buffer, &end, "\n", -1);
		gtk_text_buffer_get_iter_at_mark(sam_buffer, &start, sam_input_start);	// 'insert' has invalidated the iterators
		gtk_text_buffer_get_end_iter(sam_buffer, &end);
		gtk_text_buffer_apply_tag_by_name(sam_buffer, "protected", &start, &end);

		// Execute command
		std::string cmdOutput, cmdError;
		SAM_Exec(input, cmdOutput, cmdError);

		// Show output and prompt
		write_sam_output(cmdOutput.c_str());
		if (! cmdError.empty()) {
			write_sam_output(cmdError.c_str(), true);
		}
		write_sam_output(SAM_GetPrompt());

		return true;

	} else if (strcmp(key, "Up") == 0) {

		// Suppress "up" key
		// TODO: command history?
		return true;

	} else if (strcmp(key, "Left") == 0) {

		// Prevent cursor from moving into prompt
		GtkTextIter cursor, input_start;
		gtk_text_buffer_get_iter_at_mark(sam_buffer, &cursor, gtk_text_buffer_get_insert(sam_buffer));
		gtk_text_buffer_get_iter_at_mark(sam_buffer, &input_start, sam_input_start);
		return gtk_text_iter_equal(&cursor, &input_start);

	} else if (strcmp(key, "Home") == 0) {

		// Move cursor to start of input
		GtkTextIter input_start;
		gtk_text_buffer_get_iter_at_mark(sam_buffer, &input_start, sam_input_start);
		gtk_text_buffer_place_cursor(sam_buffer, &input_start);
		return true;
	}

	return false;
}

extern "C" G_MODULE_EXPORT void on_sam_copy_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_text_buffer_copy_clipboard(sam_buffer, gtk_clipboard_get_default(gdk_display_get_default()));
}

extern "C" G_MODULE_EXPORT void on_sam_select_all_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkTextIter start, end;
	gtk_text_buffer_get_start_iter(sam_buffer, &start);
	gtk_text_buffer_get_end_iter(sam_buffer, &end);
	gtk_text_buffer_select_range(sam_buffer, &start, &end);
}

extern "C" G_MODULE_EXPORT void on_sam_clear_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_text_buffer_set_text(sam_buffer, "", -1);
	write_sam_output(SAM_GetPrompt());
}

extern "C" G_MODULE_EXPORT void on_sam_close_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "sam_win")));
}


/*
 *  Controller button mapping editor window handling
 */

static GtkListBoxRow * add_button_mapping_editor_item(const std::string & name, const ButtonMapping & m)
{
	GtkListBox * button_map_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "button_map_list"));
	GtkContainer * row = GTK_CONTAINER(gtk_list_box_row_new());

	// Store pointer to associated ButtonMapping as custom data object
	g_object_set_data(G_OBJECT(row), "mapping", new ButtonMapping(m));

	GtkWidget * label = gtk_label_new(name.c_str());
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_container_add(row, label);

	gtk_container_add(GTK_CONTAINER(button_map_list), GTK_WIDGET(row));
	return GTK_LIST_BOX_ROW(row);
}

static std::string name_of_button_map_row(const GtkListBoxRow * row)
{
	return gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
}

static ButtonMapping * mapping_of_button_map_row(const GtkListBoxRow * row)
{
	return (ButtonMapping *) g_object_get_data(G_OBJECT(row), "mapping");
}

static int button_map_list_sort_func(GtkListBoxRow * row1, GtkListBoxRow * row2, gpointer user_data)
{
	auto ord = name_of_button_map_row(row1) <=> name_of_button_map_row(row2);
	if (ord > 0) {
		return 1;
	} else if (ord < 0) {
		return -1;
	} else {
		return 0;
	}
}

static GtkListBoxRow * selected_button_map_row()
{
	GtkListBox * button_map_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "button_map_list"));
	return gtk_list_box_get_selected_row(button_map_list);
}

static void ghost_button_mapping_editor_widgets(bool ghosted)
{
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "button_map_name")), !ghosted);
	for (const auto & [_, widget_name] : button_widgets) {
		gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, widget_name)), !ghosted);
	}
}

static void update_button_mapping_editor_widgets(const std::string & name, const ButtonMapping & m)
{
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "button_map_name")), name.c_str());
	for (const auto & [button, widget_name] : button_widgets) {
		GtkComboBox * box = GTK_COMBO_BOX(gtk_builder_get_object(builder, widget_name));
		auto it = m.find(button);
		if (it == m.end()) {
			gtk_combo_box_set_active(box, 0);	// None
		} else {
			gtk_combo_box_set_active_id(box, StringForKeycode(it->second));
		}
	}
}

extern "C" G_MODULE_EXPORT void on_new_button_mapping_clicked(GtkButton *button, gpointer user_data)
{
	GtkListBox * button_map_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "button_map_list"));
	GtkListBoxRow * row = add_button_mapping_editor_item("unnamed mapping", ButtonMapping{});
	gtk_widget_show_all(GTK_WIDGET(button_map_list));
	gtk_list_box_select_row(button_map_list, row);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "button_map_name")));
}

extern "C" G_MODULE_EXPORT void on_delete_button_mapping_clicked(GtkButton *button, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_button_map_row()) {
		delete mapping_of_button_map_row(row);
		gtk_widget_destroy(GTK_WIDGET(row));

		ghost_button_mapping_editor_widgets(true);
		update_button_mapping_editor_widgets("", ButtonMapping{});
	}
}

extern "C" G_MODULE_EXPORT void on_button_map_list_row_selected(GtkListBox *self, GtkListBoxRow *row, gpointer user_data)
{
	if (row) {
		ghost_button_mapping_editor_widgets(false);
		update_button_mapping_editor_widgets(name_of_button_map_row(row), *mapping_of_button_map_row(row));
	} else {
		ghost_button_mapping_editor_widgets(true);
		update_button_mapping_editor_widgets("", ButtonMapping{});
	}
}

extern "C" G_MODULE_EXPORT void on_button_map_name_insert_text(GtkEditable *self, gchar *new_text, gint new_text_length, gint *position, gpointer user_data)
{
	// Prevent ';' characters in mapping names
	if (strchr(new_text, ';') == nullptr) {
		g_signal_handlers_block_by_func(G_OBJECT(self), (void *) on_button_map_name_insert_text, user_data);
		gtk_editable_insert_text(self, new_text, new_text_length, position);
		g_signal_handlers_unblock_by_func(G_OBJECT(self), (void *) on_button_map_name_insert_text, user_data);
	}
	g_signal_stop_emission_by_name(self, "insert-text");
}

extern "C" G_MODULE_EXPORT void on_button_map_name_changed(GtkEditable *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_button_map_row()) {
		const gchar * name = gtk_entry_get_text(GTK_ENTRY(self));
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))), name);
		gtk_list_box_invalidate_sort(GTK_LIST_BOX(gtk_builder_get_object(builder, "button_map_list")));
	}
}

extern "C" G_MODULE_EXPORT void on_cmap_changed(GtkComboBox *box, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_button_map_row()) {
		std::string my_name = gtk_buildable_get_name(GTK_BUILDABLE(box));
		for (const auto & [button, widget_name] : button_widgets) {
			if (my_name == widget_name) {
				if (gtk_combo_box_get_active(box) > 0) {
					(*mapping_of_button_map_row(row))[button] = KeycodeFromString(gtk_combo_box_get_active_id(box));
				} else {
					mapping_of_button_map_row(row)->erase(button);	// None
				}
				break;
			}
		}
	}
}

extern "C" G_MODULE_EXPORT void on_edit_button_mappings_clicked(GtkButton *button, gpointer user_data)
{
	GtkDialog * editor = GTK_DIALOG(gtk_builder_get_object(builder, "button_mapping_editor"));
	GtkListBox * button_map_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "button_map_list"));

	// Keep mapping list sorted by name
	gtk_list_box_set_sort_func(button_map_list, button_map_list_sort_func, nullptr, nullptr);

	std::string selected_button_map = get_selected_button_map();

	// Repopulate mapping list
	GList * children = gtk_container_get_children(GTK_CONTAINER(button_map_list));
	for (GList * it = children; it != nullptr; it = g_list_next(it)) {
		GtkListBoxRow * row = GTK_LIST_BOX_ROW(it->data);
		delete mapping_of_button_map_row(row);
		gtk_widget_destroy(GTK_WIDGET(row));
	}
	g_list_free(children);

	for (const auto & [name, mapping] : prefs->ButtonMapDefs) {
		GtkListBoxRow * row = add_button_mapping_editor_item(name, mapping);
		if (name.c_str() == selected_button_map) {
			gtk_list_box_select_row(button_map_list, row);
		}
	}
	gtk_widget_show_all(GTK_WIDGET(button_map_list));

	// Run editor dialog
	gint res = gtk_dialog_run(editor);
	if (res == GTK_RESPONSE_ACCEPT) {

		// Read back mapping list
		prefs->ButtonMapDefs.clear();

		children = gtk_container_get_children(GTK_CONTAINER(button_map_list));
		for (GList * it = children; it != nullptr; it = g_list_next(it)) {
			GtkListBoxRow * row = GTK_LIST_BOX_ROW(it->data);

			// Prevent empty names, enforce uniqueness
			auto name = name_of_button_map_row(row);
			if (name.empty()) {
				name = "unnamed mapping";
			}

			auto base_name = name;
			unsigned index = 1;
			while (prefs->ButtonMapDefs.count(name) > 0) {
				name = base_name + std::to_string(index);
				++index;
			}

			prefs->ButtonMapDefs[name] = *mapping_of_button_map_row(row);
		}
		g_list_free(children);

		create_button_map_menu(selected_button_map);
	}

	gtk_widget_hide(GTK_WIDGET(editor));
}


/*
 *  ROM set editor window handling
 */

static GtkListBoxRow * add_rom_set_editor_item(const std::string & name, const ROMPaths & p)
{
	GtkListBox * rom_set_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "rom_set_list"));
	GtkContainer * row = GTK_CONTAINER(gtk_list_box_row_new());

	// Store pointer to associated ROMPaths as custom data object
	g_object_set_data(G_OBJECT(row), "roms", new ROMPaths(p));

	GtkWidget * label = gtk_label_new(name.c_str());
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_container_add(row, label);

	gtk_container_add(GTK_CONTAINER(rom_set_list), GTK_WIDGET(row));
	return GTK_LIST_BOX_ROW(row);
}

static std::string name_of_rom_set_row(const GtkListBoxRow * row)
{
	return gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
}

static ROMPaths * paths_of_rom_set_row(const GtkListBoxRow * row)
{
	return (ROMPaths *) g_object_get_data(G_OBJECT(row), "roms");
}

static int rom_set_list_sort_func(GtkListBoxRow * row1, GtkListBoxRow * row2, gpointer user_data)
{
	auto ord = name_of_rom_set_row(row1) <=> name_of_rom_set_row(row2);
	if (ord > 0) {
		return 1;
	} else if (ord < 0) {
		return -1;
	} else {
		return 0;
	}
}

static GtkListBoxRow * selected_rom_set_row()
{
	GtkListBox * rom_set_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "rom_set_list"));
	return gtk_list_box_get_selected_row(rom_set_list);
}

static void ghost_rom_set_editor_widgets(bool ghosted)
{
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "rom_set_name")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "basic_rom_path")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "kernal_rom_path")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "char_rom_path")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "drive_rom_path")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "basic_rom_builtin")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "kernal_rom_builtin")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "char_rom_builtin")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "drive_rom_builtin")), !ghosted);
	gtk_widget_set_sensitive(GTK_WIDGET(gtk_builder_get_object(builder, "delete_rom_set")), !ghosted);
}

static void update_rom_set_editor_widgets(const std::string & name, const ROMPaths & p)
{
	gtk_entry_set_text(GTK_ENTRY(gtk_builder_get_object(builder, "rom_set_name")), name.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "basic_rom_path")), p.BasicROMPath.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "kernal_rom_path")), p.KernalROMPath.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "char_rom_path")), p.CharROMPath.c_str());
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive_rom_path")), p.DriveROMPath.c_str());
}

extern "C" G_MODULE_EXPORT void on_new_rom_set_clicked(GtkButton *button, gpointer user_data)
{
	GtkListBox * rom_set_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "rom_set_list"));
	GtkListBoxRow * row = add_rom_set_editor_item("unnamed set", ROMPaths{});
	gtk_widget_show_all(GTK_WIDGET(rom_set_list));
	gtk_list_box_select_row(rom_set_list, row);
	gtk_widget_grab_focus(GTK_WIDGET(gtk_builder_get_object(builder, "rom_set_name")));
}

extern "C" G_MODULE_EXPORT void on_delete_rom_set_clicked(GtkButton *button, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		delete paths_of_rom_set_row(row);
		gtk_widget_destroy(GTK_WIDGET(row));

		ghost_rom_set_editor_widgets(true);
		update_rom_set_editor_widgets("", ROMPaths{});
	}
}

extern "C" G_MODULE_EXPORT void on_rom_set_list_row_selected(GtkListBox *self, GtkListBoxRow *row, gpointer user_data)
{
	if (row) {
		ghost_rom_set_editor_widgets(false);
		update_rom_set_editor_widgets(name_of_rom_set_row(row), *paths_of_rom_set_row(row));
	} else {
		ghost_rom_set_editor_widgets(true);
		update_rom_set_editor_widgets("", ROMPaths{});
	}
}

extern "C" G_MODULE_EXPORT void on_rom_set_name_insert_text(GtkEditable *self, gchar *new_text, gint new_text_length, gint *position, gpointer user_data)
{
	// Prevent ';' characters in ROM set names
	if (strchr(new_text, ';') == nullptr) {
		g_signal_handlers_block_by_func(G_OBJECT(self), (void *) on_rom_set_name_insert_text, user_data);
		gtk_editable_insert_text(self, new_text, new_text_length, position);
		g_signal_handlers_unblock_by_func(G_OBJECT(self), (void *) on_rom_set_name_insert_text, user_data);
	}
	g_signal_stop_emission_by_name(self, "insert-text");
}

extern "C" G_MODULE_EXPORT void on_rom_set_name_changed(GtkEditable *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		const gchar * name = gtk_entry_get_text(GTK_ENTRY(self));
		gtk_label_set_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))), name);
		gtk_list_box_invalidate_sort(GTK_LIST_BOX(gtk_builder_get_object(builder, "rom_set_list")));
	}
}

extern "C" G_MODULE_EXPORT void on_basic_rom_file_set(GtkFileChooserButton *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
		if (path) {
			paths_of_rom_set_row(row)->BasicROMPath = path;
			g_free(path);
		} else {
			paths_of_rom_set_row(row)->BasicROMPath.clear();
		}
	}
}

extern "C" G_MODULE_EXPORT void on_basic_rom_builtin_clicked(GtkButton *self, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "basic_rom_path")), "");
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		paths_of_rom_set_row(row)->BasicROMPath.clear();
	}
}

extern "C" G_MODULE_EXPORT void on_kernal_rom_file_set(GtkFileChooserButton *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
		if (path) {
			paths_of_rom_set_row(row)->KernalROMPath = path;
			g_free(path);
		} else {
			paths_of_rom_set_row(row)->KernalROMPath.clear();
		}
	}
}

extern "C" G_MODULE_EXPORT void on_kernal_rom_builtin_clicked(GtkButton *self, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "kernal_rom_path")), "");
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		paths_of_rom_set_row(row)->KernalROMPath.clear();
	}
}

extern "C" G_MODULE_EXPORT void on_char_rom_file_set(GtkFileChooserButton *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
		if (path) {
			paths_of_rom_set_row(row)->CharROMPath = path;
			g_free(path);
		} else {
			paths_of_rom_set_row(row)->CharROMPath.clear();
		}
	}
}

extern "C" G_MODULE_EXPORT void on_char_rom_builtin_clicked(GtkButton *self, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "char_rom_path")), "");
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		paths_of_rom_set_row(row)->CharROMPath.clear();
	}
}

extern "C" G_MODULE_EXPORT void on_drive_rom_file_set(GtkFileChooserButton *self, gpointer user_data)
{
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(self));
		if (path) {
			paths_of_rom_set_row(row)->DriveROMPath = path;
			g_free(path);
		} else {
			paths_of_rom_set_row(row)->DriveROMPath.clear();
		}
	}
}

extern "C" G_MODULE_EXPORT void on_drive_rom_builtin_clicked(GtkButton *self, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive_rom_path")), "");
	if (GtkListBoxRow * row = selected_rom_set_row()) {
		paths_of_rom_set_row(row)->DriveROMPath.clear();
	}
}

extern "C" G_MODULE_EXPORT void on_edit_rom_sets_clicked(GtkButton *button, gpointer user_data)
{
	GtkDialog * editor = GTK_DIALOG(gtk_builder_get_object(builder, "rom_set_editor"));
	GtkListBox * rom_set_list = GTK_LIST_BOX(gtk_builder_get_object(builder, "rom_set_list"));

	// Keep ROM set list sorted by name
	gtk_list_box_set_sort_func(rom_set_list, rom_set_list_sort_func, nullptr, nullptr);

	std::string selected_rom_set = get_selected_rom_set();

	// Repopulate ROM set list
	GList * children = gtk_container_get_children(GTK_CONTAINER(rom_set_list));
	for (GList * it = children; it != nullptr; it = g_list_next(it)) {
		GtkListBoxRow * row = GTK_LIST_BOX_ROW(it->data);
		delete paths_of_rom_set_row(row);
		gtk_widget_destroy(GTK_WIDGET(row));
	}
	g_list_free(children);

	for (const auto & [name, paths] : prefs->ROMSetDefs) {
		GtkListBoxRow * row = add_rom_set_editor_item(name, paths);
		if (name.c_str() == selected_rom_set) {
			gtk_list_box_select_row(rom_set_list, row);
		}
	}
	gtk_widget_show_all(GTK_WIDGET(rom_set_list));

	// Run editor dialog
	gint res = gtk_dialog_run(editor);
	if (res == GTK_RESPONSE_ACCEPT) {

		// Read back ROM set list
		prefs->ROMSetDefs.clear();

		children = gtk_container_get_children(GTK_CONTAINER(rom_set_list));
		for (GList * it = children; it != nullptr; it = g_list_next(it)) {
			GtkListBoxRow * row = GTK_LIST_BOX_ROW(it->data);

			// Prevent empty names, enforce uniqueness
			auto name = name_of_rom_set_row(row);
			if (name.empty()) {
				name = "unnamed set";
			}

			auto base_name = name;
			unsigned index = 1;
			while (prefs->ROMSetDefs.count(name) > 0) {
				name = base_name + std::to_string(index);
				++index;
			}

			prefs->ROMSetDefs[name] = *paths_of_rom_set_row(row);
		}
		g_list_free(children);

		create_rom_set_menu(selected_rom_set);
	}

	gtk_widget_hide(GTK_WIDGET(editor));
}


/*
 *  Signal handlers
 */

extern "C" G_MODULE_EXPORT void on_ok_clicked(GtkButton *button, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(prefs_win));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "about_win")));
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "sam_win")));

	result = true;
	gtk_main_quit();
}

extern "C" G_MODULE_EXPORT void on_auto_start_clicked(GtkButton *button, gpointer user_data)
{
	// Eject cartridge and set auto-start flag
	on_cartridge_eject_clicked(button, user_data);
	prefs->AutoStart = true;

	on_ok_clicked(button, user_data);
}

extern "C" G_MODULE_EXPORT void on_quit_clicked(GtkButton *button, gpointer user_data)
{
	result = false;
	gtk_main_quit();
}

extern "C" G_MODULE_EXPORT void on_load_snapshot(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_window_set_title(GTK_WINDOW(snapshot_dialog), "Load Snapshot");
	gtk_file_chooser_set_action(GTK_FILE_CHOOSER(snapshot_dialog), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_button_set_label(GTK_BUTTON(snapshot_accept_button), "Load");

	bool load_ok = false;
	std::string message;

	char * filename = nullptr;

	gint res = gtk_dialog_run(GTK_DIALOG(snapshot_dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(snapshot_dialog));
		load_ok = TheC64->LoadSnapshot(filename, prefs, message);
		if (load_ok) {
			message = std::format("Snapshot file '{}' loaded.", filename);
		}
		SAM_GetState(TheC64);	// Get new state from snapshot
		set_values();			// Drive settings may have changed
	}

	gtk_widget_hide(snapshot_dialog);

	if (res == GTK_RESPONSE_ACCEPT) {
		GtkWidget * msg = gtk_message_dialog_new(prefs_win, GTK_DIALOG_MODAL,
			load_ok ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"%s", message.c_str()
		);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
	}

	if (filename) {
		g_free(filename);
	}
}

extern "C" G_MODULE_EXPORT void on_save_snapshot(GtkMenuItem *menuitem, gpointer user_data)
{
	gtk_window_set_title(GTK_WINDOW(snapshot_dialog), "Save Snapshot");
	gtk_file_chooser_set_action(GTK_FILE_CHOOSER(snapshot_dialog), GTK_FILE_CHOOSER_ACTION_SAVE);
	gtk_button_set_label(GTK_BUTTON(snapshot_accept_button), "Save");

	bool save_ok = false;
	std::string message;

	char * filename = nullptr;

	gint res = gtk_dialog_run(GTK_DIALOG(snapshot_dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(snapshot_dialog));
		save_ok = TheC64->SaveSnapshot(filename, message);
		if (save_ok) {
			message = std::format("Snapshot file '{}' saved.", filename);
		}
		SAM_GetState(TheC64);	// Saving a snapshot may advance the state a few cycles in Frodo SC
	}

	gtk_widget_hide(snapshot_dialog);

	if (res == GTK_RESPONSE_ACCEPT) {
		GtkWidget * msg = gtk_message_dialog_new(prefs_win, GTK_DIALOG_MODAL,
			save_ok ? GTK_MESSAGE_INFO : GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"%s", message.c_str()
		);
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
	}

	if (filename) {
		g_free(filename);
	}
}

extern "C" G_MODULE_EXPORT void on_create_disk_image(GtkMenuItem *menuitem, gpointer user_data)
{
	bool save_ok = false;
	char * filename = nullptr;

	gint res = gtk_dialog_run(GTK_DIALOG(create_disk_image_dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(create_disk_image_dialog));
		save_ok = CreateDiskImageFile(filename);
	}

	gtk_widget_hide(create_disk_image_dialog);

	GtkWidget * msg = nullptr;
	if (save_ok) {
		msg = gtk_message_dialog_new(prefs_win,
			GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			"Disk image file '%s' created.", filename
		);
	} else if (res == GTK_RESPONSE_ACCEPT) {
		msg = gtk_message_dialog_new(prefs_win,
			GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"Can't create disk image file '%s'.", filename
		);
	}
	if (msg) {
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
	}

	if (filename) {
		g_free(filename);
	}
}

extern "C" G_MODULE_EXPORT void on_create_tape_image(GtkMenuItem *menuitem, gpointer user_data)
{
	bool save_ok = false;
	char * filename = nullptr;

	gint res = gtk_dialog_run(GTK_DIALOG(create_tape_image_dialog));
	if (res == GTK_RESPONSE_ACCEPT) {
		filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(create_tape_image_dialog));
		save_ok = CreateTapeImageFile(filename);
	}

	gtk_widget_hide(create_tape_image_dialog);

	GtkWidget * msg = nullptr;
	if (save_ok) {
		msg = gtk_message_dialog_new(prefs_win,
			GTK_DIALOG_MODAL, GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
			"Tape image file '%s' created.", filename
		);
	} else if (res == GTK_RESPONSE_ACCEPT) {
		msg = gtk_message_dialog_new(prefs_win,
			GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
			"Can't create tape image file '%s'.", filename
		);
	}
	if (msg) {
		gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);
	}

	if (filename) {
		g_free(filename);
	}
}

extern "C" G_MODULE_EXPORT void on_shortcuts_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkBuilder * shortcuts_builder = gtk_builder_new_from_string(shortcuts_win_ui, -1);
	GtkWindow * shortcuts_win = GTK_WINDOW(gtk_builder_get_object(shortcuts_builder, "shortcuts_win"));
	gtk_window_set_transient_for(shortcuts_win, prefs_win);
	gtk_window_present(shortcuts_win);
	g_object_unref(shortcuts_builder);
}

extern "C" G_MODULE_EXPORT void on_user_manual_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	// Look in base dir first, then in HTMLDIR
	std::string index_file;
	if (! app_base_dir.empty()) {
		auto p = fs::path(app_base_dir) / "docs" / "index.html";
		if (fs::is_regular_file(p)) {
			index_file = p.string();
		}
	}
	if (index_file.empty()) {
		index_file = HTMLDIR "index.html";
	}

	gchar * uri = g_filename_to_uri(index_file.c_str(), nullptr, nullptr);
	if (uri != nullptr) {
		gtk_show_uri_on_window(prefs_win, uri, GDK_CURRENT_TIME, nullptr);
		g_free(uri);
	}
}

extern "C" G_MODULE_EXPORT void on_about_activate(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkAboutDialog * about_win = GTK_ABOUT_DIALOG(gtk_builder_get_object(builder, "about_win"));
	gtk_about_dialog_set_program_name(about_win, VERSION_STRING);
	gtk_window_present(GTK_WINDOW(about_win));
}

extern "C" G_MODULE_EXPORT void on_about_win_response(GtkDialog * self, gint response_id, gpointer user_data)
{
	gtk_widget_hide(GTK_WIDGET(gtk_builder_get_object(builder, "about_win")));
}

extern "C" G_MODULE_EXPORT void on_show_sam_monitor(GtkMenuItem *menuitem, gpointer user_data)
{
	GtkWindow * sam_win = GTK_WINDOW(gtk_builder_get_object(builder, "sam_win"));
	gtk_window_present(sam_win);

	// Place cursor at end
	GtkTextIter end;
	gtk_text_buffer_get_end_iter(sam_buffer, &end);
	gtk_text_buffer_place_cursor(sam_buffer, &end);
	gtk_text_view_scroll_mark_onscreen(sam_view, gtk_text_buffer_get_insert(sam_buffer));
}

extern "C" G_MODULE_EXPORT void on_emul1541_proc_toggled(GtkToggleButton *button, gpointer user_data)
{
	prefs->Emul1541Proc = gtk_toggle_button_get_active(button);
	ghost_widgets();
}

extern "C" G_MODULE_EXPORT void on_drive_path_file_set(GtkFileChooserButton *button, gpointer user_data)
{
	ghost_widgets();	// Update auto-start button

	gchar * path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(button));
	if (path == nullptr)
		return;

	int type;
	if (! IsMountableFile(path, type) || type == FILE_TAPE_IMAGE) {

		// Not a disk image or archive file, mount the parent directory instead
		gchar * dir = g_path_get_dirname(path);
		gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(button), dir);
		g_free(dir);
	}
	g_free(path);
}

extern "C" G_MODULE_EXPORT void on_tape_path_file_set(GtkFileChooserButton *button, gpointer user_data)
{
	ghost_widgets();	// Update auto-start button
}

extern "C" G_MODULE_EXPORT void on_drive8_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive8_path")), "");
	ghost_widgets();	// Update auto-start button
}

extern "C" G_MODULE_EXPORT void on_drive9_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive9_path")), "");
}

extern "C" G_MODULE_EXPORT void on_drive10_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive10_path")), "");
}

extern "C" G_MODULE_EXPORT void on_drive11_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive11_path")), "");
}

extern "C" G_MODULE_EXPORT void on_tape_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "tape_path")), "");
	ghost_widgets();	// Update auto-start button
}

extern "C" G_MODULE_EXPORT void on_drive8_next_disk_clicked(GtkButton *button, gpointer user_data)
{
	get_drive_path(0, "drive8_path");
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive8_path")), NextDiskImageFile(prefs->DrivePath[0]).c_str());
}

extern "C" G_MODULE_EXPORT void on_drive9_next_disk_clicked(GtkButton *button, gpointer user_data)
{
	get_drive_path(1, "drive9_path");
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive9_path")), NextDiskImageFile(prefs->DrivePath[1]).c_str());
}

extern "C" G_MODULE_EXPORT void on_drive10_next_disk_clicked(GtkButton *button, gpointer user_data)
{
	get_drive_path(2, "drive10_path");
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive10_path")), NextDiskImageFile(prefs->DrivePath[2]).c_str());
}

extern "C" G_MODULE_EXPORT void on_drive11_next_disk_clicked(GtkButton *button, gpointer user_data)
{
	get_drive_path(3, "drive11_path");
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "drive11_path")), NextDiskImageFile(prefs->DrivePath[3]).c_str());
}

extern "C" G_MODULE_EXPORT void on_tape_play_toggled(GtkToggleButton *button, gpointer user_data)
{
	TheC64->SetTapeButtons(TapeState::Play);
	set_tape_controls();
}

extern "C" G_MODULE_EXPORT void on_tape_record_toggled(GtkToggleButton *button, gpointer user_data)
{
	if (TheC64->TapePosition() < 100) {
		GtkWidget * msg = gtk_message_dialog_new(prefs_win, GTK_DIALOG_MODAL,
			GTK_MESSAGE_WARNING, GTK_BUTTONS_OK_CANCEL,
			"Recording at current tape position will overwrite existing data!\n"
			"OK to proceed?"
		);
		int result = gtk_dialog_run(GTK_DIALOG(msg));
		gtk_widget_destroy(msg);

		if (result != GTK_RESPONSE_OK) {
			set_tape_controls();
			return;
		}
	}

	TheC64->SetTapeButtons(TapeState::Record);
	set_tape_controls();
}

extern "C" G_MODULE_EXPORT void on_tape_stop_toggled(GtkToggleButton *button, gpointer user_data)
{
	TheC64->SetTapeButtons(TapeState::Stop);
	set_tape_controls();
}

extern "C" G_MODULE_EXPORT void on_tape_rewind_clicked(GtkButton *button, gpointer user_data)
{
	TheC64->RewindTape();
	set_tape_controls();
}

extern "C" G_MODULE_EXPORT void on_tape_forward_clicked(GtkButton *button, gpointer user_data)
{
	TheC64->ForwardTape();
	set_tape_controls();
}

extern "C" G_MODULE_EXPORT void on_display_type_changed(GtkComboBox *box, gpointer user_data)
{
	prefs->DisplayType = gtk_combo_box_get_active(box);
	ghost_widgets();
}

extern "C" G_MODULE_EXPORT void on_reu_type_changed(GtkComboBox *box, gpointer user_data)
{
	prefs->REUType = gtk_combo_box_get_active(box);
	ghost_widgets();
}

extern "C" G_MODULE_EXPORT void on_cartridge_eject_clicked(GtkButton *button, gpointer user_data)
{
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(gtk_builder_get_object(builder, "cartridge_path")), "");
}

extern "C" G_MODULE_EXPORT gboolean on_prefs_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	// Closing the prefs editor behaves like "Quit" on startup,
	// "Continue" otherwise
	if (in_startup) {
		on_quit_clicked(nullptr, user_data);
	} else {
		on_ok_clicked(nullptr, user_data);
	}

	// Prevent destruction
	return gtk_widget_hide_on_delete(widget);
}

extern "C" G_MODULE_EXPORT gboolean on_about_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	// Prevent destruction
	return gtk_widget_hide_on_delete(widget);
}

extern "C" G_MODULE_EXPORT gboolean on_sam_win_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
	// Prevent destruction
	return gtk_widget_hide_on_delete(widget);
}
