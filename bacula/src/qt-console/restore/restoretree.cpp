/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2007 Free Software Foundation Europe e.V.

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

   Bacula® is a registered trademark of John Walker.
   The licensor of Bacula is the Free Software Foundation Europe
   (FSFE), Fiduciary Program, Sumatrastrasse 25, 8006 Zürich,
   Switzerland, email:ftf@fsfeurope.org.
*/
 
/*
 *   Version $Id$
 *
 *  Restore Class 
 *
 *   Kern Sibbald, February MMVII
 *
 */ 

#include "bat.h"
#include "restoretree.h"
#include "pages.h"

restoreTree::restoreTree()
{
   setupUi(this);
   m_name = "Version Browser";
   pgInitialize();
   QTreeWidgetItem* thisitem = mainWin->getFromHash(this);
   thisitem->setIcon(0, QIcon(QString::fromUtf8(":images/browse.png")));

   m_closeable = true;
   m_populated = false;

   dockPage();
   m_winRegExpDrive.setPattern("^[a-z]:/$");
   m_winRegExpPath.setPattern("^[a-z]:/");
   m_slashregex.setPattern("/");
   m_debugCnt = 0;
   m_debugTrap = true;

   QGridLayout *m_gridLayout = new QGridLayout(this);
   m_gridLayout->setSpacing(6);
   m_gridLayout->setMargin(9);
   m_gridLayout->setObjectName(QString::fromUtf8("m_gridLayout"));

   splitter_2 = new QSplitter(Qt::Vertical, this);
   QScrollArea *area = new QScrollArea();
   area->setObjectName(QString::fromUtf8("area"));
   area->setWidget(widget);
   area->setWidgetResizable(true);
   splitter_2->addWidget(splitter);
   splitter_2->addWidget(area);

   m_gridLayout->addWidget(splitter_2, 0, 0, 1, 1);

   /* progress widgets */
   prBar1->setVisible(false);
   prBar2->setVisible(false);
   prLabel1->setVisible(false);
   prLabel2->setVisible(false);

   /* Set Defaults for check and spin for limits */
   limitCheckBox->setCheckState(mainWin->m_recordLimitCheck ? Qt::Checked : Qt::Unchecked);
   limitSpinBox->setValue(mainWin->m_recordLimitVal);
   daysCheckBox->setCheckState(mainWin->m_daysLimitCheck ? Qt::Checked : Qt::Unchecked);
   daysSpinBox->setValue(mainWin->m_daysLimitVal);
   readSettings();
}

restoreTree::~restoreTree()
{
   writeSettings();
}

/*
 * Called from the constructor to set up the page widgets and connections.
 */
void restoreTree::setupPage()
{
   connect(refreshButton, SIGNAL(pressed()), this, SLOT(refreshButtonPushed()));
   connect(restoreButton, SIGNAL(pressed()), this, SLOT(restoreButtonPushed()));
   connect(jobCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(jobComboChanged(int)));
   connect(jobCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRefresh()));
   connect(clientCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRefresh()));
   connect(fileSetCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateRefresh()));
   connect(limitCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateRefresh()));
   connect(daysCheckBox, SIGNAL(stateChanged(int)), this, SLOT(updateRefresh()));
   connect(daysSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateRefresh()));
   connect(limitSpinBox, SIGNAL(valueChanged(int)), this, SLOT(updateRefresh()));
   connect(directoryTree, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)),
           this, SLOT(directoryCurrentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
   connect(directoryTree, SIGNAL(itemExpanded(QTreeWidgetItem *)),
           this, SLOT(directoryItemExpanded(QTreeWidgetItem *)));
   connect(directoryTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
           this, SLOT(directoryItemChanged(QTreeWidgetItem *, int)));
   connect(fileTable, SIGNAL(currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)),
           this, SLOT(fileCurrentItemChanged(QTableWidgetItem *, QTableWidgetItem *)));
   connect(jobTable, SIGNAL(cellClicked(int, int)),
           this, SLOT(jobTableCellClicked(int, int)));

   QStringList titles = QStringList() << "Directories";
   directoryTree->setHeaderLabels(titles);
   clientCombo->addItems(m_console->client_list);
   fileSetCombo->addItem("Any");
   fileSetCombo->addItems(m_console->fileset_list);
   jobCombo->addItem("Any");
   jobCombo->addItems(m_console->job_list);

   directoryTree->setContextMenuPolicy(Qt::ActionsContextMenu);
}

void restoreTree::updateRefresh()
{
   if (mainWin->m_rtPopDirDebug) Pmsg2(000, "testing prev=\"%s\" current=\"%s\"\n", m_prevJobCombo.toUtf8().data(), jobCombo->currentText().toUtf8().data());
   m_dropdownChanged = (m_prevJobCombo != jobCombo->currentText())
                       || (m_prevClientCombo != clientCombo->currentText())
                       || (m_prevFileSetCombo != fileSetCombo->currentText()
                       || (m_prevLimitSpinBox != limitSpinBox->value())
                       || (m_prevDaysSpinBox != daysSpinBox->value())
                       || (m_prevLimitCheckState != limitCheckBox->checkState())
                       || (m_prevDaysCheckState != daysCheckBox->checkState())
   );
   if (m_dropdownChanged) {
      if (mainWin->m_rtPopDirDebug) Pmsg0(000, "In restoreTree::updateRefresh Is CHANGED\n");
      refreshLabel->setText("Refresh From Re-Select");
   } else {
      if (mainWin->m_rtPopDirDebug) Pmsg0(000, "In restoreTree::updateRefresh Is not Changed\n");
      refreshLabel->setText("Refresh From JobChecks");
   }
}

/*
 * When refresh button is pushed, perform a query getting the directories and
 * use parseDirectory and addDirectory to populate the directory tree with items.
 */
void restoreTree::populateDirectoryTree()
{
   m_slashTrap = false;
   m_dirPaths.clear();
   directoryTree->clear();
   fileTable->clear();
   fileTable->setRowCount(0);
   fileTable->setColumnCount(0);
   versionTable->clear();
   versionTable->setRowCount(0);
   versionTable->setColumnCount(0);
   m_fileExceptionHash.clear();
   m_fileExceptionMulti.clear();
   m_versionExceptionHash.clear();
   m_directoryIconStateHash.clear();

   updateRefresh();
   int taskcount = 2, ontask = 1;
   if (m_dropdownChanged) taskcount += 1;
   
   /* Set progress bars and repaint */
   prBar1->setVisible(true);
   prBar1->setRange(0,taskcount);
   prBar1->setValue(0);
   prLabel1->setText("Task " + QString("%1").arg(ontask)+ " of " + QString("%1").arg(taskcount));
   prLabel1->setVisible(true);
   prBar2->setVisible(true);
   prBar2->setRange(0,0);
   prLabel2->setText("Querying Database");
   prLabel2->setVisible(true);
   repaint();

   if (m_dropdownChanged) {
      m_prevJobCombo = jobCombo->currentText();
      m_prevClientCombo = clientCombo->currentText();
      m_prevFileSetCombo = fileSetCombo->currentText();
      m_prevLimitSpinBox = limitSpinBox->value();
      m_prevDaysSpinBox = daysSpinBox->value();
      m_prevLimitCheckState = limitCheckBox->checkState();
      m_prevDaysCheckState = daysCheckBox->checkState();
      updateRefresh();
      prBar1->setValue(ontask++);
      prLabel1->setText("Task " + QString("%1").arg(ontask)+ " of " + QString("%1").arg(taskcount));
      prBar2->setValue(0);
      prBar2->setRange(0,0);
      prLabel2->setText("Querying Jobs");
      repaint();
      populateJobTable();
      setJobsCheckedList();
   } else {
      if (mainWin->m_rtPopDirDebug) Pmsg0(000, "Repopulating from checks in Job Table\n");
      setJobsCheckedList();
   }

   if (m_checkedJobs != "") {
      QString cmd =
         "SELECT DISTINCT Path.Path AS Path"
         " FROM Path"
         " INNER JOIN File ON (File.PathId=Path.PathId)"
         " INNER JOIN Job ON (File.JobId=Job.JobId)"
         " WHERE Job.Jobid IN (" + m_checkedJobs + ")"
         " ORDER BY Path";
      if (mainWin->m_sqlDebug) {
         Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
      }
      prBar1->setValue(ontask++);
      prLabel1->setText("Task " + QString("%1").arg(ontask)+ " of " + QString("%1").arg(taskcount));
      prBar2->setValue(0);
      prLabel2->setText("Processing Directories");
      QStringList directories;
      if (m_console->sql_cmd(cmd, directories)) {
         if (mainWin->m_miscDebug) {
            Pmsg1(000, "Done with query %i directories\n", directories.count());
         }
         prBar2->setRange(0,directories.count());
         repaint();
         foreach(QString directory, directories) {
            m_debugCnt += 1;
            prBar2->setValue(m_debugCnt);
            parseDirectory(directory);
         }
      }
   } else {
     QMessageBox::warning(this, tr("Bat"),
        tr("No jobs were selected in the job query !!!.\n"
      "Press OK to continue?"),
      QMessageBox::Ok );
   }
   prBar1->setVisible(false);
   prBar2->setVisible(false);
   prLabel1->setVisible(false);
   prLabel2->setVisible(false);
}

/*
 *  Function to set m_checkedJobs from the jobs that are checked in the table
 *  of jobs
 */     
