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
#include "Interop.h"
#include "TypeInfo.h"
#include "Attributes.h"
#include "SMRuntime.h"
#include "SMHashtable.h"
#include "SMContext.h"
#include "SMScript.h"
#include "SMPrototype.h"
#include "SMInstance.h"
#include "SMErrorReport.h"
#include "SMEvent.h"

using namespace smnetjs;


SMRuntime::SMRuntime()
	: autoEmbed(true)
{
	Initialize();
}

SMRuntime::SMRuntime(RuntimeOptions opts)
	: autoEmbed(true)
{
	SetOptions(opts);
	Initialize();
}

SMRuntime::!SMRuntime()
{
	for each(DictionaryEntry^ entry in embedded) {
		delete (SMPrototype^)entry->Value;
	}

	for(int i = scripts->Count - 1; i >= 0; i--) {
		delete scripts[i];
	}

	JS_DestroyRuntime(rt);
	rt = NULL;
}


void SMRuntime::SetOptions(RuntimeOptions opts)
{
	if ((opts & RuntimeOptions::DisableAutoEmbedding) == RuntimeOptions::DisableAutoEmbedding)
		autoEmbed = false;

	if ((opts & RuntimeOptions::ForceGCOnContextDestroy) == RuntimeOptions::ForceGCOnContextDestroy)
		destroyGC = true;

	if ((opts & RuntimeOptions::EnableCompartmentalGC) == RuntimeOptions::EnableCompartmentalGC) {
		compartmentGC = true;
		JS_SetGCParameter(rt, JSGC_MODE, JSGC_MODE_COMPARTMENT);
	}
}

void SMRuntime::Initialize()
{
	rt = JS_NewRuntime(8L * 1024L * 1024L);

	scripts = gcnew List<SMScript^>();
	embedded = gcnew SMHashtable<Type^, SMPrototype^>();

	pscripts = gcnew ReadOnlyCollection<SMScript^>(scripts);

	errorHandler = gcnew SMErrorReporter(this, &SMRuntime::ScriptError);

	Embed(SMEvent::typeid);
	Embed(SMEventHandler::typeid);
}


bool SMRuntime::Embed(Type^ type)
{
	SMPrototype^ proto;
	return Embed(type, proto);
}

bool SMRuntime::Embed(Type^ type, SMEmbedded^ attribute)
{
	SMPrototype^ proto;
	return Embed(type, attribute, proto);
}


bool SMRuntime::Embed(Type^ type, [Out] SMPrototype^ %proto)
{
	if (type == nullptr)
		return false;

	if (type->IsInterface || IsHandledType(type))
		return true;

	proto = FindProto(type);

	if (proto != nullptr) 
		return true;

	//no support for static generics
	//we also only want to embed generic type definitions (most basic type)
	//generic types derived from type definitions are handled
	if(type->IsGenericType)
		if (TypeInfo::IsStatic(type))
			return false;

		else if (!type->IsGenericTypeDefinition) {

			Type^ t = type->GetGenericTypeDefinition();
			return Embed(type->GetGenericTypeDefinition());
		}

	if ((type->IsArray || type->IsByRef || type->IsPointer)) {
		if (type->HasElementType)
			return Embed(type->GetElementType());

		else return false;
	}

	proto = gcnew SMPrototype(this, type);
	embedded->Add(type, proto);

	proto->Reflect();

	for (int i = 0; i < scripts->Count; i++) {
		SMScript^ script = scripts[i];
		script->Initialize(proto);
	}

	return true;
}

bool SMRuntime::Embed(Type^ type, SMEmbedded^ attribute, [Out] SMPrototype^ %proto)
{
	if (type == nullptr)
		return false;

	if (type->IsInterface || IsHandledType(type))
		return true;

	proto = FindProto(type);

	if (proto != nullptr) {
		proto->attribute = attribute;
		return true;
	}

	//no support for static generics
	//we also only want to embed generic type definitions (most basic type)
	//generic types derived from type definitions are handled
	if(type->IsGenericType)
		if (TypeInfo::IsStatic(type))
			return false;

		else if (!type->IsGenericTypeDefinition) {

			Type^ t = type->GetGenericTypeDefinition();
			return Embed(type->GetGenericTypeDefinition());
		}

	if ((type->IsArray || type->IsByRef || type->IsPointer)) {
		if (type->HasElementType)
			return Embed(type->GetElementType());

		else return false;
	}

	proto = gcnew SMPrototype(this, type, attribute);
	embedded->Add(type, proto);

	proto->Reflect();

	for (int i = 0; i < scripts->Count; i++) {
		SMScript^ script = scripts[i];
		script->Initialize(proto);
	}

	return true;
}


bool SMRuntime::IsHandledType(Type^ type)
{
	if (IsHandledType(type->Name))
		return true;

    if (!type->IsEnum) {
        switch (Type::GetTypeCode(type)) {
			case TypeCode::DBNull:
			case TypeCode::DateTime:
			case TypeCode::Object:
				return false;
        }
        return true;
    }
    return false;
}

bool SMRuntime::IsHandledType(String^ name)
{
	for(int i = 0; i < handledTypes->Length; i++) {
		if (handledTypes[i] == name)
			return true;
	}
	return false;
}

Type^ SMRuntime::ToHandledType(String^ name)
{
	if (name == "Array")
		return Array::typeid;

	if (name == "Boolean")
		return Boolean::typeid;

	if (name == "Date")
		return DateTime::typeid;

    if (name == "Math")
		return Math::typeid;

	if (name == "Number")
		return Double::typeid;

	if (name == "Object")
		return Object::typeid;

	if (name == "String")
		return String::typeid;

    return nullptr;
}


