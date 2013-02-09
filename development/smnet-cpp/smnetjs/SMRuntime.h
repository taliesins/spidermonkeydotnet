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

using namespace System;
using namespace System::Threading;
using namespace System::Collections::Generic;
using namespace System::Collections::ObjectModel;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	ref class SMPrototype;
	ref class SMScript;
	ref class SMInstance;
	ref class SMEmbedded;

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

		SMHashtable<Type^, SMPrototype^>^ embedded;

		void SetOptions(RuntimeOptions opts);
		void Initialize();
		
		bool IsHandledType(Type^ type);
		bool IsHandledType(String^ name);

		Type^ ToHandledType(String^ name);

		bool IsValidGlobal(Type^ type);

		bool Embed(Type^ type, [Out] SMPrototype^ %proto);
		bool Embed(Type^ type, SMEmbedded^ attribute, [Out] SMPrototype^ %proto);

		SMScript^ FindScript(JSContext* cx);

		System::Type^ FindType(String^ name);

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
		bool Embed(Type^ type, SMEmbedded^ attribute);
		
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

		static array<String^>^ handledTypes = gcnew array<String^> { 
			"Void", "Array", "Boolean", "Date", "Error", "EvalError", "Function", 
			"Infinity", "Math", "NaN", "Number", "Object", "RangeError", 
			"ReferenceError", "RegExp", "String", "SyntaxError", "undefined", 
			"UriError", "InternalError", "XML", "Namespace", "QName", 
			"Iterator", "StopIteration", "SMScript", "SMFunction" 
		};

		!SMRuntime();
	};

}