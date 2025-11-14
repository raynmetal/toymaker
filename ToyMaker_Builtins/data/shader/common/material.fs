struct Material {
    sampler2D mTextureAlbedo;
    sampler2D mTextureSpecular;
    sampler2D mTextureNormal;
    bool mUsingNormalMap;
    bool mUsingAlbedoMap;
    bool mUsingSpecularMap;
    float mSpecularExponent;
    vec4 mColorMultiplier;
};

uniform Material uMaterial;
