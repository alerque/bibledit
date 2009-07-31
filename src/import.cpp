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


#include "libraries.h"
#include "import.h"
#include "utilities.h"
#include "clean.h"
#include "usfmtools.h"
#include "bible.h"
#include "books.h"
#include "gwrappers.h"
#include "directories.h"
#include "unixwrappers.h"
#include "tiny_utilities.h"
#include "dialogradiobutton.h"
#include "localizedbooks.h"
#include "categorize.h"
#include "projectutils.h"
#include "progresswindow.h"


ImportBookRead::ImportBookRead(const ustring & filename, const ustring & encoding)
{
  // Read the file.
  gchar *contents;
  gsize length;
  GError *error = NULL;
  if (!g_file_get_contents(filename.c_str(), &contents, &length, &error)) {
    cerr << error->message << endl;
    g_error_free(error);
    return;
  }
  // Handle files created with Windows Notepad.
  contents = de_windows_notepad(contents);

  // Convert the data.
  if (!encoding.empty()) {
    gchar *output;
    output = unicode_convert(contents, encoding);
    if (output) {
      g_free(contents);
      contents = output;
    }
  }
  // Check whether valid UTF8 data.
  if (!g_utf8_validate(contents, -1, NULL)) {
    cerr << "Data from " << filename << " is not valid UTF8." << endl;
    return;
  }
  // Divide text into separate lines.
  ParseLine parseline(contents);
  g_free(contents);
  rawlines = parseline.lines;
}



void ImportBookRead::onlinebible(map <ustring, unsigned int> bookmap)
{
  // If there's nothing to import, bail out.
  if (rawlines.empty())
    return;

  try {

    // Get the name of the book.
    unsigned int book, chapter, verse;
    online_bible_parse_reference (rawlines[0], book, chapter, verse, bookmap);
    if (book)
      bookname = books_id_to_english(book);
    else
      return;

    // Store USFM id.
    ustring usfmid = books_id_to_paratext(book);
    lines.push_back("\\id " + usfmid);

    // Convert the Online Bible lines to USFM code.
    unsigned int previouschapter = 0;
    unsigned int previousverse = 0;
    bool verse_was_added = false;
    for (unsigned int i = 0; i < rawlines.size(); i++) {
      if (online_bible_parse_reference (rawlines[i], book, chapter, verse, bookmap)) {
        if (chapter != previouschapter) {
          lines.push_back("\\c " + convert_to_string (chapter));
          previouschapter = chapter;
          previousverse = 0;
        }
        if (verse != previousverse) {
          lines.push_back("\\p");
          lines.push_back("\\v " + convert_to_string (verse) + " ");
          previousverse = verse;
          verse_was_added = true;
        }
      } else {
        ustring line;
        if (!verse_was_added)
          line.append ("\\p ");
        line.append (rawlines[i]);
        lines.push_back(line);
        verse_was_added = false;
      }
    }
  }
  catch(exception & ex) {
    gw_critical(ex.what());
  }
}


gchar *unicode_convert(gchar * data, const ustring & encoding)
{
  gchar *output;
  gssize length = strlen(data);
  GError *error = NULL;
  output = g_convert(data, length, "UTF-8", encoding.c_str(), NULL, NULL, &error);
  if (output == NULL) {
    cerr << error->message << endl;
    g_error_free(error);
  }
  return output;
}


ustring bibleworks_file_get_bookname(const ustring & filename)
// Retrieves the bookname from a file exported by BibleWorks. Though there can
// be several books in such a file, this functions retrieves only the name
// of the first book.
{
#define MYMAX 10
  ReadText rt(filename, true, false);
  unsigned int maximum = MYMAX;
  maximum = CLAMP(maximum, 0, rt.lines.size());
  unsigned int goodline = 0;
  ustring bookname;
  try {
    for (unsigned int i = 0; i < maximum; i++) {
      ustring bookabbreviation = rt.lines[i].substr(0, 3);
      unsigned int id = books_bibleworks_to_id(bookabbreviation);
      if (id) {
        goodline++;
        bookname = books_id_to_english(id);
      }
    }
  }
  catch(exception & ex) {
    gw_critical(ex.what());
  }
  if (goodline != MYMAX)
    bookname.clear();
  return bookname;
#undef MYMAX
}


