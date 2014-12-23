/*
 Copyright (©) 2003-2014 Teus Benschop.
 
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


#include <styles/view.h>
#include <styles/logic.h>
#include <assets/view.h>
#include <assets/page.h>
#include <dialog/entry.h>
#include <dialog/list.h>
#include <filter/roles.h>
#include <filter/url.h>
#include <filter/string.h>
#include <tasks/logic.h>
#include <webserver/request.h>
#include <journal/index.h>
#include <database/config/user.h>
#include <database/logs.h>
#include <access/user.h>
#include <locale/translate.h>


string styles_view_url ()
{
  return "styles/view";
}


bool styles_view_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::manager ());
}


string styles_view (void * webserver_request) // Todo
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  string page;
  
  page = Assets_Page::header (gettext ("Style"), webserver_request, "");
  
  Assets_View view = Assets_View ();


  Database_Styles database_styles = Database_Styles ();
  
  
  string sheet = request->query ["sheet"];
  view.set_variable ("sheet", filter_string_sanitize_html (sheet));
  
  
  string style = request->query ["style"];
  view.set_variable ("style", filter_string_sanitize_html (style));

  
  Database_Styles_Item marker_data = database_styles.getMarkerData (sheet, style);
  
  
  // Whether the logged-in user has write access to the stylesheet.
  string username = request->session_logic ()->currentUser ();
  int userlevel = request->session_logic ()->currentLevel ();
  bool write = database_styles.hasWriteAccess (username, sheet);
  if (userlevel >= Filter_Roles::admin ()) write = true;
  

  // The style's name.
  string name = marker_data.name;
  if (request->query.count ("name")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("view", gettext("Please enter the name for the style"), name, "name", "");
    dialog_entry.add_query ("sheet", sheet);
    dialog_entry.add_query ("style", style);
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count ("name")) {
    name = request->post["entry"];
    if (write) database_styles.updateName (sheet, style, name);
  }
  view.set_variable ("name", filter_string_sanitize_html (name));
  

  // The style's info.
  string info = marker_data.info;
  if (request->query.count ("info")) {
    Dialog_Entry dialog_entry = Dialog_Entry ("view", gettext("Please enter the description for the style"), info, "info", "");
    dialog_entry.add_query ("sheet", sheet);
    dialog_entry.add_query ("style", style);
    page += dialog_entry.run ();
    return page;
  }
  if (request->post.count("info")) {
    info = request->post["entry"];
    if (write) database_styles.updateInfo (sheet, style, info);
  }
  view.set_variable ("info", filter_string_sanitize_html (info));
  
  
  // The style's category.
  string category = marker_data.category;
  if (request->query.count("category")) {
    category = request->query["category"];
    if (category == "") {
      Dialog_List dialog_list = Dialog_List ("view", gettext("Would you like to change the category of this style?"),gettext("Here are the various categories:"), gettext("Please pick one."));
      dialog_list.add_query ("sheet", sheet);
      dialog_list.add_query ("style", style);
      dialog_list.add_row (styles_logic_category_text ("id"),   "&category=id");
      dialog_list.add_row (styles_logic_category_text ("ith"),  "&category=ith");
      dialog_list.add_row (styles_logic_category_text ("ipp"),  "&category=ipp");
      dialog_list.add_row (styles_logic_category_text ("ioe"),  "&category=ioe");
      dialog_list.add_row (styles_logic_category_text ("t"),    "&category=t");
      dialog_list.add_row (styles_logic_category_text ("h"),    "&category=h");
      dialog_list.add_row (styles_logic_category_text ("cv"),   "&category=cv");
      dialog_list.add_row (styles_logic_category_text ("p"),    "&category=p");
      dialog_list.add_row (styles_logic_category_text ("l"),    "&category=l");
      dialog_list.add_row (styles_logic_category_text ("pe"),   "&category=pe");
      dialog_list.add_row (styles_logic_category_text ("te"),   "&category=te");
      dialog_list.add_row (styles_logic_category_text ("f"),    "&category=f");
      dialog_list.add_row (styles_logic_category_text ("x"),    "&category=x");
      dialog_list.add_row (styles_logic_category_text ("xsn"),  "&category=xsn");
      dialog_list.add_row (styles_logic_category_text ("st"),   "&category=st");
      dialog_list.add_row (styles_logic_category_text ("cs"),   "&category=cs");
      dialog_list.add_row (styles_logic_category_text ("sb"),   "&category=sb");
      dialog_list.add_row (styles_logic_category_text ("sf"),   "&category=sf");
      dialog_list.add_row (styles_logic_category_text ("pm"),   "&category=pm");
      dialog_list.add_row (styles_logic_category_text (""),     "&category=");
      page += dialog_list.run ();
      return page;
    } else {
      if (write) database_styles.updateCategory (sheet, style, category);
    }
  }
  view.set_variable ("category", styles_logic_category_text(category));
  

  
  
  
  page += view.render ("styles", "view");
  
  page += Assets_Page::footer ();
  
  return page;
}
