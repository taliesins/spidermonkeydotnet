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

	mContext = gcnew SMContext(runtime, global);
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

	for each(DictionaryEntry^ entry in mRuntime->embedded) {

		SMPrototype^ proto = (SMPrototype^)entry->Value;
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
		char* name = Interop::ToUnmanagedStringInternal(proto->AccessibleName);

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
	mContext->Lock();

	SMPrototype^ proto = mRuntime->FindProto(obj->GetType());

	if (proto != nullptr)
		proto->CreateInstance(this, obj, true);

	mContext->Unlock();
}

void SMScript::DestroyObject(Object^ obj)
{
	mContext->Lock();

	SMPrototype^ proto = mRuntime->FindProto(obj->GetType());

	if (proto != nullptr)
		proto->ReleaseInstance(this, obj);

	mContext->Unlock();
}


void SMScript::Invoke(SMInvokeDelegate^ action) {
	mContext->Lock();

	action->DynamicInvoke();

	mContext->Unlock();
}


IntPtr SMScript::GetPrototypePointer(Type^ type)
{
	return mRuntime->GetPrototypePointer(this, type);
}

IntPtr SMScript::GetInstancePointer(Object^ obj)
{
	return mRuntime->GetInstancePointer(this, obj);
}


Object^ SMScript::GetGlobalProperty(String^ name) {
	mContext->Lock();

	jsval rval;
	char* pname = Interop::ToUnmanagedStringInternal(name);
	
	JS_GetProperty(Context, mContext->GetGlobalObject(), pname, &rval);

	Marshal::FreeHGlobal(IntPtr(pname));

	Object^ ret = Interop::FromJSVal(this, rval);

	mContext->Unlock();

	return ret;
}

bool SMScript::SetGlobalProperty(String^ name, Object^ value) {
	mContext->Lock();

	jsval rval;
	char* pname = Interop::ToUnmanagedStringInternal(name);
	
	Interop::ToJSVal(this, value, &rval);
	bool ret = JS_SetProperty(Context, mContext->GetGlobalObject(), pname, &rval);

	Marshal::FreeHGlobal(IntPtr(pname));

	mContext->Unlock();
	return ret;
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

	char* pname = Interop::ToUnmanagedStringInternal(mName);
	char* pcode = Interop::ToUnmanagedStringInternal(source);

	try {
		JS_ClearPendingException(mContext->context);
		mScript = JS_CompileScript(mContext->context, mContext->GetGlobalObject(), pcode, strlen(pcode), pname, 0);
		
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

	char* pname = Interop::ToUnmanagedStringInternal(mName);
	char* pcode = Interop::ToUnmanagedStringInternal(source);

	try {
		ScheduleTimer();

		JS_ClearPendingException(Context);

		jsval rval = NULL;
		if (JS_EvaluateScript(Context, mContext->GetGlobalObject(), pcode, strlen(pcode), pname, 0, &rval)) {
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

	char* pname = Interop::ToUnmanagedStringInternal(name);

	jsval* argv = new jsval[args->Length];

	for(int i = 0; i < args->Length; i++)
		Interop::ToJSVal(this, args[i], &argv[i]);

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

	char* pname = Interop::ToUnmanagedStringInternal(name);

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