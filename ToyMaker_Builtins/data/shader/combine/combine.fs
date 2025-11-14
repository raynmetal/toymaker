uniform int nCombine=0;
uniform sampler2D uCombineTextures[4];

out vec4 outColor;

void main() {
    for(int i = 0; i < nCombine; ++i) {
        vec4 sampledColor = texture(uCombineTextures[i], fragAttr.UV1);
        if(sampledColor.a > 1.f) sampledColor.a = 1.f;

        vec3 rgbComp = sampledColor.rgb * sampledColor.a + outColor.rgb * (1.f - sampledColor.a);
        float aComp = max(sampledColor.a, outColor.a);
        outColor = vec4(rgbComp, aComp);
    }
}

