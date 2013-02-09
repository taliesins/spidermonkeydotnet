#pragma once
#include "stdafx.h"

using namespace System;

namespace smnetjs {
	
	ref class SMScript;
	ref class SMPrototype;

	public ref class SMFunction sealed
	{
	internal:

		jsval funcval;
		bool released;

		SMScript^ script;
		
		int currInstance;
		static int instanceCount;
	public:
		property String^ Name {
			String^ get() {
				return this->GetName();
			}
		}

		property SMScript^ Script {
			SMScript^ get() { 
				return this->script;
			}
		}

		Object^ Invoke(... array<Object^>^ args);

		virtual String^ ToString() override;
		String^ ToString(bool AsJSVal);

		~SMFunction() { this->!SMFunction(); }
	internal:
		SMFunction(SMScript^ script, jsval funcval);

		String^ GetName();
	private:
		!SMFunction();
	};
}