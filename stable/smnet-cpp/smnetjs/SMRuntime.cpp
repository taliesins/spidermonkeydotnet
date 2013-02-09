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
#include "Interop.h"
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
	for (int i = 0; i < embedded->Count; i++) {
		SMPrototype^ proto = embedded[i];
		delete proto;
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
	embedded = gcnew List<SMPrototype^>();

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

bool SMRuntime::Embed(Type^ type, [Out] SMPrototype^ %proto)
{
	if (type->IsInterface || IsHandledType(type))
		return true;

	proto = FindProto(type);

	if (proto != nullptr) 
		return true;

	if(type->IsGenericType || type->IsGenericParameter)
		return false;

	if ((type->IsArray || type->IsByRef || type->IsPointer)) {
		if (type->HasElementType)
			return Embed(type->GetElementType());

		else return false;
	}

	proto = gcnew SMPrototype(this, type);

	embedded->Add(proto);
	proto->Reflect();

	for (int i = 0; i < scripts->Count; i++) {
		SMScript^ script = scripts[i];
		script->Initialize(proto);
	}

	return true;
}


bool SMRuntime::IsHandledType(Type^ type)
{
	if (type->Name == "Void" ||
        type->Name == "String" ||
        type->Name == "Array" ||
        type->Name == "Object" ||
        type->Name == "IntPtr" ||
        type->Name == "SMScript" ||
		type->Name == "SMFunction") 
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

	for (int i = 0; i < embedded->Count; i++)
		script->Initialize(embedded[i]);
	
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

		for (int i = 0; i < embedded->Count; i++) {
			SMPrototype^ proto = embedded[i];

			if (proto != gproto)
				script->Initialize(proto);
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

	SMInstance^ instance = nullptr;

	for each(DictionaryEntry^ entry in proto->jsinstances) {
		SMInstance^ smobj = (SMInstance^)entry->Value;

		if (smobj->script == script && smobj->instance == obj) {
			instance = smobj; break;
		}	
	}

	if (instance == nullptr)
		return IntPtr::Zero;

	return IntPtr(instance->jsobj);
}


SMPrototype^ SMRuntime::FindProto(Type^ type)
{
	for(int i = 0; i < embedded->Count; i++) {
		if (embedded[i]->Type == type)
			return embedded[i];
	}

	return nullptr;
}

SMPrototype^ SMRuntime::FindProto(SMScript^ script, JSObject* obj)
{
	long jsobj = (long)obj;

	for(int i = 0; i < embedded->Count; i++) {
		SMPrototype^ proto = embedded[i];

		if (proto->jsprototypes[(long)script->Context] == jsobj)
			return proto;

		SMInstance^ instance = proto->jsinstances[jsobj];

		if (instance != nullptr) 
			return proto;
	}

	return nullptr;
}


SMInstance^ SMRuntime::FindInstance(JSObject* obj)
{
	long jsobj = (long)obj;

	for(int i = 0; i < embedded->Count; i++) {
		SMPrototype^ proto = embedded[i];
		SMInstance^  instance = proto->jsinstances[jsobj];

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