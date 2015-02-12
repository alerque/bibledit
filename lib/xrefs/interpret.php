<?php
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


require_once ("../bootstrap/bootstrap");


page_access_level (Filter_Roles::translator ());


$database_volatile = Database_Volatile::getInstance ();
$database_config_user = Database_Config_User::getInstance ();
$database_config_bible = Database_Config_Bible::getInstance ();
$database_books = Database_Books::getInstance ();


$bible = request->database_config_user()->getSourceXrefBible ();


// Save abbreviation / book pair.
if (isset (request->post ['save'])) {
  $abbreviation = request->post ['abbreviation'];
  $fullname = request->post ['fullname'];
  $abbreviations = Database_Config_Bible::getBookAbbreviations ($bible);
  $abbreviations = filter_abbreviations_display ($abbreviations);
  $abbreviations += "\n$fullname = $abbreviation";
  Database_Config_Bible::setBookAbbreviations ($bible, $abbreviations);
}


// Storage identifier, based on the user's name.
$identifier = filter_string_user_identifier ();


// Retrieve all notes from the database.
$allnotes = database_volatile.getValue ($identifier, "sourcexrefs");
$allnotes = unserialize ($allnotes);


// Retrieve all abbreviations, sort them, longest first.
// The replace routines replaces the longer strings first,
// to be sure that no partial book abbreviations are replaced.
$abbreviations = Database_Config_Bible::getBookAbbreviations ($bible);
$abbreviations = filter_abbreviations_read ($abbreviations);
$sorter = array ();
for ($abbreviations as $abbrev => $book) {
  $sorter [] = unicode_string_length ($abbrev);
}
array_multisort ($sorter, SORT_DESC, SORT_NUMERIC, $abbreviations);
$abbreviations = array_keys ($abbreviations);


$unknown_abbreviations = array ();


// Go through notes, do the replacement, collect unknown abbreviations.
for ($allnotes as $note) {
  $note = $note ['text'];
  $note = filter_string_str_replace ('\x*', "", $note);
  $note = filter_string_str_replace ('\x', "", $note);
  $note = filter_string_str_replace ($abbreviations, "", $note);
  $note = explode (" ", $note);
  for ($note as $fragment) {
    if (strlen ($fragment) <= 1) continue;
    if (intval ($fragment) > 0) continue;
    $unknown_abbreviations [] = $fragment;
  }
}


if (empty ($unknown_abbreviations)) {
  redirect_browser ("translate");
  die;
}


$header = new Assets_Header (translate("Cross references"));
$header->run ();


$view = new Assets_View (__FILE__);


$unknown_abbreviations = array_unique ($unknown_abbreviations);
$view.set_variable ("remaining = count ($unknown_abbreviations) - 1;


$view.set_variable ("abbreviation = $unknown_abbreviations [0];


$books = Database_Books::getIDs ();
for ($books as &$book) {
  $book = Database_Books::getEnglishFromId ($book);
}
$view.set_variable ("books = $books;


$view->render ("interpret");


Assets_Page::footer ();


?>
