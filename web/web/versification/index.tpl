<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE HTML PUBLIC "-//W3C//DTD HTML 4.01 Transitional//EN" "http://www.w3.org/TR/html4/loose.dtd" >
<html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <meta http-equiv="content-type" content="text/html; charset=UTF-8"></meta>
    <title>{t}Versifications{/t}</title>
    <link rel="stylesheet" type="text/css" href="../css/stylesheet.css"></link>
  </head>
  <body>
    {include file=../assets/header_full.tpl} 
    <h1>{t}Versifications{/t}</h1>
    <p>{t}This lists the currently available versification systems.{/t} <a href="index.php?new=">{t}You can create a new versification system.{/t}</a></p>
    {section name=offset loop=$systems} 
      <p><a href="system.php?name={$systems[offset]}">{$systems[offset]}</a></p>
    {/section} 
