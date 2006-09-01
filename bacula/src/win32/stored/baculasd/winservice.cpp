//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Bacula
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright (C) 2000-2006 Kern E. Sibbald
//


// winService

// Implementation of service-oriented functionality of Bacula
// I.e. command line options that contact a running version of
// Bacula and ask it to do something (show about, show status,
// show events, ...)


#include "bacula.h"
#include "winbacula.h"
#include "winservice.h"
#include "wintray.h"

void set_service_description(SC_HANDLE hSCManager, SC_HANDLE hService,
                             LPSTR lpDesc);

// OS-SPECIFIC ROUTINES

// Create an instance of the bacService class to cause the static fields to be
// initialised properly

bacService init;

extern bool    silent;

bacService::bacService()
{
}


BOOL
PostToBacula(UINT message, WPARAM wParam, LPARAM lParam)
{
  // Locate the hidden Bacula menu window
  HWND hservwnd = FindWindow(MENU_CLASS_NAME, NULL);
  if (hservwnd == NULL) {
     return FALSE;
  }

  // Post the message to Bacula
  PostMessage(hservwnd, message, wParam, lParam);
  return TRUE;
}


// Static routine to show the About dialog for a currently-running
// copy of Bacula, (usually a servicified version.)

BOOL
bacService::ShowAboutBox()
{
  // Post to the Bacula menu window
  if (!PostToBacula(MENU_ABOUTBOX_SHOW, 0, 0)) {
     MessageBox(NULL, _("No existing instance of Bacula storage service could be contacted"), szAppName, MB_ICONEXCLAMATION | MB_OK);
     return FALSE;
  }
  return TRUE;
}

// Static routine to show the Status dialog for a currently-running
// copy of Bacula, (usually a servicified version.)

BOOL
bacService::ShowStatus()
{
  // Post to the Bacula menu window
  if (!PostToBacula(MENU_STATUS_SHOW, 0, 0)) {
     MessageBox(NULL, _("No existing instance of Bacula storage service could be contacted"), szAppName, MB_ICONEXCLAMATION | MB_OK);
     return FALSE;
  }
  return TRUE;
}

// SERVICE-MODE ROUTINES

// Service-mode defines:

// Internal service name
#define BAC_SERVICENAME        "Bacula-sd"

// Displayed service name
#define BAC_SERVICEDISPLAYNAME "Bacula Storage Server"

// List other required services 
#define BAC_DEPENDENCIES __TEXT("tcpip\0afd\0") 


// Internal service state
SERVICE_STATUS          g_srvstatus;       // current status of the service
SERVICE_STATUS_HANDLE   g_hstatus;
DWORD                   g_error = 0;
DWORD                   g_servicethread = 0;
char*                   g_errortext[256];


// Forward defines of internal service functions
void WINAPI ServiceMain(DWORD argc, char **argv);
DWORD WINAPI ServiceWorkThread(LPVOID lpwThreadParam);
void ServiceStop();
void WINAPI ServiceCtrl(DWORD ctrlcode);
bool WINAPI CtrlHandler (DWORD ctrltype);
BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);

// ROUTINE TO QUERY WHETHER THIS PROCESS IS RUNNING AS A SERVICE OR NOT

BOOL    g_servicemode = FALSE;

BOOL
bacService::RunningAsService()
{
   return g_servicemode;
}

BOOL
bacService::KillRunningCopy()
{
  while (PostToBacula(WM_CLOSE, 0, 0))
      {  }
  return TRUE;
}

