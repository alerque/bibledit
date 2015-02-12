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


$database_config_user = Database_Config_User::getInstance ();
$database_config_bible = Database_Config_Bible::getInstance ();
$database_books = Database_Books::getInstance ();
$database_volatile = Database_Volatile::getInstance ();
$database_bibles = Database_Bibles::getInstance ();
$ipc_focus = Ipc_Focus::getInstance ();


$sourceBible = request->database_config_user()->getSourceXrefBible ();
$targetBible = request->database_config_user()->getTargetXrefBible ();
$book = Ipc_Focus::getBook ();
$chapter = Ipc_Focus::getChapter ();


// Retrieve all abbreviations for the source Bible, sort them, longest first.
// The replace routines replaces the longer strings first,
// to be sure that no partial book abbreviations are replaced.
$sourceAbbreviations = Database_Config_Bible::getBookAbbreviations ($sourceBible);
$sourceAbbreviations = filter_abbreviations_read ($sourceAbbreviations);
$sorter = array ();
for ($sourceAbbreviations as $abbrev => $dummy) {
  $sorter [] = unicode_string_length ($abbrev);
}
array_multisort ($sorter, SORT_DESC, SORT_NUMERIC, $sourceAbbreviations);


$targetAbbreviations = Database_Config_Bible::getBookAbbreviations ($targetBible);
$targetAbbreviations = filter_abbreviations_read ($targetAbbreviations);


// Create array with book abbreviations to find, and one with their matching replacements.
$find = array ();
$replace = array ();
for ($sourceAbbreviations as $sourceAbbreviation => $bk) {
  $find [] = $sourceAbbreviation;
  $key = array_search ($bk, $targetAbbreviations);
  if ($key === false) {
    $replace [] = $sourceAbbreviation;
  } else {
    $replace [] = $key;
  }
}


// Storage identifier, based on the user's name.
$identifier = filter_string_user_identifier ();


// Retrieve all notes from the database.
$allxrefs = database_volatile.getValue ($identifier, "sourcexrefs");
$allxrefs = unserialize ($allxrefs);


// Replace the abbreviations in the cross references.
for ($allxrefs as $key => $xref) {
  $allxrefs [$key] ['text'] = filter_string_str_replace ($find, $replace, $allxrefs [$key] ['text']);
}


// Get the target USFM into an array of verse => USFM fragment.
$usfmArray = array ();
$usfmString = request->database_bibles()->getChapter ($targetBible, $book, $chapter);
$verses = usfm_get_verse_numbers ($usfmString);
$verses = array_unique ($verses);
for ($verses as $verse) {
  $usfmArray [$verse] = usfm_get_verse_text ($usfmString, $verse);
}


// Go through each verse, through each note within that verse,
// look at source location, define target location, and insert the xref.
for ($verses as $verse) {
  
  
  // Gather array of cross references for this verse, if any.
  $xrefs = array ();
  reset ($allxrefs);
  for ($allxrefs as $xref) {
    if ($xref ['verse'] == $verse) {
      $xrefs [] = array ($xref ['offset'], $xref ['text']);
    }
  }
  if (empty ($xrefs)) continue;


  // Get the USFM for the current verse in the target Bible, if any.
  if (!isset ($usfmArray [$verse])) continue;
  $usfm = $usfmArray [$verse];
  
  
  // Get the length of the text of the verse in the source Bible without the xrefs.
  // Get the ratio for positioning the xrefs by comparing the lengths of source and target verse text.
  $sourceUsfm = request->database_bibles()->getChapter ($sourceBible, $book, $chapter);
  $sourceUsfm = usfm_get_verse_text ($sourceUsfm, $verse);
  $sourceUsfm = Filter_Usfm::removeNotes ($sourceUsfm, array ("x"));
  $sourceLength = unicode_string_length ($sourceUsfm);
  $targetLength = unicode_string_length ($usfm);
  $ratio = $targetLength / $sourceLength;
  
  
  // Insert the notes.
  $usfm = Filter_Usfm::insertNotes ($usfm, $xrefs, $ratio);
  $usfmArray [$verse] = $usfm;
 

}


$usfm = implode ("\n", $usfmArray);
Bible_Logic::storeChapter ($targetBible, $book, $chapter, $usfm);


$header = new Assets_Header (translate("Cross references"));
$header->run ();


$view = new Assets_View (__FILE__);


$view->render ("insert");


Assets_Page::footer ();


?>
