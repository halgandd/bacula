/*
   Bacula® - The Network Backup Solution

   Copyright (C) 2000-2007 Free Software Foundation Europe e.V.

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
 *   Version $Id: batstack.cpp 4230 2007-02-21 20:07:37Z kerns $
 *
 *   Dirk Bartley, March 2007
 */

#include "pages.h"
#include "bat.h"

/*
 * dockPage
 * This function is intended to be called from within the Pages class to pull
 * a window from floating to in the stack widget.
 */

void Pages::dockPage()
{
   /* These two lines are for making sure if it is being changed from a window
    * that it has the proper window flag and parent.
    */
   setWindowFlags(Qt::Widget);
   setParent(m_parent);

   /* This was being done already */
   m_parent->addWidget(this);

   /* Set docked flag */
   m_docked = true;
}

/*
 * undockPage
 * This function is intended to be called from within the Pages class to put
 * a window from the stack widget to a floating window.
 */

void Pages::undockPage()
{
   /* Change from a stacked widget to a normal window */
   m_parent->removeWidget(this);
   setWindowFlags(Qt::Window);
   show();
   /* Clear docked flag */
   m_docked = false;
}

/*
 * This function is intended to be called with the subclasses.  When it is called
 * the specific sublclass does not have to be known to Pages.  It is called 
 * it will take it from it's current state of floating or stacked and change it
 * to the other.
 */

void Pages::togglePageDocking()
{
   if (m_docked) {
      undockPage();
   } else {
      dockPage();
   }
}

/*
 * This function is because I wanted for some reason to keep it private but still 
 * give any subclasses the ability to find out if it is currently stacked or not.
 */

bool Pages::isDocked()
{
   return m_docked;
}

/*
 * When a window is closed, this slot is called.  The idea is to put it back in the
 * stack here, and it works.  I wanted to get it to the top of the stack so that the
 * user immediately sees where his window went.  Also, if he undocks the window, then
 * closes it with the tree item highlighted, it may be confusing that the highlighted
 * treewidgetitem is not the stack item in the front.
 */

void Pages::closeEvent(QCloseEvent* /*event*/)
{
   /* A Widget was closed, lets toggle it back into the window, and set it in front. */
   dockPage();
   mainWin->setContextMenuDockText();
//   setTreeWidgetItemDockColor(page, item);
//   foreach(Pages *page, m_pagehash){
//      if
//   }

#ifdef xxx
   /* FIXME Really having problems getting it to the front, 
      toggles back into the stack fine though */
   int stackindex=m_parent->indexOf( this );
printf("In Pages closeEvent a\n");
   if( stackindex >= 0 ){
printf("In Pages closeEvent b\n");
      m_parent->setCurrentIndex(0);
      m_parent->setCurrentWidget(this);
      show();
      //m_parent->setCurrentIndex(stackindex);
//      m_parent->setCurrentWidget(this);
/*      m_parent->update();
      update();
      setUpdatesEnabled(true);
      setVisible(true);
      m_parent->show();
      show();
      m_parent->repaint();*/
      repaint();
      raise();
   }
#endif
}

/*
 * The next three are virtual functions.  The idea here is that each subclass will have the
 * built in virtual function to override if the programmer wants to populate the window
 * when it it is first clicked.
 */
void Pages::PgSeltreeWidgetClicked()
{
}

void Pages::PgSeltreeWidgetDoubleClicked()
{
}

/*
 *  * Virtual function which is called when this page is visible on the stack
 */
void Pages::currentStackItem()
{
}

/*
 * This function exists because to have an easy way for programmers adding new features to understand
 * exactly what values needed to be set in order to behave correctly in the interface.  It can
 * be called from the constructor, like with medialist or after being constructed, like with
 * Console.
 */
void Pages::SetPassedValues(QStackedWidget* passedStackedWidget, QTreeWidgetItem* passedTreeItem, int indexseq )
{
   m_parent = passedStackedWidget;
   m_treeItem = passedTreeItem;
   m_treeItem->setData(0, Qt::UserRole, QVariant(indexseq));
}
