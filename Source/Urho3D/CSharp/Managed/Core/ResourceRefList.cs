// Copyright (c) 2024-2024 the rbfx project.
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT> or the accompanying LICENSE file.

using System.Text;

namespace Urho3DNet
{
    public partial class ResourceRefList
    {
        public static ResourceRefList Parse(string text)
        {
            if (string.IsNullOrWhiteSpace(text))
                return null;

            int splitIndex = text.IndexOf(';');

            if (splitIndex < 0)
            {
                return new ResourceRefList(text);
            }

            StringList list = new StringList();

            int prevSplit = splitIndex+1;
            for (;;)
            {
                int nextSplit = text.IndexOf(';', prevSplit);
                if (nextSplit < 0)
                {
                    list.Add(text.Substring(prevSplit));
                    break;
                }
                list.Add(text.Substring(prevSplit, nextSplit - prevSplit));
                prevSplit = nextSplit+1;
            }


            return new ResourceRefList(text.Substring(0, splitIndex), list);
        }

        public string ToString(Context context)
        {
            var sb = new StringBuilder();
            sb.Append(context.GetUrhoTypeName(this.Type));
            foreach (var name in this.Names)
            {
                sb.Append(';');
                sb.Append(name);
            }
            return sb.ToString();
        }
    }
}
