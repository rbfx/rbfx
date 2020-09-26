template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>
operator typename Urho3D::IntVector2() const { return Urho3D::IntVector2(x, y); }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
operator Urho3D::Vector2() const { return Urho3D::Vector2(x, y); }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, int>>* = nullptr>
Vector2(Urho3D::IntVector2 value) : Vector2(value.x_, value.y_) { }
template<typename U = Type, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
Vector2(Urho3D::Vector2 value) : Vector2(value.x_, value.y_) { }
