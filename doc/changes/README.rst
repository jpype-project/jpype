News fragments
==============

Every pull request with a user-visible change adds one file here, named
``<PR number>.<type>.rst``, for example ``1234.bugfix.rst``.  Valid types are
``feature``, ``bugfix``, ``removal``, ``doc`` and ``misc``.

Notes only relevant to JPype developers (build, internals, CI) go in the
``dev/`` subdirectory with the same naming.

See ``CONTRIBUTING.rst`` in the repository root for details.

Refer to documented names with a role instead of a literal, so the
changelog links to them: ``:obj:`jpype.startJVM```, ``:obj:`TypeError```,
or with the text as written, ``:obj:`startJVM() <jpype.startJVM>```.
Names from the Python documentation resolve through intersphinx.  Java
classes link to their Javadoc with ``:jobj:`java.sql.DriverManager``` (or
``:jobj:`DriverManager <java.sql.DriverManager>```).
