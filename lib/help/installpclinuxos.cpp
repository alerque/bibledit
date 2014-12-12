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


#include <help/installpclinuxos.h>
#include <assets/view.h>
#include <assets/page.h>
#include <assets/builder.h>
#include <filter/roles.h>
#include <config/logic.h>


const char * help_installpclinuxos_url ()
{
  return "help/installpclinuxos";
}


bool help_installpclinuxos_acl (void * webserver_request)
{
  return Filter_Roles::access_control (webserver_request, Filter_Roles::guest ());
}


string help_installpclinuxos (void * webserver_request)
{
  string page;

  page = Assets_Page::header (gettext("Installation"), webserver_request, "");

  Assets_View view = Assets_View (0);
  Assets_Builder builder;

  view.set_variable ("title", gettext("Install Bibledit on PCLinuxOS LXDE 2014"));

  view.set_variable ("intro", gettext("Follow the steps below to install Bibledit."));

  view.set_variable ("manager", gettext("Open the Synaptic Package Manager."));

  view.set_variable ("packages", gettext("Install the following packages:"));

  builder.clear();
  builder.sentence (gettext("To activate the web server, open Configure your Computer, click Sharing, click Configure web server."));
  builder.sentence (gettext("Follow the steps in the wizard, taking the default settings, and clicking Next, till it finishes."));
  view.set_variable ("webserver", builder.result());

  view.set_variable ("localhost", gettext("Test the web server by opening the site in the browser:"));
  view.set_variable ("ok", gettext("The browser should say that the server is alive and working."));

  view.set_variable ("terminal", gettext("Open a Terminal."));

  view.set_variable ("root", gettext("Become the administrative user:"));
  view.set_variable ("pass", gettext("Provide the administrative password."));

  view.set_variable ("download", gettext("Download and install Bibledit in folder /var/www/html:"));

  view.set_variable ("access", gettext("Bibledit will be accessible through:"));

  view.set_variable ("open", gettext("Open the web address in the browser, and follow the steps on the screen to configure Bibledit and log in."));

  view.set_variable ("reboot", gettext("Reboot."));

  page += view.render ("help", "installpclinuxos");

  page += Assets_Page::footer ();

  return page;
}