void restoreTree::setJobsCheckedList()
{
   m_JobsCheckedList = "";
   bool first = true;
   /* Update the items in the version table */
   int cnt = jobTable->rowCount();
   for (int row=0; row<cnt; row++) {
      QTableWidgetItem* jobItem = jobTable->item(row, 0);
      if (jobItem->checkState() == Qt::Checked) {
         if (!first)
            m_JobsCheckedList += ",";
         m_JobsCheckedList += jobItem->text();
         first = false;
         jobItem->setBackground(Qt::green);
      } else {
         if (jobItem->flags())
            jobItem->setBackground(Qt::gray);
         else
            jobItem->setBackground(Qt::darkYellow);
      }
   }
   m_checkedJobs = m_JobsCheckedList;
}

/*
 * Function to parse a directory into all possible subdirectories, then add to
 * The tree.
 */
void restoreTree::parseDirectory(QString &dir_in)
{
   /* m_debugTrap is to only print debugs for a few occurances of calling parseDirectory
    * instead of printing out what could potentially a whole bunch */
   if (m_debugCnt > 2)
      m_debugTrap = false;
   /* Clean up the directory string remove some funny char after last '/' */
   QRegExp rgx("[^/]$");
   int lastslash = rgx.indexIn(dir_in);
   dir_in.replace(lastslash, dir_in.length()-lastslash, "");
   if ((mainWin->m_miscDebug) && (m_debugTrap))
      Pmsg1(000, "parsing %s\n", dir_in.toUtf8().data());

   /* split and add if not in yet */
   QString direct, path;
   int index;
   bool done = false;
   QStringList pathAfter, dirAfter;
   /* start from the end, turn /etc/somedir/subdir/ into /etc/somedir and subdir/ 
    * if not added into tree, then try /etc/ and somedir/ if not added, then try
    * / and etc/ .  That should succeed, then add the ones that failed in reverse */
   while (((index = m_slashregex.lastIndexIn(dir_in, -2)) != -1) && (!done)) {
      direct = path = dir_in;
      path.replace(index+1, dir_in.length()-index-1,"");
      direct.replace(0, index+1, "");
      if ((mainWin->m_miscDebug) && (m_debugTrap)) {
         QString msg = QString("length = \"%1\" index = \"%2\" Adding \"%3\" \"%4\"\n")
                    .arg(dir_in.length()).arg(index).arg(path).arg(direct);
         Pmsg0(000, msg.toUtf8().data());
      }
      if (addDirectory(path, direct)) done = true;
      else {
         if ((mainWin->m_miscDebug) && (m_debugTrap))
            Pmsg0(000, "Saving for later\n");
         pathAfter.prepend(path);
         dirAfter.prepend(direct);
      }
      dir_in = path;
   }

   for (int k=0; k<pathAfter.count(); k++) {
      if (addDirectory(pathAfter[k], dirAfter[k]))
         if ((mainWin->m_miscDebug) && (m_debugTrap))
            Pmsg2(000, "Adding After %s %s\n", pathAfter[k].toUtf8().data(), dirAfter[k].toUtf8().data());
      else
         if ((mainWin->m_miscDebug) && (m_debugTrap))
            Pmsg2(000, "Error Adding %s %s\n", pathAfter[k].toUtf8().data(), dirAfter[k].toUtf8().data());
   }
}

/*
 * Function called from fill directory when a directory is found to see if this
 * directory exists in the directory pane and then add it to the directory pane
 */
bool restoreTree::addDirectory(QString &m_cwd, QString &newdirr)
{
   QString newdir = newdirr;
   QString fullPath = m_cwd + newdirr;
   bool ok = true, added = false;

   if ((mainWin->m_miscDebug) && (m_debugTrap)) {
      QString msg = QString("In addDirectory cwd \"%1\" newdir \"%2\"\n")
                    .arg(m_cwd)
                    .arg(newdir);
      Pmsg0(000, msg.toUtf8().data());
   }

   if (!m_slashTrap) {
      /* add unix '/' directory first */
      if (m_dirPaths.empty() && (m_winRegExpPath.indexIn(fullPath, 0) == -1)) {
         m_slashTrap = true;
         QTreeWidgetItem *item = new QTreeWidgetItem(directoryTree);
         QString text("/");
         item->setText(0, text.toUtf8().data());
         item->setData(0, Qt::UserRole, QVariant(text));
         item->setData(1, Qt::UserRole, QVariant(Qt::Unchecked));
         item->setIcon(0, QIcon(QString::fromUtf8(":images/folder.png")));
         if ((mainWin->m_miscDebug) && (m_debugTrap)) {
            Pmsg1(000, "Pre Inserting %s\n", text.toUtf8().data());
         }
         m_dirPaths.insert(text, item);
      }
      /* no need to check for windows drive if unix */
      if (m_winRegExpDrive.indexIn(m_cwd, 0) == 0) {
         if (!m_dirPaths.contains(m_cwd)) {
            /* this is a windows drive add the base widget */
            QTreeWidgetItem *item = new QTreeWidgetItem(directoryTree);
            item->setText(0, m_cwd);
            item->setData(0, Qt::UserRole, QVariant(fullPath));
            item->setData(1, Qt::UserRole, QVariant(Qt::Unchecked));
            item->setIcon(0, QIcon(QString::fromUtf8(":images/folder.png")));
            if ((mainWin->m_miscDebug) && (m_debugTrap)) {
               Pmsg0(000, "Added Base \"letter\":/\n");
            }
            m_dirPaths.insert(m_cwd, item);
         }
      }
   }
 
   /* is it already existent ?? */
   if (!m_dirPaths.contains(fullPath)) {
      QTreeWidgetItem *item = NULL;
      QTreeWidgetItem *parent = m_dirPaths.value(m_cwd);
      if (parent) {
         /* new directories to add */
         item = new QTreeWidgetItem(parent);
         item->setText(0, newdir.toUtf8().data());
         item->setData(0, Qt::UserRole, QVariant(fullPath));
         item->setCheckState(0, Qt::Unchecked);
         /* Store the current state of the check status in column 1, which at
          * this point has no text*/
         item->setData(1, Qt::UserRole, QVariant(Qt::Unchecked));
      } else {
         ok = false;
         if ((mainWin->m_miscDebug) && (m_debugTrap)) {
            QString msg = QString("In else of if parent cwd \"%1\" newdir \"%2\"\n")
                 .arg(m_cwd)
                 .arg(newdir);
            Pmsg0(000, msg.toUtf8().data());
         }
      }
      /* insert into hash */
      if (ok) {
         if ((mainWin->m_miscDebug) && (m_debugTrap)) {
            Pmsg1(000, "Inserting %s\n", fullPath.toUtf8().data());
         }
         m_dirPaths.insert(fullPath, item);
         added = true;
      }
   }
   return added;
}

/*
 * Virtual function which is called when this page is visible on the stack
 */
void restoreTree::currentStackItem()
{
   if(!m_populated) {
      if (!m_console->preventInUseConnect())
         return;
      setupPage();
      m_populated=true;
   }
}

/*
 * Populate the tree when refresh button pushed.
 */
void restoreTree::refreshButtonPushed()
{
   populateDirectoryTree();
}

/*
 * Set the values of non-job combo boxes to the job defaults
 */
void restoreTree::jobComboChanged(int)
{
   if (jobCombo->currentText() == "Any") {
      fileSetCombo->setCurrentIndex(fileSetCombo->findText("Any", Qt::MatchExactly));
      return;
   }
   job_defaults job_defs;

   (void)index;
   job_defs.job_name = jobCombo->currentText();
   if (m_console->get_job_defaults(job_defs)) {
      fileSetCombo->setCurrentIndex(fileSetCombo->findText(job_defs.fileset_name, Qt::MatchExactly));
      clientCombo->setCurrentIndex(clientCombo->findText(job_defs.client_name, Qt::MatchExactly));
   }
}

/*
 * Function to populate the file list table
 */
