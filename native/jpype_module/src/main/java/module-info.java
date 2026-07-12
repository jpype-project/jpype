// --- file: module-info.java ---
/*****************************************************************************
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  See NOTICE file for details.
**************************************************************************** */


module org.jpype {
  requires transitive java.xml;
  requires java.sql;
  requires java.management;
  requires transitive java.logging;
  requires java.scripting;

  // ==========================================
  // PUBLIC USER API
  // ==========================================
  exports org.jpype;                    // Crucial for NativeContext & PyExceptionProxy
  exports org.jpype.annotation;         // For extension modules
  exports org.jpype.pickle;             // For data serialization pipelines
  exports org.jpype.pkg;                // For interacting with Python package spaces
  exports org.jpype.script;             // JSR-223 javax.script.ScriptEngine compatibility shim

  // Python wrapper language spaces
  exports python.lang;
  exports python.exceptions;
  exports python.collections;            // PyDeque, PyCounter, PyOrderedDict, PyDefaultDict - SPI-backed, see PyCollectionsWrapperService
  exports python.io;
  exports python.datetime;               // PyDate, PyDateTime, PyTimeDelta - SPI-backed, see PyDatetimeWrapperService
  exports python.pathlib;                // PyPath - SPI-backed, see PyPathlibWrapperService
  exports python.decimal;                // PyDecimal - SPI-backed, see PyDecimalWrapperService

  // Documentation and Tooling
  exports org.jpype.html;
  exports org.jpype.javadoc;

  // ==========================================
  // SPI
  // ==========================================
  uses org.jpype.WrapperService;
  provides org.jpype.WrapperService with python.io.PyIOWrapperService,
          python.collections.PyCollectionsWrapperService,
          python.datetime.PyDatetimeWrapperService,
          python.pathlib.PyPathlibWrapperService,
          python.decimal.PyDecimalWrapperService;
  provides javax.script.ScriptEngineFactory with org.jpype.script.JPypeScriptEngineFactory;

  // ==========================================
  // REFLECTION / REFLECTIVE ACCESS
  // ==========================================
  opens python.lang;
  opens python.io;
  opens python.collections;
  exports org.jpype.internal to java.base;
}
