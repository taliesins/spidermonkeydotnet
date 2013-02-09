#pragma once
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

		IntPtr SMScript::GetPrototypePointer(Type^ type);
		IntPtr SMScript::GetInstancePointer(Object^ obj);

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