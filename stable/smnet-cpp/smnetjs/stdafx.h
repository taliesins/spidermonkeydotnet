#pragma once
#pragma unmanaged

#define XP_WIN

#include "jsapi.h"
#include "jslock.h"

#ifdef JS_THREADSAFE
# include "prmon.h"
#endif

static JSClass global_class = {
	"Global",
	JSCLASS_GLOBAL_FLAGS | JSCLASS_NEW_RESOLVE | JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_PropertyStub,
	JS_StrictPropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

#pragma managed