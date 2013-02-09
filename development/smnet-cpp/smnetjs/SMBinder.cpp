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
#include "SMBinder.h"
#include "SMRuntime.h"
#include "SMPrototype.h"
#include "SMHashtable.h"
#include "SMScript.h"
#include "SMMemberInfo.h"
#include "SMInstance.h"
#include "SMFunction.h"

using namespace smnetjs;


Delegate^ SMBinder::BindToDelegate(SMScript^ script, Type^ delegateType, ... array<Object^>^ args)
{
	//for count;
	Object^ tmp = nullptr;

	if (delegateType->IsGenericTypeDefinition) {

		array<Type^>^ gargs = delegateType->GetGenericArguments();
		args = GetGenericArguments(script, gargs, args);

		if (args == nullptr || args->Length != 1)
			return nullptr;

		tmp = args[0];

		if (!Interop::ChangeType(script, tmp, SMFunction::typeid))
			return nullptr;

		delegateType = delegateType->MakeGenericType(gargs);
	}
	else {
		if (args->Length != 1)
			return nullptr;

		tmp = args[0];

		if (!Interop::ChangeType(script, tmp, SMFunction::typeid))
			return nullptr;
	}

	return BindToDelegate(script, delegateType, (SMFunction^)tmp);
}

Delegate^ SMBinder::BindToDelegate(SMScript^ script, Type^ delegateType, SMFunction^ func)
{
	if (delegateType->IsAbstract)//can't instanciate abstract classes
		return nullptr;

	MethodInfo^ target = delegateType->GetMethod("Invoke");
	MethodInfo^ wrapper = func->GetType()->GetMethod("Invoke", BindingFlags::Public | BindingFlags::Instance);

	array<ParameterInfo^>^ params = target->GetParameters();
	array<Type^>^ sigArgs = gcnew array<Type^>(params->Length + 1);

	sigArgs[0] = array<Object^>::typeid;

	for (int i = 1; i < sigArgs->Length; i++)
		sigArgs[i] = params[i - 1]->ParameterType;

	DynamicMethod^ method = gcnew DynamicMethod("SomeMethod", target->ReturnType, sigArgs, true);
	ILGenerator^ methodIL = method->GetILGenerator();

	LocalBuilder^ consts = methodIL->DeclareLocal(array<Object^>::typeid);
    LocalBuilder^ args = methodIL->DeclareLocal(array<Object^>::typeid);

	LocalBuilder^ instance = methodIL->DeclareLocal(SMFunction::typeid);

    LocalBuilder^ returnType = methodIL->DeclareLocal(Type::typeid);
    LocalBuilder^ returnValue = methodIL->DeclareLocal(Object::typeid);

	methodIL->Emit(OpCodes::Ldarg_0);
	methodIL->Emit(OpCodes::Stloc_S, consts);
	
	//instance
	methodIL->Emit(OpCodes::Ldloc_S, consts);
    methodIL->Emit(OpCodes::Ldc_I4_S, 0);
    methodIL->Emit(OpCodes::Ldelem_Ref);
    methodIL->Emit(OpCodes::Castclass, SMFunction::typeid);
	methodIL->Emit(OpCodes::Stloc_S, instance);

    methodIL->Emit(OpCodes::Ldc_I4_S, sigArgs->Length - 1);
    methodIL->Emit(OpCodes::Newarr, Object::typeid);
    methodIL->Emit(OpCodes::Stloc_S, args);

	for (int i = 1; i < sigArgs->Length; i++) {
        methodIL->Emit(OpCodes::Ldloc_S, args);
        methodIL->Emit(OpCodes::Ldc_I4_S, i - 1);
        methodIL->Emit(OpCodes::Ldarg_S, i);
        methodIL->Emit(OpCodes::Box, sigArgs[i]);
        methodIL->Emit(OpCodes::Stelem_Ref);
    }

	methodIL->Emit(OpCodes::Ldloc_S, instance);
    methodIL->Emit(OpCodes::Ldloc_S, args);
    methodIL->Emit(OpCodes::Callvirt, wrapper);
    methodIL->Emit(OpCodes::Stloc_S, returnValue);

	//if the current delegate has a return type
    //we want to convert the return value and unbox it
    if (target->ReturnType != void::typeid) {

		MethodInfo^ convert = (Interop::typeid)->GetMethod("ChangeType");
		LocalBuilder^ scr = methodIL->DeclareLocal(SMScript::typeid);
		//script
		methodIL->Emit(OpCodes::Ldloc_S, consts);
		methodIL->Emit(OpCodes::Ldc_I4_S, 1);
		methodIL->Emit(OpCodes::Ldelem_Ref);
		methodIL->Emit(OpCodes::Castclass, SMScript::typeid);
		methodIL->Emit(OpCodes::Stloc_S, scr);

		Label ELSE = methodIL->DefineLabel();
        Label END =  methodIL->DefineLabel(); 

        //load our return type metadata token
		methodIL->Emit(OpCodes::Ldtoken, target->ReturnType);
        methodIL->Emit(OpCodes::Call, Type::typeid->GetMethod("GetTypeFromHandle"));
        methodIL->Emit(OpCodes::Stloc_S, returnType);

		methodIL->Emit(OpCodes::Ldloc_S, scr);
        methodIL->Emit(OpCodes::Ldloca_S, returnValue);
        methodIL->Emit(OpCodes::Ldloc_S, returnType);
        methodIL->Emit(OpCodes::Call, convert);
        methodIL->Emit(OpCodes::Brfalse_S, ELSE);
        methodIL->Emit(OpCodes::Br_S, END);

        methodIL->MarkLabel(ELSE);

		LocalBuilder^ local = methodIL->DeclareLocal(target->ReturnType);

		methodIL->Emit(OpCodes::Ldloca, local);
        methodIL->Emit(OpCodes::Initobj, local->LocalType);
		methodIL->Emit(OpCodes::Ldloc_S, local);
		methodIL->Emit(OpCodes::Box, Object::typeid);
		methodIL->Emit(OpCodes::Stloc_S, returnValue);

        methodIL->MarkLabel(END);

        methodIL->Emit(OpCodes::Ldloc_S, returnValue);
        methodIL->Emit(OpCodes::Unbox_Any, target->ReturnType);
	}

    methodIL->Emit(OpCodes::Ret);

	return method->CreateDelegate(delegateType, gcnew array<Object^>{ func, script });
}


