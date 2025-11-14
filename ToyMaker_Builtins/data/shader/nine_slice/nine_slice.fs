/*
Include:
    - common/versionHeader.glsl
    - common/fragmentAttributes.fs
    - common/genericSampler.fs
*/

uniform vec2 uPanelTextureDimensions;
uniform vec2 uTargetTextureDimensions;

uniform vec2 uPanelContentUVStart;
uniform vec2 uPanelContentUVEnd;

out vec4 outColor;

float mapToRange(float value, float sourceMin, float sourceMax, float destinationMin, float destinationMax) {
    return destinationMin + ((value - sourceMin) * (destinationMax - destinationMin) / (sourceMax - sourceMin));
}

vec2 mapToNewUV(vec2 oldUV) {
    vec2 panelContentPixelStart = uPanelContentUVStart * uPanelTextureDimensions;
    vec2 panelContentPixelEnd = uPanelContentUVEnd * uPanelTextureDimensions;
    vec2 targetContentPixelStart = panelContentPixelStart;
    vec2 targetContentPixelEnd = uTargetTextureDimensions - (uPanelTextureDimensions - panelContentPixelEnd);

    vec2 targetPixel = oldUV * uTargetTextureDimensions;

    vec2 srcStart;
    vec2 srcEnd;
    vec2 dstStart;
    vec2 dstEnd;

    if(targetPixel.x <= targetContentPixelStart.x) {
        srcStart.x = 0.f;
        srcEnd.x = targetContentPixelStart.x;
        dstStart.x = 0.f;
        dstEnd.x = uPanelContentUVStart.x;
    } else if(targetPixel.x >= targetContentPixelEnd.x) {
        srcStart.x = targetContentPixelEnd.x;
        srcEnd.x = uTargetTextureDimensions.x;
        dstStart.x = uPanelContentUVEnd.x;
        dstEnd.x = 1.f;
    } else {
        srcStart.x = targetContentPixelStart.x;
        srcEnd.x = targetContentPixelEnd.x;
        dstStart.x = uPanelContentUVStart.x;
        dstEnd.x = uPanelContentUVEnd.x;
    }

    if(targetPixel.y <= targetContentPixelStart.y) {
        srcStart.y = 0.f;
        srcEnd.y = targetContentPixelStart.y;
        dstStart.y = 0.f;
        dstEnd.y = uPanelContentUVStart.y;
    } else if(targetPixel.y >= targetContentPixelEnd.y) {
        srcStart.y = targetContentPixelEnd.y;
        srcEnd.y = uTargetTextureDimensions.y;
        dstStart.y = uPanelContentUVEnd.y;
        dstEnd.y = 1.f;
    } else {
        srcStart.y = targetContentPixelStart.y;
        srcEnd.y = targetContentPixelEnd.y;
        dstStart.y = uPanelContentUVStart.y;
        dstEnd.y = uPanelContentUVEnd.y;
    }

    return vec2(
        mapToRange(targetPixel.x, srcStart.x, srcEnd.x, dstStart.x, dstEnd.x),
        mapToRange(targetPixel.y, srcStart.y, srcEnd.y, dstStart.y, dstEnd.y)
    );
}

void main() {
    outColor = texture(uGenericTexture, mapToNewUV(fragAttr.UV1));
}
