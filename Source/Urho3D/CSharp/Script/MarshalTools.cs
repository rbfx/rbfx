using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Reflection;
using System.Text;
using System.Runtime.InteropServices;

namespace Urho3D.CSharp
{
    public class ScratchBuffer
    {
        class Block : IDisposable
        {
            public IntPtr Memory;
            public int Length;
            public int Used;
            public int Free => Length - Used;

            public Block(int length)
            {
                Memory = Marshal.AllocHGlobal(length);
                Length = length;
            }

            public IntPtr Take(int length)
            {
                if (Free < length)
                    return IntPtr.Zero;

                try
                {
                    return Memory + Used;
                }
                finally
                {
                    Used += length;
                }
            }

            public void Dispose()
            {
                if (Memory != System.IntPtr.Zero)
                    Marshal.FreeHGlobal(Memory);
            }
        }

        struct Allocation
        {
            public Block Block;
            public int Length;
        }

        private List<Block> _blocks = new List<Block>();
        private Dictionary<IntPtr, Allocation> _allocated = new Dictionary<IntPtr, Allocation>();
        private int _maxAllocatedMemory;

        public ScratchBuffer(int initialLength)
        {
            _maxAllocatedMemory = initialLength;
            _blocks.Add(new Block(_maxAllocatedMemory));
        }

        public IntPtr Alloc(int length)
        {
            int maxBlockSize = length;
            foreach (var block in _blocks)
            {
                var memory = block.Take(length);
                if (memory != IntPtr.Zero)
                {
                    _allocated[memory] = new Allocation{Block = block, Length = length};
                    return memory;
                }
                maxBlockSize = Math.Max(maxBlockSize, block.Length);
            }

            var newBlock = new Block(maxBlockSize);
            _blocks.Add(newBlock);
            _maxAllocatedMemory += maxBlockSize;

            var memory2 = newBlock.Take(length);
            _allocated[memory2] = new Allocation { Block = newBlock, Length = length };
            return memory2;
        }

        public void Free(IntPtr memory)
        {
            Allocation allocation;
            if (!_allocated.TryGetValue(memory, out allocation))
                return;

            allocation.Block.Used -= allocation.Length;

            if (allocation.Block.Used == 0 && allocation.Block.Length < _maxAllocatedMemory)
            {
                allocation.Block.Dispose();
                _blocks.Remove(allocation.Block);
            }

            _allocated.Remove(memory);

            if (_blocks.Count == 0)
                _blocks.Add(new Block(_maxAllocatedMemory));
        }
    }


    /// <summary>
    /// Marshals utf-8 strings. Native code controls lifetime of native string.
    /// </summary>
    public class StringUtf8 : ICustomMarshaler
    {
        [ThreadStatic]
        private static StringUtf8 _instance;
        private readonly ScratchBuffer _scratch = new ScratchBuffer(2048);

        public static ICustomMarshaler GetInstance(string cookie)
        {
            return _instance ?? (_instance = new StringUtf8());
        }

        public void CleanUpManagedData(object managedObj)
        {
        }

        public void CleanUpNativeData(IntPtr pNativeData)
        {
            _scratch.Free(pNativeData);
        }

        public int GetNativeDataSize()
        {
            return Marshal.SizeOf<IntPtr>();
        }

        public IntPtr MarshalManagedToNative(object managedObj)
        {
            if (!(managedObj is string))
                return IntPtr.Zero;

            var s = Encoding.UTF8.GetBytes((string) managedObj);
            var pStr = _scratch.Alloc(s.Length + 1);

            Marshal.Copy(s, 0, pStr, s.Length);
            Marshal.WriteByte(pStr, s.Length, 0);

            return pStr;
        }

        public unsafe object MarshalNativeToManaged(IntPtr pNativeData)
        {
            var length = 0;
            while (Marshal.ReadByte(pNativeData, length) != 0)
                ++length;
            return Encoding.UTF8.GetString((byte*) pNativeData, length);
        }
    }

