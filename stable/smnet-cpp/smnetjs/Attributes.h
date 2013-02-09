#pragma once
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
using namespace System;

namespace smnetjs {

	[AttributeUsage(
        AttributeTargets::Constructor |
        AttributeTargets::Field |
        AttributeTargets::Property |
        AttributeTargets::Method |
        AttributeTargets::Event |
        AttributeTargets::Delegate,
        AllowMultiple = false,
        Inherited = false)]
	public ref class SMIgnore sealed : Attribute 
	{
	public:
		SMIgnore() { }
	};

	[AttributeUsage(
        AttributeTargets::Class | 
		AttributeTargets::Struct, 
        AllowMultiple = false,
        Inherited = false)]
	public ref class SMEmbedded sealed : Attribute
	{
	private:
		bool indexed;
		bool inherit;
		bool dispose;

		String^ name;
		String^ aname;
	public:
		SMEmbedded() { 
			this->inherit = true;
			this->dispose = true;
		}

		property String^ Name {
			String^ get() { 
				return this->name;
			}
			void set(String^ value) { 
				this->name = value; 
			}
		}

		property String^ AccessibleName {
			String^ get() {
				return this->aname;
			}
			void set(String^ value) {
				this->aname = value;
			}
		}
		/*
		property bool Indexed {
			bool get() {
				return this->indexed;
			}
			void set(bool value) {
				this->indexed = value;
			}
		}
		*/
		property bool AllowScriptDispose {
			bool get() {
				return this->dispose;
			}
			void set(bool value) {
				this->dispose = value;
			}
		}

		property bool AllowInheritedMembers {
			bool get() {
				return this->inherit;
			}
			void set(bool value) {
				this->inherit = value;
			}
		}
	};

	[AttributeUsage(
        AttributeTargets::Constructor,
        AllowMultiple = false,
        Inherited = false)]
	public ref class SMConstructor sealed : Attribute
	{
	private:
		bool script;
		bool object;
	public:
		SMConstructor() {}

		property bool Script {
			bool get() {
				return this->script;
			}
			void set(bool value) {
				this->script = value;
			}
		}
	};

	[AttributeUsage(
        AttributeTargets::Method,
        AllowMultiple = false,
        Inherited = false)]
	public ref class SMMethod sealed : Attribute
	{
	private:
		bool script;
		bool object;
		bool enumerate;
		bool overridable;

		String^ name;
	public:
		SMMethod() {
			this->enumerate = true;
		}

		property String^ Name {
			String^ get() {
				return this->name;
			}
			void set(String^ value) {
				this->name = value;
			}
		}

		property bool Script {
			bool get() {
				return this->script;
			}
			void set(bool value) {
				this->script = value;
			}
		}

		property bool Enumerate {
			bool get() {
				return this->enumerate;
			}
			void set(bool value) {
				this->enumerate = value;
			}
		}

		property bool Overridable {
			bool get() { 
				return this->overridable;
			}
			void set(bool value) {
				this->overridable = value;
			}
		}
	};

	[AttributeUsage(
        AttributeTargets::Field | 
		AttributeTargets::Property |
		AttributeTargets::Event,
        AllowMultiple = false,
        Inherited = false)]
	public ref class SMProperty sealed : Attribute 
	{
	private:
		bool enumerate;
		String^ name;
	public:
		SMProperty() { 
			this->enumerate = true;
		}

		property String^ Name {
			String^ get() {
				return this->name;
			}
			void set(String^ value) {
				this->name = value;
			}
		}

		property bool Enumerate {
			bool get() {
				return this->enumerate;
			}
			void set(bool value) {
				this->enumerate = value;
			}
		}
	};

}

