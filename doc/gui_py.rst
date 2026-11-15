.. _managing_crossplatform_gui_environments:

Managing Cross-Platform GUI Environments
****************************************

This chapter covers the Python-calling-Java direction only. There is
presently no reverse-direction (Java-embedding-Python) GUI-environment
story -- ``python.io``/``python.collections``/``python.datetime`` don't
expose a GUI-toolkit front end, so no ``gui_java.rst`` is planned. If that
changes, this note should be replaced with a cross-reference.

JPype provides utility functions, `setupGuiEnvironment` and
`shutdownGuiEnvironment`, to manage GUI environments across platforms,
ensuring compatibility with macOS, Linux, and Windows. These functions are
particularly useful for Swing and JavaFX-based applications, where macOS
imposes specific requirements for GUI event loops. Even on Linux and Windows,
using `setupGuiEnvironment` ensures consistent behavior and avoids potential
issues with threading and event loops.

.. _managing_crossplatform_gui_environments_setupguienvironmentcb:

setupGuiEnvironment(cb)
=======================

``setupGuiEnvironment(cb)`` addresses macOS's requirement that the event
loop run on the main thread: on macOS it starts ``cb`` on a background Java
thread and runs the console event loop
(``PyObjCTools.AppHelper.runConsoleEventLoop()``) on the calling thread; on
Linux and Windows, where there is no such restriction, it just calls
``cb()`` directly. Using it unconditionally on all three platforms keeps a
Swing/JavaFX application's startup code identical everywhere.

.. code-block:: python

    from jpype import setupGuiEnvironment
    from javafx.application import Platform

    def say_hello_later():
        """Test function for scheduling a task on the JavaFX Application Thread."""
        print("Hello from JavaFX!")

    def launch_gui():
        """Launch the GUI application."""
        # Example: Schedule a task on the JavaFX Application Thread
        Platform.runLater(say_hello_later)
        print("GUI launched")

    # Use setupGuiEnvironment to ensure cross-platform compatibility
    setupGuiEnvironment(launch_gui)

.. _managing_crossplatform_gui_environments_reestablishing_an_interactive_shell_on_another_thread:

Reestablishing an Interactive Shell on Another Thread
=====================================================

When using `setupGuiEnvironment`, the main thread may be occupied by the GUI
event loop (particularly on macOS). To allow interactive debugging in Python,
you can launch an interactive shell (e.g., IPython) on a separate thread.

**Steps**:

1. Use `setupGuiEnvironment` to start the GUI application.
2. Launch an interactive shell on a separate thread using Python's `threading`
   module.

**Example**:

.. code-block:: python

    import threading
    import IPython

    def launch_interactive_shell():
        """Launch an interactive shell on a separate thread."""
        IPython.embed()

    # Start the interactive shell on another thread
    thread = threading.Thread(target=launch_interactive_shell)
    thread.start()

By combining this approach with `setupGuiEnvironment`, you can interact with
the Python environment while the GUI application is running.

.. _managing_crossplatform_gui_environments_shutdownguienvironment:

shutdownGuiEnvironment()
========================

The counterpart to ``setupGuiEnvironment``: on macOS it stops the console
event loop (``PyObjCTools.AppHelper.stopEventLoop()``); on Linux and Windows
it's a no-op, since ``setupGuiEnvironment`` never blocked the main thread
there in the first place.

.. code-block:: python

    from jpype import shutdownGuiEnvironment

    # Shutdown the GUI environment (macOS-specific)
    shutdownGuiEnvironment()

.. _managing_crossplatform_gui_environments_best_practices_on_guis:

Best Practices on GUIs
--------------------------

- Schedule GUI updates using JavaFX's `Platform.runLater` or Swing's
  `SwingUtilities.invokeLater` so they run on the toolkit's own thread
  rather than whichever thread happens to call into it.
- Wrap callback functions passed to `setupGuiEnvironment` in `try`/`except`
  to prevent an unhandled exception on the background thread from silently
  killing the GUI startup.
- Test on macOS specifically, not just Linux/Windows -- it's the one
  platform where `setupGuiEnvironment`'s behavior actually differs from
  calling the callback directly.
