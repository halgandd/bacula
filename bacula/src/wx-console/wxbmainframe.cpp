/*
 *
 *   Main frame
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

#include "wxbmainframe.h" // class's header file

#include "wxbrestorepanel.h"

#include "csprint.h"

#include "wxwin16x16.xpm"

#include <wx/arrimpl.cpp>

#include <wx/stattext.h>

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

// ----------------------------------------------------------------------------
// constants
// ----------------------------------------------------------------------------

// IDs for the controls and the menu commands
enum
{
   // menu items
   Minimal_Quit = 1,

   // it is important for the id corresponding to the "About" command to have
   // this standard value as otherwise it won't be handled properly under Mac
   // (where it is special and put into the "Apple" menu)
   Minimal_About = wxID_ABOUT,
   TypeText = 2,
   SendButton = 3,
   Thread = 4
};

/*
 *   wxbTHREAD_EVENT declaration, used by csprint
 */
BEGIN_DECLARE_EVENT_TYPES()
   DECLARE_EVENT_TYPE(wxbTHREAD_EVENT,       1)
END_DECLARE_EVENT_TYPES()

DEFINE_EVENT_TYPE(wxbTHREAD_EVENT)

typedef void (wxEvtHandler::*wxThreadEventFunction)(wxbThreadEvent&);

#define EVT_THREAD_EVENT(id, fn) \
    DECLARE_EVENT_TABLE_ENTRY( \
        wxbTHREAD_EVENT, id, wxID_ANY, \
        (wxObjectEventFunction)(wxEventFunction)(wxThreadEventFunction)&fn, \
        (wxObject *) NULL \
    ),

// the event tables connect the wxWindows events with the functions (event
// handlers) which process them. It can be also done at run-time, but for the
// simple menu events like this the static method is much simpler.
BEGIN_EVENT_TABLE(wxbMainFrame, wxFrame)
   EVT_MENU(Minimal_Quit,  wxbMainFrame::OnQuit)
   EVT_MENU(Minimal_About, wxbMainFrame::OnAbout)
   EVT_TEXT_ENTER(TypeText, wxbMainFrame::OnEnter)
   EVT_THREAD_EVENT(Thread, wxbMainFrame::OnPrint)
   EVT_BUTTON(SendButton, wxbMainFrame::OnEnter)
END_EVENT_TABLE()

// ----------------------------------------------------------------------------
// wxbThreadEvent
// ----------------------------------------------------------------------------

/*
 *  wxbThreadEvent constructor
 */
wxbThreadEvent::wxbThreadEvent(int id): wxEvent(id, wxbTHREAD_EVENT) {
   m_eventObject = NULL;
}

/*
 *  wxbThreadEvent destructor
 */
wxbThreadEvent::~wxbThreadEvent()
{
   if (m_eventObject != NULL) {
      delete m_eventObject;
   }
}

/*
 *  wxbThreadEvent copy constructor
 */
wxbThreadEvent::wxbThreadEvent(const wxbThreadEvent& te)
{
   this->m_eventType = te.m_eventType;
   this->m_id = te.m_id;
   if (te.m_eventObject != NULL) {
      this->m_eventObject = new wxbPrintObject(*((wxbPrintObject*)te.m_eventObject));
   }
   else {
      this->m_eventObject = NULL;
   }
   this->m_skipped = te.m_skipped;
   this->m_timeStamp = te.m_timeStamp;
}

/*
 *  Must be implemented (abstract in wxEvent)
 */
wxEvent* wxbThreadEvent::Clone() const
{
   return new wxbThreadEvent(*this);
}

/*
 *  Gets the wxbPrintObject attached to this event, containing data sent by director
 */
wxbPrintObject* wxbThreadEvent::GetEventPrintObject()
{
   return (wxbPrintObject*)m_eventObject;
}

/*
 *  Sets the wxbPrintObject attached to this event
 */
void wxbThreadEvent::SetEventPrintObject(wxbPrintObject* object)
{
   m_eventObject = (wxObject*)object;
}

// ----------------------------------------------------------------------------
// main frame
// ----------------------------------------------------------------------------

wxbMainFrame *wxbMainFrame::frame = NULL;

/*
 *  Singleton constructor
 */