// SERVICE MAIN ROUTINE
int
bacService::BaculaServiceMain()
{
   // Mark that we are a service
   g_servicemode = TRUE;

   // Create a service entry table
   SERVICE_TABLE_ENTRY dispatchTable[] = {
      {BAC_SERVICENAME, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
      {NULL, NULL}
   };

   // Call the service control dispatcher with our entry table
   if (!StartServiceCtrlDispatcher(dispatchTable)) {
      log_error_message(_("StartServiceCtrlDispatcher failed."));
   }

   return 0;
}

// SERVICE MAIN ROUTINE - NT ONLY !!!
// NT/Win2K/WinXP ONLY !!!
void WINAPI ServiceMain(DWORD argc, char **argv)
{
    DWORD dwThreadID;

    // Register the service control handler
    g_hstatus = RegisterServiceCtrlHandler(BAC_SERVICENAME, ServiceCtrl);

    if (g_hstatus == 0) {
       log_error_message(_("RegisterServiceCtlHandler failed")); 
       MessageBox(NULL, _("Contact Register Service Handler failure"),
          "Bacula service", MB_OK);
       return;
    }

     // Set up some standard service state values
    g_srvstatus.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
    g_srvstatus.dwServiceSpecificExitCode = 0;

        // Give this status to the SCM
    if (!ReportStatus(
            SERVICE_START_PENDING,          // Service state
            NO_ERROR,                       // Exit code type
            45000)) {                       // Hint as to how long Bacula should have hung before you assume error

        ReportStatus(SERVICE_STOPPED, g_error,  0);
        log_error_message(_("ReportStatus STOPPED failed 1")); 
        return;
    }

        // Now start the service for real
    (void)CreateThread(NULL, 0, ServiceWorkThread, NULL, 0, &dwThreadID);
    return;
}

// SERVICE START ROUTINE - thread that calls BaculaAppMain
//   NT ONLY !!!!
DWORD WINAPI ServiceWorkThread(LPVOID lpwThreadParam)
{

    // Save the current thread identifier
    g_servicethread = GetCurrentThreadId();

    // report the status to the service control manager.
    //
    if (!ReportStatus(
          SERVICE_RUNNING,       // service state
          NO_ERROR,              // exit code
          0)) {                  // wait hint
       MessageBox(NULL, _("Report Service failure"), "Bacula Service", MB_OK);
       log_error_message("ReportStatus RUNNING failed"); 
       return 0;
    }

    /* Call Bacula main code */
    BaculaAppMain();

    /* Mark that we're no longer running */
    g_servicethread = 0;

    /* Tell the service manager that we've stopped */
    ReportStatus(SERVICE_STOPPED, g_error, 0);
    return 0;
}


// SERVICE STOP ROUTINE - post a quit message to the relevant thread
void ServiceStop()
{
   // Post a quit message to the main service thread
   if (g_servicethread != 0) {
      PostThreadMessage(g_servicethread, WM_QUIT, 0, 0);
   }
}

// SERVICE INSTALL ROUTINE
int
bacService::InstallService(const char *pszCmdLine)
{
   const int pathlength = 2048;
   char path[pathlength];
   char servicecmd[pathlength];

   // Get the filename of this executable
   if (GetModuleFileName(NULL, path, pathlength-(strlen(BaculaRunService)+2)) == 0) {
      MessageBox(NULL, _("Unable to install Bacula Storage service"), szAppName, MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   // Append the service-start flag to the end of the path:
   if ((int)strlen(path) + 5 + (int)strlen(BaculaRunService) + (int)strlen(pszCmdLine) < pathlength) {
      sprintf(servicecmd, "\"%s\" %s %s", path, BaculaRunService, pszCmdLine);
   } else {
      log_error_message(_("Service command length too long")); 
      MessageBox(NULL, _("Service command length too long. Service not registered."),
          szAppName, MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   SC_HANDLE   hservice;
   SC_HANDLE   hsrvmanager;

   // Open the default, local Service Control Manager database
   hsrvmanager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
   if (hsrvmanager == NULL) {
      log_error_message("OpenSCManager failed"); 
      MessageBox(NULL,
         _("The Service Control Manager could not be contacted - the Bacula Storage service was not installed"),
         szAppName, MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   // Create an entry for the Bacula service
   hservice = CreateService(
           hsrvmanager,                    // SCManager database
           BAC_SERVICENAME,                // name of service
           BAC_SERVICEDISPLAYNAME,         // name to display
           SERVICE_ALL_ACCESS,             // desired access
           SERVICE_WIN32_OWN_PROCESS | SERVICE_INTERACTIVE_PROCESS,
                                                                   // service type
           SERVICE_AUTO_START,             // start type
           SERVICE_ERROR_NORMAL,           // error control type
           servicecmd,                     // service's binary
           NULL,                           // no load ordering group
           NULL,                           // no tag identifier
           BAC_DEPENDENCIES,               // dependencies
           NULL,                           // LocalSystem account
           NULL);                          // no password
   if (hservice == NULL) {
      CloseServiceHandle(hsrvmanager);
      log_error_message("CreateService failed"); 
      MessageBox(NULL,
          _("The Bacula Storage service could not be installed"),
           szAppName, MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   set_service_description(hsrvmanager,hservice, 
_("Provides storage services. Bacula -- the network backup solution."));

   CloseServiceHandle(hsrvmanager);
   CloseServiceHandle(hservice);

   // Everything went fine
   if (!silent) {
      MessageBox(NULL,
              _("The Bacula Storage service was successfully installed.\n"
              "The service may be started from the Control Panel and will\n"
              "automatically be run the next time this machine is rebooted."),
              szAppName,
              MB_ICONINFORMATION | MB_OK);
   }
   return 0;
}


// SERVICE REMOVE ROUTINE
int
bacService::RemoveService()
{
   SC_HANDLE   hservice;
   SC_HANDLE   hsrvmanager;

   // Open the SCM
   hsrvmanager = OpenSCManager(
      NULL,                   // machine (NULL == local)
      NULL,                   // database (NULL == default)
      SC_MANAGER_ALL_ACCESS   // access required
      );
   if (hsrvmanager) {
      hservice = OpenService(hsrvmanager, BAC_SERVICENAME, SERVICE_ALL_ACCESS);
      if (hservice != NULL) {
         SERVICE_STATUS status;

         // Try to stop the Bacula service
         if (ControlService(hservice, SERVICE_CONTROL_STOP, &status)) {
            while(QueryServiceStatus(hservice, &status)) {
               if (status.dwCurrentState == SERVICE_STOP_PENDING) {
                  Sleep(1000);
               } else {
                  break;
               }
            }

            if (status.dwCurrentState != SERVICE_STOPPED) {
               MessageBox(NULL, _("The Bacula Storage service could not be stopped"), szAppName, MB_ICONEXCLAMATION | MB_OK);
            }
         }

         // Now remove the service from the SCM
         if(DeleteService(hservice)) {
            if (!silent) {
               MessageBox(NULL, _("The Bacula Storage service has been removed"), szAppName, MB_ICONINFORMATION | MB_OK);
            }
         } else {
            MessageBox(NULL, _("The Bacula Storage service could not be removed"), szAppName, MB_ICONEXCLAMATION | MB_OK);
         }

         CloseServiceHandle(hservice);
      } else {
         MessageBox(NULL, _("The Bacula Storage service could not be found"), szAppName, MB_ICONEXCLAMATION | MB_OK);
      }

      CloseServiceHandle(hsrvmanager);
   } else {
      MessageBox(NULL, _("The SCM could not be contacted - the Bacula Storage service was not removed"), szAppName, MB_ICONEXCLAMATION | MB_OK);
   }
   return 0;
}

// USEFUL SERVICE SUPPORT ROUTINES

// Service control routine
void WINAPI ServiceCtrl(DWORD ctrlcode)
{
    // What control code have we been sent?
    switch(ctrlcode) {
    case SERVICE_CONTROL_STOP:
        // STOP : The service must stop
        g_srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
        ServiceStop();
        break;

    case SERVICE_CONTROL_INTERROGATE:
        // QUERY : Service control manager just wants to know our state
        break;

     default:
        // Control code not recognised
        break;
    }

    // Tell the control manager what we're up to.
    ReportStatus(g_srvstatus.dwCurrentState, NO_ERROR, 0);
}

// Service manager status reporting
BOOL ReportStatus(DWORD state,
                  DWORD exitcode,
                  DWORD waithint)
{
    static DWORD checkpoint = 1;
    BOOL result = TRUE;

    // If we're in the start state then we don't want the control manager
    // sending us control messages because they'll confuse us.
    if (state == SERVICE_START_PENDING) {
       g_srvstatus.dwControlsAccepted = 0;
    } else {
       g_srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
    }

    // Save the new status we've been given
    g_srvstatus.dwCurrentState = state;
    g_srvstatus.dwWin32ExitCode = exitcode;
    g_srvstatus.dwWaitHint = waithint;

    // Update the checkpoint variable to let the SCM know that we
    // haven't died if requests take a long time
    if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED)) {
       g_srvstatus.dwCheckPoint = 0;
    } else {
       g_srvstatus.dwCheckPoint = checkpoint++;
    }

    // Tell the SCM our new status
    if (!(result = SetServiceStatus(g_hstatus, &g_srvstatus))) {
       log_error_message(_("SetServiceStatus failed"));
    }

    return result;
}

// Error reporting
void LogErrorMsg(char *message, char *fname, int lineno)
{
   char        msgbuff[256];
   HANDLE      heventsrc;
   char *      strings[32];
   LPTSTR      msg;

   // Get the error code
   g_error = GetLastError();
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL,
                 g_error,
                 0,
                 (LPTSTR)&msg,
                 0,
                 NULL);

   // Use event logging to log the error
   heventsrc = RegisterEventSource(NULL, BAC_SERVICENAME);

   sprintf(msgbuff, _("\n\n%s error: %ld at %s:%d"), 
      BAC_SERVICENAME, g_error, fname, lineno);
   strings[0] = msgbuff;
   strings[1] = message;
   strings[2] = msg;

   if (heventsrc != NULL) {
      MessageBeep(MB_OK);

      ReportEvent(
              heventsrc,              // handle of event source
              EVENTLOG_ERROR_TYPE,    // event type
              0,                      // event category
              0,                      // event ID
              NULL,                   // current user's SID
              3,                      // strings in 'strings'
              0,                      // no bytes of raw data
              (const char **)strings, // array of error strings
              NULL);                  // no raw data

      DeregisterEventSource(heventsrc);
   }
   LocalFree(msg);
}
typedef BOOL  (WINAPI * WinAPI)(SC_HANDLE, DWORD, LPVOID);

void set_service_description(SC_HANDLE hSCManager, SC_HANDLE hService,
                             LPSTR lpDesc) 
{ 
    SC_LOCK sclLock; 
    LPQUERY_SERVICE_LOCK_STATUS lpqslsBuf; 
    SERVICE_DESCRIPTION sdBuf;
    DWORD dwBytesNeeded;
    WinAPI ChangeServiceDescription;
 
    HINSTANCE hLib = LoadLibrary("ADVAPI32.DLL");
    if (!hLib) {
       return;
    }
    ChangeServiceDescription = (WinAPI)GetProcAddress(hLib,
       "ChangeServiceConfig2A");
    FreeLibrary(hLib);
    if (!ChangeServiceDescription) {
       return;
    }
    
    // Need to acquire database lock before reconfiguring. 
    sclLock = LockServiceDatabase(hSCManager); 
 
    // If the database cannot be locked, report the details. 
    if (sclLock == NULL) {
       // Exit if the database is not locked by another process. 
       if (GetLastError() != ERROR_SERVICE_DATABASE_LOCKED) {
          log_error_message("LockServiceDatabase"); 
          return;
       }
 
       // Allocate a buffer to get details about the lock. 
       lpqslsBuf = (LPQUERY_SERVICE_LOCK_STATUS)LocalAlloc( 
            LPTR, sizeof(QUERY_SERVICE_LOCK_STATUS)+256); 
       if (lpqslsBuf == NULL) {
          log_error_message("LocalAlloc"); 
          return;
       }
 
       // Get and print the lock status information. 
       if (!QueryServiceLockStatus( 
              hSCManager, 
              lpqslsBuf, 
              sizeof(QUERY_SERVICE_LOCK_STATUS)+256, 
              &dwBytesNeeded)) {
          log_error_message("QueryServiceLockStatus"); 
       }
 
       if (lpqslsBuf->fIsLocked) {
          printf(_("Locked by: %s, duration: %ld seconds\n"), 
                lpqslsBuf->lpLockOwner, 
                lpqslsBuf->dwLockDuration); 
       } else {
          printf(_("No longer locked\n")); 
       }
 
       LocalFree(lpqslsBuf); 
       log_error_message(_("Could not lock database")); 
       return;
    } 
 
    // The database is locked, so it is safe to make changes. 
 
    sdBuf.lpDescription = lpDesc;

    if (!ChangeServiceDescription(
         hService,                   // handle to service
         SERVICE_CONFIG_DESCRIPTION, // change: description
         &sdBuf) ) {                 // value: new description
       log_error_message("ChangeServiceConfig2");
    }

    // Release the database lock. 
    UnlockServiceDatabase(sclLock); 
}
