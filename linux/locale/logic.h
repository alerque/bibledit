/*
Copyright (©) 2003-2016 Teus Benschop.

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


#ifndef INCLUDED_LOCALE_LOGIC_H
#define INCLUDED_LOCALE_LOGIC_H


#include <config/libraries.h>


string locale_logic_filter_default_language (string localization);
string locale_logic_month (int month);
map <string, string> locale_logic_localizations ();
map <string, string> locale_logic_read_po (string file);

string locale_logic_text_saving ();
string locale_logic_text_saved ();
string locale_logic_text_loaded ();
string locale_logic_text_retrying ();
string locale_logic_text_reformat ();
string locale_logic_text_no_privileges_modify_book ();


#endif
