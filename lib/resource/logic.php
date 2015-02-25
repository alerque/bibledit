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
Todo goes out.



  // Get the resource for the verse in plain text format.
  public static function getText ($resource, book, chapter, verse)
  {
    $database_bibles = Database_Bibles::getInstance ();
    $database_resources = Database_Resources::getInstance ();
    $database_usfmresources = Database_UsfmResources::getInstance ();
    $database_offlineresources = Database_OfflineResources::getInstance ();

    $bibles = request->database_bibles()->getBibles ();
    $usfms = $database_usfmresources->getResources ();
    $externals = $database_resources->getNames ();

    $isBible = in_array ($resource, $bibles);
    $isUsfm = in_array ($resource, $usfms);
    if ($isBible || $isUsfm) {
      if ($isBible) $chapter_usfm = request->database_bibles()->getChapter ($resource, $book, $chapter);
      if ($isUsfm) $chapter_usfm = $database_usfmresources->getUsfm ($resource, $book, $chapter);
      $verse_usfm = usfm_get_verse_text ($chapter_usfm, $verse);
      $database_config_user = Database_Config_User::getInstance ();
      $stylesheet = request->database_config_user()->getStylesheet ();
      $filter_text = new Filter_Text ($resource);
      $filter_text->text_text = new Text_Text ();
      $filter_text->addUsfmCode ($verse_usfm);
      $filter_text->run ($stylesheet);
      $text = $filter_text->text_text->get ();
    } else if (in_array ($resource, $externals)) {
      // Use offline copy if it exists, else fetch it online.
      if ($database_offlineresources->exists ($resource, book, chapter, verse)) {
        $text = $database_offlineresources->get ($resource, book, chapter, verse);
      } else {
        $text = Resource_Logic::getExternal ($resource, book, chapter, verse, true);
      }
      $text = filter_string_html2text ($text);
    } else {
      $text = "";
    }

    return $text;
  }


}


?>