void restoreTree::directoryCurrentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *)
{
   if (item == NULL)
      return;

   m_fileCheckStateList.clear();
   disconnect(fileTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(fileTableItemChanged(QTableWidgetItem *)));
   QBrush blackBrush(Qt::black);
   QString directory = item->data(0, Qt::UserRole).toString();
   directoryLabel->setText("Present Working Directory : " + directory);
   QString cmd =
      "SELECT DISTINCT Filename.Name AS FileName"
      " FROM File "
      " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
      " INNER JOIN Path ON (Path.PathId=File.PathId)"
      " INNER JOIN Job ON (File.JobId=Job.JobId)"
      " WHERE Path.Path='" + directory + "' AND Filename.Name!=''"
      " AND Job.Jobid IN (" + m_checkedJobs + ")"
      " ORDER BY FileName";
 
   QStringList headerlist = (QStringList() << "File Name");
   fileTable->clear();
   /* Also clear the version table here */
   versionTable->clear();
   versionFileLabel->setText("");
   versionTable->setRowCount(0);
   versionTable->setColumnCount(0);
   fileTable->setColumnCount(headerlist.size());
   fileTable->setHorizontalHeaderLabels(headerlist);

   if (mainWin->m_sqlDebug) {
      Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
   }
   QStringList results;
   if (m_console->sql_cmd(cmd, results)) {
   
      QTableWidgetItem* tableItem;
      QString field;
      QStringList fieldlist;
      fileTable->setRowCount(results.size());

      int row = 0;
      /* Iterate through the record returned from the query */
      foreach (QString resultline, results) {
         /* Iterate through fields in the record */
         int column = 0;
         fieldlist = resultline.split("\t");
         foreach (field, fieldlist) {
            field = field.trimmed();  /* strip leading & trailing spaces */
            tableItem = new QTableWidgetItem(field, 1);
            /* Possible flags are Qt::ItemFlags flag = Qt::ItemIsSelectable | Qt::ItemIsEditablex
             *  | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | Qt::ItemIsUserCheckable 
             *  | Qt::ItemIsEnabled | Qt::ItemIsTristate; */
            tableItem->setForeground(blackBrush);
            /* Just in case a column ever gets added */
            if (column == 0) {
               Qt::ItemFlags flag = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate;
               tableItem->setFlags(flag);
               tableItem->setData(Qt::UserRole, QVariant(directory));
               fileTable->setItem(row, column, tableItem);
               m_fileCheckStateList.append(Qt::Unchecked);
               tableItem->setCheckState(Qt::Unchecked);
            }
            column++;
         }
         row++;
      }
      fileTable->setRowCount(row);
   }
   fileTable->resizeColumnsToContents();
   fileTable->resizeRowsToContents();
   fileTable->verticalHeader()->hide();
   connect(fileTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(fileTableItemChanged(QTableWidgetItem *)));
   if (mainWin->m_rtDirCurICDebug) Pmsg0(000, "will update file table checks\n");
   updateFileTableChecks();
}

/*
 * Function to populate the version table
 */
void restoreTree::fileCurrentItemChanged(QTableWidgetItem *fileTableItem, QTableWidgetItem *)
{
   if (fileTableItem == NULL)
      return;

   m_versionCheckStateList.clear();
   disconnect(versionTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(versionTableItemChanged(QTableWidgetItem *)));

   QString file = fileTableItem->text();
   versionFileLabel->setText(file);
   QString directory = fileTableItem->data(Qt::UserRole).toString();

   QBrush blackBrush(Qt::black);
   QString cmd = 
      "SELECT Job.JobId AS JobId, Job.Level AS Type, Job.EndTime AS EndTime, File.Md5 AS MD5, File.FileId AS FileId"
      " FROM File"
      " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
      " INNER JOIN Path ON (Path.PathId=File.PathId)"
      " INNER JOIN Job ON (File.JobId=Job.JobId)"
      " WHERE Filename.Name='" + file + "' AND Path.Path='" + directory + "'"
      " AND Job.Jobid IN (" + m_checkedJobs + ")"
      " ORDER BY Job.EndTime DESC";

   QStringList headerlist = (QStringList() << "Job Id" << "Type" << "End Time" << "Md5" << "FileId");
   versionTable->clear();
   versionTable->setColumnCount(headerlist.size());
   versionTable->setHorizontalHeaderLabels(headerlist);

   if (mainWin->m_sqlDebug) {
      Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
   }
   QStringList results;
   if (m_console->sql_cmd(cmd, results)) {
   
      QTableWidgetItem* tableItem;
      QString field;
      QStringList fieldlist;
      versionTable->setRowCount(results.size());

      int row = 0;
      /* Iterate through the record returned from the query */
      foreach (QString resultline, results) {
         fieldlist = resultline.split("\t");
         int column = 0;
         /* remove directory */
         if (fieldlist[0].trimmed() != "") {
            /* Iterate through fields in the record */
            foreach (field, fieldlist) {
               field = field.trimmed();  /* strip leading & trailing spaces */
               tableItem = new QTableWidgetItem(field, 1);
               tableItem->setFlags(0);
               tableItem->setForeground(blackBrush);
               tableItem->setData(Qt::UserRole, QVariant(directory));
               versionTable->setItem(row, column, tableItem);

               if (column == 0) {
                  Qt::ItemFlags flag = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate;
                  tableItem->setFlags(flag);
                  m_versionCheckStateList.append(Qt::Unchecked);
                  tableItem->setCheckState(Qt::Unchecked);
               }
               column++;
            }
            row++;
         }
      }
   }
   versionTable->resizeColumnsToContents();
   versionTable->resizeRowsToContents();
   versionTable->verticalHeader()->hide();
   connect(versionTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(versionTableItemChanged(QTableWidgetItem *)));
   updateVersionTableChecks();
}

/*
 * Save user settings associated with this page
 */
void restoreTree::writeSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("RestoreTree");
   settings.setValue("splitterSizes", splitter->saveState());
   settings.setValue("splitter_2Sizes", splitter_2->saveState());
   settings.endGroup();
}

/*
 * Read and restore user settings associated with this page
 */
void restoreTree::readSettings()
{
   QSettings settings(m_console->m_dir->name(), "bat");
   settings.beginGroup("RestoreTree");
   splitter->restoreState(settings.value("splitterSizes").toByteArray());
   splitter_2->restoreState(settings.value("splitter_2Sizes").toByteArray());
   settings.endGroup();
}

/*
 * This is a funcion to accomplish the one thing I struggled to figure out what
 * was taking so long.  It add the icons, but after the tree is made.  Seemed to
 * work fast after changing from png to png file for graphic.
 */
void restoreTree::directoryItemExpanded(QTreeWidgetItem *item)
{
   int childCount = item->childCount();
   for (int i=0; i<childCount; i++) {
      QTreeWidgetItem *child = item->child(i);
      if (child->icon(0).isNull())
         child->setIcon(0, QIcon(QString::fromUtf8(":images/folder.png")));
   }
}

/*
 * I wanted a table to show what jobs meet the criterion and are being used to
 * populate the directory tree and file and version tables.
 */
void restoreTree::populateJobTable()
{
   QBrush blackBrush(Qt::black);

   if (mainWin->m_rtPopDirDebug) Pmsg0(000, "Repopulating the Job Table\n");
   QStringList headerlist = (QStringList() << "Job Id" << "End Time" << "Level" << "Name" << "Purged" << "TU" << "TD");
   m_toggleUpIndex = headerlist.indexOf("TU");
   m_toggleDownIndex = headerlist.indexOf("TD");
   int purgedIndex = headerlist.indexOf("Purged");
   jobTable->clear();
   jobTable->setColumnCount(headerlist.size());
   jobTable->setHorizontalHeaderLabels(headerlist);
   QString jobQuery =
      "SELECT Job.Jobid AS Id, Job.EndTime AS EndTime, Job.Level AS Level, Job.Name AS JobName, Job.purgedfiles AS Purged"
      " FROM Job"
      /* INNER JOIN FileSet eliminates all restore jobs */
      " INNER JOIN Client ON (Job.ClientId=Client.ClientId)"
      " INNER JOIN FileSet ON (Job.FileSetId=FileSet.FileSetId)"
      " WHERE"
      " Client.Name='" + clientCombo->currentText() + "'";
   if ((jobCombo->currentIndex() >= 0) && (jobCombo->currentText() != "Any")) {
      jobQuery += " AND Job.name = '" + jobCombo->currentText() + "'";
   }
   if ((fileSetCombo->currentIndex() >= 0) && (fileSetCombo->currentText() != "Any")) {
      jobQuery += " AND FileSet.FileSet='" + fileSetCombo->currentText() + "'";
   }
   /* If Limit check box For limit by days is checked  */
   if (daysCheckBox->checkState() == Qt::Checked) {
      QDateTime stamp = QDateTime::currentDateTime().addDays(-daysSpinBox->value());
      QString since = stamp.toString(Qt::ISODate);
      jobQuery += " AND Job.Starttime>'" + since + "'";
   }
   //jobQuery += " AND Job.purgedfiles=0";
   jobQuery += " ORDER BY Job.EndTime DESC";
   /* If Limit check box for limit records returned is checked  */
   if (limitCheckBox->checkState() == Qt::Checked) {
      QString limit;
      limit.setNum(limitSpinBox->value());
      jobQuery += " LIMIT " + limit;
   }
   if (mainWin->m_sqlDebug)
      Pmsg1(000, "Query cmd : %s\n", jobQuery.toUtf8().data());

   QStringList results;
   if (m_console->sql_cmd(jobQuery, results)) {
   
      QTableWidgetItem* tableItem;
      QString field;
      QStringList fieldlist;
      jobTable->setRowCount(results.size());

      int row = 0;
      /* Iterate through the record returned from the query */
      foreach (QString resultline, results) {
         fieldlist = resultline.split("\t");
         int column = 0;
         /* remove directory */
         if (fieldlist[0].trimmed() != "") {
            /* Iterate through fields in the record */
            foreach (field, fieldlist) {
               field = field.trimmed();  /* strip leading & trailing spaces */
               if (field != "") {
                  tableItem = new QTableWidgetItem(field, 1);
                  tableItem->setFlags(0);
                  tableItem->setForeground(blackBrush);
                  jobTable->setItem(row, column, tableItem);
                  if (column == 0) {
                     bool ok;
                     int purged = fieldlist[purgedIndex].toInt(&ok, 10); 
                     if (!((ok) && (purged == 1))) {
                        Qt::ItemFlags flag = Qt::ItemIsUserCheckable | Qt::ItemIsEnabled | Qt::ItemIsTristate;
                        tableItem->setFlags(flag);
                        tableItem->setCheckState(Qt::Checked);
                        tableItem->setBackground(Qt::green);
                     } else {
                        tableItem->setFlags(0);
                        tableItem->setCheckState(Qt::Unchecked);
                     }
                  }
                  column++;
               }
            }
            tableItem = new QTableWidgetItem(QIcon(QString::fromUtf8(":images/go-up.png")), "", 1);
            tableItem->setFlags(0);
            tableItem->setForeground(blackBrush);
            jobTable->setItem(row, column, tableItem);
            column++;
            tableItem = new QTableWidgetItem(QIcon(QString::fromUtf8(":images/go-down.png")), "", 1);
            tableItem->setFlags(0);
            tableItem->setForeground(blackBrush);
            jobTable->setItem(row, column, tableItem);
            row++;
         }
      }
   }
   jobTable->resizeColumnsToContents();
   jobTable->resizeRowsToContents();
   jobTable->verticalHeader()->hide();
   jobTable->hideColumn(purgedIndex);
}