bool online_bible_file (const ustring& filename)
// This function returns true if the "filename" looks like a file exported from the Online Bible.
{
  // Consider the first 10 lines on the most, because that is where the signature can be found.
  ReadText rt (filename, true, false);
  unsigned int maxline = MIN (10, rt.lines.size());

  /*  
  
  A file exported from the Online Bible, for example the commentary of Matthew Poole, should
  have this signature:
  
  Genesis 1:1 (Poole)

  In words, one or more lines are found in the text file that satisfy the following conditions:
  - It starts with the name of a book.
  - A chapter follows, then a colon, then a verse.
  - An opening bracket follows, then text, then a closing bracket at the end of the line.
  
  */

  map <unsigned int, ustring> booknames;
  map <ustring, unsigned int> bookmap = general_adapted_booknames_fill_up (booknames);
  for (unsigned int i = 0; i < maxline; i++) {
    unsigned int dummy;
    if (online_bible_parse_reference (rt.lines[i], dummy, dummy, dummy, bookmap)) {
      // Online Bible file found.
      return true;
    }
  }
  
  // No Online Bible file found.
  return false;
}


bool online_bible_parse_reference (ustring line, unsigned int& book, unsigned int& chapter, unsigned int& verse, map <ustring, unsigned int>& bookmap)
// Parses a line of text exported from the Online Bible to see if a reference can be discovered in it.
{
  // Line should contain text.
  if (!line.empty()) {
    // Last character should be a closing bracket.
    if (line.find_last_of (")") == (line.length() -1)) {
      line.erase (line.length() - 1, 1);
      // There should be a space, an opening bracket, and some text following.
      size_t pos = line.find (" (");
      if (pos != string::npos) {
        if (pos < line.length() - 3) {
          line.erase (pos, 1000);
          // There should be a number after the colon, and this is going to be the verse number.
          pos = line.find_last_of (":");
          if (pos != string::npos) {
            ustring s = line.substr (pos + 1, 4);
            line.erase (pos, 5);
            if (number_in_string (s) == s) {
              verse = convert_to_int (s);
              // There should be a number after the last space, and this is going to be the chapter number.
              pos = line.find_last_of (" ");
              if (pos != string::npos) {
                ustring s = line.substr (pos + 1, 4);
                line.erase (pos, 5);
                if (number_in_string (s) == s) {
                  chapter = convert_to_int (s);
                  // The remainder of the line should be a book name.
                  book = bookmap [line];
                  if (book) {
                    return true;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
  // No reference found.
  return false;  
}


void online_bible_save_book_internal(const ustring& directory, const ustring& inputfile, unsigned int book, vector <ustring>& lines, vector <ustring>& files)
// This saves ""lines" it in "directory", 
// giving it a name, and then adds the name to "files".
{
  if (lines.empty())
    return;
  if (!book)
    return;
  ustring bookname = books_id_to_english(book);
  ustring filename = gw_build_filename(directory, gw_path_get_basename(inputfile + "_" + bookname));
  write_lines(filename, lines);
  lines.clear();
  files.push_back(filename);
}


vector <ustring> online_bible_file_divide (const ustring& inputfile, map <ustring, unsigned int> bookmap)
// This function takes one textfile exported from the Online Bible,
// and divides it into several files, each containing only one book.
// It puts them all in the temporal directory, 
// and returns the list of filenames so produced.
{
  // Clean temporal directory.
  ustring directory = gw_build_filename(directories_get_temp(), "olb-import");
  unix_rmdir(directory);
  gw_mkdir_with_parents(directory);

  // Storage for the divided files.
  vector < ustring > divided_files;

  // Read the inputfile and go though it.
  ReadText rt(inputfile, true, false);

  unsigned int previousbook = 0;
  vector < ustring > booklines;
  for (unsigned int i = 0; i < rt.lines.size(); i++) {
    unsigned int book, chapter, verse = 0;
    if (online_bible_parse_reference (rt.lines[i], book, chapter, verse, bookmap)) {
      if (book != previousbook) {
        online_bible_save_book_internal(directory, inputfile, previousbook, booklines, divided_files);
        booklines.clear();
        previousbook = book;
      }
    }
    booklines.push_back(rt.lines[i]);
  }
  online_bible_save_book_internal(directory, inputfile, previousbook, booklines, divided_files);

  // Return result.
  return divided_files;
}


void import_check_usfm_files (vector <ustring>& filenames, vector <unsigned int>& bookids, const ustring& bible, vector <ustring>& messages)
{
  // Check whether all the USFM files have proper \id data.
  if (messages.empty()) {
    vector <ustring> files_names_temp;
    for (unsigned int i = 0; i < filenames.size(); i++) {
      unsigned int book_id = books_paratext_to_id(get_usfm_id_from_file(filenames[i]));
      if (book_id) {
        files_names_temp.push_back (filenames[i]);
        bookids.push_back (book_id);
      } else {
        messages.push_back ("Unknown book in file " + filenames[i]);
      }
    }
    filenames = files_names_temp;
  }
  // Check whether none of the books to be imported is already in the project.
  if (messages.empty()) {
    vector <unsigned int> books_in_project = project_get_books (bible);
    set <unsigned int> books_in_project_set (books_in_project.begin(), books_in_project.end());
    for (unsigned int i = 0; i < bookids.size(); i++) {
      if (books_in_project_set.find (bookids[i]) != books_in_project_set.end()) {
        messages.push_back ("File " + filenames[i] + " has book " + books_id_to_english (bookids[i]) + ", but this one is already in the project");
      }
    }
  }
}


void import_usfm_file (const ustring& file, unsigned int book, const ustring& project, vector <ustring>& messages)
{
  // Read the file.
  gchar *contents;
  gsize length;
  GError *error = NULL;
  if (!g_file_get_contents(file.c_str(), &contents, &length, &error)) {
    messages.push_back (error->message);
    g_error_free(error);
    return;
  }

  // Handle files created with Windows Notepad.
  contents = de_windows_notepad(contents);

  // Divide text into separate lines.
  ParseLine parseline(contents);
  g_free(contents);

  // Clean up according to the USFM standard.
  CleanUsfm cleanusfm(parseline.lines);
  parseline.lines.assign(cleanusfm.lines.begin(), cleanusfm.lines.end());

  // Categorize the lines.
  CategorizeChapterVerse ccv(parseline.lines);

  // Store in project.
  project_store_book(project, book, ccv);
}


void import_check_bibleworks_file (vector <ustring>& filenames, vector <unsigned int>& bookids, const ustring& bible, vector <ustring>& messages)
// Checks the file exported from BibleWorks and meant to be imported as a Bible.
{
  // Check whether there's only one file selected.
  if (messages.empty()) {
    if (filenames.size() > 1) {
      messages.push_back ("You have selected more than one file");
    }
  }

  // Check that the BibleWorks file is a valid one.
  if (messages.empty()) {
    ustring english_name = bibleworks_file_get_bookname(filenames[0]);
    unsigned int id = books_english_to_id (english_name);
    if (id == 0) {
      messages.push_back ("The file cannot be recognized as coming from BibleWorks");
    }
  }

  // Check that the Bible to import the file into is empty.
  if (messages.empty()) {
    vector <unsigned int> books = project_get_books (bible);
    if (!books.empty()) {
      messages.push_back ("You try to import data into a Bible that is not empty");
    }
  }
}


void import_bibleworks_file (const ustring& file, const ustring& bible, vector <ustring>& messages)
// Imports a bibleworks file.
{
  // Read the file.
  ReadText rt (file, true, false);

  // If there's nothing to import, bail out.
  if (rt.lines.empty()) {
    messages.push_back ("The file is empty");
  }

  // Divide the input into separate bits for each book.
  vector <VectorUstring> bookdata;
  if (messages.empty()) {
    try {
      ustring previousbook;
      vector < ustring > booklines;
      for (unsigned int i = 0; i < rt.lines.size(); i++) {
        ustring currentbook = rt.lines[i].substr(0, 3);
        if (i == 0) {
          previousbook = currentbook;
        }
        if (currentbook != previousbook) {
          bookdata.push_back (booklines);
          booklines.clear();
          previousbook = currentbook;
        }
        booklines.push_back(rt.lines[i]);
      }
      bookdata.push_back (booklines);
    }
    catch(exception & ex) {
      messages.push_back(ex.what());
    }
  }

  // Import each book.
  if (messages.empty ()) {
    ProgressWindow progresswindow ("Importing", false);
    progresswindow.set_iterate (0, 1, bookdata.size());
    for (unsigned int i = 0; i < bookdata.size(); i++) {
      progresswindow.iterate ();
      try {

        // Input and output data.
        vector <ustring> rawlines = bookdata[i];
        vector <ustring> usfmlines;

        // Get the name of the book.
        unsigned int book_id = books_bibleworks_to_id(rawlines[0].substr(0, 3));
        if (book_id == 0) {
          messages.push_back ("Unknown book: " + rawlines[0]);
          return;
        }

        // Store USFM id.
        ustring usfmid = books_id_to_paratext (book_id);
        usfmlines.push_back("\\id " + usfmid);

        // Convert the BibleWorks lines to USFM code.
        ustring previouschapter = "0";
        for (unsigned int i = 0; i < rawlines.size(); i++) {
          // Convert chapter information.
          ustring line = rawlines[i];
          line.erase(0, 4);
          ustring currentchapter = number_in_string(line);
          line.erase(0, currentchapter.length() + 1);
          if (currentchapter != previouschapter) {
            usfmlines.push_back("\\c " + currentchapter);
            usfmlines.push_back("\\p");
            previouschapter = currentchapter;
          }
          // Convert verse data.
          usfmlines.push_back("\\v " + line);
        }

        // Store into the Bible.
        CategorizeChapterVerse ccv(usfmlines);
        project_store_book(bible, book_id, ccv);
        
      }
      catch(exception & ex) {
        messages.push_back(ex.what());
      }
    }
  }

}

