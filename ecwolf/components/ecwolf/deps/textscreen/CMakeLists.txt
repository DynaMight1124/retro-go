cmake_minimum_required( VERSION 3.5.0 )

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

add_library( textscreen STATIC
	txt_radiobutton.c
	txt_window_action.c
	txt_io.c
	txt_strut.c
	txt_inputbox.c
	txt_table.c
	txt_dropdown.c
	txt_desktop.c
	txt_widget.c
	txt_window.c
	txt_scrollpane.c
	txt_spinctrl.c
	txt_separator.c
	txt_button.c
	txt_utf8.c
	txt_sdl.c
	txt_checkbox.c
	txt_fileselect.c
	txt_gui.c
	txt_label.c
)
target_link_libraries( textscreen PUBLIC SDL::SDL )
target_include_directories( textscreen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )

if( CMAKE_COMPILER_IS_GNUC )
	target_compile_options( textscreen PRIVATE -Wall -fomit-frame-pointer )
endif( CMAKE_COMPILER_IS_GNUC )
