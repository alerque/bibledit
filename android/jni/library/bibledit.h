/*
Copyright (©) 2003-2015 Teus Benschop.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifdef __cplusplus
extern "C" {
#endif

  const char * bibledit_get_version_number ();
  const char * bibledit_get_splash_screen ();
  const char * bibledit_get_network_port ();
  void bibledit_initialize_library (const char * package, const char * webroot);
  void bibledit_set_touch_enabled (bool enabled);
  void bibledit_set_quit_at_midnight ();
  void bibledit_set_timezone_hours_offset_utc (int hours);
  void bibledit_start_library ();
  bool bibledit_is_running ();
  void bibledit_stop_library ();
  void bibledit_shutdown_library ();
  void bibledit_log (const char * message);
  bool bibledit_open_browser ();

#ifdef __cplusplus
}
#endif
