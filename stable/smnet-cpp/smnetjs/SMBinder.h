#pragma once
#include "stdafx.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Linq::Expressions;
using namespace System::Collections::Generic;

namespace smnetjs {

	ref class SMScript;
	ref class SMPrototype;
	ref class SMFunction;
	ref class SMMethodInfo;
	ref class SMConstructorInfo;
	
	ref class SMBinder abstract sealed
	{
	public:
		static Delegate^ BindToDelegate(SMPrototype^ proto, SMScript^ script, SMFunction^ func);

		static SMConstructorInfo^ BindToConstructor(SMScript^ script, List<SMConstructorInfo^>^ ctors, array<Object^>^ %args);
		static SMMethodInfo^ BindToMethod(SMScript^ script, JSObject* obj, List<SMMethodInfo^>^ funcs, array<Object^>^ %args);

		static array<ParameterExpression^>^ CreateArguments(array<ParameterInfo^>^ parameters);

		static array<Object^>^ ConvertArguments(SMScript^ script, SMConstructorInfo^ construcor, array<Object^>^ args, bool _explicit);
		static array<Object^>^ ConvertArguments(SMScript^ script, JSObject* obj, SMMethodInfo^ method, array<Object^>^ args, bool _explicit);
		
		static bool ConvertArgument(SMScript^ script, Object^ %item, System::Type^ type, bool _explicit);

		static Object^ CallCLRFunction(SMScript^ script, IntPtr fobj, String^ name, array<Object^>^ args);
		static Object^ CallLambdaFunction(SMScript^ script, jsval funcval, array<Object^>^ args);

		static UnaryExpression^ SelectArguments(ParameterExpression^ s);
	};

}