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
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.IO;
using Editor.Events;
using Editor.Tabs;
using Urho3D;
using Object = Urho3D.Object;

namespace Editor
{
    public class Project : Object
    {
        private readonly string _projectPath;
        public string CachePath => _projectPath + "/Cache";
        public string DataPath => _projectPath + "/Data";
        public string ProjectFile => _projectPath + "/project.yml";
        public int Version = 1;
        private Dictionary<string, Type> _resourceDocumentTypes = new Dictionary<string, Type>
        {
            ["scene"] = typeof(SceneTab)
        };

        public Project(Context context, string projectPath) : base(context)
        {
            _projectPath = projectPath;
            if (!Directory.Exists(DataPath))
            {
                var source = new DirectoryInfo(Directory.GetCurrentDirectory() + "/CoreData");
                source.CopyAll(new DirectoryInfo(DataPath));
            }

            foreach (var path in new[]{_projectPath, CachePath, DataPath})
            {
                if (!Directory.Exists(path))
                    Directory.CreateDirectory(path);
            }

            if (!System.IO.File.Exists(ProjectFile))
                // Initialize new project
                Save();

            SubscribeToEvent<EditorKeyCombo>(e =>
            {
                if ((EditorKeyCombo.Kind) e.GetInt32(EditorKeyCombo.KeyCombo) == EditorKeyCombo.Kind.Save)
                    Save();
            });
        }

        protected override void Dispose(bool disposing)
        {
            if (disposing)
            {
                Cache.RemoveResourceDir(DataPath);
                Cache.RemoveResourceDir(CachePath);
            }

            base.Dispose(disposing);
        }

        public bool Load()
        {
            Cache.AddResourceDir(DataPath);
            Cache.AddResourceDir(CachePath);

            using (var yamlFile = new YAMLFile(Context))
            {
                if (yamlFile.LoadFile(ProjectFile))
                {
                    var save = yamlFile.Root;
                    // Notify alive objects
                    SendEvent<EditorProjectLoad>(
                        new Dictionary<StringHash, dynamic> {[EditorProjectLoad.SaveData] = save});

                    // Initialize remaining objects
                    var resources = save.Get("resources");
                    for (var i = 0; i < resources.Size(); i++)
                    {
                        var resource = resources.Get(i);
                        var tab = GetSubsystem<Editor>().OpenTab(_resourceDocumentTypes[resource.Get("type").String]);
                        tab.LoadSave(resource);
                    }

                    return true;
                }
            }

            return false;
        }

        public bool Save()
        {
            using (var save = new JSONValue(JSONValueType.Object))
            {
                save.Set("version", Version);
                save.Set("resources", new JSONValue(JSONValueType.Array));

                SendEvent<EditorProjectSave>(new Dictionary<StringHash, dynamic> {[EditorProjectSave.SaveData] = save});

                using (var yamlFile = new YAMLFile(Context))
                {
                    yamlFile.Root = save;
                    return yamlFile.SaveFile(ProjectFile);
                }
            }
        }

        public void RemoveResourcePath(string selectedPath)
        {
            try
            {
                var fullPath = $"{DataPath}/{selectedPath}";
                if (System.IO.File.Exists(fullPath))
                {
                    SendEvent<EditorDeleteResource>(new ConcurrentDictionary<StringHash, dynamic>
                    {
                        [EditorDeleteResource.ResourceName] = selectedPath
                    });
                    System.IO.File.Delete(fullPath);
                }
                else if (Directory.Exists(fullPath))
                {
                    void SendEventRecursive(DirectoryInfo dir)
                    {
                        foreach (var fileInfo in dir.GetFiles())
                        {
                            SendEvent<EditorDeleteResource>(new ConcurrentDictionary<StringHash, dynamic>
                            {
                                [EditorDeleteResource.ResourceName] = Cache.SanitateResourceName(fileInfo.FullName)
                            });
                        }

                        foreach (var directoryInfo in dir.GetDirectories())
                        {
                            SendEventRecursive(directoryInfo);
                        }
                    }

                    var dirInfo = new DirectoryInfo(fullPath);
                    SendEventRecursive(dirInfo);
                    dirInfo.DeleteAll();
                }
            }
            catch (IOException e)
            {
                Log.Write(Log.Error, e.Message);
                Log.Write(Log.Debug, e.StackTrace);
            }
        }
    }
}
