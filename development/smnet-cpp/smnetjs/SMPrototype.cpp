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
#include "SMPrototype.h"
#include "SMHashtable.h"
#include "SMMemberInfo.h"
#include "SMRuntime.h"
#include "SMScript.h"
#include "SMInstance.h"
#include "SMEvent.h"
#include "SMBinder.h"
#include "SMEnumerator.h"
#include "ISMDynamic.h"

using namespace smnetjs;


SMPrototype::SMPrototype(SMRuntime^ runtime, System::Type^ type)
{
	this->type = type;
	this->runtime = runtime;

	jsinstances = gcnew List<SMInstance^>();
	jsprototypes = gcnew SMHashtable<long, long>();
	
	constructors = gcnew List<SMConstructorInfo^>();

	properties = gcnew List<SMPropertyInfo^>();
	sproperties = gcnew List<SMPropertyInfo^>();
	iproperties = gcnew List<SMPropertyInfo^>();

	functions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();
	sfunctions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();

	ctor = gcnew SMNative(this, &SMPrototype::Constructor);
	method = gcnew SMNative(this, &SMPrototype::Method);

	getter = gcnew SMPropertyOp(this, &SMPrototype::Getter);
	setter = gcnew SMStrictPropertyOp(this, &SMPrototype::Setter);

	igetter = gcnew SMPropertyOp(this, &SMPrototype::IndexGetter);
	isetter = gcnew SMStrictPropertyOp(this, &SMPrototype::IndexSetter);
}

SMPrototype::SMPrototype(SMRuntime^ runtime, System::Type^ type, SMEmbedded^ attribute)
{
	this->type = type;
	this->runtime = runtime;

	this->attribute = attribute;

	jsinstances = gcnew List<SMInstance^>();
	jsprototypes = gcnew SMHashtable<long, long>();

	constructors = gcnew List<SMConstructorInfo^>();

	properties = gcnew List<SMPropertyInfo^>();
	sproperties = gcnew List<SMPropertyInfo^>();
	iproperties = gcnew List<SMPropertyInfo^>();

	functions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();
	sfunctions = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();

	ctor = gcnew SMNative(this, &SMPrototype::Constructor);
	method = gcnew SMNative(this, &SMPrototype::Method);

	getter = gcnew SMPropertyOp(this, &SMPrototype::Getter);
	setter = gcnew SMStrictPropertyOp(this, &SMPrototype::Setter);

	igetter = gcnew SMPropertyOp(this, &SMPrototype::IndexGetter);
	isetter = gcnew SMStrictPropertyOp(this, &SMPrototype::IndexSetter);
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
	if (attribute == nullptr) {
		SMEmbedded^ attr = TypeInfo::GetAttribute<SMEmbedded^>(type);

		if (attr != nullptr)
			attribute = attr;

		if (attribute == nullptr)
			attribute = gcnew SMEmbedded();
	}

	if (type->IsGenericTypeDefinition) {
		array<System::Type^>^ gtypes = type->GetGenericArguments();

		String^ name = this->Name;
		int badChar = name->IndexOf("`");

		if (badChar > -1)
			name = name->Remove(badChar);

		for (int i = 0; i < gtypes->Length; i++) {
			name = name + "$";
		}

		attribute->Name = name;
	}
	else ReflectConstructors();

	ReflectFunctions();
	ReflectProperties();

	CreateJSClass();
}


