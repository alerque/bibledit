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


#include <filter/merge.h>
#include <dtl/dtl.hpp>
using dtl::Diff3;
#include <filter/string.h>


// merge - three-way merge.
// Merge is useful for combining separate changes to an original.
// The function normally returns the merged text.
// If case of conflicts, it returns an empty container.
vector <string> filter_merge_merge (const vector <string>& base, const vector <string>& user, const vector <string>& server)
{
  typedef string elem;
  typedef vector <string> sequence;

  sequence A (user);
  sequence B (base);
  sequence C (server);

  Diff3 <elem, sequence> diff3 (A, B, C);
  diff3.compose ();
  if (!diff3.merge ()) {
    return {};
  }
  return diff3.getMergedSequence ();
}


string filter_merge_lines2words (string data)
{
  data = filter_string_str_replace ("\n", " new__line ", data);
  data = filter_string_str_replace (" ", "\n", data);
  return data;
}


string filter_merge_words2lines (string data)
{
  data = filter_string_str_replace ("\n", " ", data);
  data = filter_string_str_replace (" new__line ", "\n", data);
  return data;
}


string filter_merge_lines2graphemes (string data)
{
  data = filter_string_str_replace ("\n", " new__line ", data);
  string data2;
  size_t count = unicode_string_length (data);
  for (size_t i = 0; i < count; i++) {
    string grapheme = unicode_string_substr (data, i, 1);
    data2.append (grapheme);
    data2.append ("\n");
  }
  return data2;
}


string filter_merge_graphemes2lines (string data)
{
  data = filter_string_str_replace ("\n", "", data);
  data = filter_string_str_replace (" new__line ", "\n", data);
  return data;
}


// This filter merges files.
// $base: Data for the merge base.
// $user: Data as modified by the user.
// $server: Data as modified by the collaboration server.
// The filter uses a three-way merge algorithm.
// There should be one unchanged segment (either a line or word) between the modifications.
// If necessary it converts the data into a new format with one character per line for more fine-grained merging.
// In case of a conflict, it favours the edition from the server.
// The filter returns the merged data.
string filter_merge_run (string base, string user, string server)
{
  // Trim the input.
  base = filter_string_trim (base);
  user = filter_string_trim (user);
  server = filter_string_trim (server);
  
  // Try a standard line-based merge. Should be sufficient for most cases.
  vector <string> baselines = filter_string_explode (base, '\n');
  vector <string> userlines = filter_string_explode (user, '\n');
  vector <string> serverlines = filter_string_explode (server, '\n');
  vector <string> result = filter_merge_merge (baselines, userlines, serverlines);
  if (!result.empty ()) {
    return filter_string_implode (result, "\n");
  }

  // Convert the data to one word per line, and try to merge again.
  string baseWords = filter_merge_lines2words (base);
  string userWords = filter_merge_lines2words (user);
  string serverWords = filter_merge_lines2words (server);
  baselines = filter_string_explode (baseWords, '\n');
  userlines = filter_string_explode (userWords, '\n');
  serverlines = filter_string_explode (serverWords, '\n');
  result = filter_merge_merge (baselines, userlines, serverlines);
  if (!result.empty ()) {
    string mergedWords = filter_string_implode (result, "\n");
    return filter_merge_words2lines (mergedWords);
  }

  // Convert the data so it has one grapheme per line, and try again.
  string baseGraphemes = filter_merge_lines2graphemes (base);
  string userGraphemes = filter_merge_lines2graphemes (user);
  string serverGraphemes = filter_merge_lines2graphemes (server);
  baselines = filter_string_explode (baseGraphemes, '\n');
  userlines = filter_string_explode (userGraphemes, '\n');
  serverlines = filter_string_explode (serverGraphemes, '\n');
  result = filter_merge_merge (baselines, userlines, serverlines);
  if (!result.empty ()) {
    string mergedGraphemes = filter_string_implode (result, "\n");
    return filter_merge_graphemes2lines (mergedGraphemes);
  }

  // The data could not be merged no matter how hard it tried.
  // Return the data as it is on the server.
  return server;
}
