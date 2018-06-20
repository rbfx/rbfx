//
// Copyright (c) 2018 Rokas Kupstys
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
using System.IO;
using System.Linq;
using System.Text;
using IconFonts;
using Urho3D;
using ImGui;
using Urho3D.Events;


namespace Editor.Tabs
{
    public class ResourcesTab : Tab
    {
        private delegate void FileNamePopupConfirmationDelegate(string fileName);
        private struct FileNamePopup
        {
            public string WindowLabel;
            public string InputLabel;
            public bool IsOpen;
            public byte[] Buffer;
            public FileNamePopupConfirmationDelegate ConfirmationCallback;
        }

        private string _resourcePath;
        private string _resourceSelection;
        private FileNamePopup _fileNamePopup;

        public ResourcesTab(Context context, string title, TabLifetime lifetime, Vector2? initialSize = null,
            string placeNextToDock = null, DockSlot slot = DockSlot.SlotNone) : base(context, title, lifetime,
            initialSize, placeNextToDock, slot)
        {
            Uuid = "4fcf9895-e363-441d-a665-2a97f7da64ef";
            _fileNamePopup.InputLabel = "";
            _fileNamePopup.Buffer = new byte[1024];
        }

        protected override void Render()
        {
            switch (ResourceBrowser.ResourceBrowserWidget(ref _resourcePath, ref _resourceSelection))
            {
                case ResourceBrowserResult.ItemOpen:
                {
                    var resourceName = $"{_resourcePath}{_resourceSelection}";
                    var type = ContentUtilities.GetContentType(resourceName);
                    switch (type)
                    {
                        case ContentType.CtypeUnknown:
                            break;
                        case ContentType.CtypeScene:
                            var editor = GetSubsystem<Editor>();
                            var tab = editor.GetOpenTab(resourceName);
                            if (tab == null)
                                editor.OpenTab<SceneTab>().LoadResource(resourceName);
                            else
                                tab.Activate();
                            break;
                        case ContentType.CtypeSceneobject:
                            break;
                        case ContentType.CtypeUilayout:
                            break;
                        case ContentType.CtypeUistyle:
                            break;
                        case ContentType.CtypeModel:
                            break;
                        case ContentType.CtypeAnimation:
                            break;
                        case ContentType.CtypeMaterial:
                            break;
                        case ContentType.CtypeParticle:
                            break;
                        case ContentType.CtypeRenderpath:
                            break;
                        case ContentType.CtypeSound:
                            break;
                        case ContentType.CtypeTexture:
                            break;
                        case ContentType.CtypeTexturexml:
                            break;
                        default:
                            throw new ArgumentOutOfRangeException();
                    }
                    break;
                }
                case ResourceBrowserResult.ItemContextMenu:
                {
                    ui.OpenPopup("Resource context menu");
                    break;
                }
            }

            if (ui.BeginPopup("Resource context menu"))
            {
                var project = GetSubsystem<Project>();

                if (ui.MenuItem($"{FontAwesome.FileO} New Scene"))
                {
                    _fileNamePopup.IsOpen = true;
                    _fileNamePopup.InputLabel = ".scene";
                    _fileNamePopup.WindowLabel = "Enter Scene Name";
                    _fileNamePopup.ConfirmationCallback = selection =>
                    {
                        var editor = GetSubsystem<Editor>();
                        editor.OpenTab<SceneTab>().SaveAs($"{_resourcePath}{selection}.scene");
                    };
                }

                if (ui.MenuItem($"{FontAwesome.FolderOpenO} New Directory"))
                {
                    _fileNamePopup.IsOpen = true;
                    _fileNamePopup.InputLabel = "  ";    // Extends width of dialog enough so full titlebar is visible.
                    _fileNamePopup.WindowLabel = "Enter Directory Name";
                    _fileNamePopup.ConfirmationCallback = selection =>
                    {
                        Directory.CreateDirectory($"{project.DataPath}/{selection}");
                    };
                }

                if (ui.MenuItem($"{FontAwesome.TrashO} Remove"))
                {
                    project.RemoveResourcePath($"{_resourcePath}{_resourceSelection}");
                }

                ui.EndPopup();
            }

            if (_fileNamePopup.IsOpen)
            {
                ui.SetNextWindowSize(Vector2.Zero, Condition.Always);
                if (ui.Begin($"{_fileNamePopup.WindowLabel}###ResourceBrowserFileNameDialog", ref _fileNamePopup.IsOpen,
                    WindowFlags.NoCollapse | WindowFlags.NoResize))
                {
                    var accept = ui.InputText(_fileNamePopup.InputLabel, _fileNamePopup.Buffer, (uint) _fileNamePopup.Buffer.Length,
                        InputTextFlags.EnterReturnsTrue, null);
                    ui.SameLine();
                    accept |= ui.Button("Ok");
                    if (accept)
                    {
                        var length = _fileNamePopup.Buffer.TakeWhile(b => b != '\0').Count();
                        var name = Encoding.UTF8.GetString(_fileNamePopup.Buffer, 0, length);
                        _fileNamePopup.ConfirmationCallback(name);
                        _fileNamePopup.IsOpen = false;
                    }
                }
                ui.End();
            }
        }
    }
}