void SMPrototype::CreateJSClass()
{
	for each(System::Type^ t in type->GetNestedTypes())
			runtime->Embed(t);

	if (TypeInfo::IsStatic(type)) {
		JSClass jsclass = {
			Interop::ToUnmanagedStringInternal(Name),
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
		bool enumerable = TypeInfo::InheritsFrom<Collections::IEnumerable^>(Type);

		uint32 flags = JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE;

		if (enumerable) {
			flags = flags | JSCLASS_NEW_ENUMERATE;
			enumerate = gcnew SMEnumerateOp(this, &SMPrototype::Enumerate);
		}

		finalize = gcnew SMFinalizeOp(this, &SMPrototype::Finalizer);

		JSClass jsclass = {
			Interop::ToUnmanagedStringInternal(Name),
			flags,
			JS_PropertyStub,
			JS_PropertyStub,
			(JSPropertyOp)Interop::DelegatePointer(igetter),
			(JSStrictPropertyOp)Interop::DelegatePointer(isetter),
			(!enumerable) ? JS_EnumerateStub : (JSEnumerateOp)Interop::DelegatePointer(enumerate),
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
	SMInstance^ instance = FindInstance(script, object);

	if (instance == nullptr) {

		JSObject* obj = JS_NewObject(script->Context, clasp, (JSObject*)jsprototypes[(long)script->Context], NULL);

		if (!obj) return NULL;

		instance = gcnew SMInstance(script, obj, object);
		jsinstances->Add(instance);
	}

	if (lock) instance->GCLock();

	return instance->jsobj;
}


void SMPrototype::ReleaseInstance(SMScript^ script, Object^ object)
{
	SMInstance^ instance = FindInstance(script, object);

	if (instance != nullptr)
		instance->GCUnlock(false);
}


void SMPrototype::ReflectConstructors()
{
	BindingFlags bf = BindingFlags::Public |
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	array<ConstructorInfo^>^ ctors = type->GetConstructors(bf);

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

		if (method->IsSpecialName)
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
			if (!String::IsNullOrEmpty(mg->Name))
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
			Interop::ToUnmanagedStringInternal(name),
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
			Interop::ToUnmanagedStringInternal(name),
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
			properties->Add(gcnew SMPropertyInfo(mg, field));
		else
			sproperties->Add(gcnew SMPropertyInfo(mg, field));
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
		
		if (!TypeInfo::IsStatic(prop)) {
			if (prop->GetIndexParameters()->Length > 0) {
				iproperties->Add(gcnew SMPropertyInfo(mg, prop));
			}
			else properties->Add(gcnew SMPropertyInfo(mg, prop));
		}
		else sproperties->Add(gcnew SMPropertyInfo(mg, prop));
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
			properties->Add(gcnew SMPropertyInfo(mg, _event));
		else
			sproperties->Add(gcnew SMPropertyInfo(mg, _event));
	}
#pragma endregion

#pragma region " Build PropertySpec "

	int index = -1;
	int8 tinyid = -1;

	JSPropertySpec nullspec = { NULL, 0, 0, NULL, NULL };
	this->props = new JSPropertySpec[properties->Count + 1];

	for (int i = 0; i < properties->Count; i++) {
		SMPropertyInfo^ member = properties[i];
		uint8 pflags = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;

		if (!member->Attribute->Enumerate)
			pflags ^= JSPROP_ENUMERATE;

		if (TypeInfo::InheritsFrom<EventInfo^>(member->Member->GetType()))
			pflags ^= JSPROP_SHARED;

		JSPropertySpec spec = {
			Interop::ToUnmanagedStringInternal(member->Name),
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
		SMPropertyInfo^ member = sproperties[i];
		uint8 pflags = JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_SHARED;
		
		if (!member->Attribute->Enumerate)
			pflags ^= JSPROP_ENUMERATE;

		if (TypeInfo::InheritsFrom<EventInfo^>(member->Member->GetType()))
			pflags ^= JSPROP_SHARED;

		JSPropertySpec spec = {
			Interop::ToUnmanagedStringInternal(member->Name),
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


SMInstance^ SMPrototype::FindInstance(JSObject* obj)
{
	for (int i = 0; i < jsinstances->Count; i++) {
		if (jsinstances[i]->jsobj == obj)
			return jsinstances[i];
	}

	return nullptr;
}

SMInstance^ SMPrototype::FindInstance(SMScript^ script, Object^ obj)
{
	for (int i = 0; i < jsinstances->Count; i++) {
		SMInstance^ inst = jsinstances[i];
		if (inst->script == script && inst->instance == obj)
			return inst;
	}

	return nullptr;
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


bool SMPrototype::HasMemberName(String^ name)
{
	for (int i = 0; i < properties->Count; i++)
		if (properties[i]->Name == name) return true;

	for (int i = 0; i < sproperties->Count; i++)
		if (sproperties[i]->Name == name) return true;

	for each(DictionaryEntry^ entry in functions) {
		String^ mname = (String^)entry->Key;
		if (mname == name) return true;
	}

	for each(DictionaryEntry^ entry in sfunctions) {
		String^ mname = (String^)entry->Key;
		if (mname == name) return true;
	}

	return false;
}


JSBool SMPrototype::Constructor(JSContext* cx, uintN argc, jsval* vp)
{
	Object^ ret = nullptr;
	jsval* argv = JS_ARGV(cx, vp);

	SMScript^ script = runtime->FindScript(cx);
	array<Object^>^ args = gcnew array<Object^>(argc);

	for(int i = 0; i < (int)argc; i++)
		args[i] = Interop::FromJSVal(script, argv[i]);

	if (TypeInfo::IsDelegate(type)) {

		ret = SMBinder::BindToDelegate(script, Type, args);
		if (ret == nullptr) return JS_FALSE;
	}
	else {
		ConstructorInfo^ ctor = SMBinder::BindToConstructor(script, this, args);

		if (ctor == nullptr) 
			return JS_FALSE;

		try {
			JSAutoSuspendRequest ar(cx);
			ret = ctor->Invoke(args);
		}
		catch(Exception^ e) {

			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, 
				"ctor", 
				e->InnerException->Message);

			return JS_FALSE;
		}
	}

	if (ret == nullptr) return JS_FALSE;

	JSObject* obj = CreateInstance(script, ret);
	JS_SET_RVAL(cx, vp, OBJECT_TO_JSVAL(obj));

	return JS_TRUE;
}


JSBool SMPrototype::IndexGetter(JSContext* cx, JSObject* obj, jsid id, jsval* vp) 
{ 
	jsval idv;
	JS_IdToValue(cx, id, &idv);

	SMScript^ script = runtime->FindScript(cx);
	SMInstance^ clrobj = FindInstance(obj);;

	Object^ ret = nullptr;
	Object^ idval = Interop::FromJSVal(script, idv);

	if (idval != nullptr && idval->GetType() == String::typeid) {
		String^ value = (String^)idval;

		if (HasMemberName(value))
			return JS_TRUE;

		if (ISMDynamic::typeid->IsAssignableFrom(Type)) {
			try {

				ISMDynamic^ dyn = (ISMDynamic^)clrobj->instance;
				ret = dyn->OnPropertyGetter(script, value);

				if (ret != nullptr) {
					Interop::ToJSVal(script, ret, vp);
					return JS_TRUE;
				}

			}catch(...) {

			}
		}
	}

	if (iproperties->Count > 0) {

		List<SMPropertyInfo^>^ props = nullptr;

		if (!Type->IsGenericType)
			props = iproperties;
		else {
			System::Type^ t = clrobj->instance->GetType();
			props = ReflectGenericIndexProperties(t->GetGenericArguments());
		}

		for (int i = 0; i < props->Count; i++) {

			PropertyInfo^ prop = (PropertyInfo^)props[i]->Member;
			array<ParameterInfo^>^ params = prop->GetIndexParameters();

			if (!prop->CanRead || params->Length != 1)
				continue;

			if (Interop::ChangeType(script, idval, params[0]->ParameterType)) {
				try {

					JSAutoSuspendRequest ar(script->Context);
					ret = prop->GetValue(clrobj->instance, gcnew array<Object^>{ idval });

				} catch (...) { continue; }
			
				Interop::ToJSVal(script, ret, vp);
				return JS_TRUE;
			}
		}
	}

	return JS_TRUE;
}

JSBool SMPrototype::IndexSetter(JSContext* cx, JSObject* obj, jsid id, JSBool strict, jsval* vp) 
{
	jsval idv;
	JS_IdToValue(cx, id, &idv);

	SMScript^ script = runtime->FindScript(cx);
	SMInstance^ clrobj = FindInstance(obj);

	Object^ idval = Interop::FromJSVal(script, idv);
	Object^ ret = Interop::FromJSVal(script, *vp);

	if (idval != nullptr && idval->GetType() == String::typeid) {
		String^ value = (String^)idval;

		if (HasMemberName(value))
			return JS_TRUE;

		if (ISMDynamic::typeid->IsAssignableFrom(Type)) {
			try {

				ISMDynamic^ dyn = (ISMDynamic^)clrobj->instance;
				dyn->OnPropertySetter(script, value, ret);

			} catch (...) {

			}
		}
	}

	if (iproperties->Count > 0) {

		List<SMPropertyInfo^>^ props = nullptr;

		if (!Type->IsGenericType)
			props = iproperties;
		else {
			System::Type^ t = clrobj->instance->GetType();
			props = ReflectGenericIndexProperties(t->GetGenericArguments());
		}

		for (int i = 0; i < props->Count; i++) {

			PropertyInfo^ prop = (PropertyInfo^)props[i]->Member;
			array<ParameterInfo^>^ params = prop->GetIndexParameters();

			if (!prop->CanWrite || params->Length != 1)
				continue;

			if (Interop::ChangeType(script, idval, params[0]->ParameterType)) {
				try {

					JSAutoSuspendRequest ar(script->Context);
					prop->SetValue(clrobj->instance, ret, gcnew array<Object^>{ idval });
				
				} catch (...) { continue; }

				return JS_TRUE;
			}
		}
	}

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

		Object^ ret = nullptr;
		SMScript^ script = runtime->FindScript(cx);

		SMInstance^ clrobj = FindInstance(obj);

#pragma region " Member Lookup "

		SMPropertyInfo^ member;

		if (!Type->IsGenericType) {
			if (index < pcount)
				member = properties[index];
			else {
				index -= pcount;
				if (index < scount)
					member = sproperties[index];

				else return JS_FALSE;
			}
		}
		else if (clrobj != nullptr) {
			System::Type^ t = clrobj->instance->GetType();

			array<System::Type^>^ targs = t->GetGenericArguments();
			List<SMPropertyInfo^>^ props = ReflectGenericProperties(targs, false);

			if (index < props->Count)
				member = props[index];
			else
			{
				index -= props->Count;
				props = ReflectGenericProperties(targs, true);

				if (index < props->Count)
					member = props[index];

				else return JS_FALSE;
			}
		}
		else return JS_FALSE;

#pragma endregion

#pragma region " Invoke PropertyInfo "

		System::Type^ memberType = member->Member->GetType();

		try {
			if (TypeInfo::InheritsFrom<FieldInfo^>(memberType)) {
				JSAutoSuspendRequest ar(script->Context);
				FieldInfo^ finfo = (FieldInfo^)member->Member;

				if (!finfo->IsStatic) {
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
				//event properties are not embedded with JSPROP_SHARED
				//so we only need to create and return a value once
				//after that it's sufficient to just return JS_TRUE.
				SMEvent^ _event = nullptr;
				Object^ instance = nullptr;

				EventInfo^ einfo = (EventInfo^)member->Member;
				SMPrototype^ proto = runtime->FindProto(SMEvent::typeid);

				for each(SMInstance^ i in proto->jsinstances) {
					_event = (SMEvent^)i->instance;

					if (_event->JSParent == obj && _event->Event == einfo)
						return JS_TRUE;
				}
				
				if (!TypeInfo::IsStatic(type)){
					if (clrobj != nullptr)
						instance = clrobj->instance;

					else return JS_FALSE;
				}

				_event = gcnew SMEvent(script, obj, instance, einfo);
				*vp = OBJECT_TO_JSVAL(proto->CreateInstance(script, _event));

				return JS_TRUE;
			}
		} catch (Exception^ e) {

			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, 
				member->Name,
				e->InnerException->Message);

			return JS_FALSE;
		}

#pragma endregion

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

#pragma region " Member Lookup "

		SMPropertyInfo^ member;
		SMInstance^ clrobj = FindInstance(obj);

		if (!Type->IsGenericType) {
			if (index < pcount)
				member = properties[index];
			else {
				index -= pcount;
				if (index < scount)
					member = sproperties[index];

				else return JS_FALSE;
			}
		}
		else if (clrobj != nullptr) {
			System::Type^ t = clrobj->instance->GetType();

			array<System::Type^>^ targs = t->GetGenericArguments();
			List<SMPropertyInfo^>^ props = ReflectGenericProperties(targs, false);

			if (index < props->Count)
				member = props[index];
			else
			{
				index -= props->Count;
				props = ReflectGenericProperties(targs, true);

				if (index < props->Count)
					member = props[index];

				else return JS_FALSE;
			}
		}
		else return JS_FALSE;

#pragma endregion
	
		SMScript^ script = runtime->FindScript(cx);
		Object^ ret = Interop::FromJSVal(script, *vp);

		System::Type^ memberType = member->Member->GetType();

		try {
			JSAutoSuspendRequest ar(script->Context);
			if (TypeInfo::InheritsFrom<FieldInfo^>(memberType)) {
				FieldInfo^ finfo = (FieldInfo^)member->Member;

				if (!finfo->IsInitOnly && Interop::ChangeType(script, ret, finfo->FieldType)) {
					if (!finfo->IsStatic) {
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

			JS_ReportError(cx, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				AccessibleName, 
				member->Name,
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
	String^ name = Interop::ToManagedStringInternal(nameBytes);

	JS_free(cx, nameBytes);

	Object^ ret = nullptr;

	SMScript^ script = runtime->FindScript(cx);
	SMInstance^ clrobj = FindInstance(obj);

	array<Object^>^ args = gcnew array<Object^>(argc);

	for(int i = 0; i < (int)argc; i++)
		args[i] = Interop::FromJSVal(script, argv[i]);

	MethodInfo^ method = SMBinder::BindToMethod(script, this, clrobj, name, args);

	if (method == nullptr) 
		return JS_FALSE;

	try {
		JSAutoSuspendRequest ar(cx);

		if (!method->IsStatic) {
			if (clrobj != nullptr)
				ret = method->Invoke(clrobj->instance, args);

			else return JS_FALSE;
		}
		else ret = method->Invoke(nullptr, args);
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

//Only used if a type is instance and implements IEnumerable
JSBool SMPrototype::Enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op, jsval *statep, jsid *idp)
{
	SMScript^ script = runtime->FindScript(cx);

	SMInstance^ instance = runtime->FindInstance(obj);
	SMEnumerator^ enumerator = nullptr;

	if (instance == nullptr)
		return JS_TRUE;

	switch (enum_op) {
		case JSENUMERATE_INIT:
			{
				enumerator = gcnew SMEnumerator(instance);

				if (!runtime->Embed(enumerator->GetType()))
					return JS_FALSE;

				enumerator->Reset();

				if (idp) 
					*idp = INT_TO_JSID(0);

				Interop::ToJSVal(script, enumerator, statep);
			}
			break;
		case JSENUMERATE_NEXT:
			{
				jsval tmp;
				jsval rval;

				enumerator = (SMEnumerator^)Interop::FromJSVal(script, *statep);

				if (enumerator->MoveNext()) {
					
					if (enumerator->Current != nullptr)
						runtime->Embed(enumerator->Current->GetType());

					String^ name = String::Format("{0}", enumerator->Index - 1); 

					Interop::ToJSVal(script, name, &tmp);
					Interop::ToJSVal(script, enumerator->Current, &rval);

					JS_ValueToId(script->Context, tmp, idp);
					JS_DefinePropertyById(cx, obj, *idp, rval, NULL, NULL, 0);

					break;
				}
				else {
					if (!JS_NextProperty(cx, enumerator->Iterator, idp))
						return JS_FALSE;

					if (!JSID_IS_VOID(*idp))
						break;
				}
				/* fall through */
			}
		case JSENUMERATE_DESTROY:
			if (enumerator != nullptr)
				enumerator->Reset();

			*statep = JSVAL_NULL;
			break;
	}
	
	return JS_TRUE;
}

void SMPrototype::Finalizer(JSContext* cx, JSObject* obj)
{
	SMInstance^ clrobj = FindInstance(obj);
	SMPrototype^ proto = runtime->FindProto(SMEvent::typeid);

	if (clrobj != nullptr) {
		
		if (this->Type == SMEventHandler::typeid) {
			SMEventHandler^ del = (SMEventHandler^)clrobj->instance;

			for (int i = 0; i < proto->jsinstances->Count; i++) {

				SMInstance^ sminst = proto->jsinstances[i];
				SMEvent^ _event = (SMEvent^)sminst->instance;

				for (int i = (_event->Delegates->Count - 1); i >= 0; i--) {

					SMEventHandler^ del2 = _event->Delegates[i];
					if (del2 == del) _event->RemoveHandler(del2);
				}
			}
		}

		jsinstances->Remove(clrobj);

		if (attribute->AllowScriptDispose && TypeInfo::IsDisposable(type))
			delete clrobj->instance;
		
		delete clrobj;
	}

	for (int i = (proto->jsinstances->Count - 1); i >= 0; i--) {
		SMInstance^ inst = proto->jsinstances[i];
		SMEvent^ _event = (SMEvent^)inst->instance;

		if (_event->JSParent == obj) {
			proto->jsinstances->RemoveAt(i);

			_event->Release();
			delete inst;
		}
	}
}


List<SMConstructorInfo^>^ SMPrototype::ReflectGenericConstructors(array<System::Type^>^ types)
{
	BindingFlags bf = BindingFlags::Public |
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	System::Type^ genType = this->Type->MakeGenericType(types);
	array<ConstructorInfo^>^ ctors = genType->GetConstructors(bf);

	List<SMConstructorInfo^>^ ret = gcnew List<SMConstructorInfo^>();

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

		ret->Add(gcnew SMConstructorInfo(mg, ctor));
	}

	return ret;
}


List<SMPropertyInfo^>^ SMPrototype::ReflectGenericIndexProperties(array<System::Type^>^ types)
{
	System::Type^ genType = Type->MakeGenericType(types);
	List<SMPropertyInfo^>^ gprops = gcnew List<SMPropertyInfo^>();

#pragma region " Reflect Properties "

	BindingFlags bf = BindingFlags::Public |
					  BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	array<PropertyInfo^>^ props = genType->GetProperties(bf);

	for (int i = 0; i < props->Length; i++) {
		PropertyInfo^ prop = props[i];

		if (prop->GetIndexParameters()->Length == 0)
			continue;

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)prop);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed && !runtime->Embed(prop->PropertyType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)prop);

		if (mg == nullptr)
			mg = gcnew SMProperty();

		gprops->Add(gcnew SMPropertyInfo(mg, prop));
	}

#pragma endregion

	return gprops;
}

List<SMPropertyInfo^>^ SMPrototype::ReflectGenericProperties(array<System::Type^>^ types, bool _static)
{
	System::Type^ genType = Type->MakeGenericType(types);
	List<SMPropertyInfo^>^ gprops = gcnew List<SMPropertyInfo^>();

#pragma region " Reflect Properties "

	BindingFlags bf = BindingFlags::Public;

	if (_static)
		bf = bf | BindingFlags::Static;
	else
		bf = bf | BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	array<FieldInfo^>^ fields = genType->GetFields(bf);

	for (int i = 0; i < fields->Length; i++) {
		FieldInfo^ field = fields[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)field);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed && !runtime->Embed(field->FieldType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)field);

		if (mg == nullptr)
			mg = gcnew SMProperty();

		gprops->Add(gcnew SMPropertyInfo(mg, field));
	}

	array<PropertyInfo^>^ props = genType->GetProperties(bf);

	for (int i = 0; i < props->Length; i++) {
		PropertyInfo^ prop = props[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)prop);
		if (ig != nullptr) continue;

		if (runtime->autoEmbed && !runtime->Embed(prop->PropertyType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)prop);

		if (mg == nullptr)
			mg = gcnew SMProperty();
		
		gprops->Add(gcnew SMPropertyInfo(mg, prop));
	}

	array<EventInfo^>^ events = genType->GetEvents(bf);

	for (int i = 0; i < events->Length; i++) {
		EventInfo^ _event = events[i];

		SMIgnore^ ig = TypeInfo::GetAttribute<SMIgnore^>((MemberInfo^)_event);
		if (ig != nullptr) continue;

		if (!runtime->Embed(_event->EventHandlerType))
			continue;

		SMProperty^ mg = TypeInfo::GetAttribute<SMProperty^>((MemberInfo^)_event);

		if (mg == nullptr)
			mg = gcnew SMProperty();

		gprops->Add(gcnew SMPropertyInfo(mg, _event));
	}
#pragma endregion

	return gprops;
}


SMHashtable<String^, List<SMMethodInfo^>^>^ SMPrototype::ReflectGenericFunctions(array<System::Type^>^ types, bool _static)
{
	System::Type^ genType = Type->MakeGenericType(types);
	SMHashtable<String^, List<SMMethodInfo^>^>^ gfuncs = gcnew SMHashtable<String^, List<SMMethodInfo^>^>();

#pragma region " Reflect Functions "

	BindingFlags bf = BindingFlags::Public;

	if (_static)
		bf = bf | BindingFlags::Static;
	else
		bf = bf | BindingFlags::Instance;

	if (!attribute->AllowInheritedMembers)
		bf = bf | BindingFlags::DeclaredOnly;

	array<MethodInfo^>^ methods = genType->GetMethods(bf);

	for(int i = 0; i < methods->Length; i++) {
		MethodInfo^ method = methods[i];

		if (method->IsSpecialName)
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
			if (!String::IsNullOrEmpty(mg->Name))
				name = mg->Name;
		}
		else mg = gcnew SMMethod();

		List<SMMethodInfo^>^ m;

		m = gfuncs[name];

		if (m == nullptr)
			m = gcnew List<SMMethodInfo^>();

		m->Add(gcnew SMMethodInfo(mg, method));

		gfuncs[name] = m;
	}
#pragma endregion

	return gfuncs;
}