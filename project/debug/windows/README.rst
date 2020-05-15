JVM startup debugging
=====================

So the JVM won't start under JPype.  It works fine for everyone else, but just
not you.  Now what?

JAVA_HOME
---------

First make sure your JAVA_HOME is set correctly.  It should point to a
directory containing the JRE or JDK.  It should look something like...::

  bin conf COPYRIGHT include jmods legal lib release

For Windows we expect to find ``%JAVA_HOME%\bin\server\jvm.dll`` or
``%JAVA_HOME%\bin\client\jvm.dll``.  We will assume it is server for this
document.


Architecture
------------

Next make sure that Python and the JVM have the same architecture.  We can do
this with ``DUMPBIN``.

Run::

    dumpbin /headers "%JAVA_HOME%/bin/jvm.dll"

It will produce a big output.  The relevant section is::

    File Type: DLL

    FILE HEADER VALUES
            8664 machine (x64)  <== Type is x64
               7 number of sections
        5CA35D83 time date stamp Tue Apr  2 06:02:59 2019
               0 file pointer to symbol table
               0 number of symbols
              F0 size of optional header
            2022 characteristics
                   Executable
                   Application can handle large (>2GB) addresses
                   DLL
    
Run the same command on the Python executable::

    File Type: EXECUTABLE IMAGE

    FILE HEADER VALUES
            8664 machine (x64) <== Type is x64
               6 number of sections
        5C9BF5B3 time date stamp Wed Mar 27 15:14:11 2019
               0 file pointer to symbol table
               0 number of symbols
              F0 size of optional header
              22 characteristics
                   Executable
                   Application can handle large (>2GB) addresses

If they match then the issue is not an architecture problem. 


Dependencies
------------

The JVM has many dependencies.  If any of them are missing, or
not in a search path, or they are the wrong architecture then 
it will fail to load.  To get a list of dependencies use::

    dumpbin /dependents "%JAVA_HOME%/bin/server/jvm.dll"

It should produce an output::

    Dump of file C:\Program Files\Java\jdk-12.0.1/bin/server/jvm.dll

    File Type: DLL
      Image has the following dependencies:
	KERNEL32.dll
	USER32.dll
	ADVAPI32.dll
	WSOCK32.dll
	WINMM.dll
	VERSION.dll
	PSAPI.DLL
	VCRUNTIME140.dll
	api-ms-win-crt-stdio-l1-1-0.dll
	api-ms-win-crt-string-l1-1-0.dll
	api-ms-win-crt-runtime-l1-1-0.dll
	api-ms-win-crt-convert-l1-1-0.dll
	api-ms-win-crt-environment-l1-1-0.dll
	api-ms-win-crt-utility-l1-1-0.dll
	api-ms-win-crt-math-l1-1-0.dll
	api-ms-win-crt-filesystem-l1-1-0.dll
	api-ms-win-crt-time-l1-1-0.dll
	api-ms-win-crt-heap-l1-1-0.dll

      Summary
	   DB000 .data
	   59000 .pdata
	  243000 .rdata
	   39000 .reloc
	    1000 .rsrc
	  7BF000 .text
	    3000 _RDATA

So every one of these libraries needs to be found on the path.  All of the required
DLLs should be located in ``%JAVA_HOME%/bin``.  However, Windows search order may 
find another copy of one of these DLLs elsewhere in the search process.  If the file
it finds is the wrong architechure or corrupt the JVM will fail to load.

Please note the search order in
https://docs.microsoft.com/en-us/windows/win32/dlls/dynamic-link-library-search-order

The order is::

  1) The directory from which the application loaded.

  2) The current directory.

  3) The system directory. Use the GetSystemDirectory function to get the path
     of this directory.

  4) The 16-bit system directory. There is no function that obtains the path of
     this directory, but it is searched.

  5) The Windows directory. Use the GetWindowsDirectory function to get the
     path of this directory.

  6) The directories that are listed in the PATH environment variable. Note
     that this does not include the per-application path specified by the App Paths
     registry key. The App Paths key is not used when computing the DLL search path.

So even if the PATH is pointing to ``%JAVA_HOME%/bin``, you may have a bad dll
in any of the first 5 places.


Testing the JVM
---------------

Assuming that you have the JAVA_HOME set, the architectures are correct, and 
all of the dependencies are in place it should load.  But lets run a test.

Compile using the Visual Studio shell ``x64 Native Tools Command`` or 
``x86 Native Tools Command`` depending on your Python architecture.

Use::

   CL.EXE testJVM.cpp

Run it with::

   testJVM.exe "%JAVA_HOME%\bin\server\jvm.dll"

It should produce and output like::

    Check paths
      SystemDirectory: C:\windows\system32
      WindowsDirectory: C:\windows
    Load library
    Load entry points
      Entry point found 00007FFEDE4703B0
    Pack JVM arguments
      Num options: 0
    Create JVM
    Create Java resources
    Destroy JVM
    Unload library
    Success

Here are some common problems:

The architecture of the jvm or one of the dependencies does not match the
program or is corrupted::

    Failed to load JVM from C:\Program Files (x86)\Java\jre1.8.0_251\bin\client\jvm.dll
    LastError 193
    Message: %1 is not a valid Win32 application.

A module or jar file is missing from the Java distribution::

    Failed to load JVM from jvm1.dll
    Error occurred during initialization of VM
    Failed setting boot class path.

A dll used by the JVM is missing::

    Failed to load JVM from jvm1.dll
    LastError 126
    Message: The specified module could not be found.

