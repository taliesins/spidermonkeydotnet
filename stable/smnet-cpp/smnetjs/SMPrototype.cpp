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
#include "TypeInfo.h"
#include "SMPrototype.h"
#include "SMHashtable.h"
#include "SMMemberInfo.h"
#include "SMRuntime.h"
#include "SMScript.h"
#include "SMInstance.h"
#include "SMEvent.h"
#include "SMBinder.h"

using namespace smnetjs;


SMPrototype::SMPrototype(SMRuntime^ runtime, System::Type^ type)
{
	this->type = type;
	this->runtime = runtime;

	jsprototypes = gcnew SMHashtable<long, long>();
	jsinstances = gcnew SMHashtable<long, SMInstance^>();
	jsevents = gcnew SMHashtable<long, SMEvent^>();

	constructors = gcnew List<SMConstructorInfo^>();

	properties = gcnew List<SMMemberInfo^>();
	sproperties = gcnew List<SMMemberInfo^>();

	functions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();
	sfunctions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();

	ctor = gcnew SMNative(this, &SMPrototype::Constructor);
	method = gcnew SMNative(this, &SMPrototype::Method);

	getter = gcnew SMPropertyOp(this, &SMPrototype::Getter);
	setter = gcnew SMStrictPropertyOp(this, &SMPrototype::Setter);

	finalize = gcnew SMFinalizeOp(this, &SMPrototype::Finalizer);
}

SMPrototype::!SMPrototype()
{
	FreeFunctionSpec(funcs);
	FreeFunctionSpec(sfuncs);
	
	FreePropertySpec(props);
	FreePropertySpec(sprops);

	char* t = (char*)clasp->name;
	Marshal::FreeHGlobal(IntPtr(t));

	delete clasp;
	clasp = NULL;

	ctor = nullptr;
	method = nullptr;

	getter = nullptr;
	setter = nullptr;

	finalize = nullptr;

	jsprototypes->Clear();
	constructors->Clear();
	properties->Clear();
	sproperties->Clear();
	functions->Clear();
	sfunctions->Clear();
}


void SMPrototype::Reflect()
{
	CreateJSClass();

	ReflectConstructors();
	ReflectFunctions();
	ReflectProperties();
}


void SMPrototype::CreateJSClass()
{
	attribute = TypeInfo::GetAttribute<SMEmbedded^>(type);

	if (attribute == nullptr)
		attribute = gcnew SMEmbedded();

	if (runtime->autoEmbed)
		for each(System::Type^ t in type->GetNestedTypes())
			runtime->Embed(t);

	if (TypeInfo::IsStatic(type)) {
		JSClass jsclass = {
			Interop::ToUnmanagedString(Name),
			JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
			JS_PropertyStub,
			JS_PropertyStub,
			JS_PropertyStub,
			JS_StrictPropertyStub,
			JS_EnumerateStub,
			JS_ResolveStub,
			JS_ConvertStub,
			JS_FinalizeStub,
			JSCLASS_NO_OPTIONAL_MEMBERS
		};

		clasp = new JSClass(jsclass);
	}
	else {
		JSClass jsclass = {
			Interop::ToUnmanagedString(Name),
			JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
			JS_PropertyStub,
			JS_PropertyStub,
			JS_PropertyStub,
			JS_StrictPropertyStub,
			JS_EnumerateStub,
			JS_ResolveStub,
			JS_ConvertStub,
			(JSFinalizeOp)Interop::DelegatePointer(finalize),
			JSCLASS_NO_OPTIONAL_MEMBERS
		};

		clasp = new JSClass(jsclass);
	}
}


JSObject* SMPrototype::CreateInstance(SMScript^ script, Object^ object)
{
	return CreateInstance(script, object, false);
}

JSObject* SMPrototype::CreateInstance(SMScript^ script, Object^ object, bool lock)
{
	SMInstance^ instance = nullptr;

	for each(DictionaryEntry^ entry in jsinstances) {
		SMInstance^ smobj = (SMInstance^)entry->Value;

		if (smobj->script == script && object == smobj->instance) {
			instance = smobj; break;
		}	
	}

	if (instance == nullptr) {

		JSObject* obj = JS_NewObject(script->Context, clasp, (JSObject*)jsprototypes[(long)script->Context], NULL);

		if (!obj) return NULL;

		instance = gcnew SMInstance(script, obj, object);
		jsinstances->Add((long)obj, instance);
	}

	if (lock) instance->GCLock();

	return instance->jsobj;
}

