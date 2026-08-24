#pragma once

#include <array>

#include <glm/glm.hpp>

class TransformFrameState
{
public:
    virtual ~TransformFrameState() = default;

    glm::vec3 position{0.0f};
    glm::vec3 size{1.0f};
    glm::vec3 size2{1.0f};
    glm::mat4 Transform{1.0f};
    glm::mat4 Transform2{1.0f};
};

class RotatableTransformFrameState : public TransformFrameState
{
public:
    virtual ~RotatableTransformFrameState() = default;

    glm::vec3 getEulerDegrees() const
    {
        return glm::vec3(anglex, angley, anglez);
    }

protected:
    float anglex = 0.0f;
    float angley = 0.0f;
    float anglez = 0.0f;
    glm::vec3 x{1.0f, 0.0f, 0.0f};
    glm::vec3 y{0.0f, 1.0f, 0.0f};
    glm::vec3 z{0.0f, 0.0f, 1.0f};
};

class DualAxisTransformFrameState : public RotatableTransformFrameState
{
public:
    virtual ~DualAxisTransformFrameState() = default;

    const std::array<glm::vec3, 3> &getDualScales() const
    {
        return dual_scales;
    }

    const std::array<glm::vec3, 3> &getDualTranslations() const
    {
        return dual_translations;
    }

protected:
    std::array<glm::vec3, 3> dual_scales{
        glm::vec3(1.0f), glm::vec3(1.0f), glm::vec3(1.0f)};
    std::array<glm::vec3, 3> dual_translations{
        glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(0.0f)};
};

class SpatialTransformScope : public DualAxisTransformFrameState
{
public:
    SpatialTransformScope();
    SpatialTransformScope(SpatialTransformScope *other);
    void translate(const glm::vec3 &translation);
    void scalePrimary(const glm::vec3 &size);
    void scaleSecondary(const glm::vec3 &size);
    void scaleDualAxis(int axis, const glm::vec3 &size);
    void translateDualAxis(int axis, const glm::vec3 &translation);
    void rotateAroundX(float angle);
    void rotateAroundY(float angle);
    void rotateAroundZ(float angle);
    void T(const glm::vec3 &translation);
    void S(const glm::vec3 &size);
    void D(const glm::vec3 &size);
    void DS(int axis, const glm::vec3 &size);
    void DT(int axis, const glm::vec3 &translation);
    void Rx(float angle);
    void Ry(float angle);
    void Rz(float angle);
    const glm::mat4 &getTransform() const;
    const glm::mat4 &getTransform2() const;
    const std::array<glm::vec3, 3> &getDualScales() const;
    const std::array<glm::vec3, 3> &getDualTranslations() const;
    const glm::vec3 &getPosition() const;
    const glm::vec3 &getSize() const;
    const glm::vec3 &getSize2() const;
    glm::vec3 getEulerDegrees() const;
    glm::vec3 setPosition(glm::vec3 pos);
};

using Scope = SpatialTransformScope;
