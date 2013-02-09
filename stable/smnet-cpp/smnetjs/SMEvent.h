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
#include "Attributes.h"
#include "SMBinder.h"
#include "SMScript.h"
#include "SMFunction.h"

using namespace System;
using namespace System::Reflection;
using namespace System::Collections::Generic;

namespace smnetjs {

	[SMEmbedded(AllowInheritedMembers = false)]
	ref class SMEventHandler 
	{
	private:
		SMScript^ script;
		SMFunction^ func;

		System::Delegate^ del;
	internal:
		SMEventHandler(SMScript^ script, System::Delegate^ del, SMFunction^ func) {
			this->del = del;
			this->func = func;
			this->script = script;
		}

		property SMScript^ Script {
			SMScript^ get() {
				return this->script;
			}
		}
	public:
		[SMProperty(Name = "function")]
		property SMFunction^ Function {
			SMFunction^ get() {
				return this->func;
			}
		}
		[SMProperty(Name = "delegate")]
		property System::Delegate^ Delegate {
			System::Delegate^ get() {
				return this->del;
			}
		}
	};

	[SMEmbedded(AllowInheritedMembers = false)]
	ref class SMEvent
	{
	internal:
		bool disposed;

		Object^ parent;
		EventInfo^ eventInfo;

		List<SMEventHandler^>^ delegates;

		property Object^ Parent {
			Object^ get() {
				return this->parent;
			}
		}

		property EventInfo^ Event {
			EventInfo^ get() {
				return this->eventInfo;
			}
		}

		property List<SMEventHandler^>^ Delegates {
			List<SMEventHandler^>^ get() {
				return this->delegates;
			}
		}

		SMEvent(Object^ parent, EventInfo^ _event) {
			this->parent = parent;
			this->eventInfo = _event;

			this->delegates = gcnew List<SMEventHandler^>();
		}

	public:

		[SMProperty(Name = "handlerType")]
		property String^ HandlerType {
			String^ get() {
				return this->eventInfo->EventHandlerType->Name;
			}
		}

		[SMProperty(Name = "handlers")]
		property array<SMEventHandler^>^ Handlers {
			array<SMEventHandler^>^ get() {
				return this->delegates->ToArray();
			}
		}

		[SMMethod(Name = "addHandler", Script = true)]
		bool AddHandler(SMScript^ script, SMFunction^ func) {

			if (func == nullptr)
				return false;

			Delegate^ del = SMBinder::BindToDelegate(
				script->mRuntime->FindProto(eventInfo->EventHandlerType), script, func);

			SMEventHandler^ handler = gcnew SMEventHandler(script, del, func);

			//GC lock the instance
			script->mContext->Lock();
			script->CreateObject(handler);
			script->mContext->Unlock();

			this->eventInfo->AddEventHandler(parent, del);
			this->delegates->Add(handler);

			return true;
		}


		[SMMethod(Name = "removeHandler")]
		void RemoveHandler(SMFunction^ func) {

			for (int i = 0; i < delegates->Count; i++) {
				SMEventHandler^ handler = delegates[i];

				if (handler->Function->funcval == func->funcval) {
					//GC unlock the instance

					handler->Script->mContext->Lock();
					handler->Script->DestroyObject(handler);
					handler->Script->mContext->Unlock();

					this->eventInfo->RemoveEventHandler(parent, handler->Delegate);
					this->delegates->RemoveAt(i);

					break;
				}
			}
		}

		[SMMethod(Name = "removeHandler")]
		void RemoveHandler(SMEventHandler^ handler) {

			if (handler == nullptr)
				return;

			//GC unlock the instance
			handler->Script->mContext->Lock();
			handler->Script->DestroyObject(handler);
			handler->Script->mContext->Unlock();

			this->eventInfo->RemoveEventHandler(parent, handler->Delegate);
			this->delegates->Remove(handler);
		}

		[SMIgnore]
		void Release() {
			if (!disposed) {
				disposed = true;

				this->!SMEvent();

				GC::SuppressFinalize(this);
			}
		}
	private:
		!SMEvent() {
			for (int i = 0; i < delegates->Count; i++) {
				SMEventHandler^ handler = delegates[i];

				//GC unlock the instance
				handler->Script->mContext->Lock();
				handler->Script->DestroyObject(handler);
				handler->Script->mContext->Unlock();

				this->eventInfo->RemoveEventHandler(parent, handler->Delegate);
			}
			this->delegates->Clear();
		}
	};

}