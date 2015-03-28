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


#include <checks/settingspatterns.h>
#include <assets/view.h>
#include <assets/page.h>
#include <filter/roles.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <database/config/bible.h>
#include <access/bible.h>


string checks_settingspatterns_url ()
{
  return "checks/settingspatterns";
}


bool checks_settingspatterns_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::manager ());
}


string checks_settingspatterns (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  
  string page;
  page = Assets_Page::header (translate ("Patterns"), webserver_request, "");
  Assets_View view = Assets_View ();
  
  
  string bible = access_bible_clamp (webserver_request, request->database_config_user()->getBible ());
  
  
  if (request->post.count ("patterns")) {
    string patterns = request->post ["patterns"];
    if (!bible.empty ()) Database_Config_Bible::setCheckingPatterns (bible, patterns);
    view.set_variable ("success", translate("The patterns were saved"));
  }
  
  
  view.set_variable ("bible", bible);
  view.set_variable ("patterns", Database_Config_Bible::getCheckingPatterns (bible));
                                          
                                          
  page += view.render ("checks", "settingspatterns");
  page += Assets_Page::footer ();
  return page;
}
