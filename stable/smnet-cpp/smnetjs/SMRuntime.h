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
#include "Delegates.h"

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	ref class SMPrototype;
	ref class SMScript;
	ref class SMInstance;

	generic<typename TKey, typename TValue>
	ref class SMHashtable;

	[Flags]
	public enum struct RuntimeOptions 
		: int
	{
		None = 0,
		DisableAutoEmbedding = 1,
		EnableCompartmentalGC = 2,
		ForceGCOnContextDestroy = 4,
	};

	public ref class SMRuntime sealed
	{
	internal:
		JSRuntime* rt;

		bool autoEmbed;
		bool destroyGC;
		bool compartmentGC;

		SMErrorReporter^ errorHandler;

		List<SMScript^>^ scripts;
		ReadOnlyCollection<SMScript^>^ pscripts;

		List<SMPrototype^>^ embedded;

		void SetOptions(RuntimeOptions opts);
		void Initialize();
		
		bool IsHandledType(Type^ type);
		bool IsValidGlobal(Type^ type);

		bool Embed(Type^ type, [Out] SMPrototype^ %proto);

		SMScript^ FindScript(JSContext* cx);

		SMPrototype^ FindProto(Type^ type);
		SMPrototype^ FindProto(SMScript^ script, JSObject* obj);

		SMInstance^ FindInstance(JSObject* obj);
	public:

		property IntPtr RuntimePtr {
			IntPtr get() {
				return IntPtr(rt);
			}
		}

		property ReadOnlyCollection<SMScript^>^ Scripts {
			ReadOnlyCollection<SMScript^>^ get() {
				return this->pscripts;
			}
		}

		SMRuntime();
		SMRuntime(RuntimeOptions opts);

		~SMRuntime() { this->!SMRuntime(); }

		bool Embed(Type^ type);
		
		SMScript^ InitScript(String^ name);
		SMScript^ InitScript(String^ name, Type^ global);

		SMScript^ FindScript(String^ name);

		void DestroyScript(String^ name);
		void DestroyScript(SMScript^ script);

		IntPtr GetPrototypePointer(SMScript^ script, Type^ type);
		IntPtr GetInstancePointer(SMScript^ script, Object^ obj);

		void GarbageCollect();

		event ScriptErrorHandler^ OnScriptError;
	private:
		void ScriptError(JSContext* cx, const char* message, JSErrorReport* report);

		!SMRuntime();
	};

}