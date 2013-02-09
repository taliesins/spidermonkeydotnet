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
		String^ name;
		bool enumerate;
		
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

