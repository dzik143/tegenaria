/******************************************************************************/
/*                                                                            */
/* Copyright (c) 2010, 2014 Sylwester Wysocki <sw143@wp.pl>                   */
/*                                                                            */
/* Permission is hereby granted, free of charge, to any person obtaining a    */
/* copy of this software and associated documentation files (the "Software"), */
/* to deal in the Software without restriction, including without limitation  */
/* the rights to use, copy, modify, merge, publish, distribute, sublicense,   */
/* and/or sell copies of the Software, and to permit persons to whom the      */
/* Software is furnished to do so, subject to the following conditions:       */
/*                                                                            */
/* The above copyright notice and this permission notice shall be included in */
/* all copies or substantial portions of the Software.                        */
/*                                                                            */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    */
/* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING    */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER        */
/* DEALINGS IN THE SOFTWARE.                                                  */
/*                                                                            */
/******************************************************************************/

//
// Purpose: Manage windows services.
//

#ifdef WIN32

#include "Service.h"
#include "Utils.h"

namespace Tegenaria
{
  //
  // Convert service type string to int code.
  // serviceType - string with start type (eg. "FILE_SYSTEM") (IN).
  //
  // RETURNS: Service type code or -1 if error.
  //

  int ServiceGetTypeCode(const char *serviceType)
  {
    static const StringIntPair serviceTypes[] =
    {
      {"FILE_SYSTEM",         SERVICE_FILE_SYSTEM_DRIVER},
      {"KERNEL_DRIVER",       SERVICE_KERNEL_DRIVER},
      {"RECOGNIZER_DRIVER",   SERVICE_RECOGNIZER_DRIVER},
      {"PROCESS",             SERVICE_WIN32_OWN_PROCESS},
      {"SHARE_PROCESS",       SERVICE_WIN32_SHARE_PROCESS},
      {"INTERACTIVE_PROCESS", SERVICE_INTERACTIVE_PROCESS},
      {NULL, 0}
    };

    for (int i = 0; serviceTypes[i].string_; i++)
    {
      if (strcmp(serviceTypes[i].string_, serviceType) == 0)
      {
        return serviceTypes[i].code_;
      }
    }

    return -1;
  }

  //
  // Convert service start type string to int code.
  //
  // serviceStartType - string with start type (IN).
  //
  // RETURNS: Start type code or -1 if error.
  //

  int ServiceGetStartTypeCode(const char *startType)
  {
    static const StringIntPair startTypes[] =
    {
      {"AUTO",     SERVICE_AUTO_START},
      {"BOOT",     SERVICE_BOOT_START},
      {"DEMAND",   SERVICE_DEMAND_START},
      {"DISABLED", SERVICE_DISABLED},
      {"SYSTEM",   SERVICE_SYSTEM_START},
      {NULL, 0}
    };

    for (int i = 0; startTypes[i].string_; i++)
    {
      if (strcmp(startTypes[i].string_, startType) == 0)
      {
        return startTypes[i].code_;
      }
    }

    return -1;
  }