void restoreTree::jobTableCellClicked(int row, int column)
{
   if (column == m_toggleUpIndex){
      int cnt;
      for (cnt=0; cnt<row+1; cnt++) {
         QTableWidgetItem *item = jobTable->item(cnt, 0);
         if (item->flags()) {
            Qt::CheckState state = item->checkState();
            if (state == Qt::Checked)
               item->setCheckState(Qt::Unchecked);
            else if (state == Qt::Unchecked)
               item->setCheckState(Qt::Checked);
         }
      }
   }
   if (column == m_toggleDownIndex){
      int cnt, max = jobTable->rowCount();
      for (cnt=row; cnt<max; cnt++) {
         QTableWidgetItem *item = jobTable->item(cnt, 0);
         if (item->flags()) {
            Qt::CheckState state = item->checkState();
            if (state == Qt::Checked)
               item->setCheckState(Qt::Unchecked);
            else if (state == Qt::Unchecked)
               item->setCheckState(Qt::Checked);
         }
      }
   }
}

/*
 * When a directory item is "changed" check the state of the checkable item
 * to see if it is different than what it was which is stored in Qt::UserRole
 * of the 2nd column, column 1, of the tree widget.
 */
void restoreTree::directoryItemChanged(QTreeWidgetItem *item, int /*column*/)
{
   Qt::CheckState prevState = (Qt::CheckState)item->data(1, Qt::UserRole).toInt();
   Qt::CheckState curState = item->checkState(0);
   QTreeWidgetItem* parent = item->parent();
   Qt::CheckState parState;
   if (parent) parState = parent->checkState(0);
   else parState = (Qt::CheckState)3;
   if (mainWin->m_rtDirICDebug) {
      QString msg = QString("directory item OBJECT has changed prev=%1 cur=%2 par=%3 dir=%4\n")
         .arg(prevState).arg(curState).arg(parState).arg(item->text(0));
      Pmsg1(000, "%s", msg.toUtf8().data()); }
   /* I only care when the check state changes */
   if (prevState == curState) {
      if (mainWin->m_rtDirICDebug) Pmsg0(000, "Returning Early\n");
      return;
   }

   if ((prevState == Qt::Unchecked) && (curState == Qt::Checked) && (parState != Qt::Unchecked)) {
      if (mainWin->m_rtDirICDebug) Pmsg0(000, "Disconnected Setting to Qt::PartiallyChecked\n");
      directoryTreeDisconnectedSet(item, Qt::PartiallyChecked);
      curState = Qt::PartiallyChecked;
   }
   if ((prevState == Qt::PartiallyChecked) && (curState == Qt::Checked)) {
      if (mainWin->m_rtDirICDebug) Pmsg0(000, "Disconnected Setting to Qt::Unchecked\n");
      directoryTreeDisconnectedSet(item, Qt::Unchecked);
      curState = Qt::Unchecked;
   }
   if (mainWin->m_rtDirICDebug) {
      QString msg = QString("directory item CHECKSTATE has changed prev=%1 cur=%2 par=%3 dir=%4\n")
         .arg(prevState).arg(curState).arg(parState).arg(item->text(0));
      Pmsg1(000, "%s", msg.toUtf8().data()); }

   item->setData(1, Qt::UserRole, QVariant(curState));
   Qt::CheckState childState = curState;
   if (childState == Qt::Checked)
      childState = Qt::PartiallyChecked;
   setCheckofChildren(item, childState);

   /* Remove items from the exception lists.  The multi exception list is my index
    * of what exceptions can be removed when the directory is known*/
   QString directory = item->data(0, Qt::UserRole).toString();
   QStringList fullPathList = m_fileExceptionMulti.values(directory);
   int fullPathListCount = fullPathList.count();
   if ((mainWin->m_rtDirICDebug) && fullPathListCount) Pmsg2(000, "Will attempt to remove file exceptions for %s count %i\n", directory.toUtf8().data(), fullPathListCount);
   foreach (QString fullPath, fullPathList) {
      /* If there is no value in the hash for the key fullPath a value of 3 will be returned
       * which will match no Qt::xxx values */
      Qt::CheckState hashState = m_fileExceptionHash.value(fullPath, (Qt::CheckState)3);
      if (mainWin->m_rtDirICDebug) Pmsg2(000, "hashState=%i childState=%i\n", hashState, childState);
      if (hashState == Qt::Unchecked) {
         fileExceptionRemove(fullPath, directory);
         m_versionExceptionHash.remove(fullPath);
         if (mainWin->m_rtDirICDebug) Pmsg0(000, "Attempted Removal A\n");
      }
      if (hashState == Qt::Checked) {
         fileExceptionRemove(fullPath, directory);
         m_versionExceptionHash.remove(fullPath);
         if (mainWin->m_rtDirICDebug) Pmsg0(000, "Attempted Removal B\n");
      }
   }

   if (item == directoryTree->currentItem()) {
      if (mainWin->m_rtDirICDebug) Pmsg0(000, "Will attempt to update File Table Checks\n");
      updateFileTableChecks();
      versionTable->clear();
      versionTable->setRowCount(0);
      versionTable->setColumnCount(0);
   }
   if (mainWin->m_rtDirICDebug) Pmsg0(000, "Returning At End\n");
}

/*
 * When a directory item check state is changed, this function iterates through
 * all subdirectories and sets all to the passed state, which is either partially
 * checked or unchecked.
 */
void restoreTree::setCheckofChildren(QTreeWidgetItem *item, Qt::CheckState state)
{
   int childCount;
   childCount = item->childCount();
   for (int i=0; i<childCount; i++) {
      QTreeWidgetItem *child = item->child(i);
      child->setData(1, Qt::UserRole, QVariant(state));
      child->setCheckState(0, state);
      setCheckofChildren(child, state);
   }
}

/*
 * When a File Table Item is "changed" check to see if the state of the checkable
 * item has changed which is stored in m_fileCheckStateList
 * If changed store in a hash m_fileExceptionHash that whether this file should be
 * restored or not.
 * Called as a slot, connected after populated (after directory current changed called)
 */
void restoreTree::fileTableItemChanged(QTableWidgetItem *item)
{
   /* get the previous and current check states */
   int row = fileTable->row(item);
   Qt::CheckState prevState;
   /* prevent a segfault */
   prevState = m_fileCheckStateList[row];
   Qt::CheckState curState = item->checkState();

   /* deterimine the default state from the state of the directory */
   QTreeWidgetItem *dirTreeItem = directoryTree->currentItem();
   Qt::CheckState dirState = (Qt::CheckState)dirTreeItem->data(1, Qt::UserRole).toInt();
   Qt::CheckState defState = Qt::PartiallyChecked;
   if (dirState == Qt::Unchecked) defState = Qt::Unchecked;

   /* determine if it is already in the m_fileExceptionHash */
   QString directory = directoryTree->currentItem()->data(0, Qt::UserRole).toString();
   QString file = item->text();
   QString fullPath = directory + file;
   Qt::CheckState hashState = m_fileExceptionHash.value(fullPath, (Qt::CheckState)3);
   int verJobNum = m_versionExceptionHash.value(fullPath, 0);

   if (mainWin->m_rtFileTabICDebug) {
      QString msg = QString("filerow=%1 prev=%2 cur=%3 def=%4 hash=%5 dir=%6 verJobNum=%7\n")
         .arg(row).arg(prevState).arg(curState).arg(defState).arg(hashState).arg(dirState).arg(verJobNum);
      Pmsg1(000, "%s", msg.toUtf8().data()); }

   /* Remove the hash if currently checked previously unchecked and directory is checked or partial */
   if ((prevState == Qt::Checked) && (curState == Qt::Unchecked) && (dirState == Qt::Unchecked)) {
      /* it can behave as defaulted so current of unchecked is fine */
      if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "Will fileExceptionRemove and m_versionExceptionHash.remove here\n");
      fileExceptionRemove(fullPath, directory);
      m_versionExceptionHash.remove(fullPath);
   } else if ((prevState == Qt::PartiallyChecked) && (curState == Qt::Checked) && (dirState != Qt::Unchecked) && (verJobNum == 0)) {
      if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "Will fileExceptionInsert here\n");
      fileExceptionInsert(fullPath, directory, Qt::Unchecked);
   } else if ((prevState == Qt::Unchecked) && (curState == Qt::Checked) && (dirState != Qt::Unchecked) && (verJobNum == 0) && (defState == Qt::PartiallyChecked)) {
      /* filerow=2 prev=0 cur=2 def=1 hash=0 dir=2 verJobNum=0 */
      if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "Will fileExceptionRemove here\n");
      fileExceptionRemove(fullPath, directory);
   } else if ((prevState == Qt::Checked) && (curState == Qt::Unchecked) && (defState == Qt::PartiallyChecked) && (verJobNum != 0) && (hashState == Qt::Checked)) {
      /* Check dir, check version, attempt uncheck in file
       * filerow=4 prev=2 cur=0 def=1 hash=2 dir=2 verJobNum=53 */
      if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "Will fileExceptionRemove and m_versionExceptionHash.remove here\n");
      fileExceptionRemove(fullPath, directory);
      m_versionExceptionHash.remove(fullPath);
   } else if ((prevState == Qt::Unchecked) && (curState == Qt::Checked) && (dirState != Qt::Unchecked) && (verJobNum == 0)) {
      /* filerow=0 prev=0 cur=2 def=1 hash=0 dirState=2 verJobNum */
      if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "Will Not remove here\n");
   } else if (prevState != curState) {
      if (mainWin->m_rtFileTabICDebug) Pmsg2(000, "  THE STATE OF THE Check has changed, Setting StateList[%i] to %i\n", row, curState);
      /* A user did not set the check state to Partially checked, ignore if so */
      if (curState != Qt::PartiallyChecked) {
         if ((defState == Qt::Unchecked) && (prevState == Qt::PartiallyChecked) && (curState == Qt::Unchecked)) {
            if (mainWin->m_rtFileTabICDebug) Pmsg0(000, "  got here\n");
         } else {
            if (mainWin->m_rtFileTabICDebug) Pmsg2(000, "  Inserting into m_fileExceptionHash %s, %i\n", fullPath.toUtf8().data(), curState);
            fileExceptionInsert(fullPath, directory, curState);
         }
      } else {
         if (mainWin->m_rtFileTabICDebug) Pmsg1(000, "Removing version hash for %s\n", fullPath.toUtf8().data());
         /* programattically been changed back to a default state of Qt::PartiallyChecked remove the version hash here */
         m_versionExceptionHash.remove(fullPath);
      }
   }

   updateFileTableChecks();
   updateVersionTableChecks();
}

