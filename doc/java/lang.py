import jpype


class Thread:
    """ Thread support for Python

    JPype adds  methods to ``java.lang.Thread`` to interact with 
    Python threads.  These methods are all classmethods that 
    act on the current Python thread.
    """

    isAttached = jpype._jthread._JThread.isAttached
    attach = jpype._jthread._JThread.attach
    attachAsDaemon = jpype._jthread._JThread.attachAsDaemon
    detach = jpype._jthread._JThread.detach


class AutoCloseable:
    """ Customizer for ``java.lang.AutoCloseable`` and ``java.io.Closeable``

    This customizer adds support of the ``with`` operator to all Java
    classes that implement the Java ``AutoCloseable`` interface.

    Example:

    .. code-block:: python

        from java.nio.file import Files, Paths
        with Files.newInputStream(Paths.get("foo")) as fd:
          # operate on the input stream

        # Input stream closes at the end of the block.

    """
    ...
