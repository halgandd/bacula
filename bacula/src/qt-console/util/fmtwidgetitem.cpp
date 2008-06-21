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
 *  Helper functions for tree widget formatting
 *
 *   Riccardo Ghetta, May 2008
 *
 */ 

#include <QTreeWidgetItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QBrush>
#include <QString>
#include <QStringList>
#include <math.h>
#include "bat.h"
#include "fmtwidgetitem.h"

/***********************************************
 *
 * common helpers
 *
 ***********************************************/

QString convertJobStatus(const QString &sts)
{
   QString code( sts.trimmed() );
   if ( code.size() != 1) {
      return QObject::tr("Invalid job status %1").arg(sts);
   }

   char buf[256];
   jobstatus_to_ascii_gui( code[0].toAscii(), buf, sizeof(buf));
   return QString(buf);
}

/***********************************************
 *
 * ItemFormatterBase static members
 *
 ***********************************************/

ItemFormatterBase::BYTES_CONVERSION ItemFormatterBase::cnvFlag(BYTES_CONVERSION_IEC);

QString ItemFormatterBase::convertBytesIEC(qint64 qfld)
{
   static const qint64 KB = Q_INT64_C(1024);
   static const qint64 MB = (KB * KB);
   static const qint64 GB = (MB * KB);
   static const qint64 TB = (GB * KB);
   static const qint64 PB = (TB * KB);
   static const qint64 EB = (PB * KB);

   /* note: division is integer, so to have some decimals we divide for a
      smaller unit (e.g. GB for a TB number and so on) */
   char suffix;
   if (qfld >= EB) {
      qfld /= PB; 
      suffix = 'E';
   }
   else if (qfld >= PB) {
      qfld /= TB; 
      suffix = 'P';
   }
   else if (qfld >= TB) {
      qfld /= GB; 
      suffix = 'T';
   }
   else if (qfld >= GB) {
      qfld /= MB;
      suffix = 'G';
   }
   else if (qfld >= MB) {
      qfld /= KB;
      suffix = 'M';
   }
   else if (qfld >= KB) {
      suffix = 'K';
   }
   else  {
      /* plain bytes, no need to reformat */
      return QString("%1 B").arg(qfld); 
   }

   /* having divided for a smaller unit, now we can safely convert to double and
      use the extra room for decimals */
   return QString("%1 %2iB").arg(qfld / 1000.0, 0, 'f', 2).arg(suffix);
}

QString ItemFormatterBase::convertBytesSI(qint64 qfld)
{
   static const qint64 KB = Q_INT64_C(1000);
   static const qint64 MB = (KB * KB);
   static const qint64 GB = (MB * KB);
   static const qint64 TB = (GB * KB);
   static const qint64 PB = (TB * KB);
   static const qint64 EB = (PB * KB);

   /* note: division is integer, so to have some decimals we divide for a
      smaller unit (e.g. GB for a TB number and so on) */
   char suffix;
   if (qfld >= EB) {
      qfld /= PB; 
      suffix = 'E';
   }
   else if (qfld >= PB) {
      qfld /= TB; 
      suffix = 'P';
   }
   else if (qfld >= TB) {
      qfld /= GB; 
      suffix = 'T';
   }
   else if (qfld >= GB) {
      qfld /= MB;
      suffix = 'G';
   }
   else if (qfld >= MB) {
      qfld /= KB;
      suffix = 'M';
   }
   else if (qfld >= KB) {
      suffix = 'k'; /* SI uses lowercase k */
   }
   else  {
      /* plain bytes, no need to reformat */
      return QString("%1 B").arg(qfld); 
   }

   /* having divided for a smaller unit, now we can safely convert to double and
      use the extra room for decimals */
   return QString("%1 %2B").arg(qfld / 1000.0, 0, 'f', 2).arg(suffix);
}

/***********************************************
 *
 * base formatting routines
 *
 ***********************************************/

ItemFormatterBase::ItemFormatterBase()
{
}

ItemFormatterBase::~ItemFormatterBase()
{
}

void ItemFormatterBase::setTextFld(int index, const QString &fld, bool center)
{
   setText(index, fld.trimmed());
   if (center) {
      setTextAlignment(index, Qt::AlignCenter);
   }
}

