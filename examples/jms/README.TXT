This example of using JPype and JMS contributed by David Morgenthaler

Here is the message that accompanied it when he posted it on comp.lang.python :
=============================================================================
Your question intrigued me, so I gave it a try. And the anwer (at
least for JMS Publish/Subscribe TextMessages) is YES.

It includes:
- Java classes for a publisher (src/messaging/JpypePublisher) and a
subscriber (src/messaging/JpypeSubscriber). These Java classes, which
do most of the dirty work of connecting to the JMS server, are used
from python via jpype.
- Java classes that test the publisher and subscriber
(src/messaging/testJpepePublisher, src/messaging/testJpypeSubscriber)
- A Java interface that is used by python for the JProxy
- python scripts for publishing and subscribing
- An build.xml for ant to build and run the Java classes

The  Java and python publishers and subscribers work in any reasonable
combination (J pub/J sub, J pub/p sub, p pub/p sub, p pub/J sub, and
even with multiple subscribers)

Copy conveniently located folder. You'll find a build.xml for
ant to build the Java. You'll have to edit (in the testers and in the
python scripts) the naming and connection factory stuff for your
situation. You'll also have to edit the python scripts to correctly
locate your jvm and other classpath information. Once you're correctly
configured, you can use ant to build and/or execute the Java (type
"ant help"). And once the Java is built, you can try the python
versions.

I can't imagine that this approach wouldn't work for JMS Queues. And
while I haven't tested it, I don't see anything preventing the use of
other message types (BinaryMessage, ObjectMessage,...).

BTW, I don't detect any noticable slowdown in the python versions from
the Java versions (probably because of the strightforward conversion
of python strings to Java Strings). 

=============================================================================