wxbMainFrame* wxbMainFrame::CreateInstance(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
{
   frame = new wxbMainFrame(title, pos, size, style);
   return frame;
}

/*
 *  Returns singleton instance
 */
wxbMainFrame* wxbMainFrame::GetInstance()
{
   return frame;
}

/*
 *  Private destructor
 */
wxbMainFrame::~wxbMainFrame()
{
   if (ct != NULL) { // && (!ct->IsRunning())
      ct->Delete();
   }
   frame = NULL;
}

/*
 *  Private constructor
 */
wxbMainFrame::wxbMainFrame(const wxString& title, const wxPoint& pos, const wxSize& size, long style)
      : wxFrame(NULL, -1, title, pos, size, style)
{
   ct = NULL;

   // set the frame icon
   SetIcon(wxIcon(wxwin16x16_xpm));

#if wxUSE_MENUS
   // create a menu bar
   wxMenu *menuFile = new wxMenu;

   // the "About" item should be in the help menu
   wxMenu *helpMenu = new wxMenu;
   helpMenu->Append(Minimal_About, _T("&About...\tF1"), _T("Show about dialog"));

   menuFile->Append(Minimal_Quit, _T("E&xit\tAlt-X"), _T("Quit this program"));

   // now append the freshly created menu to the menu bar...
   wxMenuBar *menuBar = new wxMenuBar();
   menuBar->Append(menuFile, _T("&File"));
   menuBar->Append(helpMenu, _T("&Help"));

   // ... and attach this menu bar to the frame
   SetMenuBar(menuBar);
#endif // wxUSE_MENUS

   CreateStatusBar(1);
   SetStatusText(wxString("Welcome to bacula wx-console ") << VERSION << " (" << BDATE << ")!\n");

   wxPanel* global = new wxPanel(this, -1);

   notebook = new wxNotebook(global, -1);

   /* Console */

   wxPanel* consolePanel = new wxPanel(notebook, -1);
   notebook->AddPage(consolePanel, "Console");

   consoleCtrl = new wxTextCtrl(consolePanel,-1,"",wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH);
   wxFont font(10, wxMODERN, wxNORMAL, wxNORMAL);
#if defined __WXGTK12__ && !defined __WXGTK20__ // Fix for "chinese" fonts under gtk+ 1.2
   font.SetDefaultEncoding(wxFONTENCODING_ISO8859_1);
   consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
   Print("Warning : Unicode is disabled because you are using wxWidgets for GTK+ 1.2.\n", CS_DEBUG);
#else
   consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK, wxNullColour, font));
#endif

   wxFlexGridSizer *consoleSizer = new wxFlexGridSizer(2, 1, 0, 0);
   consoleSizer->AddGrowableCol(0);
   consoleSizer->AddGrowableRow(0);

   typeCtrl = new wxTextCtrl(consolePanel,TypeText,"",wxDefaultPosition,wxSize(200,20), wxTE_PROCESS_ENTER);
   sendButton = new wxButton(consolePanel, SendButton, "Send");
   
   wxFlexGridSizer *typeSizer = new wxFlexGridSizer(1, 3, 0, 0);
   typeSizer->AddGrowableCol(1);
   typeSizer->AddGrowableRow(0);

   typeSizer->Add(new wxStaticText(consolePanel, -1, "Command: "), 0, wxALIGN_CENTER | wxALL, 0);
   typeSizer->Add(typeCtrl, 1, wxEXPAND | wxALL, 0);
   typeSizer->Add(sendButton, 1, wxEXPAND | wxLEFT, 5);

   consoleSizer->Add(consoleCtrl, 1, wxEXPAND | wxALL, 0);
   consoleSizer->Add(typeSizer, 0, wxEXPAND | wxALL, 0);

   consolePanel->SetAutoLayout( TRUE );
   consolePanel->SetSizer( consoleSizer );
   consoleSizer->SetSizeHints( consolePanel );

   // Creates the list of panels which are included in notebook, and that need to receive director information

   panels = new wxbPanel* [2];
   panels[0] = new wxbRestorePanel(notebook);
   panels[1] = NULL;

   for (int i = 0; panels[i] != NULL; i++) {
      notebook->AddPage(panels[i], panels[i]->GetTitle());
   }

   wxBoxSizer* globalSizer = new wxBoxSizer(wxHORIZONTAL);

   globalSizer->Add(new wxNotebookSizer(notebook), 1, wxEXPAND, 0);

   global->SetSizer( globalSizer );
   globalSizer->SetSizeHints( global );

   wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);

   sizer->Add(global, 1, wxEXPAND | wxALL, 0);
   SetAutoLayout(true);
   SetSizer( sizer );
   sizer->SetSizeHints( this );
   this->SetSize(size);
   EnableConsole(false);
   
   lockedbyconsole = false;
   
   consoleBuffer = "";
}

/*
 *  Starts the thread interacting with the director
 */
