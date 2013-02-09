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