/*
 * function to insert keys and values to both m_fileExceptionHash and m_fileExceptionMulti
 */
void restoreTree::fileExceptionInsert(QString &fullPath, QString &direcotry, Qt::CheckState state)
{
   m_fileExceptionHash.insert(fullPath, state);
   m_fileExceptionMulti.insert(direcotry, fullPath);
   directoryIconStateInsert(fullPath, state);
}

/*
 * function to remove keys from both m_fileExceptionHash and m_fileExceptionMulti
 */
void restoreTree::fileExceptionRemove(QString &fullPath, QString &directory)
{
   m_fileExceptionHash.remove(fullPath);
   /* pull the list of values in the multi */
   QStringList fullPathList = m_fileExceptionMulti.values(directory);
   /* get the index of the fullpath to remove */
   int index = fullPathList.indexOf(fullPath);
   if (index != -1) {
      /* remove the desired item in the list */
      fullPathList.removeAt(index);
      /* remove the entire list from the multi */
      m_fileExceptionMulti.remove(directory);
      /* readd the remaining */
      foreach (QString fp, fullPathList) {
         m_fileExceptionMulti.insert(directory, fp);
      }
   }
   directoryIconStateRemove();
}

/*
 * Overloaded function to be called from the slot and from other places to set the state
 * of the check marks in the version table
 */
void restoreTree::versionTableItemChanged(QTableWidgetItem *item)
{
   /* get the previous and current check states */
   int row = versionTable->row(item);
   QTableWidgetItem *colZeroItem = versionTable->item(row, 0);
   Qt::CheckState prevState = m_versionCheckStateList[row];
   Qt::CheckState curState = (Qt::CheckState)colZeroItem->checkState();
   m_versionCheckStateList[row] = curState;

   /* deterimine the default state from the state of the file */
   QTableWidgetItem *fileTableItem = fileTable->currentItem();
   Qt::CheckState fileState = (Qt::CheckState)fileTableItem->checkState();

   /* determine the default state */
   Qt::CheckState defState;
   if (row == 0) {
      defState = Qt::PartiallyChecked;
      if (fileState == Qt::Unchecked)
         defState = Qt::Unchecked;
   }
   else
      defState = Qt::Unchecked;

   /* determine if it is already in the versionExceptionHash */
   QString directory = directoryTree->currentItem()->data(0, Qt::UserRole).toString();
   Qt::CheckState dirState = directoryTree->currentItem()->checkState(0);
   QString file = fileTableItem->text();
   QString fullPath = directory + file;
   int thisJobNum = colZeroItem->text().toInt();
   int hashJobNum = m_versionExceptionHash.value(fullPath, 0);

   if (mainWin->m_rtVerTabICDebug) {
      QString msg = QString("versrow=%1 prev=%2 cur=%3 def=%4 dir=%5 hashJobNum=%6 thisJobNum=%7 filestate=%8 fec=%9 vec=%10\n")
         .arg(row).arg(prevState).arg(curState).arg(defState).arg(dirState).arg(hashJobNum).arg(thisJobNum).arg(fileState)
         .arg(m_fileExceptionHash.count()).arg(m_versionExceptionHash.count());
      Pmsg1(000, "%s", msg.toUtf8().data()); }
   /* if changed from partially checked to checked, make it unchecked */
   if ((curState == Qt::Checked) && (row == 0) && (fileState == Qt::Unchecked)) {
      if (mainWin->m_rtVerTabICDebug) Pmsg0(000, "Setting to Qt::Checked\n");
      fileTableItem->setCheckState(Qt::Checked);
   } else if ((prevState == Qt::PartiallyChecked) && (curState == Qt::Checked) && (row == 0) && (fileState == Qt::Checked) && (dirState == Qt::Unchecked)) {
      //versrow=0 prev=1 cur=2 def=1 dir=0 hashJobNum=0 thisJobNum=64 filestate=2 fec=1 vec=0
      if (mainWin->m_rtVerTabICDebug) Pmsg1(000, "fileExceptionRemove %s, %i\n", fullPath.toUtf8().data());
      fileExceptionRemove(fullPath, directory);
   } else if ((curState == Qt::Checked) && (row == 0) && (hashJobNum != 0) && (dirState != Qt::Unchecked)) {
      //versrow=0 prev=0 cur=2 def=1 dir=2 hashJobNum=53 thisJobNum=64 filestate=2 fec=1 vec=1
      if (mainWin->m_rtVerTabICDebug) Pmsg1(000, "m_versionExceptionHash.remove %s\n", fullPath.toUtf8().data());
      m_versionExceptionHash.remove(fullPath);
      fileExceptionRemove(fullPath, directory);
   } else if ((curState == Qt::Checked) && (row == 0)) {
      if (mainWin->m_rtVerTabICDebug) Pmsg1(000, "m_versionExceptionHash.remove %s\n", fullPath.toUtf8().data());
      m_versionExceptionHash.remove(fullPath);
   } else if (prevState != curState) {
      if (mainWin->m_rtVerTabICDebug) Pmsg2(000, "  THE STATE OF THE version Check has changed, Setting StateList[%i] to %i\n", row, curState);
      if ((curState == Qt::Checked) || (curState == Qt::PartiallyChecked) && (row != 0)) {
         if (mainWin->m_rtVerTabICDebug) Pmsg2(000, "Inserting into m_versionExceptionHash %s, %i\n", fullPath.toUtf8().data(), thisJobNum);
         m_versionExceptionHash.insert(fullPath, thisJobNum);
         if (fileState != Qt::Checked) {
            if (mainWin->m_rtVerTabICDebug) Pmsg2(000, "Inserting into m_fileExceptionHash %s, %i\n", fullPath.toUtf8().data(), curState);
            fileExceptionInsert(fullPath, directory, curState);
         }
      } else {
         if (mainWin->m_rtVerTabICDebug) Pmsg0(000, "got here\n");
      }
   } else {
     if (mainWin->m_rtVerTabICDebug) Pmsg0(000, "no conditions met\n");
   }

   updateFileTableChecks();
   updateVersionTableChecks();
}

/*
 * Simple function to set the check state in the file table by disconnecting the
 * signal/slot the setting then reconnecting the signal/slot
 */
void restoreTree::fileTableDisconnectedSet(QTableWidgetItem *item, Qt::CheckState state, bool color)
{
   disconnect(fileTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(fileTableItemChanged(QTableWidgetItem *)));
   item->setCheckState(state);
   if (color) item->setBackground(Qt::yellow);
   else item->setBackground(Qt::white);
   connect(fileTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(fileTableItemChanged(QTableWidgetItem *)));
}

/*
 * Simple function to set the check state in the version table by disconnecting the
 * signal/slot the setting then reconnecting the signal/slot
 */
void restoreTree::versionTableDisconnectedSet(QTableWidgetItem *item, Qt::CheckState state)
{
   disconnect(versionTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(versionTableItemChanged(QTableWidgetItem *)));
   item->setCheckState(state);
   connect(versionTable, SIGNAL(itemChanged(QTableWidgetItem *)),
           this, SLOT(versionTableItemChanged(QTableWidgetItem *)));
}

/*
 * Simple function to set the check state in the directory tree by disconnecting the
 * signal/slot the setting then reconnecting the signal/slot
 */
