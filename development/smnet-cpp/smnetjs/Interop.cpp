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
#include "jsdate.h"
#include "Interop.h"
#include "TypeInfo.h"
#include "SMBinder.h"
#include "SMRuntime.h"
#include "SMPrototype.h"
#include "SMScript.h"
#include "SMHashtable.h"
#include "SMFunction.h"
#include "SMInstance.h"

using namespace smnetjs;


void* Interop::DelegatePointer(Delegate^ del)
{
	return Marshal::GetFunctionPointerForDelegate(del).ToPointer();
}


IntPtr Interop::ToUnmanagedString(String^ str) {
	return IntPtr(ToUnmanagedStringInternal(str));
}

char* Interop::ToUnmanagedStringInternal(String^ str)
{
	str = Encoding::Default->GetString(Encoding::UTF8->GetBytes(str));
	return (char*)Marshal::StringToHGlobalAnsi(str).ToPointer();
}

String^ Interop::ToManagedString(IntPtr str) {
	return ToManagedStringInternal((char*)str.ToPointer());
}

String^ Interop::ToManagedStringInternal(const char* str)
{
	String^ ret = Marshal::PtrToStringAnsi(IntPtr((void*)str));
	return Encoding::UTF8->GetString(Encoding::Default->GetBytes(ret));
}


//public API -- Thread-Safety?
void Interop::ToJSVal(SMScript^ script, Object^ value, IntPtr rval)
{
	script->mContext->Lock();
	ToJSVal(script, value, (jsval*)rval.ToPointer());
	script->mContext->Unlock();
}

void Interop::ToJSVal(SMScript^ script, Object^ value, jsval* rval)
{
	if (value == nullptr) {
		*rval = JSVAL_NULL;
		return;
	}

	Type^ type = value->GetType();

	if (type->IsArray) {

		Array^ valarray = (Array^)value;
		JSObject* jsarray = JS_NewArrayObject(script->Context, 0, NULL);

		for(int i = 0; i < valarray->Length; i++) {
			jsval ret;

			ToJSVal(script, valarray->GetValue(i), &ret);
			JS_SetElement(script->Context, jsarray, i, &ret);
		}

		*rval = OBJECT_TO_JSVAL(jsarray);
	}
	else {
		switch(Type::GetTypeCode(type))
		{
			case TypeCode::Boolean:
				*rval = BOOLEAN_TO_JSVAL((bool)value);
				break;
			case TypeCode::Byte:
			case TypeCode::SByte:
			case TypeCode::Int16:
			case TypeCode::UInt16:
			case TypeCode::Single:
			case TypeCode::Int32:
			case TypeCode::UInt32:
			case TypeCode::Int64:
			case TypeCode::UInt64:
			case TypeCode::Double:
			case TypeCode::Decimal:
				if (!JS_NewNumberValue(script->Context, Convert::ToDouble(value), rval))
					*rval = JSVAL_VOID;

				break;
			case TypeCode::Char:
			case TypeCode::String:
				{
					char* pstr = ToUnmanagedStringInternal(Convert::ToString(value));

					JSString* str = JS_NewStringCopyZ(script->Context, pstr);

					if (!str) 
						*rval = JSVAL_VOID;
					else
						*rval = STRING_TO_JSVAL(str);

					Marshal::FreeHGlobal(IntPtr(pstr));
					break;
				}
			case TypeCode::DateTime:
			case TypeCode::Object:
				{
					if (type == SMFunction::typeid) {
						SMFunction^ smfunc = (SMFunction^)value;
						*rval = smfunc->funcval; return;
					}

					SMPrototype^ proto = script->mRuntime->FindProto(type);

					if (proto == nullptr && script->mRuntime->autoEmbed) {
						script->mRuntime->Embed(type, proto);
					}

					if (proto != nullptr) {
						*rval = OBJECT_TO_JSVAL(proto->CreateInstance(script, value));
						break;
					}
				}
			default: *rval = JSVAL_VOID;
		}
	}
}


