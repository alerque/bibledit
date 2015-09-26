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


#include <sword/logic.h>
#include <webserver/request.h>
#include <filter/string.h>
#include <filter/url.h>
#include <filter/archive.h>
#include <filter/shell.h>
#include <locale/translate.h>
#include <config/logic.h>
#include <client/logic.h>
#include <database/logs.h>
#include <database/books.h>
#include <database/cache.h>
#include <database/config/general.h>
#include <sync/resources.h>


string sword_logic_get_path ()
{
  return filter_url_create_root_path ("sword");
}


void sword_logic_refresh_module_list ()
{
  Database_Logs::log ("Refreshing SWORD module list");
  
  string out_err;
  vector <string> lines;
  
  // Initialize SWORD directory structure and configuration.
  string sword_path = sword_logic_get_path ();
  filter_url_mkdir (sword_path);
  string swordconf = "[Install]\n"
  "DataPath=" + sword_path + "/\n";
  filter_url_file_put_contents (filter_url_create_path (sword_path, "sword.conf"), swordconf);
  
  // Initialize basic user configuration
  filter_shell_run ("cd " + sword_path + "; echo yes | installmgr -init", out_err);
  lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    Database_Logs::log (line);
  }
  
  // Sync the configuration with the online known remote repository list.
  filter_shell_run ("cd " + sword_path + "; echo yes | installmgr -sc", out_err);
  filter_string_replace_between (out_err, "WARNING", "enable? [no]", "");
  lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    Database_Logs::log (line);
  }
  
  // List the remote sources.
  vector <string> remote_sources;
  filter_shell_run ("cd " + sword_path + "; installmgr -s", out_err);
  lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    Database_Logs::log (line);
    line = filter_string_trim (line);
    if (line.find ("[") != string::npos) {
      line.erase (0, 1);
      if (line.find ("]") != string::npos) {
        line.erase (line.length () - 1, 1);
        remote_sources.push_back (line);
      }
    }
  }
  
  vector <string> sword_modules;
  
  for (auto remote_source : remote_sources) {
    
    Database_Logs::log ("Reading modules from remote resource: " + remote_source);
    
    filter_shell_run ("cd " + sword_path + "; echo yes | installmgr -r \"" + remote_source + "\"", out_err);
    filter_string_replace_between (out_err, "WARNING", "type yes at the prompt", "");
    Database_Logs::log (out_err);
    
    filter_shell_run ("cd " + sword_path + "; installmgr -rl \"" + remote_source + "\"", out_err);
    lines = filter_string_explode (out_err, '\n');
    for (auto line : lines) {
      line = filter_string_trim (line);
      if (line.empty ()) continue;
      Database_Logs::log (line);
      if (line.find ("[") == string::npos) continue;
      if (line.find ("]") == string::npos) continue;
      sword_modules.push_back ("[" + remote_source + "]" + " " + line);
    }
    
  }
  
  // Store the list of remote sources and their modules.
  // It is stored in the client files area.
  // Clients can access it from there too.
  string path = sword_logic_module_list_path ();
  filter_url_file_put_contents (path, filter_string_implode (sword_modules, "\n"));
  
  Database_Logs::log ("Ready refreshing SWORD module list");
}


string sword_logic_module_list_path ()
{
  return filter_url_create_root_path ("databases", "client", "sword_modules.txt");
}


// Gets the name of the remote source of the $line like this:
// [CrossWire] *[Shona] (1.1) - Shona Bible
string sword_logic_get_source (string line)
{
  if (line.length () > 10) {
    line.erase (0, 1);
    size_t pos = line.find ("]");
    if (pos != string::npos) line.erase (pos);
  }
  return line;
}


// Gets the module name of the $line like this:
// [CrossWire] *[Shona] (1.1) - Shona Bible
string sword_logic_get_remote_module (string line)
{
  if (line.length () > 10) {
    line.erase (0, 2);
  }
  if (line.length () > 10) {
    size_t pos = line.find ("[");
    if (pos != string::npos) line.erase (0, pos + 1);
    pos = line.find ("]");
    if (pos != string::npos) line.erase (pos);
  }
  return line;
}


