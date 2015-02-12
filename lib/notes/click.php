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
page_access_level (Filter_Roles::consultant ());


@$open = request->query ['open'];
if (isset ($open)) {
  $open = intval (basename ($open));
  $database_notes = Database_Notes::getInstance ();
  if ($database_notes->identifierExists ($open)) {
    $ipc_notes = Ipc_Notes::getInstance ();
    $ipc_notes->open ($open);
  }
}


@$new = request->query ['new'];
if (isset ($new)) {
  $new = intval (basename ($new));
  $database_modifications = Database_Modifications::getInstance ();
  $database_bibles = Database_Bibles::getInstance ();
  $bible = $database_modifications->getNotificationBible ($new);
  $summary = translate("Query about a change in the text");
  $contents = "<p>" . translate("Old text:") . "</p>";
  $contents += $database_modifications->getNotificationOldText ($new);
  $contents += "<p>" .  translate("Change:") . "</p>";
  $contents += "<p>" . $database_modifications->getNotificationModification ($new) . "</p>";
  $contents += "<p>" . translate("New text:") . "</p>";
  $contents += $database_modifications->getNotificationNewText ($new);
  $passage = $database_modifications->getNotificationPassage ($new);
  $database_notes = Database_Notes::getInstance ();
  $notes_logic = Notes_Logic::getInstance();
  $identifier = $notes_logic->createNote ($bible, $passage ['book'], $passage ['chapter'], $passage ['verse'], $summary, $contents, false);
  $ipc_notes = Ipc_Notes::getInstance ();
  $ipc_notes->open ($identifier);
}


?>
