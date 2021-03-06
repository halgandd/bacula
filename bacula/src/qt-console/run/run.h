
#ifndef _RUN_H_
#define _RUN_H_

#include <QtGui>
#include "ui_run.h"
#include "ui_runcmd.h"
#include "ui_estimate.h"
#include "ui_prune.h"
#include "console.h"

class runPage : public Pages, public Ui::runForm
{
   Q_OBJECT 

public:
   runPage(const QString &defJob);

public slots:
   void okButtonPushed();
   void cancelButtonPushed();
   void job_name_change(int index);

private:
   int m_conn;
};

class runCmdPage : public Pages, public Ui::runCmdForm
{
   Q_OBJECT 

public:
   runCmdPage(int conn);

public slots:
   void okButtonPushed();
   void cancelButtonPushed();

private:
   void fill();
   int m_conn;
};

class estimatePage : public Pages, public Ui::estimateForm
{
   Q_OBJECT 

public:
   estimatePage();

public slots:
   void okButtonPushed();
   void cancelButtonPushed();
   void job_name_change(int index);

private:
   int m_conn;
   bool m_aButtonPushed;
};

class prunePage : public Pages, public Ui::pruneForm
{
   Q_OBJECT 

public:
   prunePage(const QString &volume, const QString &client);

public slots:
   void okButtonPushed();
   void cancelButtonPushed();
   void volumeChanged();
   void clientChanged();

private:
   int m_conn;
};

#endif /* _RUN_H_ */
