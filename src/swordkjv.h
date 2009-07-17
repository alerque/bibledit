/*
** Copyright (©) 2003-2009 Teus Benschop.
**  
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 3 of the License, or
** (at your option) any later version.
**  
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**  
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
**  
*/


#ifndef INCLUDED_SWORD_KJV_H
#define INCLUDED_SWORD_KJV_H


#include "libraries.h"
#include "htmlwriter2.h"


ustring sword_kjv_html_entry_url ();
ustring sword_kjv_action_url ();
void import_sword_kjv_home_entry (HtmlWriter2& htmlwriter);
void import_sword_kjv_detailed_page (HtmlWriter2& htmlwriter);
vector <ustring> import_sword_kjv_action ();
void import_sword_kjv_result_page (const vector <ustring>& messages, HtmlWriter2& htmlwriter);


#endif