ConstructorInfo^ SMBinder::BindToConstructor(SMScript^ script, SMPrototype^ proto, array<Object^>^ %args)
{
	if (!proto->Type->IsGenericType)
		return BindToConstructor(script, proto->constructors, args);
	else {
		array<Type^>^ gargs = proto->Type->GetGenericArguments();

		args = GetGenericArguments(script, gargs, args);
		if (args == nullptr) return nullptr;

		return BindToConstructor(script, proto->ReflectGenericConstructors(gargs), args);
	}
}

ConstructorInfo^ SMBinder::BindToConstructor(SMScript^ script, List<SMConstructorInfo^>^ ctors, array<Object^>^ %args)
{
	for (int a = 0; a < 2; a++) {
		for (int i = 0; i < ctors->Count; i++) {
			SMConstructorInfo^ method = ctors[i];
			ConstructorInfo^ info = method->Method;

			array<Object^>^ tmpargs = ConvertArguments(script, info, method->Attribute, args, (a == 0));

			if (tmpargs != nullptr) {
				args = tmpargs;
				return info;
			}
		}
	}
	return nullptr;
}


MethodInfo^ SMBinder::BindToMethod(SMScript^ script, SMPrototype^ proto, SMInstance^ clrobj, String^ name, array<Object^>^ %args)
{
	MethodInfo^ method = nullptr;

	if (TypeInfo::IsStatic(proto->Type)) {
		method = SMBinder::BindToMethod(script, proto->sfunctions[name], args);
	} else {
		if (!proto->Type->IsGenericType) {

			method = SMBinder::BindToMethod(script, proto->functions[name], args);

			if (method == nullptr)
				method = SMBinder::BindToMethod(script, proto->sfunctions[name], args);

		} else if (clrobj != nullptr) {
			System::Type^ t = clrobj->instance->GetType();
			array<System::Type^>^ targs = t->GetGenericArguments();

			method = SMBinder::BindToMethod(script, proto->ReflectGenericFunctions(targs, false)[name], args);

			if (method == nullptr)
				method = SMBinder::BindToMethod(script, proto->ReflectGenericFunctions(targs, true)[name], args);
		}
	}

	return method;
}