void restoreTree::directoryTreeDisconnectedSet(QTreeWidgetItem *item, Qt::CheckState state)
{
   disconnect(directoryTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
           this, SLOT(directoryItemChanged(QTreeWidgetItem *, int)));
   item->setCheckState(0, state);
   connect(directoryTree, SIGNAL(itemChanged(QTreeWidgetItem *, int)),
           this, SLOT(directoryItemChanged(QTreeWidgetItem *, int)));
}

/*
 * Simplify the updating of the check state in the File table by iterating through
 * each item in the file table to determine it's appropriate state.
 * !! Will probably want to concoct a way to do this without iterating for the possibility
 * of the very large directories.
 */
void restoreTree::updateFileTableChecks()
{
   /* deterimine the default state from the state of the directory */
   QTreeWidgetItem *dirTreeItem = directoryTree->currentItem();
   Qt::CheckState dirState = dirTreeItem->checkState(0);

   QString dirName = dirTreeItem->data(0, Qt::UserRole).toString();

   /* Update the items in the version table */
   int rcnt = fileTable->rowCount();
   for (int row=0; row<rcnt; row++) {
      QTableWidgetItem* item = fileTable->item(row, 0);

      Qt::CheckState curState = item->checkState();
      Qt::CheckState newState = Qt::PartiallyChecked;
      if (dirState == Qt::Unchecked) newState = Qt::Unchecked;

      /* determine if it is already in the m_fileExceptionHash */
      QString file = item->text();
      QString fullPath = dirName + file;
      Qt::CheckState hashState = m_fileExceptionHash.value(fullPath, (Qt::CheckState)3);
      int hashJobNum = m_versionExceptionHash.value(fullPath, 0);

      if (hashState != 3) newState = hashState;

      if (mainWin->m_rtUpdateFTDebug) {
         QString msg = QString("file row=%1 cur=%2 hash=%3 new=%4 dirState=%5\n")
            .arg(row).arg(curState).arg(hashState).arg(newState).arg(dirState);
         Pmsg1(000, "%s", msg.toUtf8().data());
      }

      bool docolor = false;
      if (hashJobNum != 0) docolor = true;
      bool isyellow = item->background().color() == QColor(Qt::yellow);
      if ((newState != curState) || (hashState == 3) || ((isyellow && !docolor) || (!isyellow && docolor)))
         fileTableDisconnectedSet(item, newState, docolor);
      m_fileCheckStateList[row] = newState;
   }
}

/*
 * Simplify the updating of the check state in the Version table by iterating through
 * each item in the file table to determine it's appropriate state.
 */
void restoreTree::updateVersionTableChecks()
{
   /* deterimine the default state from the state of the directory */
   QTreeWidgetItem *dirTreeItem = directoryTree->currentItem();
   Qt::CheckState dirState = dirTreeItem->checkState(0);
   QString dirName = dirTreeItem->data(0, Qt::UserRole).toString();

   /* deterimine the default state from the state of the file */
   QTableWidgetItem *fileTableItem = fileTable->item(fileTable->currentRow(), 0);
   Qt::CheckState fileState = fileTableItem->checkState();
   QString file = fileTableItem->text();
   QString fullPath = dirName + file;
   int hashJobNum = m_versionExceptionHash.value(fullPath, 0);

   /* Update the items in the version table */
   int cnt = versionTable->rowCount();
   for (int row=0; row<cnt; row++) {
      QTableWidgetItem* item = versionTable->item(row, 0);

      Qt::CheckState curState = item->checkState();
      Qt::CheckState newState = Qt::Unchecked;

      if ((row == 0) && (fileState != Qt::Unchecked) && (hashJobNum == 0))
         newState = Qt::PartiallyChecked;
      /* determine if it is already in the versionExceptionHash */
      if (hashJobNum) {
         int thisJobNum = item->text().toInt();
         if (thisJobNum == hashJobNum)
            newState = Qt::Checked;
      }
      if (mainWin->m_rtChecksDebug) {
         QString msg = QString("ver row=%1 cur=%2 hashJobNum=%3 new=%4 dirState=%5\n")
            .arg(row).arg(curState).arg(hashJobNum).arg(newState).arg(dirState);
         Pmsg1(000, "%s", msg.toUtf8().data());
      }
      if (newState != curState)
         versionTableDisconnectedSet(item, newState);
      m_versionCheckStateList[row] = newState;
   }
}

/*
 * Quick subroutine to "return via subPaths" a list of subpaths when passed a fullPath
 */
void restoreTree::fullPathtoSubPaths(QStringList &subPaths, QString &fullPath_in)
{
   int index;
   bool done = false;
   QString fullPath = fullPath_in;
   QString direct, path;
   while (((index = m_slashregex.lastIndexIn(fullPath, -2)) != -1) && (!done)) {
      direct = path = fullPath;
      path.replace(index+1, fullPath.length()-index-1, "");
      direct.replace(0, index+1, "");
      if (false) {
         QString msg = QString("length = \"%1\" index = \"%2\" Considering \"%3\" \"%4\"\n")
                    .arg(fullPath.length()).arg(index).arg(path).arg(direct);
         Pmsg0(000, msg.toUtf8().data());
      }
      fullPath = path;
      subPaths.append(fullPath);
   }
}

/*
 * A Function to set the icon state and insert a record into
 * m_directoryIconStateHash when an exception is added by the user
 */
void restoreTree::directoryIconStateInsert(QString &fullPath, Qt::CheckState excpState)
{
   QStringList paths;
   fullPathtoSubPaths(paths, fullPath);
   /* an exception that causes the item in the file table to be "Checked" has occured */
   if (excpState == Qt::Checked) {
      bool foundAsUnChecked = false;
      QTreeWidgetItem *firstItem = m_dirPaths.value(paths[0]);
      if (firstItem) {
         if (firstItem->checkState(0) == Qt::Unchecked)
            foundAsUnChecked = true;
      }
      if (foundAsUnChecked) {
          /* as long as directory item is Unchecked, set icon state to "green check" */
         bool done = false;
         QListIterator<QString> siter(paths);
         while (siter.hasNext() && !done) {
            QString path = siter.next();
            QTreeWidgetItem *item = m_dirPaths.value(path);
            if (item) {
               if (item->checkState(0) != Qt::Unchecked)
                  done = true;
               else {
                  directorySetIcon(1, FolderGreenChecked, path, item);
                  if (mainWin->m_rtIconStateDebug) Pmsg1(000, "In restoreTree::directoryIconStateInsert inserting %s\n", path.toUtf8().data());
               }
            }
         }
      } else {
         /* if it is partially checked or fully checked insert green Check until a unchecked is found in the path */
         if (mainWin->m_rtIconStateDebug) Pmsg1(000, "In restoreTree::directoryIconStateInsert Aqua %s\n", paths[0].toUtf8().data());
         bool done = false;
         QListIterator<QString> siter(paths);
         while (siter.hasNext() && !done) {
            QString path = siter.next();
            QTreeWidgetItem *item = m_dirPaths.value(path);
            if (item) {  /* if the directory item is checked, set icon state to unchecked "green check" */
               if (item->checkState(0) == Qt::Checked)
                  done = true;
               directorySetIcon(1, FolderGreenChecked, path, item);
               if (mainWin->m_rtIconStateDebug) Pmsg1(000, "In restoreTree::directoryIconStateInsert boogie %s\n", path.toUtf8().data());
            }
         }
      }
   }
   /* an exception that causes the item in the file table to be "Unchecked" has occured */
   if (excpState == Qt::Unchecked) {
      bool done = false;
      QListIterator<QString> siter(paths);
      while (siter.hasNext() && !done) {
         QString path = siter.next();
         QTreeWidgetItem *item = m_dirPaths.value(path);
         if (item) {  /* if the directory item is checked, set icon state to unchecked "white check" */
            if (item->checkState(0) == Qt::Checked)
               done = true;
            directorySetIcon(1, FolderWhiteChecked, path, item);
            if (mainWin->m_rtIconStateDebug) Pmsg1(000, "In restoreTree::directoryIconStateInsert boogie %s\n", path.toUtf8().data());
         }
      }
   }
}

/*
 * A function to set the icon state back to "folder" and to remove a record from
 * m_directoryIconStateHash when an exception is removed by a user.
 */
