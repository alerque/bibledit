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


page_access_level (Filter_Roles::manager ());


Assets_Page::header (translate("Hyphenation"));
$view = new Assets_View (__FILE__);


$database_config_user = Database_Config_User::getInstance ();
$database_config_bible = Database_Config_Bible::getInstance ();
$database_bibles = Database_Bibles::getInstance ();
$session_logic = Session_Logic::getInstance ();


$bible = access_bible_clamp (request->database_config_user()->getBible ());


$success = "";
$error = "";


// Character sets submission.
if (isset(request->post['sets'])) {
  $firstset = request->post['firstset'];
  Database_Config_Bible::setHyphenationFirstSet ($bible, $firstset);
  $secondset = request->post['secondset'];
  Database_Config_Bible::setHyphenationSecondSet ($bible, $secondset);
  $success = translate("The two sets of characters were saved");
}
$firstset = Database_Config_Bible::getHyphenationFirstSet ($bible);
$secondset = Database_Config_Bible::getHyphenationSecondSet ($bible);


@$bible = request->query ['bible'];
if (isset ($bible)) {
  if ($bible == "") {
    $dialog_list = new Dialog_List2 (translate("Which Bible would you like to take the data from?"));
    $bibles = access_bible_bibles ();
    for ($bibles as $item) {
      $dialog_list->add_row ($item, "bible=$item");
    }
    $dialog_list->run();
  } else {
    request->database_config_user()->setBible ($bible);
  }
}
$bible = access_bible_clamp (request->database_config_user()->getBible ());


if (request->query.count ('run'])) {
  if ($bible == "") {
    $error = translate("No Bible given");
  } else if ($firstset == "") {
    $error = translate("No first set of characters given");
  } else if ($secondset == "") {
    $error = translate("No second set of characters given");
  } else {
    $workingdirectory = __DIR__;
    tasks_logic_queue (Tasks_Logic::PHP, array ("$workingdirectory/hyphenate", $bible, request->session_logic()->currentUser ()));
    $success = translate("The Bible is being hyphenated. See the Journal for details.");
  }
}


$view.set_variable ("firstset = $firstset;
$view.set_variable ("secondset = $secondset;
$view.set_variable ("bible = $bible;
$view.set_variable ("success = $success;
$view.set_variable ("error = $error;
$view->render ("hyphenation");


Assets_Page::footer ();


?>
