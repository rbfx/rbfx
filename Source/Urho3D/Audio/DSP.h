#pragma once

namespace Urho3D
{

class URHO3D_API DSPFilter : Object
{
    URHO3D_OBJECT(DSPFilter, Object);
public:

    bool IsSixteenBit() const;

    unsigned char* Filter(unsigned* pos, unsigned* end, float pan, float reach, float azimuth) = 0;

private:

};

class URHO3D_API LowPassFilter : DSPFilter {
public:
};

class URHO3D_API HighPassFilter : DSPFilter {

};

class URHO3D_API AllPassFilter : DSPFilter {

};

class URHO3D_API FlangerFilter : DSPFilter {

};

class URHO3D_API DistortionFilter : DSPFilter {

};

class URHO3D_API RadioFilter : DSPFilter {

};

class URHO3D_API PitchFilter : DSPFilter {

};

class URHO3D_API ReverbFilter : DSPFilter
{
public:
};

class URHO3D_API HRTFFilter : DSPFilter {
public:
};

}
