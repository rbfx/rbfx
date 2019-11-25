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

using System.Collections.Generic;
using System.Linq;

namespace Urho3DNet
{
    public partial class Node
    {
        public T CreateComponent<T>(CreateMode mode = CreateMode.Replicated, uint id = 0) where T : Component
        {
            return (T)CreateComponent(typeof(T).Name, mode, id);
        }

        public T GetComponent<T>(bool recursive) where T : Component
        {
            return (T)GetComponent(typeof(T).Name, recursive);
        }

        /// <summary>
        /// Get first occurance of a component type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public T GetComponent<T>() where T : Component
        {
            return (T)GetComponent(typeof(T).Name);
        }

        /// <summary>
        /// get all components of a type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns></returns>
        public ComponentList GetComponents<T>(bool recursive = false) where T : Component
        {
            ComponentList components = new ComponentList();
            GetComponents(components, nameof(T), recursive);

            return components;
        }

        /// <summary>
        /// Remove all childern of a specific Type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        public void RemoveAllChildren<T>() where T : Node
        {
            var childrenNode = GetChildren<T>();
            foreach (var childnode in childrenNode)
            {
                RemoveChild(childnode);
            }
        }

        /// <summary>
        /// Get all Children of a specific type
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <param name="recursive"></param>
        /// <returns></returns>
        public IEnumerable<T> GetChildren<T>(bool recursive = false) where T : Node
        {
            NodeList children = new NodeList();
            GetChildren(children, recursive);
            return children.Where(o => o is T).Cast<T>();
        }
    }
}
