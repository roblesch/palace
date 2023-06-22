#pragma once

#include "types.hpp"
#include <glm/gtc/constants.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/vector_angle.hpp>

namespace pl {

struct Camera {
    float fovy;
    float znear;
    float zfar;

    float aspect;

    glm::vec3 eye;
    glm::vec3 center;
    glm::vec3 up;

    glm::mat4 view { 1.0f };
    glm::mat4 proj { 1.0f };

    void lookAt(const glm::vec3& eye, const glm::vec3& center, const glm::vec3& up)
    {
        this->eye = eye;
        this->center = center;
        this->up = up;

        view = glm::lookAt(eye, center, up);
    }

    void resize(float aspect)
    {
        proj = glm::perspective(fovy, aspect, znear, zfar);
    }

    void rotate(int dx, int dy)
    {
        glm::vec3 forward = glm::normalize(center - eye);
        glm::vec3 right = glm::normalize(glm::cross(forward, up));
        float angle = glm::angle(forward, up);
        forward = glm::rotate(forward, dx / 350.0f, up);
        if ((dy > 0 && angle > 0.01) || (dy < 0 && angle < glm::pi<float>() - 0.01f))
            forward = glm::rotate(forward, dy / 350.0f, right);
        lookAt(eye, eye + glm::vec3(forward) * glm::length(center - eye), up);
    }

    void zoom(float dz)
    {
        glm::vec3 forward = center - eye;
        if (dz > 0 && glm::length(forward) < 0.1f)
            return;
        eye += dz * glm::normalize(forward);
        lookAt(eye, center, up);
    }

    void move(glm::vec4 wasd)
    {
        glm::vec3 forward = center - eye;
        glm::vec3 right = glm::cross(forward, up);
        eye += 0.1f * wasd[0] * glm::normalize(forward);
        eye += 0.1f * wasd[1] * glm::normalize(right);
        eye -= 0.1f * wasd[2] * glm::normalize(forward);
        eye -= 0.1f * wasd[3] * glm::normalize(right);
        center += 0.1f * wasd[0] * glm::normalize(forward);
        center += 0.1f * wasd[1] * glm::normalize(right);
        center -= 0.1f * wasd[2] * glm::normalize(forward);
        center -= 0.1f * wasd[3] * glm::normalize(right);
        lookAt(eye, center, up);
    }

    void reset()
    {
        eye = { 0.0f, 0.0f, 2.0f };
        center = { 0.0f, 0.0f, 0.0f };
        up = { 0.0f, -1.0f, 0.0f };
    }
};

}
