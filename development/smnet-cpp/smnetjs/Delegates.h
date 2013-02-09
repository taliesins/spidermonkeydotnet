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
#pragma once

using namespace System;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	ref class SMScript;
	ref class SMErrorReport;

	public delegate void SMInvokeDelegate();

	public delegate void ScriptErrorHandler(SMScript^ script, SMErrorReport^ report);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMNative(JSContext* cx, uintN argc, jsval* vp);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMPropertyOp(JSContext* cx, JSObject* obj, jsid id, jsval* vp);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMStrictPropertyOp(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate void SMFinalizeOp(JSContext* cx, JSObject* obj);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMEnumerateOp(JSContext* cx, JSObject* obj, JSIterateOp enum_op, jsval *statep, jsid *idp);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMResolveOp(JSContext *cx, JSObject *obj, jsid id, uintN flags, JSObject **objp);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
	delegate JSBool SMOperationCallback(JSContext* cx);

	[UnmanagedFunctionPointer(CallingConvention::Cdecl)]
    delegate void SMErrorReporter(JSContext* cx, const char* message, JSErrorReport* report);
}