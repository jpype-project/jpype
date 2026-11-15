.. _awtswing:

AWT/Swing
*********

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) AWT/Swing story --
``python.io``/``python.collections``/``python.datetime`` don't expose a
GUI-toolkit front end, so no ``awt_java.rst`` is planned. If that changes,
this note should be replaced with a cross-reference.

Java GUI elements can be used from Python.  To use Swing
elements the event loop in Java must be started from a user thread.
This will prevent the JVM from shutting down until the user thread
is completed.

Here is a simple example which creates a hello world frame and
launches it from within Python.

.. code-block:: python

    import jpype
    import jpype.imports

    jpype.startJVM()
    import java
    import javax
    from javax.swing import *

    def createAndShowGUI():
        frame = JFrame("HelloWorldSwing")
        frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE)
        label = JLabel("Hello World")
        frame.getContentPane().add(label)
        frame.pack()
        frame.setVisible(True)

    # Start an event loop thread to handling gui events
    @jpype.JImplements(java.lang.Runnable)
    class Launch:
        @jpype.JOverride
        def run(self):
            createAndShowGUI()
    javax.swing.SwingUtilities.invokeLater(Launch())

