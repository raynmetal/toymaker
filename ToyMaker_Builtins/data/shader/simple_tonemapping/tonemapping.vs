/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.vs
    - common/vertexAttributes.vs
*/

void main() {
    fragAttr.position = attrPosition;
    fragAttr.UV1= attrUV1;

    gl_Position = fragAttr.position;
}
