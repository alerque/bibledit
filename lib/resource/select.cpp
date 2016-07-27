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


#include <resource/select.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/header.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/passage.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <resource/logic.h>
#include <resource/external.h>
#include <resource/sword.h>
#include <sync/logic.h>
#include <access/bible.h>
#include <client/logic.h>
#include <dialog/list.h>
#include <database/usfmresources.h>
#include <database/imageresources.h>
#include <lexicon/logic.h>
#include <sword/logic.h>
#include <access/logic.h>


string resource_select_url ()
{
  return "resource/select";
}


bool resource_select_acl (void * webserver_request)
{
  return access_logic_privilege_view_resources (webserver_request);
}


string resource_select (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  
  string page;
  Assets_Header header = Assets_Header (translate("Resources"), request);
  page = header.run ();
  Assets_View view; // TodojQuery

  
  view.set_variable ("page", resource_logic_selector_page (webserver_request));
  string caller = resource_logic_selector_caller (webserver_request);
  view.set_variable ("caller", caller);
  
  
  if (request->query.count ("bible")) {
    // Any resources added is POSTed because when it was still done through GET,
    // the crawlers of the search engines kept clicking links and so adding resources on the public demo.
    // The POST method is supposed to be immune to that.
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a Bible"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> bibles = access_bible_bibles (webserver_request);
    for (auto bible : bibles) {
      dialog_list.add_row (bible, "add", bible);
    }
    page += dialog_list.run();
    return page;
  }
  

  if (request->query.count ("usfm")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a USFM resource"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources;
#ifdef HAVE_CLIENT
    // Client takes resources available from the Cloud.
    resources = client_logic_usfm_resources_get ();
#else
    // Cloud takes its locally available USFM resources.
    Database_UsfmResources database_usfmresources;
    resources = database_usfmresources.getResources ();
#endif
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  

  if (request->query.count ("web_orig")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select an original language text"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = resource_external_get_original_language_resources ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("web_bibles")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a Bible translation"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = resource_external_get_bibles ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("web_commentaries")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a commentary"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = resource_external_get_commentaries ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("web_other")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a general resource"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = resource_external_get_general_resources ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("image")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select an image resource"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    Database_ImageResources database_imageresources;
    vector <string> resources = database_imageresources.names ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  

  if (request->query.count ("lexicon")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a lexicon resource"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = lexicon_logic_resource_names ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("sword")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a SWORD resource"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = sword_logic_get_available ();
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  
  if (request->query.count ("divider")) {
    Dialog_List dialog_list = Dialog_List (caller, translate("Select a divider"), "", "", true);
    dialog_list.add_query ("page", request->query["page"]);
    vector <string> resources = {
      resource_logic_yellow_divider (),
      resource_logic_green_divider (),
      resource_logic_blue_divider (),
      resource_logic_violet_divider (),
      resource_logic_red_divider (),
      resource_logic_orange_divider ()
    };
    for (auto resource : resources) {
      dialog_list.add_row (resource, "add", resource);
    }
    page += dialog_list.run();
    return page;
  }
  
  page += view.render ("resource", "select");
  page += Assets_Page::footer ();
  return page;
}
