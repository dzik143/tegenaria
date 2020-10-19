/*
   Copyright (c) 2010 ,
     Cloud Wu . All rights reserved.

     http://www.codingnow.com

   Use, modification and distribution are subject to the "New BSD License"
   as listed at <url: http://www.opensource.org/licenses/bsd-license.php >.

   filename: backtrace.c

   compiler: gcc 3.4.5 (mingw-win32)

   build command: gcc -O2 -shared -Wall -o backtrace.dll backtrace.c -lbfd -liberty -limagehlp

   how to use: Call LoadLibraryA("backtrace.dll"); at beginning of your program .

  */

#define WINVER 0x501

#include <windows.h>

/*

TODO: Temporary disabled due to missing bfd.h on new MinGW.

#include <set>
#include <string>
#include <excpt.h>
#include <imagehlp.h>
#include <bfd.h>
#include <psapi.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <ctime>
#include "md5.h"

using std::set;
using std::string;

#define BUFFER_MAX (16*1024)

char ProcessName[MAX_PATH]     = {0};
char ModuleDirectory[MAX_PATH] = {0};

LPEXCEPTION_POINTERS ExceptionPointers = NULL;

set<string> Modules;

int SignalCode = -1;

struct bfd_ctx
{
  bfd * handle;
  asymbol ** symbol;
};

struct bfd_set
{
  char * name;
  struct bfd_ctx * bc;
  struct bfd_set *next;
};

struct find_info
{
  asymbol **symbol;
  bfd_vma counter;
  const char *file;
  const char *func;
  unsigned line;
};

struct output_buffer
{
  char * buf;
  size_t sz;
  size_t ptr;
};

//
// Get full directory, where current running module loaded.
//

void GetCurrentModuleDirectory()
{
  int lastSlash = 0;

  GetModuleFileName(NULL, ModuleDirectory, sizeof(ModuleDirectory) - 1);

  //
  // Remove filename from path.
  //

  for (int i = 0; ModuleDirectory[i] && i < ModuleDirectory[i]; i++)
  {
    if (ModuleDirectory[i] == '\\' || ModuleDirectory[i] == '/')
    {
      lastSlash = i;
    }
  }

  ModuleDirectory[lastSlash] = 0;
}

//
// Convert full path to file name only.
// E.g. it convert 'a\b\c\file.dll' to file.dll.
//

const char *GetFileName(const char *path)
{
  int lastSlash = -1;

  for (int i = 0; path[i]; i++)
  {
    if (path[i] == '\\' || path[i] == '/')
    {
      lastSlash = i;
    }
  }

  if (lastSlash != -1)
  {
    return path + lastSlash + 1;
  }
  else
  {
    return path;
  }
}

static void output_init(struct output_buffer *ob, char * buf, size_t sz)
{
  ob->buf = buf;
  ob->sz = sz;
  ob->ptr = 0;
  ob->buf[0] = '\0';
}

static void output_print(struct output_buffer *ob, const char * format, ...)
{
  if (ob->sz == ob->ptr)
    return;

  ob->buf[ob->ptr] = '\0';
  va_list ap;
  va_start(ap,format);

  vsnprintf(ob->buf + ob->ptr , ob->sz - ob->ptr , format, ap);

  va_end(ap);

  ob->ptr = strlen(ob->buf + ob->ptr) + ob->ptr;
}

static void lookup_section(bfd *abfd, asection *sec, void *opaque_data)
{
  struct find_info *data = (struct find_info *) opaque_data;

  if (data->func)
    return;

  if (!(bfd_get_section_flags(abfd, sec) & SEC_ALLOC))
    return;

  bfd_vma vma = bfd_get_section_vma(abfd, sec);

  if (data->counter < vma || vma + bfd_get_section_size(sec) <= data->counter)
    return;

  bfd_find_nearest_line(abfd, sec, data->symbol, data->counter - vma, &(data->file), &(data->func), &(data->line));
}

static void find(struct bfd_ctx * b, DWORD offset,
                     const char **file, const char **func, unsigned *line)
{
  struct find_info data;
  data.func = NULL;
  data.symbol = b->symbol;
  data.counter = offset;
  data.file = NULL;
  data.func = NULL;
  data.line = 0;

  bfd_map_over_sections(b->handle, &lookup_section, &data);
  if (file) {
    *file = data.file;
  }
  if (func) {
    *func = data.func;
  }
  if (line) {
    *line = data.line;
  }
}

static int init_bfd_ctx(struct bfd_ctx *bc,
                            const char *procname, struct output_buffer *ob)
{
  bc -> handle = NULL;
  bc -> symbol = NULL;

  bfd *b = bfd_openr(procname, 0);

  if (!b)
  {
    output_print(ob,"Failed to open bfd from (%s)\n" , procname);

    return 1;
  }

  int r1 = bfd_check_format(b, bfd_object);
  int r2 = bfd_check_format_matches(b, bfd_object, NULL);
  int r3 = bfd_get_file_flags(b) & HAS_SYMS;

  if (!(r1 && r2 && r3))
  {
    bfd_close(b);

    //output_print(ob,"Failed to init bfd from (%s)\n", procname);

    return 1;
  }

  void *symbol_table;

  unsigned dummy = 0;

  if (bfd_read_minisymbols(b, FALSE, &symbol_table, &dummy) == 0)
  {
    if (bfd_read_minisymbols(b, TRUE, &symbol_table, &dummy) < 0)
    {
      free(symbol_table);

      bfd_close(b);

      output_print(ob, "Failed to read symbols from (%s)\n", procname);

      return 1;
    }
  }

  bc -> handle = b;
  bc -> symbol = (asymbol **) symbol_table;

  return 0;
}

static void close_bfd_ctx(struct bfd_ctx *bc)
{
  if (bc)
  {
    if (bc->symbol)
    {
      free(bc->symbol);
    }

    if (bc->handle)
    {
      bfd_close(bc->handle);
    }
  }
}

static struct bfd_ctx *get_bc(struct output_buffer *ob,
                                  struct bfd_set *set, const char *procname)
{
  while(set->name)
  {
    if (strcmp(set->name , procname) == 0)
    {
      return set->bc;
    }

    set = set->next;
  }

  struct bfd_ctx bc;

  if (init_bfd_ctx(&bc, procname , ob))
  {
    return NULL;
  }

  set->next = (bfd_set *) calloc(1, sizeof(*set));

  set -> bc = (bfd_ctx *) malloc(sizeof(struct bfd_ctx));

  memcpy(set->bc, &bc, sizeof(bc));

  set->name = strdup(procname);

  return set->bc;
}

static void release_set(struct bfd_set *set)
{
  while(set)
  {
    struct bfd_set * temp = set->next;

    free(set->name);

    close_bfd_ctx(set->bc);

    free(set);

    set = temp;
  }
}

static void _backtrace(struct output_buffer *ob, struct bfd_set *set,
                           int depth , LPCONTEXT context)
{
  char procname[MAX_PATH];

  GetModuleFileNameA(NULL, procname, sizeof procname);

  struct bfd_ctx *bc = NULL;

  STACKFRAME frame;

  strncpy(ProcessName, procname, sizeof(ProcessName) - 1);

  memset(&frame,0,sizeof(frame));

  frame.AddrPC.Offset = context->Eip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrStack.Offset = context->Esp;
  frame.AddrStack.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = context->Ebp;
  frame.AddrFrame.Mode = AddrModeFlat;

  HANDLE process = GetCurrentProcess();
  HANDLE thread = GetCurrentThread();

  char symbol_buffer[sizeof(IMAGEHLP_SYMBOL) + 255];
  char module_name_raw[MAX_PATH];

  while(StackWalk(IMAGE_FILE_MACHINE_I386, process, thread, &frame,
                      context, 0, SymFunctionTableAccess, SymGetModuleBase, 0))
  {
    --depth;

    if (depth < 0)
    {
      break;
    }

    IMAGEHLP_SYMBOL *symbol = (IMAGEHLP_SYMBOL *)symbol_buffer;

    symbol->SizeOfStruct = (sizeof *symbol) + 255;

    symbol->MaxNameLength = 254;

    DWORD module_base = SymGetModuleBase(process, frame.AddrPC.Offset);

    const char * module_name = "[unknown module]";

    if (module_base && GetModuleFileNameA((HINSTANCE)module_base, module_name_raw, MAX_PATH))
    {
      module_name = module_name_raw;

      bc = get_bc(ob, set, module_name);

      Modules.insert(module_name);
    }

    const char * file = NULL;
    const char * func = NULL;
    unsigned line = 0;

    if (bc)
    {
      find(bc,frame.AddrPC.Offset,&file,&func,&line);
    }

    if (file == NULL)
    {
      DWORD dummy = 0;

      if (SymGetSymFromAddr(process, frame.AddrPC.Offset, &dummy, symbol))
      {
        file = symbol->Name;
      }
      else
      {
        file = "";
      }
    }

    if (func == NULL)
    {
      if (file[0])
      {
        output_print(ob, "0x%x : %s : %s \n",
                         frame.AddrPC.Offset, GetFileName(module_name), file);
      }
      else
      {
        output_print(ob, "0x%x : %s \n",
                         frame.AddrPC.Offset, GetFileName(module_name));
      }
    }
    else
    {
      output_print(ob, "0x%x : %s : %s (%d) : in function (%s) \n",
                       frame.AddrPC.Offset, GetFileName(module_name), file, line, func);
    }
  }
}

static char * g_output = NULL;

static LPTOP_LEVEL_EXCEPTION_FILTER g_prev = NULL;

//
// Generate <pid>.<timestamp>.dump file.
//

void CreateDumpFile()
{
  FILE *f = NULL;

  char fname[1024] = {0};

  //
  // Get path for current running process.
  //

  GetModuleFileName(NULL, ProcessName, sizeof(ProcessName) - 1);

  //
  // Print log to <pid>.<timestamp>.dump
  //

  if (getenv("CRASHREPORT_DIR"))
  {
    sprintf(fname, "%s\\%d.%d.dump", getenv("CRASHREPORT_DIR"), GetCurrentProcessId(), time(NULL));
  }
  else
  {
    sprintf(fname, "%s\\%d.%d.dump", ModuleDirectory, GetCurrentProcessId(), time(NULL));
  }

  f = fopen(fname, "wt+");

  if (f)
  {
    MD5 md5;

    SYSTEMTIME st;

    OSVERSIONINFOEX ver = {0};

    SYSTEM_INFO si = {0};

    //
    // Timestamp.
    //

    GetSystemTime(&st);

    fprintf(f, "Unhandled exception at %d-%02d-%02d %02d:%02d:%02d %03d\n\n",
                (int) st.wYear, (int) st.wMonth, (int) st.wDay,
                    (int) st.wHour, (int) st.wMinute,
                        (int) st.wSecond, (int) st.wMilliseconds);

    fflush(f);

    //
    // Process info.
    //

    fprintf(f, "Process name : [%s].\n", ProcessName);
    fprintf(f, "Process ID   : [%d].\n", GetCurrentProcessId());
    fprintf(f, "Thread ID    : [%d].\n", GetCurrentThreadId());

    if (ProcessName[0])
    {
      fprintf(f, "Image md5    : [%s].\n\n", md5.digestFile(ProcessName));
    }
    else
    {
      fprintf(f, "Image md5    : [unknown].\n\n");
    }

    fflush(f);

    //
    // Print exceptions pointers if any.
    //

    if (ExceptionPointers)
    {
      fprintf(f, "Exception code    : [0x%x].\n", ExceptionPointers -> ExceptionRecord -> ExceptionCode);
      fprintf(f, "Exception fags    : [0x%x].\n", ExceptionPointers -> ExceptionRecord -> ExceptionFlags);
      fprintf(f, "Exception address : [%p].\n\n", ExceptionPointers -> ExceptionRecord -> ExceptionAddress);
    }

    //
    // Print signal info if any.
    //

    if (SignalCode != -1)
    {
      const char *signalName = "Unknown";

      switch(SignalCode)
      {
        case SIGABRT: signalName = "SIGABRT"; break;
        case SIGFPE:  signalName = "SIGFPE"; break;
        case SIGILL:  signalName = "SIGILL"; break;
        case SIGINT:  signalName = "SIGINT"; break;
        case SIGSEGV: signalName = "SIGSEGV"; break;
        case SIGTERM: signalName = "SIGTERM"; break;
      }

      fprintf(f, "Signal : [%s]\n\n", signalName);

      fflush(f);
    }

    //
    // OS version info.
    //

    ver.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);

    GetVersionEx((OSVERSIONINFO *) &ver);

    fprintf(f, "OS version       : [%d.%d.%d].\n", ver.dwMajorVersion, ver.dwMinorVersion, ver.dwBuildNumber);
    fprintf(f, "OS platform ID   : [%d].\n", ver.dwPlatformId);
    fprintf(f, "OS CSD version   : [%s].\n", ver.szCSDVersion);
    fprintf(f, "OS SP version    : [%d.%d].\n", ver.wServicePackMajor, ver.wServicePackMinor);
    fprintf(f, "OS suite mask    : [%d].\n", (int) ver.wSuiteMask);
    fprintf(f, "OS product type  : [%d].\n\n", (int) ver.wProductType);

    fflush(f);

    //
    // Architecture info.
    //

    GetNativeSystemInfo(&si);

    fprintf(f, "CPU architecture : ");

    fflush(f);

    switch(si.wProcessorArchitecture)
    {
      case PROCESSOR_ARCHITECTURE_AMD64: fprintf(f, "[x64]\n"); break;
      case PROCESSOR_ARCHITECTURE_ARM:   fprintf(f, "[ARM]\n"); break;
      case PROCESSOR_ARCHITECTURE_IA64:  fprintf(f, "[IA64]\n"); break;
      case PROCESSOR_ARCHITECTURE_INTEL: fprintf(f, "[i386]\n"); break;

      default: fprintf(f, "[Unknown]\n"); break;
    }

    fprintf(f, "Number of cores  : [%d].\n\n", si.dwNumberOfProcessors);

    fflush(f);

    //
    // Backtrace.
    //

    if (g_output)
    {
      fprintf(f, g_output);
    }

    fflush(f);

    //
    // MD5 sums of used modules.
    //

    fprintf(f, "\nModules MD5 sums:\n");

    for (set<string>::iterator it = Modules.begin(); it != Modules.end(); it++)
    {
      fprintf(f, "  [%s] : [%s]\n", md5.digestFile((char *) it -> c_str()), it -> c_str());
    }

    fclose(f);
  }

}

//
// Handler for abort like signals.
//

void SignalHandler(int signal)
{
  SignalCode = signal;

  //
  // Prepare backtrace.
  //

  RaiseException(0x666, 0, 0, NULL);
}

//
// Crash handler.
//

static LONG WINAPI exception_filter(LPEXCEPTION_POINTERS info)
{
  struct output_buffer ob;

  output_init(&ob, g_output, BUFFER_MAX);

  if (!SymInitialize(GetCurrentProcess(), 0, TRUE))
  {
    output_print(&ob, "Failed to init symbol context\n");
  }
  else
  {
    bfd_init();

    struct bfd_set *set = (bfd_set *) calloc(1,sizeof(*set));

    _backtrace(&ob , set , 128 , info->ContextRecord);

    release_set(set);

    SymCleanup(GetCurrentProcess());
  }

  ExceptionPointers = info;

  fputs(g_output, stderr);

  CreateDumpFile();

  exit(1);

  return 0;
}

static void backtrace_register(void)
{
  if (g_output == NULL)
  {
    g_output = (char *) malloc(BUFFER_MAX);

    g_prev = SetUnhandledExceptionFilter(exception_filter);
  }
}

static void backtrace_unregister(void)
{
  if (g_output)
  {
    free(g_output);

    SetUnhandledExceptionFilter(g_prev);

    g_prev = NULL;
    g_output = NULL;
  }
}
*/

extern "C" BOOL WINAPI DllMain(HANDLE hinstDLL,
                                   DWORD dwReason, LPVOID lpvReserved)
{
/*
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
    {
      //
      // Get directory where current binary stored.
      //

      GetCurrentModuleDirectory();

      //
      // Crash handler.
      //

      backtrace_register();

      //
      // Abort handler.
      //

      typedef void (*SignalHandlerPointer)(int);

      SignalHandlerPointer previousHandler;

      previousHandler = signal(SIGABRT, SignalHandler);

      break;
    }

    case DLL_PROCESS_DETACH:
    {
      backtrace_unregister();

      break;
    }
  }
*/
  return TRUE;
}
