/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.fs
    - common/genericSampler.fs
*/

out vec4 outColor;

void main() {
    outColor = texture(uGenericTexture, fragAttr.UV1);
    // outColor = vec4(1.f);
}
