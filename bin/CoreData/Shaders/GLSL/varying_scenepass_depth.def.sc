vec3 vTexCoord      : TEXCOORD0 = vec3(0.0, 0.0, 0.0);

vec4 a_position  : POSITION;
vec3 a_normal    : NORMAL;
vec2 a_texcoord0 : TEXCOORD0;
vec4 a_color     : COLOR0;
vec2 a_texcoord1 : TEXCOORD1;
vec4 a_tangent   : TANGENT;
vec4 a_weight    : BLENDWEIGHT;
ivec4 a_indices  : BLENDINDICES;

vec4 i_data0     : TEXCOORD4;
vec4 i_data1     : TEXCOORD5;
vec4 i_data2     : TEXCOORD6;
vec4 i_data3     : TEXCOORD7;
vec4 i_data4     : TEXCOORD0;
vec4 i_data5     : TEXCOORD1;
vec4 i_data6     : TEXCOORD2;
vec4 i_data7     : TEXCOORD3;
