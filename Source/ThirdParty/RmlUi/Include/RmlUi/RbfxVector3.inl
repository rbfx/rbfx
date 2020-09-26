template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>
operator typename Urho3D::IntVector3() const { return Urho3D::IntVector3(x, y, z); }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
operator Urho3D::Vector3() const { return Urho3D::Vector3(x, y, z); }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>
Vector3(Urho3D::IntVector3 value) : Vector3(value.x_, value.y_, value.z_) { }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
Vector3(Urho3D::Vector3 value) : Vector3(value.x_, value.y_, value.z_) { }
