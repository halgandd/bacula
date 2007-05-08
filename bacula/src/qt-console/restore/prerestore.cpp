/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2007-2007 Free Software Foundation Europe e.V.

   The main author of Bacula is Kern Sibbald, with contributions from
   many others, a complete list can be found in the file AUTHORS.
   This program is Free Software; you can redistribute it and/or
   modify it under the terms of version two of the GNU General Public
   License as published by the Free Software Foundation plus additions
   that are listed in the file LICENSE.

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
 *   Version $Id: restore.cpp 4307 2007-03-04 10:24:39Z kerns $
 *
 *  preRestore -> dialog put up to determine the restore type
 *
 *   Kern Sibbald, February MMVII
 *
 */ 

#include "bat.h"
#include "restore.h"

/* Constructor to have job id list default in */
prerestorePage::prerestorePage(QString &jobIdString)
{
   m_jobIdListIn = jobIdString;
   buildPage();
}

/* Basic Constructor */
prerestorePage::prerestorePage()
{
   m_jobIdListIn = "";
   buildPage();
}

/*
 * This is really the constructor
 */
void prerestorePage::buildPage()
{
   m_dtformat = "yyyy-MM-dd HH:MM:ss";
   m_name = "Restore";
   setupUi(this);
   pgInitialize();
   m_console->notify(false);
   m_closeable = true;

   jobCombo->addItems(m_console->job_list);
   filesetCombo->addItems(m_console->fileset_list);
   clientCombo->addItems(m_console->client_list);
   poolCombo->addItem("Any");
   poolCombo->addItems(m_console->pool_list);
   storageCombo->addItems(m_console->storage_list);
   /* current or before . .  Start out with current checked */
   recentCheckBox->setCheckState(Qt::Checked);
   beforeDateTime->setDisplayFormat(m_dtformat);
   beforeDateTime->setDateTime(QDateTime::currentDateTime());
   beforeDateTime->setEnabled(false);
   selectFilesRadio->setChecked(true);
   if (m_jobIdListIn == "") {
      selectJobsRadio->setChecked(true);
      jobIdEdit->setText("Comma separted list of jobs id's");
      jobIdEdit->setEnabled(false);
   } else {
      listJobsRadio->setChecked(true);
      jobIdEdit->setText(m_jobIdListIn);
      jobsRadioClicked(false);
      QStringList fieldlist;
      jobdefsFromJob(fieldlist,m_jobIdListIn);
      filesetCombo->setCurrentIndex(filesetCombo->findText(fieldlist[2], Qt::MatchExactly));
      clientCombo->setCurrentIndex(clientCombo->findText(fieldlist[1], Qt::MatchExactly));
      jobCombo->setCurrentIndex(jobCombo->findText(fieldlist[0], Qt::MatchExactly));
   }
   job_name_change(0);
   connect(jobCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(job_name_change(int)));
   connect(okButton, SIGNAL(pressed()), this, SLOT(okButtonPushed()));
   connect(cancelButton, SIGNAL(pressed()), this, SLOT(cancelButtonPushed()));
   connect(recentCheckBox, SIGNAL(stateChanged(int)), this, SLOT(recentChanged(int)));
   connect(selectJobsRadio, SIGNAL(toggled(bool)), this, SLOT(jobsRadioClicked(bool)));
   connect(jobIdEdit, SIGNAL(editingFinished()), this, SLOT(jobIdEditFinished()));

   dockPage();
   setCurrent();
   this->show();
}


/*
 * Check to make sure all is ok then start either the select window or the restore
 * run window
 */
void prerestorePage::okButtonPushed()
{
   if (!selectJobsRadio->isChecked()) {
      if (!checkJobIdList())
         return;
   }
   QString cmd;

   this->hide();

   cmd = QString("restore ");
   if (selectFilesRadio->isChecked()) {
      cmd += "select ";
   } else {
      cmd += "all done ";
   }
   cmd += "fileset=\"" + filesetCombo->currentText() + "\" ";
   cmd += "client=\"" + clientCombo->currentText() + "\" ";
   if (selectJobsRadio->isChecked()) {
      if (poolCombo->currentText() != "Any" ){
         cmd += "pool=\"" + poolCombo->currentText() + "\" ";
      }
      cmd += "storage=\"" + storageCombo->currentText() + "\" ";
      if (recentCheckBox->checkState() == Qt::Checked) {
         cmd += " current";
      } else {
         QDateTime stamp = beforeDateTime->dateTime();
         QString before = stamp.toString(m_dtformat);
         cmd += " before=\"" + before + "\"";
      }
   } else {
      cmd += "jobid=\"" + jobIdEdit->text() + "\"";
   }

   /* ***FIXME*** */
   //printf("preRestore command \'%s\'\n", cmd.toUtf8().data());
   consoleCommand(cmd);
   /* Note, do not turn notifier back on here ... */
   if (selectFilesRadio->isChecked()) {
      setConsoleCurrent();
      new restorePage();
      closeStackPage();
   } else {
      m_console->notify(true);
      closeStackPage();
      mainWin->resetFocus();
   }
}


/*
 * Destroy the instace of the class
 */