bool SMRuntime::IsValidGlobal(Type^ type)
{
	if (type->IsArray || 
		type->IsByRef || 
		type->IsPointer || 
		type->IsInterface || 
		type->IsGenericType ||
		type->IsGenericParameter ||
		IsHandledType(type)) return false;

	return true;
}


SMScript^ SMRuntime::InitScript(String^ name)
{
	SMScript^ script = gcnew SMScript(name, this);
	script->SetErrorReporter(errorHandler);

	for each(DictionaryEntry^ entry in embedded)
		script->Initialize((SMPrototype^)entry->Value);
	
	scripts->Add(script);
	return script;
}

SMScript^ SMRuntime::InitScript(String^ name, Type^ global)
{
	if (!IsValidGlobal(global)) {
		throw gcnew ArgumentException(
			"global", "Type cannot be used as a global object");
	}

	SMPrototype^ gproto;
	if (Embed(global, gproto)) {

		SMScript^ script = gcnew SMScript(name, this, gproto);
		script->SetErrorReporter(errorHandler);

		for each(DictionaryEntry^ entry in embedded) {

			SMPrototype^ proto = (SMPrototype^)entry->Value;
			if (proto != gproto) script->Initialize(proto);
		}
	
		scripts->Add(script);
		return script;
	}

	return nullptr;
}


SMScript^ SMRuntime::FindScript(String^ name)
{
	for (int i = 0; i < scripts->Count; i++) {
		SMScript^ script = scripts[i];

		if (script->Name == name)
			return script;
	}

	return nullptr;
}

SMScript^ SMRuntime::FindScript(JSContext* cx)
{
	for (int i = 0; i < scripts->Count; i++) {
		SMScript^ script = scripts[i];

		if (script->Context == cx)
			return script;
	}

	return nullptr;
}


IntPtr SMRuntime::GetPrototypePointer(SMScript^ script, Type^ type)
{
	if (script == nullptr || type == nullptr)
		return IntPtr::Zero;

	SMPrototype^ proto = FindProto(type);

	if (proto == nullptr) 
		return IntPtr::Zero;

	JSObject* jsproto = (JSObject*)proto->jsprototypes[(long)script->Context];

	if (!jsproto) return IntPtr::Zero;

	return IntPtr(jsproto);
}

IntPtr SMRuntime::GetInstancePointer(SMScript^ script, Object^ obj)
{
	if (script == nullptr || obj == nullptr)
		return IntPtr::Zero;

	SMPrototype^ proto = FindProto(obj->GetType());

	if (proto == nullptr) 
		return IntPtr::Zero;

	SMInstance^ instance = proto->FindInstance(script, obj);

	if (instance == nullptr)
		return IntPtr::Zero;

	return IntPtr(instance->jsobj);
}


System::Type^ SMRuntime::FindType(String^ name)
{
	Type^ t = ToHandledType(name);
	if (t != nullptr) return t;

	for each(DictionaryEntry^ entry in embedded) {

		SMPrototype^ proto = (SMPrototype^)entry->Value;
		if (proto->Name == name) return proto->Type;
	}

	return nullptr;
}


SMPrototype^ SMRuntime::FindProto(Type^ type)
{
	if (type->IsGenericType && !type->IsGenericTypeDefinition)
		type = type->GetGenericTypeDefinition();

	return embedded[type];
}

SMPrototype^ SMRuntime::FindProto(SMScript^ script, JSObject* obj)
{
	long jsobj = (long)obj;

	for each(DictionaryEntry^ entry in embedded) {
		SMPrototype^ proto = (SMPrototype^)entry->Value;

		if (proto->jsprototypes[(long)script->Context] == jsobj)
			return proto;

		SMInstance^ instance = proto->FindInstance(obj);

		if (instance != nullptr) 
			return proto;
	}

	return nullptr;
}


SMInstance^ SMRuntime::FindInstance(JSObject* obj)
{
	long jsobj = (long)obj;

	for each(DictionaryEntry^ entry in embedded) {
		SMPrototype^ proto = (SMPrototype^)entry->Value;
		SMInstance^  instance = proto->FindInstance(obj);

		if (instance != nullptr) return instance;
	}

	return nullptr;
}


void SMRuntime::DestroyScript(String^ name)
{
	DestroyScript(FindScript(name));
}

void SMRuntime::DestroyScript(SMScript^ script)
{
	delete script;
}


void SMRuntime::GarbageCollect()
{
	if (scripts->Count > 0) {
		if (compartmentGC)
			JS_SetGCParameter(rt, JSGC_MODE, JSGC_MODE_GLOBAL);

		/*
		 Modilla MDC: 
		 JS_GC performs garbage collection, if necessary, of JS objects, numbers, and strings 
		 that are no longer reachable in the runtime of cx.

		 - when compartmental GC is turned off, this should only need to be called once for all live scripts
		*/

		SMScript^ script = scripts[0];
		script->GarbageCollect();

		if (compartmentGC)
			JS_SetGCParameter(rt, JSGC_MODE, JSGC_MODE_COMPARTMENT);
	}
}


void SMRuntime::ScriptError(JSContext* cx, const char* message, JSErrorReport* report)
{
	SMScript^ script = FindScript(cx);

	if (script != nullptr) {
		if (script->errorDepth < 2) {

			script->errorDepth++;
			OnScriptError(script, gcnew SMErrorReport(message, report));
			script->errorDepth--;
		}
	}
}