JSObject* SMPrototype::CreateInstance(SMScript^ script, SMPrototype^ proto, SMEvent^ object)
{
	SMInstance^ instance = nullptr;

	for each(DictionaryEntry^ entry in jsinstances) {
		SMInstance^ smobj = (SMInstance^)entry->Value;

		if (smobj->script == script && object == smobj->instance) {
			instance = smobj; break;
		}	
	}

	if (instance == nullptr) {

		JSObject* obj = JS_NewObject(script->Context, clasp, (JSObject*)jsprototypes[(long)script->Context], NULL);

		if (!obj) return NULL;

		instance = gcnew SMInstance(script, obj, object);
		jsinstances->Add((long)obj, instance);

		proto->jsevents->Add((long)obj, (SMEvent^)object);
	}

	return instance->jsobj;
}


void SMPrototype::ReleaseInstance(SMScript^ script, Object^ object)
{
	SMInstance^ instance = nullptr;

	for each(DictionaryEntry^ entry in jsinstances) {
		SMInstance^ smobj = (SMInstance^)entry->Value;

		if (smobj->script == script && object == smobj->instance) {
			instance = smobj; break;
		}	
	}

	if (instance != nullptr)
		instance->GCUnlock(false);
}


bool SMPrototype::IsValidMethod(MethodInfo^ method) 
{
    String^ name = method->Name;

    if (name->IndexOf("op_") == 0 ||
        name->IndexOf("get_") == 0 ||
        name->IndexOf("set_") == 0 ||
        name->IndexOf("add_") == 0 ||
        name->IndexOf("remove_") == 0) return false;

    return true;
}


void SMPrototype::ReflectConstructors()
{
	BindingFlags bf = BindingFlags::Public |
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	array<ConstructorInfo^>^ ctors = type->GetConstructors();

	for (int i = 0; i < ctors->Length; i++) {
		ConstructorInfo^ ctor = ctors[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)ctor);
		if (ig != nullptr) continue;
		
		if (runtime->autoEmbed) {
			bool ignore = false;
			array<ParameterInfo^>^ params = ctor->GetParameters();

			for (int i = 0; i < params->Length; i++) {
				ParameterInfo^ p = params[i];
				if (!runtime->Embed(p->ParameterType)) {
					ignore = true; break;
				}
			}

			if (ignore) continue;
		}

		SMConstructor^ mg = TypeInfo::GetAttribute<SMConstructor^>((MemberInfo^)ctor);

		if (mg == nullptr)
			mg = gcnew SMConstructor();

		constructors->Add(gcnew SMConstructorInfo(mg, ctor));
	}
}

void SMPrototype::ReflectFunctions()
{
	BindingFlags bf = BindingFlags::Public |
					  BindingFlags::Static | 
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

#pragma region " Reflect Methods "

	array<MethodInfo^>^ methods = type->GetMethods(bf);

	for(int i = 0; i < methods->Length; i++) {
		MethodInfo^ method = methods[i];

		if (!IsValidMethod(method))
			continue;

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>(method);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed) {
			bool ignore = false;
			array<ParameterInfo^>^ params = method->GetParameters();

			for (int i = 0; i < params->Length; i++) {
				ParameterInfo^ p = params[i];
				if (!runtime->Embed(p->ParameterType)) {
					ignore = true; break;
				}
			}

			if (ignore) continue;

			if (!runtime->Embed(method->ReturnType))
				continue;
		}

		String^ name = method->Name;
		SMMethod^ mg = TypeInfo::GetAttribute<SMMethod^>(method);

		if (mg != nullptr) {
			if (!String::IsNullOrWhiteSpace(mg->Name))
				name = mg->Name;
		}
		else mg = gcnew SMMethod();

		List<SMMethodInfo^>^ m;

		if (!method->IsStatic)
			m = functions[name];
		else 
			m = sfunctions[name];

		if (m == nullptr)
			m = gcnew List<SMMethodInfo^>();

		m->Add(gcnew SMMethodInfo(mg, method));

		if (!method->IsStatic)
			functions[name] = m;
		else 
			sfunctions[name] = m;
	}
