/*
 *
 *   Class used to parse tables received from director in this format :
 *
 *  +---------+---------+-------------------+
 *  | Header1 | Header2 | ...               |
 *  +---------+---------+-------------------+
 *  |  Data11 | Data12  | ...               |
 *  |  ....   | ...     | ...               |
 *  +---------+---------+-------------------+
 *
 *    Nicolas Boichat, April 2004
 *
 */
/*
   Copyright (C) 2004 Kern Sibbald and John Walker

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "wxbtableparser.h" // class's header file

#include "csprint.h"

#include <wx/tokenzr.h>

#include "wxbmainframe.h"

/*
 *   wxbTableParser constructor
 */
wxbTableParser::wxbTableParser() : wxbTable(5), wxbDataParser() {
   separatorNum = 0;
   tableHeader = wxbTableRow(5);
}

/*
 *   wxbTableParser destructor
 */
wxbTableParser::~wxbTableParser() {

}

/*
 *   Returns table header as an array of wxStrings.
 */
wxbTableRow* wxbTableParser::GetHeader() {
   return &tableHeader;
}

/*
 *   Receives director information, forwarded by the wxbPanel which
 *  uses this parser.
 */
void wxbTableParser::Print(wxString str, int status) {
   if ((status == CS_END) && (separatorNum > 0)) {
      separatorNum = 3;
   }

   if (separatorNum == 3) return;

   if (str.Length() > 4) {
      if ((str.GetChar(0) == '+') && (str.GetChar(str.Length()-2) == '+') && (str.GetChar(str.Length()-1) == '\n')) {
         separatorNum++;
         return;
      }

      if ((str.GetChar(0) == '|') && (str.GetChar(str.Length()-2) == '|') && (str.GetChar(str.Length()-1) == '\n')) {
         str.RemoveLast();
         wxStringTokenizer tkz(str, "|", wxTOKEN_STRTOK);

         if (separatorNum == 1) {
            int i = 0;
            while ( tkz.HasMoreTokens() ) {
               tableHeader[i++] = tkz.GetNextToken().Trim(true).Trim(false);
            }
         }
         else if (separatorNum == 2) {
            wxbTableRow tablerow(tableHeader.size());
            int i = 0;
            while ( tkz.HasMoreTokens() ) {
               tablerow[i++] = tkz.GetNextToken().Trim(true).Trim(false);
            }
            (*this)[size()] = tablerow;
         }
      }
   }
}

/*
 *   Return true table parsing has finished.
 */
bool wxbTableParser::hasFinished() {
   return (separatorNum == 3);
}