void wxbMainFrame::StartConsoleThread()
{
   if (ct != NULL) {
      ct->Delete();
   }
   else {
      promptparser = new wxbPromptParser();      
   }
   
   wxString configfile;
   
   
   if ((wxTheApp->argc == 3) && (wxString(wxTheApp->argv[1]) == "-c")) {
      configfile = wxTheApp->argv[2];
   }
   else {
#ifdef HAVE_MACOSX
      configfile = "/Library/Preferences/org.bacula.wxconsole.conf";
#else
      configfile = "./wx-console.conf";
#endif
      if (wxTheApp->argc > 1) {
         Print("Error while parsing command line arguments, using defaults.\n", CS_DEBUG);
         Print("Usage: wx-console [-c configfile]\n", CS_DEBUG);
      }
   }
   
   ct = new console_thread(configfile);
   ct->Create();
   ct->Run();
   SetStatusText("Connecting to the director...");
}

/* Register a new wxbDataParser */
void wxbMainFrame::Register(wxbDataParser* dp) {
   parsers.Add(dp);
}
   
/* Unregister a wxbDataParser */
void wxbMainFrame::Unregister(wxbDataParser* dp) {
   int index;
   if ((index = parsers.Index(dp)) != wxNOT_FOUND) {
      parsers.RemoveAt(index);
   }
   else {
      Print("Failed to unregister a data parser !", CS_DEBUG);
   }
}

// event handlers

void wxbMainFrame::OnQuit(wxCommandEvent& WXUNUSED(event))
{
   Print("Quitting.\n", CS_DEBUG);
   if (ct != NULL) {
      ct->Delete();
      ct = NULL;
   }
   frame = NULL;
   Close(TRUE);
}

void wxbMainFrame::OnAbout(wxCommandEvent& WXUNUSED(event))
{
   wxString msg;
   msg.Printf( _T("Welcome to Bacula wx-console.\nWritten by Nicolas Boichat <nicolas@boichat.ch>\n(C) 2004 Kern Sibbald and John Walker\n"));

   wxMessageBox(msg, _T("About Bacula wx-console"), wxOK | wxICON_INFORMATION, this);
}

void wxbMainFrame::OnEnter(wxCommandEvent& WXUNUSED(event))
{
   lockedbyconsole = true;
   DisablePanels();
   wxString str = typeCtrl->GetValue() + "\n";
   Send(str);
}

/*
 *  Called when data is arriving from director
 */
void wxbMainFrame::OnPrint(wxbThreadEvent& event) {
   wxbPrintObject* po = event.GetEventPrintObject();

   Print(po->str, po->status);
}

/*
 *  Prints data received from director to the console, and forwards it to the panels
 */
void wxbMainFrame::Print(wxString str, int status)
{
   if (lockedbyconsole) {
      EnableConsole(false);
   }
   
   if (status == CS_TERMINATED) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = "";
      SetStatusText("Console thread terminated.");
      ct = NULL;
      DisablePanels();
      int answer = wxMessageBox("Connection to the director lost. Quit program?", "Connection lost",
                        wxYES_NO | wxICON_EXCLAMATION, this);
      if (answer == wxYES) {
         frame = NULL;
         Close(true);
      }
      return;
   }
   
   if (status == CS_CONNECTED) {
      SetStatusText("Connected to the director.");
      EnablePanels();
      return;
   }
   if (status == CS_DISCONNECTED) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = "";
      SetStatusText("Disconnected of the director.");
      DisablePanels();
      return;
   }
      
   // CS_DEBUG is often sent by panels, 
   // and resend it to them would sometimes cause infinite loops
   
   /* One promptcaught is normal, so we must have two true Print values to be
    * sure that the prompt has effectively been caught.
    */
   int promptcaught = -1;
   
   if (status != CS_DEBUG) {
      for (unsigned int i = 0; i < parsers.GetCount(); i++) {
         promptcaught += parsers[i]->Print(str, status) ? 1 : 0;
      }
         
      if ((status == CS_PROMPT) && (promptcaught < 1) && (promptparser->isPrompt())) {
         Print("Unexpected question has been received.\n", CS_DEBUG);
//         Print(wxString("(") << promptparser->getIntroString() << "/-/" << promptparser->getQuestionString() << ")\n", CS_DEBUG);
         
         wxString message;
         if (promptparser->getIntroString() != "") {
            message << promptparser->getIntroString() << "\n";
         }
         message << promptparser->getQuestionString();
         
         if (promptparser->getChoices()) {
            wxString *choices = new wxString[promptparser->getChoices()->GetCount()];
            int *numbers = new int[promptparser->getChoices()->GetCount()];
            int n = 0;
            
            for (unsigned int i = 0; i < promptparser->getChoices()->GetCount(); i++) {
               if ((*promptparser->getChoices())[i] != "") {
                  choices[n] = (*promptparser->getChoices())[i];
                  numbers[n] = i;
                  n++;
               }
            }
            
            int res = ::wxGetSingleChoiceIndex(message,
               "wx-console: unexpected director's question.", n, choices, this);
            if (res == -1) {
               Send("\n");
            }
            else {
               Send(wxString() << numbers[res] << "\n");
            }
         }
         else {
            Send(::wxGetTextFromUser(message,
               "wx-console: unexpected director's question.", "", this) + "\n");
         }
      }
   }
      
   if (status == CS_END) {
      if (lockedbyconsole) {
         EnablePanels();
         lockedbyconsole = false;
      }
      str = "#";
   }

   if (status == CS_DEBUG) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = "";
      consoleCtrl->SetDefaultStyle(wxTextAttr(wxColour(0, 128, 0)));
   }
   else {
      consoleCtrl->SetDefaultStyle(wxTextAttr(*wxBLACK));
   }
   consoleBuffer << str;
   if (status == CS_PROMPT) {
      if (lockedbyconsole) {
         EnableConsole(true);
      }
      consoleBuffer << "<P>";
   }
   
   if ((status == CS_END) || (status == CS_PROMPT) || (str.Find("\n") > -1)) {
      consoleCtrl->AppendText(consoleBuffer);
      consoleBuffer = "";
   
      consoleCtrl->ScrollLines(3);
   }
   
