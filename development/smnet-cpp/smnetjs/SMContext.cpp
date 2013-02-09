/*
spidermonkey-dotnet: a binding between the SpiderMonkey  JavaScript engine and the .Net platform. 
Copyright (C) 2012 Derek Petillo, Ryan McLeod

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

*/
#include "stdafx.h"
#include "SMContext.h"
#include "SMRuntime.h"
#include "SMPrototype.h"

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

SMContext::SMContext(SMRuntime^ runtime, SMPrototype^ proto)
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