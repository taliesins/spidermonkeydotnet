#pragma once

using namespace System;
using namespace System::Collections;
using namespace System::Runtime::CompilerServices;

namespace smnetjs {

	[Extension]
	public ref class CollectionExtensions abstract sealed
	{
	public:
		generic<typename T> [Extension] static T Find(Generic::IEnumerable<T>^ list, Predicate<T>^ predicate) {
			for each(T t in list)
				if (predicate(t)) return t;

			return T();
		}
	};

}