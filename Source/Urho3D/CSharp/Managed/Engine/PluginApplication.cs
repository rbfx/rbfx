//
// Copyright (c) 2017-2019 the rbfx project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
using System;
using System.Reflection;
using Urho3DNet.CSharp;

namespace Urho3DNet
{
    public partial class PluginApplication
    {
        private Assembly _hostAssembly;
        /// Sets assembly that is being managed by this plugin.
        internal void SetHostAssembly(Assembly assembly)
        {
            _hostAssembly = assembly;
        }

        private void OnSetupInstance()
        {
            SubscribeToEvent(E.PluginLoad, this, map =>
            {
                if (_hostAssembly == null)
                    _hostAssembly = GetType().Assembly;

                // Register factories marked with attributes
                foreach ((Type type, ObjectFactoryAttribute attr) in _hostAssembly.GetTypesWithAttribute<ObjectFactoryAttribute>())
                    RegisterFactory(type, attr.Category);
                UnsubscribeFromEvent(E.PluginLoad);
            });
        }

        public void RegisterFactory(Type type, string category = "")
        {
            Context.RegisterFactory(type, category);
            RecordPluginFactory(type, category);
        }
    }
}
