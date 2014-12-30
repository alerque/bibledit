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
?>
<p>
  gettext("Resources")
  <a href="organize.php">[gettext("organize")]</a>
</p>
<hr>
<?php for ($this->resources as $offset => $resource) {
  <div id="line$offset + 1" style="clear:both">
  <span id="name$offset + 1" class="small">$resource</span>
  <span id="loading$offset + 1"><img src="../pix/loading.gif"></span>
  <span id="content$offset + 1"></span>
  <hr style="clear:both">
  </div>
<?php }
<script>$this->script;</script>
<script type="text/javascript" src="index.js?config_logic_version ()"></script>
<script type="text/javascript" src="../navigation/nav.js?config_logic_version ()"></script>
