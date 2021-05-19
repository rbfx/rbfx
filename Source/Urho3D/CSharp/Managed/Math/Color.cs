//
// Copyright (c) 2008-2019 the Urho3D project.
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
using System.Runtime.InteropServices;

namespace Urho3DNet
{
    /// RGBA color.
    [StructLayout(LayoutKind.Sequential)]
    public struct Color : IEquatable<Color>
    {
        public Color(in Color color)
        {
            R = color.R;
            G = color.G;
            B = color.B;
            A = color.A;
        }

        /// Construct from another color and modify the alpha.
        public Color(in Color color, float a)
        {
            R = color.R;
            G = color.G;
            B = color.B;
            A = a;
        }

        /// Construct from RGB values and set alpha fully opaque.
        public Color(float r, float g, float b)
        {
            R = r;
            G = g;
            B = b;
            A = 1.0f;
        }

        /// Construct from RGB values or opaque white by default.
        public Color(float r = 1.0f, float g = 1.0f, float b = 1.0f, float a = 1.0f)
        {
            R = r;
            G = g;
            B = b;
            A = a;
        }

        /// Construct from a float array.
        public unsafe Color(float* data)
        {
            R = data[0];
            G = data[1];
            B = data[2];
            A = data[3];
        }

        /// Test for equality with another color without epsilon.
        public static bool operator ==(in Color lhs, in Color rhs)
        {
            return MathDefs.Equals(lhs.R, rhs.R) &&
                   MathDefs.Equals(lhs.G, rhs.G) &&
                   MathDefs.Equals(lhs.B, rhs.B) &&
                   MathDefs.Equals(lhs.A, rhs.A);
        }

        /// Test for inequality with another color without epsilon.
        public static bool operator !=(in Color lhs, in Color rhs)
        {
            return !MathDefs.Equals(lhs.R, rhs.R) ||
                   !MathDefs.Equals(lhs.G, rhs.G) ||
                   !MathDefs.Equals(lhs.B, rhs.B) ||
                   !MathDefs.Equals(lhs.A, rhs.A);
        }

        /// Multiply with a scalar.
        public static Color operator *(in Color lhs, float rhs) { return new Color(lhs.R * rhs, lhs.G * rhs, lhs.B * rhs, lhs.A * rhs); }

        /// Modulate with a color.
        public static Color operator *(in Color lhs, in Color rhs) { return new Color(lhs.R * rhs.R, lhs.G * rhs.G, lhs.B * rhs.B, lhs.A * rhs.A); }

        /// Add a color.
        public static Color operator +(in Color lhs, in Color rhs) { return new Color(lhs.R + rhs.R, lhs.G + rhs.G, lhs.B + rhs.B, lhs.A + rhs.A); }

        /// Return negation.
        public static Color operator -(in Color lhs) { return new Color(-lhs.R, -lhs.G, -lhs.B, -lhs.A); }

        /// Subtract a color.
        public static Color operator -(in Color lhs, in Color rhs) { return new Color(lhs.R - rhs.R, lhs.G - rhs.G, lhs.B - rhs.B, lhs.A - rhs.A); }

        /// Return color packed to a 32-bit integer, with R component in the lowest 8 bits. Components are clamped to [0, 1] range.
        public uint ToUInt()
        {
            var r = (uint) MathDefs.Clamp(((int) (R * 255.0f)), 0, 255);
            var g = (uint) MathDefs.Clamp(((int) (G * 255.0f)), 0, 255);
            var b = (uint) MathDefs.Clamp(((int) (B * 255.0f)), 0, 255);
            var a = (uint) MathDefs.Clamp(((int) (A * 255.0f)), 0, 255);
            return (a << 24) | (b << 16) | (g << 8) | r;
        }

