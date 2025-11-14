/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.fs
    - common/genericSampler.fs
*/

uniform float uExposure;
uniform float uGamma;
uniform bool uCombine;

out vec4 outColor;

void main() {
    vec4 inColor = texture(uGenericTexture, fragAttr.UV1);
    if(uCombine) inColor += texture(uGenericTexture1, fragAttr.UV1);
    outColor = 1.f - exp(uExposure * (-inColor));
    outColor = pow(outColor, vec4(1.f/uGamma));
    outColor.a = inColor.a;
    // outColor = vec4(inColor.a);
}
