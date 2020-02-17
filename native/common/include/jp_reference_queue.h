/*****************************************************************************
   Copyright 2004-2008 Steve Ménard

   Licensed under the Apache License, Version 2.0 (the "License
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

	   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.

 *****************************************************************************/
#ifndef JP_REFERENCE_QUEUE_H__
#define JP_REFERENCE_QUEUE_H__
#include <jpype.h>

extern "C"
{
typedef void (*JCleanupHook)(void*) ;
}

namespace JPReferenceQueue
{
void init();
void startJPypeReferenceQueue();
void shutdown();
void registerRef( jobject obj, PyObject*  targetRef);
void registerRef(jobject obj, void* host, JCleanupHook func);
} // end of namespace JPReferenceQueue

#endif
