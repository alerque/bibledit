/*
 ** Copyright (©) 2003-2009 Teus Benschop.
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
 ** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **  
 */


#ifndef INCLUDED_ASSISTANT_EXPORT_H
#define INCLUDED_ASSISTANT_EXPORT_H


#include <gtk/gtk.h>
#include "ustring.h"
#include "assistants.h"
#include "backup.h"
#include "export_utils.h"


class ExportAssistant : public AssistantBase
{
public:
  ExportAssistant(int dummy);
  virtual ~ExportAssistant();
private:
  // Assistant page preparation.
  static void on_assistant_prepare_signal (GtkAssistant *assistant, GtkWidget *page, gpointer user_data);
  void on_assistant_prepare (GtkWidget *page);

  // Backup type selection.
  int page_number_select_type;
  GtkWidget *vbox_select_type;
  GtkWidget *radiobutton_select_type_bible;
  GtkWidget *radiobutton_select_type_references;
  GtkWidget *radiobutton_select_type_stylesheet;
  GtkWidget *radiobutton_select_type_notes;
  ExportType get_type ();

  // Confirm or change Bible.
  int page_number_bible_name;
  GtkWidget *vbox_bible_name;
  GtkWidget *label_project_name;
  GtkWidget *button_bible_name;
  GtkWidget *alignment1;
  GtkWidget *hbox1;
  GtkWidget *image1;
  GtkWidget *label12;
  static void on_button_bible_name_clicked (GtkButton *button, gpointer user_data);
  void on_button_bible_name ();
  ustring bible_name;

  // Bible export type.
  int page_number_bible_type;
  GtkWidget *vbox_bible_type;
  GtkWidget *radiobutton_bible_usfm;
  GtkWidget *radiobutton_bible_bibleworks;
  GtkWidget *radiobutton_bible_osis;
  GtkWidget *radiobutton_bible_sword;
  GtkWidget *radiobutton_bible_opendocument;

  // Select file.
  int page_number_file;
  GtkWidget *vbox_file;
  GtkWidget *button_file;
  GtkWidget *label_file;
  static void on_button_file_clicked (GtkButton *button, gpointer user_data);
  void on_button_file ();
  ustring filename;
  
  // Confirmation stuff.
  int page_number_confirm;
  GtkWidget *label_confirm;
  GtkWidget *label_progress;
  GtkWidget *label_summary;
  unsigned int summary_page_number;
  static void on_assistant_apply_signal (GtkAssistant *assistant, gpointer user_data);
  void on_assistant_apply ();
  static gint assistant_forward_function (gint current_page, gpointer user_data);
  gint assistant_forward (gint current_page);
};


#endif
