<?php
require_once ("../bootstrap/bootstrap.php");
page_access_level (MANAGER_LEVEL);
Assets_Page::header (gettext ("Management"));
$view = new Assets_View (__FILE__);
$view->render ("index.php");
Assets_Page::footer ();
?>
