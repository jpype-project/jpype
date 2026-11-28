:orphan:

Release cycle docs
==================

This project uses bump2version
  See https://medium.com/@williamhayes/versioning-using-bumpversion-4d13c914e9b8

To start a new cycle use:
  ``bumpversion patch``

To increment the build number during development:
  ``bumpversion build``

To release:
  ``bumpversion release``

Full process:
(first copy the checklist to an issue)

- [ ] Start from the release branch
      ``git checkout release``
- [ ] Make a new branch for the release cycle
      ``git checkout -b releases/{version}``
- [ ] Update release process to current
    - [ ] Merge the current master with the release
          ``git pull origin master``
    - [ ] Check the .azure scripts to see if they are up to date.
          Look on https://devguide.python.org/versions/ to see what versions can be dropped
          Check Python versions for Windows
          Check Python versions for OSX
          Check the manylinux image for Linux
    - [ ] Update jpype/pyproject.toml `required-python` line to match the current builds.
    - [ ] Check patterns in .azure/scripts/build-wheels.sh
    - [ ] Review the pending news fragments in doc/changes
          ``towncrier build --draft --version next``
- [ ] Create a release candidate
    - [ ] Assemble the changelog and bump the version to release, in one commit.
        bumpversion reports the version it will bump to; towncrier writes
        the changelog section for it and removes the fragments; bumpversion
        then commits that together with the version bump::

            V=$(bumpversion --dry-run --list release | sed -n 's/^new_version=//p')
            towncrier build --yes --version "$V"
            bumpversion --allow-dirty release

        A pull request carrying this commit (e.g. merging the release back
        into master) edits doc/CHANGELOG.rst, so it needs the
        ``skip-changelog`` label to pass CI.  Other bumps do not touch the
        changelog.
    - [ ] Send the release to be evaluated
        ``git push``
    - [ ] Verify CI on Azure  ([Azure](https://dev.azure.com/jpype-project/jpype/_build?definitionId=1))
    - [ ] Manually trigger a ``jpype.release`` on ([Azure](https://dev.azure.com/jpype-project/jpype/_build?definitionId=2))
          If successful, download the artifacts for publication.
          Do not publish musllinux wheels unless the gate in
          `musllinux wheels`_ below has been passed.
- [ ] Advance the release pointer 
      ``git checkout release``
      ``git merge releases/<version>``
- [ ] Publish the release
  - Add draft release on github
  - Attach the artifacts to the release.
- [ ] Start master on a new cycle
  - Use a PR to pull release back to master
  - ``git checkout master``
  - ``git checkout -b cycle``
  - ``git merge release``
  - ``bumpversion patch``
  - Use PR to insert the cycle in master

**Last, update this document with any changes in process that were required.**


musllinux wheels
----------------

**Status:** not published.  No jpype1 release on PyPI has ever included a
musllinux wheel (checked against all releases up to 1.7.1), so users of
Alpine and other musl based distributions build from the source
distribution.  Building them in ``release.yml`` (``build_profile: musl`` or
``all``) is a test build; leaving the wheels out of an upload is not a
regression.

**How they are built.**  The ``Musllinux_*`` jobs use the
``quay.io/pypa/musllinux_1_2_*`` images (Alpine based; ``musllinux_1_1``
reached end of life on 2024-11-01).  ``.azure/scripts/build-wheels.sh``
detects Alpine by the presence of ``apk`` and installs ``openjdk11-jdk``
(falling back to ``openjdk17-jdk``) and ``apache-ant`` instead of using
``yum``.

**C++ runtime.**  The auditwheel musllinux policy only treats ``libc`` and
``libz`` as system libraries, so by default ``auditwheel repair`` copies
``libstdc++`` and ``libgcc_s`` into the wheel under hashed names.  The JVM's
``libjvm.so`` loads the system copies, which would put two C++ runtimes
(and two unwinders) in one process.  JPype has seen a SIGSEGV during
exception unwinding before, so ``build-wheels.sh`` instead passes
``--exclude libstdc++.so.6 --exclude libgcc_s.so.1`` for musllinux.  The
wheel then links against the system runtime, just as the manylinux wheels
do.  Consequences:

- The wheel is not strictly self-contained, as the musllinux policy expects.
  The library is present on any Alpine system with a JDK installed, since the
  JDK package depends on it.
- The wheel is compiled with the image's native GCC (GCC 14 for Alpine 3.22),
  so it most likely needs the ``libstdc++`` of Alpine 3.21 or newer.  On older
  systems the import fails with a symbol-version error rather than crashing.
  Check the required ``GLIBCXX_*`` versions with ``readelf -V _jpype.so``
  before publishing.

**Gate before publishing musllinux wheels:**

- [ ] The ``Musllinux_*`` jobs pass, including the smoke test in
      ``build-wheels.sh`` (start the JVM, catch a Java exception through
      ``_jpype.so``).
- [ ] The full test suite passes on Alpine with the built wheel installed,
      on both x86_64 and aarch64.
- [ ] The minimum Alpine version is checked as above and stated in the
      changelog and in ``doc/install.rst``.
- [ ] The maintainers agree to support the platform long term (see the note
      above the ``Musllinux_*`` jobs in ``.azure/release.yml``).