  //
  // Open service manager and given service.
  //
  // WARNING! serviceManager and service MUST be closed by
  // CloseServiceHandle() function.
  //
  // serviceManager - handle to service manager (IN/OUT).
  //                  If NULL, new handle opened, otherside
  //                  given handle used as manager.
  //
  // service        - handle to opened service with given name (OUT).
  // name           - name of service to open (IN).
  // rights         - requested access right for opened service (IN).
  // quiet          - don't write error messages if set to 1 (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceOpen(SC_HANDLE *serviceManager, SC_HANDLE *service,
                      const char *name, DWORD rights, int quiet)
  {
    DBG_ENTER2("ServiceOpen");

    int exitCode = -1;

    //
    // Check args.
    //

    DEBUG3("ServiceOpen : Checking args...\n");

    SetLastError(ERROR_INVALID_PARAMETER);

    FAIL(name == NULL);
    FAIL(service == NULL);
    FAIL(serviceManager == NULL);

    //
    // Open service manager if needed.
    //

    if (*serviceManager == NULL)
    {
      DEBUG3("ServiceOpen : Opening service manager...\n");

      *serviceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);

      FAIL(*serviceManager == NULL);
    }

    //
    // Open service.
    //

    DEBUG3("ServiceOpen : Opening service '%s' with rights '0x%x'...\n", name, rights);

    *service = OpenService(*serviceManager, name, rights);

    FAIL(*service == NULL);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      if (quiet == 0)
      {
        Error("ERROR: Cannot open service '%s'.\nError code is %d.\n", name, err);
      }
      else
      {
        DEBUG1("Cannot open service '%s'. Error code is %d.\n", name, err);
      }

      if (serviceManager)
      {
        CloseServiceHandle(*serviceManager);
      }

      if (service)
      {
        CloseServiceHandle(*service);
      }

      if (err != 0)
      {
        exitCode = err;
      }
    }

    DBG_LEAVE2("ServiceOpen");

    SetLastError(exitCode);

    return exitCode;
  }

  //
  // Install and start new service.
  //
  // name         - service name (IN)
  // displayName  - service "long" name. Can be NULL. (IN)
  // type         - service type (IN).
  // startType    - start type (IN).
  // path         - path to executable file with service's code (IN).
  // failIfExists - function will fail if service already exists (IN).
  // obj          - target account name e.g. ".\\nx" (IN).
  // pass         - account's password if obj parameter specified (IN).
  // startAfter   - start service after add (default set to 1) (IN/OPT).
  // quiet        - do not write error messages if 1 (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceAdd(const char *name, const char *displayName, int type,
                     int startType, const char *path, bool failIfExists,
                         const char *obj, const char *pass, int startAfter,
                             int quiet)
  {
    DBG_ENTER2("ServiceAdd");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    char fullPath[MAX_PATH] = {0};

    SERVICE_STATUS status = {0};

    //
    // Check args.
    //

    DEBUG3("ServiceAdd : Checking args...\n");

    DEBUG1("ServiceAdd : Name         : [%s]\n", name);
    DEBUG1("ServiceAdd : Display name : [%s]\n", displayName);
    DEBUG1("ServiceAdd : Type         : [0x%x]\n", type);
    DEBUG1("ServiceAdd : Start type   : [0x%x]\n", startType);
    DEBUG1("ServiceAdd : Path         : [%s]\n", path);
    DEBUG1("ServiceAdd : FailIfExists : [%d]\n", failIfExists);
    DEBUG1("ServiceAdd : Account      : [%s]\n", obj);
    DEBUG1("ServiceAdd : Password     : [%s]\n", pass ? "Specified" : "Not specified");

    SetLastError(ERROR_INVALID_PARAMETER);

    FAILEX(name == NULL, "ERROR: Service's name is not valid.\n");
    FAILEX(path == NULL, "ERROR: Service's path is not valid.\n");

    //
    // Get absolute path to service binary.
    //

    DEBUG3("ServiceAdd : Retrieving absolute path to service binary...\n");

    FAIL(ExpandRelativePath(fullPath, MAX_PATH, path));

    //
    // Open service manager.
    //

    DEBUG3("ServiceAdd : Opening service manager...\n");

    serviceManager = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT |
                                       SC_MANAGER_CREATE_SERVICE);

    FAIL(serviceManager == NULL);

    //
    // Create new service.
    //

    DEBUG3("ServiceAdd : Creating new service...\n");

    service = CreateService(serviceManager, name, displayName,
                                SERVICE_START | DELETE | SERVICE_QUERY_STATUS | SERVICE_STOP,
                                    type, startType, SERVICE_ERROR_IGNORE,
                                        fullPath, NULL, NULL, NULL, obj, pass);

    //
    // CreateService failed.
    //

    if (service == NULL)
    {
      DEBUG1("ServiceAdd : CreateService() failed.\n");

      //
      // When service already exists try open existing one
      // to make sure is it in running state.
      //

      if (GetLastError() == ERROR_SERVICE_EXISTS)
      {
        FAIL(failIfExists);

        DEBUG1("ServiceAdd : Service already exists, going to open existing one.\n");

        service = OpenService(serviceManager, name,
                                  SERVICE_START | SERVICE_STOP |
                                      SERVICE_QUERY_STATUS);

        FAIL(service == NULL);
      }

      //
      // Service doesn't exists. We can't do more, fail.
      //

      else
      {
        FAIL(1);
      }
    }

    //
    // Send SC_START signal to new service if needed.
    //

    if (startAfter)
    {
      DEBUG3("ServiceAdd : Starting new service...\n");

      FAIL(StartService(service, 0, NULL) == FALSE
               && GetLastError() != ERROR_SERVICE_ALREADY_RUNNING);

      //
      // Wait until service change state from SERVICE_START_PENDING
      // to SERVICE_RUNNING.
      //

      FAIL(ServiceWaitUntilRunning(service, SERVICE_START_PENDING,
                                       SERVICE_RUNNING, SERVICE_RUN_TIMEOUT, 1));
    }

    DEBUG1("Service '%s' created.\n", name);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      if (quiet == 0)
      {
        Error("ERROR: Cannot create service '%s'.\n"
                  "Error code is %d.\n", name, err);
      }

      //
      // Sometimes service is created with errors (e.g. binary
      // path doesn't exist). We don't want service in this case.
      //

      if (err != ERROR_SERVICE_EXISTS)
      {
        DEBUG3("ServiceAdd : Cleaning up unusable service...\n");

        //
        // Send stop signal.
        //

        if (ControlService(service, SERVICE_CONTROL_STOP, &status) == FALSE)
        {
          DEBUG1("WARNING: Cannot send stop to service.\n"
                      "Error code is : %d.\n", (int) GetLastError());
        }

        //
        // Delete service from registry.
        //

        if (DeleteService(service) == FALSE)
        {
          Error("WARNING: Cannot delete service '%s'.\n"
                      "Error code is : %d.\n", name, (int) GetLastError());
        }
      }

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceAdd");

    return exitCode;
  }

  //
  // Change parameters for existing service.
  //
  // name        - service name (IN)
  // displayName - service "long" name. Can be NULL. (IN)
  // type        - service type (IN).
  // startType   - start type (IN).
  // path        - path to executable file with service's code (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceChange(const char *name, const char *displayName, int type,
                        int startType, const char *path)
  {
    DBG_ENTER2("ServiceChange");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    char fullPath[MAX_PATH] = {0};

    char *pathArg  = NULL;

    //
    // Check args.
    //

    DEBUG3("ServiceChange : Checking args...\n");

    SetLastError(ERROR_INVALID_PARAMETER);

    FAIL(name == NULL);

    DEBUG1("ServiceChange : Name         : [%s]\n", name);
    DEBUG1("ServiceChange : Display name : [%s]\n", displayName);
    DEBUG1("ServiceChange : Type         : [0x%x]\n", type);
    DEBUG1("ServiceChange : Start type   : [0x%x]\n", startType);
    DEBUG1("ServiceChange : Path         : [%s]\n", path);

    //
    // Get absolute path to service binary if needed.
    //

    if (path)
    {
      DEBUG3("ServiceChange : Retrieving absolute path to service binary...\n");

      FAIL(ExpandRelativePath(fullPath, MAX_PATH, path));

      pathArg = fullPath;
    }

    //
    // Open service and service manager.
    //

    DEBUG3("ServiceChange : Opening service...\n");

    FAIL(ServiceOpen(&serviceManager, &service, name, SERVICE_CHANGE_CONFIG));

    //
    // Change service parameters.
    //

    DEBUG3("ServiceChange : Changing service parameters...\n");

    FAIL(ChangeServiceConfig(service, type, startType, SERVICE_ERROR_IGNORE,
                                 pathArg, NULL, NULL, NULL, NULL, NULL,
                                     NULL) == FALSE);
    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot change service parameters.\n"
                "Error code is %d.\n", err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceChange");

    return exitCode;
  }

  //
  // Stop and delete existing service.
  //
  // name  - service name to delete (IN).
  // quiet - do not write error messages if 1 (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceDelete(const char *name, int quiet)
  {
    DBG_ENTER2("ServiceDelete");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    SERVICE_STATUS status;

    //
    // Check args.
    //

    DEBUG3("ServiceDelete : Checking args...\n");

    FAIL(name == NULL);

    //
    // If service does not exists simply exit with success.
    //

    if (ServiceExists(name) == 0)
    {
      DEBUG1("ServiceDelete : Service '%s' already deleted.\n", name);

      exitCode = 0;

      goto fail;
    }

    //
    // Open service.
    //

    DEBUG3("Opening service '%s'...\n", name);

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS, quiet));

    //
    // Delete service.
    //

    DEBUG3("Deleting service '%s'...\n", name);

    FAIL(DeleteService(service) == FALSE);

    //
    // We unregistered service, but service's instance can still exists.
    // We stop current running instance if exist.
    //

    DEBUG3("Sending STOP to '%s' service...\n", name);

    if (ControlService(service, SERVICE_CONTROL_STOP, &status) == TRUE)
    {
      //
      // Wait until service change state from
      // STOP_PENDING to STOPPED.
      //

      if (ServiceWaitUntilRunning(service, SERVICE_STOP_PENDING,
                                      SERVICE_STOPPED,
                                          SERVICE_RUN_TIMEOUT, 1))
      {
        //
        // Kill service process if timeout reached.
        //

        ServiceKill(service, 1);
      }
    }

    DEBUG1("Service '%s' deleted.\n", name);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      if (quiet == 0)
      {
        Error("ERROR: Cannot delete service '%s'.\n"
                  "Error code is %d.\n", name, err);
      }

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceDelete");

    return exitCode;
  }

  //
  // Retrieve status of given service.
  //
  // status - buffer, where to store retrieved status (OUT).
  // name   - service name to query (IN).
  // quiet  - don't write log on errors if 1 (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceGetStatus(SERVICE_STATUS *status, const char *name, int quiet)
  {
    DBG_ENTER2("ServiceGetStatus");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    //
    // Check args.
    //

    DEBUG3("ServiceGetStatus : Checking args...\n");

    FAIL(name == NULL);

    FAIL(status == NULL);

    //
    // Open service.
    //

    DEBUG3("ServiceGetStatus : Opening service...\n");

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         SERVICE_QUERY_STATUS, quiet));

    //
    // Retrieve status.
    //

    DEBUG3("ServiceGetStatus : Retrieving status of service...\n");

    FAIL(QueryServiceStatus(service, status) == FALSE);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      if (quiet == 0)
      {
        Error("ERROR: Cannot retrieve status for service '%s'.\n"
                  "Error code is %d.\n", name, err);
      }

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceGetStatus");

    return exitCode;
  }

  //
  // Start service.
  //
  // name - name of service to start (IN).
  // argc - number of elements in argv table (IN).
  // argv - table with input arguments for service binary (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceStart(const char *name, int argc, const char **argv)
  {
    DBG_ENTER2("ServiceStart");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    //
    // Check args.
    //

    DEBUG3("ServiceStart : Checking args...\n");

    FAIL(name == NULL);

    //
    // Open service.
    //

    DEBUG3("ServiceStart : Opening service...\n");

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         SERVICE_START | SERVICE_QUERY_STATUS));

    //
    // Start service.
    //

    DEBUG3("ServiceStart : Retrieving status of service...\n");

    FAIL(StartService(service, argc, argv) == FALSE);

    //
    // Wait until service change state from SERVICE_START_PENDING
    // to SERVICE_RUNNING.
    //

    ServiceWaitUntilRunning(service, SERVICE_START_PENDING,
                                SERVICE_RUNNING, SERVICE_RUN_TIMEOUT, 1);

    DEBUG1("Service '%s' started.\n", name);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot start service '%s'.\n"
                "Error code is %d.\n", name, err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceStart");

    return exitCode;
  }

  //
  // Send stop signal to given service and optionally
  // wait until service reach stopped.
  //
  // name    - name of service to stop (IN).
  // timeout - maximum time in ms to wait. If service not stopped before
  //           we kill it manually (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceStop(const char *name, int timeout)
  {
    DBG_ENTER2("ServiceStop");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    SERVICE_STATUS status;

    //
    // Check args.
    //

    DEBUG3("ServiceStop : Checking args...\n");

    FAIL(name == NULL);

    //
    // Open service.
    //

    DEBUG3("ServiceStop : Opening service...\n");

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         SERVICE_STOP | SERVICE_QUERY_STATUS));

    //
    // Send stop control code to service.
    //

    DEBUG3("ServiceStop : Sending STOP to service...\n");

    if (ControlService(service, SERVICE_CONTROL_STOP, &status) == FALSE)
    {
      Error("WARNING: Cannot send STOP signal to service.\n"
                "Error code is : %d.\n", (int) GetLastError());
    }

    //
    // Wait until service change state from SERVICE_STOP_PENDING
    // to SERVICE_STOPPED.
    //

    if (ServiceWaitUntilRunning(service, SERVICE_STOP_PENDING,
                                    SERVICE_STOPPED, timeout, 1))
    {
      //
      // If service doen't response on STOP signal silently
      // kill it's process.
      //

      ServiceKill(service, 1);
    }

    DEBUG1("Service '%s' stopped.\n", name);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      Error("ERROR: Cannot stop service '%s'.\n"
                "Error code is %d.\n", name, err);

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceStop");

    return exitCode;
  }

  //
  // Retrieve config of given service.
  //
  // WARNING: Output config buffer MUST be free by caller.
  //
  // config     - pointer to new allocated config struct (OUT).
  // name       - service name to query (IN).
  // quiet      - don't write log on errors if 1 (IN).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceGetConfig(QUERY_SERVICE_CONFIG **config, const char *name, int quiet)
  {
    DBG_ENTER2("ServiceGetConfig");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;

    SC_HANDLE service = NULL;

    DWORD configSize = 0;

    //
    // Check args.
    //

    DEBUG3("ServiceGetConfig : Checking args...\n");

    FAIL(name == NULL);

    FAIL(config == NULL);

    *config = NULL;

    //
    // Open service.
    //

    DEBUG3("ServiceGetConfig : Opening service...\n");

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         SERVICE_QUERY_CONFIG, quiet));

    //
    // Retrieve status.
    //

    DEBUG3("ServiceGetConfig : Retrieving config of service...\n");

    QueryServiceConfig(service, NULL, 0, &configSize);

    *config = (QUERY_SERVICE_CONFIG *) malloc(configSize);

    FAIL(QueryServiceConfig(service, *config, configSize, &configSize) == FALSE);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      int err = GetLastError();

      if (quiet == 0)
      {
        Error("ERROR: Cannot retrieve config for service '%s'.\n"
                  "Error code is %d.\n", name, err);
      }

      if (config && *config)
      {
        free(*config);
      }

      if (err != 0)
      {
        exitCode = err;
      }
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE2("ServiceGetConfig");

    return exitCode;
  }

  //
  // Check does service already exists.
  //
  // name - short name of service (IN)
  //
  // RETURNS: 1 if exists,
  //          0 if not exists or error.
  //

  int ServiceExists(const char *name)
  {
    SERVICE_STATUS status;

    if (ServiceGetStatus(&status, name, 1) == 0)
    {
      SetLastError(ERROR_ALREADY_EXISTS);

      DEBUG3("Service '%s' exists.\n", name);

      return 1;
    }
    else
    {
      SetLastError(ERROR_FILE_NOT_FOUND);

      DEBUG3("Service '%s' does not exist.\n", name);

      return 0;
    }
  }

  //
  // Wait up to <timeout> ms until given service change state from <initState>
  // to <targetState> state.
  // We use it to wait until start/stop operations finished.
  //
  // WARNING: Input handle MUST have SERVICE_QUERY_STATUS right.
  //
  // service     - handle to opened service (IN).
  // targetState - expected state, when reached we quit with success (IN).
  // timeout     - maximum time limit in ms. If missed default one used. (IN/OPT).
  // quiet       - do not write errors on stderr if 1 (IN/OPT).
  //
  // RETURNS: 0 if service is running on function's exit.
  //

  int ServiceWaitUntilRunning(SC_HANDLE service, DWORD initState,
                                  DWORD targetState, int timeout, int quiet)
  {
    DBG_ENTER2("ServiceWaitUntilRunning");

    SERVICE_STATUS status = {0};

    int exitCode = -1;

    int stateReached = 0;

    //
    // Wait until service change state.
    //

    DEBUG3("ServiceWaitUntilRunning : Waiting until service state changed from [%d] to [%d]...\n",
                initState, targetState);

    while(stateReached == 0 && timeout > 0)
    {
      //
      // Get current service state.
      //

      FAIL(QueryServiceStatus(service, &status) == FALSE);

      //
      // If target state reached go out with success.
      //

      if (status.dwCurrentState == targetState)
      {
        DEBUG2("ServiceWaitUntilRunning : Service reached state [%d]. Going to finish.\n", targetState);

        stateReached = 1;
      }

      //
      // If service is still in <initState>, keep waiting until timeout left.
      //

      else if (status.dwCurrentState == initState)
      {
        DEBUG3("ServiceWaitUntilRunning : Service is still in [%d] state...\n", initState);

        Sleep(500);

        timeout -= 500;
      }

      //
      // Unexpected state. Probably error on service side.
      //

      else
      {
        FAILEX(1, "ERROR: Unexpected service state [%d] reached.\n",
                   (int) status.dwCurrentState);
      }
    }

    //
    // If still in <initState>, it means probably
    // hung-up in service code.
    //

    SetLastError(ERROR_SERVICE_REQUEST_TIMEOUT);

    FAIL(stateReached == 0);

    //
    // Error handler.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (quiet)
      {
        DEBUG1("WARNING: Service doesn't response.\n"
                   "Error code is : %d.\n", GetLastError());
      }
      else
      {
        Error("ERROR: Service doesn't response.\n"
                  "Error code is : %d.\n", (int) GetLastError());
      }
    }

    DBG_LEAVE2("ServiceWaitUntilRunning");

    return exitCode;
  }

  //
  // Retrieve PID for opened process and terminate it.
  //
  // WARNING: Handle MUSTS have SERVICE_QUERY_STATE right.
  // WARNING: Caller user MUSTS have SE_DEBUG right to open not-own processes.
  //
  // service - handle to opened service (IN).
  // quiet   - do not write errors on stderr if 1 (IN/OPT).
  //
  // RETURNS: 0 if OK.
  //

  int ServiceKill(SC_HANDLE service, int quiet)
  {
    DBG_ENTER2("ServiceKill");

    int exitCode = -1;

    SERVICE_STATUS_PROCESS sp;

    DWORD spSize = 0;

    HANDLE process = NULL;

    //
    // Get PID of given service.
    //

    DEBUG3("ServiceKill : Retrieving service's PID...\n");

    FAIL(QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                  (PBYTE) &sp, sizeof(sp), &spSize) == FALSE);

    //
    // Enable SE_DEBUG to open not-own process.
    // This step needs caller user with SE_DEBUG privilege.
    //

    DEBUG3("ServiceKill : Enabling SE_DEBUG privilege...\n");

    FAIL(SetPrivilege("SeDebugPrivilege", 1));

    //
    // Open service process.
    //

    DEBUG3("ServiceKill : Opening service process [%d]...\n", (int) sp.dwProcessId);

    process = OpenProcess(PROCESS_TERMINATE, FALSE, sp.dwProcessId);

    FAIL(process == NULL);

    //
    // Terminate process.
    //

    DEBUG3("ServiceKill : Terminating service process [%d]...\n", (int) sp.dwProcessId);

    FAIL(TerminateProcess(process, -1) == FALSE);

    DEBUG1("Service process PID #%d terminated.\n", (int) sp.dwProcessId);

    //
    // Clean up.
    //

    exitCode = 0;

    fail:

    if (exitCode)
    {
      if (quiet)
      {
        DBG_MSG("WARNING: Cannot terminate service process.\n"
                    "Error code is : %d.\n", (int) GetLastError());
      }
      else
      {
        Error("WARNING: Cannot terminate service process.\n"
                  "Error code is : %d.\n", (int) GetLastError());
      }
    }

    SetPrivilege("SeDebugPrivilege", 0);

    DBG_LEAVE2("ServiceKill");

    return exitCode;
  }

  //
  // Retrieve PID of service process.
  //
  // name - service name (IN).
  //
  // RETURNS: PID of service process,
  //          or -1 if error.
  //

  int ServiceGetPid(const char *name)
  {
    DBG_ENTER3("ServiceGetPid");

    int exitCode = -1;

    SC_HANDLE serviceManager = NULL;
    SC_HANDLE service        = NULL;

    SERVICE_STATUS_PROCESS sp = {0};

    DWORD spSize = 0;

    //
    // Open service.
    //

    FAIL(ServiceOpen(&serviceManager, &service, name,
                         DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS));

    //
    // Query for service process information.
    //

    FAIL(QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
                                  (PBYTE) &sp, sizeof(sp), &spSize) == FALSE);

    exitCode = sp.dwProcessId;

    DEBUG2("Service '%s' has PID #%d.\n", name, int(sp.dwProcessId));

    //
    // Clean up.
    //

    fail:

    if (exitCode == -1)
    {
      Error("ERROR: Cannot retrieve PID for service '%s'.\n"
                "Error code is : %d.\n", name, GetLastError());
    }

    CloseServiceHandle(serviceManager);
    CloseServiceHandle(service);

    DBG_LEAVE3("ServiceGetPid");

    return exitCode;
  }

} /* namespace Tegenaria */

#endif /* WIN32 */