#pragma endregion

#pragma region " Build FunctionSpec "

	int index = -1;
	this->funcs = new JSFunctionSpec[functions->Count + 1];

	JSFunctionSpec nullspec = { NULL, NULL, 0, 0 };

	for each(DictionaryEntry^ entry in functions) {
		String^ name = (String^)entry->Key;
		List<SMMethodInfo^>^ methods = (List<SMMethodInfo^>^)entry->Value;

		uint32 pflags = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

		for (int i = 0; i < methods->Count; i++) {
			SMMethodInfo^ method = methods[i];

			if (!method->Attribute->Enumerate && (pflags & JSPROP_ENUMERATE) == JSPROP_ENUMERATE)
				pflags = pflags ^ JSPROP_ENUMERATE;
				
			if (method->Attribute->Overridable && (pflags & JSPROP_READONLY) == JSPROP_READONLY)
				pflags = pflags ^ JSPROP_READONLY;
		}

		JSFunctionSpec spec = {
			Interop::ToUnmanagedString(name),
			(JSNative)Interop::DelegatePointer(method),
			0/*nargs - not used*/,
			pflags
		};

		this->funcs[++index] = spec;
	}
	this->funcs[++index] = nullspec;

	index = -1;
	this->sfuncs = new JSFunctionSpec[sfunctions->Count + 1];

	for each(DictionaryEntry^ entry in sfunctions) {
		String^ name = (String^)entry->Key;
		List<SMMethodInfo^>^ methods = (List<SMMethodInfo^>^)entry->Value;

		uint32 pflags = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

		for (int i = 0; i < methods->Count; i++) {
			SMMethodInfo^ method = methods[i];

			if (!method->Attribute->Enumerate && (pflags & JSPROP_ENUMERATE) == JSPROP_ENUMERATE)
				pflags ^= JSPROP_ENUMERATE;
				
			if (method->Attribute->Overridable && (pflags & JSPROP_READONLY) == JSPROP_READONLY)
				pflags ^= JSPROP_READONLY;
		}

		JSFunctionSpec spec = {
			Interop::ToUnmanagedString(name),
			(JSNative)Interop::DelegatePointer(method),
			0/*nargs - not used*/,
			pflags
		};

		this->sfuncs[++index] = spec;
	}
	this->sfuncs[++index] = nullspec;
#pragma endregion
}

void SMPrototype::ReflectProperties()
{
	BindingFlags bf = BindingFlags::Public | 
					  BindingFlags::Static | 
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

#pragma region " Reflect Properties "

	array<FieldInfo^>^ fields = type->GetFields(bf);

	for (int i = 0; i < fields->Length; i++) {
		FieldInfo^ field = fields[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)field);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed && !runtime->Embed(field->FieldType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)field);

		if (mg == nullptr)
			mg = gcnew SMProperty();

		if (!field->IsStatic) 
			properties->Add(gcnew SMMemberInfo(mg, field));
		else
			sproperties->Add(gcnew SMMemberInfo(mg, field));
	}

	array<PropertyInfo^>^ props = type->GetProperties(bf);

	for (int i = 0; i < props->Length; i++) {
		PropertyInfo^ prop = props[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)prop);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed && !runtime->Embed(prop->PropertyType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)prop);

		if (mg == nullptr)
			mg = gcnew SMProperty();
		
		if (!TypeInfo::IsStatic(prop))
			properties->Add(gcnew SMMemberInfo(mg, prop));
		else
			sproperties->Add(gcnew SMMemberInfo(mg, prop));
	}

	array<EventInfo^>^ events = type->GetEvents(bf);

	for (int i = 0; i < events->Length; i++) {
		EventInfo^ _event = events[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)_event);
		if (ig != nullptr) continue;

		if (!runtime->Embed(_event->EventHandlerType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)_event);

		if (mg == nullptr)
			mg = gcnew SMProperty();

		if (!TypeInfo::IsStatic(_event))
			properties->Add(gcnew SMMemberInfo(mg, _event));
		else
			sproperties->Add(gcnew SMMemberInfo(mg, _event));
	}
#pragma endregion