void prerestorePage::cancelButtonPushed()
{
   mainWin->set_status("Canceled");
   this->hide();
   m_console->notify(true);
   closeStackPage();
}


/*
 * Handle updating the other widget with job defaults when the job combo is changed.
 */
void prerestorePage::job_name_change(int index)
{
   job_defaults job_defs;

   (void)index;
   job_defs.job_name = jobCombo->currentText();
   if (m_console->get_job_defaults(job_defs)) {
      filesetCombo->setCurrentIndex(filesetCombo->findText(job_defs.fileset_name, Qt::MatchExactly));
      clientCombo->setCurrentIndex(clientCombo->findText(job_defs.client_name, Qt::MatchExactly));
      poolCombo->setCurrentIndex(poolCombo->findText(job_defs.pool_name, Qt::MatchExactly));
      storageCombo->setCurrentIndex(storageCombo->findText(job_defs.store_name, Qt::MatchExactly));
   }
}

/*
 * Handle the change of enabled of input widgets when the recent checkbox state
 * is changed.
 */
void prerestorePage::recentChanged(int state)
{
   if ((state == Qt::Unchecked) && (selectJobsRadio->isChecked())) {
      beforeDateTime->setEnabled(true);
   } else {
      beforeDateTime->setEnabled(false);
   }
}

/*
 * Handle the change of enabled of input widgets when the job radio buttons
 * are changed.
 */
void prerestorePage::jobsRadioClicked(bool checked)
{
   if (checked) {
      jobCombo->setEnabled(true);
      filesetCombo->setEnabled(true);
      clientCombo->setEnabled(true);
      poolCombo->setEnabled(true);
      storageCombo->setEnabled(true);
      recentCheckBox->setEnabled(true);
      if (!recentCheckBox->isChecked()) {
         beforeDateTime->setEnabled(true);
      }
      jobIdEdit->setEnabled(false);
   } else {
      jobCombo->setEnabled(false);
      filesetCombo->setEnabled(false);
      clientCombo->setEnabled(false);
      poolCombo->setEnabled(false);
      storageCombo->setEnabled(false);
      recentCheckBox->setEnabled(false);
      beforeDateTime->setEnabled(false);
      jobIdEdit->setEnabled(true);
   }
}

/*
 * For when jobs list is to be used, return a list which is the needed items from
 * the job record
 */
void prerestorePage::jobdefsFromJob(QStringList &fieldlist, QString jobId)
{
   QString job, client, fileset;
   QString query("");
   query = "SELECT DISTINCT Job.Name AS JobName, Client.Name AS Client, Fileset.Fileset AS Fileset "
   " From Job, Client, Fileset"
   " WHERE Job.FilesetId=FileSet.FilesetId AND Job.ClientId=Client.ClientId"
   " AND JobId=\'" + jobId + "\'";
   //printf("query = %s\n", query.toUtf8().data());
   QStringList results;
   if (m_console->sql_cmd(query, results)) {
      QString field;

      /* Iterate through the lines of results, there should only be one. */
      foreach (QString resultline, results) {
         fieldlist = resultline.split("\t");
      } /* foreach resultline */
   } /* if results from query */
}

/*
 * Function to handle when the jobidlist line edit input loses focus or is entered
 */
void prerestorePage::jobIdEditFinished()
{
   checkJobIdList();
}

bool prerestorePage::checkJobIdList()
{
   /* Need to check and make sure the text is a comma separated list of integers */
   QString line = jobIdEdit->text();
   if (line.contains(" ")) {
      QMessageBox::warning(this, tr("Bat"),
         tr("There can be no spaces in the text for the joblist.\n"
         "Press OK to continue?"), QMessageBox::Ok );
      return false;
   }
   //printf("In prerestorePage::jobIdEditFinished %s\n",line.toUtf8().data());
   QStringList joblist = line.split(",", QString::SkipEmptyParts);
   bool allintokay = true, alljobok = true, allisjob = true;
   QString jobName(""), clientName("");
   foreach (QString job, joblist) {
      bool intok;
      job.toInt(&intok, 10);
      if (intok) {
         /* are the intergers representing a list of jobs all with the same job
          * and client */
         QStringList fields;
         jobdefsFromJob(fields, job);
         int count = fields.count();
         if (count > 0) {
            if (jobName == "")
               jobName = fields[0];
            else if (jobName != fields[0])
               alljobok = false;
            if (clientName == "")
               clientName = fields[1];
            else if (clientName != fields[1])
               alljobok = false;
         } else {
            allisjob = false;
         }
      } else {
         allintokay = false;
      }
   }
   if (!allintokay){
      QMessageBox::warning(this, tr("Bat"),
         tr("The string is not a comma separated list if integers.\n"
         "Press OK to continue?"), QMessageBox::Ok );
      return false;
   }
   if (!allisjob){
      QMessageBox::warning(this, tr("Bat"),
         tr("At least one of the jobs is not a valid job.\n"
         "Press OK to continue?"), QMessageBox::Ok );
      return false;
   }
   if (!alljobok){
      QMessageBox::warning(this, tr("Bat"),
         tr("All jobs in the list must be of the same jobName and same client.\n"
         "Press OK to continue?"), QMessageBox::Ok );
      return false;
   }
   return true;
}

