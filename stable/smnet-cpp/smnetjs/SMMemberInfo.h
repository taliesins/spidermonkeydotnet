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
namespace smnetjs {

	ref class SMMethod;
	ref class SMProperty;
	ref class SMConstructor;

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

	ref class SMMemberInfo
	{
	private:
		SMProperty^ attribute;
		System::Reflection::MemberInfo^ memberinfo;
	public:
		SMMemberInfo(System::Reflection::MemberInfo^ memberinfo) {
			this->memberinfo = memberinfo;
		}

		SMMemberInfo(SMProperty^ attribute, System::Reflection::MemberInfo^ memberinfo) {
			this->attribute = attribute;
			this->memberinfo = memberinfo;
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