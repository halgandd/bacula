/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2008 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation and included
   in the file LICENSE.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301, USA.

   Bacula® is a registered trademark of Kern Sibbald.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
 
/*
 *   Version $Id$
 *
 *  Clients Class
 *
 *   Dirk Bartley, March 2007
 *
 */ 

#include <QAbstractEventDispatcher>
#include <QMenu>
#include "bat.h"
#include "clients/clients.h"
#include "run/run.h"
#include "status/clientstat.h"
#include "util/fmtwidgetitem.h"

Clients::Clients()
{
   setupUi(this);
   m_name = tr("Clients");
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0,QIcon(QString::fromUtf8(":images/network-server.png")));

   /* tableWidget, Storage Tree Tree Widget inherited from ui_client.h */
   m_populated = false;
   m_checkcurwidget = true;
   m_closeable = false;
   /* add context sensitive menu items specific to this classto the page
    * selector tree. m_contextActions is QList of QActions */
   m_contextActions.append(actionRefreshClients);
   createContextMenu();
   dockPage();
}

Clients::~Clients()
{
}

/*
 * The main meat of the class!!  The function that querries the director and 
 * creates the widgets with appropriate values.
 */
void Clients::populateTable()
{
   QBrush blackBrush(Qt::black);

   if (!m_console->preventInUseConnect())
      return;

   QStringList headerlist = (QStringList() << tr("Client Name") << tr("File Retention")
       << tr("Job Retention") << tr("AutoPrune") << tr("ClientId") << tr("Uname") );

   int sortcol = headerlist.indexOf(tr("Client Name"));
   Qt::SortOrder sortord = Qt::AscendingOrder;
   if (tableWidget->rowCount()) {
      sortcol = tableWidget->horizontalHeader()->sortIndicatorSection();
      sortord = tableWidget->horizontalHeader()->sortIndicatorOrder();
   }

   m_checkcurwidget = false;
   tableWidget->clear();
   m_checkcurwidget = true;

   tableWidget->setColumnCount(headerlist.count());
   tableWidget->setHorizontalHeaderLabels(headerlist);
   tableWidget->horizontalHeader()->setHighlightSections(false);
   tableWidget->setRowCount(m_console->client_list.count());
   tableWidget->verticalHeader()->hide();
   tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
   tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);
   tableWidget->setSortingEnabled(false); /* rows move on insert if sorting enabled */
   int row = 0;

   foreach (QString clientName, m_console->client_list){
      /* Set up query QString and header QStringList */
      QString query("");
      query += "SELECT Name, FileRetention, JobRetention, AutoPrune, ClientId, Uname"
           " FROM Client"
           " WHERE ";
      query += " Name='" + clientName + "'";
      query += " ORDER BY ClientId LIMIT 1";

      QStringList results;
      /* This could be a log item */
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "Clients query cmd : %s\n",query.toUtf8().data());
      }
      if (m_console->sql_cmd(query, results)) {
         int resultCount = results.count();
         if (resultCount){
            /* only use the last one */
            QString resultline = results[resultCount - 1];
            QStringList fieldlist = resultline.split("\t");

	    TableItemFormatter item(*tableWidget, row);

	    /* Iterate through fields in the record */
	    QStringListIterator fld(fieldlist);
	    int col = 0;

	    /* name */
	    item.setTextFld(col++, fld.next());

	    /* file retention */
	    item.setDurationFld(col++, fld.next());

	    /* job retention */
	    item.setDurationFld(col++, fld.next());

	    /* autoprune */
	    item.setBoolFld(col++, fld.next());

	    /* client id */
	    item.setNumericFld(col++, fld.next());

	    /* uname */
	    item.setTextFld(col++, fld.next());

         }
      }
      row ++;
   }
   /* set default sorting */
   tableWidget->sortByColumn(sortcol, sortord);
   tableWidget->setSortingEnabled(true);
   
   /* Resize rows and columns */
   tableWidget->resizeColumnsToContents();
   tableWidget->resizeRowsToContents();
}

/*
 * When the treeWidgetItem in the page selector tree is singleclicked, Make sure
 * The tree has been populated.
 */
void Clients::PgSeltreeWidgetClicked()
{
   if(!m_populated) {
      populateTable();
      m_populated=true;
   }
}

/*
 * Added to set the context menu policy based on currently active treeWidgetItem
 * signaled by currentItemChanged
 */
