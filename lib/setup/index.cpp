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


#include <setup/index.h>
#include <assets/view.h>
#include <assets/page.h>
#include <webserver/request.h>
#include <filter/roles.h>
#include <filter/string.h>
#include <filter/url.h>
#include <index/index.h>
#include <demo/logic.h>
#include <config/logic.h>
#include <config/globals.h>
#include <database/config/general.h>
#include <setup/logic.h>


// Returns a html page with a Bibledit splash screen.
// In case of $refresh, it refreshes to the home page.
string notice;
const char * setup_initialization_notice ()
{
  // Use http://base64online.org/encode/ to convert an image to inline graphic data.
  
  notice =
  "<!DOCTYPE html>\n"
  "<html>\n"
  "<head>\n"
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
  "<meta http-equiv=\"refresh\" content=\"2;URL=/index/index\" />\n"
  "<title>Bibledit</title>\n"
  "<style>\n"
  "html,body {\n"
  " margin:0;\n"
  " height:100%;\n"
  " overflow:hidden;\n"
  "}\n"
  "img {\n"
  " min-height:100%;\n"
  " min-width:100%;\n"
  " height:auto;\n"
  " width:auto;\n"
  " position:absolute;\n"
  " top:-50%; bottom:-50%;\n"
  " left:-50%; right:-50%;\n"
  " margin:auto;\n"
  " z-index: -1;\n"
  "}\n"
  "</style>\n"
  "</head>\n"
  "<body>\n"
  "<p><img src=\"data:image/jpeg;base64,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\"></p>\n"
  "<div style=\"text-align: center;\">\n"
  "<h1>Bibledit</h1>\n"
  "<h2>... initializing ...</h2>\n";

  // Visual progress indicator.
  // The progress bar restarts when it is near the end and the device is not yet ready initializing.
  // The user will understand that the device is still busy, and not think that it is not stuck.
  config_globals_setup_progress += 5;
  if (config_globals_setup_progress > 90) config_globals_setup_progress = 10;
  notice.append ("<p><progress value=\"" + convert_to_string (config_globals_setup_progress) + "\" max=\"100\"></progress></p>");
  
  notice.append (
  "</div>\n"
  "</body>\n"
  "</html>\n"
  );

  return notice.c_str();
}


string setup_index (void * webserver_request)
{
  Webserver_Request * request = (Webserver_Request *) webserver_request;
  
  Assets_View view;

  // Get the existing Administrators.
  vector <string> admins = request->database_users ()->getAdministrators ();

  // Admins do not yet exist: Allow to enter an admin.
  if (admins.empty ()) {
    if (!request->post ["Submit"].empty ()) {
      string admin_username = request->post ["admin_username"];
      string admin_password = request->post ["admin_password"];
      string admin_email = request->post ["admin_email"];
      vector <string> errors;
      if (admin_username.length() < 5) errors.push_back ("Choose a longer username.");
      if (admin_password.length() < 7) errors.push_back ("Choose a longer password.");
      if (admin_email.length() < 5) errors.push_back ("Enter a valid email address.");
      if (errors.empty()) {
        setup_set_admin_details (admin_username, admin_password, admin_email);
        setup_complete_gui ();
        redirect_browser (request, index_index_url ());
      } else {
        view.enable_zone ("errors");
        view.set_variable ("error", filter_string_implode (errors, " "));
      }
    }
  }

  // Enable appropriate zones: Either enter admin's details, or else display the details.
  if (admins.empty ()) {
    view.enable_zone ("enteradmin");
    view.enable_zone ("enteruser");
    view.enable_zone ("enterpass");
    view.enable_zone ("entermail");
    view.enable_zone ("displaysubmit");
  } else {
    string usernames;
    string emails;
    for (unsigned int i = 0; i < admins.size(); i++) {
      if (i) {
        usernames.append (" / ");
        emails.append (" / ");
      }
      usernames.append (admins[i]);
      emails.append (request->database_users ()->getUserToEmail (admins[i]));
    }
    view.set_variable ("usernames", usernames);
    view.set_variable ("emails", emails);
    view.enable_zone ("displayok");
    view.set_variable ("readonly", "readonly");
    // If the admin's are already there, then the setup has completed.
    // The automatic page refresh will kick in, and navigate to the main screen.
    Database_Config_General::setInstalledInterfaceVersion (config_logic_version ());
  }

  return view.render ("setup", "index");
}