void ItemFormatterBase::setRightFld(int index, const QString &fld)
{
   setText(index, fld.trimmed());
   setTextAlignment(index, Qt::AlignRight | Qt::AlignVCenter);
}

void ItemFormatterBase::setBoolFld(int index, const QString &fld, bool center)
{
   if (fld.trimmed().toInt())
     setTextFld(index, QObject::tr("Yes"), center);
   else
     setTextFld(index, QObject::tr("No"), center);
}

void ItemFormatterBase::setBoolFld(int index, int fld, bool center)
{
   if (fld)
     setTextFld(index, QObject::tr("Yes"), center);
   else
     setTextFld(index, QObject::tr("No"), center);
}

void ItemFormatterBase::setNumericFld(int index, const QString &fld)
{
   setRightFld(index, fld.trimmed());
   setSortValue(index, fld.toDouble() );
}

void ItemFormatterBase::setNumericFld(int index, const QString &fld, const QVariant &sortval)
{
   setRightFld(index, fld.trimmed());
   setSortValue(index, sortval );
}

void ItemFormatterBase::setBytesFld(int index, const QString &fld)
{
   qint64 qfld = fld.trimmed().toLongLong();
   QString msg;
   switch (cnvFlag) {
   case BYTES_CONVERSION_NONE:
      msg = QString::number(qfld);
      break;
   case BYTES_CONVERSION_IEC:
      msg = convertBytesIEC(qfld);
      break;
   case BYTES_CONVERSION_SI:
      msg = convertBytesSI(qfld);
      break;
   }

   setNumericFld(index, msg, qfld);
}

void ItemFormatterBase::setDurationFld(int index, const QString &fld)
{
   static const qint64 HOUR = Q_INT64_C(3600);
   static const qint64 DAY = HOUR * 24;
   static const qint64 WEEK = DAY * 7;
   static const qint64 MONTH = DAY * 30;
   static const qint64 YEAR = DAY * 365;
   static const qint64 divs[] = { YEAR, MONTH, WEEK, DAY, HOUR };
   static const char sufs[] = { 'y', 'm', 'w', 'd', 'h', '\0' };

   qint64 dfld = fld.trimmed().toLongLong();

   char suffix = 's';
   if (dfld) {
      for (int pos = 0 ; sufs[pos] ; ++pos) {
	  if (dfld % divs[pos] == 0) {
	     dfld /= divs[pos];
	     suffix = sufs[pos];
	     break;
	  }
      }
   }
   QString msg;
   if (dfld < 100) {
      msg = QString("%1%2").arg(dfld).arg(suffix);
   } else {
      /* previous check returned a number too big. The original specification perhaps
         was mixed, like 1d 2h, so we try to match with this routine */
      dfld = fld.trimmed().toLongLong();
      msg = "";
      for (int pos = 0 ; sufs[pos] ; ++pos) {
	  if (dfld / divs[pos] != 0) {
	     msg += QString(" %1%2").arg(dfld / divs[pos]).arg(sufs[pos]);
	     dfld %= divs[pos];
	  }
      }
      if (dfld)
 	 msg += QString(" %1s").arg(dfld);
   }

   setNumericFld(index, msg, fld.trimmed().toLongLong());
}

void ItemFormatterBase::setVolStatusFld(int index, const QString &fld, bool center)
{
  QString mp(fld.trimmed());
   setTextFld(index, volume_status_to_str(mp.toUtf8()), center);

   if (mp == "Append" ) {
      setBackground(index, Qt::green);
   } else if (mp == "Error") {
      setBackground(index, Qt::red);
   } else if (mp == "Used" || mp == "Full"){
      setBackground(index, Qt::yellow);
   } else if (mp == "Read-only" || mp == "Disabled"){
      setBackground(index, Qt::lightGray);
   }
}

