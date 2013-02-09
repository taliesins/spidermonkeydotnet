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
#include "SMBinder.h"
#include "SMScript.h"
#include "SMFunction.h"

using namespace System;
using namespace System::Text;
using namespace System::Reflection;
using namespace System::Collections::Generic;

namespace smnetjs {

	[SMEmbedded(AllowInheritedMembers = false)]
	ref class SMEventHandler 
	{
	private:
		SMFunction^ func;
		System::Delegate^ del;

	internal:
		SMEventHandler(SMFunction^ func, System::Delegate^ del) {
			this->del = del;
			this->func = func;
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
		JSObject* jsparent;

		SMScript^ script;

		EventInfo^ eventInfo;

		List<SMEventHandler^>^ delegates;

		property Object^ Parent {
			Object^ get() {
				return this->parent;
			}
		}

		property JSObject* JSParent {
			JSObject* get() {
				return this->jsparent;
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

		SMEvent(SMScript^ script, JSObject* jsparent, Object^ parent, EventInfo^ _event) {

			this->script = script;
			this->parent = parent;
			this->jsparent = jsparent;
			this->eventInfo = _event;

			this->delegates = gcnew List<SMEventHandler^>();
		}

	public:

		[SMProperty(Name = "handlers")]
		property array<SMEventHandler^>^ Handlers {
			array<SMEventHandler^>^ get() {
				return this->delegates->ToArray();
			}
		}

		[SMProperty(Name = "handlerType")]
		property String^ HandlerType {
			String^ get() {
				SMPrototype^ proto = script->mRuntime->FindProto(eventInfo->EventHandlerType);

				if (proto != nullptr)//for sanity
					return proto->Name;

				return eventInfo->EventHandlerType->Name;
			}
		}

		[SMProperty(Name = "handlerSig")]
		property String^ HandlerSignature {
			String^ get() {
				MethodInfo^ invoke = eventInfo->EventHandlerType->GetMethod("Invoke");

				StringBuilder^ sb = gcnew StringBuilder();

				if (invoke->ReturnType != Void::typeid)
					sb->Append(invoke->ReturnType);

				sb->Append("(");

				array<ParameterInfo^>^ params = invoke->GetParameters();
				for (int i = 0; i < params->Length; i++) {

					ParameterInfo^ pinfo = params[i];
					SMPrototype^ proto = script->mRuntime->FindProto(pinfo->ParameterType);

					if (proto != nullptr)
						sb->Append(proto->Name);
					else 
						sb->Append(pinfo->ParameterType->Name);

					sb->Append(", ");
				}

				if (params->Length > 0)
					sb->Remove(sb->Length - 2, 2);

				sb->Append(")");

				return sb->ToString();
			}
		}

		[SMMethod(Name = "addHandler")]
		bool AddHandler(SMFunction^ func) {

			if (func == nullptr)
				return false;

			script->mContext->Lock();

			SMEventHandler^ handler = gcnew SMEventHandler(
				func,
				SMBinder::BindToDelegate(script, eventInfo->EventHandlerType, func));

			script->CreateObject(handler);
			script->mContext->Unlock();

			this->eventInfo->AddEventHandler(parent, handler->Delegate);
			this->delegates->Add(handler);

			return true;
		}


		[SMMethod(Name = "removeHandler")]
		void RemoveHandler(SMFunction^ func) {

			for (int i = 0; i < delegates->Count; i++) {
				SMEventHandler^ handler = delegates[i];

				if (handler->Function->funcval == func->funcval) {
					//GC unlock the instance

					script->mContext->Lock();
					script->DestroyObject(handler);
					script->mContext->Unlock();

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
			script->mContext->Lock();
			script->DestroyObject(handler);
			script->mContext->Unlock();

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
				script->mContext->Lock();
				script->DestroyObject(handler);
				script->mContext->Unlock();

				this->eventInfo->RemoveEventHandler(parent, handler->Delegate);
			}
			this->delegates->Clear();
		}
	};

}