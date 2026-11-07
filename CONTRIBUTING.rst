Contributing to JPype
=====================

Thank you for helping improve JPype.  This file covers what a pull request
needs; ``doc/develguide.rst`` describes how JPype works internally.


Changelog entries (news fragments)
----------------------------------

The changelog in ``doc/CHANGELOG.rst`` is assembled at release time by
`towncrier <https://towncrier.readthedocs.io/>`__.  Please do not edit it
directly.  Instead, every pull request with a user-visible change adds a
short file to ``doc/changes/``, named::

    doc/changes/<PR number>.<type>.rst

``<type>`` is one of:

``feature``
    New functionality.
``bugfix``
    A fix for incorrect behavior.
``removal``
    A deprecation, or removal of something deprecated earlier.
``doc``
    A documentation-only improvement.
``misc``
    Small changes worth tracking, such as refactoring.  Only the PR number is
    listed, not the text.

The file holds one or a few sentences of reStructuredText, written for users
and in the past tense.  For example, ``doc/changes/1234.bugfix.rst``::

    Fixed a crash when a Java exception was raised while a proxy was being
    garbage collected.

Changes that only matter to people working on JPype itself, such as the
build system, native internals, test harness or CI, go in the
``doc/changes/dev/`` subdirectory instead, with the same naming and types.
They appear in the changelog under a separate "For developers" heading::

    doc/changes/dev/1235.removal.rst

The PR number is added as a link automatically, so do not repeat it in the
text.  If you do not know the PR number yet, open the pull request first and
then add the fragment.  To reference an issue instead, name the file after
the issue number.  Changes with no user impact (typos, test-only edits) need
no fragment.

CI fails a pull request that adds no fragment.  If a change really needs
none, a maintainer can add the ``skip-changelog`` label to the pull request.
To run the same check locally::

    pip install towncrier
    towncrier check --compare-with origin/master

To preview how pending fragments will render::

    towncrier build --draft --version next

At release time a maintainer runs ``towncrier build --version <version>``,
which writes the entries into ``doc/CHANGELOG.rst`` and deletes the fragments
(see ``doc/release.rst``).
