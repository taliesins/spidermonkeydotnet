/*

= Singleton Development License =

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

 * This is a license for research and development use only, and does not 
include a license for commercial use. If you desire a license for commercial 
use, you must contact the copyright owner for a commercial use license, which
would supersede the terms of this license. "Commercial Use" means any use 
(internal or external), copying, sublicensing or distribution internal or 
external), directly or indirectly, for commercial or strategic gain or 
advantage, including use in internal operations or in providing products or 
services to any third party. Research and development for eventual commercial
use is not "Commercial Use" so long as a commercial use license is obtained 
prior to commercial use. Redistribution to others for their research and 
development use is not "Commercial Use".

 * Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer. 

 * Redistributions in binary form must reproduce the above copyright notice, 
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution. Redistribution in binary form 
does not require redistribution of source code.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
THE POSSIBILITY OF SUCH DAMAGE.
 
*/
#include "stdafx.h"
#include "SMContext.h"
#include "SMRuntime.h"

using namespace smnetjs;


SMContext::SMContext(SMRuntime^ runtime)
{
	this->monitor = PR_NewMonitor();
	this->context = JS_NewContext(runtime->rt, 8192);

	this->runtime = runtime;
	JSAutoRequest ar(context);

	JS_SetVersion(context, JSVERSION_LATEST);
	JS_SetOptions(context, JS_GetOptions(context) | JSOPTION_ANONFUNFIX | JSOPTION_VAROBJFIX);

	JS_SetGCParameterForThread(context, JSGC_MAX_CODE_CACHE_BYTES, 16 * 1024 * 1024);

	globalobj = JS_NewCompartmentAndGlobalObject(context, &global_class, NULL);

	JSAutoEnterCompartment ac;
	ac.enterAndIgnoreErrors(context, globalobj);

	JS_InitStandardClasses(context, globalobj);
	JS_SetGlobalObject(context, globalobj);
}

SMContext::!SMContext()
{
	PR_EnterMonitor(monitor);

	if (context) {
		if (ShouldLock())
			JS_SetContextThread(context);

		if (runtime->destroyGC)
			JS_DestroyContext(context);
		else
			JS_DestroyContextMaybeGC(context);

		context = NULL;
		globalobj = NULL;
	}

	while(lockDepth-- >= 0) 
		PR_ExitMonitor(monitor);
	
	PR_DestroyMonitor(monitor);
}


void SMContext::Lock()
{
	PR_EnterMonitor(monitor);
	
	if (ShouldLock() && lockDepth == 0) {

		currThread = Thread::CurrentThread;
		JS_SetContextThread(context);
	}

	lockDepth++;
	JS_BeginRequest(context);
}

bool SMContext::ShouldLock()
{
	return (currThread != Thread::CurrentThread);
}

bool SMContext::ShouldUnlock()
{
	return (currThread == Thread::CurrentThread);
}

void SMContext::Unlock()
{
	lockDepth--;
	JS_EndRequest(context);

	if (ShouldUnlock() && lockDepth == 0) {

		JS_ClearContextThread(context);
		currThread = nullptr;
	}

	PR_ExitMonitor(monitor);
}


JSObject* SMContext::GetGlobalObject()
{
	return globalobj;
}