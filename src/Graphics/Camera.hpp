// src/Graphics/Camera.hpp
#pragma once

#include "../Math/Math.hpp"

namespace Engine::Graphics
{
    // �J�����̓��e�^�C�v
    enum class ProjectionType
    {
        Perspective,    // �������e�i3D�Q�[���p�j
        Orthographic   // ���s���e�i2D�Q�[����Z�p�}�ʗp�j
    };

    // �J�����N���X
    class Camera
    {
    public:
        Camera();
        ~Camera() = default;

        // �R�s�[�E���[�u����
        Camera(const Camera&) = default;
        Camera& operator=(const Camera&) = default;
        Camera(Camera&&) = default;
        Camera& operator=(Camera&&) = default;

        // �ʒu�E��]�̐ݒ�
        void setPosition(const Math::Vector3& position);
        void setRotation(const Math::Vector3& eulerAngles);
        void lookAt(const Math::Vector3& target, const Math::Vector3& up = Math::Vector3::up());

        // ���e�ݒ�
        void setPerspective(float fov, float aspect, float nearPlane, float farPlane);
        void setOrthographic(float left, float right, float bottom, float top, float nearPlane, float farPlane);

        // �Q�b�^�[
        const Math::Vector3& getPosition() const { return m_position; }
        const Math::Vector3& getRotation() const { return m_rotation; }
        Math::Vector3 getForward() const;
        Math::Vector3 getRight() const;
        Math::Vector3 getUp() const;

        float getFov() const { return m_fov; }
        float getAspect() const { return m_aspect; }
        float getNearPlane() const { return m_nearPlane; }
        float getFarPlane() const { return m_farPlane; }

        ProjectionType getProjectionType() const { return m_projectionType; }

        // �s��̎擾
        Math::Matrix4 getViewMatrix() const;
        Math::Matrix4 getProjectionMatrix() const;
        Math::Matrix4 getViewProjectionMatrix() const;

        // �J�����ړ��iFPS�X�^�C���j
        void moveForward(float distance);
        void moveRight(float distance);
        void moveUp(float distance);

        // �J������]
        void rotate(float pitch, float yaw, float roll = 0.0f);
        void rotatePitch(float pitch);
        void rotateYaw(float yaw);

        // �A�X�y�N�g����X�V�i�E�B���h�E���T�C�Y���Ɏg�p�j
        void updateAspect(float newAspect);

        // �}�E�X����iFPS�J�����j
        void processMouseMovement(float deltaX, float deltaY, float sensitivity = 0.1f);

        // �r���[�|�[�g���W����3D��Ԃւ̕ϊ�
        Math::Vector3 screenToWorldPoint(const Math::Vector3& screenPoint, float viewportWidth, float viewportHeight) const;

    private:
        // �J�����̈ʒu�E��]
        Math::Vector3 m_position;
        Math::Vector3 m_rotation;  // x=pitch, y=yaw, z=roll (�x)

        // ���e�ݒ�
        ProjectionType m_projectionType;
        float m_fov;           // ����p�i�x�j
        float m_aspect;        // �A�X�y�N�g��
        float m_nearPlane;     // �߃N���b�v��
        float m_farPlane;      // ���N���b�v��

        // ���s���e�p
        float m_left, m_right, m_bottom, m_top;

        // �����v�Z�p
        mutable bool m_viewMatrixDirty;
        mutable bool m_projectionMatrixDirty;
        mutable Math::Matrix4 m_viewMatrix;
        mutable Math::Matrix4 m_projectionMatrix;

        // �s��̍X�V
        void updateViewMatrix() const;
        void updateProjectionMatrix() const;

        // ��]�𐳋K���i-180~180�x�Ɏ��߂�j
        void normalizeRotation();
    };

    // �J�����R���g���[���[�i�ȒP��FPS�J��������j
    class FPSCameraController
    {
    public:
        explicit FPSCameraController(Camera* camera);

        // ���͏���
        void update(float deltaTime);
        void processKeyboard(bool forward, bool backward, bool left, bool right, bool up, bool down, float deltaTime);
        void processMouseMovement(float deltaX, float deltaY);

        // �ݒ�
        void setMovementSpeed(float speed) { m_movementSpeed = speed; }
        void setMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

        float getMovementSpeed() const { return m_movementSpeed; }
        float getMouseSensitivity() const { return m_mouseSensitivity; }

    private:
        Camera* m_camera;
        float m_movementSpeed;      // �ړ����x
        float m_mouseSensitivity;   // �}�E�X���x
    };
}