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


#include <database/imageresources.h>
#include <filter/url.h>
#include <filter/string.h>
#include <database/sqlite.h>


// Database resilience: 
// The data is stored as images and files in the filesystem.
// That should be resilient enough.


string Database_ImageResources::mainFolder ()
{
  return filter_url_create_root_path ("databases", "imageresources");
}


string Database_ImageResources::resourceFolder (const string& name)
{
  return filter_url_create_path (mainFolder (), name);
}


string Database_ImageResources::imagePath (string name, string image)
{
  return filter_url_create_path (resourceFolder (name), image);
}


string Database_ImageResources::databaseFile ()
{
  return "passages.sqlite";
}


sqlite3 * Database_ImageResources::connect (string name)
{
  string path = filter_url_create_path (resourceFolder (name), databaseFile ());
  return database_sqlite_connect (path);
}


vector <string> Database_ImageResources::names ()
{
  return filter_url_scandir (mainFolder ());
}


void Database_ImageResources::create (string name)
{
  // Create folder to store the images.
  string path = resourceFolder (name);
  filter_url_unlink (path);
  filter_url_mkdir (path);

  // Create the passages database.
  sqlite3 * db = connect (name);
  string sql =
  "CREATE TABLE IF NOT EXISTS passages ("
  " start integer,"
  " end integer,"
  " image text"
  ");";
  database_sqlite_exec (db, sql);
  database_sqlite_disconnect (db);
}


void Database_ImageResources::erase (string name)
{
  string path = resourceFolder (name);
  // If a folder: Delete it.
  filter_url_rmdir (path);
  // If a file: Delete it.
  filter_url_unlink (path);
}


void Database_ImageResources::erase (string name, string image) // Todo test
{
  filter_url_unlink (imagePath (name, image));
}


// Moves $file (path to an image file) into the database.
void Database_ImageResources::store (string name, string file) // Todo test
{
  string folder = resourceFolder (name);
  string basename = filter_url_basename (file);
  string path;
  bool exists = false;
  do {
    path = filter_url_create_path (folder, basename);
    exists = file_exists (path);
    if (exists) basename = filter_string_str_replace (".", "0.", basename);
  } while (exists);
  filter_url_rename (file, path);
}


// Assign a passage range to the $image.
// It means that this image contains text for the passage range.
void Database_ImageResources::assign (string name, string image,
                                      int book1, int chapter1, int verse1,
                                      int book2, int chapter2, int verse2) // Todo test.
{
  sqlite3 * db = connect (name);
  {
    SqliteSQL sql = SqliteSQL ();
    sql.add ("DELETE FROM passages WHERE image =");
    sql.add (image);
    sql.add (";");
    database_sqlite_exec (db, sql.sql);
  }
  {
    SqliteSQL sql = SqliteSQL ();
    sql.add ("INSERT INTO passages VALUES (");
    sql.add (filter_passage_to_integer (Passage ("", book1, chapter1, convert_to_string (verse1))));
    sql.add (",");
    sql.add (filter_passage_to_integer (Passage ("", book2, chapter2, convert_to_string (verse2))));
    sql.add (",");
    sql.add (image);
    sql.add (");");
    database_sqlite_exec (db, sql.sql);
  }
  database_sqlite_disconnect (db);
}


vector <string> Database_ImageResources::get (string name, int book, int chapter, int verse) // Todo write and test
{
  int passage = filter_passage_to_integer (Passage ("", book, chapter, convert_to_string (verse)));
  SqliteSQL sql = SqliteSQL ();
  sql.add ("SELECT image FROM passages WHERE start <=");
  sql.add (passage);
  sql.add ("AND end >=");
  sql.add (passage);
  sql.add (";");
  sqlite3 * db = connect (name);
  vector <string> images = database_sqlite_query (db, sql.sql) ["image"];
  database_sqlite_disconnect (db);
  return images;
}


vector <string> Database_ImageResources::get (string name) // Todo test
{
  vector <string> images = filter_url_scandir (resourceFolder (name));
  images = filter_string_array_diff (images, {databaseFile()});
  return images;
}
