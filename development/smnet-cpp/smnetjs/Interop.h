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
using namespace System::Text;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	ref class SMPrototype;
	ref class SMScript;

	public ref class Interop abstract sealed
	{
	internal:
		static void* DelegatePointer(Delegate^ del);

		static char* ToUnmanagedStringInternal(String^ str);
		static String^ ToManagedStringInternal(const char* str);

		static String^ FromJSValToManagedString(JSContext* cx, jsval value);
		static char*   FromJSValToUnmanagedString(JSContext* cx, jsval value);

		static void    ToJSVal(SMScript^ script, Object^ value, jsval* rval);
	public:

		static IntPtr ToUnmanagedString(String^ str);
		static String^ ToManagedString(IntPtr str);

		static void    ToJSVal(SMScript^ script, Object^ value, [Out] IntPtr rval);
		static Object^ FromJSVal(SMScript^ script, jsval value);

		static bool    ChangeType(SMScript^ script, Object^ %value, Type^ typeTo);
	};

}