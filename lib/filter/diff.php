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


class Filter_Diff
{


  /**
  * This filter produces files in USFM, html and text format.
  * The text files are to be used for showing the differences between them.
  * The files contain all verses that differ.
  * $bible: The Bible to go through.
  * $directory: The existing directory where to put the files.
  * Two files are created: verses_old.usfm and verses_new.usfm.
  * The book chapter.verse precede each verse.
  */
  public static function produceVerseLevel ($bible, $directory)
  {
    $database_bibles = Database_Bibles::getInstance ();
    $database_modifications = Database_Modifications::getInstance ();
    $database_books = Database_Books::getInstance ();
    $database_config_bible = Database_Config_Bible::getInstance ();
    $stylesheet = $database_config_bible->getExportStylesheet ($bible);

    $old_vs_usfm = array ();
    $new_vs_usfm = array ();

    $filter_text_old = new Filter_Text ($bible);
    $filter_text_old->html_text_standard = new Html_Text (gettext("Bible"));
    $filter_text_old->text_text = new Text_Text ();
    $filter_text_new = new Filter_Text ($bible);
    $filter_text_new->html_text_standard = new Html_Text (gettext("Bible"));
    $filter_text_new->text_text = new Text_Text ();

    $books = $database_modifications->getTeamDiffBooks ($bible);
    for ($books as $book) {
      $bookname = $database_books->getEnglishFromId ($book);
      $chapters = $database_modifications->getTeamDiffChapters ($bible, $book);
      for ($chapters as $chapter) {
        // Go through the combined verse numbers in the old and new chapter.
        $old_chapter_usfm = $database_modifications->getTeamDiff ($bible, $book, $chapter);
        $new_chapter_usfm = $database_bibles->getChapter ($bible, $book, $chapter);
        $old_verse_numbers = usfm_get_verse_numbers ($old_chapter_usfm);
        $new_verse_numbers = usfm_get_verse_numbers ($new_chapter_usfm);
        $verses = array_merge ($old_verse_numbers, $new_verse_numbers);
        $verses = array_unique ($verses);
        sort ($verses, SORT_NUMERIC);
        for ($verses as $verse) {
          $old_verse_text = usfm_get_verse_text ($old_chapter_usfm, $verse);
          $new_verse_text = usfm_get_verse_text ($new_chapter_usfm, $verse);
          if ($old_verse_text != $new_verse_text) {
            $usfmCode = "\\p $bookname $chapter.$verse $old_verse_text";
            $old_vs_usfm [] = $usfmCode;
            $filter_text_old->addUsfmCode ($usfmCode);
            $usfmCode = "\\p $bookname $chapter.$verse $new_verse_text";
            $new_vs_usfm [] = $usfmCode;
            $filter_text_new->addUsfmCode ($usfmCode);
          }
        }
      }
    }

   filter_url_file_put_contents ("$directory/verses_old.usfm", implode ("\n", $old_vs_usfm));
   filter_url_file_put_contents ("$directory/verses_new.usfm", implode ("\n", $new_vs_usfm));
    $filter_text_old->run ($stylesheet);
    $filter_text_new->run ($stylesheet);
    $filter_text_old->html_text_standard->save ("$directory/verses_old.html");
    $filter_text_new->html_text_standard->save ("$directory/verses_new.html");
    $filter_text_old->text_text->save ("$directory/verses_old.txt");
    $filter_text_new->text_text->save ("$directory/verses_new.txt");
  }


  /**
  * This filter runs a diff.
  * $oldfile: The name of the old file for input.
  * $newfile: The name of the new file for input.
  * $outputfile: The name of the output file
  */
  public static function runDiffFile ($oldfile, $newfile, $outputfile)
  {
    $oldstring = filter_url_file_get_contents ($oldfile);
    $newstring = filter_url_file_get_contents ($newfile);

    $differences = self::diff ($oldstring, $newstring);

    $differences = explode ("\n", $differences);
    for ($differences as &$line) {
      $line = "<p>" . $line . "</p>";
    }
    $differences = implode ("\n", $differences);

   filter_url_file_put_contents ($outputfile, $differences);
  }


}

?>
