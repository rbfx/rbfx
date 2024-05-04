using System;
using System.Globalization;

namespace Urho3DNet
{
    /// <summary>
    /// Helper class to parse space separated strings line vectors, quaternions.
    /// </summary>
    public struct SpaceSeparatedValueHelper
    {
        /// <summary>
        /// String to parse.
        /// </summary>
        private readonly string _string;

        /// <summary>
        /// Current position in the string.
        /// </summary>
        private int _position;

        /// <summary>
        /// Create parser helper instance for the string.
        /// </summary>
        /// <param name="str">String to parse.</param>
        public SpaceSeparatedValueHelper(string str)
        {
            _position = 0;
            _string = str ?? String.Empty;
        }

        /// <summary>
        /// Parse next block of text in string as integer.
        /// </summary>
        /// <returns>Integer parsed or 0.</returns>
        public int ReadInt()
        {
            while (_position < _string.Length && Char.IsWhiteSpace(_string[_position]))
            {
                ++_position;
            }
            var start = _position;
            while (_position < _string.Length && !Char.IsWhiteSpace(_string[_position]) && _string[_position] != ' ')
            {
                ++_position;
            }
            if (start != _position)
            {
                if (int.TryParse(_string.Substring(start, _position - start), NumberStyles.Any,
                        CultureInfo.InvariantCulture, out var value))
                    return value;
            }

            return default;
        }

        /// <summary>
        /// Parse next block of text in string as float.
        /// </summary>
        /// <returns>Float parsed or 0.</returns>
        public float ReadFloat()
        {
            while (_position < _string.Length && Char.IsWhiteSpace(_string[_position]))
            {
                ++_position;
            }
            var start = _position;
            while (_position < _string.Length && !Char.IsWhiteSpace(_string[_position]) && _string[_position] != ' ')
            {
                ++_position;
            }
            if (start != _position)
            {
                if (float.TryParse(_string.Substring(start, _position - start), NumberStyles.Any,
                        CultureInfo.InvariantCulture, out var value))
                    return value;
            }

            return default;
        }
    }
}
