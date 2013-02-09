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
#include "Interop.h"
#include "TypeInfo.h"
#include "SMScript.h"
#include "SMHashtable.h"
#include "SMRuntime.h"
#include "SMPrototype.h"

using namespace smnetjs;


SMScript::SMScript(String^ name, SMRuntime^ runtime)
{
	mName = name;
	mRuntime = runtime;

	execTimeout = 0;
	hashCode = __super::GetHashCode();

	opTimer = gcnew Timer(
			  gcnew TimerCallback(this, &SMScript::OperationTimer), nullptr, Timeout::Infinite, Timeout::Infinite);

	opcallback = gcnew SMOperationCallback(this, &SMScript::OperationCallback);

	mContext = gcnew SMContext(runtime);
	mContext->Lock();

	JS_SetOperationCallback(Context, (JSOperationCallback)Interop::DelegatePointer(opcallback));

	mContext->Unlock();
}

SMScript::SMScript(String^ name, SMRuntime^ runtime, SMPrototype^ global)
{
	mName = name;
	mRuntime = runtime;

	hashCode = __super::GetHashCode();

	opTimer = gcnew Timer(
			  gcnew TimerCallback(this, &SMScript::OperationTimer), nullptr, Timeout::Infinite, Timeout::Infinite);

	opcallback = gcnew SMOperationCallback(this, &SMScript::OperationCallback);

	mContext = gcnew SMContext(runtime);
	mContext->Lock();

	JS_SetOperationCallback(Context, (JSOperationCallback)Interop::DelegatePointer(opcallback));

	JSObject* obj = mContext->GetGlobalObject();
	
	global->jsprototypes->Add((long)Context, (long)obj);

	JS_DefineFunctions(Context, obj, global->sfuncs);
	JS_DefineProperties(Context, obj, global->sprops);

	mContext->Unlock();
}


SMScript::!SMScript()
{
	KillTimer();
	delete opTimer;

	ScriptDisposing(this, EventArgs::Empty);

	mRuntime->scripts->Remove(this);

	for (int i = 0; i < mRuntime->embedded->Count; i++) {
		SMPrototype^ proto = mRuntime->embedded[i];
		proto->jsprototypes->Remove((long)Context);
	}

	delete mContext;
	mContext = nullptr;
}


void SMScript::Initialize(SMPrototype^ proto)
{
	mContext->Lock();

	JSObject* obj;

	if (TypeInfo::IsStatic(proto->type)) {
		char* name = Interop::ToUnmanagedString(proto->AccessibleName);

		uint32 pflags = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;
		obj = JS_DefineObject(Context, mContext->GetGlobalObject(), name, proto->clasp, NULL, pflags);

		JS_DefineFunctions(Context, obj, proto->sfuncs);
		JS_DefineProperties(Context, obj, proto->sprops);

		Marshal::FreeHGlobal(IntPtr(name));
	}
	else {
		obj = JS_InitClass(Context, 
				mContext->GetGlobalObject(), 
				NULL, 
				proto->clasp, 
				(JSNative)Interop::DelegatePointer(proto->ctor), 0,
				proto->props, 
				proto->funcs, 
				proto->sprops, 
				proto->sfuncs);
	}

	proto->jsprototypes->Add((long)Context, (long)obj);
	mContext->Unlock();
}


void SMScript::ScheduleTimer()
{
	if (execTimeout > 0)
		opTimer->Change((uintN)execTimeout, (uintN)Timeout::Infinite);
}

void SMScript::KillTimer()
{
	opTimer->Change(Timeout::Infinite, Timeout::Infinite);
	cancelOp = false;
}


void SMScript::CreateObject(Object^ obj)
{
	SMPrototype^ proto = mRuntime->FindProto(obj->GetType());

	if (proto != nullptr)
		proto->CreateInstance(this, obj, true);
}

void SMScript::DestroyObject(Object^ obj)
{
	SMPrototype^ proto = mRuntime->FindProto(obj->GetType());

	if (proto != nullptr)
		proto->ReleaseInstance(this, obj);
}


IntPtr SMScript::GetPrototypePointer(Type^ type)
{
	return mRuntime->GetPrototypePointer(this, type);
}

IntPtr SMScript::GetInstancePointer(Object^ obj)
{
	return mRuntime->GetInstancePointer(this, obj);
}


void SMScript::GarbageCollect()
{
	if (Context) {
		mContext->Lock();
		JS_GC(Context);
		mContext->Unlock();
	}
}

void SMScript::SetOperationTimeout(uintN timeout)
{
	execTimeout = timeout;
}


void SMScript::OperationTimer(Object^ state)
{
	cancelOp = true;
	JS_TriggerOperationCallback(Context);
}

JSBool SMScript::OperationCallback(JSContext* cx)
{
	if (!cancelOp)
		return JS_TRUE;

	cancelOp = false;
	JS_ClearPendingException(cx);

	return JS_FALSE;
}


void SMScript::SetErrorReporter(SMErrorReporter^ reporter)
{
	JSErrorReporter old = JS_SetErrorReporter(
		Context,
		(JSErrorReporter)Interop::DelegatePointer(reporter));

	mOldReporter = &old;
}


bool SMScript::Compile(String^ source)
{
	if (isCompiled) 
		return true;

	mContext->Lock();

	source = Encoding::ASCII->GetString(
			 Encoding::UTF8->GetBytes(source));

	char* pname = Interop::ToUnmanagedString(mName);
	char* pcode = Interop::ToUnmanagedString(source);

	try {
		JS_ClearPendingException(mContext->context);
		mScript = JS_CompileScript(mContext->context, mContext->GetGlobalObject(), pcode, source->Length, pname, 0);
		
		if (mScript)
			isCompiled = true;
	}
	catch(...) {
		return false;
	}
	finally {
		mContext->Unlock();

		Marshal::FreeHGlobal(IntPtr(pname));
		Marshal::FreeHGlobal(IntPtr(pcode));
	}

	return isCompiled;
}


