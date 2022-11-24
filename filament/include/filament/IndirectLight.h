/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//! \file

#ifndef TNT_FILAMENT_INDIRECTLIGHT_H
#define TNT_FILAMENT_INDIRECTLIGHT_H

#include <filament/FilamentAPI.h>
#include <filament/Options.h>

#include <utils/compiler.h>

#include <math/mathfwd.h>

namespace filament {

class Engine;
class Texture;

class FIndirectLight;

/**
 * IndirectLight is used to simulate environment lighting, a form of global illumination.
 *
 * Environment lighting has a two components:
 *  1. irradiance
 *  2. reflections (specular component)
 *
 * Environments are usually captured as high-resolution HDR equirectangular images and processed
 * by the **cmgen** tool to generate the data needed by IndirectLight.
 *
 * @note
 * Currently IndirectLight is intended to be used for "distant probes", that is, to represent
 * global illumination from a distant (i.e. at infinity) environment, such as the sky or distant
 * mountains. Only a single IndirectLight can be used in a Scene. This limitation will be lifted
 * in the future.
 *
 * Creation and destruction
 * ========================
 *
 * An IndirectLight object is created using the IndirectLight::Builder and destroyed by calling
 * Engine::destroy(const IndirectLight*).
 *
 * ~~~~~~~~~~~{.cpp}
 *  filament::Engine* engine = filament::Engine::create();
 *
 *  filament::IndirectLight* environment = filament::IndirectLight::Builder()
 *              .reflections(cubemap)
 *              .build(*engine);
 *
 *  engine->destroy(environment);
 * ~~~~~~~~~~~
 *
 *
 * Irradiance
 * ==========
 *
 * The irradiance represents the light that comes from the environment and shines an
 * object's surface.
 *
 * The irradiance is calculated automatically from the Reflections (see below), and generally
 * doesn't need to be provided explicitly.  However, it can be provided separately from the
 * Reflections as
 * [spherical harmonics](https://en.wikipedia.org/wiki/Spherical_harmonics) (SH) of 1, 2 or
 * 3 bands, respectively 1, 4 or 9 coefficients.
 *
 * @note
 * Use the **cmgen** tool to generate the `SH` for a given environment.
 *
 * Reflections
 * ===========
 *
 * The reflections on object surfaces (specular component) is calculated from a specially
 * filtered cubemap pyramid generated by the **cmgen** tool.
 *
 *
 * @see Scene, Light, Texture, Skybox
 */
class UTILS_PUBLIC IndirectLight : public FilamentAPI {
    struct BuilderDetails;

public:

    //! Use Builder to construct an IndirectLight object instance
    class Builder : public BuilderBase<BuilderDetails> {
        friend struct BuilderDetails;
    public:
        Builder() noexcept;
        Builder(Builder const& rhs) noexcept;
        Builder(Builder&& rhs) noexcept;
        ~Builder() noexcept;
        Builder& operator=(Builder const& rhs) noexcept;
        Builder& operator=(Builder&& rhs) noexcept;

        /**
         * Set the reflections cubemap mipmap chain.
         *
         * @param cubemap   A mip-mapped cubemap generated by **cmgen**. Each cubemap level
         *                  encodes a the irradiance for a roughness level.
         *
         * @return This Builder, for chaining calls.
         *
         */
        Builder& reflections(Texture const* cubemap) noexcept;

        /**
         * Sets the irradiance as Spherical Harmonics.
         *
         * The irradiance must be pre-convolved by \f$ \langle n \cdot l \rangle \f$ and
         * pre-multiplied by the Lambertian diffuse BRDF \f$ \frac{1}{\pi} \f$ and
         * specified as Spherical Harmonics coefficients.
         *
         * Additionally, these Spherical Harmonics coefficients must be pre-scaled by the
         * reconstruction factors \f$ A_{l}^{m} \f$ below.
         *
         * The final coefficients can be generated using the `cmgen` tool.
         *
         * The index in the \p sh array is given by:
         *
         *  `index(l, m) = l * (l + 1) + m`
         *
         *  \f$ sh[index(l,m)] = L_{l}^{m} \frac{1}{\pi} A_{l}^{m} \hat{C_{l}} \f$
         *
         *   index |  l  |  m  |  \f$ A_{l}^{m} \f$ |  \f$ \hat{C_{l}} \f$  |  \f$ \frac{1}{\pi} A_{l}^{m}\hat{C_{l}} \f$ |
         *  :-----:|:---:|:---:|:------------------:|:---------------------:|:--------------------------------------------:
         *     0   |  0  |  0  |      0.282095      |       3.1415926       |   0.282095
         *     1   |  1  | -1  |     -0.488602      |       2.0943951       |  -0.325735
         *     2   |  ^  |  0  |      0.488602      |       ^               |   0.325735
         *     3   |  ^  |  1  |     -0.488602      |       ^               |  -0.325735
         *     4   |  2  | -2  |      1.092548      |       0.785398        |   0.273137
         *     5   |  ^  | -1  |     -1.092548      |       ^               |  -0.273137
         *     6   |  ^  |  0  |      0.315392      |       ^               |   0.078848
         *     7   |  ^  |  1  |     -1.092548      |       ^               |  -0.273137
         *     8   |  ^  |  2  |      0.546274      |       ^               |   0.136569
         *
         *
         * Only 1, 2 or 3 bands are allowed.
         *
         * @param bands     Number of spherical harmonics bands. Must be 1, 2 or 3.
         * @param sh        Array containing the spherical harmonics coefficients.
         *                  The size of the array must be \f$ bands^{2} \f$.
         *                  (i.e. 1, 4 or 9 coefficients respectively).
         *
         * @return This Builder, for chaining calls.
         *
         * @note
         * Because the coefficients are pre-scaled, `sh[0]` is the environment's
         * average irradiance.
         */
        Builder& irradiance(uint8_t bands, math::float3 const* sh) noexcept;

