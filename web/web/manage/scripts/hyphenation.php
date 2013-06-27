<h1><?php echo gettext ("Hyphenation") ?></h1>
<p><?php echo gettext ("Hyphenation can be used when preparing a Bible for printing.") ?></p>
<p><?php echo gettext ("It allows for more efficient usage of paper, and for more regular appearance of margins at the end of the lines without requiring spacing adjustments.") ?></p>
<p><?php echo gettext ("The details of hyphenating properly are complex and dependent on the language, and can interact with other orthographic and typesetting practices.") ?></p>
<p><?php echo gettext ("Hyphenation may not be so important when preparing a Bible for proofreading. It is usually regarded important for production quality Bibles.") ?></p>
<br>
<p><?php echo gettext ("When preparing a Bible in LibreOffice, OpenOffice, AbiWord, en so on, the wordprocessor itself provides hyphenation algorithms for many languages.") ?></p>
<p><?php echo gettext ("If no suitable hyphenation algorithm can be used, perhaps Bibledit-Web can assist to a limited degree.") ?></p>
<br>
<p><?php echo gettext ("Bibledit-Web can take the data from a Bible, insert soft hyphens in the words according to a certain algorithm, and then save this data as a new Bible.") ?></p>
<p><?php echo gettext ("This new Bible can then be exported to OpenDocument format, and opened in a wordprocessor.") ?></p>
<p><?php echo gettext ("The wordprocessor sees these soft hyphens. When a soft hyphen occurs near the end of the line, it breaks the word there.") ?></p>
<br>
<p><?php echo gettext ("Bibledit-Web has one hyphenating algorithm. When there is a transition in the text from any character of a first set of characters, to any character of a second set of characters, then the soft hyphen will be inserted at that location in the text.") ?></p>
<br>
<a name="sets"></a>
<form action="hyphenation.php?bible=<?php echo $this->bible ?>#sets" method="post">
<p><?php echo gettext ("First set of characters") ?>:</p>
<p><input type="text" name="firstset" value="<?php echo $this->firstset ?>" class="fullwidth" /></p>
<p><?php echo gettext ("Second set of characters") ?>:</p>
<p><input type="text" name="secondset" value="<?php echo $this->secondset ?>" class="fullwidth" /></p>
<p><?php echo gettext ("Enter the sets of characters, and press Save.") ?></p>
<p><input type="submit" name="sets" value=<?php echo gettext ("Save") ?> /></p>
</form>
<br>
<p><?php echo gettext ("The Bible where the data will be taken from is:") ?> <?php echo $this->bible ?> <a href="hyphenation.php?bible={$bible}&changebible=">[<?php echo gettext ("change") ?>]</a></p>
<?php if ($this->bible != "") { ?>
<p><?php echo gettext ("The Bible where the hyphenated data will be written to is:") ?> <?php echo $this->bible ?>-hyphenated</p>
<?php } ?>
<br>
<p><a href="hyphenation.php?bible=<?php echo $this->bible ?>&run="><?php echo gettext ("Insert the soft hyphens according to the settings.") ?></a></p>
<p class="error"><?php echo $this->error ?></p>
<p class="success"><?php echo $this->success ?></p>
