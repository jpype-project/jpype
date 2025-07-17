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
package python.lang;

/**
 * This package holds all the concrete classes we support from Python.
 *
 * We implement a Java collection interface if it does not interfere with the
 * Python behaviors. We use Java case conventions or map the the nearest Java
 * concept name so long as it the method has the same concept. We use
 * CharSequence rather than String so that both PyString and Java String can be
 * taken as an argument.
 *
 * Return types are as tight as can be supported by Python.
 *
 * Parameter types are as lose as possible to accept a wide range of arguments.
 *
 * Some methods or combination of arguments may not be available in Java on
 * these wrappers but can be accessed using the
 * {@link org.jpype.bridge.BuiltIn#eval eval} method. Thus type restrictions can
 * generally be avoided when they are burdensome.
 *
 * Not every protocol will be available on every object as Python objects may
 * pick and choose what protocols they want to implement. Exceptions will be
 * thrown if the action is not allowed.
 *
 */