MethodInfo^ SMBinder::BindToMethod(SMScript^ script, List<SMMethodInfo^>^ funcs, array<Object^>^ %args)
{
	if (funcs == nullptr)
		return nullptr;

	array<Object^>^ tmp = (array<Object^>^)args->Clone();
	array<Object^>^ ret = nullptr;

	for (int a = 0; a < 2; a++) {
		for (int i = 0; i < funcs->Count; i++) {

			SMMethodInfo^ method = funcs[i];
			MethodInfo^ info = method->Method;

			if (method->Method->IsGenericMethod) {
				array<Type^>^ targs = info->GetGenericArguments();

				ret = GetGenericArguments(script, targs, tmp);
				if (ret == nullptr) continue;

				info = info->MakeGenericMethod(targs);
			}
			else ret = tmp;

			ret = ConvertArguments(script, info, method->Attribute, ret, (a == 0));

			if (ret != nullptr) {
				args = ret;
				return info;
			}
		}
	}
	return nullptr;
}


array<Object^>^ SMBinder::GetGenericArguments(SMScript^ script, array<Type^>^ %gargs, array<Object^>^ args)
{
	array<Type^>^ tmp = gcnew array<Type^>(gargs->Length);

	if (gargs->Length > args->Length)
		return nullptr;

	for (int i = 0; i < gargs->Length; i++) {
		if (args[i] == nullptr)
			return nullptr;

		if (args[i]->GetType() != SMFunction::typeid)
			return nullptr;

		Type^ t = script->mRuntime->FindType(((SMFunction^)args[i])->Name);

		if (t == nullptr) 
			return nullptr;

		tmp[i] = t;
	}

	gargs = tmp;

	array<Object^>^ ret = gcnew array<Object^>(args->Length - gargs->Length);
	Array::Copy(args, gargs->Length, ret, 0, ret->Length);

	return ret;
}

array<Object^>^ SMBinder::ConvertArguments(SMScript^ script, ConstructorInfo^ constructor, SMConstructor^ attribute, array<Object^>^ args, bool _explicit)
{
	array<ParameterInfo^>^ params = constructor->GetParameters();

	bool hasParams = false;

	int pcount = args->Length;
	int preq = params->Length;

	if (attribute->Script)
		pcount++;

	if (preq > 0) {
		for(int i = (params->Length - 1); i >= 0; i--) {
			if (params[i]->IsOptional)
				preq--;
		}

		if (hasParams = TypeInfo::IsParamArray(params[params->Length - 1]))
			preq--;		
	}

	if (pcount < preq || (!hasParams && pcount > params->Length)) 
		return nullptr;

	int startIndex = -1;
	array<Object^>^ argd = gcnew array<Object^>(preq);

	if (attribute->Script)
		argd[++startIndex] = script;

	for(int i = ++startIndex; i < params->Length; i++) {
		ParameterInfo^ param = params[i];

		if (hasParams && i == (params->Length - 1)) {
			int argindex = (i - startIndex);

			System::Type^ ptype = param->ParameterType->GetElementType();
			Array^ arr = Array::CreateInstance(ptype, args->Length - argindex);

			for(int x = argindex; x < args->Length; x++) {
				Object^ item2 = args[x];

				if (_explicit) {
					if (item2 == nullptr) {
						if (ptype->IsValueType)
							return nullptr;
					}
					else if (item2->GetType() != ptype)
						return nullptr;
				}
				else if (!Interop::ChangeType(script, item2, ptype))
					return nullptr;

				arr->SetValue(item2, x - argindex);
			}

			Array::Resize<Object^>(argd, i + 1);
			argd[i] = arr;
		}
		else if (i >= args->Length && param->IsOptional) {

			Array::Resize<Object^>(argd, i + 1);
			argd[i] = param->DefaultValue;
		}
		else {
			Object^ item = args[i - startIndex];

			if (_explicit) {
				if (item == nullptr) {
					if (param->ParameterType->IsValueType)
						return nullptr;
				}
				else if (item->GetType() != param->ParameterType)
					return nullptr;
			}	
			else if (!Interop::ChangeType(script, item, param->ParameterType))
				return nullptr;

			argd[i] = item;
		}
	}

	return argd;
}

