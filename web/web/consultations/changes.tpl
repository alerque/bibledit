<h1>{t}Changes{/t}</h1>
<p>
  {t}This lists the changes in the text of the Bibles.{/t}
  {t}Click ✔ to approve a change.{/t}
  {t}Each entry shows the number of Consultation Notes for the verse, and the ones you subscribed to or have been assigned to you.{/t}
</p>
<p class="error">{$error}</p>
<p class="success">{$success}</p>
<table>
  {section name=offset loop=$ids} 
  <tr>
    <td><a href="changes.php?approve={$ids[offset]}"> ✔ </a></td>
    <td><a href="changes.php?goto={$ids[offset]}" target="_blank">{$passages[offset]}</a></td>
    <td><a href="change.php?id={$ids[offset]}">{$modifications[offset]}</a></td>
    <td>{$totalNotesCount[offset]}</td>
    <td>{$yourNotesCount[offset]}</td>
  </tr>
  {/section} 
</table>
