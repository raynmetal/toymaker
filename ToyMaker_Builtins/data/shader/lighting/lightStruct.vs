layout(location=0) in vec4 attrPosition;
in vec4 attrLightPlacement_mPosition; // loc 11
in vec4 attrLightPlacement_mDirection; // loc 12

in vec4 attrLightEmission_mDiffuseColor; // loc 13
in vec4 attrLightEmission_mSpecularColor; // loc 14
in vec4 attrLightEmission_mAmbientColor; // loc 15
in int attrLightEmission_mType; // loc 16 --- 0-directional;  1-point;  2-spot;
in float attrLightEmission_mDecayLinear; // loc 17
in float attrLightEmission_mDecayQuadratic; // loc 18
in float attrLightEmission_mCosCutoffInner; // loc 19
in float attrLightEmission_mCosCutoffOuter; // loc 20

out FragLightPlacement {
    vec4 mVolumePosition;
    flat vec4 mPosition;
    flat vec4 mDirection;
} fragAttrLightPlacement;

out FragLightEmission {
    flat vec4 mDiffuseColor; // loc 13
    flat vec4 mSpecularColor; // loc 14
    flat vec4 mAmbientColor; // loc 15

    flat int mType; // loc 16 --- 0-directional;  1-point;  2-spot;
    flat float mDecayLinear; // loc 17
    flat float mDecayQuadratic; // loc 18
    flat float mCosCutoffInner; // loc 19
    flat float mCosCutoffOuter; // loc 20
} fragAttrLightEmission;
