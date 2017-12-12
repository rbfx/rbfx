vec4 vScreenPos     : TEXCOORD0 = vec4(0.0, 0.0, 0.0, 0.0);
vec2 vReflectUV     : TEXCOORD1 = vec2(0.0, 0.0);
vec2 vWaterUV       : TEXCOORD2 = vec2(0.0, 0.0);
vec3 vNormal        : NORMAL    = vec3(0.0, 0.0, 1.0);
vec4 vEyeVec        : TEXCOORD4 = vec4(0.0, 0.0, 0.0, 0.0);

vec4 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec2 a_texcoord0 : TEXCOORD0;

vec4 i_data0     : TEXCOORD3;
vec4 i_data1     : TEXCOORD4;
vec4 i_data2     : TEXCOORD5;
vec4 i_data3     : TEXCOORD6;
vec4 i_data4     : TEXCOORD7;
vec4 i_data5     : TEXCOORD0;
vec4 i_data6     : TEXCOORD1;
vec4 i_data7     : TEXCOORD2;
