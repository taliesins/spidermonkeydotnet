using System;
using System.Threading;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Text.RegularExpressions;

using smnetjs;
using smnet_demo.Javascript;
using System.Net;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Linq.Expressions;
using System.Runtime.CompilerServices;

namespace smnet_demo
{
    class Program
    {
        public static SMRuntime SMRuntime 
        { 
            get; 
            private set; 
        }


        static void Main(string[] args) 
        {
            SMRuntime = new SMRuntime();

            SMRuntime.Embed(typeof(Script));
            SMRuntime.Embed(typeof(DateTime));
            SMRuntime.Embed(typeof(Environment));
            SMRuntime.Embed(typeof(JSHttpRequest));
            SMRuntime.Embed(typeof(TestDelegate));

            //Assembly.

            SMRuntime.OnScriptError += new ScriptErrorHandler(
                (scr, report) =>
                {
                    Console.WriteLine("{0} in {1} on line {2}", report.Message, scr.Name, report.LineNumber);
                }
            );

            SMScript script = SMRuntime.InitScript("eval", typeof(Program));

            script.SetOperationTimeout(30000);

            while (true)
            {
                Console.Write("js> ");

                string text = Console.ReadLine();
                if (text.ToLower() == "exit") break;

                //testing thread-safety
                DoEval(new object[] { script, text });
                //ThreadPool.QueueUserWorkItem(new WaitCallback(DoEval), new object[] { script, text });
            }

            script.Dispose();
            SMRuntime.Dispose();
        }

        static void DoEval(object state) 
        {
            SMScript script = (SMScript)((object[])state)[0];
            String text = (String)((object[])state)[1];

            text = script.Eval<string>(text);

            if (!String.IsNullOrEmpty(text))
                Console.WriteLine(text);

            script.Runtime.GarbageCollect();
            GC.Collect();
        }


        /// <summary>
        /// Used to test implicit conversions from SMFunction to Delegate 
        /// (Usage: testDel(function() { print("this is a test!"); }));
        /// </summary>
        /// <param name="delegate"></param>
        public static void testDel(TestDelegate @delegate) 
        {
            @delegate.Invoke();
        }

        /*
        public static void print(string arg) 
        {
            Console.WriteLine(arg);
        }
        */

        public static void printf(string text, params object[] args) 
        {
            int count = -1;

            Regex regex = new Regex("%s", RegexOptions.Singleline);
            Match match = regex.Match(text);

            while (match.Success)
            {
                string replacement = Convert.ToString(args[++count]);

                text = text.Remove(match.Index, match.Length);
                text = text.Insert(match.Index, replacement);

                match = regex.Match(text, match.Index + replacement.Length);
            }

            Console.WriteLine(text);
        }

        public static void print(string text) 
        {
            Console.WriteLine(text);
        }

        public delegate void TestDelegate();
    }
}
