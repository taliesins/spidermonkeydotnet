using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

using smnetjs;
using System.Runtime.InteropServices;

namespace smnet_demo.Javascript
{
    [SMEmbedded(AllowInheritedMembers = false)]
    class DynamicObject : ISMDynamic
    {
        string Name = "WHAT";

        public object OnPropertyGetter(SMScript script, string name) {

            if (name == "name")
                return Name;

            return null;
        }

        public void OnPropertySetter(SMScript script, string name, object value) {

            if (name == "name")
                Name = (string)value;
        }

        [DllImport("mozjs185-1.0.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern void JS_GetContextPrivate(IntPtr cx);
    }
}
