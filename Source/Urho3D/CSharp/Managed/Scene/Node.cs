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

using System.ComponentModel;

namespace Urho3DNet
{
    public partial class Node
    {
        public T CreateComponent<T>(uint id = 0) where T: Component
        {
            return (T)CreateComponent(ObjectReflection<T>.TypeId, id);
        }

        public T GetComponent<T>(bool recursive) where T: Component
        {
            return (T)GetComponent(ObjectReflection<T>.TypeId, recursive);
        }

        public T GetOrCreateComponent<T>(uint id = 0) where T: Component
        {
            return (T)GetOrCreateComponent(ObjectReflection<T>.TypeId, id);
        }

        public void RemoveComponent<T>() where T : Component
        {
            RemoveComponent(ObjectReflection<T>.TypeId);
        }

        /// <summary>
        /// Get first occurrence of a component type.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns>Found component or null.</returns>
        public T GetComponent<T>() where T: Component
        {
            return (T)GetComponent(ObjectReflection<T>.TypeId);
        }

        /// <summary>
        /// Get all components of a type.
        /// </summary>
        /// <typeparam name="T"></typeparam>
        /// <returns>List of found components.</returns>
        public ComponentList GetComponents<T>(bool recursive = false) where T: Component
        {
            ComponentList componentList = new ComponentList();
            GetComponents(componentList, ObjectReflection<T>.TypeId, recursive);
            return componentList;
        }

        /// <summary>
        /// Return component in parent node. If there are several, returns the first. May optional traverse up to the root node.
        /// </summary>
        /// <typeparam name="T">Type of the component.</typeparam>
        /// <returns>Found component or null.</returns>
        public T GetParentComponent<T>(bool fullTraversal = false) where T : Component
        {
            return (T)GetParentComponent(ObjectReflection<T>.TypeId, fullTraversal);
        }

        /// <summary>
        /// Get first occurrence of a component derived from the type.
        /// </summary>
        /// <typeparam name="T">Type inherited from <see cref="Urho3DNet.Component"/> or interface marked with <see cref="Urho3DNet.DerivedFromAttribute"/></typeparam>
        /// <returns>Found component or null.</returns>
        public T GetDerivedComponent<T>() where T: class
        {
            return GetDerivedComponent(ObjectReflection<T>.TypeId) as T;
        }

        /// <summary>
        /// Get all components that derives from type.
        /// </summary>
        /// <typeparam name="T">Type inherited from <see cref="Urho3DNet.Component"/> or interface marked with <see cref="Urho3DNet.DerivedFromAttribute"/></typeparam>
        /// <returns>List of found components.</returns>
        public ComponentList GetDerivedComponents<T>(bool recursive = false)
        {
            ComponentList componentList = new ComponentList();
            GetDerivedComponents(componentList, ObjectReflection<T>.TypeId, recursive);
            return componentList;
        }

        /// <summary>
        /// Return component in parent node that derives from type. If there are several, returns the first. May optional traverse up to the root node.
        /// </summary>
        /// <typeparam name="T">Type of the component.</typeparam>
        /// <returns>Found component or null.</returns>
        public T GetParentDerivedComponent<T>(bool fullTraversal = false) where T : Component
        {
            return (T)GetParentDerivedComponent(ObjectReflection<T>.TypeId, fullTraversal);
        }
    }
}
