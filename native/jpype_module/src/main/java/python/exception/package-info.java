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
package python.exception;

/**
 * Package for Python Exceptions.
 *
 * Python has its own exception tree. Unfortunately, while some of the
 * exceptions are good match for Java they are organized with a different tree.
 * Some exceptions only map the Java exceptions under certain conditions thus
 * there it is not possible to get a completely one to one mapping.
 *
 * Instead we have wrapped Python exceptions with the same relationships they
 * have in its native form. This will allow the easiest mapping from Python
 * documented code to Java.
 *
 * The exception types are all identical aside from the front end which is
 * required to catch by type. Every class is backed by the native PyExc class.
 *
 */
// As to why exception warranted its own package?  Because there is a lot of
// them.
//
// To save bytes I have omitted the usual license header.