void restoreTree::directoryIconStateRemove()
{
   QHash<QString, int> shouldBeIconStateHash;
   /* First determine all paths with icons that should be checked with m_fileExceptionHash */
   /* Use iterator tera to iterate through m_fileExceptionHash */
   QHashIterator<QString, Qt::CheckState> tera(m_fileExceptionHash);
   while (tera.hasNext()) {
      tera.next();
      if (mainWin->m_rtIconStateDebug) Pmsg2(000, "Alpha Key %s value %i\n", tera.key().toUtf8().data(), tera.value());

      QString keyPath = tera.key();
      Qt::CheckState state = tera.value();

      QStringList paths;
      fullPathtoSubPaths(paths, keyPath);
      /* if the state of the item in m_fileExceptionHash is checked 
       * each of the subpaths should be "Checked Green" */
      if (state == Qt::Checked) {

         bool foundAsUnChecked = false;
         QTreeWidgetItem *firstItem = m_dirPaths.value(paths[0]);
         if (firstItem) {
            if (firstItem->checkState(0) == Qt::Unchecked)
               foundAsUnChecked = true;
         }
         if (foundAsUnChecked) {
            /* The right most directory is Unchecked, iterate leftwards
             * as long as directory item is Unchecked, set icon state to "green check" */
            bool done = false;
            QListIterator<QString> siter(paths);
            while (siter.hasNext() && !done) {
               QString path = siter.next();
               QTreeWidgetItem *item = m_dirPaths.value(path);
               if (item) {
                  if (item->checkState(0) != Qt::Unchecked)
                     done = true;
                  else {
                     shouldBeIconStateHash.insert(path, FolderGreenChecked);
                     if (mainWin->m_rtIconStateDebug) Pmsg1(000, "In restoreTree::directoryIconStateInsert inserting %s\n", path.toUtf8().data());
                  }
               }
            }
         }
         else {
            /* The right most directory is Unchecked, iterate leftwards
             * until directory item is Checked, set icon state to "green check" */
            bool done = false;
            QListIterator<QString> siter(paths);
            while (siter.hasNext() && !done) {
               QString path = siter.next();
               QTreeWidgetItem *item = m_dirPaths.value(path);
               if (item) {
                  if (item->checkState(0) == Qt::Checked)
                     done = true;
                  shouldBeIconStateHash.insert(path, FolderGreenChecked);
               }
            }
         }
      }
      /* if the state of the item in m_fileExceptionHash is UNChecked
       * each of the subpaths should be "Checked white" until the tree item
       * which represents that path is Qt::Checked */
      if (state == Qt::Unchecked) {
         bool done = false;
         QListIterator<QString> siter(paths);
         while (siter.hasNext() && !done) {
            QString path = siter.next();
            QTreeWidgetItem *item = m_dirPaths.value(path);
            if (item) {
               if (item->checkState(0) == Qt::Checked)
                  done = true;
               shouldBeIconStateHash.insert(path, FolderWhiteChecked);
            }
         }
      }
   }
   /* now iterate through m_directoryIconStateHash which are the items that are checked
    * and remove all of those that are not in shouldBeIconStateHash */
   QHashIterator<QString, int> iter(m_directoryIconStateHash);
   while (iter.hasNext()) {
      iter.next();
      if (mainWin->m_rtIconStateDebug) Pmsg2(000, "Beta Key %s value %i\n", iter.key().toUtf8().data(), iter.value());

      QString keyPath = iter.key();
      if (shouldBeIconStateHash.value(keyPath)) {
         if (mainWin->m_rtIconStateDebug) Pmsg1(000, "WAS found in shouldBeStateHash %s\n", keyPath.toUtf8().data());
         //newval = m_directoryIconStateHash.value(path, FolderUnchecked) & (~change);
         int newval = shouldBeIconStateHash.value(keyPath);
         newval = ~newval;
         newval = newval & FolderBothChecked;
         QTreeWidgetItem *item = m_dirPaths.value(keyPath);
         if (item)
            directorySetIcon(0, newval, keyPath, item);
      } else {
         if (mainWin->m_rtIconStateDebug) Pmsg1(000, "NOT found in shouldBeStateHash %s\n", keyPath.toUtf8().data());
         QTreeWidgetItem *item = m_dirPaths.value(keyPath);
         if (item)
            directorySetIcon(0, FolderBothChecked, keyPath, item);
            //item->setIcon(0,QIcon(QString::fromUtf8(":images/folder.png")));
            //m_directoryIconStateHash.remove(keyPath);
      }
   }
}

void restoreTree::directorySetIcon(int operation, int change, QString &path, QTreeWidgetItem* item) {
   int newval;
   /* we are adding a check type white or green */
   if (operation > 0) {
      /* get the old val and "bitwise OR" with the change */
      newval = m_directoryIconStateHash.value(path, FolderUnchecked) | change;
      if (mainWin->m_rtIconStateDebug) Pmsg2(000, "Inserting into m_directoryIconStateHash path=%s newval=%i\n", path.toUtf8().data(), newval);
      m_directoryIconStateHash.insert(path, newval);
   } else {
   /* we are removing a check type white or green */
      newval = m_directoryIconStateHash.value(path, FolderUnchecked) & (~change);
      if (newval == 0) {
         if (mainWin->m_rtIconStateDebug) Pmsg2(000, "Removing from m_directoryIconStateHash path=%s newval=%i\n", path.toUtf8().data(), newval);
         m_directoryIconStateHash.remove(path);
      }
      else {
         if (mainWin->m_rtIconStateDebug) Pmsg2(000, "Inserting into m_directoryIconStateHash path=%s newval=%i\n", path.toUtf8().data(), newval);
         m_directoryIconStateHash.insert(path, newval);
      }
   }
   if (newval == FolderUnchecked)
      item->setIcon(0, QIcon(QString::fromUtf8(":images/folder.png")));
   else if (newval == FolderGreenChecked)
      item->setIcon(0, QIcon(QString::fromUtf8(":images/folderchecked.png")));
   else if (newval == FolderWhiteChecked)
      item->setIcon(0, QIcon(QString::fromUtf8(":images/folderunchecked.png")));
   else if (newval == FolderBothChecked)
      item->setIcon(0, QIcon(QString::fromUtf8(":images/folderbothchecked.png")));
}

/*
 * Test Button
 */
