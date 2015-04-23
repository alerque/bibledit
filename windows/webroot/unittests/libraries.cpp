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


#include <unittests/libraries.h>
#include <unittests/utilities.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/md5.h>
#include <filter/usfm.h>
#include <flate/flate.h>
#include <config/globals.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/sqlite.h>
#include <database/users.h>
#include <database/books.h>
#include <database/versifications.h>
#include <checksum/logic.h>
#include <editor/export.h>
#include <editor/import.h>
#include <workbench/logic.h>
#include <config/logic.h>
#include <client/logic.h>
#include <styles/logic.h>
#include <checks/sentences.h>
#include <checks/versification.h>
#include <checks/usfm.h>
#include <checks/verses.h>
#include <manage/hyphenate.h>


void test_sqlite ()
{
  // Tests for SQLite.
  sqlite3 * db = database_sqlite_connect ("sqlite");
  if (!db) error_message (__LINE__, __func__, "pointer", "NULL");
  database_sqlite_exec (db, "CREATE TABLE test (column1 integer, column2 integer, column3 integer);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (123, 456, 789);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (234, 567, 890);");
  database_sqlite_exec (db, "INSERT INTO test VALUES (345, 678, 901);");
  map <string, vector <string> > actual = database_sqlite_query (db, "SELECT column1, column2, column3 FROM test;");
  evaluate (__LINE__, __func__, "567", actual ["column2"] [1]);
  database_sqlite_disconnect (db);
  database_sqlite_disconnect (NULL);

  evaluate (__LINE__, __func__, true, database_sqlite_healthy ("sqlite"));
  unlink (database_sqlite_file ("sqlite").c_str());
  evaluate (__LINE__, __func__, false, database_sqlite_healthy ("sqlite"));

  evaluate (__LINE__, __func__, "He''s", database_sqlite_no_sql_injection ("He's"));
}


void test_session_logic ()
{
  // Test for class Session_Logic.
  {
    refresh_sandbox (true);
    
    // The session logic depends on users in the database.
    Database_Users database_users = Database_Users ();
    database_users.create ();
    
    // In an demo installation, a client is always logged in as user admin, even after logging out.
    config_globals_open_installation = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, session_admin_credentials (), request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    request.session_logic ()->logout ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, session_admin_credentials (), request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    
    // Test function to set the username.
    string username = "ঃইঝম";
    request.session_logic ()->setUsername (username);
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    config_globals_open_installation = false;
  }
  {
    // In a client installation, a client is logged in as admin when there's no user in the database.
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    config_globals_client_prepared = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, session_admin_credentials (), request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::admin (), request.session_logic ()->currentLevel ());
    config_globals_client_prepared = false;
  }
  {
    // In a client installation, a client is logged in as the first user in the database.
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    string username = "ঃইঝম";
    int level = 10;
    database_users.addNewUser (username, "password", level, "email");
    config_globals_client_prepared = true;
    Webserver_Request request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, level, request.session_logic ()->currentLevel ());
    config_globals_client_prepared = false;
  }
  {
    refresh_sandbox (true);
    Database_Users database_users = Database_Users ();
    database_users.create ();
    Webserver_Request request = Webserver_Request ();

    // Enter a user into the database.
    string username = "ঃইঝম";
    string password = "ᨃᨄᨔᨕ";
    string email = "email@website";
    int level = 10;
    database_users.addNewUser (username, password, level, email);

    // Log in by providing username and password.
    evaluate (__LINE__, __func__, false, request.session_logic ()->attemptLogin (username, "incorrect", true));
    evaluate (__LINE__, __func__, true, request.session_logic ()->attemptLogin (username, password, true));

    // Check whether logged in also from another session.
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, username, request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, level, request.session_logic ()->currentLevel ());
    
    // Logout in another session, and check it in a subsequent session.
    request = Webserver_Request ();
    request.session_logic ()->logout ();
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    evaluate (__LINE__, __func__, "", request.session_logic ()->currentUser ());
    evaluate (__LINE__, __func__, Filter_Roles::guest(), request.session_logic ()->currentLevel ());
    
    // Login. then vary the browser's signature for subsequent sessions.
    request = Webserver_Request ();
    evaluate (__LINE__, __func__, true, request.session_logic ()->attemptLogin (username, password, true));
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    string remote_address = request.remote_address;
    string user_agent = request.user_agent;
    string accept_language = request.accept_language;
    request = Webserver_Request ();
    request.remote_address = "1.2.3.4";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.remote_address = remote_address;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.user_agent = "User's Agent";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.user_agent = user_agent;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.accept_language = "xy_ZA";
    evaluate (__LINE__, __func__, false, request.session_logic ()->loggedIn ());
    request = Webserver_Request ();
    request.accept_language = accept_language;
    evaluate (__LINE__, __func__, true, request.session_logic ()->loggedIn ());
  }
}


void test_empty_folders ()
{
  // There should be no empty folders in the library, because git does not include them.
  int result = system ("find . -type d -empty > /tmp/bibledittest.txt");
  evaluate (__LINE__, __func__, 0, result);
  string contents = filter_url_file_get_contents ("/tmp/bibledittest.txt");
  evaluate (__LINE__, __func__, "", contents);
  filter_url_unlink ("/tmp/bibledittest.txt");
}


void test_flate ()
{
  // Test for the flate2 template engine.
  string tpl1 = filter_url_create_root_path ("unittests", "tests", "flate1.html");
  string tpl2 = filter_url_create_root_path ("unittests", "tests", "flate2.html");
  Flate flate;
  string desired;
  string actual;

  flate = Flate ();
  desired = 
    "line 1\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl1));
  evaluate (__LINE__, __func__, desired, actual);

  flate = Flate ();
  flate.enable_zone ("one");
  flate.enable_zone ("two");
  desired = 
    "line 1\n"
    "\n"
    "line 2\n"
    "\n"
    "\n"
    "\n"
    "line 3\n"
    "\n"
    "line 4\n"
    "\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl1));
  evaluate (__LINE__, __func__, desired, actual);

  flate = Flate ();
  flate.enable_zone ("one");
  flate.enable_zone ("three");
  flate.set_variable ("three", "THREE");
  desired = 
    "line 1\n"
    "\n"
    "line 2\n"
    "\n"
    "\n"
    "\n"
    "line 4\n"
    "\n"
    "THREE\n"
    "line 5\n"
    "\n"
    "\n"
    "line 6\n"
    "LocalizeOne\n"
    "LocalizeTwo";
  actual = filter_string_trim (flate.render (tpl1));
  evaluate (__LINE__, __func__, desired, actual);
  
  // Test situation that a variable contains dashes (#).
  // This should work correctly.
  flate = Flate ();
  flate.set_variable ("one", "one##one");
  flate.set_variable ("two", "two####two");
  flate.set_variable ("three", "three######three");
  desired =
  "one##one\n"
  "two####two\n"
  "three######three";
  actual = filter_string_trim (flate.render (tpl2));
  evaluate (__LINE__, __func__, desired, actual);
}


void test_checksum_logic ()
{
  // Get1
  {
    string data = "\\v Verse 1";
    string checksum = Checksum_Logic::get (data);
    evaluate (__LINE__, __func__, "10", checksum);
  }
  // Get2
  {
    string data = "Line one\nLine 2\n";
    string checksum = Checksum_Logic::get (data);
    evaluate (__LINE__, __func__, "16", checksum);
  }
  // Send1
  {
    string data = "\\v Verse 1";
    string checksum = Checksum_Logic::send (data);
    string standard = "10\n" + data;
    evaluate (__LINE__, __func__, standard, checksum);
  }
  // Send2
  {
    string data = "Line one\nLine 2\n";
    string checksum = Checksum_Logic::send (data);
    string standard = "16\n" + data;
    evaluate (__LINE__, __func__, standard, checksum);
  }
  // Setup some data.
  refresh_sandbox (true);
  Webserver_Request request;
  request.database_search()->create ();
  request.database_bibles()->storeChapter ("phpunit1", 1, 2, "data1");
  request.database_bibles()->storeChapter ("phpunit1", 1, 3, "data2");
  request.database_bibles()->storeChapter ("phpunit1", 1, 4, "data3");
  request.database_bibles()->storeChapter ("phpunit2", 2, 5, "data4");
  // GetChapter1
  {
    string checksum = Checksum_Logic::getChapter (&request, "phpunit1", 1, 2);
    evaluate (__LINE__, __func__, md5 ("data1"), checksum);
  }
  // GetChapter2
  {
    string checksum = Checksum_Logic::getChapter (&request, "phpunit2", 2, 6);
    evaluate (__LINE__, __func__, md5 (""), checksum);
  }
  // GetBook1
  {
    string checksum = Checksum_Logic::getBook (&request, "phpunit1", 1);
    evaluate (__LINE__, __func__, "2ab6425924e6cd38b2474c543c5ea602", checksum);
  }
  // GetBook2
  {
    string checksum = Checksum_Logic::getBook (&request, "phpunit3", 1);
    evaluate (__LINE__, __func__, "d41d8cd98f00b204e9800998ecf8427e", checksum);
  }
  // GetBible1
  {
    string checksum = Checksum_Logic::getBible (&request, "phpunit1");
    evaluate (__LINE__, __func__, "f9dc679a8712eb6f65b584e9688e9680", checksum);
  }
  // GetBible2
  {
    string checksum = Checksum_Logic::getBible (&request, "phpunit2");
    evaluate (__LINE__, __func__, "ee84a85bac14adb35e887c3d89bc80ab", checksum);
  }
  // GetBibles1
  {
    string checksum = Checksum_Logic::getBibles (&request, {"phpunit1", "phpunit2"});
    evaluate (__LINE__, __func__, "440b2008993816f0bc8c557b64fbdaf2", checksum);
  }
  // GetBibles2
  {
    string checksum = Checksum_Logic::getBibles (&request, {"phpunit3", "phpunit4"});
    evaluate (__LINE__, __func__, "020eb29b524d7ba672d9d48bc72db455", checksum);
  }
}


