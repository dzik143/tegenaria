# [ARCHIVE/2014] Tegenaria - OS independent utils for C++
- **ARCHIVAL** set of C++ utils,
- developed mainly between **2010-2014**,
- still used within some **OLDER** projects, but **NOT** maintaned anymore (even for bug fixes),
- Should works on Windows 32/64, Linux and MacOS,
- **MIT LICENSE** - use for any purpose (including commercial).
  
# Overview
  - Core:
    - [LibArgs](Source/Core/LibArgs) - argv[] parser driven by **CONFIG TABLE**,
    - [LibDebug](Source/Core/LibDebug) - logs and debug helper, **RESOURCE MONITOR**, which tracks used resources (files, sockets, mutexes etc.) in human-readable file and updates it on-the-fly,
    - [LibFile](Source/Core/LibFile) - OS independent **FILE** functions (open/read/write, read content at-once, temporary files, [transactional/atomic write](https://en.wikipedia.org/wiki/Database_transaction) etc.)
    - [LibIO](Source/Core/LibIO) - high-level **I/O routines** with built-in timeout functionality, abstract I/O constructs: FIFO, [circular buffer](https://en.wikipedia.org/wiki/Circular_buffer), IO Multiplexer,
    - [LibIpc](Source/Core/LibIpc) - **INTER-PROCESS COMUNICATION** (IPC) using [Named Pipe](https://docs.microsoft.com/en-us/windows/win32/ipc/named-pipes) (Windows) or [local socket](https://opensource.com/article/19/4/interprocess-communication-linux-networking) (Linux/MacOS),
    - [LibJob](Source/Core/LibJob) - abstract Job queue and synchronization schemes,
    - [LibLock](Source/Core/LibLock) - OS independent **SYNCHRONIZATION** ([mutexes](https://en.wikipedia.org/wiki/Lock_(computer_science)) and [semaphores](https://en.wikipedia.org/wiki/Semaphore_(programming))),
    - [LibObject](Source/Core/LibObject) - base C++ object scheme with thread-safe **REFERENCE COUNTER**,
    - [LibProcess](Source/Core/LibProcess) - OS independent **PROCESS** management (create, wait, kill, etc.),
    - [LibReg](Source/Core/LibReg) - high-level [WINDOWS REGISTRY](https://en.wikipedia.org/wiki/Windows_Registry) management (Windows only),
    - [LibService](Source/Core/LibService) - high-level routines to manage [WINDOWS SERVICES](https://docs.microsoft.com/en-us/dotnet/framework/windows-services/introduction-to-windows-service-applications),
    - [LibSSMap](Source/Core/LibSSMap) - String-to-String dictionary class, often used to read/write **CONFIGURATION FILES**,
    - [LibStr](Source/Core/LibStr) - **STRING** and **RAW BUFFER** helpers,
    - [LibSystem](Source/Core/LibSystem) - helpers to read **SYSTEM DATA** such as OS version, amount of free memory, supported CPU instructions etc.,
    - [LibThread](Source/Core/LibThread) - OS independent **THREAD** management (create, wait, kill, etc.),
    - [LibVariant](Source/Core/LibVariant) - implementation of **VARIANT TYPE** with operator overloading (add, sub, div etc.),
              
  - Net:
    - [LibCGI](Source/Net/LibCGI) - C++ wrappers for [Common Gateway Interface](https://pl.wikipedia.org/wiki/Common_Gateway_Interface) (CGI) interface,
    - [LibNet](Source/Net/LibNet) - middle-level **NETWORK** related functions (TCP client, callback-based TCP server, [epool](https://en.wikipedia.org/wiki/Epoll), [IO Completion Ports](https://docs.microsoft.com/en-us/windows/win32/fileio/i-o-completion-ports), [SMTP client](https://pl.wikipedia.org/wiki/Simple_Mail_Transfer_Protocol)),
    - [LibNetEx](Source/Net/LibNetEx) - high-level network related functions,
    - [LibSecure](Source/Net/LibSecure) - **SECURITY** related ([SSL/TLS](https://pl.wikipedia.org/wiki/Transport_Layer_Security), encryption, random numbers), needs [OpenSSL](https://www.openssl.org/) to work,
    - [LibSftp](Source/Net/LibSftp) - OS independent [SFPT client](https://en.wikipedia.org/wiki/SSH_File_Transfer_Protocol) library.

# Build (Windows 32/64)
  1. Install [MinGW](http://www.mingw.org/)
  2. Get [qcbuild tool](Source/Tools/QCBuild/Prebuild/Win32)
  3. Go to main Tegenaria project root in cmd shell.
  4. Execute commands:
  ```
  qcbuild
  Build.bat
  ```
# Build (Linux)
  1. Go to main Tegenaria project root in terminal.
  2. Get [qcbuild tool](Source/Tools/QCBuild/Prebuild/Linux)
  3. Execute commands:
  ```
  ./qcbuild
  ./Build.sh
  ```
