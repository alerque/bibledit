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
<h1>gettext("Take data in repository")</h1>
<p>gettext("Bible"): $this->object</p>
<p>gettext("Repository URL"): $this->url</p>
<p>gettext("The data in the remote repository is being copied over into Bibledit's data, overwriting the data that was there before.")</p>
<?php for ($this->contents as $line) {
  <p><code>$line</code></p>
<?php }