        /// Return HSL color-space representation as a Vector3; the RGB values are clipped before conversion but not changed in the process.
        public Vector3 ToHSL()
        {
            float min, max;
            Bounds(out min, out max, true);

            float h = Hue(min, max);
            float s = SaturationHSL(min, max);
            float l = (max + min) * 0.5f;

            return new Vector3(h, s, l);
        }

        /// Return HSV color-space representation as a Vector3; the RGB values are clipped before conversion but not changed in the process.
        public Vector3 ToHSV()
        {
            float min, max;
            Bounds(out min, out max, true);

            float h = Hue(min, max);
            float s = SaturationHSV(min, max);
            float v = max;

            return new Vector3(h, s, v);
        }

        /// Set RGBA values from packed 32-bit integer, with R component in the lowest 8 bits (format 0xAABBGGRR).
        public void FromUInt(uint color)
        {
            A = ((color >> 24) & 0xff) / 255.0f;
            B = ((color >> 16) & 0xff) / 255.0f;
            G = ((color >> 8) & 0xff) / 255.0f;
            R = ((color >> 0) & 0xff) / 255.0f;
        }

        /// Set RGBA values from specified HSL values and alpha.
        public void FromHSL(float h, float s, float l, float a = 1.0f)
        {
            float c;
            if (l < 0.5f)
                c = (1.0f + (2.0f * l - 1.0f)) * s;
            else
                c = (1.0f - (2.0f * l - 1.0f)) * s;

            float m = l - 0.5f * c;

            FromHCM(h, c, m);

            A = a;
        }

        /// Set RGBA values from specified HSV values and alpha.
        public void FromHSV(float h, float s, float v, float a = 1.0f)
        {
            float c = v * s;
            float m = v - c;

            FromHCM(h, c, m);

            A = a;
        }

        /// Return RGB as a three-dimensional vector.
        public Vector3 ToVector3()
        {
            return new Vector3(R, G, B);
        }

        /// Return RGBA as a four-dimensional vector.
        public Vector4 ToVector4()
        {
            return new Vector4(R, G, B, A);
        }

        /// Return sum of RGB components.
        public float SumRGB()
        {
            return R + G + B;
        }

        /// Return average value of the RGB channels.
        public float Average()
        {
            return (R + G + B) / 3.0f;
        }

        /// Return the 'grayscale' representation of RGB values, as used by JPEG and PAL/NTSC among others.
        public float Luma()
        {
            return R * 0.299f + G * 0.587f + B * 0.114f;
        }

        /// Return the colorfulness relative to the brightness of a similarly illuminated white.
        public float Chroma()
        {
            float min, max;
            Bounds(out min, out max, true);

            return max - min;
        }

        /// Return hue mapped to range [0, 1.0).
        public float Hue()
        {
            float min, max;
            Bounds(out min, out max, true);

            return Hue(min, max);
        }

        /// Return saturation as defined for HSL.
        public float SaturationHSL()
        {
            float min, max;
            Bounds(out min, out max, true);

            return SaturationHSL(min, max);
        }

        /// Return saturation as defined for HSV.
        public float SaturationHSV()
        {
            float min, max;
            Bounds(out min, out max, true);

            return SaturationHSV(min, max);
        }

        /// Return value as defined for HSV: largest value of the RGB components. Equivalent to calling MinRGB().
        public float Value()
        {
            return MaxRGB();
        }

        /// Return lightness as defined for HSL: average of the largest and smallest values of the RGB components.
        public float Lightness()
        {
            float min, max;
            Bounds(out min, out max, true);

            return (max + min) * 0.5f;
        }

        /// Stores the values of least and greatest RGB component at specified pointer addresses, optionally clipping those values to [0, 1] range.
        public void Bounds(out float min, out float max, bool clipped = false)
        {
            if (R > G)
            {
                if (G > B) // r > g > b
                {
                    max = R;
                    min = B;
                }
                else // r > g && g <= b
                {
                    max = R > B ? R : B;
                    min = G;
                }
            }
            else
            {
                if (B > G) // r <= g < b
                {
                    max = B;
                    min = R;
                }
                else // r <= g && b <= g
                {
                    max = G;
                    min = R < B ? R : B;
                }
            }

            if (clipped)
            {
                max = max > 1.0f ? 1.0f : (max < 0.0f ? 0.0f : max);
                min = min > 1.0f ? 1.0f : (min < 0.0f ? 0.0f : min);
            }
        }

