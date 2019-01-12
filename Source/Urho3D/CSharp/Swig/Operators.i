
%rename(Equals) operator==;
%rename(Set) operator=;
%rename(At) operator[];
%ignore operator!=;


%define AddEqualityOperators(fqn)
    %typemap(cscode) fqn %{
        public static bool operator ==($typemap(cstype, fqn) a, $typemap(cstype, fqn) b)
        {
            if (!ReferenceEquals(a, null))
                return !ReferenceEquals(b, null) && a.Equals(b);     // Equality checked when both not null.
            return b == null;                                        // true when both are null
        }

        public static bool operator !=($typemap(cstype, fqn) a, $typemap(cstype, fqn) b)
        {
            return !(a == b);
        }

        public override bool Equals(object other)
        {
            return Equals(other as $typemap(cstype, fqn));
        }
    %}
%enddef

AddEqualityOperators(Urho3D::ResourceRef);
AddEqualityOperators(Urho3D::ResourceRefList);
AddEqualityOperators(Urho3D::AttributeInfo);
AddEqualityOperators(Urho3D::Splite);
AddEqualityOperators(Urho3D::JSONValue);
AddEqualityOperators(Urho3D::PListValue);
AddEqualityOperators(Urho3D::VertexElement);
AddEqualityOperators(Urho3D::RenderTargetInfo);
AddEqualityOperators(Urho3D::RenderPathCommand);
AddEqualityOperators(Urho3D::Bone);
AddEqualityOperators(Urho3D::ModelMorph);
AddEqualityOperators(Urho3D::AnimationKeyFrame);
AddEqualityOperators(Urho3D::AnimationTrack);
AddEqualityOperators(Urho3D::AnimationTriggerPoint);
AddEqualityOperators(Urho3D::AnimationControl);
AddEqualityOperators(Urho3D::Billboard);
AddEqualityOperators(Urho3D::DecalVertex);
AddEqualityOperators(Urho3D::TechniqueEntry);
AddEqualityOperators(Urho3D::CustomGeometryVertex);
AddEqualityOperators(Urho3D::ColorFrame);
AddEqualityOperators(Urho3D::TextureFrame);
AddEqualityOperators(Urho3D::Variant);