void test_store_bible_data_safely_setup (Webserver_Request * request, string usfm)
{
  refresh_sandbox (true);
  request->database_search()->create ();
  request->database_bibles()->createBible ("phpunit");
  request->database_bibles()->storeChapter ("phpunit", 1, 1, usfm);
}


void test_store_bible_data ()
{
  Webserver_Request request;
  string usfm =
  "\\c 1\n"
  "\\p\n"
  "\\v 1 Verse 1.\n"
  "\\v 2 Verse 2.\n"
  "\\v 3 Verse 3.\n"
  "\\v 4 Verse 4.\n"
  "\\v 5 Verse 5.\n";
  // SafeStoreChapterSaveOne
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n"
    "\\v 4 Verse 4.\n"
    "\\v 5 Verse 5.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, data, result);
  }
  // SafeStoreChapterSaveTwo
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n"
    "\\v 4 Verse 4.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, data, result);
  }
  // SafeStoreChapterLength
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 2 Verse 2.\n"
    "\\v 3 Verse 3.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, false, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    refresh_sandbox (false);
  }
  // SafeStoreChapterSimilarity
  {
    test_store_bible_data_safely_setup (&request, usfm);
    string data =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Verse 1.\n"
    "\\v 3 Verse 3.\n"
    "\\v 2 Verse 2.\n"
    "\\v 4 Verse 4.\n";
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, data);
    evaluate (__LINE__, __func__, false, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    refresh_sandbox (false);
  }
  // SafeStoreChapterNoChange
  {
    test_store_bible_data_safely_setup (&request, usfm);
    int currentId = request.database_bibles()->getChapterId ("phpunit", 1, 1);
    bool stored = usfm_safely_store_chapter (&request, "phpunit", 1, 1, usfm);
    evaluate (__LINE__, __func__, true, stored);
    string result = request.database_bibles()->getChapter ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, usfm, result);
    int currentId2 = request.database_bibles()->getChapterId ("phpunit", 1, 1);
    evaluate (__LINE__, __func__, currentId, currentId2);
  }
}


