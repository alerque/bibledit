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


#ifndef INCLUDED_LEXICONS_H
#define INCLUDED_LEXICONS_H


#include "libraries.h"
#include "reference.h"
#include <glib.h>
#include <sqlite3.h>


void lexicons_import (GKeyFile *keyfile);
void lexicons_import_strongs_greek ();
void lexicons_get_strongs_data (const Reference& reference, vector <unsigned int>& strongs, vector <ustring>& phrases, bool include_unmarked);
vector <Reference> lexicons_get_strongs_verses (const Reference& reference, unsigned int strongs);
ustring lexicons_get_verse (const Reference& reference);


#endif
