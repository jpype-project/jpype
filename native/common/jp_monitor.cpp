// --- file: common/jp_monitor.cpp ---
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
 *****************************************************************************/
#include "jpype.h"
#include "jp_monitor.h"

JPMonitor::JPMonitor(JPJavaFrame& frame, jobject value)
{
	m_Context = frame.getContext();
	m_Value = frame.storeGlobal(value);
}

JPMonitor::~JPMonitor()
{
	tryRelease(m_Value);
}

void JPMonitor::enter()
{
	// This can hold off for a while so we need to release resource
	// so that we don't dead lock.
	JPPyCallRelease call;
	JPJavaFrame frame = JPJavaFrame::outer(m_Context);
	frame.MonitorEnter(frame.retrieveGlobal(m_Value));
}

void JPMonitor::exit()
{
	JPJavaFrame frame = JPJavaFrame::outer(m_Context);
	frame.MonitorExit(frame.retrieveGlobal(m_Value));
}

