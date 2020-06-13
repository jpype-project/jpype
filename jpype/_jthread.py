import _jpype
from . import _jcustomizer


@_jcustomizer.JImplementationFor('java.lang.Thread')
class _JThread(object):
    """ Customizer for ``java.land.Thread``

    This adds addition JPype methods to java.lang.Thread to support
    Python.
    """

    @staticmethod
    def isAttached():
        """ Checks if a thread is attached to the JVM.

        Python automatically attaches as daemon threads when a Java method is
        called.  This creates a resource in Java for the Python thread. This
        method can be used to check if a Python thread is currently attached so
        that it can be disconnected prior to thread termination to prevent
        leaks.

        Returns:
          True if the thread is attached to the JVM, False if the thread is
          not attached or the JVM is not running.
        """
        return _jpype.isThreadAttachedToJVM()

    @staticmethod
    def attach():
        """ Attaches the current thread to the JVM as a user thread.

        User threads that are attached to the JVM will prevent the JVM from
        shutting down until the thread is terminated or detached.  To convert
        a daemon thread to a main thread, the thread must first be detached.

        Raises:
          RuntimeError: If the JVM is not running.
        """
        return _jpype.attachThreadToJVM()

    @staticmethod
    def attachAsDaemon():
        """ Attaches the current thread to the JVM as a daemon.

        Daemon threads act as background tasks and do not prevent the JVM from
        shutdown normally.  JPype automatically attaches any threads that call
        Java resources as daemon threads.  To convert a daemon thread to a user
        thread, the thread must first be detached.

        Raises:
          RuntimeError: If the JVM is not running.
        """
        return _jpype.attachThreadAsDaemon()

    @staticmethod
    def detach():
        """ Detaches a thread from the JVM.

        This function detaches the thread and frees the associated resource in
        the JVM. For codes making heavy use of threading this should be used
        to prevent resource leaks. The thread can be reattached, so there
        is no harm in detaching early or more than once. This method cannot fail
        and there is no harm in calling it when the JVM is not running.
        """
        return _jpype.detachThreadFromJVM()