        /**
         * Sets the irradiance from the radiance expressed as Spherical Harmonics.
         *
         * The radiance must be specified as Spherical Harmonics coefficients \f$ L_{l}^{m} \f$
         *
         * The index in the \p sh array is given by:
         *
         *  `index(l, m) = l * (l + 1) + m`
         *
         *  \f$ sh[index(l,m)] = L_{l}^{m} \f$
         *
         *   index |  l  |  m
         *  :-----:|:---:|:---:
         *     0   |  0  |  0
         *     1   |  1  | -1
         *     2   |  ^  |  0
         *     3   |  ^  |  1
         *     4   |  2  | -2
         *     5   |  ^  | -1
         *     6   |  ^  |  0
         *     7   |  ^  |  1
         *     8   |  ^  |  2
         *
         * @param bands     Number of spherical harmonics bands. Must be 1, 2 or 3.
         * @param sh        Array containing the spherical harmonics coefficients.
         *                  The size of the array must be \f$ bands^{2} \f$.
         *                  (i.e. 1, 4 or 9 coefficients respectively).
         *
         * @return This Builder, for chaining calls.
         */
        Builder& radiance(uint8_t bands, math::float3 const* sh) noexcept;

        /**
         * Sets the irradiance as a cubemap.
         *
         * The irradiance can alternatively be specified as a cubemap instead of Spherical
         * Harmonics coefficients. It may or may not be more efficient, depending on your
         * hardware (essentially, it's trading ALU for bandwidth).
         *
         * @param cubemap   Cubemap representing the Irradiance pre-convolved by
         *                  \f$ \langle n \cdot l \rangle \f$.
         *
         * @return This Builder, for chaining calls.
         *
         * @note
         * This irradiance cubemap can be generated with the **cmgen** tool.
         *
         * @see irradiance(uint8_t bands, math::float3 const* sh)
         */
        Builder& irradiance(Texture const* cubemap) noexcept;

        /**
         * (optional) Environment intensity.
         *
         * Because the environment is encoded usually relative to some reference, the
         * range can be adjusted with this method.
         *
         * @param envIntensity  Scale factor applied to the environment and irradiance such that
         *                      the result is in lux, or lumen/m^2 (default = 30000)
         *
         * @return This Builder, for chaining calls.
         */
        Builder& intensity(float envIntensity) noexcept;

        /**
         * Specifies the rigid-body transformation to apply to the IBL.
         *
         * @param rotation 3x3 rotation matrix. Must be a rigid-body transform.
         *
         * @return This Builder, for chaining calls.
         */
        Builder& rotation(math::mat3f const& rotation) noexcept;

        /**
         * Creates the IndirectLight object and returns a pointer to it.
         *
         * @param engine Reference to the filament::Engine to associate this IndirectLight with.
         *
         * @return pointer to the newly created object or nullptr if exceptions are disabled and
         *         an error occurred.
         *
         * @exception utils::PostConditionPanic if a runtime error occurred, such as running out of
         *            memory or other resources.
         * @exception utils::PreConditionPanic if a parameter to a builder function was invalid.
         */
        IndirectLight* build(Engine& engine);

    private:
        friend class FIndirectLight;
    };

    /**
     * Sets the environment's intensity.
     *
     * Because the environment is encoded usually relative to some reference, the
     * range can be adjusted with this method.
     *
     * @param intensity  Scale factor applied to the environment and irradiance such that
     *                   the result is in lux, or <i>lumen/m^2</i> (default = 30000)
     */
    void setIntensity(float intensity) noexcept;

