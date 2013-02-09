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

#include "stdafx.h"
#include "Delegates.h"
#include "SMContext.h"

using namespace System;
using namespace System::Text;
using namespace System::Threading;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	ref class SMRuntime;
	ref class SMPrototype;

	public ref class SMScript sealed
	{
	internal:
		int hashCode;
		int errorDepth;
		int execTimeout;

		bool cancelOp;
		bool isCompiled;
		
		Timer^ opTimer;
		String^ mName;
		
		JSObject* mScript;

		SMContext^ mContext;
		SMRuntime^ mRuntime;

		property JSContext* Context {
			JSContext* get() {
				return mContext->context;
			}
		}

		JSErrorReporter* mOldReporter;
		SMOperationCallback^ opcallback;

		SMScript(String^ name, SMRuntime^ runtime);
		SMScript(String^ name, SMRuntime^ runtime, SMPrototype^ global);

		void Initialize(SMPrototype^ proto);

		void ScheduleTimer();
		void KillTimer();

		void OperationTimer(Object^ state);
		JSBool OperationCallback(JSContext* cx);

		void SetErrorReporter(SMErrorReporter^ reporter);
	public:
		~SMScript() { this->!SMScript(); }

		property String^ Name {
			String^ get() {
				return this->mName;
			}
		}

		property IntPtr ContextPtr {
			IntPtr get() {
				return IntPtr(this->Context);
			}
		}

		property IntPtr GlobalPtr {
			IntPtr get() {
				return IntPtr(this->mContext->GetGlobalObject());
			}
		}

		property SMRuntime^ Runtime {
			SMRuntime^ get() {
				return this->mRuntime;
			}
		}

		void CreateObject(Object^ obj);
		void DestroyObject(Object^ obj);

		void Invoke(SMInvokeDelegate^ action);

		IntPtr GetPrototypePointer(Type^ type);
		IntPtr GetInstancePointer(Object^ obj);

		Object^ GetGlobalProperty(String^ name);
		bool SetGlobalProperty(String^ name, Object^ value);
		
		void GarbageCollect();
		void SetOperationTimeout(uintN timeout);

		bool Compile(String^ source);

		bool Execute();
		generic<typename T> T Execute([Out] bool %success);

		String^ Eval(String^ source);
		String^ Eval(String^ source, [Out] bool %success);

		generic<typename T> T Eval(String^ source);
		generic<typename T> T Eval(String^ source, [Out] bool %success);

		void CallFunction(String^ name, ... array<Object^>^ args);
		void CallFunction(String^ name, [Out] bool %success, ... array<Object^>^ args);
		
		void CallFunction(String^ name, IntPtr obj, ... array<Object^>^ args);
		void CallFunction(String^ name, IntPtr obj, [Out] bool %success, ... array<Object^>^ args);

		generic<typename T> T CallFunction(String^ name, ... array<Object^>^ args);
		generic<typename T> T CallFunction(String^ name, [Out] bool %success, ... array<Object^>^ args);

		generic<typename T> T CallFunction(String^ name, IntPtr obj, ... array<Object^>^ args);
		generic<typename T> T CallFunction(String^ name, IntPtr obj, [Out] bool %success, ... array<Object^>^ args);

		virtual int GetHashCode() override;

		event EventHandler^ ScriptDisposing;
	private:
		generic<typename T> T GetResult(jsval rval);

		!SMScript();
	};

}