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


#include <sync/settings.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/bibles.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>


string sync_settings_url ()
{
  return "sync/settings";
}


bool sync_settings_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string sync_settings (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Sync_Logic sync_logic = Sync_Logic (webserver_request);

  // Check on the credentials.
  if (!sync_logic.credentials_okay ()) return "";
  
  // Client makes a prioritized server call: Record the client's IP address.
  sync_logic.prioritized_ip_address_record ();

  // Get the relevant parameters the client POSTed to us, the server.
  int action = convert_to_int (request->post ["a"]);
  string value = request->post ["v"];
  // The value can be all Bibles, or one Bible.
  string bible_s = request->post ["b"];

  switch (action) {
    case Sync_Logic::settings_get_total_checksum:
    {
      return sync_logic.settings_checksum (filter_string_explode (bible_s, '\n'));
    }
    case Sync_Logic::settings_send_workbench_urls:
    {
      request->database_config_user()->setWorkbenchURLs (value);
      return "";
    }
    case Sync_Logic::settings_get_workbench_urls:
    {
      return request->database_config_user()->getWorkbenchURLs ();
    }
    case Sync_Logic::settings_send_workbench_widths:
    {
      request->database_config_user()->setWorkbenchWidths (value);
      return "";
    }
    case Sync_Logic::settings_get_workbench_widths:
    {
      return request->database_config_user()->getWorkbenchWidths ();
    }
    case Sync_Logic::settings_send_workbench_heights:
    {
      request->database_config_user()->setWorkbenchHeights (value);
      return "";
    }
    case Sync_Logic::settings_get_workbench_heights:
    {
      return request->database_config_user()->getWorkbenchHeights ();
    }
    case Sync_Logic::settings_send_resources_organization:
    {
      vector <string> resources = filter_string_explode (value, '\n');
      request->database_config_user()->setActiveResources (resources);
      return "";
    }
    case Sync_Logic::settings_get_resources_organization:
    {
      vector <string> resources = request->database_config_user()->getActiveResources ();
      return filter_string_implode (resources, "\n");
    }
    case Sync_Logic::settings_get_bible_id:
    {
      return convert_to_string (request->database_bibles()->getID (bible_s));
    }
    case Sync_Logic::settings_get_bible_font:
    {
      return Database_Config_Bible::getTextFont (bible_s);
    }
  }

  // Bad request.
  // Delay a while to obstruct a flood of bad requests.
  this_thread::sleep_for (chrono::seconds (1));
  request->response_code = 400;
  return "";
}