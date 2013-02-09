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
using System;
using System.Threading;
using System.Collections.Generic;
using System.Text.RegularExpressions;
using System.Runtime.InteropServices;

using System.Net;
using System.Net.Sockets;

using smnetjs;
using smnet_demo.Javascript;
using System.Reflection.Emit;

namespace smnet_demo
{
    class Program
    {
        [SMIgnore]
        public static SMRuntime SMRuntime {
            get;
            private set;
        }

        //Example using custom JS API functions from outside the wrapper
        [SMIgnore]
        [DllImport("mozjs185-1.0.dll", CallingConvention = CallingConvention.Cdecl)]
        public static extern bool JS_SetProperty(IntPtr cx, IntPtr obj, string name, IntPtr value);


        static void Main(string[] args) {
            SMRuntime = new SMRuntime(RuntimeOptions.ForceGCOnContextDestroy);

            SMRuntime.Embed(typeof(Script));
            SMRuntime.Embed(typeof(JSHttpRequest));
            SMRuntime.Embed(typeof(TestDelegate));
            SMRuntime.Embed(typeof(IPEndPoint));
            SMRuntime.Embed(typeof(DynamicObject));
            SMRuntime.Embed(typeof(Socket), new SMEmbedded() { AllowScriptDispose = true });
            SMRuntime.Embed(typeof(SocketAsyncEventArgs), new SMEmbedded() { AllowScriptDispose = true });

            SMRuntime.Embed(typeof(List<>));
            SMRuntime.Embed(typeof(Dictionary<,>));

            SMRuntime.OnScriptError += new ScriptErrorHandler(
                (scr, report) => {
                    Console.WriteLine("{0} in {1} on line {2}", report.Message, scr.Name, report.LineNumber);
                }
            );

            SMScript script = SMRuntime.InitScript("eval", typeof(Program));

            //example 1
            script.SetGlobalProperty("expando1", 1234);
            
            //example 2 (using P/Invoke)
            //Because contexts need to locked for thread-safety when accessing the API
            //SMScript::Invoke wraps the delegate call with the appropriate context locking logic
            script.Invoke(() => {
                IntPtr jsval = Marshal.AllocHGlobal(Marshal.SizeOf(typeof(IntPtr)));
                Interop.ToJSVal(script, new Action(() => Console.WriteLine("Worked!")), jsval);

                JS_SetProperty(script.ContextPtr, script.GlobalPtr, "expando2", jsval);

                Marshal.FreeHGlobal(jsval);
            });

            Console.Write("js> ");

            while (true) {
                
                string text = Console.ReadLine();
                if (text.ToLower() == "exit") break;

                ThreadPool.QueueUserWorkItem((state) => {
                    DoEval(new object[] { script, text });
                    Console.Write("js> ");
                });
            }

            script.Dispose();
            SMRuntime.Dispose();
        }

        static void DoEval(object state) {
            SMScript script = (SMScript)((object[])state)[0];
            String text = (String)((object[])state)[1];

            text = script.Eval<string>(text);

            if (!String.IsNullOrEmpty(text))
                Console.WriteLine(text);

            script.GarbageCollect();
        }

        [SMMethod(Overridable = true)]
        public static void testMethod(bool item) {
            Console.WriteLine(item.GetType());
        }

        public static void testMethod(int item) {
            Console.WriteLine(item.GetType());
        }

        public static void testMethod<T>(T item) {
            Console.WriteLine(typeof(T));
        }

        public static void testMethod<T>(T item, SMFunction func) {
            func.Invoke(item);
        }

        /// <summary>
        /// Used to test implicit conversions from SMFunction to Delegate 
        /// (Usage: testDel(function() { print("this is a test!"); }));
        /// </summary>
        /// <param name="delegate"></param>
        public static void testDel(TestDelegate @delegate) {
            object r = @delegate();
        }

        /*
        public static void print(string arg) 
        {
            Console.WriteLine(arg);
        }
        */

        public static void printf(string text, params object[] args) {
            int count = -1;

            Regex regex = new Regex("%s", RegexOptions.Singleline);
            Match match = regex.Match(text);

            while (match.Success) {
                string replacement = Convert.ToString(args[++count]);

                text = text.Remove(match.Index, match.Length);
                text = text.Insert(match.Index, replacement);

                match = regex.Match(text, match.Index + replacement.Length);
            }

            Console.WriteLine(text);
        }

        public static void print(string text) {
            Console.WriteLine(text);
        }

        public delegate object TestDelegate();
    }
}
