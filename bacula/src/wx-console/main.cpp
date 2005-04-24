/*
 *
 *    Bacula wx-console application
 *
 *    Nicolas Boichat, April 2004
 *
 *    Version $Id$
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

#include <wx/wxprec.h>

#include "wxbmainframe.h"

#include "csprint.h"

/* Dummy functions */
int generate_daemon_event(JCR *jcr, const char *event) { return 1; }
int generate_job_event(JCR *jcr, const char *event) { return 1; }


// ----------------------------------------------------------------------------
// resources
// ----------------------------------------------------------------------------

class MyApp : public wxApp
{
public:
   virtual bool OnInit();
};

// ============================================================================
// implementation
// ============================================================================

IMPLEMENT_APP(MyApp)

// ----------------------------------------------------------------------------
// the application class
// ----------------------------------------------------------------------------

// 'Main program' equivalent: the program execution "starts" here
bool MyApp::OnInit()
{
   wxbMainFrame *frame = wxbMainFrame::CreateInstance(_T("Bacula wx-console"),
                         wxPoint(50, 50), wxSize(780, 500));

   frame->Show(TRUE);

   frame->Print(wxString("Welcome to bacula wx-console ") << VERSION << " (" << BDATE << ")!\n", CS_DEBUG);

   frame->StartConsoleThread("");
   
   return TRUE;
}
