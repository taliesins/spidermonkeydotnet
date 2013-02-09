using System;
using System.IO;
using System.Net;
using System.Collections.Generic;
using System.Text;

using smnetjs;

namespace smnet_demo.Javascript
{
    [SMEmbedded(Name = "HttpRequest")]
    class JSHttpRequest
    {
        class RequestState
        {
            public SMScript Script;
            public HttpWebRequest Request;
        }

        [SMMethod(Name = "request", Script = true)]
        public void Request(SMScript script, string url) 
        {
            Uri uri = null;
            if (Uri.TryCreate(url, UriKind.RelativeOrAbsolute, out uri))
            {
                HttpWebRequest request = (HttpWebRequest)WebRequest.Create(uri);

                request.BeginGetResponse(
                    new AsyncCallback(RequestCallback),
                    new RequestState()
                    {
                        Script = script,
                        Request = request
                    });
            }
        }

        private void RequestCallback(IAsyncResult ar) 
        {
            RequestState state = (RequestState)ar.AsyncState;
            HttpWebResponse response = (HttpWebResponse)state.Request.EndGetResponse(ar);

            Stream stream = response.GetResponseStream();
            StreamReader reader = new StreamReader(stream);

            string page = reader.ReadToEnd();

            reader.Dispose();

            state.Script.CallFunction("onReceived", state.Script.GetInstancePointer(this), page);
        }


        [SMMethod(Name = "onReceived", Overridable = true)]
        public void Callback(string page) { }
    }
}
