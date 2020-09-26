template<typename U = ColourType, typename std::enable_if_t<std::is_same_v<U, byte>>* = nullptr>
operator Urho3D::Color() const { return Urho3D::Color(
	(float)red / 256.0f, (float)green / 256.0f, (float)blue / 256.0f, (float)alpha / 256.0f); }
template<typename U = ColourType, typename std::enable_if_t<std::is_same_v<U, float>>* = nullptr>
operator Urho3D::Color() const { return Urho3D::Color(red, green, blue, alpha); }