//   consoleCtrl->ShowPosition(consoleCtrl->GetLastPosition());
   
   /*if (status != CS_DEBUG) {
      consoleCtrl->AppendText("@");
   }*/
   //consoleCtrl->SetInsertionPointEnd();
   
/*   if ((consoleCtrl->GetNumberOfLines()-1) > nlines) {
      nlines = consoleCtrl->GetNumberOfLines()-1;
   }
   
   if (status == CS_END) {
      consoleCtrl->ShowPosition(nlines);
   }*/
}

/*
 *  Sends data to the director
 */
void wxbMainFrame::Send(wxString str)
{
   if (ct != NULL) {
      ct->Write((const char*)str);
      typeCtrl->SetValue("");
      consoleCtrl->SetDefaultStyle(wxTextAttr(*wxRED));
      consoleCtrl->AppendText(str);
      consoleCtrl->ScrollLines(3);
   }
   
/*   if ((consoleCtrl->GetNumberOfLines()-1) > nlines) {
      nlines = consoleCtrl->GetNumberOfLines()-1;
   }
   
   consoleCtrl->ShowPosition(nlines);*/
}

/* Enable panels */
void wxbMainFrame::EnablePanels() {
   for (int i = 0; panels[i] != NULL; i++) {
      panels[i]->EnablePanel(true);
   }
   EnableConsole(true);
}

/* Disable panels, except the one passed as parameter */
void wxbMainFrame::DisablePanels(void* except) {
   for (int i = 0; panels[i] != NULL; i++) {
      if (panels[i] != except) {
         panels[i]->EnablePanel(false);
      }
      else {
         panels[i]->EnablePanel(true);
      }
   }
   if (this != except) {
      EnableConsole(false);
   }
}

/* Enable or disable console typing */
void wxbMainFrame::EnableConsole(bool enable) {
   typeCtrl->Enable(enable);
   sendButton->Enable(enable);
   if (enable) {
      typeCtrl->SetFocus();
   }
}

/*
 *  Used by csprint, which is called by console thread.
 *
 *  In GTK and perhaps X11, only the main thread is allowed to interact with
 *  graphical components, so by firing an event, the main loop will call OnPrint.
 *
 *  Calling OnPrint directly from console thread produces "unexpected async replies".
 */
void firePrintEvent(wxString str, int status)
{
   wxbPrintObject* po = new wxbPrintObject(str, status);

   wxbThreadEvent evt(Thread);
   evt.SetEventPrintObject(po);
   
   if (wxbMainFrame::GetInstance()) {
      wxbMainFrame::GetInstance()->AddPendingEvent(evt);
   }
}

//wxString csBuffer; /* Temporary buffer for receiving data from console thread */

/*
 *  Called by console thread, this function forwards data line by line and end
 *  signals to the GUI.
 */
void csprint(const char* str, int status)
{
   if (str != 0) {
      firePrintEvent(wxString(str), status);
   }
   else {
      firePrintEvent("", status);
   }
}

