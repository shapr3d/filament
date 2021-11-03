#include <viewer/TweakableMaterial.h>

void TweakableMaterial::drawUI() {
    mBaseColor.addWidget("baseColor");
    mRoughness.addWidget("roughness");
    mClearCoat.addWidget("clearCoat");
}