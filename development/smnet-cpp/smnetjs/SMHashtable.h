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

using namespace System;
using namespace System::Collections;

namespace smnetjs {

	generic<typename TKey, typename TValue>
	ref class SMHashtable : public Collections::IEnumerable
	{
	private:
		Hashtable^ baseTable;
	public:
		SMHashtable() {
			this->baseTable = gcnew Hashtable();
		}

		property int Count {
			int get() { return this->baseTable->Count; }
		}

		property TValue default[TKey] {
			TValue get(TKey key) {
				Object^ ret = this->baseTable[key];
				if (ret != nullptr)
					return (TValue)ret;

				return TValue();
			}
			void set(TKey key, TValue value) {
				this->baseTable[key] = value;
			}
		}

		void Clear() {
			this->baseTable->Clear();
		}
	internal:
		void Add(TKey key, TValue value) {
			this->baseTable->Add(key, value);
		}
		void Remove(TKey key) {
			this->baseTable->Remove(key);
		}
	public:
		bool ContainsKey(TKey key) {
			return this->baseTable->ContainsKey(key);
		}

		bool ContainsValue(TValue value) {
			return this->baseTable->ContainsValue(value);
		}

		virtual System::Collections::IEnumerator^ GetEnumerator() {
			return this->baseTable->GetEnumerator();
		}
	};

}