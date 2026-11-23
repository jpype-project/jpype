Fixed :obj:`@JImplements <jpype.JImplements>` discarding the documentation of the decorated class. The
resulting class now keeps the original ``__doc__``, ``__name__``, ``__qualname__`` and
``__module__``. Previously the decorated type was given a new name ``proxy.<name>``, now
it retains the original class name.