void ItemFormatterBase::setJobStatusFld(int index, const QString &status, bool center)
{
   /* C (created, not yet running) uses the default background */
   static QString greenchars("TR");
   static QString redchars("BEf");
   static QString yellowchars("eDAFSMmsjdctp");

   setTextFld(index, convertJobStatus(status), center);

   QString st(status.trimmed());
   if (greenchars.contains(st, Qt::CaseSensitive)) {
      setBackground(index, Qt::green);
   } else if (redchars.contains(st, Qt::CaseSensitive)) {
      setBackground(index, Qt::red);
   } else if (yellowchars.contains(st, Qt::CaseSensitive)){ 
      setBackground(index, Qt::yellow);
   }
}

void ItemFormatterBase::setJobTypeFld(int index, const QString &fld, bool center)
{
   QByteArray jtype(fld.trimmed().toAscii());
   if (jtype.size()) {
      setTextFld(index, job_type_to_str(jtype[0]), center);
   } else {
      setTextFld(index, "", center);
   }
}

void ItemFormatterBase::setJobLevelFld(int index, const QString &fld, bool center)
{
   QByteArray lvl(fld.trimmed().toAscii());
   if (lvl.size()) {
      setTextFld(index, job_level_to_str(lvl[0]), center);
   } else {
      setTextFld(index, "", center);
   }
}



/***********************************************
 *
 * treeitem formatting routines
 *
 ***********************************************/
TreeItemFormatter::TreeItemFormatter(QTreeWidgetItem &parent, int indent_level):
ItemFormatterBase(),
wdg(new QTreeWidgetItem(&parent)),
level(indent_level)
{
}

void TreeItemFormatter::setText(int index, const QString &fld)
{
   wdg->setData(index, Qt::UserRole, level);
   wdg->setText(index, fld);
}

void TreeItemFormatter::setTextAlignment(int index, int align)
{
   wdg->setTextAlignment(index, align);
}

void TreeItemFormatter::setBackground(int index, const QBrush &qb)
{
   wdg->setBackground(index, qb);
}

/* at this time we don't sort trees, so this method does nothing */
void TreeItemFormatter::setSortValue(int /* index */, const QVariant & /* value */)
{
}

/***********************************************
 *
 * Specialized table widget used for sorting
 *
 ***********************************************/
TableItemFormatter::BatSortingTableItem::BatSortingTableItem():
QTableWidgetItem(1)
{
}

void TableItemFormatter::BatSortingTableItem::setSortData(const QVariant &d)
{
   setData(SORTDATA_ROLE, d);
}

bool TableItemFormatter::BatSortingTableItem::operator< ( const QTableWidgetItem & o ) const 
{
   QVariant my = data(SORTDATA_ROLE);
   QVariant other = o.data(SORTDATA_ROLE);
   if (!my.isValid() || !other.isValid() || my.type() != other.type())
      return QTableWidgetItem::operator< (o); /* invalid combination, revert to default sorting */

   /* 64bit integers must be handled separately, others can be converted to double */
   if (QVariant::ULongLong == my.type()) {
      return my.toULongLong() < other.toULongLong(); 
   } else if (QVariant::LongLong == my.type()) {
      return my.toLongLong() < other.toLongLong(); 
   } else if (my.canConvert(QVariant::Double)) {
      return my.toDouble() < other.toDouble(); 
   } else {
      return QTableWidgetItem::operator< (o); /* invalid combination, revert to default sorting */
   }
}

/***********************************************
 *
 * tableitem formatting routines
 *
 ***********************************************/
TableItemFormatter::TableItemFormatter(QTableWidget &tparent, int trow):
ItemFormatterBase(),
parent(&tparent),
row(trow),
last(NULL)
{
}

void TableItemFormatter::setText(int col, const QString &fld)
{
   last = new BatSortingTableItem;
   parent->setItem(row, col, last);
   last->setText(fld);
}

void TableItemFormatter::setTextAlignment(int /*index*/, int align)
{
   last->setTextAlignment(align);
}

void TableItemFormatter::setBackground(int /*index*/, const QBrush &qb)
{
   last->setBackground(qb);
}

void TableItemFormatter::setSortValue(int /* index */, const QVariant &value )
{
   last->setSortData(value);
}

QTableWidgetItem *TableItemFormatter::widget(int col)
{
   return parent->item(row, col);
}

const QTableWidgetItem *TableItemFormatter::widget(int col) const
{
   return parent->item(row, col);
}

