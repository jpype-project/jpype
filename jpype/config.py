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

# This files holds hints used to control behavior of the JVM
# It is global variables rather than options to a function so that
# they can be set a module at any time rather than requiring arguments
# for a function.
#
# These options are to be treated as hints which may or may not be supported
# in future versions.  Setting an unsupported option shall have no effect.
# Variables shall not be removed even if deprecated so that conditions
# based on these will be valid in all future versions.
#

onexit = True
"""If this is False, the JVM not will be notified of Python exit.  Java will not execute any
resource cleanup routines."""

destroy_jvm = True
""" If this is False, the JVM will not execute any cleanup routines and memory will
not be freed."""

free_resources = True
""" If this is False, the resources will be allowed to leak after the shutdown call.
"""