void test_editor_export ()
{
  refresh_sandbox (true);
  // Basic test.
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The earth brought forth.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Non-Breaking Spaces
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The&nbsp;earth &nbsp; brought&nbsp;&nbsp;forth.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Embedded Text Spans One
  {
    Webserver_Request request;
    string html = "<p class=\"p\"><span>The <span class=\"add\"><span class=\"nd\">Lord God</span> is calling</span> you</span><span>.</span></p>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The \\add \\+nd Lord God\\+nd* is calling\\add* you.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Basic note
  {
    Webserver_Request request;
    string html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">x</a><span>.</span></p>\n"
    "<div id=\"notes\">\n"
    "<hr>\n"
    "<p class=\"x\"><a href=\"#citation1\" id=\"note1\">x</a><span> </span><span>+ 2 Joh. 1.1</span></p>\n"
    "<br>\n"
    "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth\\x + 2 Joh. 1.1\\x*.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  // Footnote Deleted Body
  {
    Webserver_Request request;
    string html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">f</a><span>.</span></p>\n"
    "<div id=\"notes\">\n"
    "<hr>\n"
    "<p class=\"f\"></p>\n"
    "<br>\n"
    "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
    // Clear message from logbook.
    refresh_sandbox (false);
  }
  // Footnote Deleted Citation
  {
    Webserver_Request request;
    string html =
      "<p class=\"p\"><span>The earth brought forth</span><span>.</span></p>\n"
      "<div id=\"notes\">\n"
      "<hr>\n"
      "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">f</a><span> </span><span>+ </span><span class=\"fk\">brought: </span><span class=\"fl\">Heb. </span><span class=\"fq\">explanation.</span></p>\n"
      "</div>";
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    string standard = "\\p The earth brought forth.";
    evaluate (__LINE__, __func__, standard, usfm);
  }
  refresh_sandbox (false);
}


void test_editor_import ()
{
  // Text Length One
  {
    string usfm =
      "\\c 2\n"
      "\\p\n"
      "\\v 1 Kwasekuqediswa amazulu lomhlaba lalo lonke ibutho lakho\\x + Dute. 4.19. Hlab. 33.6.\\x*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    evaluate (__LINE__, __func__, 61, editor_import.textLength);
    evaluate (__LINE__, __func__,  { make_pair (0, 0), make_pair (1, 2) }, editor_import.verseStartOffsets);
  }
  // Text Length More
  {
    string usfm =
    "\\c 2\n"
    "\\p\n"
    "\\v 1 Kwasekuqediswa amazulu lomhlaba lalo lonke ibutho lakho\\x + Dute. 4.19. Hlab. 33.6.\\x*.\n"
    "\\v 2 UNkulunkulu wasewuqeda ngosuku lwesikhombisa umsebenzi wakhe abewenza. Waphumula ngosuku lwesikhombisa\\x + Eks. 20.11; 31.17. Dute. 5.14. Heb. 4.4.\\x* emsebenzini wakhe wonke abewenza.\n"
    "\\v 3 UNkulunkulu wasebusisa usuku lwesikhombisa, walungcwelisa; ngoba ngalo waphumula emsebenzini wakhe wonke, uNkulunkulu awudalayo wawenza\\f + \\fk wawenza: \\fl Heb. \\fq ukuwenza.\\f*.\n"
    "\\s Isivande seEdeni\n"
    "\\p\n"
    "\\v 4 Lezi yizizukulwana zamazulu lezomhlaba ekudalweni kwakho\\x + 1.1.\\x*, mhla iN\\nd kosi\\nd* uNkulunkulu isenza umhlaba lamazulu,\n"
    "\\v 5 laso sonke isihlahlakazana sensimu, singakabi khona emhlabeni, layo yonke imibhida yeganga\\x + 1.12.\\x*, ingakamili; ngoba iN\\nd kosi\\nd* uNkulunkulu yayinganisanga izulu emhlabeni, njalo kwakungelamuntu wokulima umhlabathi.\n"
    "\\v 6 Kodwa kwenyuka inkungu ivela emhlabathini, yasithelela ubuso bonke bomhlabathi.\n"
    "\\v 7 IN\\nd kosi\\nd* uNkulunkulu yasibumba umuntu ngothuli oluvela emhlabathini\\x + 3.19,23. Hlab. 103.14. Tshu. 12.7. 1 Kor. 15.47.\\x*, yaphefumulela emakhaleni akhe umoya wempilo; umuntu wasesiba ngumphefumulo ophilayo\\x + 7.22. Jobe 33.4. Isa. 2.22. 1 Kor. 15.45.\\x*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    evaluate (__LINE__, __func__, 913, editor_import.textLength);
    evaluate (__LINE__, __func__, { make_pair (0, 0),
                                    make_pair (1, 2),
                                    make_pair (2, 62),
                                    make_pair (3, 202),
                                    make_pair (4, 359),
                                    make_pair (5, 469),
                                    make_pair (6, 676),
                                    make_pair (7, 758) },
                                    editor_import.verseStartOffsets);
  }
  // Space After Starting Marker
  {
    string usfm =
    "\\c 1\n"
    "\\p\n"
    "\\v 2 Text \\add of the \\add*1st\\add  second verse\\add*.\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    string standard =
    "<p class=\"c\"><span>1</span></p>"
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span>Text </span><span class=\"add\">of the </span><span>1st</span><span class=\"add\"> second verse</span><span>.</span></p>";
    evaluate (__LINE__, __func__, standard, html);
  }
  // Apostrophy etc.
  {
    string usfm =
    "\\c 1\n"
    "\\p\n"
    "\\v 1 Judha muranda waJesu Kristu, uye munin'ina waJakobho ...\n";
    Webserver_Request request;
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    string standard =
    "<p class=\"c\"><span>1</span></p><p class=\"p\"><span class=\"v\">1</span><span> </span><span>Judha muranda waJesu Kristu, uye munin'ina waJakobho ...</span></p>";
    evaluate (__LINE__, __func__, standard, html);
  }
  
}


void test_editor_roundtrip ()
{
  refresh_sandbox (true);
  // One Unknown Marker Opener
  {
    string standard_usfm = "\\abc";
    string standard_html = "<p class=\"mono\"><span>\\abc </span></p>";
    
    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, filter_string_trim (html));
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, filter_string_trim (usfm));
  }
  // Two Unknown Marker Openers
  {
    string standard_usfm =
    "\\abc\n"
    "\\abc";
    string standard_html =
    "<p class=\"mono\"><span>\\abc </span></p>"
    "<p class=\"mono\"><span>\\abc </span></p>";
    
    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // One Unknown Markers Closer
  {
    string standard_usfm = "\\abc text\\abc*.";
    string standard_html = "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Two Unknown Markers Closer
  {
    string standard_usfm =
    "\\abc text\\abc*.\n"
    "\\abc text\\abc*.";
    string standard_html =
    "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>"
    "<p class=\"mono\"><span>\\abc </span><span>text</span><span>\\abc*</span><span>.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Identifiers
  {
    string standard_usfm =
    "\\id GEN\n"
    "\\h Header\n"
    "\\toc1 The Book of Genesis\n"
    "\\cl Chapter\n"
    "\\cp ②\n"
    "\\cp Ⅰ";
    string standard_html =
    "<p class=\"mono\"><span>\\id </span><span>GEN</span></p>"
    "<p class=\"mono\"><span>\\h </span><span>Header</span></p>"
    "<p class=\"mono\"><span>\\toc1 </span><span>The Book of Genesis</span></p>"
    "<p class=\"mono\"><span>\\cl </span><span>Chapter</span></p>"
    "<p class=\"mono\"><span>\\cp </span><span>②</span></p>"
    "<p class=\"mono\"><span>\\cp </span><span>Ⅰ</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // One Paragraph
  {
    string standard_usfm = "\\p Paragraph text.";
    string standard_html = "<p class=\"p\"><span>Paragraph text.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Two Paragraphs
  {
    string standard_usfm =
    "\\p Paragraph text.\n"
    "\\p Paragraph txt.";
    string standard_html =
    "<p class=\"p\"><span>Paragraph text.</span></p>"
    "<p class=\"p\"><span>Paragraph txt.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Inline Text
  {
    string standard_usfm = "\\p Paragraph text plus \\add added\\add* text.";
    string standard_html = "<p class=\"p\"><span>Paragraph text plus </span><span class=\"add\">added</span><span> text.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Inline Texts
  {
    string standard_usfm = "\\p Paragraph text plus \\add added\\add* text plus \\add added\\add* text.";
    string standard_html = "<p class=\"p\"><span>Paragraph text plus </span><span class=\"add\">added</span><span> text plus </span><span class=\"add\">added</span><span> text.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Chapter
  {
    string standard_usfm =
    "\\c 1\n"
    "\\p Paragraph.";
    string standard_html =
    "<p class=\"c\"><span>1</span></p>"
    "<p class=\"p\"><span>Paragraph.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Verses
  {
    string standard_usfm =
    "\\p\n"
    "\\v 1 One.\n"
    "\\v 2 Two.";
    string standard_html =
    "<p class=\"p\"><span class=\"v\">1</span><span> </span><span>One.</span><span> </span><span class=\"v\">2</span><span> </span><span>Two.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Peripherals
  {
    string standard_usfm =
    "\\periph Title Page\n"
    "\\periph Publication Data";
    string standard_html =
    "<p class=\"mono\"><span>\\periph </span><span>Title Page</span></p>"
    "<p class=\"mono\"><span>\\periph </span><span>Publication Data</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Picture
  {
    string standard_usfm =
    "\\p Text\n"
    "\\fig DESC|FILE|SIZE|LOC|COPY|CAP|REF\\fig*\n"
    "\\p Text";
    string standard_html =
    "<p class=\"p\"><span>Text</span></p>"
    "<p class=\"mono\"><span>\\fig </span><span>DESC|FILE|SIZE|LOC|COPY|CAP|REF</span><span>\\fig*</span></p>"
    "<p class=\"p\"><span>Text</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Table
  {
    Webserver_Request request;

    string standard_usfm =
    "\\tr \\th1 Tribe \\th2 Leader \\thr3 Number\n"
    "\\tr \\tc1 Reuben \\tc2 Elizur son of Shedeur \\tcr3 46500\n"
    "\\tr \\tc1 Simeon \\tc2 Shelumiel son of Zurishaddai \\tcr3 59300\n"
    "\\tr \\tc1 Gad \\tc2 Eliasaph son of Reuel \\tcr3 45650\n"
    "\\tr \\tc1 \\tcr2 Total: \\tcr3 151450";
    string standard_html =
    "<p class=\"mono\"><span>\\tr </span><span class=\"th1\">Tribe </span><span class=\"th2\">Leader </span><span class=\"thr3\">Number</span></p>"
    "<p class=\"mono\"><span>\\tr </span><span class=\"tc1\">Reuben </span><span class=\"tc2\">Elizur son of Shedeur </span><span class=\"tcr3\">46500</span></p>"
    "<p class=\"mono\"><span>\\tr </span><span class=\"tc1\">Simeon </span><span class=\"tc2\">Shelumiel son of Zurishaddai </span><span class=\"tcr3\">59300</span></p>"
    "<p class=\"mono\"><span>\\tr </span><span class=\"tc1\">Gad </span><span class=\"tc2\">Eliasaph son of Reuel </span><span class=\"tcr3\">45650</span></p>"
    "<p class=\"mono\"><span>\\tr </span><span class=\"tcr2\">Total: </span><span class=\"tcr3\">151450</span></p>";
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    standard_usfm =
    "\\tr \\th1 Tribe \\th2 Leader \\thr3 Number\n"
    "\\tr \\tc1 Reuben \\tc2 Elizur son of Shedeur \\tcr3 46500\n"
    "\\tr \\tc1 Simeon \\tc2 Shelumiel son of Zurishaddai \\tcr3 59300\n"
    "\\tr \\tc1 Gad \\tc2 Eliasaph son of Reuel \\tcr3 45650\n"
    "\\tr \\tcr2 Total: \\tcr3 151450";
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Word List Entry.
  {
    string standard_usfm = "\\p A \\ndx index\\ndx* b \\wh Hebrew\\wh* c.";
    string standard_html =
    "<p class=\"p\"><span>A </span><span class=\"ndx\">index</span><span> b </span><span class=\"wh\">Hebrew</span><span> c.</span></p>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Crossreference
  {
    string standard_usfm = "\\p The elder\\x + 2 Joh. 1.1\\x* to the beloved Gaius.";
    string standard_html =
    "<p class=\"p\"><span>The elder</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span> to the beloved Gaius.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"x\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ 2 Joh. 1.1</span></p>"
    "</div>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Crossreferences
  {
    string standard_usfm =
    "\\p The elder\\x + 2 Joh. 1.1\\x* to the beloved Gaius.\n"
    "\\v 1 The elders\\x + 2 Joh. 2.2\\x* to the beloved.";
    string standard_html =
    "<p class=\"p\"><span>The elder</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span> to the beloved Gaius.</span><span> </span><span class=\"v\">1</span><span> </span><span>The elders</span><a href=\"#note2\" id=\"citation2\" class=\"superscript\">2</a><span> to the beloved.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"x\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ 2 Joh. 1.1</span></p>"
    "<p class=\"x\"><a href=\"#citation2\" id=\"note2\">2</a><span> </span><span>+ 2 Joh. 2.2</span></p>"
    "</div>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Footnote
  {
    string standard_usfm =
    "\\p The earth brought forth\\f + \\fk brought: \\fl Heb. \\fq explanation.\\f*.";
    string standard_html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span>.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ </span><span class=\"fk\">brought: </span><span class=\"fl\">Heb. </span><span class=\"fq\">explanation.</span></p>"
    "</div>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Cycling Note Caller
  {
    string standard_usfm =
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.\n"
    "\\p Text\\f + note\\f*.\n"
    "\\p Text\\fe + note\\fe*.\n"
    "\\p Text\\x + note\\x*.";
    string standard_html =
    "<p class=\"p\"><span>Text</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note2\" id=\"citation2\" class=\"superscript\">2</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note3\" id=\"citation3\" class=\"superscript\">3</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note4\" id=\"citation4\" class=\"superscript\">4</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note5\" id=\"citation5\" class=\"superscript\">5</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note6\" id=\"citation6\" class=\"superscript\">6</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note7\" id=\"citation7\" class=\"superscript\">7</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note8\" id=\"citation8\" class=\"superscript\">8</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note9\" id=\"citation9\" class=\"superscript\">9</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note10\" id=\"citation10\" class=\"superscript\">1</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note11\" id=\"citation11\" class=\"superscript\">2</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note12\" id=\"citation12\" class=\"superscript\">3</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note13\" id=\"citation13\" class=\"superscript\">4</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note14\" id=\"citation14\" class=\"superscript\">5</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note15\" id=\"citation15\" class=\"superscript\">6</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note16\" id=\"citation16\" class=\"superscript\">7</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note17\" id=\"citation17\" class=\"superscript\">8</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note18\" id=\"citation18\" class=\"superscript\">9</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note19\" id=\"citation19\" class=\"superscript\">1</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note20\" id=\"citation20\" class=\"superscript\">2</a><span>.</span></p>"
    "<p class=\"p\"><span>Text</span><a href=\"#note21\" id=\"citation21\" class=\"superscript\">3</a><span>.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation2\" id=\"note2\">2</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation3\" id=\"note3\">3</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation4\" id=\"note4\">4</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation5\" id=\"note5\">5</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation6\" id=\"note6\">6</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation7\" id=\"note7\">7</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation8\" id=\"note8\">8</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation9\" id=\"note9\">9</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation10\" id=\"note10\">1</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation11\" id=\"note11\">2</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation12\" id=\"note12\">3</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation13\" id=\"note13\">4</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation14\" id=\"note14\">5</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation15\" id=\"note15\">6</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation16\" id=\"note16\">7</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation17\" id=\"note17\">8</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation18\" id=\"note18\">9</a><span> </span><span>+ note</span></p>"
    "<p class=\"f\"><a href=\"#citation19\" id=\"note19\">1</a><span> </span><span>+ note</span></p>"
    "<p class=\"fe\"><a href=\"#citation20\" id=\"note20\">2</a><span> </span><span>+ note</span></p>"
    "<p class=\"x\"><a href=\"#citation21\" id=\"note21\">3</a><span> </span><span>+ note</span></p>"
    "</div>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Endnote
  {
    string standard_usfm =
    "\\p The earth brought forth\\fe + \\fk brought: \\fl Heb. \\fq explanation.\\fe*.";
    string standard_html =
    "<p class=\"p\"><span>The earth brought forth</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span>.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"fe\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ </span><span class=\"fk\">brought: </span><span class=\"fl\">Heb. </span><span class=\"fq\">explanation.</span></p>"
    "</div>";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    evaluate (__LINE__, __func__, standard_html, html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Realistic Round Trip 1
  {
    string standard_usfm =
    "\\c 1\n"
    "\\s Ukufika kukaJesu Kristu kuyamenyezelwa nguJohane uMbhabhathizi\n"
    "\\p\n"
    "\\v 1 Ukuqala kwevangeli likaJesu Kristu, iNdodana kaNkulunkulu\\x + Mat. 14.33.\\x*;\n"
    "\\v 2 njengokulotshiweyo kubaprofethi\\x + Mat. 3.1-11. Luka 3.2-16.\\x* \\add ukuthi\\add*: Khangela, mina ngiyathuma isithunywa sami phambi kobuso bakho, esizalungisa indlela yakho phambi kwakho\\x + Mal. 3.1. Mat. 11.10. Luka 7.27.\\x*.\n"
    "\\v 3 Ilizwi lomemezayo enkangala, \\add lithi\\add*: Lungisani indlela yeNkosi; qondisani izindledlana zayo\\x + Isa. 40.3. Joha. 1.23.\\x*.\n"
    "\\v 4 Kweza uJohane ebhabhathiza enkangala, etshumayela ubhabhathizo lokuphenduka kukho ukuthethelelwa kwezono\\x + Mat. 26.28. Luka 1.77.\\x*,\n"
    "\\v 5 njalo kwaphumela kuye ilizwe lonke leJudiya, labo abeJerusalema, basebebhabhathizwa bonke nguye emfuleni iJordani, bevuma izono zabo.\n"
    "\\v 6 Futhi uJohane wayembethe uboya bekamela, elozwezwe lwesikhumba ekhalweni lwakhe, wayesidla intethe loluju lweganga.\n"
    "\\p\n"
    "\\v 7 Watshumayela, esithi: Olamandla kulami uyeza emva kwami, engingafanele ukukhothama ngithukulule umchilo wamanyathela akhe\\x + Gen. 14.23. Isa. 5.27. Luka 3.16. Joha. 1.27.\\x*.\n"
    "\\v 7 Watshumayela, esithi: Olamandla kulami uyeza emva kwami, engingafanele ukukhothama ngithukulule umchilo wamanyathela akhe\\x + Gen. 14.23. Isa. 5.27. Luka 3.16. Joha. 1.27.\\x*.\n"
    "\\v 8 Mina isibili ngilibhabhathize ngamanzi; kodwa yena uzalibhabhathiza ngoMoya oNgcwele.\n"
    "\\s Ukubhabhathizwa kukaJesu\n"
    "\\p\n"
    "\\v 9 Kwasekusithi ngalezonsuku\\x + Mat. 3.13-17. Luka 3.21,22.\\x* kwafika uJesu evela eNazaretha yeGalili\\x + Mat. 2.23.\\x*, wabhabhathizwa nguJohane eJordani.\n"
    "\\v 10 Wenela ukukhuphuka emanzini, wabona amazulu eqhekezeka, loMoya kungathi lijuba esehlela phezu kwakhe;\n"
    "\\v 11 kwasekusiza ilizwi livela emazulwini, \\add lathi\\add*: Wena uyiNdodana yami ethandekayo, engithokoza ngayo.";

    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Realistic Round Trip 2
  {
    string standard_usfm =
    "\\c 1\n"
    "\\s Ukudalwa komhlaba laye umuntu\n"
    "\\p\n"
    "\\v 1 Ekuqaleni\\x + Hlab. 33.6; 136.5; Joha. 1.1-3; Seb. 14.15; 17.24; Kol. 1.16,17; Heb. 1.8-10; 11.3.\\x* uNkulunkulu wadala amazulu lomhlaba.\n"
    "\\v 2 Njalo umhlaba wawungelasimo ungelalutho\\x + Isa. 34.11 Jer. 4.23.\\x*, lobumnyama babuphezu kokujula; loMoya kaNkulunkulu wayendiza phezu kwamanzi.\n"
    "\\v 3 UNkulunkulu wasesithi: Kakube khona ukukhanya\\x + 2 Kor. 4.6\\x*; kwasekusiba khona ukukhanya.\n"
    "\\v 4 UNkulunkulu wasebona ukukhanya ukuthi kuhle; uNkulunkulu wasesehlukanisa phakathi kokukhanya lobumnyama.\n"
    "\\v 5 UNkulunkulu wasebiza ukukhanya ngokuthi yimini, lobumnyama wabubiza ngokuthi yibusuku. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lokuqala.\n"
    "\\p\n"
    "\\v 6 UNkulunkulu wasesithi: Kakube khona umkhathi\\x + Jobe 37.18. Hlab. 136.5. Jer. 10.12; 51.15.\\x* phakathi kwamanzi; kawehlukanise phakathi kwamanzi lamanzi.\n"
    "\\v 7 UNkulunkulu wasesenza umkhathi, wasesehlukanisa phakathi kwamanzi angaphansi komkhathi lamanzi aphezu komkhathi\\x + Hlab. 148.4.\\x*; kwasekusiba njalo.\n"
    "\\v 8 UNkulunkulu wasebiza umkhathi ngokuthi ngamazulu. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesibili.\n"
    "\\p\n"
    "\\v 9 UNkulunkulu wasesithi: Kakuqoqelwe amanzi avela ngaphansi kwamazulu ndawonye\\x + Jobe 38.8. Hlab. 33.7. 2 Pet. 3.5.\\x*, njalo kakubonakale umhlabathi owomileyo; kwasekusiba njalo.\n"
    "\\v 10 UNkulunkulu wasebiza umhlabathi owomileyo ngokuthi ngumhlaba, lenhlangano yamanzi wayibiza ngokuthi zinlwandle. UNkulunkulu wasebona ukuthi kuhle.\n"
    "\\v 11 UNkulunkulu wasesithi: Umhlaba kawumilise utshani\\f + \\fk utshani: \\fl Heb. \\fq utshani obubuthakathaka.\\f*, imibhida ethela inhlanyelo, isihlahla sesithelo sisenza isithelo ngohlobo lwaso, onhlanyelo yaso ikuso phezu komhlaba. Kwasekusiba njalo.\n"
    "\\v 12 Umhlaba wasuveza utshani, imibhida ethela inhlanyelo ngohlobo lwayo, lesihlahla sisenza isithelo, esinhlanyelo yaso ikuso ngohlobo lwaso. UNkulunkulu wasebona ukuthi kuhle.\n"
    "\\v 13 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesithathu.\n"
    "\\p\n"
    "\\v 14 UNkulunkulu wasesithi: Kakube khona izikhanyiso emkhathini wamazulu, ukuze zehlukanise phakathi kwemini lobusuku\\f + \\fk phakathi kwemini lobusuku: \\fl Heb. \\fq phakathi kwemini laphakathi kobusuku.\\f*; njalo zibe yizibonakaliso lezikhathi ezimisiweyo, lensuku leminyaka.\n"
    "\\v 15 Njalo zibe yizikhanyiso emkhathini wamazulu, ukuze zikhanyise emhlabeni. Kwasekusiba njalo.\n"
    "\\v 16 UNkulunkulu wasesenza izikhanyiso ezimbili ezinkulu\\x + Dute. 4.19. Hlab. 136.7,8,9.\\x*: Isikhanyiso esikhulu ukuze sibuse imini, lesikhanyiso esincinyane ukuze sibuse\\f + \\fk sibuse: \\fl Heb. \\fq kube yikubusa kwe-.\\f* ubusuku, kanye lezinkanyezi.\n"
    "\\v 17 UNkulunkulu wasezimisa emkhathini wamazulu, ukuthi zikhanyise emhlabeni,\n"
    "\\v 18 lokuthi zibuse emini\\x + Jer. 31.35\\x* lebusuku, lokwehlukanisa phakathi kokukhanya lobumnyama. UNkulunkulu wasebona ukuthi kuhle.\n"
    "\\v 19 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesine.\n"
    "\\p\n"
    "\\v 20 UNkulunkulu wasesithi: Amanzi kawagcwale ukunyakazela kwemiphefumulo ephilayo, njalo kakuphaphe inyoni phezu komhlaba emkhathini\\f + \\fk emkhathini: \\fl Heb. \\fq ebusweni bomkhathi.\\f* wamazulu.\n"
    "\\v 21 UNkulunkulu wasedala oququmadevu bolwandle abakhulu, laso sonke isidalwa esiphilayo esibhinqikayo, amanzi agcwele sona, ngohlobo lwaso, layo yonke inyoni elempiko, ngohlobo lwayo. UNkulunkulu wasebona ukuthi kuhle.\n"
    "\\v 22 UNkulunkulu wasekubusisa, esithi: Zalani, lande\\x + 8.17; 9.1.\\x*, njalo ligcwalise amanzi enlwandle, lezinyoni kazande emhlabeni.\n"
    "\\v 23 Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesihlanu.\n"
    "\\p\n"
    "\\v 24 UNkulunkulu wasesithi: Umhlaba kawuveze izidalwa eziphilayo ngohlobo lwazo, izifuyo lokuhuquzelayo lenyamazana zomhlaba ngenhlobo zazo. Kwasekusiba njalo.\n"
    "\\v 25 UNkulunkulu wasesenza izinyamazana zomhlaba ngenhlobo zazo, lezifuyo ngenhlobo zazo, lakho konke okuhuquzelayo komhlaba ngenhlobo zakho. UNkulunkulu wasebona ukuthi kuhle.\n"
    "\\p\n"
    "\\v 26 UNkulunkulu wasesithi: Asenze abantu ngomfanekiso wethu\\x + 5.1; 9.6. 1 Kor. 11.7. Efe. 4.24. Kol. 3.10. Jak. 3.9.\\x*, ngesimo sethu, njalo babuse phezu kwenhlanzi zolwandle, laphezu kwenyoni zamazulu, laphezu kwezifuyo, laphezu komhlaba wonke, laphezu kwakho konke okuhuquzelayo lokho okuhuquzela emhlabeni\\x + 9.2. Hlab. 8.6.\\x*.\n"
    "\\v 27 UNkulunkulu wasemdala umuntu ngomfanekiso wakhe; wamdala ngomfanekiso kaNkulunkulu; owesilisa lowesifazana wabadala\\x + 2.18; 5.2. Mal. 2.15. Mat. 19.4. Mark. 10.6.\\x*.\n"
    "\\v 28 UNkulunkulu wasebabusisa, uNkulunkulu wasesithi kibo: Zalani lande, ligcwalise umhlaba\\x + 9.1,7.\\x* njalo liwehlisele phansi; libuse phezu kwenhlanzi zolwandle, laphezu kwenyoni zamazulu, laphezu kwakho konke okuphilayo okuhuquzela emhlabeni.\n"
    "\\p\n"
    "\\v 29 UNkulunkulu wasesithi: Khangelani, ngilinike yonke imibhida ethela inhlanyelo, esebusweni bomhlaba wonke, laso sonke isihlahla, okukuso isithelo sesihlahla esithela\\f + \\fk esithela: \\fl Heb. \\fq esihlanyela.\\f* inhlanyelo; kini singesokudla\\x + 9.3. Hlab. 145.15,16.\\x*.\n"
    "\\v 30 Njalo kuyo yonke inyamazana yomhlaba, lakuyo yonke inyoni yamazulu, lakukho konke okuhuquzela emhlabeni, okukukho umoya\\f + \\fk umoya: \\fl Heb. \\fq umphefumulo.\\f* ophilayo, lonke uhlaza ngolokudla. Kwasekusiba njalo.\n"
    "\\v 31 UNkulunkulu wasebona konke akwenzileyo; khangela-ke kwakukuhle kakhulu\\x + Tshu. 7.29. 1 Tim. 4.4.\\x*. Kwasekusiba yikuhlwa, njalo kwaba yikusa, usuku lwesithupha.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Missing Note Caller.
  {
    string standard_usfm =
    "\\p The earth brought forth\\f + \\fk brought: \\fl Heb. \\fq explanation.\\f*.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    
    html = filter_string_str_replace ("+", " ", html);

    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Missing Note Callers
  {
    string standard_usfm =
    "\\p The earth brought forth\\f + \\fk brought: \\fl Heb. \\fq explanation.\\f*.\n"
    "\\p The earth brought forth\\f + \\fk brought: \\fl Heb. \\fq explanation.\\f*.\n"
    "\\p The earth brought forth\\f + \\fk brought: \\fl Heb. \\fq explanation.\\f*.\n"
    "\\p The earth brought forth\\x + \\fk brought: \\fl Heb. \\fq explanation.\\x*.\n"
    "\\p The earth brought forth\\fe + \\fk brought: \\fl Heb. \\fq explanation.\\fe*.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (standard_usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string html = editor_import.get ();
    
    html = filter_string_str_replace ("+", " ", html);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    string usfm = editor_export.get ();
    evaluate (__LINE__, __func__, standard_usfm, usfm);
  }
  // Nested Text Markup 1
  {
    string usfm =
    "\\p\n"
    "\\v 2 \\add add\\+nd addnd\\+nd*\\add*.";

    Webserver_Request request;

    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span class=\"add\">add</span><span class=\"add nd\">addnd</span><span>.</span></p>";
    evaluate (__LINE__, __func__, html, output);

    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    usfm =
    "\\p\n"
    "\\v 2 \\add add\\add*\\add \\+nd addnd\\+nd*\\add*.";
    evaluate (__LINE__, __func__, usfm, output);
  }
  // Nested Text Markup 2
  {
    string usfm =
    "\\p\n"
    "\\v 2 \\add add\\+nd addnd\\add*.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span class=\"add\">add</span><span class=\"add nd\">addnd</span><span>.</span></p>";
    evaluate (__LINE__, __func__, html, output);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    usfm =
    "\\p\n"
    "\\v 2 \\add add\\add*\\add \\+nd addnd\\+nd*\\add*.";
    evaluate (__LINE__, __func__, usfm, output);
  }
  // Nested Text Markup 3
  {
    string usfm =
    "\\p The \\add \\+nd Lord God\\+nd* is\\add* calling you";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"p\"><span>The </span><span class=\"add nd\">Lord God</span><span class=\"add\"> is</span><span> calling you</span></p>";
    evaluate (__LINE__, __func__, html, output);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    usfm = "\\p The \\add \\+nd Lord God\\+nd*\\add*\\add  is\\add* calling you";
    evaluate (__LINE__, __func__, usfm, output);
  }
  // Nested Note Markup 1
  {
    string usfm =
    "\\p\n"
    "\\v 2 text\\f + \\fk fk \\+fdc key-fdc\\+fdc*\\fk* normal\\f*.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span>text</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span>.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ </span><span class=\"fk\">fk </span><span class=\"fk fdc\">key-fdc</span><span> normal</span></p>"
    "</div>";
    evaluate (__LINE__, __func__, html, output);
    
    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    usfm =
    "\\p\n"
    "\\v 2 text\\f + \\fk fk \\fk \\+fdc key-fdc\\+fdc*\\fk* normal\\f*.";
    evaluate (__LINE__, __func__, usfm, output);
  }
  // Nested Note Markup 2
  {
    string usfm =
    "\\p"
    "\\v 2 text\\f + \\fk fk \\+fdc key-fdc\\fk* normal\\f*.";
    
    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"p\"><span class=\"v\">2</span><span> </span><span>text</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span>.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ </span><span class=\"fk\">fk </span><span class=\"fk fdc\">key-fdc</span><span> normal</span></p>"
    "</div>";
    evaluate (__LINE__, __func__, html, output);

    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    usfm =
    "\\p\n"
    "\\v 2 text\\f + \\fk fk \\fk \\+fdc key-fdc\\+fdc*\\fk* normal\\f*.";
    evaluate (__LINE__, __func__, usfm, output);
  }
  // Fix for change \ft to \fk
  {
    // A bug was discovered in the Bible editor where "... \fk ... \ft ..." was changed to "... \fk ... \fk ...".
    // The bug was fixed.
    // This test would catch any regression.
    string usfm =
    "\\c 1\n"
    "\\v 1 Canonical text\\f + \\fr 1:1 \\fk Footnote fk style \\ft Footnote ft style\\f* canonical text.";

    Webserver_Request request;
    
    Editor_Import editor_import = Editor_Import (&request);
    editor_import.load (usfm);
    editor_import.stylesheet (styles_logic_standard_sheet ());
    editor_import.run ();
    string output = editor_import.get ();
    string html =
    "<p class=\"c\"><span>1</span><span> </span><span class=\"v\">1</span><span> </span><span>Canonical text</span><a href=\"#note1\" id=\"citation1\" class=\"superscript\">1</a><span> canonical text.</span></p>"
    "<div id=\"notes\">"
    "<hr/>"
    "<p class=\"f\"><a href=\"#citation1\" id=\"note1\">1</a><span> </span><span>+ </span><span class=\"fr\">1:1 </span><span class=\"fk\">Footnote fk style </span><span class=\"ft\">Footnote ft style</span></p>"
    "</div>";
    evaluate (__LINE__, __func__, html, output);

    Editor_Export editor_export (&request);
    editor_export.load (html);
    editor_export.stylesheet (styles_logic_standard_sheet ());
    editor_export.run ();
    output = editor_export.get ();
    evaluate (__LINE__, __func__, usfm, output);
  }
  refresh_sandbox (false);
}


void test_workbench_logic ()
{
  // Initial setup for the tests.
  refresh_sandbox (true);
  Webserver_Request request;
  request.database_users ()->create ();
  request.session_logic ()->setUsername ("phpunit");
  {
    evaluate (__LINE__, __func__, "100%", workbenchProcessUnits ("100"));
    evaluate (__LINE__, __func__, "100%", workbenchProcessUnits ("100 %"));
    evaluate (__LINE__, __func__, "100px", workbenchProcessUnits ("100 px"));
  }
  {
    evaluate (__LINE__, __func__, "default", workbenchGetActiveWorkbench (&request));
    request.database_config_user()->setActiveWorkbench ("unittest");
    evaluate (__LINE__, __func__, "unittest", workbenchGetActiveWorkbench (&request));
  }
  {
    map <int, string> standard = { make_pair (0, "edit/index"), make_pair (5, "resource/index")};
    evaluate (__LINE__, __func__, standard, workbenchDefaultURLs (1));
  }
  {
    map <int, string> urls = { make_pair (10, "url1"), make_pair (2, "url2")};
    workbenchSetURLs (&request, urls);
    map <int, string> result = workbenchGetURLs (&request, false);
    evaluate (__LINE__, __func__, urls, result);
  }
  {
    map <int, string> widths = { make_pair (0, "24%"), make_pair (1, "24%"), make_pair (2, "24%"), make_pair (3, "24%")};
    map <int, string> result = workbenchGetWidths (&request);
    evaluate (__LINE__, __func__, widths, result);
  }
  {
    vector <string> workbenches = workbenchGetWorkbenches (&request);
    evaluate (__LINE__, __func__, {"unittest"}, workbenches);
  }
  refresh_sandbox (true);
  request.database_users ()->create ();
  {
    request.database_config_user()->setActiveWorkbench ("unittest");
    workbenchSetURLs (&request, {make_pair (10, "url10")});
    request.database_config_user()->setActiveWorkbench ("unittest2");
    map <int, string> standard = { make_pair (0, "url0"), make_pair (5, "url5")};
    workbenchSetURLs (&request, standard);
    vector <string> workbenches = workbenchGetWorkbenches (&request);
    evaluate (__LINE__, __func__, {"unittest", "unittest2"}, workbenches);
    workbenchDeleteWorkbench (&request, "unittest3");
    workbenches = workbenchGetWorkbenches (&request);
    evaluate (__LINE__, __func__, {"unittest", "unittest2"}, workbenches);
    workbenchDeleteWorkbench (&request, "unittest2");
    workbenches = workbenchGetWorkbenches (&request);
    evaluate (__LINE__, __func__, {"unittest"}, workbenches);
  }
  refresh_sandbox (true);
  request.database_users ()->create ();
  {
    request.database_config_user()->setActiveWorkbench ("unittest2");
    workbenchSetURLs (&request, {make_pair (10, "url10")});
    request.database_config_user()->setActiveWorkbench ("abc32");
    workbenchSetURLs (&request, {make_pair (10, "url10"), make_pair (11, "url11")});
    request.database_config_user()->setActiveWorkbench ("zzz");
    workbenchSetURLs (&request, {make_pair (120, "url120"), make_pair (121, "url121")});
    workbenchOrderWorkbenches (&request, {"zzz", "yyy", "unittest2", "abc32"});
    vector <string> workbenches = workbenchGetWorkbenches (&request);
    evaluate (__LINE__, __func__, {"zzz", "unittest2", "abc32"}, workbenches);
  }
}


void test_client_logic ()
{
  refresh_sandbox (true);
  // Test Client Enabled.
  {
    bool enabled = client_logic_client_enabled ();
    evaluate (__LINE__, __func__, false, enabled);
    client_logic_enable_client (true);
    // When a client is disabled in config.h, it remains disabled, no matter the setting in the database.
    enabled = client_logic_client_enabled ();
    evaluate (__LINE__, __func__, false, enabled);
    client_logic_enable_client (false);
    enabled = client_logic_client_enabled ();
    evaluate (__LINE__, __func__, false, enabled);
  }
  // Test Create Note En/Decode
  {
    string data = client_logic_create_note_encode ("bible", 1, 2, 3, "summary", "line1\nline2", false);
    string standard =
    "bible\n"
    "1\n"
    "2\n"
    "3\n"
    "summary\n"
    "0\n"
    "line1\n"
    "line2";
    evaluate (__LINE__, __func__, standard, data);

    string bible;
    int book, chapter, verse;
    string summary, contents;
    bool raw;
    client_logic_create_note_decode (standard, bible, book, chapter, verse, summary, contents, raw);
    evaluate (__LINE__, __func__, "bible", bible);
    evaluate (__LINE__, __func__, 1, book);
    evaluate (__LINE__, __func__, 2, chapter);
    evaluate (__LINE__, __func__, 3, verse);
    evaluate (__LINE__, __func__, "summary", summary);
    evaluate (__LINE__, __func__, false, raw);
    standard =
    "line1\n"
    "line2";
    evaluate (__LINE__, __func__, standard, contents);
  }
}


Checks_Sentences test_check_sentences_setup ()
{
  Checks_Sentences check;
  check.enterCapitals ("A B C D E F G H I J K L M N O P Q R S T U V W X Y Z");
  check.enterSmallLetters ("a b c d e f g h i j k l m n o p q r s t u v w x y z");
  check.enterEndMarks (". ! ? :");
  check.enterCenterMarks (", ;");
  check.enterDisregards ("( ) [ ] { } ' \" * - 0 1 2 3 4 5 6 7 8 9");
  check.enterNames ("Nkosi Longnamelongnamelongname");
  check.initialize ();
  return check;
}


void test_check_sentences ()
{
  // Test Unknown Character
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({make_pair (1, "Abc ζ abc.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {make_pair (1, "Unknown character: ζ")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Capital After Mid Sentence Punctuation Mark
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({make_pair (2, "He said, Go.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {make_pair (2, "Capital follows mid-sentence punctuation mark: He said, Go.")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Capital Straight After Mid Sentence Punctuation Mark
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({make_pair (2, "He said,Go.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { make_pair (2, "Capital follows straight after a mid-sentence punctuation mark: He said,Go.")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Small Letter Straight After Mid Sentence Punctuation Mark
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({make_pair (2, "He said,go.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { make_pair (2, "Small letter follows straight after a mid-sentence punctuation mark: He said,go.")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Two Verses Okay
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({ make_pair (17, "Jezus kwam naar de wereld,"), make_pair (18, "dat hij zou lijden.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard;
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Name After Comma Several Verses Okay
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({
      make_pair (17, "Kwasekusithi esebakhuphele ngaphandle, yathi: Baleka ngenxa yempilo yakho, ungakhangeli ngemva kwakho, ungemi egcekeni lonke; balekela entabeni hlezi ubhujiswe."),
      make_pair (18, "ULothi wasesithi kuwo: Kakungabi njalo, Nkosi yami."),
      make_pair (19, "Khangela-ke, inceku yakho ithole umusa emehlweni akho, ukhulisile isihawu sakho, osenze kimi, ukugcina uphila umphefumulo wami; kodwa mina ngingeke ngiphephele entabeni, hlezi ububi bunamathele kimi, besengisifa."),
      make_pair (20, "Khangela-ke, lumuzi useduze ukubalekela kuwo; futhi umncinyane. Ngicela ngibalekele kuwo (kambe kawumncinyane?) Lomphefumulo wami uphile."),
      make_pair (21, "Yasisithi kuye: Khangela, ngibemukele ubuso bakho lakulolu udaba, ukuze ngingawuchithi umuzi okhulume ngawo."),
      make_pair (22, "Phangisa, balekela kuwo; ngoba ngingeze ngenza ulutho uze ufike kuwo. Ngakho babiza ibizo lomuzi ngokuthi yiZowari.")
    });
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard;
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Long Name
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({make_pair (17, "O, Longnamelongnamelongname.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard;
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test No Space After Full Stop
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({ make_pair (2, "He did that.He went.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (2, "A letter follows straight after an end-sentence punctuation mark: did that.He went."),
      make_pair (2, "No capital after an end-sentence punctuation mark: id that.He went.")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Capital Full Stop
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({ make_pair (2, "He did that. he went.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { make_pair (2, "No capital after an end-sentence punctuation mark: id that. he went.")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Paragraph One
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.paragraphs ({ make_pair (1, "he said")}, {0});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
                       make_pair (1, "Paragraph does not start with a capital: h"),
                       make_pair (1, "Paragraph does not end with an end marker: d")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Paragraph Two
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.paragraphs ({ make_pair (1, "εὐθέως")}, {0});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
                       make_pair (1, "Paragraph does not start with a capital: ε"),
                       make_pair (1, "Paragraph does not end with an end marker: ς")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Paragraph Three
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.paragraphs ({ make_pair (1, "Immediately εὐθέως.")}, {0});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard;
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test  Two Punctuation Marks One
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({ make_pair (2, "He did that..")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { make_pair (2, "Two punctuation marks in sequence: did that..")};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Two Punctuation Marks Two
  {
    Checks_Sentences check = test_check_sentences_setup ();
    check.check ({ make_pair (2, "He did ;. That.")});
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { make_pair (2, "Two punctuation marks in sequence: He did ;. That.")};
    evaluate (__LINE__, __func__, standard, results);
  }
}


void test_check_versification ()
{
  // Test Available Books
  {
    refresh_sandbox (true);
    Database_Versifications database_versifications;
    database_versifications.create ();
    database_versifications.defaults ();
    Database_Check database_check;
    database_check.create ();
    vector <int> books = database_versifications.getBooks ("English");
    vector <int> fault = filter_string_array_diff (books, {10});
    Checks_Versification::books ("Bible", fault);
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 1, results.size());
    if (results.size ()) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 10, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 1, hit.verse);
      evaluate (__LINE__, __func__, "This book is absent from the Bible", hit.data);
    }
  }
  // Test Available Chapters
  {
    refresh_sandbox (true);
    Database_Versifications database_versifications;
    database_versifications.create ();
    database_versifications.defaults ();
    Database_Check database_check;
    database_check.create ();

    // Ruth.
    int book = 8;
    // One missing and one extra chapter.
    vector <int> chapters = {0, 2, 3, 4, 5};
    // Run check.
    Checks_Versification::chapters ("Bible", book, chapters);

    // Verify results.
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 2, results.size());
    if (results.size () == 2) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 1, hit.verse);
      evaluate (__LINE__, __func__, "This chapter is missing", hit.data);
      hit = results[1];
      evaluate (__LINE__, __func__, 2, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 5, hit.chapter);
      evaluate (__LINE__, __func__, 1, hit.verse);
      evaluate (__LINE__, __func__, "This chapter is extra", hit.data);
    }
  }
  // Test Available Verses One
  {
    refresh_sandbox (true);
    Database_Versifications database_versifications;
    database_versifications.create ();
    database_versifications.defaults ();
    Database_Check database_check;
    database_check.create ();

    // Ruth 1.
    int book = 8;
    int chapter = 1;
    // One missing and one extra verse.
    vector <int> verses = {0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24};

    // Run check.
    Checks_Versification::verses ("Bible", book, chapter, verses);

    // Verify results.
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 4, results.size());
    if (results.size () == 4) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 1, hit.verse);
      evaluate (__LINE__, __func__, "This verse is missing according to the versification system", hit.data);
      hit = results[1];
      evaluate (__LINE__, __func__, 2, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 24, hit.verse);
      evaluate (__LINE__, __func__, "This verse is extra according to the versification system", hit.data);
      hit = results[2];
      evaluate (__LINE__, __func__, 3, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 2, hit.verse);
      evaluate (__LINE__, __func__, "The verse is out of sequence", hit.data);
      hit = results[3];
      evaluate (__LINE__, __func__, 4, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 24, hit.verse);
      evaluate (__LINE__, __func__, "The verse is out of sequence", hit.data);
    }
  }
  // Test Available Verses Two
  {
    refresh_sandbox (true);
    Database_Versifications database_versifications;
    database_versifications.create ();
    database_versifications.defaults ();
    Database_Check database_check;
    database_check.create ();
    
    // Ruth 1.
    int book = 8;
    int chapter = 1;

    // Verses out of order.
    vector <int> verses = {0, 2, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22};
    
    // Run check.
    Checks_Versification::verses ("Bible", book, chapter, verses);
    
    // Verify results.
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 3, results.size());
    if (results.size () == 3) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 2, hit.verse);
      evaluate (__LINE__, __func__, "The verse is out of sequence", hit.data);
      hit = results[1];
      evaluate (__LINE__, __func__, 2, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 1, hit.verse);
      evaluate (__LINE__, __func__, "The verse is out of sequence", hit.data);
      hit = results[2];
      evaluate (__LINE__, __func__, 3, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 8, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 3, hit.verse);
      evaluate (__LINE__, __func__, "The verse is out of sequence", hit.data);
    }
  }
}


void test_check_usfm ()
{
  refresh_sandbox (true);
  Database_Styles database_styles = Database_Styles ();
  database_styles.create ();
  // Test Malformed Verse
  {
    string usfm =
    "\\c 1\n"
    "\\p\n"
    "\\v 2,He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (2, "Malformed verse number: \\v 2,He said.")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test New Line In USFM Good
  {
    string usfm =
    "\\c 1\n"
    "\\p He said.\n"
    "\\v 1 He said.\n"
    "\\p He said.\n"
    "\\v 2 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = { };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test New Line In USFM One
  {
    string usfm =
    "\\c 1\n"
    "\\p He said.\n"
    "\\v 1 He said.\n"
    "\\\n"
    "\\p He said.\n"
    "\\v 2 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "New line within USFM:  \\ \\p He s")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test New Line In USFM Two
  {
    string usfm =
    "\\c 1\n"
    "\\p He said.\n"
    "\\v 1 He said.\n"
    "\\\n"
    "\\p He said.\n"
    "\\v 3 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "New line within USFM:  \\ \\p He s")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Unknown Usfm One
  {
    string usfm =
    "\\c 1\n"
    "\\p He said.\n"
    "\\v 1 He said \\add something\\add*.\n"
    "\\p,p He said.\n"
    "\\v 3 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (1, "Marker not in stylesheet: \\p,p ")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Unknown Usfm Two
  {
    string usfm =
    "\\c 1\n"
    "\\pHe said.\n"
    "\\v 1 He said \\add something\\add*.\n"
    "\\p He said.\n"
    "\\v 3 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Marker not in stylesheet: \\pHe ")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Id One
  {
    string usfm =
    "\\id GENN\n"
    "\\p He said.\n"
    "\\v 1 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Unknown ID: \\id GENN")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Id Two
  {
    string usfm =
    "\\id\n"
    "\\p He said.\n"
    "\\v 1 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Unknown ID: \\id\\p ")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Id Three
  {
    string usfm =
    "\\id Gen\n"
    "\\p He said.\n"
    "\\v 1 He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Unknown ID: \\id Gen")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Forward Slash One
  {
    string usfm =
    "\\id GEN\n"
    "\\p He said.\n"
    "\\v 1 He said. He said something/add*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Forward slash instead of backslash: /add")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Forward Slash Two
  {
    string usfm =
    "\\id GEN\n"
    "\\p He said.\n"
    "\\v 1 /v He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (0, "Forward slash instead of backslash: /v")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Clean Usfm
  {
    string usfm =
    "\\id GEN\n"
    "\\c 35\n"
    "\\s UNkulunkulu ubusisa uJakobe eBhetheli\n"
    "\\p\n"
    "\\v 1 UNkulunkulu wasesithi kuJakobe: Sukuma, yenyukela eBhetheli\\x + 28.13-19.\\x*, uhlale khona; umenzele lapho ilathi uNkulunkulu owabonakala kuwe mhla ubaleka ebusweni bukaEsawu umnewenu\\x + 27.43.\\x*.\n"
    "\\v 2 UJakobe wasesithi kundlu yakhe\\x + 18.19. Josh. 24.15.\\x* lakubo bonke ababelaye: Susani onkulunkulu bezizwe abaphakathi kwenu\\x + 31.19. Josh. 24.2,23. 1 Sam. 7.3.\\x*, lizihlambulule, lintshintshe izembatho zenu\\x + Eks. 19.10.\\x*.\n"
    "\\v 3 Asisukume senyukele eBhetheli, ngimenzele khona uNkulunkulu ilathi owangiphendula ngosuku lokuhlupheka kwami\\x + 32.7,24.\\x*, njalo waba lami endleleni engahamba kuyo\\x + 28.20; 31.3.\\x*.\n"
    "\\v 4 Basebenika uJakobe bonke onkulunkulu bezizwe ababesesandleni sabo, lamacici\\x + Hos. 2.13. Gen. 24.22.\\x* ayesezindlebeni zabo; uJakobe wasekufihla ngaphansi \\add kwesihlahla\\add* \\w se-okhi\\w**\\x + Josh. 24.26. 2 Sam. 18.9. 1 Kho. 13.14. 1 Lan. 10.12. Gen. 35.8.\\x* eliseShekema.\n"
    "\\v 5 Basebehamba; lokwesabeka kukaNkulunkulu\\x + 9.2. Eks. 15.16; 23.27. Dute. 11.25. Josh. 2.9. 2 Lan. 14.14.\\x* kwakuphezu kwemizi eyayibazingelezele, njalo kabawaxotshanga amadodana kaJakobe.\n"
    "\\p\n"
    "\\v 6 UJakobe wasefika eLuzi\\x + 28.19.\\x*, eselizweni leKhanani, eyiBhetheli, yena labantu bonke ababelaye.\n"
    "\\v 7 Wasesakha khona ilathi, wayibiza indawo \\add ngokuthi\\add* iEli-Bhetheli\\f + \\fk iEli-Bhetheli: \\fl okuyikuthi, \\fq uNkulunkulu weBhetheli.\\f*, ngoba lapho uNkulunkulu wabonakala kuye ekubalekeleni ubuso bomnewabo.\n"
    "\\v 8 Njalo uDebora umlizane kaRebeka wafa\\x + 24.59.\\x*, wangcwatshwa ezansi kweBhetheli ngaphansi kwesihlahla se-okhi\\x + 13.18; 35.4.\\x*, ngakho wasitha ibizo laso iAloni-Bakuthi\\f + \\fk iAloni-Bakuthi: \\fl okuyikuthi, \\fq ie-oki yokulila.\\f*.\n"
    "\\p\n"
    "\\v 9 UNkulunkulu wasebuya wabonakala kuJakobe evela ePadani-Arama, wambusisa.\n"
    "\\v 10 UNkulunkulu wasesithi kuye: Ibizo lakho nguJakobe; ibizo lakho kalisayikuthiwa nguJakobe\\x + 17.5,15.\\x*, kodwa uIsrayeli kuzakuba libizo lakho. Wabiza ibizo lakhe \\add ngokuthi\\add* uIsrayeli\\x + 32.28.\\x*.\n"
    "\\v 11 UNkulunkulu wasesithi kuye: NginguNkulunkulu uSomandla\\x + 17.1.\\x*; zala wande\\x + 28.3; 48.4.\\x*; isizwe, yebo ibandla lezizwe kuzavela kuwe\\x + 17.5,6,16; 26.4.\\x*; lamakhosi azaphuma ekhalweni lwakho.\n"
    "\\v 12 Lelizwe engalinika uAbrahama loIsaka ngizalinika wena, lenzalweni yakho emva kwakho ngizayinika ilizwe\\x + 12.7; 13.15; 17.8; 26.3; 28.13.\\x*.\n"
    "\\v 13 UNkulunkulu wasesenyuka esuka kuye endaweni lapho ayekhuluma laye khona\\x + 17.22.\\x*.\n"
    "\\v 14 UJakobe wasemisa insika endaweni lapho ayekhuluma laye khona, insika yelitshe\\x + 28.18; 31.45.\\x*. Wathululela phezu kwayo umnikelo wokunathwayo\\x + Nani 28.7.\\x*, wathela phezu kwayo amagcobo.\n"
    "\\v 15 UJakobe waseyitha ibizo lendawo, lapho uNkulunkulu ayekhulume laye khona, iBhetheli\\x + 28.19.\\x*.\n"
    "\\s Ukufa kukaRasheli\n"
    "\\p\n"
    "\\v 16 Basebehamba besuka eBhetheli; kwathi kusesengummango\\f + \\fk ummango: \\fl Heb. \\fq isiqetshana somhlabathi.\\f* ukuya eEfrathi, uRasheli wabeletha, wayelobunzima ekubeletheni.\n"
    "\\v 17 Kwasekusithi ekubeletheni kwakhe nzima, umbelethisi wathi kuye: Ungesabi, ngoba lalo uzakuba yindodana yakho\\x + 30.24. 1 Sam. 4.20.\\x*.\n"
    "\\v 18 Kwasekusithi umphefumulo wakhe usuphuma, ngoba esesifa, wayitha ibizo layo uBenoni\\f + \\fk uBenoni: \\fl okuyikuthi, \\fq indodana yosizi lwami.\\f*; kodwa uyise wayibiza \\add ngokuthi\\add* nguBhenjamini\\f + \\fk uBhenjamini: \\fl okuyikuthi, \\fq indodana yesandla sokunene.\\f*.\n"
    "\\v 19 URasheli wasesifa\\x + 48.7.\\x*, wangcwatshwa endleleni eya eEfrathi, eyiBhethelehema\\x + Ruthe 1.2; 4.11. Mika 5.2. Mat. 2.6,16-18.\\x*.\n"
    "\\v 20 UJakobe wasemisa insika phezu kwengcwaba lakhe; le yinsika yengcwaba likaRasheli kuze kube lamuhla\\x + 1 Sam. 10.2.\\x*.\n"
    "\\p\n"
    "\\v 21 UIsrayeli wasehamba, wamisa ithente lakhe ngaphambili komphotshongo weEderi\\x + Mika 4.8.\\x*.\n"
    "\\v 22 Kwasekusithi uIsrayeli ehlala kulelolizwe, uRubeni waya walala loBiliha\\x + 49.4. 1 Lan. 5.1. Gen. 30.4; 37.2. 2 Sam. 16.22; 20.3. 1 Kor. 5.1.\\x* umfazi omncinyane kayise; uIsrayeli wasekuzwa.\n"
    "\\s Amadodana kaJakobe lokufa kukaIsaka\n"
    "\\p Amadodana kaJakobe-ke ayelitshumi lambili\\x + 46.8-27. Eks. 1.2-4.\\x*;\n"
    "\\v 23 amadodana kaLeya: Izibulo likaJakobe, uRubeni, loSimeyoni loLevi loJuda loIsakari loZebuluni;\n"
    "\\v 24 amadodana kaRasheli: OJosefa loBhenjamini;\n"
    "\\v 25 lamadodana kaBiliha, incekukazi kaRasheli: ODani loNafithali;\n"
    "\\v 26 lamadodana kaZilipa, incekukazi kaLeya: OGadi loAsheri; la ngamadodana kaJakobe azalelwa yena ePadani-Arama.\n"
    "\\p\n"
    "\\v 27 UJakobe wasefika kuIsaka uyise eMamre\\x + 13.18; 23.2,19.\\x*, eKiriyathi-Arba\\x + Josh. 14.15; 15.13.\\x*, eyiHebroni, lapho uAbrahama ahlala khona njengowezizwe, loIsaka\\x + 28.10.\\x*.\n"
    "\\v 28 Lensuku zikaIsaka zaziyiminyaka elikhulu lamatshumi ayisificaminwembili.\n"
    "\\v 29 UIsaka wasehodoza, wafa, wabuthelwa ezizweni zakibo\\x + 15.15; 25.8.\\x*, emdala, enele ngensuku. Amadodana akhe uEsawu loJakobe asemngcwaba\\x + 25.9; 49.31.\\x*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Widow Backslash One
  {
    string usfm =
    "\\id GEN\n"
    "\\p\n"
    "\\v 1 \\ He said.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (1, "Widow backslash: \\ ")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Matching Markers One
  {
    string usfm =
    "\\id GEN\n"
    "\\v 1  He said \\add addition\\add*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Matching Markers Two
  {
    string usfm =
    "\\id GEN\n"
    "\\v 1  He said addition\\add*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (1, "Closing marker does not match opening marker : \\add*")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Matching Markers Three
  {
    string usfm =
    "\\id GEN\n"
    "\\v 1  He said \\add addition\\add .\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (1, "Repeating opening marker: \\add "),
      make_pair (1, "Unclosed markers: add")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Matching Markers Four
  {
    string usfm =
    "\\id GEN\n"
    "\\v 8 Kodwa uNowa wazuza umusa emehlweni eN\\nd kosi\\x + 19.19.\\nd*\\x*.\n"
    "\\v 9 Kodwa uNowa wazuza umusa emehlweni eN\\nd kosi\\x + 19.19.\\x*\\nd*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {};
    evaluate (__LINE__, __func__, standard, results);
  }
  // Test Matching Markers Five
  {
    string usfm =
    "\\id GEN\n"
    "\\v 8 Kodwa uNowa wazuza umusa \\add emehlweni eN\\nd kosi\\x + 19.19.\\nd*\\x*.\n";
    Checks_Usfm check = Checks_Usfm ("phpunit");
    check.initialize ();
    check.check (usfm);
    check.finalize ();
    vector <pair<int, string>> results = check.getResults ();
    vector <pair<int, string>> standard = {
      make_pair (8, "Unclosed markers: add")
    };
    evaluate (__LINE__, __func__, standard, results);
  }
}


void test_check_verses ()
{
  refresh_sandbox (true);
  Database_Check database_check;
  database_check.create ();
  // Test Missing Punctuation At End1
  {
    map <int, string> verses = {
      make_pair (2, "He said."),
      make_pair (3, "He didn't say"),
      make_pair (4, "He said.")
    };
    Checks_Verses::missingPunctuationAtEnd ("1", 1, 1, verses, ", ;", ". ! ? :");
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 1, results.size());
    if (results.size ()) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 1, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 3, hit.verse);
      evaluate (__LINE__, __func__, "No punctuation at end of verse: y", hit.data);
    }
  }
  database_check.truncateOutput ("");
  // Test Pattern1
  {
    map <int, string> verses = {
      make_pair (2, "He said."),
      make_pair (3, "He didn't say"),
      make_pair (4, "He said.")
    };
    Checks_Verses::patterns ("1", 1, 1, verses, {"did"});
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 1, results.size());
    if (results.size ()) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 1, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 3, hit.verse);
      evaluate (__LINE__, __func__, "Pattern found in text: did", hit.data);
    }
  }
  database_check.truncateOutput ("");
  // Test Pattern2
  {
    map <int, string> verses = {
      make_pair (2, "He said."),
      make_pair (3, "He didn't say"),
      make_pair (4, "He said.")
    };
    Checks_Verses::patterns ("1", 1, 1, verses, {"Did"});
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 0, results.size());
  }
  database_check.truncateOutput ("");
  // Test Pattern3
  {
    map <int, string> verses = {
      make_pair (2, "He said."),
      make_pair (3, "He didn't say"),
      make_pair (4, "He said.")
    };
    Checks_Verses::patterns ("1", 1, 1, verses, {"said"});
    vector <Database_Check_Hit> results = database_check.getHits ();
    evaluate (__LINE__, __func__, 2, results.size());
    if (results.size () == 2) {
      Database_Check_Hit hit = results[0];
      evaluate (__LINE__, __func__, 1, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 1, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 2, hit.verse);
      evaluate (__LINE__, __func__, "Pattern found in text: said", hit.data);
      hit = results[1];
      evaluate (__LINE__, __func__, 2, hit.rowid);
      evaluate (__LINE__, __func__, 0, hit.bible);
      evaluate (__LINE__, __func__, 1, hit.book);
      evaluate (__LINE__, __func__, 1, hit.chapter);
      evaluate (__LINE__, __func__, 4, hit.verse);
      evaluate (__LINE__, __func__, "Pattern found in text: said", hit.data);
    }
  }
  database_check.truncateOutput ("");
}


void test_hyphenate ()
{
  vector <string> firstset = {"a", "e", "i", "o", "u"};
  vector <string> secondset = {"b", "c", "d", "f", "g", "h", "j", "k", "l", "m", "n", "p", "q", "r", "s", "t", "v", "w", "x", "y", "z"};
  string input =
  "\\s \\nd UNkulunkulu\\nd* u\\add ba\\add*xwayisa ngokulunga okungokwabantu 文字ab化け\n"
  "\\s Ukulunga okuku\\nd Kristu\\nd* אבabגד kuyinzuzo אבגד ab";
  string output = hyphenate_at_transition (firstset, secondset, input);
  string standard =
  "\\s \\nd UNku­lu­nku­lu\\nd* u\\add ba\\add*­xwa­yi­sa ngo­ku­lu­nga oku­ngo­kwa­ba­ntu 文字a­b化け\n"
  "\\s Uku­lu­nga oku­ku\\nd Kri­stu\\nd* אבa­bגד ku­yi­nzu­zo אבגד ab";
  evaluate (__LINE__, __func__, standard, output);
}