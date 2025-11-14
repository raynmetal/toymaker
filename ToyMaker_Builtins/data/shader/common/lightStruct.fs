in FragLightPlacement{
    vec4 mPosition; // loc 11
    vec4 mDirection; // loc 12
} fragAttrLightPlacement;

in FragLightEmission{
    vec4 mDiffuseColor; // loc 13
    vec4 mSpecularColor; // loc 14
    vec4 mAmbientColor; // loc 15

    flat int mType; // loc 16 --- 0-directional;  1-point;  2-spot;
    float mDecayLinear; // loc 17
    float mDecayQuadratic; // loc 18
    float mCosCutoffInner; // loc 19
    float mCosCutoffOuter; // loc 20
} fragAttrLightEmission;
