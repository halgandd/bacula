/*
 *
 *    Bacula wx GUI application
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

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

#include "wx/wxprec.h"

#ifdef __BORLANDC__
   #pragma hdrstop
#endif

// for all others, include the necessary headers (this file is usually all you
// need because it includes almost all "standard" wxWindows headers)
#ifndef WX_PRECOMP
   #include "wx/wx.h"
#endif

#include "wxbmainframe.h"

#include "csprint.h"

// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------


class MyApp : public wxApp
{
public:
   virtual bool OnInit();
};

IMPLEMENT_APP(MyApp)

// ============================================================================
// implementation
// ============================================================================

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
   wxbMainFrame *frame = wxbMainFrame::CreateInstance(_T("Minimal wxWindows App"),
                         wxPoint(50, 50), wxSize(780, 500));

   frame->Show(TRUE);

   frame->StartConsoleThread();

   csprint("Console started !\n");

   return TRUE;
}
