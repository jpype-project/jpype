# -*- coding: utf-7 -*-
# *****************************************************************************
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   See NOTICE file for details.
#
# *****************************************************************************
import os
import shutil
from pathlib import Path
from setuptools.command.sdist import sdist

# --- Logging compatibility shim ---
try:
    import distutils.log as _log
    LOG_INFO = _log.INFO
    LOG_WARN = _log.WARN
    LOG_ERROR = _log.ERROR
except ImportError:
    import logging
    class _log:
        INFO = logging.INFO
        WARN = logging.WARNING
        ERROR = logging.ERROR
# ----------------------------------

class BuildSourceDistribution(sdist):
    """
    Custom sdist command to include prebuilt JPype jar files in the source distribution.
    This removes the need for javac/jdk at install time.
    """

    def run(self):
        # Destination directory for the jar file(s)
        dest = Path('native') / 'jars'
        self.announce(f"Preparing to build and package JPype jar files at {dest}", level=LOG_INFO)

        # Get the build_ext command object and set the jar build flag
        cmd = self.distribution.get_command_obj('build_ext')

        # Call with jar only option
        cmd.jar = True

        # Run build_ext and test_java to ensure all Java artifacts are built
        try:
            self.run_command('build_ext')
            self.run_command('test_java')
        except Exception as e:
            raise RuntimeError(f"Failed during build_ext or test_java: {e}")

        # Determine the path to the generated jar file
        jar_basename = "org.jpype.jar"
        # The get_ext_fullpath method returns the path to the extension module,
        # so we use its directory as the jar location.
        ext_fullpath = cmd.get_ext_fullpath("JAVA")
        jar_dir = Path(os.path.dirname(ext_fullpath))
        jar_path = jar_dir / jar_basename

        # Check that the jar file exists
        if not jar_path.exists():
            self.announce(f"Jar source file is missing: {jar_path}", level=LOG_ERROR)
            raise RuntimeError(f"Error copying jar file: {jar_path}")

        # Ensure the destination directory exists
        dest.mkdir(parents=True, exist_ok=True)

        # Copy the jar file to the destination
        shutil.copy2(jar_path, dest / jar_basename)
        self.announce(f"Copied {jar_path} to {dest}", level=LOG_INFO)

        # Run the standard sdist process to package the source distribution
        super().run()

        # Clean up: remove the jar cache directory after sdist is complete
        try:
            shutil.rmtree(dest)
            self.announce(f"Cleaned up temporary jar directory: {dest}", level=LOG_INFO)
        except Exception as cleanup_error:
            self.announce(f"Failed to clean up {dest}: {cleanup_error}", level=LOG_WARN)