    /**
     * Returns the environment's intensity in <i>lux</i>, or <i>lumen/m^2</i>.
     */
    float getIntensity() const noexcept;

    /**
     * Sets the rigid-body transformation to apply to the IBL.
     *
     * @param rotation 3x3 rotation matrix. Must be a rigid-body transform.
     */
    void setRotation(math::mat3f const& rotation) noexcept;

    /**
     * Returns the rigid-body transformation applied to the IBL.
     */
    const math::mat3f& getRotation() const noexcept;

    /**
     * Returns the associated reflection map, or null if it does not exist.
     */
    Texture const* getReflectionsTexture() const noexcept;

    /**
     * Returns the associated irradiance map, or null if it does not exist.
     */
    Texture const* getIrradianceTexture() const noexcept;

    /**
     * Helper to estimate the direction of the dominant light in the environment represented by
     * spherical harmonics.
     *
     * This assumes that there is only a single dominant light (such as the sun in outdoors
     * environments), if it's not the case the direction returned will be an average of the
     * various lights based on their intensity.
     *
     * If there are no clear dominant light, as is often the case with low dynamic range (LDR)
     * environments, this method may return a wrong or unexpected direction.
     *
     * The dominant light direction can be used to set a directional light's direction,
     * for instance to produce shadows that match the environment.
     *
     * @param sh        3-band spherical harmonics
     *
     * @return A unit vector representing the direction of the dominant light
     *
     * @see LightManager::Builder::direction()
     * @see getColorEstimate()
     */
    static math::float3 getDirectionEstimate(const math::float3 sh[9]) noexcept;

    /**
     * Helper to estimate the color and relative intensity of the environment represented by
     * spherical harmonics in a given direction.
     *
     * This can be used to set the color and intensity of a directional light. In this case
     * make sure to multiply this relative intensity by the the intensity of this indirect light.
     *
     * @param sh        3-band spherical harmonics
     * @param direction a unit vector representing the direction of the light to estimate the
     *                  color of. Typically this the value returned by getDirectionEstimate().
     *
     * @return A vector of 4 floats where the first 3 components represent the linear color and
     *         the 4th component represents the intensity of the dominant light
     *
     * @see LightManager::Builder::color()
     * @see LightManager::Builder::intensity()
     * @see getDirectionEstimate, getIntensity, setIntensity
     */
    static math::float4 getColorEstimate(const math::float3 sh[9], math::float3 direction) noexcept;


    /** @deprecated use static versions instead */
    UTILS_DEPRECATED
    math::float3 getDirectionEstimate() const noexcept;

    /** @deprecated use static versions instead */
    UTILS_DEPRECATED
    math::float4 getColorEstimate(math::float3 direction) const noexcept;

    /**
     * Sets IBL lookup options.
     *
     * @param options Options for IBL lookup settings.
     */
    void setOptions(IblOptions const& options) noexcept;

    /**
     * Gets IBL lookup options.
     *
     * @return IBL lookup options.
     */
    IblOptions const& getOptions() const noexcept;

    /**
     * Sets IBL type.
     *
     * @param iblTechnique Determine the IBL type (infinite, or finite box/sphere)
     */
    void setTechnique(const IblOptions::IblTechnique iblTechnique) noexcept;

    /**
     * Gets IBL type.
     */
    IblOptions::IblTechnique getTechnique() const noexcept;

    /**
     * Sets the center of the finite IBL proxy geometry.
     *
     * @param iblCenter The center of the finite IBL proxy geometry
     */
    void setCenter(const math::float3& iblCenter) noexcept;

    /**
     * Gets IBL center.
     */
    const math::float3& getCenter() const noexcept;

    /**
     * Sets the half extents of the finite IBL proxy geometry.
     *
     * @param iblCenter The half extents of the finite IBL proxy geometry
     */
    void setHalfExtents(const math::float3& iblHalfExtents) noexcept;

    /**
     * Gets IBL half extents.
     */
    const math::float3& getHalfExtents() const noexcept;

    /**
     * Sets the IBL tint (.rgb) and blend intensity (.a)
     *
     * @param iblTintAndStrength The tint color (.rgb) and blend weight between shaded and tinted (.a)
     */
    void setTintAndStrength(const math::float4& iblTintAndStrength) noexcept;

    /**
     * Gets IBL tint (.rgb) and blend intensity (.a).
     */    
    const math::float4& getTintAndStrength() const noexcept;    
};

} // namespace filament

#endif // TNT_FILAMENT_INDIRECTLIGHT_H
