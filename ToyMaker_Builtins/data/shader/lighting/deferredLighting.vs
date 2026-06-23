void main() {
    // Light volume calculations
    if(attrLightEmission_mType != 0) { // Lights which are not directional require perspective transformations
        fragAttrLightPlacement.mVolumePosition = viewMatrix * attrModelMatrix * attrPosition;
        gl_Position = projectionMatrix * fragAttrLightPlacement.mVolumePosition;

        // project light into near plane
        gl_Position.xy /= gl_Position.w;
        gl_Position.w = 1.f;
        gl_Position.z = 0.f; 
    } else { // pass vertices as is, light covers the whole screen
        fragAttrLightPlacement.mVolumePosition = attrPosition;
        gl_Position = fragAttrLightPlacement.mVolumePosition;
    }

    // Light placement calculations
    fragAttrLightPlacement.mPosition = viewMatrix * attrLightPlacement_mPosition;
    fragAttrLightPlacement.mDirection = viewMatrix * attrLightPlacement_mDirection;

    // Light emission, passed on as is
    fragAttrLightEmission.mDiffuseColor = attrLightEmission_mDiffuseColor;
    fragAttrLightEmission.mSpecularColor = attrLightEmission_mSpecularColor;
    fragAttrLightEmission.mAmbientColor = attrLightEmission_mAmbientColor;
    fragAttrLightEmission.mType = attrLightEmission_mType;
    fragAttrLightEmission.mDecayLinear = attrLightEmission_mDecayLinear;
    fragAttrLightEmission.mDecayQuadratic = attrLightEmission_mDecayQuadratic;
    fragAttrLightEmission.mCosCutoffInner = attrLightEmission_mCosCutoffInner;
    fragAttrLightEmission.mCosCutoffOuter = attrLightEmission_mCosCutoffOuter;
}
