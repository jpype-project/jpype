.. _miscellaneous_topics_database_access_with_jpypedbapi2:

Database Access with jpype.dbapi2
**********************************

JPype provides the ``jpype.dbapi2`` module, giving Python applications
access to JDBC drivers through an implementation of the Python Database
API Specification (:pep:`249`). This chapter's original tutorial content
duplicated material already covered, in full and in more depth, by
:doc:`dbapi2`. Rather than maintain two drifting copies of the same
reference, this page just points there.

- :doc:`dbapi2` -- the real reference: connection objects, globals,
  exceptions, type handling.

There is no reverse-direction (Java-embedding-Python) ``jpype.dbapi2``
story -- ``python.io``/``python.collections``/``python.datetime`` don't
expose a database-access front end, so no ``dbapi2_java.rst`` is planned.
