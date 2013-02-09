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

#include "Attributes.h"

namespace smnetjs {

	ref class SMMethodInfo
	{
	private:
		SMMethod^ attribute;
		System::Reflection::MethodInfo^ methodinfo;
	public:
		SMMethodInfo(System::Reflection::MethodInfo^ methodinfo) {
			this->attribute = attribute;
			this->methodinfo = methodinfo;
		}

		SMMethodInfo(SMMethod^ attribute, System::Reflection::MethodInfo^ methodinfo) {
			this->attribute = attribute;
			this->methodinfo = methodinfo;
		}

		property String^ Name {
			String^ get() {
				if (!String::IsNullOrEmpty(attribute->Name))
					return attribute->Name;

				return methodinfo->Name;
			}
		}

		property SMMethod^ Attribute {
			SMMethod^ get() {
				return this->attribute;
			}
		}

		property System::Reflection::MethodInfo^ Method {
			System::Reflection::MethodInfo^ get() {
				return this->methodinfo;
			}
		}
	};

	ref class SMPropertyInfo
	{
	private:
		SMProperty^ attribute;
		System::Reflection::MemberInfo^ memberinfo;
	public:
		SMPropertyInfo(System::Reflection::MemberInfo^ memberinfo) {
			this->memberinfo = memberinfo;
		}

		SMPropertyInfo(SMProperty^ attribute, System::Reflection::MemberInfo^ memberinfo) {
			this->attribute = attribute;
			this->memberinfo = memberinfo;
		}

		property String^ Name {
			String^ get() {
				if (!String::IsNullOrEmpty(attribute->Name))
					return attribute->Name;

				return memberinfo->Name;
			}
		}

		property SMProperty^ Attribute {
			SMProperty^ get() {
				return this->attribute;
			}
		}

		property System::Reflection::MemberInfo^ Member {
			System::Reflection::MemberInfo^ get() {
				return this->memberinfo;
			}
		}
	};

	ref class SMConstructorInfo
	{
	private:
		SMConstructor^ attribute;
		System::Reflection::ConstructorInfo^ constructorinfo;
	public:
		SMConstructorInfo(System::Reflection::ConstructorInfo^ constructorinfo) {
			this->constructorinfo = constructorinfo;
		}

		SMConstructorInfo(SMConstructor^ attribute, System::Reflection::ConstructorInfo^ constructorinfo) {
			this->attribute = attribute;
			this->constructorinfo = constructorinfo;
		}

		property SMConstructor^ Attribute {
			SMConstructor^ get() {
				return this->attribute;
			}
		}

		property System::Reflection::ConstructorInfo^ Method {
			System::Reflection::ConstructorInfo^ get() {
				return this->constructorinfo;
			}
		}
	};

}