// Gets the module name of the $line like this:
// [Shona]  (1.1)  - Shona Bible
string sword_logic_get_installed_module (string line)
{
  line = filter_string_trim (line);
  if (line.length () > 10) {
    line.erase (0, 1);
    size_t pos = line.find ("]");
    if (pos != string::npos) line.erase (pos);
  }
  return line;
}


// Gets the version number of a module of the $line like this:
// [Shona]  (1.1)  - Shona Bible
string sword_logic_get_version (string line)
{
  line = filter_string_trim (line);
  if (line.length () > 10) {
    line.erase (0, 3);
  }
  if (line.length () > 10) {
    size_t pos = line.find ("(");
    if (pos != string::npos) line.erase (0, pos + 1);
    pos = line.find (")");
    if (pos != string::npos) line.erase (pos);
  }
  return line;
}


// Gets the human-readable name of a $line like this:
// [CrossWire] *[Shona] (1.1) - Shona Bible
string sword_logic_get_name (string line)
{
  vector <string> bits = filter_string_explode (line, '-');
  if (bits.size () >= 2) {
    bits.erase (bits.begin ());
  }
  line = filter_string_implode (bits, "-");
  line = filter_string_trim (line);
  return line;
}


void sword_logic_install_module (string source, string module)
{
  Database_Logs::log ("Install SWORD module " + module + " from source " + source);
  string out_err;
  string sword_path = sword_logic_get_path ();
  filter_shell_run ("cd " + sword_path + "; echo yes | installmgr -ri \"" + source + "\" \"" + module + "\"", out_err);
  vector <string> lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    line = filter_string_trim (line);
    if (line.empty ()) continue;
    Database_Logs::log (line);
  }
  // List this resource in the cache as being accessed,
  // so it won't disappear straight away next time the caches are trimmed.
  Database_Cache::create (module);
  Database_Cache::cache (module, 0, 0, 0, "installed");
}


void sword_logic_update_module (string source, string module)
{
  sword_logic_uninstall_module (module);
  sword_logic_install_module (source, module);
}


void sword_logic_uninstall_module (string module)
{
  Database_Logs::log ("Uninstall SWORD module " + module);
  string out_err;
  string sword_path = sword_logic_get_path ();
  filter_shell_run ("cd " + sword_path + "; installmgr -u \"" + module + "\"", out_err);
  vector <string> lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    line = filter_string_trim (line);
    if (line.empty ()) continue;
    Database_Logs::log (line);
  }
}


// Get available SWORD modules.
vector <string> sword_logic_get_available ()
{
  string contents = filter_url_file_get_contents (sword_logic_module_list_path ());
  return filter_string_explode (contents, '\n');
}


// Get installed SWORD modules.
vector <string> sword_logic_get_installed ()
{
  vector <string> modules;
  string out_err;
  string sword_path = sword_logic_get_path ();
  filter_shell_run ("cd " + sword_path + "; installmgr -l", out_err);
  vector <string> lines = filter_string_explode (out_err, '\n');
  for (auto line : lines) {
    line = filter_string_trim (line);
    if (line.empty ()) continue;
    if (line.find ("[") == string::npos) continue;
    modules.push_back (line);
  }
  return modules;
}