void Clients::tableItemChanged(QTableWidgetItem *currentwidgetitem, QTableWidgetItem *previouswidgetitem )
{
   /* m_checkcurwidget checks to see if this is during a refresh, which will segfault */
   if (m_checkcurwidget) {
      int currentRow = currentwidgetitem->row();
      QTableWidgetItem *currentrowzeroitem = tableWidget->item(currentRow, 0);
      m_currentlyselected = currentrowzeroitem->text();

      /* The Previous item */
      if (previouswidgetitem) { /* avoid a segfault if first time */
         tableWidget->removeAction(actionListJobsofClient);
         tableWidget->removeAction(actionStatusClientInConsole);
         tableWidget->removeAction(actionStatusClientWindow);
         tableWidget->removeAction(actionPurgeJobs);
         tableWidget->removeAction(actionPrune);
      }

      if (m_currentlyselected.length() != 0) {
         /* set a hold variable to the client name in case the context sensitive
          * menu is used */
         tableWidget->addAction(actionListJobsofClient);
         tableWidget->addAction(actionStatusClientInConsole);
         tableWidget->addAction(actionStatusClientWindow);
         tableWidget->addAction(actionPurgeJobs);
         tableWidget->addAction(actionPrune);
      }
   }
}

/* 
 * Setup a context menu 
 * Made separate from populate so that it would not create context menu over and
 * over as the tree is repopulated.
 */
void Clients::createContextMenu()
{
   tableWidget->setContextMenuPolicy(Qt::ActionsContextMenu);
   tableWidget->addAction(actionRefreshClients);
   /* for the tableItemChanged to maintain m_currentJob */
   connect(tableWidget, SIGNAL(
           currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)),
           this, SLOT(tableItemChanged(QTableWidgetItem *, QTableWidgetItem *)));

   /* connect to the action specific to this pages class */
   connect(actionRefreshClients, SIGNAL(triggered()), this,
                SLOT(populateTable()));
   connect(actionListJobsofClient, SIGNAL(triggered()), this,
                SLOT(showJobs()));
   connect(actionStatusClientInConsole, SIGNAL(triggered()), this,
                SLOT(consoleStatusClient()));
   connect(actionStatusClientWindow, SIGNAL(triggered()), this,
                SLOT(statusClientWindow()));
   connect(actionPurgeJobs, SIGNAL(triggered()), this,
                SLOT(consolePurgeJobs()));
   connect(actionPrune, SIGNAL(triggered()), this,
                SLOT(prune()));
}

/*
 * Function responding to actionListJobsofClient which calls mainwin function
 * to create a window of a list of jobs of this client.
 */
void Clients::showJobs()
{
   QTreeWidgetItem *parentItem = mainWin->getFromHash(this);
   mainWin->createPageJobList("", m_currentlyselected, "", "", parentItem);
}

/*
 * Function responding to actionListJobsofClient which calls mainwin function
 * to create a window of a list of jobs of this client.
 */
void Clients::consoleStatusClient()
{
   QString cmd("status client=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void Clients::currentStackItem()
{
   if(!m_populated) {
      populateTable();
      /* Create the context menu for the client table */
      m_populated=true;
   }
}

/*
 * Function responding to actionPurgeJobs 
 */
void Clients::consolePurgeJobs()
{
   if (QMessageBox::warning(this, "Bat",
      tr("Are you sure you want to purge all jobs of client \"%1\" ?\n"
"The Purge command will delete associated Catalog database records from Jobs and"
" Volumes without considering the retention period. Purge  works only on the"
" Catalog database and does not affect data written to Volumes. This command can"
" be dangerous because you can delete catalog records associated with current"
" backups of files, and we recommend that you do not use it unless you know what"
" you are doing.\n\n"
" Is there any way I can get you to click Cancel here?  You really don't want to do"
" this\n\n"
	 "Press OK to proceed with the purge operation?").arg(m_currentlyselected),
	 QMessageBox::Ok | QMessageBox::Cancel,
	 QMessageBox::Cancel)
      == QMessageBox::Cancel) { return; }

   QString cmd("purge jobs client=");
   cmd += m_currentlyselected;
   consoleCommand(cmd);
}

/*
 * Function responding to actionPrune
 */
void Clients::prune()
{
   new prunePage("", m_currentlyselected);
}

/*
 * Function responding to action to create new client status window
 */
void Clients::statusClientWindow()
{
   QTreeWidgetItem *parentItem = mainWin->getFromHash(this);
   new ClientStat(m_currentlyselected, parentItem);
}