Object^ Interop::FromJSVal(SMScript^ script, jsval value)
{
	try {
		script->mContext->Lock();

		JSType t = JS_TypeOfValue(script->Context, value);

		switch(t) 
		{
			case JSTYPE_NULL:
			case JSTYPE_VOID:
				return nullptr;

			case JSTYPE_BOOLEAN:
				return (bool)JSVAL_TO_BOOLEAN(value);
			case JSTYPE_NUMBER:
				{
					if (JSVAL_IS_INT(value))
						return (int)JSVAL_TO_INT(value);

					else if (JSVAL_IS_DOUBLE(value))
						return (double)JSVAL_TO_DOUBLE(value);
					
					else return nullptr;
				}
			case JSTYPE_OBJECT:
				{
					JSObject* rval = JSVAL_TO_OBJECT(value);
					if (!rval) return nullptr;

					if (JS_ObjectIsDate(script->Context, rval)) {
						double dbl = (double)js_DateGetMsecSinceEpoch(script->Context, rval);

						DateTime date = DateTime(1970, 1, 1);
						date.AddMilliseconds(dbl);

						return date.ToLocalTime();
					}
					else if (JS_IsArrayObject(script->Context, rval)) {
						jsuint len;
						JS_GetArrayLength(script->Context, rval, &len);

						array<Object^>^ ret = gcnew array<Object^>(len);

						jsval element;
						for(int i = 0; i < (int)len; i++) {
							JS_GetElement(script->Context, rval, i, &element);
							ret[i] = FromJSVal(script, element);
						}

						return ret;
					}
					else {
						SMInstance^ instance = script->mRuntime->FindInstance(rval);

						if (instance != nullptr) 
							return instance->instance;
					}

					return FromJSValToManagedString(script->Context, value);
				}
			case JSTYPE_FUNCTION:
				{
					JS_ConvertValue(script->Context, value, JSTYPE_FUNCTION, &value);
					if (!value) return nullptr;

					return gcnew SMFunction(script, value);
				}
			default: //JS.JSType.JSTYPE_STRING
				return FromJSValToManagedString(script->Context, value);
		}
		return nullptr;
	}
	finally { script->mContext->Unlock(); }
}


String^ Interop::FromJSValToManagedString(JSContext* cx, jsval value)
{
	JSString* str = JS_ValueToString(cx, value);
	char* bytes = JS_EncodeString(cx, str);

	String^ ret = Interop::ToManagedStringInternal(bytes);

	JS_free(cx, bytes);
	return ret;
}

char* Interop::FromJSValToUnmanagedString(JSContext* cx, jsval value)
{
	JSString* str = JS_ValueToString(cx, value);
	return JS_EncodeString(cx, str);
}


