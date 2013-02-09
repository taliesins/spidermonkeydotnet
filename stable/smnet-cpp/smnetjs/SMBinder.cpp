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

Delegate^ SMBinder::BindToDelegate(SMPrototype^ proto, SMScript^ script, SMFunction^ func)
{
	MethodInfo^ target = proto->Type->GetMethod("Invoke");

	MethodInfo^ wrapper = func->GetType()->GetMethod("Invoke", BindingFlags::Public | BindingFlags::Instance);
	MethodInfo^ convert = (Interop::typeid)->GetMethod("ChangeType");

	Generic::IEnumerable<ParameterExpression^>^ parameters = CreateArguments(target->GetParameters());

	ConstantExpression^ instance = Expression::Constant(func);

	ParameterExpression^ arguments = Expression::Parameter(array<Object^>::typeid, "args");
	ParameterExpression^ retval = Expression::Parameter(Object::typeid, "retval");
    ParameterExpression^ success = Expression::Parameter(Boolean::typeid, "succeess");

	Func<ParameterExpression^, UnaryExpression^>^ select = 
		gcnew Func<ParameterExpression^, UnaryExpression^>(&SMBinder::SelectArguments);

	if (target->ReturnType == void::typeid) {

		LambdaExpression^ expr = Expression::Lambda(
			proto->Type,
			Expression::Block(
				gcnew array<ParameterExpression^>{ arguments },
				Expression::Assign(
					arguments,
					Expression::NewArrayInit(Object::typeid, (Generic::IEnumerable<Expression^>^)Enumerable::Select<ParameterExpression^, UnaryExpression^>(parameters, select))
				),
				Expression::Call(
					instance,
					wrapper, 
					arguments
				)
			),
			parameters
		);

		return expr->Compile();
	}
	else {
		LambdaExpression^ expr = Expression::Lambda(
			proto->Type,
			Expression::Block(
				gcnew array<ParameterExpression^>{ arguments },
				Expression::Assign(
					arguments,
					Expression::NewArrayInit(Object::typeid, (Generic::IEnumerable<Expression^>^)
						Enumerable::Select<ParameterExpression^, UnaryExpression^>(parameters, select))
				),
				Expression::Assign(
					retval,
					Expression::Call(
						instance,
						wrapper, 
						arguments
					)
				),
				Expression::Assign(
					success,
					Expression::Call(
						convert,
						Expression::Constant(script),
						retval,
						Expression::Constant(target->ReturnType)
					)
				),
				Expression::IfThen(
					Expression::IsFalse(success),
					Expression::Assign(retval, Expression::Convert(Expression::Default(target->ReturnType), Object::typeid))
				),
				Expression::Unbox(retval, target->ReturnType)
			),
			parameters
		);

		return expr->Compile();
	}
}


SMConstructorInfo^ SMBinder::BindToConstructor(SMScript^ script, List<SMConstructorInfo^>^ ctors, array<Object^>^ %args)
{
	for (int i = 0; i < ctors->Count; i++) {
		SMConstructorInfo^ method = ctors[i];

		array<Object^>^ tmpargs = ConvertArguments(script, method, args, true);

		if (tmpargs != nullptr) {
			args = tmpargs;
			return method;
		}
	}

	for (int i = 0; i < ctors->Count; i++) {
		SMConstructorInfo^ method = ctors[i];

		array<Object^>^ tmpargs = ConvertArguments(script, method, args, false);

		if (tmpargs != nullptr) {
			args = tmpargs;
			return method;
		}
	}
	return nullptr;
}

SMMethodInfo^ SMBinder::BindToMethod(SMScript^ script, JSObject* obj, List<SMMethodInfo^>^ funcs, array<Object^>^ %args)
{
	if (funcs == nullptr)
		return nullptr;

	for (int i = 0; i < funcs->Count; i++) {
		SMMethodInfo^ method = funcs[i];

		array<Object^>^ tmpargs = ConvertArguments(script, obj, method, args, true);

		if (tmpargs != nullptr) {
			args = tmpargs;
			return method;
		}
	}

	for (int i = 0; i < funcs->Count; i++) {
		SMMethodInfo^ method = funcs[i];

		array<Object^>^ tmpargs = ConvertArguments(script, obj, method, args, false);

		if (tmpargs != nullptr) {
			args = tmpargs;
			return method;
		}
	}
	return nullptr;
}


array<ParameterExpression^>^ SMBinder::CreateArguments(array<ParameterInfo^>^ parameters)
{
	array<ParameterExpression^>^ ret = gcnew array<ParameterExpression^>(parameters->Length);

    for (int i = 0; i < ret->Length; i++)
        ret[i] = Expression::Parameter(parameters[i]->ParameterType, parameters[i]->Name);

    return ret;
}


