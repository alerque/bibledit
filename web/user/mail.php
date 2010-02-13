<?php

require_once ("../bootstrap/bootstrap.php");
page_access_level (MEMBER_LEVEL);

$smarty = new Smarty_Bibledit (__FILE__);
$database_mail = Database_Mail::getInstance();

if ($_GET['delete'] != "") {
  $database_mail->delete ($_GET['delete']);
}

if ($_GET['view'] != "") {
  $result  = $database_mail->get ($_GET['view']);
  $row     = $result->fetch_assoc();
  $subject = $row['subject'];
  $subject = strip_tags ($subject);
  $body    = $row['body'];
  $body    = strip_tags ($body, '<p>');
  $smarty->assign ("subject", $subject);
  $smarty->assign ("body",    $body);
}

$active_label = $_GET['label'];
if ($active_label == "") $active_label = $database_mail->labelInbox ();
$smarty->assign ("active_label", $active_label);

$label_inbox   = $database_mail->labelInbox ();
$link_inbox    = "mail.php?label=$label_inbox";
$smarty->assign ("link_inbox", $link_inbox);

$label_trash   = $database_mail->labelTrash ();
$link_trash    = "mail.php?label=$label_trash";
$smarty->assign ("link_trash", $link_trash);

$mails = $database_mail->getMails ($active_label);
while ($row = $mails->fetch_assoc()) {
  $id             = $row["id"];
  $ids         [] = $id;
  $time           = date ('j F Y, g:i a', $row["timestamp"]);
  $timestamps  [] = $time;
  $subjects    [] = $row["subject"];
  $deletes     [] = "mail.php?label=$active_label&delete=$id";
  $views       [] = "mail.php?label=$active_label&view=$id";
}
$smarty->assign ("ids",          $ids);
$smarty->assign ("timestamps",   $timestamps);
$smarty->assign ("subjects",     $subjects);
$smarty->assign ("deletes",      $deletes);
$smarty->assign ("views",        $views);

$smarty->display ("mail.tpl");

/*

Todo site mailer.



The cron daemon sends out any mails it finds.
If an exception occurs, a flag is set in a table so the retry mechanism can work from there.
If an exception occurs, the site admin gets mail that there was a problem, with the description of the exception.




Users that sign up should provide a valid email address.
A confirmation is then sent to that address.
If the user replies to it, or clicks on the link it contains, the account is then confirmed.
This means that the users table should have an 'active' field as well, or possibly 'status'.
The unique id for that user comes from the md5 of the username, so we can see which user confirms himself.
Once it all is set up we should also give the option to log in by email address.
And the link for a forgotten password should be made to work as well.



The system for signing up works thus:
* User signs up.
* Confirmation mail is sent out to the user's email address
* The confirmation mail contains a md5 value generated from the input data.
* A SQL query is stored in the database, with this unique value.
* If any mail comes back with this unique value, the SQL query in the database is executed
* But this may not be enough, since it needs to send a confirmation mail too.


mailer.
Steps: 
* the site also has a page, but differently, since it processes mail upon arrival - yes, it stores outgoing mail
* The site mailer's errors go to the administrator's email box.
* Mail should be removed from the Trash after 30 days.




*/

?>
