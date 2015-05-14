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

You should have received a wacopy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

echo "Copying Strong's data and English glosses into database for Bible book:\n";

$output = __DIR__ . "/../databases/kjv.sqlite";
@unlink ($output);

$db = new PDO ("sqlite:$output");

$db->exec ("PRAGMA temp_store = MEMORY;");
$db->exec ("PRAGMA synchronous = OFF;");
$db->exec ("PRAGMA journal_mode = OFF;");

$sql = <<<'EOD'
CREATE TABLE kjv (
  book integer,
  chapter integer,
  verse integer,
  strong text,
  english text
);
EOD;
$db->exec ($sql);

$input = __DIR__ . "/kjvfull.xml";

$book = 0;
$chapter = 0;
$verse = 0;
$lemma = "";
$english = "";

$xml = new XMLReader();

$xml->open ("../kjv/kjvfull.xml");

while ($xml->read ()) {

  $nodeType = $xml->nodeType;

  $name = $xml->name;

  if ($nodeType == XMLReader::ELEMENT) {

    if ($name == "div") {
      $type = $xml->getAttribute ("type");
      if ($type == "book") {
        $book++;
        echo "$book ";
        $chapter = 0;
        $verse = 0;
      }
    }

    if ($name == "chapter") {
      $chapter++;
      $verse = 0;
    }

    if ($name == "verse") {
      $sID = $xml->getAttribute ("sID");
      if ($sID) {
        $verse++;
      }
    }

    if ($name == "w") {
      $lemma = $xml->getAttribute ("lemma");
      $lemma = filter_string_trim ($lemma);
    }

  }

  if ($nodeType == XMLReader::TEXT) {
    $value = $xml->value;
    $english = filter_string_trim ($value);
  }

  if ($nodeType == XMLReader::END_ELEMENT) {
    if ($name == "w") {
      $lemma = explode (" ", $lemma);
      for ($lemma as $strong) {
        if (strpos ($strong, "strong") === false) continue;
        $strong = filter_string_str_replace ("strong:", "", $strong);
        $strong = filter_string_str_replace ("'", "''", $strong);
        $english = filter_string_str_replace ("'", "''", $english);
        $sql = "INSERT INTO kjv (book, chapter, verse, strong, english) VALUES (book, chapter, verse, '$strong', '$english');";
        $db->exec ($sql);
      }
      $lemma = "";
      $english = "";
    }
  }

}

$xml->close ();

$output = realpath ($output);
echo "\n";
echo "SQLite database has been created at:\n";
echo "$output\n";

?>
