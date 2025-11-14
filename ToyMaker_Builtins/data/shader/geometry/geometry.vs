/*
Include:
    - common/versionHeader.glsl
    - common/vertexAttributes.vs
    - common/projectionViewMatrices.vs
    - common/modelNormalMatrices.vs
    - common/fragmentAttributes.vs
*/

void main() {
    // precomputed view and normal matrices
    mat4 viewModelMatrix = viewMatrix * attrModelMatrix;

    // fragment attributes, in terms of camera space
    fragAttr.position = viewModelMatrix * attrPosition;
    fragAttr.normal = viewModelMatrix * attrNormal;
    fragAttr.tangent = viewModelMatrix * attrTangent;

    // color related fragment attributes
    fragAttr.UV1 = attrUV1;
    fragAttr.color = attrColor;

    // pre-NDC position, after projection and before viewport
    // transform is applied
    gl_Position = projectionMatrix * fragAttr.position;
}
