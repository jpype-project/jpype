/*
 *  Licensed under the Apache License, Version 2.0 (the "License"); you may not
 *  use this file except in compliance with the License. You may obtain a copy of
 *  the License at
 * 
 *  http://www.apache.org/licenses/LICENSE-2.0
 * 
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 *  WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 *  License for the specific language governing permissions and limitations under
 *  the License.
 * 
 *  See NOTICE file for details.
 */
package org.jpype.bridge;

/**
 * Internal behaviors used by the binding.
 *
 * Loaded by the _jpype module.
 *
 */
public class Natives
{

  native static void start(String[] modulePaths, String[] args,
          String name, String prefix, String home, String exec_prefix, String executable,
          boolean isolated, boolean fault_handler, boolean quiet, boolean verbose,
          boolean site_import, boolean user_site, boolean write_bytecode);

  native static void interactive();

  native static void finish();

}