#pragma region " Build PropertySpec "

	int index = -1;
	int8 tinyid = -1;

	JSPropertySpec nullspec = { NULL, 0, 0, NULL, NULL };
	this->props = new JSPropertySpec[properties->Count + 1];

	for (int i = 0; i < properties->Count; i++) {
		SMMemberInfo^ member = properties[i];

		String^ name = member->Member->Name;
		uint8 pflags = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;

		if (!member->Attribute->Enumerate)
			pflags ^= JSPROP_ENUMERATE;

		if (!String::IsNullOrWhiteSpace(member->Attribute->Name))
			name = member->Attribute->Name;

		JSPropertySpec spec = {
			Interop::ToUnmanagedString(name),
			++tinyid,
			pflags,
			(JSPropertyOp)Interop::DelegatePointer(getter),
			(JSStrictPropertyOp)Interop::DelegatePointer(setter)
		};

		this->props[++index] = spec;
	}
	this->props[++index] = nullspec;

	index = -1;
	this->sprops = new JSPropertySpec[sproperties->Count + 1];

	for (int i = 0; i < sproperties->Count; i++) {
		SMMemberInfo^ member = sproperties[i];

		String^ name = member->Member->Name;
		uint8 pflags = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;
		
		if (!member->Attribute->Enumerate)
			pflags ^= JSPROP_ENUMERATE;

		if (!String::IsNullOrWhiteSpace(member->Attribute->Name))
			name = member->Attribute->Name;

		JSPropertySpec spec = {
			Interop::ToUnmanagedString(name),
			++tinyid,
			pflags,
			(JSPropertyOp)Interop::DelegatePointer(getter),
			(JSStrictPropertyOp)Interop::DelegatePointer(setter)
		};

		this->sprops[++index] = spec;
	}
	this->sprops[++index] = nullspec;
#pragma endregion
}


void SMPrototype::FreeFunctionSpec(JSFunctionSpec* specs)
{
	int i = 0;
	JSFunctionSpec spec = specs[0];

	while(spec.name != NULL) {
		char* t = (char*)spec.name;
		Marshal::FreeHGlobal(IntPtr(t));

		spec = specs[++i];
	}

	delete specs;
}

void SMPrototype::FreePropertySpec(JSPropertySpec* specs)
{
	int i = 0;
	JSPropertySpec spec = specs[0];

	while(spec.name != NULL) {
		char* t = (char*)spec.name;
		Marshal::FreeHGlobal(IntPtr(t));

		spec = specs[++i];
	}

	delete specs;
}


JSBool SMPrototype::Constructor(JSContext* cx, uintN argc, jsval* vp)
{
	Object^ ret = nullptr;
	jsval* argv = JS_ARGV(cx, vp);

	SMScript^ script = runtime->FindScript(cx);

	if (TypeInfo::IsDelegate(type)) {
		if (argc != 1)
			return JS_FALSE;

		ret = Interop::FromJSVal(script, argv[0]);

		if (!Interop::ChangeType(script, ret, SMFunction::typeid))
			return JS_FALSE;

		ret = SMBinder::BindToDelegate(this, script, (SMFunction^)ret);
	}
	else {
		array<Object^>^ args = gcnew array<Object^>(argc);

		for(int i = 0; i < (int)argc; i++) {
			args[i] = Interop::FromJSVal(script, argv[i]);
		}

		try {
			JSAutoSuspendRequest ar(cx);
			SMConstructorInfo^ ctor = SMBinder::BindToConstructor(script, constructors, args);

			if (ctor == nullptr) 
				return JS_FALSE;

			ret = ctor->Method->Invoke(args);
		}
		catch(Exception^ e) {
			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, "ctor", 
				e->InnerException->Message);

			return JS_FALSE;
		}
	}

	if (ret == nullptr) return JS_FALSE;

	JSObject* obj = CreateInstance(script, ret);

	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));
	return JS_TRUE;
}


