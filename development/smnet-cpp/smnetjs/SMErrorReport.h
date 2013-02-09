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
#include "Interop.h"

using namespace System::Text;

namespace smnetjs {

	[Flags]
	public enum struct SMErrorType 
		: unsigned int
	{
		Error = 0,
		Warning = 1,
		Exception = 2,
		Strict = 4,
	};

	public ref class SMErrorReport sealed
	{
	internal:
		int lineno;

		String^ token;
		String^ source;
		String^ message;
		String^ filename;

		SMErrorType error;

		SMErrorReport(const char* message, JSErrorReport* report) {
			this->lineno = report->lineno;
			this->error = (SMErrorType)report->flags;

			this->message = Interop::ToManagedStringInternal(message);

			if (report->tokenptr)
				this->token = Interop::ToManagedStringInternal(report->tokenptr);

			if (report->linebuf)
				this->source = Interop::ToManagedStringInternal(report->linebuf);
			
			if (report->filename)
				this->filename = Interop::ToManagedStringInternal(report->filename);
		}
	public:
		property String^ Filename {
			String^ get() {
				return this->filename;
			}
		}

		property String^ Message {
			String^ get() {
				return this->message;
			}
		}

		property int LineNumber {
			int get() {
				return this->lineno;
			}
		}

		property String^ LineSource {
			String^ get() {
				return this->source;
			}
		}

		property String^ Token {
			String^ get() {
				return this->token;
			}
		}

		property SMErrorType ErrorType {
			SMErrorType get() {
				return this->error;
			}
		}
	};

}