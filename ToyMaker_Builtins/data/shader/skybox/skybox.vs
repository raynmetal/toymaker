/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.vs
    - common/vertexAttributes.vs
    - common/projectionViewMatrices.vs
*/

void main() {
    // isolate the rotation of the view matrix
    mat4 viewRotationMatrix = transpose(inverse(viewMatrix));

    // fragment attributes, in terms of camera space
    fragAttr.position = viewRotationMatrix * attrPosition;

    // color related fragment attributes
    fragAttr.UV1 = attrUV1;

    // The depth of the skybox is the max possible depth for the frustum, exactly at
    // the far plane relative to the camera.
    gl_Position = (projectionMatrix * fragAttr.position).xyww;
}