bool SMScript::Execute()
{
	bool success = false;
	Execute<Object^>(success);

	return success;
}

generic<typename T> T SMScript::Execute([Out] bool %success)
{
	if (!isCompiled)
		return T();

	T ret;

	success = false;
	mContext->Lock();

	try {
		ScheduleTimer();
		
		jsval rval;
		if (JS_ExecuteScript(mContext->context, mContext->GetGlobalObject(), mScript, &rval)) {
			success = true;

			ret = GetResult<T>(rval);
			JS_MaybeGC(mContext->context);
		}
		else ret = T();
	}
	catch(...) {
		return T();
	}
	finally {
		KillTimer();
		mContext->Unlock();
	}

	return ret;
}


String^ SMScript::Eval(String^ source) 
{
	bool success = false;
	return Eval(source, success);
}

String^ SMScript::Eval(String^ source, [Out] bool %success)
{
	return Eval<String^>(source, success);
}


generic<typename T> T SMScript::Eval(String^ source)
{
	bool success = false;
	return Eval<T>(source, success);
}

generic<typename T> T SMScript::Eval(String^ source, [Out] bool %success)
{
	T ret;

	success = false;
	mContext->Lock();

	source = Encoding::ASCII->GetString(
			 Encoding::UTF8->GetBytes(source));

	char* pname = Interop::ToUnmanagedString(mName);
	char* pcode = Interop::ToUnmanagedString(source);

	try {
		ScheduleTimer();

		JS_ClearPendingException(Context);

		jsval rval = NULL;
		if (JS_EvaluateScript(Context, mContext->GetGlobalObject(), pcode, source->Length, pname, 0, &rval)) {
			success = true;

			ret = GetResult<T>(rval);
			JS_MaybeGC(mContext->context);
		}
		else ret = T();
	}
	catch(...) {
		return T();
	}
	finally {
		KillTimer();

		mContext->Unlock();

		Marshal::FreeHGlobal(IntPtr(pname));
		Marshal::FreeHGlobal(IntPtr(pcode));
	}

	return ret;
}


void SMScript::CallFunction(String^ name, ... array<Object^>^ args)
{
	bool success = false;
	CallFunction(name, success, args);
}

void SMScript::CallFunction(String^ name, [Out] bool %success, ... array<Object^>^ args)
{
	CallFunction<Object^>(name, success, args);
}
	

void SMScript::CallFunction(String^ name, IntPtr obj, ... array<Object^>^ args)
{
	bool success = false;
	CallFunction(name, obj, success, args);
}

void SMScript::CallFunction(String^ name, IntPtr obj, [Out] bool %success, ... array<Object^>^ args)
{
	CallFunction<Object^>(name, obj, success, args);
}


generic<typename T> T SMScript::CallFunction(String^ name, ... array<Object^>^ args)
{
	bool success = false;
	return SMScript::CallFunction<T>(name, success, args);
}

generic<typename T> T SMScript::CallFunction(String^ name, [Out] bool %success, ... array<Object^>^ args)
{
	T ret;

	success = false;
	mContext->Lock();

	char* pname = Interop::ToUnmanagedString(name);

	jsval* argv = new jsval[args->Length];

	for(int i = 0; i < args->Length; i++) {
		Interop::ToJSVal(this, args[i], &argv[i]);
	}

	try 
	{
		ScheduleTimer();

		jsval rval;
		if (JS_CallFunctionName(Context, mContext->GetGlobalObject(), pname, args->Length, argv, &rval)) {
			success = true;

			ret = GetResult<T>(rval);
			JS_MaybeGC(Context);
		}
		else ret = T();
	}
	catch(...) {
		return T();
	}
	finally {
		KillTimer();
		delete argv;

		mContext->Unlock();

		Marshal::FreeHGlobal(IntPtr(pname));
	}

	return ret;
}


generic<typename T> T SMScript::CallFunction(String^ name, IntPtr obj, ... array<Object^>^ args)
{
	bool success = false;
	return CallFunction<T>(name, obj, success, args);
}

generic<typename T> T SMScript::CallFunction(String^ name, IntPtr obj, [Out] bool %success, ... array<Object^>^ args)
{
	T ret;

	success = false;
	mContext->Lock();

	char* pname = Interop::ToUnmanagedString(name);

	jsval* argv = new jsval[args->Length];

	for(int i = 0; i < args->Length; i++) {
		Interop::ToJSVal(this, args[i], &argv[i]);
	}

	try 
	{
		JSObject* jsobj = (JSObject*)obj.ToPointer();

		ScheduleTimer();

		jsval rval;
		if (JS_CallFunctionName(Context, jsobj, pname, args->Length, argv, &rval)) {
			success = true;

			ret = GetResult<T>(rval);
			JS_MaybeGC(Context);
		}
		else ret = T();
	}
	catch(...) {
		return T();
	}
	finally {
		KillTimer();
		delete argv;

		mContext->Unlock();

		Marshal::FreeHGlobal(IntPtr(pname));
	}

	return ret;
}


generic<typename T> T SMScript::GetResult(jsval rval)
{
	T ret;
	if (rval == JSVAL_NULL) 
		ret = T();
		
	else if (String::typeid == T::typeid) {
		ret = (T)Interop::FromJSValToManagedString(Context, rval);
	} else {
		Object^ retobj = Interop::FromJSVal(this, rval);

		if (Interop::ChangeType(this, retobj, T::typeid))
			ret = (T)retobj;

		else ret = T();
	}

	return ret;
}


int SMScript::GetHashCode()
{
	return hashCode;
}