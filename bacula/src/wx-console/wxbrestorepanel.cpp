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

#include "wxbrestorepanel.h"

#include "wxbmainframe.h"

#include "csprint.h"

#include <wx/choice.h>
#include <wx/datetime.h>

#include "unmarked.xpm"
#include "marked.xpm"
#include "partmarked.xpm"

/* A macro named Yield is defined under MinGW */
#undef Yield

/*
 *  Class which is stored in the tree and in the list to keep informations
 *  about the element.
 */
class wxbTreeItemData : public wxTreeItemData {
   public:
      wxbTreeItemData(wxString path, wxString name, int marked, long listid = -1);
      wxbTreeItemData(wxString path, wxString name, wxString marked, long listid = -1);
      ~wxbTreeItemData();
      wxString GetPath();
      wxString GetName();
            
      int GetMarked();
      void SetMarked(int marked);
      void SetMarked(wxString marked);
      
      long GetListId();

      static int GetMarkedStatus(wxString file);
   private:
      wxString* path; /* Full path */
      wxString* name; /* File name */
      int marked; /* 0 - Not Marked, 1 - Marked, 2 - Some file under is marked */
      long listid; /* list ID : >-1 if this data is in the list (and/or on the tree) */
};

wxbTreeItemData::wxbTreeItemData(wxString path, wxString name, int marked, long listid): wxTreeItemData() {
   this->path = new wxString(path);
   this->name = new wxString(name);
   this->marked = marked;
   this->listid = listid;
}

wxbTreeItemData::wxbTreeItemData(wxString path, wxString name, wxString marked, long listid): wxTreeItemData() {
   this->path = new wxString(path);
   this->name = new wxString(name);
   SetMarked(marked);
   this->listid = listid;
}

wxbTreeItemData::~wxbTreeItemData() {
   delete path;
   delete name;
}

int wxbTreeItemData::GetMarked() {
   return marked;
}

void wxbTreeItemData::SetMarked(wxString marked) {
   if (marked == "*") {
      this->marked = 1;
   }
   else if (marked == "+") {
      this->marked = 2;
   }
   else {
      this->marked = 0;
   }
}

void wxbTreeItemData::SetMarked(int marked) {
   this->marked = marked;
}

long wxbTreeItemData::GetListId() {
   return listid;
}

wxString wxbTreeItemData::GetPath() {
   return *path;
}

wxString wxbTreeItemData::GetName() {
   return *name;
}

/*wxbTreeItemData* wxbTreeItemData::GetChild(wxString dirname) {
   int marked = GetMarkedStatus(dirname);
   return new wxbTreeItemData(path + (marked ? dirname.Mid(1) : dirname), marked);
}*/

int wxbTreeItemData::GetMarkedStatus(wxString file) {
   if (file.Length() == 0)
      return 0;
   
   switch (file.GetChar(0)) {
       case '*':
          return 1;
       case '+':
          return 2;
       default:
          return 0;
    }
}

// ----------------------------------------------------------------------------
// event tables and other macros for wxWindows
// ----------------------------------------------------------------------------

enum
{
   RestoreStart = 1,
   TreeCtrl = 2,
   ListCtrl = 3,
   ClientChoice = 4
};

BEGIN_EVENT_TABLE(wxbRestorePanel, wxPanel)
   EVT_BUTTON(RestoreStart, wxbRestorePanel::OnStart)
   EVT_TREE_SEL_CHANGING(TreeCtrl, wxbRestorePanel::OnTreeChanging)
   EVT_TREE_SEL_CHANGED(TreeCtrl, wxbRestorePanel::OnTreeChanged)
   EVT_TREE_ITEM_EXPANDING(TreeCtrl, wxbRestorePanel::OnTreeExpanding)
   EVT_LIST_ITEM_ACTIVATED(ListCtrl, wxbRestorePanel::OnListActivated)

   EVT_TREE_MARKED_EVENT(wxID_ANY, wxbRestorePanel::OnTreeMarked)
   EVT_LIST_MARKED_EVENT(wxID_ANY, wxbRestorePanel::OnListMarked)   
  
   EVT_CHOICE(ClientChoice, wxbRestorePanel::OnClientChoiceChanged)
