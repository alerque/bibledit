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


$username = Filter_Hex::hex2bin (request->post ['u']);
$password = request->post ['p'];
$bible = request->post ['bi'];
$book = request->post ['bo'];
$chapter = request->post ['ch'];
$oldusfm = request->post ['o'];
$newusfm = request->post ['n'];
$checksum = request->post ['s'];


$database_users = Database_Users::getInstance (); 
$database_logs = Database_Logs::getInstance ();
$database_bibles = Database_Bibles::getInstance ();
$database_books = Database_Books::getInstance ();
$session_logic = Session_Logic::getInstance ();
$database_mail = Database_Mail::getInstance ();


$user_ok = request->database_users ()->usernameExists ($username);
if (!$user_ok) Database_Logs::log ("Non existing user $username", Filter_Roles::manager ());
$pass_ok = ($password == request->database_users ()->getmd5 ($username));
if (!$pass_ok) Database_Logs::log ("Incorrect password $password for user $username", Filter_Roles::manager ());
if (!$user_ok || !$pass_ok) {
  // Unauthorized.
  request->response_code = 401); 
  die;
}


request->session_logic ()->setUsername ($username);


$bookname = Database_Books::getEnglishFromId ($book);


Database_Logs::log ("Client sent Bible data: $bible $bookname $chapter", Filter_Roles::manager ());


// Check whether the user has write-access to the Bible.
if (!access_bible_write ($bible, $username)) {
  $message = "User $username does not have write access to Bible $bible";
  Database_Logs::log ($message, Filter_Roles::manager ());
  echo $message;
  die;
}


// Check checksum.
if ($checksum != Checksum_Logic::get ($oldusfm . $newusfm)) {
  $message = "The received data is corrupted";
  Database_Logs::log ($message, Filter_Roles::manager ());
  echo $message;
  die;
}


$serverusfm = request->database_bibles()->getChapter (bible, book, chapter);


// Gather data for recording the changes made by the user, for the change notifications.
$old_id = request->database_bibles()->getChapterId (bible, book, chapter);
$old_text = $serverusfm;
$new_text = $newusfm;


if ($serverusfm == "") {
  // If the chapter on the server is still empty, then just store the client's version on the server, and that's it.
  Bible_Logic::storeChapter (bible, book, chapter, $newusfm);
} else if ($newusfm != $serverusfm) {
  // Do a merge in case the client sends USFM that differs from what's on the server.
  $mergedusfm = filter_merge_run ($oldusfm, $newusfm, $serverusfm);
  if ($mergedusfm == $serverusfm) {
    // When the merged USFM is the same as what's already on the server, then it means there was a merge conflict.
    // Email the user with the details, so the user can resolve the conflicts.
    $subject = "Problem sending chapter to server";
    $body = "<p>While sending $bible $bookname $chapter to the server, the server didn't manage to merge it.</p>";
    $body += "<p>Please re-enter your changes as you see fit.</p>";
    $body += "<p>Here is the chapter you sent to the server:</p>";
    $body += "<pre>$newusfm</pre>";
    $database_mail->send ($username, $subject, $body);
  } else {
    // Update the server with the new chapter data.
    Bible_Logic::storeChapter (bible, book, chapter, $mergedusfm);
  }
}


// If text was saved, record it as a change entered by the user.
$new_id = request->database_bibles()->getChapterId (bible, book, chapter);
if ($new_id != $old_id) {
  $database_modifications = Database_Modifications::getInstance ();
  $database_modifications->recordUserSave ($username, bible, book, chapter, $old_id, $old_text, $new_id, $new_text);
}


// Send the updated chapter back to the client.
// Do this only in case the updated chapter USFM is different from the new USFM the client sent.
// This means that in most cases, nothing will be sent back.
// That saves bandwidth.
// And it allows the user on the client to keep editing without the returned chapter to overwrite the changes the user made.
$serverusfm = request->database_bibles()->getChapter (bible, book, chapter);
if ($serverusfm != $newusfm) {
  $checksum = Checksum_Logic::get ($serverusfm);
  echo "$checksum\n$serverusfm";
}


?>
