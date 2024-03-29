#pragma once

#include "types.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace pl {

struct Camera {
    // https://developer.nvidia.com/content/depth-precision-visualized
    float fovy { 45.0f };
    float znear { 0.1f };
    float zfar { 1000.0f };

    float aspect;

    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;

    glm::vec3 forward;
    glm::vec3 right;

    glm::mat4 view { 1.0f };
    glm::mat4 proj { 1.0f };

    void lookAt(const glm::vec3& _eye, const glm::vec3& _center, const glm::vec3& _up)
    {
        eye = _eye;
        center = _center;

        up = glm::normalize(_up);
        forward = glm::normalize(center - eye);
        right = glm::normalize(glm::cross(forward, up));

        view = glm::lookAt(eye, center, up);
    }

    void resize(float _aspect)
    {
        aspect = _aspect;
        proj = glm::perspective(fovy, _aspect, zfar, znear);
    }

    void rotate(int dx, int dy)
    {
        forward = glm::rotate(forward, -(float)dx / 350.0f, up);
        right = glm::rotate(right, -(float)dx / 350.0f, up);

        float angle = glm::angle(forward, up);
        if ((dy < 0 && angle > 0.1f) || (dy > 0 && angle < glm::pi<float>() - 0.1f))
            forward = glm::rotate(forward, -(float)dy / 350.0f, right);

        center = eye + forward * glm::length(center - eye);
        lookAt(eye, center, up);
    }

    void zoom(float dz)
    {
    }

    void move(glm::ivec4 wasd, glm::ivec2 spacelctrl, float speed)
    {
        eye += (float)wasd[0] * forward * speed;
        eye -= (float)wasd[1] * right * speed;
        eye -= (float)wasd[2] * forward * speed;
        eye += (float)wasd[3] * right * speed;
        center += (float)wasd[0] * forward * speed;
        center -= (float)wasd[1] * right * speed;
        center -= (float)wasd[2] * forward * speed;
        center += (float)wasd[3] * right * speed;
        eye += (float)spacelctrl[0] * up * speed;
        eye -= (float)spacelctrl[1] * up * speed;
        center += (float)spacelctrl[0] * up * speed;
        center -= (float)spacelctrl[1] * up * speed;

        lookAt(eye, center, up);
    }

    void reset()
    {
    }
};

}
