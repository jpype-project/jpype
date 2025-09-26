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
    - [ ] Check the .azure scripts to see if they are up to date.
          Look on https://devguide.python.org/versions/ to see what versions can be dropped
          Check Python versions for Windows
          Check Python versions for OSX
          Check the manylinux image for Linux
    - [ ] Check patterns in .azure/scripts/build-wheels.sh
    - [ ] Merge the current master with the release
          ``git pull origin master``
    - [ ] Edit doc/CHANGELOG.rst
- [ ] Create a release candidate
    - [ ] Bump the version to release
        ``bumpversion release``
    - [ ] Send the release to be evaluated
        ``git push``
    - [ ] Verify CI on Azure  ([Azure](https://dev.azure.com/jpype-project/jpype/_build?definitionId=1))
    - [ ] Manually trigger a ``jpype.release`` on ([Azure](https://dev.azure.com/jpype-project/jpype/_build?definitionId=2))
          If successful, download the artifacts for publication.
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
