<h1>{t}Members{/t}</h1>
<p><a href="../downloads/changes">{t}Daily changes in the Bibles{/t}</a></p>
<p><a href="../downloads/exports">{t}Exported Bibles{/t}</a></p>
{foreach key=key item=item from=$plugins} 
  <p><a href={$key}>{$item}</a></p>
{/foreach} 
