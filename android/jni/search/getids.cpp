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


#include <search/getids.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/passage.h>
#include <webserver/request.h>
#include <locale/translate.h>
#include <database/config/general.h>
#include <search/logic.h>


string search_getids_url ()
{
  return "search/getids";
}


bool search_getids_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::translator ());
}


string search_getids (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;

  // Get search variables from the query.
  string bible = request->query ["b"];
  string searchfor = request->query ["q"];
  bool casesensitive = (request->query ["c"] == "true");

  // Do the search.
  vector <Passage> passages;
  if (casesensitive) {
    passages = search_logic_search_bible_text_case_sensitive (bible, searchfor);
  } else {
    passages = search_logic_search_bible_text (bible, searchfor);
  }

  // Output identifiers of the search results.
  string output;
  for (auto & passage : passages) {
    if (!output.empty ()) output.append ("\n");
    output.append (passage.to_text ());
  }
  return output;
}