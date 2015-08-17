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


#ifndef INCLUDED_LEXICON_LOGIC_H
#define INCLUDED_LEXICON_LOGIC_H


#include <config/libraries.h>


string lexicon_logic_strong_number_cleanup (string strong);
string lexicon_logic_strong_hover_text (string strong);
string lexicon_logic_create_xml_document (string xml);
string lexicon_logic_render_definition (string strong);
string lexicon_logic_render_part_of_speech (string value);
string lexicon_logic_render_part_of_speech_stem (string abbrev);
string lexicon_logic_render_part_of_speech_person (string abbrev);
string lexicon_logic_render_part_of_speech_gender (string abbrev);
string lexicon_logic_render_part_of_speech_number (string abbrev);
string lexicon_logic_render_part_of_speech_state (string abbrev);


#endif