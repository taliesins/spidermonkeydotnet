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

using namespace System;
using namespace System::Reflection;
using namespace System::Reflection::Emit;
using namespace System::Collections::Generic;
using namespace System::Runtime::CompilerServices;

namespace smnetjs {

	ref class SMScript;
	ref class SMFunction;
	ref class SMPrototype;
	ref class SMInstance;

	ref class SMMethod;
	ref class SMMethodInfo;
	ref class SMConstructor;
	ref class SMConstructorInfo;
	
	ref class SMBinder abstract sealed
	{
	internal:
		static Delegate^        BindToDelegate(SMScript^ script, Type^ delegateType, SMFunction^ func);
		static ConstructorInfo^ BindToConstructor(SMScript^ script, List<SMConstructorInfo^>^ ctors, array<Object^>^ %args);
		static MethodInfo^      BindToMethod(SMScript^ script, List<SMMethodInfo^>^ funcs, array<Object^>^ %args);
	public:
		static Delegate^        BindToDelegate(SMScript^ script, Type^ delegateType, ... array<Object^>^ args);
		static ConstructorInfo^ BindToConstructor(SMScript^ script, SMPrototype^ proto, array<Object^>^ %args);
		static MethodInfo^      BindToMethod(SMScript^ script, SMPrototype^ proto, SMInstance^ instance, String^ name, array<Object^>^ %args);

		static array<Object^>^ GetGenericArguments(SMScript^ script, array<Type^>^ %gargs, array<Object^>^ args);

		static Object^ CallCLRFunction(SMScript^ script, IntPtr fobj, String^ name, array<Object^>^ args);
		static Object^ CallLambdaFunction(SMScript^ script, jsval funcval, array<Object^>^ args);

		static array<Object^>^ ConvertArguments(SMScript^ script, ConstructorInfo^ constructor, SMConstructor^ attribute, array<Object^>^ args, bool _explicit);
		static array<Object^>^ ConvertArguments(SMScript^ script, MethodInfo^ method, SMMethod^ attribute, array<Object^>^ args, bool _explicit);
	};

}