        /// Return the largest value of the RGB components.
        public float MaxRGB()
        {
            if (R > G)
                return (R > B) ? R : B;
            else
                return (G > B) ? G : B;
        }

        /// Return the smallest value of the RGB components.
        public float MinRGB()
        {
            if (R < G)
                return (R < B) ? R : B;
            else
                return (G < B) ? G : B;
        }

        /// Return range, defined as the difference between the greatest and least RGB component.
        public float Range()
        {
            float min, max;
            Bounds(out min, out max);
            return max - min;
        }

        /// Clip to [0, 1.0] range.
        public void Clip(bool clipAlpha = false)
        {
            R = (R > 1.0f) ? 1.0f : ((R < 0.0f) ? 0.0f : R);
            G = (G > 1.0f) ? 1.0f : ((G < 0.0f) ? 0.0f : G);
            B = (B > 1.0f) ? 1.0f : ((B < 0.0f) ? 0.0f : B);

            if (clipAlpha)
                A = (A > 1.0f) ? 1.0f : ((A < 0.0f) ? 0.0f : A);
        }

        /// Inverts the RGB channels and optionally the alpha channel as well.
        public void Invert(bool invertAlpha = false)
        {
            R = 1.0f - R;
            G = 1.0f - G;
            B = 1.0f - B;

            if (invertAlpha)
                A = 1.0f - A;
        }

        /// Return linear interpolation of this color with another color.
        public Color Lerp(in Color rhs, float t)
        {
            float invT = 1.0f - t;
            return new Color(
                R * invT + rhs.R * t,
                G * invT + rhs.G * t,
                B * invT + rhs.B * t,
                A * invT + rhs.A * t
            );
        }

        /// Return color with absolute components.
        public Color Abs()
        {
            return new Color(Math.Abs(R), Math.Abs(G), Math.Abs(B), Math.Abs(A));
        }

        /// Test for equality with another color with epsilon.
        public bool Equals(Color obj)
        {
            return MathDefs.Equals(R, obj.R) &&
                   MathDefs.Equals(G, obj.G) &&
                   MathDefs.Equals(B, obj.B) &&
                   MathDefs.Equals(A, obj.A);
        }

        /// Test for equality with another color with epsilon.
        public override bool Equals(object obj)
        {
            if (ReferenceEquals(null, obj))
                return false;
            return obj is Color && Equals((Color) obj);
        }

        /// Return as string.
        public new string ToString()
        {
            return $"{R} {G} {B} {A}";
        }

        /// Return color packed to a 32-bit integer, with B component in the lowest 8 bits. Components are clamped to [0, 1] range.
        public uint ToUIntArgb()
        {
            uint r = (uint) MathDefs.Clamp(((int) (R * 255.0f)), 0, 255);
            uint g = (uint) MathDefs.Clamp(((int) (G * 255.0f)), 0, 255);
            uint b = (uint) MathDefs.Clamp(((int) (B * 255.0f)), 0, 255);
            uint a = (uint) MathDefs.Clamp(((int) (A * 255.0f)), 0, 255);
            return (a << 24) | (r << 16) | (g << 8) | b;
        }

        /// Return hash value.
        public override int GetHashCode()
        {
            return (int)ToUInt();
        }

