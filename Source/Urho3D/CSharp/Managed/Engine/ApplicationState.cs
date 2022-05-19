//
// Copyright (c) 2022-2022 the rbfx project.
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

namespace Urho3DNet
{
    public partial class ApplicationState
    {
        private UI _cachedUI;
        private ResourceCache _resourceCache;
        private Input _input;
        private Graphics _graphics;

        /// <summary>
        /// Get input subsystem from current context.
        /// </summary>
        public Input Input => _input ?? (_input = Context.GetSubsystem<Input>());

        /// <summary>
        /// Get graphics subsystem from current context.
        /// </summary>
        public Graphics Graphics => _graphics ?? (_graphics = Context.GetSubsystem<Graphics>());

        /// <summary>
        /// Get resource cache subsystem from current context.
        /// </summary>
        public ResourceCache ResourceCache => _resourceCache ?? (_resourceCache = Context.GetSubsystem<ResourceCache>());

        /// <summary>
        /// Get UI subsystem from current context.
        /// </summary>
        public UI UI => _cachedUI ?? (_cachedUI = Context.GetSubsystem<UI>());
    }
}
