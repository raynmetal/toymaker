/*
INCLUDE:
    - common/versionHeader.glsl
    - common/fragmentAttributes.vs
    - common/vertexAttributes.vs
*/

void main() {
    fragAttr.UV1 = attrUV1;
    gl_Position = attrPosition;
}
