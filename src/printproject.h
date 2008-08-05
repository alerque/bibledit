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
 ** Foundation, Inc.,  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */

#ifndef INCLUDED_PRINTPROJECT_H
#define INCLUDED_PRINTPROJECT_H

#include <gtk/gtk.h>
#include "ustring.h"
#include "settings.h"
#include "usfm.h"
#include "usfm-inline-markers.h"
#include "scriptureportions.h"
#include "project_memory.h"

class PrintProject
{
public:
  PrintProject(ProjectMemory * project);
  ~PrintProject();
  void portion_project(const ustring& project);
  void comment(const ustring& text);
  void print();
private:
  ProjectMemory * myproject;
  ustring portionproject;
  ScripturePortions * scriptureportions;
  vector<ustring> xslfo_lines;
  vector <ustring> comments;
};

class PrintProject2
{
public:
  PrintProject2(ProjectMemory * project);
  ~PrintProject2();
  void portion_project(const ustring& project);
  void comment(const ustring& text);
  void print();
private:
  ProjectMemory * myproject;
  ustring portionproject;
  ScripturePortions * scriptureportions;
  vector<ustring> xslfo_lines;
  vector <ustring> comments;
};

#endif