END_EVENT_TABLE()

/*
 *  wxbRestorePanel constructor
 */
wxbRestorePanel::wxbRestorePanel(wxWindow* parent): wxbPanel(parent) {
   imagelist = new wxImageList(16, 16, TRUE, 3);
   imagelist->Add(wxIcon(unmarked_xpm));
   imagelist->Add(wxIcon(marked_xpm));
   imagelist->Add(wxIcon(partmarked_xpm));

   wxFlexGridSizer *sizer = new wxFlexGridSizer(3, 1, 10, 10);
   sizer->AddGrowableCol(0);
   sizer->AddGrowableRow(1);

   wxBoxSizer *firstSizer = new wxBoxSizer(wxHORIZONTAL);

   start = new wxButton(this, RestoreStart, "Enter restore mode", wxDefaultPosition, wxSize(150, 30));
   firstSizer->Add(start, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   wxString* elist = new wxString[1];

   clientChoice = new wxChoice(this, ClientChoice, wxDefaultPosition, wxSize(150, 30), 0, elist);
   firstSizer->Add(clientChoice, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   jobChoice = new wxChoice(this, -1, wxDefaultPosition, wxSize(150, 30), 0, elist);
   firstSizer->Add(jobChoice, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   sizer->Add(firstSizer, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL, 10);

   wxFlexGridSizer *secondSizer = new wxFlexGridSizer(1, 2, 10, 10);

   tree = new wxbTreeCtrl(this, TreeCtrl, wxDefaultPosition, wxSize(200,50));
   secondSizer->Add(tree, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, 10);

   tree->SetImageList(imagelist);

   list = new wxbListCtrl(this, ListCtrl, wxDefaultPosition, wxDefaultSize);
   secondSizer->Add(list, 1, wxALIGN_CENTER_HORIZONTAL | wxALIGN_CENTER_VERTICAL | wxEXPAND, 10);

   list->SetImageList(imagelist, wxIMAGE_LIST_SMALL);

   wxListItem info;
   info.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT);
   info.SetText("M");
   info.SetAlign(wxLIST_FORMAT_CENTER);
   list->InsertColumn(0, info);
   
   info.SetText("Filename");
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(1, info);

   info.SetText("Size");
   info.SetAlign(wxLIST_FORMAT_RIGHT);   
   list->InsertColumn(2, info);

   info.SetText("Date");
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(3, info);

   info.SetText("Perm.");
   info.SetAlign(wxLIST_FORMAT_LEFT);
   list->InsertColumn(4, info);
   
   info.SetText("User");
   info.SetAlign(wxLIST_FORMAT_RIGHT);
   list->InsertColumn(5, info);
   
   info.SetText("Group");
   info.SetAlign(wxLIST_FORMAT_RIGHT);
   list->InsertColumn(6, info);
   
   secondSizer->AddGrowableCol(1);
   secondSizer->AddGrowableRow(0);

   sizer->Add(secondSizer, 1, wxEXPAND, 10);

   gauge = new wxGauge(this, -1, 1, wxDefaultPosition, wxSize(200,20));

   sizer->Add(gauge, 1, wxEXPAND, 5);
   gauge->SetValue(0);
   gauge->Enable(false);

   SetSizer(sizer);
   sizer->SetSizeHints(this);

   SetStatus(disabled);

   jobChoice->Enable(false);

   for (int i = 0; i < 7; i++) {
      list->SetColumnWidth(i, 70);
   }

   working = false;
}

/*
 *  wxbRestorePanel destructor
 */
wxbRestorePanel::~wxbRestorePanel() {
   delete imagelist;
}

/*----------------------------------------------------------------------------
   wxbPanel overloadings
  ----------------------------------------------------------------------------*/

wxString wxbRestorePanel::GetTitle() {
   return "Restore";
}

void wxbRestorePanel::EnablePanel(bool enable) {
   if (enable) {
      if (status == disabled) {
         SetStatus(activable);
      }
   }
   else {
      SetStatus(disabled);
   }
}

/*----------------------------------------------------------------------------
   Commands called by events handler
  ----------------------------------------------------------------------------*/

/* The main button has been clicked */
void wxbRestorePanel::CmdStart() {
   if (status == activable) {
      wxbTableParser* tableparser = CreateAndWaitForParser("list clients\n");

      clientChoice->Clear();
      for (unsigned int i = 0; i < tableparser->size(); i++) {
         long* j = new long;
         (*tableparser)[i][0].ToLong(j);
         clientChoice->Append((*tableparser)[i][1], (void*)j);
      }
      
      delete tableparser;

      SetStatus(entered);
   }
   else if (status == entered) {
      if (jobChoice->GetStringSelection().Length() < 1) {
         wxbMainFrame::GetInstance()->SetStatusText("Please select a client.");
         return;
      }
      WaitForEnd("restore\n");
      WaitForEnd("6\n");
      WaitForEnd(wxString() << jobChoice->GetStringSelection() << "\n");
      WaitForEnd(wxString() << *((long*)clientChoice->GetClientData(clientChoice->GetSelection())) << "\n");
      WaitForEnd("unmark *\n");
      SetStatus(choosing);
      wxTreeItemId root = tree->AddRoot(clientChoice->GetStringSelection(), -1, -1, new wxbTreeItemData("/", clientChoice->GetStringSelection(), 0));
      tree->Refresh();
      UpdateTreeItem(root, true);
      wxbMainFrame::GetInstance()->SetStatusText("Right click on a file or on a directory, or double-click on its mark to add it to the restore list.");
      tree->Expand(root);
   }
   else if (status == choosing) {
      SetStatus(restoring);

      wxbMainFrame::GetInstance()->SetStatusText("Restoring, please wait...");

      totfilemessages = 0;
      wxbDataTokenizer* dt;
      
      dt = WaitForEnd("estimate\n", true);
      
      int j, k;
          
      for (unsigned int i = 0; i < dt->GetCount(); i++) {
         /* 15847 total files; 1 marked to be restored; 1,034 bytes. */
         if ((j = (*dt)[i].Find(" marked to be restored;")) > -1) {
            k = (*dt)[i].Find("; ");
            (*dt)[i].Mid(k+2, j).ToLong(&totfilemessages);
            break;
         }
      }
      
      delete dt;
      
      dt = WaitForEnd("done\n", true);

      for (unsigned int i = 0; i < dt->GetCount(); i++) {
         if ((j = (*dt)[i].Find(" files selected to be restored.")) > -1) {
            (*dt)[i].Mid(0, j).ToLong(&totfilemessages);
            break;
         }

         if ((j = (*dt)[i].Find(" file selected to be restored.")) > -1) {
            (*dt)[i].Mid(0, j).ToLong(&totfilemessages);
            break;
         }
      }
      
      delete dt;

      if (totfilemessages == 0) {
         wxbMainFrame::GetInstance()->Print("Restore failed : no file selected.\n", CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText("Restore failed : no file selected.");
         SetStatus(finished);
         return;
      }

      WaitForEnd("yes\n");

      gauge->SetValue(0);
      gauge->SetRange(totfilemessages);

      wxString cmd = "list jobid=";

      wxbTableParser* tableparser = CreateAndWaitForParser("list jobs\n");
        /* TODO (#1#): Check more carefully which job we just have run. */
      cmd << (*tableparser)[tableparser->size()-1][0] << "\n";

      delete tableparser;

      filemessages = 0;

      while (true) {
         tableparser = CreateAndWaitForParser(cmd);
         if ((*tableparser)[0][7] != "C") {
            break;
         }
         delete tableparser;

         dt = WaitForEnd("messages\n", true);
         
         for (unsigned int i = 0; i < dt->GetCount(); i++) {
            wxStringTokenizer tkz((*dt)[i], " ", wxTOKEN_STRTOK);
   
            wxDateTime datetime;
   
            //   Date    Time   name:   perm      ?   user   grp      size    date     time
            //04-Apr-2004 17:19 Tom-fd: -rwx------   1 nicolas  None     514967 2004-03-20 20:03:42  filename
   
            if (datetime.ParseDate(tkz.GetNextToken()) != NULL) { // Date
               if (datetime.ParseTime(tkz.GetNextToken()) != NULL) { // Time
                  if (tkz.GetNextToken().Last() == ':') { // name:
                  tkz.GetNextToken(); // perm
                  tkz.GetNextToken(); // ?
                  tkz.GetNextToken(); // user
                  tkz.GetNextToken(); // grp
                  tkz.GetNextToken(); // size
                  if (datetime.ParseDate(tkz.GetNextToken()) != NULL) { //date
                        if (datetime.ParseTime(tkz.GetNextToken()) != NULL) { //time
                           filemessages++;
                           //wxbMainFrame::GetInstance()->Print(wxString("(") << filemessages << ")", CS_DEBUG);
                           gauge->SetValue(filemessages);
                        }
                     }
                  }
               }
            }
         }
         
         delete dt;

         wxbMainFrame::GetInstance()->SetStatusText(wxString("Restoring, please wait (") << filemessages << " of " << totfilemessages << " files done)...");

         time_t start = wxDateTime::Now().GetTicks();
         while (((wxDateTime::Now().GetTicks())-start) < 3) {
            wxTheApp->Yield();
         }
      }

      WaitForEnd("messages\n");

      gauge->SetValue(totfilemessages);

      if ((*tableparser)[0][7] == "T") {
         wxbMainFrame::GetInstance()->Print("Restore done successfully.\n", CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText("Restore done successfully.");
      }
      else {
         wxbMainFrame::GetInstance()->Print("Restore failed, please look at messages.\n", CS_DEBUG);
         wxbMainFrame::GetInstance()->SetStatusText("Restore failed, please look at messages in console.");
      }
      delete tableparser;
      SetStatus(finished);
   }
}

/* List jobs for a specified client */
void wxbRestorePanel::CmdListJobs() {
   if (status == entered) {
      jobChoice->Clear();
      WaitForEnd("query\n");
      WaitForEnd("6\n");
      wxbTableParser* tableparser = CreateAndWaitForParser(clientChoice->GetString(clientChoice->GetSelection()) + "\n");

      for (int i = tableparser->size()-1; i > -1; i--) {
         wxString str = (*tableparser)[i][3];
         wxDateTime datetime;
         const char* chr;
         if ( ( (chr = datetime.ParseDate(str.GetData()) ) != NULL ) && ( datetime.ParseTime(++chr) != NULL ) ) {
            datetime = datetime.GetTicks() + 1;
            //wxbMainFrame::GetInstance()->Print(wxString("-") << datetime.Format("%Y-%m-%d %H:%M:%S"), CS_DEBUG);
            jobChoice->Append(datetime.Format("%Y-%m-%d %H:%M:%S"));
         }
         /*else {
         jobChoice->Append("Invalid");
         }*/
      }
      
      delete tableparser;

      jobChoice->SetSelection(0);
   }
}

/* List files and directories for a specified tree item */
void wxbRestorePanel::CmdList(wxTreeItemId item) {
   if (status == choosing) {
      list->DeleteAllItems();

      if (!item.IsOk()) {
         return;
      }
      UpdateTreeItem(item, (tree->GetSelection() == item));
    
      if (list->GetItemCount() > 1) {
         int firstwidth = list->GetSize().GetWidth(); 
         for (int i = 2; i < 7; i++) {
            list->SetColumnWidth(i, wxLIST_AUTOSIZE);
            firstwidth -= list->GetColumnWidth(i);
         }
       
         list->SetColumnWidth(0, 18);
         firstwidth -= 18;
         list->SetColumnWidth(1, wxLIST_AUTOSIZE);
         if (list->GetColumnWidth(1) < firstwidth) {
            list->SetColumnWidth(1, firstwidth-20);
         }
      }
   }
}

/* Mark a treeitem (directory) or a listitem (file or directory) */
void wxbRestorePanel::CmdMark(wxTreeItemId treeitem, long listitem) {
   if (status == choosing) {
      wxbTreeItemData* itemdata = NULL;
      if (listitem != -1) {
         itemdata = (wxbTreeItemData*)list->GetItemData(listitem);
      }
      else if (treeitem.IsOk()) {
         itemdata = (wxbTreeItemData*)tree->GetItemData(treeitem);
      }
      else {
         return;
      }

      if (itemdata == NULL) //Should never happen
         return;

      wxString dir = itemdata->GetPath();
      wxString file;

      if (dir != "/") {
         if (dir.GetChar(dir.Length()-1) == '/') {
            dir.RemoveLast();
         }

         int i = dir.Find('/', TRUE);
         if (i == -1) {
            file = dir;
            dir = "/";
         }
         else { /* first dir below root */
            file = dir.Mid(i+1);
            dir = dir.Mid(0, i+1);
         }
      }
      else {
         dir = "/";
         file = "*";
      }

      WaitForEnd(wxString("cd ") << dir << "\n");
      WaitForEnd(wxString((itemdata->GetMarked() == 1) ? "unmark " : "mark ") << file << "\n");

      /* TODO: Check commands results */

      /*if ((dir == "/") && (file == "*")) {
            itemdata->SetMarked((itemdata->GetMarked() == 1) ? 0 : 1);
      }*/

      if (listitem == -1) { /* tree item state changed */
         SetTreeItemState(treeitem, (itemdata->GetMarked() == 1) ? 0 : 1);
         /*treeitem = tree->GetSelection();
         UpdateTree(treeitem, true);
         treeitem = tree->GetItemParent(treeitem);*/
      }
      else {
         SetListItemState(listitem, (itemdata->GetMarked() == 1) ? 0 : 1);
         /*UpdateTree(treeitem, (tree->GetSelection() == treeitem));
         treeitem = tree->GetItemParent(treeitem);*/
      }

      /*while (treeitem.IsOk()) {
         WaitForList(treeitem, false);
         treeitem = tree->GetItemParent(treeitem);
      }*/
   }
}

/*----------------------------------------------------------------------------
   General functions
  ----------------------------------------------------------------------------*/

/* Parse a table in tableParser */
wxbTableParser* wxbRestorePanel::CreateAndWaitForParser(wxString cmd) {
   wxbTableParser* tableParser = new wxbTableParser();

   wxbMainFrame::GetInstance()->Send(cmd);

   //time_t base = wxDateTime::Now().GetTicks();
   while (!tableParser->hasFinished()) {
      //innerThread->Yield();
      wxTheApp->Yield();
      //if (base+15 < wxDateTime::Now().GetTicks()) break;
   }
   return tableParser;
}

/* Run a command, and waits until result is fully received. */
wxbDataTokenizer* wxbRestorePanel::WaitForEnd(wxString cmd, bool keepresults) {
   wxbDataTokenizer* datatokenizer = new wxbDataTokenizer();

   wxbMainFrame::GetInstance()->Send(cmd);
   
   //wxbMainFrame::GetInstance()->Print("(<WFE)", CS_DEBUG);
   
   //time_t base = wxDateTime::Now().GetTicks();
   while (!datatokenizer->hasFinished()) {
      //innerThread->Yield();
      wxTheApp->Yield();
      //if (base+15 < wxDateTime::Now().GetTicks()) break;
   }
   
   //wxbMainFrame::GetInstance()->Print("(>WFE)", CS_DEBUG);
   
   if (keepresults) {
      return datatokenizer;
   }
   else {
      delete datatokenizer;
      return NULL;
   }
}

/* Run a dir command, and waits until result is fully received. */
void wxbRestorePanel::UpdateTreeItem(wxTreeItemId item, bool updatelist) {
//   this->updatelist = updatelist;
   currentTreeItem = item;

   wxbDataTokenizer* dt;

   dt = WaitForEnd(wxString("cd \"") << 
      static_cast<wxbTreeItemData*>(tree->GetItemData(currentTreeItem))
         ->GetPath() << "\"\n", false);

   /* TODO: check command result */
   
   //delete dt;

   SetStatus(listing);

   if (updatelist)
      list->DeleteAllItems();
   dt = WaitForEnd("dir\n", true);
   
   wxString str;
   
   for (unsigned int i = 0; i < dt->GetCount(); i++) {
      str = (*dt)[i];
      
      if (str.Find("cwd is:") == 0) { // Sometimes cd command result "infiltrate" into listings.
         break;
      }

      str.RemoveLast();

      wxString* file = ParseList(str);
      
      if (file == NULL)
            break;

      wxTreeItemId treeid;

      if (file[8].GetChar(file[8].Length()-1) == '/') {
         wxString itemStr;

         long cookie;
         treeid = tree->GetFirstChild(currentTreeItem, cookie);

         bool updated = false;

         while (treeid.IsOk()) {
            itemStr = tree->GetItemText(treeid);
            if (file[8] == itemStr) {
               int stat = wxbTreeItemData::GetMarkedStatus(file[6]);
               if (static_cast<wxbTreeItemData*>(tree->GetItemData(treeid))->GetMarked() != stat) {
                  tree->SetItemImage(treeid, stat, wxTreeItemIcon_Normal);
                  tree->SetItemImage(treeid, stat, wxTreeItemIcon_Selected);
                  static_cast<wxbTreeItemData*>(tree->GetItemData(treeid))->SetMarked(file[6]);
               }
               updated = true;
               break;
            }
            treeid = tree->GetNextChild(currentTreeItem, cookie);
         }

         if (!updated) {
            int img = wxbTreeItemData::GetMarkedStatus(file[6]);
            treeid = tree->AppendItem(currentTreeItem, file[8], img, img, new wxbTreeItemData(file[7], file[8], file[6]));
         }
      }

      if (updatelist) {
         long ind = list->InsertItem(list->GetItemCount(), wxbTreeItemData::GetMarkedStatus(file[6]));
         wxbTreeItemData* data = new wxbTreeItemData(file[7], file[8], file[6], ind);
         data->SetId(treeid);
         list->SetItemData(ind, (long)data);
         list->SetItem(ind, 1, file[8]); // filename
         list->SetItem(ind, 2, file[4]); //Size
         list->SetItem(ind, 3, file[5]); //date
         list->SetItem(ind, 4, file[0]); //perm
         list->SetItem(ind, 5, file[2]); //user
         list->SetItem(ind, 6, file[3]); //grp
      }

      delete[] file;
   }
   
   delete dt;
   
   tree->Refresh();
   SetStatus(choosing);
}

/* Parse dir command results. */
wxString* wxbRestorePanel::ParseList(wxString line) {
   //drwx------  11 1003    42949672      0  2001-07-30 16:45:14 *filename
   //+ 10    ++ 4++   10   ++   8  ++   8  + +      19         + *+ ->
   //0       10  14         24      32       42                  62

   if (line.Length() < 63)
      return NULL;

   wxString* ret = new wxString[9];

   ret[0] = line.Mid(0, 10).Trim();
   ret[1] = line.Mid(10, 4).Trim();
   ret[2] = line.Mid(14, 10).Trim();
   ret[3] = line.Mid(24, 8).Trim();
   ret[4] = line.Mid(32, 8).Trim();
   ret[5] = line.Mid(42, 19).Trim();
   ret[6] = line.Mid(62, 1);
   ret[7] = line.Mid(63).Trim();

   if (ret[6] == " ") ret[6] = "";

   if (ret[7].GetChar(ret[7].Length()-1) == '/') {
      ret[8] = ret[7];
      ret[8].RemoveLast();
      ret[8] = ret[7].Mid(ret[8].Find('/', true)+1);
   }
   else {
      ret[8] = ret[7].Mid(ret[7].Find('/', true)+1);
   }

   return ret;
}

/* Sets a list item state, and update its parents and children if it is a directory */
void wxbRestorePanel::SetListItemState(long listitem, int newstate) {
   wxbTreeItemData* itemdata = (wxbTreeItemData*)list->GetItemData(listitem);
   
   wxTreeItemId treeitem;
   
   itemdata->SetMarked(newstate);
   list->SetItemImage(listitem, newstate, 0); /* TODO: Find what these ints are for */
   list->SetItemImage(listitem, newstate, 1);
      
   if ((treeitem = itemdata->GetId()).IsOk()) {
      SetTreeItemState(treeitem, newstate);
   }
   else {
      UpdateTreeItemState(tree->GetSelection());
   }
}

/* Sets a tree item state, and update its children, parents and list (if necessary) */
void wxbRestorePanel::SetTreeItemState(wxTreeItemId item, int newstate) {
   long cookie;
   wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);

   wxbTreeItemData* itemdata;

   while (currentChild.IsOk()) {
      itemdata = (wxbTreeItemData*)tree->GetItemData(currentChild);
      int state = itemdata->GetMarked();
      
      if (state != newstate) {
         itemdata->SetMarked(newstate);
         tree->SetItemImage(currentChild, newstate, wxTreeItemIcon_Normal);
         tree->SetItemImage(currentChild, newstate, wxTreeItemIcon_Selected);
      }
      
      currentChild = tree->GetNextChild(item, cookie);
   }
     
   itemdata = (wxbTreeItemData*)tree->GetItemData(item);  
   itemdata->SetMarked(newstate);
   tree->SetItemImage(item, newstate, wxTreeItemIcon_Normal);
   tree->SetItemImage(item, newstate, wxTreeItemIcon_Selected);
   tree->Refresh();
   
   if (tree->GetSelection() == item) {
      for (long i = 0; i < list->GetItemCount(); i++) {
         list->SetItemImage(i, newstate, 0); /* TODO: Find what these ints are for */
         list->SetItemImage(i, newstate, 1);
      }
   }
   
   UpdateTreeItemState(tree->GetParent(item));
}

/* Update a tree item state, and its parents' state */
void wxbRestorePanel::UpdateTreeItemState(wxTreeItemId item) {  
   if (!item.IsOk()) {
      return;
   }
   
   int state = 0;
       
   long cookie;
   wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);

   bool onechildmarked = false;
   bool onechildunmarked = false;

   while (currentChild.IsOk()) {
      state = ((wxbTreeItemData*)tree->GetItemData(currentChild))->GetMarked();
      switch (state) {
      case 0:
         onechildunmarked = true;
         break;
      case 1:
         onechildmarked = true;
         break;
      case 2:
         onechildmarked = true;
         onechildunmarked = true;
         break;
      }
      
      if (onechildmarked && onechildunmarked) {
         break;
      }
      
      currentChild = tree->GetNextChild(item, cookie);
   }
   
   if (tree->GetSelection() == item) {
      for (long i = 0; i < list->GetItemCount(); i++) {
         state = ((wxbTreeItemData*)list->GetItemData(i))->GetMarked();
         
         switch (state) {
         case 0:
            onechildunmarked = true;
            break;
         case 1:
            onechildmarked = true;
            break;
         case 2:
            onechildmarked = true;
            onechildunmarked = true;
            break;
         }
         
         if (onechildmarked && onechildunmarked) {
            break;
         }
      }
   }
   
   state = 0;
   
   if (onechildmarked && onechildunmarked) {
      state = 2;
   }
   else if (onechildmarked) {
      state = 1;
   }
   else if (onechildunmarked) {
      state = 0;
   }
   else { // no child, don't change anything
      UpdateTreeItemState(tree->GetParent(item));
      return;
   }
   
   wxbTreeItemData* itemdata = (wxbTreeItemData*)tree->GetItemData(item);
      
   itemdata->SetMarked(state);
   tree->SetItemImage(item, state, wxTreeItemIcon_Normal);
   tree->SetItemImage(item, state, wxTreeItemIcon_Selected);
   
   UpdateTreeItemState(tree->GetParent(item));
}

/* 
 * Refresh a tree item, and all its childs, 
 * by asking the director for new lists 
 */
void wxbRestorePanel::RefreshTree(wxTreeItemId item) {
/*   UpdateTreeItem(item, updatelist);*/

   /* Update all child which are not collapsed */
/*   long cookie;
   wxTreeItemId currentChild = tree->GetFirstChild(item, cookie);

   while (currentChild.IsOk()) {
      if (tree->IsExpanded(currentChild))
         UpdateTree(currentChild, false);

      currentChild = tree->GetNextChild(item, cookie);
   }*/
}

/*----------------------------------------------------------------------------
   Status function
  ----------------------------------------------------------------------------*/

/* Set current status by enabling/disabling components */
void wxbRestorePanel::SetStatus(status_enum newstatus) {
   switch (newstatus) {
   case disabled:
      start->SetLabel("Enter restore mode");
      start->Enable(false);
      clientChoice->Enable(false);
      jobChoice->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      gauge->Enable(false);
      break;
   case finished:
      tree->DeleteAllItems();
      list->DeleteAllItems();
      clientChoice->Clear();
      jobChoice->Clear();
      wxbMainFrame::GetInstance()->EnablePanels();
      newstatus = activable;
   case activable:
      start->SetLabel("Enter restore mode");
      start->Enable(true);
      clientChoice->Enable(false);
      jobChoice->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      gauge->Enable(false);
      break;
   case entered:
      wxbMainFrame::GetInstance()->DisablePanels(this);
      gauge->SetValue(0);
      start->SetLabel("Choose files to restore");
      clientChoice->Enable(true);
      jobChoice->Enable(true);
      tree->Enable(false);
      list->Enable(false);
      break;
   case listing:
      
      break;
   case choosing:
      start->SetLabel("Restore");
      clientChoice->Enable(false);
      jobChoice->Enable(false);
      tree->Enable(true);
      list->Enable(true);
      working = false;
      break;
   case restoring:
      start->SetLabel("Restoring...");
      gauge->Enable(true);
      gauge->SetValue(0);
      start->Enable(false);
      clientChoice->Enable(false);
      jobChoice->Enable(false);
      tree->Enable(false);
      list->Enable(false);
      working = true;
      break;
   }
   status = newstatus;
}

/*----------------------------------------------------------------------------
   Event handling
  ----------------------------------------------------------------------------*/

void wxbRestorePanel::OnClientChoiceChanged(wxCommandEvent& event) {
   if (working) {
      return;
   }
   working = true;
   clientChoice->Enable(false);
   CmdListJobs();
   clientChoice->Enable(true);
   jobChoice->Refresh();
   working = false;
}

void wxbRestorePanel::OnStart(wxEvent& WXUNUSED(event)) {
   if (working) {
      return;
   }
   working = true;
   CmdStart();
   working = false;
}

void wxbRestorePanel::OnTreeChanging(wxTreeEvent& event) {
   if (working) {
      event.Veto();
   }
}

void wxbRestorePanel::OnTreeExpanding(wxTreeEvent& event) {
   if (working) {
      event.Veto();
      return;
   }
   //working = true;
   //CmdList(event.GetItem());
   if (tree->GetSelection() != event.GetItem()) {
      tree->SelectItem(event.GetItem());
   }
   //working = false;
}

void wxbRestorePanel::OnTreeChanged(wxTreeEvent& event) {
   if (working) {
      return;
   }

   working = true;
   CmdList(event.GetItem());
   working = false;
}

void wxbRestorePanel::OnTreeMarked(wxbTreeMarkedEvent& event) {
   if (working) {
      //event.Skip();
      return;
   }
   working = true;
   CmdMark(event.GetItem(), -1);
   //event.Skip();
   tree->Refresh();
   working = false;
}

void wxbRestorePanel::OnListMarked(wxbListMarkedEvent& event) {
   if (working) {
      //event.Skip();
      return;
   }
   working = true;
   //long item = event.GetId(); 
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   CmdMark(wxTreeItemId(), item);
   event.Skip();
   tree->Refresh();
   working = false;
}

void wxbRestorePanel::OnListActivated(wxListEvent& event) {
   if (working) {
      //event.Skip();
      return;
   }
   working = true;
   long item = list->GetNextItem(-1, wxLIST_NEXT_ALL, wxLIST_STATE_FOCUSED);
   if (item > -1) {
      wxbTreeItemData* itemdata = (wxbTreeItemData*)list->GetItemData(item);
      wxString name = itemdata->GetName();
      event.Skip();

      wxString itemStr;

      long cookie;

      if (name.GetChar(name.Length()-1) == '/') {
         wxTreeItemId currentChild = tree->GetFirstChild(currentTreeItem, cookie);

         while (currentChild.IsOk()) {
            wxString name2 = tree->GetItemText(currentChild);
            if (name2 == name) {
               //tree->UnselectAll();
               working = false;
               tree->Expand(currentTreeItem);
               tree->SelectItem(currentChild);
               //tree->Refresh();
               return;
            }
            currentChild = tree->GetNextChild(currentTreeItem, cookie);
         }
      }
   }
   working = false;
}

