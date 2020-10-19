/**************************************************************************/
/*                                                                        */
/* DIRLIGO                                                                */
/* All rights reserved.                                                   */
/*                                                                        */
/**************************************************************************/

#ifndef CrashReport_H
#define CrashReport_H

#ifdef WIN32
# include <windows.h>
# define CRASH_REPORT_INIT() LoadLibraryA("CrashReport.dll")
#else
# include <dlfcn.h>
# define CRASH_REPORT_INIT() dlopen("./libcrashreport.so", RTLD_NOW);
#endif

#endif /* CrashReport_H */