    /// <summary>
    /// Marshals utf-8 strings. Managed code frees native string after obtaining it. Used in cases when native code
    /// returns by value.
    /// </summary>
    public class StringUtf8Copy : StringUtf8
    {
        [ThreadStatic]
        private static StringUtf8Copy _instance;

        public new static ICustomMarshaler GetInstance(string _)
        {
            return _instance ?? (_instance = new StringUtf8Copy());
        }

        public new object MarshalNativeToManaged(IntPtr pNativeData)
        {
            try
            {
                return base.MarshalNativeToManaged(pNativeData);
            }
            finally
            {
                NativeInterface.Native.FreeMemory(pNativeData);
            }
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    internal struct SafeArray
    {
        public IntPtr Data;
        public int Length;

        [ThreadStatic] private static ScratchBuffer _scratch;

        static SafeArray()
        {
            _scratch = new ScratchBuffer(1024*1024);
        }

        public static unsafe T[] GetManagedInstance<T>(SafeArray data, bool ownsNativeInstance)
        {
            if (data.Length == 0)
                return new T[0];

            var type = typeof(T);
            T[] result;
            if (type.IsValueType)
            {
                // Array of structs or builtin types.
                var size = Marshal.SizeOf<T>();
                result = new T[data.Length / size];
                var handle = GCHandle.Alloc(result, GCHandleType.Pinned);
                Buffer.MemoryCopy((void*) data.Data, (void*) handle.AddrOfPinnedObject(), data.Length, data.Length);
                handle.Free();
            }
            else
            {
                // Array of pointers to objects
                var tFromPInvoke = type.GetMethod("GetManagedInstance", BindingFlags.Public | BindingFlags.Static);
                var pointers = (void**) data.Data;
                Debug.Assert(pointers != null);
                Debug.Assert(tFromPInvoke != null);
                var count = data.Length / IntPtr.Size;
                result = new T[count];
                for (var i = 0; i < count; i++)
                    result[i] = (T) tFromPInvoke.Invoke(null, new object[] {new IntPtr(pointers[i]), false});
            }

            return result;
        }

        public static unsafe SafeArray GetNativeInstance<T>(T[] data)
        {
            if (data == null)
                return new SafeArray();

            var type = typeof(T);
            var length = data.Length * (type.IsValueType ? Marshal.SizeOf<T>() : IntPtr.Size);
            var result = new SafeArray
            {
                Length = length,
                Data = _scratch.Alloc(length)
            };

            if (type.IsValueType)
            {
                // Array of structs or builtin types.
                var handle = GCHandle.Alloc(result, GCHandleType.Pinned);
                Buffer.MemoryCopy((void*) handle.AddrOfPinnedObject(), (void*) result.Data, result.Length, result.Length);
                handle.Free();
            }
            else
            {
                // Array of pointers to objects
                var tToPInvoke = type.GetMethod("GetNativeInstance", BindingFlags.Public | BindingFlags.Static);
                var pointers = (void**) result.Length;
                Debug.Assert(tToPInvoke != null);
                Debug.Assert(pointers != null);
                for (var i = 0; i < data.Length; i++)
                    pointers[i] = ((IntPtr) tToPInvoke.Invoke(null, new object[] {data[i]})).ToPointer();
            }

            return result;
        }
    }

    internal static class MarshalTools
    {
        internal static bool HasOverride(this MethodInfo method)
        {
            return (method.Attributes & MethodAttributes.Virtual) != 0 &&
                   (method.Attributes & MethodAttributes.NewSlot) == 0;
        }

        internal static bool HasOverride(this Type type, string methodName, params Type[] paramTypes)
        {
            var method = type.GetMethod(methodName, BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic, null,
                CallingConventions.HasThis, paramTypes, new ParameterModifier[0]);
            return method != null && method.HasOverride();
        }
    }
}
