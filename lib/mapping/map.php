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


Assets_Page::header (gettext("Verse mappings"));
$view = new Assets_View (__FILE__);


$database_mappings = Database_Mappings::getInstance ();
$session_logic = Session_Logic::getInstance ();


$username = request->session_logic()->currentUser ();
$userlevel = request->session_logic ()->currentLevel ();


$name = request->query['name'];
$view.set_variable ("name = filter_string_sanitize_html ($name);


//$write = request->database_styles()->hasWriteAccess ($username, $name);
$write = true;
if ($userlevel >= Filter_Roles::admin ()) $write = true;


if (isset(request->post['submit'])) {
  $data = request->post['data'];
  if ($write) {
    $database_mappings->import ($name, $data);
  }
}


$data = $database_mappings->export ($name);
$view.set_variable ("data = $data;


$view->render ("map");


Assets_Page::footer ();


?>
