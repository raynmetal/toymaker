/*
Include: 
    -common/versionHeader.glsl
    -common/lightStruct.fs
    -common/geometrySampler.fs
    -common/fragmentAttributes.fs
*/

uniform float uBrightCutoff = 1.f;

layout (location=0) out vec4 outColor;
layout (location=1) out vec4 brightColor;

void main() {
    vec2 textureCoordinates = gl_FragCoord.xy/vec2(uScreenWidth, uScreenHeight);

    // Sample gBuffer textures
    vec4 gPosition = texture(uGeometryPositionMap, textureCoordinates);
    vec4 gNormal = normalize(texture(uGeometryNormalMap, textureCoordinates));
    vec4 gAlbedoSpec = texture(uGeometryAlbedoSpecMap, textureCoordinates);
    vec4 gAlbedo = vec4(gAlbedoSpec.rgb, 1.f);
    vec4 gSpecular = vec4(vec3(gAlbedoSpec.a), 1.f);

    // Common intermediate calculations
    vec4 eyeDir = vec4(normalize(vec3(-gPosition)), 0.f); //fragment -> eye
    vec4 incidentDir = normalize(gPosition - fragAttrLightPlacement.mPosition); //light -> fragment
    if(fragAttrLightEmission.mType == 0) incidentDir = normalize(fragAttrLightPlacement.mDirection);
    float lightDistance = length(gPosition - fragAttrLightPlacement.mPosition);
    vec4 halfwayDir = normalize(eyeDir + (-incidentDir));

    float factorDiffuse = max(dot(gNormal, -incidentDir), 0.f);
    float factorSpecular = factorDiffuse <= 0.f? 0.f: (
        pow(
            max(dot(halfwayDir, gNormal), 0.f),
            16.f
        )
    );
    float factorAttenuation = 1.f;
    float factorSpotIntensity = 1.f;
    switch(fragAttrLightEmission.mType) {
        case 2: // spot light
            float cosTheta = dot(fragAttrLightPlacement.mDirection, incidentDir);
            factorSpotIntensity = clamp(
                (cosTheta - fragAttrLightEmission.mCosCutoffOuter) 
                    / (fragAttrLightEmission.mCosCutoffInner - fragAttrLightEmission.mCosCutoffOuter),
                0.f,
                1.f
            );

        //  | | | -- flow through
        //  v v v 

        case 1: // point light
            // both point and spot lights experience attenuation
            factorAttenuation = (
                1.f
                / (
                    1.f
                    + fragAttrLightEmission.mDecayLinear * lightDistance
                    + fragAttrLightEmission.mDecayQuadratic * lightDistance * lightDistance
                )
            );
        break;
    }

    vec4 componentDiffuse = factorDiffuse * fragAttrLightEmission.mDiffuseColor * gAlbedo;
    vec4 componentSpecular = factorSpecular * fragAttrLightEmission.mSpecularColor * gSpecular;
    vec4 componentAmbient = fragAttrLightEmission.mAmbientColor * gAlbedo;

    outColor = ((componentDiffuse) + (componentSpecular) + (componentAmbient)) * factorAttenuation * factorSpotIntensity;

    outColor.a = dot(gPosition, gPosition) > 0.f? 1.f: 0.f;

    if(dot(outColor.xyz, vec3(.2f, .7f, .1f)) >= uBrightCutoff) {
        brightColor = outColor;
    }
}
