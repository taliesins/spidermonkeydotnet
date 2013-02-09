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
#include "SMScript.h"
#include "SMInstance.h"

using namespace System;
using namespace System::Collections;
using namespace System::Runtime::InteropServices;

namespace smnetjs {

	[StructLayout(LayoutKind::Sequential)]
	ref class SMEnumerator
	{
	private:
		int index;

		JSObject* iterator;
		Collections::IEnumerator^ enumerator;
	public:
		property int Index {
			int get() {
				return this->index;
			}
			void set(int value) {
				this->index = value;
			}
		}

		property Object^ Current {
			Object^ get() {
				return this->enumerator->Current;
			}
		}

		property JSObject* Iterator {
			JSObject* get() {
				return this->iterator;
			}
		}

		SMEnumerator(SMInstance^ instance) {
			this->iterator = JS_NewPropertyIterator(instance->script->Context, instance->jsobj);
			this->enumerator = ((Collections::IEnumerable^)instance->instance)->GetEnumerator();
		}

		~SMEnumerator() { this->!SMEnumerator(); }

		bool MoveNext() {
			this->index++;
			return this->enumerator->MoveNext();
		}

		void Reset() {
			this->index = 0;
			this->enumerator->Reset();
		}
	private:
		!SMEnumerator() {
			this->iterator = NULL;
			this->enumerator = nullptr;
		}
	};

}