string sword_logic_get_text (string source, string module, int book, int chapter, int verse, bool redo)
{
  if (config_logic_client_prepared ()) {

    // Client checks for and optionally creates the cache for this SWORD module.
    if (!Database_Cache::exists (module)) {
      Database_Cache::create (module);
    }

    // If this module/passage exists in the cache, return it (it updates the access days in the cache).
    if (Database_Cache::exists (module, book, chapter, verse)) {
      return Database_Cache::retrieve (module, book, chapter, verse);
    }

    // Fetch this SWORD resource from the server.
    string address = Database_Config_General::getServerAddress ();
    int port = Database_Config_General::getServerPort ();
    string url = client_logic_url (address, port, sync_resources_url ());
    string resource = "[" + source + "][" + module + "]";
    url = filter_url_build_http_query (url, "r", resource);
    url = filter_url_build_http_query (url, "b", convert_to_string (book));
    url = filter_url_build_http_query (url, "c", convert_to_string (chapter));
    url = filter_url_build_http_query (url, "v", convert_to_string (verse));
    string error;
    string html = filter_url_http_get (url, error);
    
    // In case of an error, don't cache that error, but let the user see it.
    if (!error.empty ()) return error;

    // Client caches this info for later.
    Database_Cache::cache (module, book, chapter, verse, html);
    
    // Client triggers a cache of the entire SWORD module.
    sword_logic_trigger_cache (source, module);
    
    return html;
    
  } else {
    
    // The server fetches the module text as follows:
    // diatheke -b KJV -k Jn 3:16
    string output;
    string sword_path = sword_logic_get_path ();
    string osis = Database_Books::getOsisFromId (book);
    string command = "cd " + sword_path + "; diatheke -b " + module + " -k " + osis + " " + convert_to_string (chapter) + ":" + convert_to_string (verse);
    filter_shell_run (command, output);
    
    // If the module has not been installed, the output of "diatheke" will be empty.
    // If the module was installed, but the requested passage is out of range,
    // the output of "diatheke" contains the module name, so it won't be empty.
    if (output.empty () && !redo) {
      // Install the module and redo getting the passage text.
      sword_logic_install_module (source, module);
      string text = sword_logic_get_text (source, module, book, chapter, verse, true);
      return text;
    }
    
    // The server hits the cache for recording the last day it was accessed.
    // It hits passage 0.0.0 because the installed SWORD module is one data unit.
    if (Database_Cache::retrieve (module, 0, 0, 0).empty ()) {
      Database_Cache::create (module);
      Database_Cache::cache (module, 0, 0, 0, "accessed");
    }
    
    // The standard output of a Bible verse starts with the passage, like so:
    // Ruth 1:2:
    // Remove that.
    size_t pos = output.find (":");
    if (pos != string::npos) {
      output.erase (0, pos + 1);
    }
    pos = output.find (":");
    if (pos != string::npos) {
      output.erase (0, pos + 1);
    }
    
    // The standard output ends with the module name, like so:
    // (KJV)
    // Remove that.
    output = filter_string_str_replace ("(" + module + ")", "", output);
    
    // Remove any OSIS elements.
    filter_string_replace_between (output, "<", ">", "");
    
    // Clean whitespace away.
    output = filter_string_trim (output);
    
    return output;
  }

  return "";
}


// Checks the installed modules, whether they need to be updated.
void sword_logic_update_installed_modules ()
{
  Database_Logs::log ("Updating installed SWORD modules");

  vector <string> available_modules = sword_logic_get_available ();

  vector <string> installed_modules = sword_logic_get_installed ();
  for (auto & installed_module : installed_modules) {
    string module = sword_logic_get_installed_module (installed_module);
    string installed_version = sword_logic_get_version (installed_module);
    for (auto & available_module : available_modules) {
      if (sword_logic_get_remote_module (available_module) == module) {
        if (sword_logic_get_version (available_module) != installed_version) {
          string source = sword_logic_get_source (available_module);
          sword_logic_update_module (source, module);
        }
        continue;
      }
    }
  }
  
  Database_Logs::log ("Ready updating installed SWORD modules");
}


// Trims the installed SWORD modules.
void sword_logic_trim_modules ()
{
  if (!config_logic_client_prepared ()) {
    Database_Logs::log ("Trimming the SWORD caches and modules");
    vector <string> modules = sword_logic_get_installed ();
    for (auto module : modules) {
      module = sword_logic_get_installed_module (module);
      if (!Database_Cache::exists (module)) {
        sword_logic_uninstall_module (module);
      }
    }
    Database_Logs::log ("Ready trimming the SWORD caches and modules");
  }
}


// Trigger caching the $module from $source.
void sword_logic_trigger_cache (string source, string module)
{
  // The resource consists of the $source and the $module.
  string resource = "[" + source + "][" + module + "]";
  // Add it to the general configuration to be cached, if it is not already there.
  vector <string> resources = Database_Config_General::getResourcesToCache ();
  if (!in_array (resource, resources)) {
    resources.push_back (resource);
    Database_Config_General::setResourcesToCache (resources);
  }
}