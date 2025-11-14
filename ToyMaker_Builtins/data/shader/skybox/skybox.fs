/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.fs
*/

uniform sampler2D textureSkybox;

out layout(location=0) vec4 outColor;

void main() {
    outColor = texture(textureSkybox, fragAttr.UV1);
}
