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


#include <sync/notes.h>
#include <filter/url.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/merge.h>
#include <tasks/logic.h>
#include <config/logic.h>
#include <database/config/general.h>
#include <database/config/bible.h>
#include <database/logs.h>
#include <database/notes.h>
#include <database/modifications.h>
#include <client/logic.h>
#include <locale/translate.h>
#include <webserver/request.h>
#include <sync/logic.h>
#include <checksum/logic.h>
#include <access/bible.h>
#include <bible/logic.h>


string sync_notes_url ()
{
  return "sync/notes";
}


bool sync_notes_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string sync_notes (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  Sync_Logic sync_logic = Sync_Logic (webserver_request);
  Database_Notes database_notes = Database_Notes (&request);
  

  // Bail out if the notes databases are not available or in good shape.
  bool available = true;
  if (!database_notes.healthy ()) available = false;
  if (!database_notes.checksums_healthy ()) available = false;
  if (!database_notes.available ()) available = false;
  if (!available) {
    Database_Logs::log ("Notes databases are unhealthy or unavailable", Filter_Roles::translator ());
    request->response_code = 405;
    return "";
  }

  
  // What action does the client request from us?
  int action = convert_to_int (request->post ["a"]);

  
  // Check on the credentials when the clients sends data to the server to be stored there.
  if (action >= Sync_Logic::notes_put_create) {
    if (!sync_logic.credentials_okay ()) return "";
  }


  // Check on username only, without password or level.
  string user = hex2bin (request->post ["u"]);
  if ((action == Sync_Logic::notes_get_total) || (action == Sync_Logic::notes_get_identifiers)) {
    if (!request->database_users ()->usernameExists (user)) {
      Database_Logs::log ("A client passes non existing user " + user, Filter_Roles::manager ());
      return "";
    }
  }
  
  
  // Note lower and upper limits.
  int lowId = convert_to_int (request->post ["l"]);
  int highId = convert_to_int (request->post ["h"]);

  
  int identifier = convert_to_int (request->post ["i"]);

  
  switch (action) {
    case Sync_Logic::notes_get_total:
    {
      vector <string> bibles = access_bible_bibles (webserver_request, user);
      vector <int> identifiers = database_notes.getNotesInRangeForBibles (lowId, highId, bibles, false);
      string checksum = database_notes.getMultipleChecksum (identifiers);
      string response = to_string (identifiers.size ()) + "\n" + checksum;
      return response;
    }
    case Sync_Logic::notes_get_identifiers:
    {
      vector <string> bibles = access_bible_bibles (webserver_request, user);
      vector <int> identifiers = database_notes.getNotesInRangeForBibles (lowId, highId, bibles, false);
      string response;
      for (auto identifier : identifiers) {
        if (!response.empty ()) response.append ("\n");
        response.append (to_string (identifier));
        response.append ("\n");
        response.append (database_notes.getChecksum (identifier));
      }
      return response;
    }
    case Sync_Logic::notes_get_summary:
    {
      // Update search and checksum when the client requests the summary of a note,
      // because this is the first thing a client does when it requests a full note.
      // The client requests the notes in bits and pieces.
      database_notes.updateSearchFields (identifier);
      database_notes.updateChecksum (identifier);
      // Return summary.
      string summary = database_notes.getSummary (identifier);
      Database_Logs::log ("Client requested a note from server: " + summary, Filter_Roles::manager ());
      return summary;
    }
    case Sync_Logic::notes_get_contents:
    {
      return database_notes.getContents (identifier);
    }
    case Sync_Logic::notes_get_subscribers:
    {
      vector <string> subscribers = database_notes.getSubscribers (identifier);
      return filter_string_implode (subscribers, "\n");
    }
    case Sync_Logic::notes_get_assignees:
    {
      vector <string> assignees = database_notes.getAssignees (identifier);
      return filter_string_implode (assignees, "\n");
    }
    case Sync_Logic::notes_get_status:
    {
      return database_notes.getRawStatus (identifier);
    }
    case Sync_Logic::notes_get_passages:
    {
      vector <Passage> passages = database_notes.getPassages (identifier);
      vector <string> lines;
      for (auto & passage : passages) {
        lines.push_back (to_string (filter_passage_to_integer (passage)));
      }
      return filter_string_implode (lines, "\n");
    }
    case Sync_Logic::notes_get_severity:
    {
      return to_string (database_notes.getRawSeverity (identifier));
    }
    case Sync_Logic::notes_get_bible:
    {
      return database_notes.getBible (identifier);
    }
    case Sync_Logic::notes_get_modified:
    {
      return to_string (database_notes.getModified (identifier));
    }
    case Sync_Logic::notes_put_create:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_summary:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_contents:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_subscribers:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_assignees:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_status:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_passages:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_severity:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_bible:
    {
      return ""; // Todo
    }
    case Sync_Logic::notes_put_modified:
    {
      return ""; // Todo
    }
  }
  
  // Bad request.
  // Delay a while to obstruct a flood of bad requests.
  this_thread::sleep_for (chrono::seconds (1));
  request->response_code = 400;
  return "";
}