array<Object^>^ SMBinder::ConvertArguments(SMScript^ script, MethodInfo^ method, SMMethod^ attribute, array<Object^>^ args, bool _explicit)
{
	array<ParameterInfo^>^ params = method->GetParameters();

	int pcount = args->Length;
	int preq = params->Length;

	bool hasParams = false;

	if (attribute->Script)
		pcount++;

	if (preq > 0) {
		for(int i = (params->Length - 1); i >= 0; i--) {
			if (params[i]->IsOptional)
				preq--;
		}

		if (hasParams = TypeInfo::IsParamArray(params[params->Length -1]))
			preq--;		
	}

	if (pcount < preq || (!hasParams && pcount > params->Length)) 
		return nullptr;

	int startIndex = -1;
	array<Object^>^ argd = gcnew array<Object^>(preq);

	if (attribute->Script)
		argd[++startIndex] = script;

	for(int i = ++startIndex; i < params->Length; i++) {
		ParameterInfo^ param = params[i];

		if (hasParams && i == (params->Length - 1)) {
			int argindex = (i - startIndex);

			System::Type^ ptype = param->ParameterType->GetElementType();
			Array^ arr = Array::CreateInstance(ptype, args->Length - argindex);

			for(int x = argindex; x < args->Length; x++) {
				Object^ item2 = args[x];

				if (_explicit) {
					if (item2 == nullptr) {
						if (ptype->IsValueType)
							return nullptr;
					}
					else if (item2->GetType() != ptype)
						return nullptr;
				}
				else if (!Interop::ChangeType(script, item2, ptype))
					return nullptr;

				arr->SetValue(item2, x - argindex);
			}

			Array::Resize<Object^>(argd, i + 1);
			argd[i] = arr;
		}
		else if (i >= args->Length && param->IsOptional) {

			Array::Resize<Object^>(argd, i + 1);
			argd[i] = param->DefaultValue;
		}
		else {
			Object^ item = args[i - startIndex];

			if (_explicit) {
				if (item == nullptr) {
					if (param->ParameterType->IsValueType)
						return nullptr;
				}
				else if (item->GetType() != param->ParameterType)
					return nullptr;
			}	
			else if (!Interop::ChangeType(script, item, param->ParameterType))
				return nullptr;

			argd[i] = item;
		}
	}

	return argd;
}

Object^ SMBinder::CallCLRFunction(SMScript^ script, IntPtr fobj, String^ name, array<Object^>^ args)
{
	Object^ ret = nullptr;

	if (script->mContext != nullptr) {
		script->mContext->Lock();

		JSObject* obj = (JSObject*)fobj.ToPointer();

		SMPrototype^ proto = script->mRuntime->FindProto(script, obj);
		SMInstance^ clrobj = script->mRuntime->FindInstance(obj);

		if (proto == nullptr) 
			return nullptr;

		MethodInfo^ method = SMBinder::BindToMethod(script, proto, clrobj, name, args);

		if (method == nullptr) 
			return nullptr;

		try {
			JSAutoSuspendRequest ar(script->Context);

			if (!method->IsStatic) {
				if (clrobj != nullptr)
					ret = method->Invoke(clrobj->instance, args);

				else return nullptr;
			}
			else ret = method->Invoke(nullptr, args);
		}
		catch(Exception^ e) {
			ret = nullptr;

			JS_ReportError(script->Context, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				proto->AccessibleName, name,
				e->InnerException->Message);
		}
		finally { script->mContext->Unlock(); }
	}
	return ret;
}

Object^ SMBinder::CallLambdaFunction(SMScript^ script, jsval funcval, array<Object^>^ args)
{
	if (script->mContext != nullptr) {
		script->mContext->Lock();

		try {
			JSFunction* func = JS_ValueToFunction(script->Context, funcval);
			if (!func) return nullptr;

			JSObject* funcobj = JS_GetFunctionObject(func);
			if (!funcobj) return nullptr;

			JSObject* fobj = JS_GetParent(script->Context, funcobj);
			if (!fobj) fobj = script->mContext->GetGlobalObject();

			uint32 flags = JS_GetFunctionFlags(func);

			if ((flags & JSFUN_LAMBDA) == JSFUN_LAMBDA) {
				jsval* argv = new jsval[args->Length];

				for(int i = 0; i < args->Length; i++) {
					Interop::ToJSVal(script, args[i], &argv[i]);
				}
				
				jsval rval;
				Object^ ret = nullptr;

				if (JS_CallFunctionValue(script->Context, fobj, funcval, args->Length, argv, &rval)) {
					ret = Interop::FromJSVal(script, rval);
					JS_MaybeGC(script->Context);
				}

				delete argv;
				return ret;
			}
			else {
				char* nameBytes = JS_EncodeString(script->Context, JS_GetFunctionId(func));
				String^ name = Interop::ToManagedStringInternal(nameBytes);

				JS_free(script->Context, nameBytes);

				return script->CallFunction<Object^>(name, IntPtr(fobj), args);
			}
		} 
		finally { script->mContext->Unlock(); }
	}
	return nullptr;
}