JSBool SMPrototype::Getter(JSContext* cx, JSObject* obj, jsid id, jsval* vp)
{
	jsval idval;
	JS_IdToValue(cx, id, &idval);

	if (JSVAL_IS_INT(idval)) {
		int index = JSVAL_TO_INT(idval);

		int pcount = properties->Count;
		int scount = sproperties->Count;

		SMMemberInfo^ member;

		if (index < pcount)
			member = properties[index];
		else {
			index -= pcount;
			if (index < scount)
				member = sproperties[index];

			else return JS_FALSE;
		}

		Object^ ret;
		SMScript^ script = runtime->FindScript(cx);

		System::Type^ memberType = member->Member->GetType();

		try {
			if (TypeInfo::InheritsFrom<FieldInfo^>(memberType)) {
				JSAutoSuspendRequest ar(script->Context);
				FieldInfo^ finfo = (FieldInfo^)member->Member;

				if (!finfo->IsStatic) {
					SMInstance^ clrobj = jsinstances[(long)obj];

					if (clrobj != nullptr)
						ret = finfo->GetValue(clrobj->instance);

					else return JS_FALSE;
				}
				else ret = finfo->GetValue(nullptr);
			}
			else if (TypeInfo::InheritsFrom<PropertyInfo^>(memberType)) {
				JSAutoSuspendRequest ar(script->Context);
				PropertyInfo^ pinfo = (PropertyInfo^)member->Member;

				if (pinfo->CanRead) {
					if (!TypeInfo::IsStatic(pinfo)) {
						SMInstance^ clrobj = jsinstances[(long)obj];

						if (clrobj != nullptr)
							ret = pinfo->GetValue(clrobj->instance, nullptr);

						else return JS_FALSE;
					}
					else ret = pinfo->GetValue(nullptr, nullptr);
				}
				else return JS_FALSE;
			}
			else if (TypeInfo::InheritsFrom<EventInfo^>(memberType)) {
				//don't suspend request here, we use the api
				EventInfo^ einfo = (EventInfo^)member->Member;

				for each(DictionaryEntry^ entry in jsevents) {
					SMEvent^ _event = (SMEvent^)entry->Value;

					if (_event->Event == einfo) {
						*vp = OBJECT_TO_JSVAL((JSObject*)(long)entry->Key);
						return JS_TRUE;
					}
				}

				SMPrototype^ proto = runtime->FindProto(SMEvent::typeid);
				
				SMEvent^ _event = nullptr;
				Object^ instance = nullptr;

				if (!TypeInfo::IsStatic(type)){
					SMInstance^ clrobj = jsinstances[(long)obj];
					instance = clrobj->instance;
				}

				_event = gcnew SMEvent(instance, einfo);

				jsval propid;
				JS_LookupPropertyById(cx, obj, id, &propid);

				char* pname = Interop::FromJSValToUnmanagedString(cx, propid);
				*vp = OBJECT_TO_JSVAL(proto->CreateInstance(script, this, _event));
				
				JS_SetProperty(script->Context, obj, pname, vp);
				JS_free(script->Context, pname);

				return JS_TRUE;
			}
		} catch (Exception^ e) {
			jsval propid;
			JS_LookupPropertyById(cx, obj, id, &propid);

			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, 
				Interop::FromJSValToManagedString(cx, propid),
				e->InnerException->Message);

			return JS_FALSE;
		}

		Interop::ToJSVal(script, ret, vp);
	}

	return JS_TRUE;
}

JSBool SMPrototype::Setter(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp)
{
	jsval idval;
	JS_IdToValue(cx, id, &idval);

	if (JSVAL_IS_INT(idval)) {
		int index = JSVAL_TO_INT(idval);

		int pcount = properties->Count;
		int scount = sproperties->Count;

		SMMemberInfo^ member;

		if (index < pcount)
			member = properties[index];
		else {
			index -= pcount;
			if (index < scount)
				member = sproperties[index];

			else return JS_FALSE;
		}
	
		SMScript^ script = runtime->FindScript(cx);
		Object^ ret = Interop::FromJSVal(script, *vp);

		System::Type^ memberType = member->Member->GetType();

		try {
			JSAutoSuspendRequest ar(script->Context);
			if (TypeInfo::InheritsFrom<FieldInfo^>(memberType)) {
				FieldInfo^ finfo = (FieldInfo^)member->Member;

				if (!finfo->IsInitOnly && Interop::ChangeType(script, ret, finfo->FieldType)) {
					if (!finfo->IsStatic) {
						SMInstance^ clrobj = jsinstances[(long)obj];

						if (clrobj != nullptr)
							finfo->SetValue(clrobj->instance, ret);

						else return JS_FALSE;
					}
					else finfo->SetValue(nullptr, ret);
				}
			}
			else if (TypeInfo::InheritsFrom<PropertyInfo^>(memberType)) {
				PropertyInfo^ pinfo = (PropertyInfo^)member->Member;

				if (pinfo->CanWrite && Interop::ChangeType(script, ret, pinfo->PropertyType)) {
					if (!TypeInfo::IsStatic(pinfo)) {
						SMInstance^ clrobj = jsinstances[(long)obj];

						if (clrobj != nullptr)
							pinfo->SetValue(clrobj->instance, ret, nullptr);

						else return JS_FALSE;
					}
					else pinfo->SetValue(nullptr, ret, nullptr);
				}
				else return JS_FALSE;
			}
			else return JS_FALSE;

		} catch (Exception^ e) {
			jsval propid;
			JS_LookupPropertyById(cx, obj, id, &propid);

			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, 
				Interop::FromJSValToManagedString(cx, propid),
				e->InnerException->Message);

			return JS_FALSE;
		}
	}
	return JS_TRUE;
}


