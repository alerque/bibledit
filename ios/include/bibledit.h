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

  const char * bibledit_version_number ();
  const char * bibledit_network_port ();
  void bibledit_set_web_root (const char * directory);
  void bibledit_start_server ();
  bool bibledit_is_running ();
  void bibledit_stop_server ();

#ifdef __cplusplus
}
#endif