/*
INCLUDE
    - common/versionHeader.glsl
    - common/fragmentAttributes.fs
    - common/genericSampler.fs
*/


uniform bool uHorizontal;
uniform float weights[5] = {.227027f, .1945946f, .1216216f, .054054f, .016216f};

out vec4 outColor;

void main() {
    vec2 offset;
    if(uHorizontal) {
        offset = vec2(1.f, 0.f);
    } else {
        offset = vec2(0.f, 1.f);
    }
    offset /= textureSize(uGenericTexture, 0);

    outColor = vec4(0.f);
    for(int i = 0; i < 5; ++i) {
        vec4 sampleOne = texture(uGenericTexture, i * offset + fragAttr.UV1);
        vec4 sampleTwo = texture(uGenericTexture, -i * offset + fragAttr.UV1);
        outColor += vec4(
            weights[i] * (sampleOne.rgb + sampleTwo.rgb), // gaussian/bell-curve for color
            .25f * (1.f - .2f*i) * (sampleOne.a + sampleTwo.a) // linear for alpha
        );
    }
}