bool Interop::ChangeType(SMScript^ script, Object^ %value, Type^ typeTo)
{
	if (value == nullptr) 
    {
        if (typeTo->IsValueType)
			return false;

		else return true;
    }

    Type^ typeFrom = value->GetType();

    // most embedded classes / implicit casts / to-interface casts
    if (typeTo->IsAssignableFrom(typeFrom))
        return true;

    else if (typeTo->IsEnum)
    {
        if (!ChangeType(script, value, Enum::GetUnderlyingType(typeTo)))
            return false;

        value = Enum::ToObject(typeTo, value);
        return true;
    }
    else if (typeFrom->IsEnum) 
        return false;

    else if (typeFrom->IsArray && typeTo->IsArray)
    {
        Type^ eltype = typeTo->GetElementType();

        Array^ v1 = (Array^)value;
        Array^ v2 = Array::CreateInstance(eltype, v1->Length);

        for (int i = 0; i < v1->Length; i++)
        {
            Object^ tmp = v1->GetValue(i);

            if (!ChangeType(script, tmp, eltype))
                return false;

            v2->SetValue(tmp, i);
        }
        value = v2;
        return true;
    }
    else
    {
		/* Since this is (almost) always called on the return of FromJSVal
		*
        * We can assume 'value' is:
        * null (handled at top)
        * bool
        * int
        * double
        * string
        * object (variable runtime types)
        *
        * This following conversion routine is a speed optomization.
        * I was previously using:
        * 
        * if (typeof(IConvertible).IsAssignableFrom(typeFrom) &&
        *     typeof(IConvertible).IsAssignableFrom(typeTo)) {
        *  
        *      try
        *      {
        *          value = Convert.ChangeType(value, typeTo);
        *          return true;
        *      }
        *      catch { return false; }
        * }
        * 
        * And this worked almost flawlessly.
        * 
        * However, the overhead cost of throwing an exception for failed conversions 
		* (example: converting an int whose value was > 255 to a byte) 
        * was too great and slowed down the evaluations of conversions executed on a loop (ex. from a javascript for/while loop)
        * 
        */
        Type^ typeToConv = typeTo;

		bool ret = false;

        if (typeToConv->IsByRef || typeToConv->IsPointer) {
			if (typeToConv->HasElementType)
				typeToConv = typeTo->GetElementType();

			else return ret;
		}

        try
        {
            if (typeFrom == Boolean::typeid)
            {
                switch (Type::GetTypeCode(typeToConv))
                {
					case TypeCode::DateTime:
                        return ret;
					case TypeCode::Object:
                        if (typeToConv != Object::typeid)
                            return ret;

                        break;
                }

                value = Convert::ChangeType(value, typeToConv);

				ret = true;
                return ret;
            }
            else if (typeFrom == int::typeid)
            {
                int v = (int)value;

                switch (Type::GetTypeCode(typeToConv))
                {
					case TypeCode::Byte:
                        if (v < Byte::MinValue || v > Byte::MaxValue)
                            return ret;

                        break;
                    case TypeCode::SByte:
                        if (v < SByte::MinValue || v > SByte::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Char:
                        if (v < Char::MinValue || v > Char::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Int16:
                        if (v < Int16::MinValue || v > Int16::MaxValue)
                            return ret;

                        break;
                    case TypeCode::UInt16:
                        if (v < UInt16::MinValue || v > UInt16::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Single:
                        if (v < Single::MinValue || v > Single::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Object:
                        if (typeToConv != Object::typeid)
                            return ret;

                        break;
					case TypeCode::DateTime:
                        return ret;
                }

                value = Convert::ChangeType(value, typeToConv);
                return true;
            }
            else if (typeFrom == Double::typeid)
            {
                double v = (double)value;

                switch (Type::GetTypeCode(typeToConv))
                {
					case TypeCode::Byte:
                        if (v < Byte::MinValue || v > Byte::MaxValue)
                            return ret;

                        break;
                    case TypeCode::SByte:
                        if (v < SByte::MinValue || v > SByte::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Char:
                        if (v < Char::MinValue || v > Char::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Int16:
                        if (v < Int16::MinValue || v > Int16::MaxValue)
                            return ret;

                        break;
                    case TypeCode::UInt16:
                        if (v < UInt16::MinValue || v > UInt16::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Single:
                        if (v < Single::MinValue || v > Single::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Int32:
                        if (v < Int32::MinValue || v > Int32::MaxValue)
                            return ret;

                        break;
                    case TypeCode::UInt32:
                        if (v < UInt32::MinValue || v > UInt32::MaxValue)
                            return ret;

                        break;
                    case TypeCode::Object:
                        if (typeToConv != Object::typeid)
                            return ret;

                        break;
					case TypeCode::DateTime:
                        return ret;
                }

                value = Convert::ChangeType(value, typeToConv);

				ret = true;
                return ret;
            }
            else if (typeFrom == String::typeid)
            {
                String^ v = (String^)value;

                switch (Type::GetTypeCode(typeToConv))
                {
					case TypeCode::Byte:
                        {
                            Byte b = 0;

                            if (Byte::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::SByte:
                        {
                            SByte b = 0;

                            if (SByte::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            return ret;
                        }
                    case TypeCode::Char:
                        {
                            if (v->Length > 0)
                                value = Convert::ToChar(v->Substring(0, 1));

							ret = true;
                            return ret;
                        }
                    case TypeCode::Int16:
                        {
                            Int16 b = 0;

                            if (Int16::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::UInt16:
                        {
                            UInt16 b = 0;

                            if (UInt16::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Single:
                        {
                            Single b = 0;

                            if (Single::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Int32:
                        {
                            int b = 0;

                            if (Int32::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::UInt32:
                        {
                            UInt32 b = 0;

                            if (UInt32::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Int64:
                        {
                            Int64 b = 0;

                            if (Int64::TryParse(v, b))
                            {
                                value = b;
                                
								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::UInt64:
                        {
                            UInt64 b = 0;

                            if (UInt64::TryParse(v, b))
                            {
                                value = b;

								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Double:
                        {
                            double b = 0;

                            if (Double::TryParse(v, b))
                            {
                                value = b;
                                
								ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Decimal:
                        {
                            Decimal b = 0;

                            if (Decimal::TryParse(v, b))
                            {
                                value = b;

                                ret = true;
                                return ret;
                            }
                            else return ret;
                        }
                    case TypeCode::Object:
                        if (typeToConv != Object::typeid)
                            return ret;

                        break;
                }

                value = Convert::ChangeType(value, typeToConv);
                
				ret = true;
                return ret;
            }
			else if (typeFrom == SMFunction::typeid) 
			{
				switch (Type::GetTypeCode(typeToConv))
				{
					case TypeCode::Byte:
                    case TypeCode::SByte:
                    case TypeCode::Char:
                    case TypeCode::Int16:
                    case TypeCode::UInt16:
                    case TypeCode::Single:
                    case TypeCode::Int32:
                    case TypeCode::UInt32:
                    case TypeCode::Int64:
                    case TypeCode::UInt64:
                    case TypeCode::Double:
                    case TypeCode::Decimal:
					case TypeCode::DateTime:
						return ret;
					case TypeCode::String:
						{
							jsval rval;
							ToJSVal(script, value, &rval);

							value = FromJSValToManagedString(script->Context, rval);
							
							ret = true;
                            return ret;
						}
					case TypeCode::Object:
						{
							if (TypeInfo::IsDelegate(typeToConv) &&
								TypeInfo::InheritsFrom<SMFunction^>(value->GetType())) {

								value = SMBinder::BindToDelegate(script, typeToConv, (SMFunction^)value);
								
								if (value != nullptr) {
									ret = true;
									return ret;
								}
							}
							else if (typeToConv != Object::typeid)
								return ret;
						}
				}

				value = Convert::ChangeType(value, typeToConv);
                
				ret = true;
                return ret;
			}
            else
            {
                switch (Type::GetTypeCode(typeToConv))
                {
					case TypeCode::String:
						{
							jsval rval;
							ToJSVal(script, value, &rval);

							value = FromJSValToManagedString(script->Context, rval);
							
							ret = true;
                            return ret;
						}
                    case TypeCode::Byte:
                    case TypeCode::SByte:
                    case TypeCode::Char:
                    case TypeCode::Int16:
                    case TypeCode::UInt16:
                    case TypeCode::Single:
                    case TypeCode::Int32:
                    case TypeCode::UInt32:
                    case TypeCode::Int64:
                    case TypeCode::UInt64:
                    case TypeCode::Double:
                    case TypeCode::Decimal:
                        value = Convert::ChangeType(value, typeToConv);

                        ret = true;
                        return ret;
                }
            }
        }
        catch(...) { return ret; }
    }

    return false;
}