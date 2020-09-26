template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
operator Urho3D::Vector4() const { return Urho3D::Vector4(x, y, z, w); }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
Vector4(Urho3D::Vector4 value) : Vector4(value.x_, value.y_, value.z_, value.w_) { }
