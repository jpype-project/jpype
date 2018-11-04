The 2 scripts in this derectory are here to resolve a weakness in the linux implementation.

Since the JVM is loaded dynamically, the LD_LIBRARY_PATH may not contain the proper directories. 

Unfortunately, dlopen() will ignore any changes made to the LD_LIBRARY_PATH in the current process.

The jpype.sh is meant as an example for people redistributing apps using jpype. Instead of having 
users launch python script.py directly, have then launch script.sh. The companion finjvm.py can be 
delivered along your app, but is also present in the ptyhon distribution of jpype.

Developpers are encouraged to make their lives simpler by adding the correct directories to their 
LD_LIBRARY_PATH directly.

To learn more about the dlopen problem, you can look at :

http://forum.java.sun.com/thread.jsp?forum=52&thread=303583&message=1210441

and

http://www.nathanr.net/diary/index.php?year=2004&month=05
