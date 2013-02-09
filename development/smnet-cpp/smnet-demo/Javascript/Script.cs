using System;
using System.IO;
using System.Collections.Generic;
using System.Text;

using smnetjs;
using System.Runtime.InteropServices;

namespace smnet_demo.Javascript
{
    static class Script
    {
        [SMMethod(Name = "load", Script = true)]
        public static bool Load(SMScript script, string name, string filename) 
        {
            if (File.Exists(filename))
            {
                SMScript script2 = Program.SMRuntime.FindScript(name);

                //don't allow a script to kill itself
                if (script2 != null && script == script2)
                    return false;

                Program.SMRuntime.DestroyScript(name);

                script = Program.SMRuntime.InitScript(name, typeof(Program));

                if (script == null) return false;

                Stream stream = null;
                StreamReader reader = null;
                try
                {
                    stream = File.Open(filename, FileMode.Open, FileAccess.Read);
                    reader = new StreamReader(stream);

                    string code = reader.ReadToEnd();

                    if (!script.Compile(code))
                    {
                        script.Dispose();
                        return false;
                    }
                    else script.Execute();

                    if (ScriptLoaded != null)
                        ScriptLoaded(script, EventArgs.Empty);

                    return true;
                }
                catch { return false; }
                finally
                {
                    if (reader != null)
                        reader.Dispose();

                    else if (stream != null)
                        stream.Dispose();
                }
            }
            else return false;
        }

        [SMMethod(Name = "create", Script = true)]
        public static bool Create(SMScript script, string name, string code) 
        {
            SMScript script2 = Program.SMRuntime.FindScript(name);

            //don't allow a script to kill itself
            if (script2 != null && script == script2)
                return false;

            Program.SMRuntime.DestroyScript(name);

            script = Program.SMRuntime.InitScript(name, typeof(Program));

            if (script == null) return false;

            if (!script.Compile(code))
            {
                script.Dispose();
                return false;
            }
            else script.Execute();

            if (ScriptLoaded != null)
                ScriptLoaded(script, EventArgs.Empty);

            return true;
        }


        [SMMethod(Name = "eval")]
        public static string Eval(string name, string code) 
        {
            SMScript script = Program.SMRuntime.FindScript(name);

            if (script == null) return String.Empty;

            return script.Eval<string>(code);
        }

        [SMMethod(Name = "kill", Script = true)]
        public static bool Destroy(SMScript script, string name) 
        {
            //don't allow a script to kill itself
            if (script.Name != name)
            {
                Program.SMRuntime.DestroyScript(name);
                return true;
            }
            return false;
        }


        [SMProperty(Name = "scriptLoaded")]
        public static event EventHandler ScriptLoaded;
    }
}
