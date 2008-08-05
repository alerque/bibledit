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

#ifndef INCLUDED_TEXT2PDF_H
#define INCLUDED_TEXT2PDF_H

#include "libraries.h"
#include <pango/pangocairo.h>
#include "text2pdf_utils.h"
#include "text2pdf_area.h"
#include "text2pdf_block.h"
#include "text2pdf_ref_area.h"
#include "text2pdf_input.h"
#include "text2pdf_page.h"
#include "progresswindow.h"

class Text2Pdf
{
public:
  Text2Pdf(const ustring& pdf_file);
  ~Text2Pdf();

  // Initialization.
public:
private:
  void initialize_variables();

  // Cairo data.
public:

private:
  cairo_t *cairo;
  cairo_status_t status;
  cairo_surface_t *surface;

  // Input data.
public:
  void open_keep_together();
  void close_keep_together();
  void open_paragraph();
  void ensure_open_paragraph();
  void paragraph_set_font_size(unsigned int points);
  void paragraph_set_italic(bool italic);
  void paragraph_set_bold(bool bold);
  void paragraph_set_underline(bool underline);
  void paragraph_set_small_caps(bool small_caps);
  void paragraph_set_alignment(T2PAlignmentType alignment);
  void paragraph_set_space_before(int millimeters);
  void paragraph_set_space_after(int millimeters);
  void paragraph_set_left_margin(int millimeters);
  void paragraph_set_right_margin(int millimeters);
  void paragraph_set_first_line_indent(int millimeters);
  void paragraph_set_column_count(unsigned int count);
  void paragraph_set_keep_with_next();
  void close_paragraph();
  void inline_set_font_size_percentage(int percentage);
  void inline_clear_font_size_percentage();
  void inline_set_italic(int italic);
  void inline_clear_italic();
  void inline_set_bold(int bold);
  void inline_clear_bold();
  void inline_set_underline(int underline);
  void inline_clear_underline();
  void inline_set_small_caps(int small_caps);
  void inline_clear_small_caps();
  void inline_set_superscript(bool superscript = true);
  void inline_clear_superscript();
  void inline_set_colour(int colour);
  void inline_clear_colour();
  void inline_set_strike_through();
  void inline_clear_strike_through();
  void add_text(const ustring& text);
  void open_note();
  void close_note();
private:
  T2PInputParagraph * input_paragraph;
  T2PInputParagraph * stacked_input_paragraph;
  vector <T2PInput *> text_input_data;
  bool keep_data_together;
  deque <T2PBlock *> text_input_blocks;

  // Areas and their sizes.
public:
  void page_size_set(double width_centimeters, double height_centimeters);
  void page_margins_set(double inside_margin_centimeters, double outside_margin_centimeters, double top_margin_centimeters, double bottom_margin_centimeters);
  void header_height_set(double size_centimeters);
  void footer_height_set(double size_centimeters);
  void column_spacing_set(double spacing_centimeters);
  void page_one_column_only();
private:
  int page_width_pango_units, page_height_pango_units;
  int inside_margin_pango_units, outside_margin_pango_units, top_margin_pango_units, bottom_margin_pango_units;
  int header_height_pango_units;
  int footer_height_pango_units;
  int column_spacing_pango_units;
  bool one_column_only;

  // Various setting for whole engine.
public:
  void set_font(const ustring& font_in);
  void set_line_spacing(unsigned int line_spacing_in);
  void set_right_to_left(bool right_to_left_in);
  void print_date_in_header();
private:
  ustring font;
  unsigned int line_spacing;
  bool right_to_left;
  bool print_date;

  // Running.
public:
  void run();
private:
  void run_input(vector <T2PInput *>& input);
  T2PPage * page;
  vector <T2PPage *> pages;
  T2PBlock * block;
  T2PLayoutContainer * layoutcontainer;
  void lay_out_paragraph();
  void get_next_layout_container();
  void fit_blocks_on_pages();
  void next_page();
  void find_potential_widows_and_orphans();
  ProgressWindow * progresswindow;

  // Output.
public:
  void view();
private:
  ustring pdffile;

  // Test.
public:
  void test();
private:
  
};

#endif
