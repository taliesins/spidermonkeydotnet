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
#include "stdafx.h"
#include "TypeInfo.h"

using namespace smnetjs;


bool TypeInfo::IsStatic(Type^ type)
{
	return (type->IsAbstract && type->IsSealed);
}

bool TypeInfo::IsStatic(EventInfo^ _event)
{
	MethodInfo^ add = _event->GetAddMethod(true);
	return add->IsStatic;
}

bool TypeInfo::IsStatic(PropertyInfo^ prop)
{
	MethodInfo^ accessor;

	if (prop->CanRead)
		accessor = prop->GetGetMethod(true);
	else
		accessor = prop->GetSetMethod(true);

	return (accessor->IsStatic);
}


bool TypeInfo::IsDelegate(Type^ type)
{
	return (Delegate::typeid)->IsAssignableFrom(type);
}

bool TypeInfo::IsDisposable(Type^ type)
{
	return (IDisposable::typeid)->IsAssignableFrom(type);
}


bool TypeInfo::IsParamArray(ParameterInfo^ param)
{
	array<ParamArrayAttribute^>^ attrs = (array<ParamArrayAttribute^>^)
		param->GetCustomAttributes(ParamArrayAttribute::typeid, false);

	return (attrs->Length > 0);
}


generic<typename T> T TypeInfo::GetAttribute(Type^ type) 
{
	array<T>^ attrs = (array<T>^)type->GetCustomAttributes(T::typeid, false);

	if (attrs->Length > 0)
		return (T)attrs[0];

	return T();
}

generic<typename T> T TypeInfo::GetAttribute(MemberInfo^ type) 
{
	array<T>^ attrs = (array<T>^)type->GetCustomAttributes(T::typeid, false);

	if (attrs->Length > 0)
		return (T)attrs[0];

	return T();
}


generic<typename T> bool TypeInfo::InheritsFrom(Type^ type)
{
	return (T::typeid)->IsAssignableFrom(type);
}