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