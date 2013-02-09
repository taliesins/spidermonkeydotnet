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
using namespace System::Reflection;

namespace smnetjs {

	ref class TypeInfo abstract sealed
	{
	public:
		static bool IsStatic(Type^ type);
		static bool IsStatic(EventInfo^ _event);
		static bool IsStatic(PropertyInfo^ prop);

		static bool IsDelegate(Type^ type);
		static bool IsDisposable(Type^ type);

		static bool IsParamArray(ParameterInfo^ param);
		
		generic<typename T> static T GetAttribute(Type^ type);
		generic<typename T> static T GetAttribute(MemberInfo^ type);

		generic<typename T> static bool InheritsFrom(Type^ type);
	};

}