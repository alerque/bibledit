/*
 Copyright (©) 2003-2016 Teus Benschop.
 
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


#include <resource/download.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/url.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <resource/logic.h>
#include <database/offlineresources.h>
#include <database/usfmresources.h>
#include <database/logs.h>
#include <database/cache.h>
#include <database/config/general.h>
#include <tasks/logic.h>
#include <journal/index.h>
#include <menu/logic.h>
#include <resource/cache.h>


string resource_download_url ()
{
  return "resource/download";
}


bool resource_download_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::manager ());
}


string resource_download (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Database_OfflineResources database_offlineresources;
  Database_UsfmResources database_usfmresources;
  
  
  string page;
  Assets_Header header = Assets_Header (translate("Download resource"), request);
  header.addBreadCrumb (menu_logic_settings_menu (), menu_logic_settings_text ());
  header.addBreadCrumb (resource_cache_url (), menu_logic_resources_text ());
  page = header.run ();
  Assets_View view;

  
  string name = request->query["name"];
  view.set_variable ("name", name);
  
  
  
  bool old = request->query.count ("old");
  if (old) view.enable_zone ("old");
  else view.enable_zone ("current");
                        
                      
  if (request->query.count ("clear")) {
    // The client clears the two older storage locations just to be sure they are gone.
    database_offlineresources.erase (name);
    database_usfmresources.deleteResource (name);
    // The client clears the installed resource.
    Database_Cache::remove (name);
  }
  if (request->query.count ("clearold")) {
    // The client clears the two older storage locations just to be sure they are gone.
    database_offlineresources.erase (name);
    database_usfmresources.deleteResource (name);
    redirect_browser (request, resource_cache_url ());
    return "";
  }
  
  
  if (request->query.count ("download")) {
    // Trigger caching the resource.
    // Add the resource to the general configuration to be cached, if it is not already there.
    vector <string> resources = Database_Config_General::getResourcesToCache ();
    if (!in_array (name, resources)) {
      resources.push_back (name);
      Database_Config_General::setResourcesToCache (resources);
    }
    tasks_logic_queue (SYNCRESOURCES);
    redirect_browser (request, journal_index_url ());
    return "";
  }
  
  
  int count = 0;
  if (count == 0) count = database_offlineresources.count (name);
  if (count == 0) {
    vector <int> books = database_usfmresources.getBooks (name);
    for (auto book : books) {
      vector <int> chapters = database_usfmresources.getChapters (name, book);
      // Rough average of 20 verses per chapter.
      count += (20 * chapters.size());
    }
  }
  if (count == 0) count = Database_Cache::count (name);
  view.set_variable ("count", convert_to_string (count));
                      
                      
  page += view.render ("resource", "download");
  page += Assets_Page::footer ();
  return page;
}
