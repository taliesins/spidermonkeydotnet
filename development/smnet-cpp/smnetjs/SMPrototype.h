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

#include "Delegates.h"
#include "Attributes.h"

using namespace System;
using namespace System::Text;
using namespace System::Reflection;
using namespace System::Collections::Generic;

namespace smnetjs {

	ref class SMRuntime;
	ref class SMEmbedded;
	ref class SMConstructorInfo;
	ref class SMMethodInfo;
	ref class SMPropertyInfo;
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

		SMResolveOp^ resolve;

		SMPropertyOp^ getter;
		SMPropertyOp^ igetter;//indexer

		SMStrictPropertyOp^ setter;
		SMStrictPropertyOp^ isetter;//indexer
		
		SMEnumerateOp^ enumerate;

		SMFinalizeOp^ finalize;

		//instance tracking

		List<SMInstance^>^ jsinstances;
		SMHashtable<long, long>^ jsprototypes;

		//reflection information

		List<SMConstructorInfo^>^ constructors;

		List<SMPropertyInfo^>^ properties; //instance
		List<SMPropertyInfo^>^ sproperties; //static
		List<SMPropertyInfo^>^ iproperties; //indexed

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
					if (!String::IsNullOrEmpty(attribute->Name))
						return attribute->Name;

				return type->Name;
			}
		}

		property String^ AccessibleName {
			String^ get() {
				if (attribute != nullptr)
					if (!String::IsNullOrEmpty(attribute->AccessibleName))
						return attribute->AccessibleName;

				return this->Name;
			}
		}
	internal:

		SMPrototype(SMRuntime^ runtime, System::Type^ type);
		SMPrototype(SMRuntime^ runtime, System::Type^ type, SMEmbedded^ attribute);

		~SMPrototype() { this->!SMPrototype(); }

		void Reflect();

		SMInstance^ FindInstance(JSObject* obj);
		SMInstance^ FindInstance(SMScript^ script, Object^ obj);

		void ReleaseInstance(SMScript^ script, Object^ object);

		JSObject* CreateInstance(SMScript^ script, Object^ object);
		JSObject* CreateInstance(SMScript^ script, Object^ object, bool lock);
	private:
		!SMPrototype();

		void CreateJSClass();

		void ReflectConstructors();
		void ReflectFunctions();
		void ReflectProperties();

		bool HasMemberName(String^ name);

		void FreeFunctionSpec(JSFunctionSpec* specs);
		void FreePropertySpec(JSPropertySpec* specs);

		JSBool Constructor(JSContext* cx, uintN argc, jsval* vp);

		JSBool IndexGetter(JSContext* cx, JSObject* obj, jsid id, jsval* vp);
		JSBool IndexSetter(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp);

		JSBool Getter(JSContext* cx, JSObject* obj, jsid id, jsval* vp);
		JSBool Setter(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp);

		JSBool Method(JSContext* cx, uintN argc, jsval* vp);
		JSBool Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op, jsval *statep, jsid *idp);

		void Finalizer(JSContext* cx, JSObject* obj);
	internal:
		//Generic support!?
		List<SMConstructorInfo^>^                   ReflectGenericConstructors(array<System::Type^>^ types);

		List<SMPropertyInfo^>^                      ReflectGenericIndexProperties(array<System::Type^>^ types);
		List<SMPropertyInfo^>^                      ReflectGenericProperties(array<System::Type^>^ types, bool _static);

		SMHashtable<String^, List<SMMethodInfo^>^>^ ReflectGenericFunctions(array<System::Type^>^ types, bool _static);
	};

}