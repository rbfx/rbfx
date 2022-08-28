//
// Copyright (c) 2015 Xamarin Inc.
// 
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//   
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//

using System;

namespace Urho3DNet
{
    public static class EaseMath
    {
        public static float BackIn(float time)
        {
            const float overshoot = 1.70158f;

            return time * time * ((overshoot + 1) * time - overshoot);
        }

        public static float BackOut(float time)
        {
            const float overshoot = 1.70158f;

            time = time - 1;
            return time * time * ((overshoot + 1) * time + overshoot) + 1;
        }

        public static float BackInOut(float time)
        {
            const float overshoot = 1.70158f * 1.525f;

            time = time * 2;
            if (time < 1)
            {
                return (time * time * ((overshoot + 1) * time - overshoot)) / 2;
            }
            else
            {
                time = time - 2;
                return (time * time * ((overshoot + 1) * time + overshoot)) / 2 + 1;
            }
        }

        public static float BounceOut(float time)
        {
            if (time < 1 / 2.75)
            {
                return 7.5625f * time * time;
            }
            else if (time < 2 / 2.75)
            {
                time -= 1.5f / 2.75f;
                return 7.5625f * time * time + 0.75f;
            }
            else if (time < 2.5 / 2.75)
            {
                time -= 2.25f / 2.75f;
                return 7.5625f * time * time + 0.9375f;
            }

            time -= 2.625f / 2.75f;
            return 7.5625f * time * time + 0.984375f;
        }

        public static float BounceIn(float time)
        {
            return 1f - BounceOut(1f - time);
        }

        public static float BounceInOut(float time)
        {
            if (time < 0.5f)
            {
                time = time * 2;
                return (1 - BounceOut(1 - time)) * 0.5f;
            }
            return BounceOut(time * 2 - 1) * 0.5f + 0.5f;
        }

        public static float SineOut(float time)
        {
            return (float)Math.Sin(time * MathDefs.PiOver2);
        }

        public static float SineIn(float time)
        {
            return -1f * (float)Math.Cos(time * MathDefs.PiOver2) + 1f;
        }

        public static float SineInOut(float time)
        {
            return -0.5f * ((float)Math.Cos((float)Math.PI * time) - 1f);
        }

        public static float ExponentialOut(float time)
        {
            return time == 1f ? 1f : (-(float)Math.Pow(2f, -10f * time / 1f) + 1f);
        }

        public static float ExponentialIn(float time)
        {
            return time == 0f ? 0f : (float)Math.Pow(2f, 10f * (time / 1f - 1f)) - 1f * 0.001f;
        }

        public static float ExponentialInOut(float time)
        {
            time /= 0.5f;
            if (time < 1)
            {
                return 0.5f * (float)Math.Pow(2f, 10f * (time - 1f));
            }
            else
            {
                return 0.5f * (-(float)Math.Pow(2f, -10f * (time - 1f)) + 2f);
            }
        }

        public static float ElasticIn(float time, float period)
        {
            if (time == 0 || time == 1)
            {
                return time;
            }
            else
            {
                float s = period / 4;
                time = time - 1;
                return -(float)(Math.Pow(2, 10 * time) * Math.Sin((time - s) * MathDefs.Pi * 2.0f / period));
            }
        }

        public static float ElasticOut(float time, float period)
        {
            if (time == 0 || time == 1)
            {
                return time;
            }
            else
            {
                float s = period / 4;
                return (float)(Math.Pow(2, -10 * time) * Math.Sin((time - s) * MathDefs.Pi * 2f / period) + 1);
            }
        }

        public static float ElasticInOut(float time, float period)
        {
            if (time == 0 || time == 1)
            {
                return time;
            }
            else
            {
                time = time * 2;
                if (period == 0)
                {
                    period = 0.3f * 1.5f;
                }

                float s = period / 4;

                time = time - 1;
                if (time < 0)
                {
                    return (float)(-0.5f * Math.Pow(2, 10 * time) * Math.Sin((time - s) * MathDefs.TwoPi / period));
                }
                else
                {
                    return (float)(Math.Pow(2, -10 * time) * Math.Sin((time - s) * MathDefs.TwoPi / period) * 0.5f + 1);
                }
            }
        }

    }
}