        /// Return hue value given greatest and least RGB component, value-wise.
        private float Hue(float min, float max)
        {
            float chroma = max - min;

            // If chroma equals zero, hue is undefined
            if (chroma <= MathDefs.Epsilon)
                return 0.0f;

            // Calculate and return hue
            if (MathDefs.Equals(G, max))
                return (B + 2.0f * chroma - R) / (6.0f * chroma);
            else if (MathDefs.Equals(B, max))
                return (4.0f * chroma - G + R) / (6.0f * chroma);
            else
            {
                float r = (G - B) / (6.0f * chroma);
                return (r < 0.0f) ? 1.0f + r : ((r >= 1.0f) ? r - 1.0f : r);
            }
        }

        /// Return saturation (HSV) given greatest and least RGB component, value-wise.
        private float SaturationHSV(float min, float max)
        {
            // Avoid div-by-zero: result undefined
            if (max <= MathDefs.Epsilon)
                return 0.0f;

            // Saturation equals chroma:value ratio
            return 1.0f - (min / max);
        }

        /// Return saturation (HSL) given greatest and least RGB component, value-wise.
        private float SaturationHSL(float min, float max)
        {
            // Avoid div-by-zero: result undefined
            if (max <= MathDefs.Epsilon || min >= 1.0f - MathDefs.Epsilon)
                return 0.0f;

            // Chroma = max - min, lightness = (max + min) * 0.5
            float hl = (max + min);
            if (hl <= 1.0f)
                return (max - min) / hl;
            else
                return (min - max) / (hl - 2.0f);
        }

        /// Calculate and set RGB values. Convenience function used by FromHSV and FromHSL to avoid code duplication.
        private void FromHCM(float h, float c, float m)
        {
            if (h < 0.0f || h >= 1.0f)
                h -= (float) Math.Floor(h);

            float hs = h * 6.0f;
            float x = c * (1.0f - Math.Abs((hs % 2.0f) - 1.0f));

            // Reconstruct r', g', b' from hue
            if (hs < 2.0f)
            {
                B = 0.0f;
                if (hs < 1.0f)
                {
                    G = x;
                    R = c;
                }
                else
                {
                    G = c;
                    R = x;
                }
            }
            else if (hs < 4.0f)
            {
                R = 0.0f;
                if (hs < 3.0f)
                {
                    G = c;
                    B = x;
                }
                else
                {
                    G = x;
                    B = c;
                }
            }
            else
            {
                G = 0.0f;
                if (hs < 5.0f)
                {
                    R = x;
                    B = c;
                }
                else
                {
                    R = c;
                    B = x;
                }
            }

            R += m;
            G += m;
            B += m;
        }

        /// Red value.
        public float R;

        /// Green value.
        public float G;

        /// Blue value.
        public float B;

        /// Alpha value.
        public float A;

        /// Opaque white color.
        public static readonly Color White = new Color(1.0f, 1.0f, 1.0f, 1.0f);

        /// Opaque gray color.
        public static readonly Color Gray = new Color(0.5f, 0.5f, 0.5f);

        /// Opaque black color.
        public static readonly Color Black = new Color(0.0f, 0.0f, 0.0f);

        /// Opaque red color.
        public static readonly Color Red = new Color(1.0f, 0.0f, 0.0f);

        /// Opaque green color.
        public static readonly Color Green = new Color(0.0f, 1.0f, 0.0f);

        /// Opaque blue color.
        public static readonly Color Blue = new Color(0.0f, 0.0f, 1.0f);

        /// Opaque cyan color.
        public static readonly Color Cyan = new Color(0.0f, 1.0f, 1.0f);

        /// Opaque magenta color.
        public static readonly Color Magenta = new Color(1.0f, 0.0f, 1.0f);

        /// Opaque yellow color.
        public static readonly Color Yellow = new Color(1.0f, 1.0f, 0.0f);

        /// Transparent color (black with no alpha).
        public static readonly Color TransparentBlack = new Color(0.0f, 0.0f, 0.0f, 0.0f);

        /// Transparent color (black with no alpha).
        public static readonly Color Transparent = TransparentBlack;
    }
}
