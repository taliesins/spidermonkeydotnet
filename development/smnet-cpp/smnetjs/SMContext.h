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
using namespace System::Threading;

namespace smnetjs {

	ref class SMRuntime;
	ref class SMPrototype;

	ref class SMContext
	{
	private:
		int lockDepth;

		Thread^ currThread;
		PRMonitor* monitor;
		SMRuntime^ runtime;
	public:
		JSContext* context;
		JSObject* globalobj;
		
		SMContext(SMRuntime^ runtime);
		SMContext(SMRuntime^ runtime, SMPrototype^ proto);

		~SMContext() { this->!SMContext(); }

		void Lock();
		void Unlock();

		JSObject* GetGlobalObject();
	private:
		bool ShouldLock();
		bool ShouldUnlock();
	
		!SMContext();
	};

}