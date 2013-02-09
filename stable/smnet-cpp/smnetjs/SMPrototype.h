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
#include "Attributes.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Linq;
using namespace System::Linq::Expressions;
using namespace System::Collections::Generic;

namespace smnetjs {

	ref class SMRuntime;
	ref class SMEmbedded;
	ref class SMConstructorInfo;
	ref class SMMethodInfo;
	ref class SMMemberInfo;
	ref class SMEvent;
	ref class SMInstance;

	generic<typename TKey, typename TValue>
	ref class SMHashtable;

	ref class SMPrototype sealed
	{
	internal:
		System::Type^ type;
		JSClass* clasp;

		SMRuntime^ runtime;
		SMEmbedded^ attribute;

		JSPropertySpec* props;
		JSPropertySpec* sprops;

		JSFunctionSpec* funcs;
		JSFunctionSpec* sfuncs;

		//callback delegates
		SMNative^ ctor;
		SMNative^ method;

		SMPropertyOp^ getter;
		SMStrictPropertyOp^ setter;

		SMFinalizeOp^ finalize;

		SMHashtable<long, long>^ jsprototypes;
		SMHashtable<long, SMInstance^>^ jsinstances;
		SMHashtable<long, SMEvent^>^ jsevents;

		List<SMConstructorInfo^>^ constructors;

		List<SMMemberInfo^>^ properties;
		List<SMMemberInfo^>^ sproperties;

		SMHashtable<String^, List<SMMethodInfo^>^>^ functions;
		SMHashtable<String^, List<SMMethodInfo^>^>^ sfunctions;
	public:
		property System::Type^ Type {
			System::Type^ get() {
				return this->type;
			}
		}

		property String^ Name {
			String^ get() {
				if (attribute != nullptr)
					if (!String::IsNullOrWhiteSpace(attribute->Name))
						return attribute->Name;

				return type->Name;
			}
		}

		property String^ AccessibleName {
			String^ get() {
				if (attribute != nullptr)
					if (!String::IsNullOrWhiteSpace(attribute->AccessibleName))
						return attribute->AccessibleName;

				return this->Name;
			}
		}
	internal:

		SMPrototype(SMRuntime^ runtime, System::Type^ type);
		~SMPrototype() { this->!SMPrototype(); }

		void Reflect();

		void ReleaseInstance(SMScript^ script, Object^ object);

		JSObject* CreateInstance(SMScript^ script, Object^ object);
		JSObject* CreateInstance(SMScript^ script, Object^ object, bool lock);

		JSObject* CreateInstance(SMScript^ script, SMPrototype^ proto, SMEvent^ object);
	private:
		!SMPrototype();

		void CreateJSClass();

		void ReflectConstructors();
		void ReflectFunctions();
		void ReflectProperties();

		bool IsValidMethod(MethodInfo^ method);

		void FreeFunctionSpec(JSFunctionSpec* specs);
		void FreePropertySpec(JSPropertySpec* specs);

		JSBool Constructor(JSContext* cx, uintN argc, jsval* vp);

		JSBool Getter(JSContext* cx, JSObject* obj, jsid id, jsval* vp);
		JSBool Setter(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp);

		JSBool Method(JSContext* cx, uintN argc, jsval* vp);

		void Finalizer(JSContext* cx, JSObject* obj);
	};

}