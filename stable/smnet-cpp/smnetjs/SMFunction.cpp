#include "stdafx.h"
#include "Interop.h"
#include "TypeInfo.h"
#include "SMRuntime.h"
#include "SMBinder.h"
#include "SMFunction.h"
#include "SMScript.h"
#include "SMMemberInfo.h"
#include "SMPrototype.h"
#include "SMHashtable.h"

using namespace smnetjs;

SMFunction::SMFunction(SMScript^ script, jsval funcval)
{
	this->funcval = funcval;	
	this->script = script;

	instanceCount++;
	currInstance = instanceCount;

	char* pname = Interop::ToUnmanagedString(String::Format("SMFunction{0}", currInstance));

	JS_DefineProperty(
		script->Context, 
		script->mContext->GetGlobalObject(),
		pname, funcval, NULL, NULL, 
		JSPROP_READONLY | JSPROP_PERMANENT);

	Marshal::FreeHGlobal(IntPtr(pname));
}

SMFunction::!SMFunction()
{
	instanceCount--;
	
	if (script->mContext != nullptr) {
		try {
			script->mContext->Lock();

			char* pname = Interop::ToUnmanagedString(String::Format("SMFunction{0}", currInstance));

			JSBool foundp = JS_FALSE;

			JS_SetPropertyAttributes(
				script->Context,
				script->mContext->GetGlobalObject(),
				pname, 0, &foundp);

			JS_DeleteProperty(
				script->Context, 
				script->mContext->GetGlobalObject(), pname);

			Marshal::FreeHGlobal(IntPtr(pname));
		} finally {
			script->mContext->Unlock();
		}
	}
}



Object^ SMFunction::Invoke(... array<Object^>^ args)
{
	try {

		if (!released) {
			script->mContext->Lock();

			JSFunction* func = JS_ValueToFunction(script->Context, funcval);
			if (!func) return nullptr;

			uint32 flags = JS_GetFunctionFlags(func);

			if (flags == 0) {
				JSObject* funcobj = JS_GetFunctionObject(func);
				if (!funcobj) return nullptr;

				JSObject* fobj = JS_GetParent(script->Context, funcobj);
				if (!fobj) fobj = script->mContext->GetGlobalObject();

				return SMBinder::CallCLRFunction(script, IntPtr(fobj), GetName(), args);
			}
			else
				return SMBinder::CallLambdaFunction(script, funcval, args);
		}

	} catch(Exception^ e) {
		JS_ReportError(script->Context, 
			"%s (SMFunction.Invoke): %s", 
			e->InnerException->GetType()->ToString(),
			e->InnerException->Message);
	}
	finally { if(!released) script->mContext->Unlock(); }

	return nullptr;
}


String^ SMFunction::GetName() 
{
	JSFunction* func = JS_ValueToFunction(script->Context, funcval);
	if (!func) return String::Empty;

	JSString* id = JS_GetFunctionId(func);
	if (!id) return String::Empty;

	char* nameBytes = JS_EncodeString(script->Context, id);
	String^ name = Interop::ToManagedString(nameBytes);

	JS_free(script->Context, nameBytes);
	return name;
}


String^ SMFunction::ToString()
{
	return this->ToString(true);
}

String^ SMFunction::ToString(bool AsJSVal)
{
	if (AsJSVal) {
		try {
			script->mContext->Lock();

			JSFunction* func = JS_ValueToFunction(script->Context, funcval);
			if (!func) return nullptr;

			jsval rval = OBJECT_TO_JSVAL(JS_GetFunctionObject(func));
			return Interop::FromJSValToManagedString(script->Context, rval);
		} finally {
			script->mContext->Unlock();
		}
	}
	else return __super::ToString();
}