// Copyright (c) 2022-2022 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

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