array<Object^>^ SMBinder::ConvertArguments(SMScript^ script, SMConstructorInfo^ constructor, array<Object^>^ args, bool _explicit)
{
	int pcount = args->Length;

	if (constructor->Attribute->Script)
		pcount++;

	array<ParameterInfo^>^ params = constructor->Method->GetParameters();

	if (params->Length > 0) {
		int preq = params->Length;

		for(int i = (params->Length - 1); i >= 0; i--) {
			if (params[i]->IsOptional)
				preq--;
		}

		if (TypeInfo::IsParamArray(params[params->Length -1]))
			preq--;

		if (pcount < preq) 
			return nullptr;
	}
	else if (pcount != params->Length)
		return nullptr;

	int startIndex = -1;
	array<Object^>^ argd = gcnew array<Object^>(pcount);

	if (constructor->Attribute->Script)
		argd[++startIndex] = script;

	for(int i = ++startIndex; i < params->Length; i++) {
		ParameterInfo^ param = params[i - startIndex];

		if (TypeInfo::IsParamArray(param)) {
			int argindex = (i - startIndex);
			if (args->Length > argindex) {

				System::Type^ ptype = param->ParameterType->GetElementType();
				Array^ arr = Array::CreateInstance(ptype, args->Length - argindex);

				for(int x = argindex; x < args->Length; x++) {
					Object^ item2 = args[x];

					if (!ConvertArgument(script, item2, ptype, _explicit))
						return nullptr;

					arr->SetValue(item2, x - argindex);
				}
				argd[i] = arr;
			}
			Array::Resize<Object^>(argd, i + 1);
		}
		else if (i >= args->Length && param->IsOptional) {

			Array::Resize<Object^>(argd, argd->Length + 1);
			argd[i] = param->DefaultValue;
		}
		else {
			Object^ item = args[i - startIndex];

			if (!ConvertArgument(script, item, param->ParameterType, _explicit))
				return nullptr;

			argd[i] = item;
		}
	}

	return argd;
}

array<Object^>^ SMBinder::ConvertArguments(SMScript^ script, JSObject* obj, SMMethodInfo^ method, array<Object^>^ args, bool _explicit)
{
	int pcount = args->Length;

	if (method->Attribute->Script)
		pcount++;

	array<ParameterInfo^>^ params = method->Method->GetParameters();

	if (params->Length > 0) {
		int preq = params->Length;

		for(int i = (params->Length - 1); i >= 0; i--) {
			if (params[i]->IsOptional)
				preq--;
		}

		if (TypeInfo::IsParamArray(params[params->Length -1]))
			preq--;

		if (pcount < preq) 
			return nullptr;
	}
	else if (pcount != params->Length)
		return nullptr;

	int startIndex = -1;
	array<Object^>^ argd = gcnew array<Object^>(pcount);

	if (method->Attribute->Script)
		argd[++startIndex] = script;

	for(int i = ++startIndex; i < params->Length; i++) {
		ParameterInfo^ param = params[i];

		if (TypeInfo::IsParamArray(param)) {
			int argindex = (i - startIndex);
			if (args->Length > argindex) {

				System::Type^ ptype = param->ParameterType->GetElementType();
				Array^ arr = Array::CreateInstance(ptype, args->Length - argindex);

				for(int x = argindex; x < args->Length; x++) {
					Object^ item2 = args[x];

					if (!ConvertArgument(script, item2, ptype, _explicit))
						return nullptr;

					arr->SetValue(item2, x - argindex);
				}
				argd[i] = arr;
			}
			Array::Resize<Object^>(argd, i + 1);
		}
		else if (i >= args->Length && param->IsOptional) {

			Array::Resize<Object^>(argd, argd->Length + 1);
			argd[i] = param->DefaultValue;
		}
		else {
			Object^ item = args[i - startIndex];

			if (!ConvertArgument(script, item, param->ParameterType, _explicit))
				return nullptr;

			argd[i] = item;
		}
	}

	return argd;
}


bool SMBinder::ConvertArgument(SMScript^ script, Object^ %item, System::Type^ type, bool _explicit)
{
	if (_explicit)
    {
        if (item == nullptr)
        {
            if (type->IsValueType)
                return false;

            else return true;
        }

        System::Type^ t = item->GetType();

        if (t != type)
            return false;
    }
    else if (!Interop::ChangeType(script, item, type))
        return false;

    return true;
}


UnaryExpression^ SMBinder::SelectArguments(ParameterExpression^ s)
{
	return Expression::Convert(s, Object::typeid);
}


Object^ SMBinder::CallCLRFunction(SMScript^ script, IntPtr fobj, String^ name, array<Object^>^ args)
{
	Object^ ret = nullptr;
	if (script->mContext != nullptr) {
		JSObject* obj = (JSObject*)fobj.ToPointer();

		SMPrototype^ proto = script->mRuntime->FindProto(script, obj);
		SMInstance^ clrobj = script->mRuntime->FindInstance(obj);

		if (proto == nullptr) return nullptr;

		try {
			SMMethodInfo^ method = nullptr;
	
			if (!TypeInfo::IsStatic(proto->type))
				method = BindToMethod(script, obj, proto->functions[name], args);

			if (method == nullptr)
				method = BindToMethod(script, obj, proto->sfunctions[name], args);

			if (method == nullptr) 
				return nullptr;

			if (clrobj == nullptr)
				ret = method->Method->Invoke(nullptr, args);
			else
				ret = method->Method->Invoke(clrobj->instance, args);
		}
		catch(Exception^ e) {
			ret = nullptr;

			JS_ReportError(script->Context, "%s (%s.%s): %s", 
				e->InnerException->GetType()->ToString(),
				proto->AccessibleName, name,
				e->InnerException->Message);
		}
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
				String^ name = Interop::ToManagedString(nameBytes);

				JS_free(script->Context, nameBytes);

				return script->CallFunction<Object^>(name, IntPtr(fobj), args);
			}
		} 
		finally { script->mContext->Unlock(); }
	}
	return nullptr;
}