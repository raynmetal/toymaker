/*
Include:
    -common/versionHeader.glsl
    -common/lightStruct.vs
    -common/projectionViewMatrices.vs
    -common/modelNormalMatrices.vs
    -common/fragmentAttributes.vs
    -common/vertexAttributes.vs
*/


void main() {
    // Light volume calculations
    if(attrLightEmission_mType != 0) { // Lights which are not directional require perspective transformations
        gl_Position = projectionMatrix * viewMatrix * attrModelMatrix * attrPosition;
    } else { // pass vertices as is, light covers the whole screen
        gl_Position = mat4(1.f) * attrPosition;
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
