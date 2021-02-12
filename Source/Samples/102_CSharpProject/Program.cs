//
// Copyright (c) 2017-2020 the rbfx project.
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
using System.Diagnostics;
using System.IO;
using Urho3DNet;
using ImGuiNet;

namespace DemoApplication
{
    class DemoApplication : Application
    {
        private Viewport _viewport;
        private Scene _scene;
        private Node _camera;
        private Node _cube;
        private Node _light;

        public DemoApplication(Context context) : base(context)
        {
        }

        protected override void Dispose(bool disposing)
        {
            // It is vital to dispose of objects that were allocated by using `new` operator or context factories.
            _viewport.Dispose();
            _scene.Dispose();
            base.Dispose(disposing);
        }

        public override void Setup()
        {
            var currentDir = Directory.GetCurrentDirectory();
            EngineParameters[Urho3D.EpFullScreen] = false;
            EngineParameters[Urho3D.EpWindowWidth] = 1920;
            EngineParameters[Urho3D.EpWindowHeight] = 1080;
            EngineParameters[Urho3D.EpWindowTitle] = "Hello C#";
            EngineParameters[Urho3D.EpResourcePrefixPaths] = $"{currentDir};{currentDir}/..";
            // Opt in for automatic compilation of scripts in ResourcePath/Script folder.
            EngineParameters[Urho3D.EpEngineAutoLoadScripts] = true;
        }

        public override void Start()
        {
            Context.Input.SetMouseVisible(true);

            // Viewport
            _scene = new Scene(Context);
            _scene.CreateComponent<Octree>();

            _camera = _scene.CreateChild("Camera");
            _viewport = new Viewport(Context);
            _viewport.Scene = _scene;
            _viewport.Camera = _camera.CreateComponent<Camera>();
            Context.Renderer.SetViewport(0, _viewport);

            // Background
            Context.Renderer.DefaultZone.FogColor = new Color(0.5f, 0.5f, 0.7f);

            // Scene
            _camera.Position = new Vector3(0, 2, -2);
            _camera.LookAt(Vector3.Zero);

            // Cube
            _cube = _scene.CreateChild("Cube");
            var model = _cube.CreateComponent<StaticModel>();
            model.SetModel(Context.ResourceCache.GetResource<Model>("Models/Box.mdl"));
            model.SetMaterial(0, Context.ResourceCache.GetResource<Material>("Materials/Stone.xml"));

            // RotateObject component is implemented in Data/Scripts/RotateObject.cs
            _cube.CreateComponent("ScriptRotateObject");

            // Light
            _light = _scene.CreateChild("Light");
            _light.CreateComponent<Light>();
            _light.Position = new Vector3(0, 2, -1);
            _light.LookAt(Vector3.Zero);

            SubscribeToEvent(E.Update, args =>
            {
                float timestep = args[E.Update.TimeStep].Float;
                Debug.Assert(this != null);

                if (ImGui.Begin("Urho3D.NET"))
                {
                    ImGui.TextColored(Color.Red, $"Hello world from C#.\nFrame time: {timestep}");
                }
                ImGui.End();
            });
        }
    }

    internal class Program
    {
        public static void Main(string[] args)
        {
            // This is required for loading and compiling *.cs scripts from resource path.
            Context.SetRuntimeApi(new CompiledScriptRuntimeApiImpl());
            using (var context = new Context())
            {
                using (var application = new DemoApplication(context))
                {
                    Environment.ExitCode = application.Run();
                }
            }
        }
    }
}
