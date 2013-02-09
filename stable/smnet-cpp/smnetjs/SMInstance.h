#pragma once

namespace smnetjs {

	ref class SMScript;

	ref class SMInstance
	{
	internal:
		int lockCount;

		SMScript^ script;
		Object^ instance;

		JSObject* jsobj;
	public:
		SMInstance(SMScript^ script, JSObject* jsobj, Object^ instance);
		~SMInstance() { this->!SMInstance(); }

		void GCLock();
		void GCUnlock(bool force);
	private:
		void OnScriptDisposed(Object^ sender, EventArgs^ e);

		!SMInstance();
	};

}