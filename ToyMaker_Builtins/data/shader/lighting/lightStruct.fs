in FragLightPlacement{
    vec4 mVolumePosition;
    flat vec4 mPosition; // loc 11
    flat vec4 mDirection; // loc 12
} fragAttrLightPlacement;

in FragLightEmission{
    flat vec4 mDiffuseColor; // loc 13
    flat vec4 mSpecularColor; // loc 14
    flat vec4 mAmbientColor; // loc 15

    flat int mType; // loc 16 --- 0-directional;  1-point;  2-spot;
    flat float mDecayLinear; // loc 17
    flat float mDecayQuadratic; // loc 18
    flat float mCosCutoffInner; // loc 19
    flat float mCosCutoffOuter; // loc 20
} fragAttrLightEmission;