void restoreTree::restoreButtonPushed()
{
   /* Set progress bars and repaint */
   prLabel1->setVisible(true);
   prLabel1->setText("Task 1 of 3");
   prLabel2->setVisible(true);
   prLabel2->setText("Processing Checked directories");
   prBar1->setVisible(true);
   prBar1->setRange(0, 3);
   prBar1->setValue(0);
   prBar2->setVisible(true);
   prBar2->setRange(0, 0);
   repaint();
   QMultiHash<int, QString> versionFilesMulti;
   int vFMCounter = 0;
   QHash <QString, bool> fullPathDone;
   QHash <QString, int> fileIndexHash;
   if ((mainWin->m_rtRestore1Debug) || (mainWin->m_rtRestore2Debug) || (mainWin->m_rtRestore3Debug))
      Pmsg0(000, "In restoreTree::restoreButtonPushed\n");
   /* Use a tree widget item iterator to count directories for the progress bar */
   QTreeWidgetItemIterator diterc(directoryTree, QTreeWidgetItemIterator::Checked);
   int ditcount = 0;
   while (*diterc) {
      ditcount += 1;
      ++diterc;
   } /* while (*diterc) */
   prBar2->setRange(0, ditcount);
   prBar2->setValue(0);
   ditcount = 0;
   /* Use a tree widget item iterator filtering for Checked Items */
   QTreeWidgetItemIterator diter(directoryTree, QTreeWidgetItemIterator::Checked);
   while (*diter) {
      QString directory = (*diter)->data(0, Qt::UserRole).toString();
      if (mainWin->m_rtRestore1Debug)
      Pmsg1(000, "Directory Checked=\"%s\"\n", directory.toUtf8().data());
      /* With a checked directory, query for the files in the directory */

      QString cmd =
         "SELECT t1.Filename AS Filename, t1.JobId AS JobId, File.FileIndex AS FileIndex"
         " FROM"
         " ( SELECT Filename.Name AS Filename, MAX(Job.JobId) AS JobId"
           " FROM File"
             " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
             " INNER JOIN Path ON (Path.PathId=File.PathId)"
             " INNER JOIN Job ON (Job.JobId=File.JobId)"
           " WHERE Path.Path='" + directory + "' AND Filename.Name!=''"
           "  AND Job.Jobid IN (" + m_checkedJobs + ")"
           " GROUP BY Filename.Name"
         ") t1, File "
           " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
           " INNER JOIN Path ON (Path.PathId=File.PathId)"
           " INNER JOIN Job ON (Job.JobId=File.JobId)"
         " WHERE"
           " Path.Path='" + directory + "'"
           " AND Filename.Name=t1.Filename"
           " AND Job.Jobid=t1.JobId"
         " ORDER BY Filename";

      if (mainWin->m_sqlDebug) Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
      QStringList results;
      if (m_console->sql_cmd(cmd, results)) {
         QStringList fieldlist;
   
         int row = 0;
         /* Iterate through the record returned from the query */
         foreach (QString resultline, results) {
            /* Iterate through fields in the record */
            int column = 0;
            QString fullPath = "";
            Qt::CheckState fileExcpState = (Qt::CheckState)4;
            fieldlist = resultline.split("\t");
            int version = 0;
            int fileIndex = 0;
            foreach (QString field, fieldlist) {
               if (column == 0) {
                  fullPath = directory + field;
               }
               if (column == 1) {
                  version = field.toInt();
               }
               if (column == 2) {
                  fileIndex = field.toInt();
               }
               column++;
            }
            fileExcpState = m_fileExceptionHash.value(fullPath, (Qt::CheckState)3);
            
            int excpVersion = m_versionExceptionHash.value(fullPath, 0);
            if (fileExcpState != Qt::Unchecked) {
               QString debugtext;
               if (excpVersion != 0) {
                  debugtext = QString("*E* version=%1").arg(excpVersion);
                  version = excpVersion;
                  fileIndex = queryFileIndex(fullPath, excpVersion);
               } else
                  debugtext = QString("___ version=%1").arg(version);
               if (mainWin->m_rtRestore1Debug)
                  Pmsg2(000, "Restoring %s File %s\n", debugtext.toUtf8().data(), fullPath.toUtf8().data());
               fullPathDone.insert(fullPath, 1);
               fileIndexHash.insert(fullPath, fileIndex);
               versionFilesMulti.insert(version, fullPath);
               vFMCounter += 1;
            }
            row++;
         }
      }
      ditcount += 1;
      prBar2->setValue(ditcount);
      ++diter;
   } /* while (*diter) */
   prBar1->setValue(1);
   prLabel1->setText("Task 2 of 3");
   prLabel2->setText("Processing Exceptions");
   prBar2->setRange(0, 0);
   repaint();

   /* There may be some exceptions not accounted for yet with fullPathDone */
   QHashIterator<QString, Qt::CheckState> ftera(m_fileExceptionHash);
   while (ftera.hasNext()) {
      ftera.next();
      QString fullPath = ftera.key();
      Qt::CheckState state = ftera.value();
      if (state != 0) {
         /* now we don't want the ones already done */
         if (fullPathDone.value(fullPath, 0) == 0) {
            int version = m_versionExceptionHash.value(fullPath, 0);
            int fileIndex = 0;
            QString debugtext = "";
            if (version != 0) {
               fileIndex = queryFileIndex(fullPath, version);
               debugtext = QString("E1* version=%1 fileid=%2").arg(version).arg(fileIndex);
            } else {
               version = mostRecentVersionfromFullPath(fullPath);
               if (version) {
                  fileIndex = queryFileIndex(fullPath, version);
                  debugtext = QString("E2* version=%1 fileid=%2").arg(version).arg(fileIndex);
               } else
                  debugtext = QString("Error det vers").arg(version);
            }
            if (mainWin->m_rtRestore1Debug)
               Pmsg2(000, "Restoring %s file %s\n", debugtext.toUtf8().data(), fullPath.toUtf8().data());
            versionFilesMulti.insert(version, fullPath);
            vFMCounter += 1;
            fileIndexHash.insert(fullPath, fileIndex);
         } /* if fullPathDone.value(fullPath, 0) == 0 */
      } /* if state != 0 */
   } /* while ftera.hasNext */
   /* The progress bars for the next step */
   prBar1->setValue(2);
   prLabel1->setText("Task 3 of 3");
   prLabel2->setText("Filling Database Table");
   prBar2->setRange(0, vFMCounter);
   vFMCounter = 0;
   prBar2->setValue(vFMCounter);
   repaint();

   /* now for the final spit out of the versions and lists of files for each version */
   QHash<int, int> doneKeys;
   QHashIterator<int, QString> vFMiter(versionFilesMulti);
   QString tempTable = "";
   QList<int> jobList;
   while (vFMiter.hasNext()) {
      vFMiter.next();
      int fversion = vFMiter.key();
      /* did not succeed in getting an iterator to work as expected on versionFilesMulti so use doneKeys */
      if (doneKeys.value(fversion, 0) == 0) {
         if (tempTable == "") {
            QSettings settings("www.bacula.org", "bat");
            settings.beginGroup("Restore");
            int counter = settings.value("Counter", 1).toInt();
            settings.setValue("Counter", counter+1);
            settings.endGroup();
            tempTable = "restore_" + QString("%1").arg(qrand()) + "_" + QString("%1").arg(counter);
            QString sqlcmd = "CREATE TEMPORARY TABLE " + tempTable + " (JobId INTEGER, FileIndex INTEGER)";
            if (mainWin->m_sqlDebug)
               Pmsg1(000, "Query cmd : %s ;\n", sqlcmd.toUtf8().data());
            QStringList results;
            if (!m_console->sql_cmd(sqlcmd, results))
               Pmsg1(000, "CREATE TABLE FAILED!!!! %s\n", sqlcmd.toUtf8().data());
         }

         if (mainWin->m_rtRestore2Debug) Pmsg1(000, "Version->%i\n", fversion);
         QStringList fullPathList = versionFilesMulti.values(fversion);
         /* create the command to perform the restore */
         foreach(QString ffullPath, fullPathList) {
            int fileIndex = fileIndexHash.value(ffullPath);
            if (mainWin->m_rtRestore2Debug) Pmsg2(000, "  file->%s id %i\n", ffullPath.toUtf8().data(), fileIndex);
            QString sqlcmd = "INSERT INTO " + tempTable + " (JobId, FileIndex) VALUES (" + QString("%1").arg(fversion) + ", " + QString("%1").arg(fileIndex) + ")";
            if (mainWin->m_rtRestore3Debug)
               Pmsg1(000, "Insert cmd : %s\n", sqlcmd.toUtf8().data());
            QStringList results;
            if (!m_console->sql_cmd(sqlcmd, results))
               Pmsg1(000, "INSERT INTO FAILED!!!! %s\n", sqlcmd.toUtf8().data());
            prBar2->setValue(++vFMCounter);
         } /* foreach fullPathList */
         doneKeys.insert(fversion,1);
         jobList.append(fversion);
      } /*  if (doneKeys.value(fversion, 0) == 0) */
   } /* while (vFMiter.hasNext()) */
   if (tempTable != "") {
      /* a table was made, lets run the job */
      QString jobOption = " jobid=\"";
      bool first = true;
      /* create a list of jobs comma separated */
      foreach (int job, jobList) {
         if (first) first = false;
         else jobOption += ",";
         jobOption += QString("%1").arg(job);
      }
      jobOption += "\"";
      QString cmd = QString("restore");
      cmd += jobOption +
             " client=\"" + m_prevClientCombo + "\"" +
             " file=\"?" + tempTable + "\" done";
      if (mainWin->m_commandDebug)
         Pmsg1(000, "preRestore command \'%s\'\n", cmd.toUtf8().data());
      consoleCommand(cmd);
   }
   /* turn off the progress widgets */
   prBar1->setVisible(false);
   prBar2->setVisible(false);
   prLabel1->setVisible(false);
   prLabel2->setVisible(false);
}

int restoreTree::mostRecentVersionfromFullPath(QString &fullPath)
{
   int qversion = 0;
   QString directory, fileName;
   int index = m_slashregex.lastIndexIn(fullPath, -2);
   if (index != -1) {
      directory = fileName = fullPath;
      directory.replace(index+1, fullPath.length()-index-1, "");
      fileName.replace(0, index+1, "");
      if (false) {
         QString msg = QString("length = \"%1\" index = \"%2\" Considering \"%3\" \"%4\"\n")
                    .arg(fullPath.length()).arg(index).arg(fileName).arg(directory);
         Pmsg0(000, msg.toUtf8().data());
      }
      /* so now we need the latest version from the database */
      QString cmd =
         "SELECT MAX(Job.JobId)"
         " FROM File "
         " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
         " INNER JOIN Path ON (Path.PathId=File.PathId)"
         " INNER JOIN Job ON (File.JobId=Job.JobId)"
         " WHERE Path.Path='" + directory + "' AND Filename.Name!=''"
         " AND Job.Jobid IN (" + m_checkedJobs + ")"
         " AND Filename.Name='" + fileName + "'"
         " GROUP BY Filename.Name";
 
      if (mainWin->m_sqlDebug) Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
      QStringList results;
      if (m_console->sql_cmd(cmd, results)) {
         QStringList fieldlist;
         int row = 0;
         /* Iterate through the record returned from the query */
         foreach (QString resultline, results) {
            /* Iterate through fields in the record */
            int column = 0;
            fieldlist = resultline.split("\t");
            foreach (QString field, fieldlist) {
               if (column == 0) {
                  qversion = field.toInt();
               }
               column++;
            }
            row++;
         }
      }
   } /* if (index != -1) */
   return qversion;
}


int restoreTree::queryFileIndex(QString &fullPath, int jobId)
{
   int qfileIndex = 0;
   QString directory, fileName;
   int index = m_slashregex.lastIndexIn(fullPath, -2);
   if (index != -1) {
      directory = fileName = fullPath;
      directory.replace(index+1, fullPath.length()-index-1, "");
      fileName.replace(0, index+1, "");
      if (false) {
         QString msg = QString("length = \"%1\" index = \"%2\" Considering \"%3\" \"%4\"\n")
                    .arg(fullPath.length()).arg(index).arg(fileName).arg(directory);
         Pmsg0(000, msg.toUtf8().data());
      }
      /* so now we need the latest version from the database */
      QString cmd =
         "SELECT"
          " File.FileIndex"
         " FROM File"
          " INNER JOIN Filename on (Filename.FilenameId=File.FilenameId)"
          " INNER JOIN Path ON (Path.PathId=File.PathId)"
          " INNER JOIN Job ON (File.JobId=Job.JobId)"
         " WHERE"
          " Path.Path='" + directory + "'"
          " AND Filename.Name='" + fileName + "'"
          " AND Job.Jobid='" + QString("%1").arg(jobId) + "'"
         " GROUP BY File.FileIndex";
 
      if (mainWin->m_sqlDebug) Pmsg1(000, "Query cmd : %s\n", cmd.toUtf8().data());
      QStringList results;
      if (m_console->sql_cmd(cmd, results)) {
         QStringList fieldlist;
         int row = 0;
         /* Iterate through the record returned from the query */
         foreach (QString resultline, results) {
            /* Iterate through fields in the record */
            int column = 0;
            fieldlist = resultline.split("\t");
            foreach (QString field, fieldlist) {
               if (column == 0) {
                  qfileIndex = field.toInt();
               }
               column++;
            }
            row++;
         }
      }
   } /* if (index != -1) */
   return qfileIndex;
}
