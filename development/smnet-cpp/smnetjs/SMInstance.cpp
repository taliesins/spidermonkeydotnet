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
#include "SMScript.h"
#include "SMInstance.h"

using namespace smnetjs;

SMInstance::SMInstance(SMScript^ script, JSObject* jsobj, Object^ instance)
{
	this->script = script;
	this->jsobj = jsobj;
	this->instance = instance;

	script->ScriptDisposing += gcnew EventHandler(this, &SMInstance::OnScriptDisposed);
}

SMInstance::!SMInstance() 
{
	script->ScriptDisposing -= gcnew EventHandler(this, &SMInstance::OnScriptDisposed);
	script = nullptr;

	instance = nullptr;
	jsobj = NULL;
}

void SMInstance::GCLock()
{
	lockCount++;

	if (lockCount == 1)
		JS_LockGCThing(script->Context, jsobj);
}

void SMInstance::GCUnlock(bool force)
{
	if (lockCount > 1 && force)
		lockCount = 1;

	if (lockCount == 1)
		JS_UnlockGCThing(script->Context, jsobj);

	lockCount--;
}

void SMInstance::OnScriptDisposed(Object^ sender, EventArgs^ e)
{
	GCUnlock(true);
}