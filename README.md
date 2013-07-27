JPype
=====

From the [original Website](http://jpype.sourceforge.net/index.html):

> JPype is an effort to allow python programs full access to java class libraries.
> This is achieved not through re-implementing Python, as Jython/JPython has done,
> but rather through interfacing at the native level in both Virtual Machines.
> Eventually, it should be possible to replace Java with python in many, though not all, situations. 
> JSP, Servlets, RMI servers and IDE plugins are good candidates.
> 
> Once this integration is achieved, a second phase will be started to separate the Java logic from 
> the Python logic, eventually allowing the bridging technology to be used in other environments, 
> I.E. Ruby, Perl, COM, etc ...

This github fork applies changes to make the installation easier on OSX and Linux,
as suggested by [Yuvul Adam](http://blog.y3xz.com/post/5037243230/installing-jpype-on-mac-os-x)
and the beautiful people in [one of the JPype Stackoverflow postings](http://stackoverflow.com/questions/8525193/cannot-install-jpype-on-os-x-lion-to-use-with-neo4j).

Known Bugs/Limitations
----------------------
* Java classes outside of a package (in the `<default>`) cannot be imported.
* unable to access a field or method if it conflicts with a python keyword.
* Because of lack of JVM support, you cannot shutdown the JVM and then restart it.
* Some methods rely on the "current" class/caller. Since calls coming directly from 
  python code do not have a current class, these methods do not work. The User Manual 
  lists all the known methods like that.

Requirements
------------

Either the Sun/Oracle JDK/JRE Variant or OpenJDK.

### Debian/Ubuntu ###

Debian/Ubuntu users will have to install `g++` and `python-dev` first:

    sudo apt-get install g++ python-dev

Installation
------------

Easy as

    python setup.py install

:+1:

Tested on
---------

* OS X 10.7.4 with Sun/Oracle JDK 1.6.0
* OSX 10.8.1–10.8.4 with Sun/Oracle JDK 1.6.0
* Debian 6.0.4/6.0.5 with Sun/Oracle JDK 1.6.0
* Debian 7.1 with OpenJDK 1.6.0
* Ubuntu 12.04 with Sun/Oracle JDK 1.6.0 
