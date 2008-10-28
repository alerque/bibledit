/*
 ** Copyright (©) 2003-2008 Teus Benschop.
 **  
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 3 of the License, or
 ** (at your option) any later version.
 **  
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **  
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 **  
 */

#include "libraries.h"
#include <glib.h>
#include "windowresource.h"
#include "help.h"
#include "windows.h"
#include "keyterms.h"
#include "tiny_utilities.h"
#include "projectutils.h"
#include "settings.h"
#include "resource_utils.h"

WindowResource::WindowResource(const ustring& name, bool startup) :
  WindowBase(widResource, name, startup)
// Window for showing the quick references.  
{
  resource = new Resource (window);
  resource->open(resourcename_to_filename(name));
}

WindowResource::~WindowResource() {
  delete resource;
}

void WindowResource::go_to(Reference& reference)
// Go to the references.
{
  resource->go_to(reference);
}

ustring WindowResource::resourcename_to_filename(const ustring& resourcename) {
  vector <ustring> filenames;
  vector <ustring> resources = resource_get_resources(filenames, false);
  ustring filename;
  for (unsigned int i = 0; i < resources.size(); i++) {
    if (resourcename == resources[i]) {
      filename = filenames[i];
    }
  }
  return filename;
}
