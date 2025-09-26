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
import _jpype
import jpype
import common


class ZZZTestCase(common.JPypeTestCase):

    def testShutdown(self):
        # Install a coverage hook
        instance = jpype.JClass("org.jpype.JPypeContext").getInstance()
        jpype.JClass("org.jpype.test.common.OnShutdown").addCoverageHook(instance)

        # Shutdown
        jpype.shutdownJVM()

        # Check that shutdown does not raise
        jpype._core._JTerminate()