JSBool SMPrototype::Method(JSContext* cx, uintN argc, jsval* vp)
{
	jsval* argv = JS_ARGV(cx, vp);

	JSFunction* fun = JS_ValueToFunction(cx, JS_CALLEE(cx, vp));
	JSObject* obj = JS_THIS_OBJECT(cx, vp);

	char* nameBytes = JS_EncodeString(cx, JS_GetFunctionId(fun));

	String^ name = Interop::ToManagedString(nameBytes);

	JS_free(cx, nameBytes);

	SMScript^ script = runtime->FindScript(cx);

	array<Object^>^ args = gcnew array<Object^>(argc);

	for(int i = 0; i < (int)argc; i++) {
		args[i] = Interop::FromJSVal(script, argv[i]);
	}

	SMMethodInfo^ method = nullptr;
	
	if (!TypeInfo::IsStatic(type))
		method = SMBinder::BindToMethod(script, obj, functions[name], args);

	if (method == nullptr)
		method = SMBinder::BindToMethod(script, obj, sfunctions[name], args);

	if (method == nullptr) return JS_FALSE;

	Object^ ret = nullptr;

	try {
		JSAutoSuspendRequest ar(cx);
		if (!method->Method->IsStatic) {
			SMInstance^ clrobj = jsinstances[(long)obj];

			if (clrobj != nullptr)
				ret = method->Method->Invoke(clrobj->instance, args);

			else return JS_FALSE;
		}
		else ret = method->Method->Invoke(nullptr, args);
	}
	catch(Exception^ e) {
		ret = nullptr;

		JS_ReportError(cx, "%s (%s.%s): %s", 
			e->InnerException->GetType()->ToString(),
			AccessibleName, name,
			e->InnerException->Message);

		return JS_FALSE;
	}

	jsval rval;
	Interop::ToJSVal(script, ret, &rval);

	JS_SET_RVAL(script->Context, vp, rval);
	return JS_TRUE;
}


void SMPrototype::Finalizer(JSContext* cx, JSObject* obj)
{
	SMInstance^ clrobj = jsinstances[(long)obj];
	SMPrototype^ proto = runtime->FindProto(SMEvent::typeid);

	if (clrobj != nullptr) {
		if (attribute->AllowScriptDispose) {
			if (TypeInfo::IsDisposable(type)) {
				delete clrobj->instance;
			}
		}

		if (this->Type == SMEventHandler::typeid) {
			SMEventHandler^ del = (SMEventHandler^)clrobj->instance;

			for each (DictionaryEntry^ entry in proto->jsinstances) {

				SMInstance^ sminst = (SMInstance^)entry->Value;
				SMEvent^ _event = (SMEvent^)sminst->instance;

				for (int i = (_event->Delegates->Count - 1); i >= 0; i--) {

					SMEventHandler^ del2 = _event->Delegates[i];
					if (del2 == del) _event->RemoveHandler(del2);
				}
			}
		}
		
		jsinstances->Remove((long)obj);
		delete clrobj;
	}

	for each(DictionaryEntry^ entry in jsevents) {

		SMEvent^ _event = (SMEvent^)entry->Value;
		long eventobj = (long)entry->Key;

		SMInstance^ sminst = proto->jsinstances[eventobj];
		proto->jsinstances->Remove(eventobj);

		_event->Release();

		delete sminst;
	}
}

