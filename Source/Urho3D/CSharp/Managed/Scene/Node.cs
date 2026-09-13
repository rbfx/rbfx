// Copyright (c) 2017-2026 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

namespace Urho3DNet
{
    public partial class Node
    {
        public T CreateComponent<T>(uint id = 0) where T: Component
        {
            return (T)CreateComponent(ObjectReflection<T>.TypeId, id);
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
        public ComponentList GetComponents<T>() where T: Component
        {
            ComponentList componentList = new ComponentList();
            GetComponents(componentList, ObjectReflection<T>.TypeId);
            return componentList;
        }

        /// <summary>
        /// Return component in parent node. If there are several, returns the first. May optional traverse up to the root node.
        /// </summary>
        /// <typeparam name="T">Type of the component.</typeparam>
        /// <returns>Found component or null.</returns>
        public T GetParentComponent<T>() where T : Component
        {
            return (T)GetParentComponent(ObjectReflection<T>.TypeId);
        }

        /// <summary>
        /// Get first occurrence of a component derived from the type.
        /// </summary>
        /// <typeparam name="T">Type inherited from <see cref="Urho3DNet.Component"/> or interface marked with <see cref="Urho3DNet.DerivedFromAttribute"/></typeparam>
        /// <returns>Found component or null.</returns>
        public T GetDerivedComponent<T>() where T : class
        {
            return GetDerivedComponent(ObjectReflection<T>.TypeId) as T;
        }

        public T FindComponent<T>(ComponentSearchFlag flags = ComponentSearchFlag.Default) where T: class
        {
            return FindComponent(ObjectReflection<T>.TypeId, flags) as T;
        }

        /// <summary>
        /// Get all components that derives from type.
        /// </summary>
        /// <typeparam name="T">Type inherited from <see cref="Urho3DNet.Component"/> or interface marked with <see cref="Urho3DNet.DerivedFromAttribute"/></typeparam>
        /// <returns>List of found components.</returns>
        public ComponentList FindComponents<T>(ComponentSearchFlag flags = ComponentSearchFlag.Default)
        {
            ComponentList componentList = new ComponentList();
            FindComponents(componentList, ObjectReflection<T>.TypeId, flags);
            return componentList;
        }
    }
}
