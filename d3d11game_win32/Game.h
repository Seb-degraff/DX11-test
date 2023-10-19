//
// Game.h
//

#pragma once

#include "StepTimer.h"
//#include <directxmath.h>


struct vec2_t vec2(float x, float y);

struct vec2_t
{
    float x;
    float y;

    inline vec2_t operator+(const vec2_t& other)
    {
        return vec2(x + other.x, y + other.y);
    }
};

inline vec2_t vec2(float x, float y)
{
    vec2_t vec;

    vec.x = x;
    vec.y = y;

    return vec;
}

struct vec3_t vec3(float x, float y, float z);

struct vec3_t
{
    float x;
    float y;
    float z;

    inline vec3_t operator+(const vec3_t& other)
    {
        return vec3(x + other.x, y + other.y, z + other.z);
    }
};

inline vec3_t vec3(float x, float y, float z)
{
    vec3_t vec;

    vec.x = x;
    vec.y = y;
    vec.z = z;

    return vec;
}

struct Transforms
{
    vec3_t pos;
    vec3_t rot;
};


struct MatrixBufferType
{
    DirectX::XMMATRIX world;
    DirectX::XMMATRIX view;
    DirectX::XMMATRIX projection;
};

struct VertexType
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT4 color;
};

extern HWND g_window;

// A basic game implementation that creates a D3D11 device and
// provides a game loop.
class Game
{
public:
    float m_cam_rot_y;
    vec3_t _player_pos;
    vec3_t _player_rot;

    Game() noexcept;
    ~Game() = default;

    Game(Game&&) = default;
    Game& operator= (Game&&) = default;

    Game(Game const&) = delete;
    Game& operator= (Game const&) = delete;

    // Initialization and management
    void Initialize(HWND window, int width, int height);

    // Basic game loop
    void Tick();

    // Messages
    void OnActivated();
    void OnDeactivated();
    void OnSuspending();
    void OnResuming();
    void OnWindowSizeChanged(int width, int height);

    // Properties
    void GetDefaultSize( int& width, int& height ) const noexcept;

private:

    void Update(DX::StepTimer const& timer);
    void shader_set_parameters();
    void shader_output_error(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename);
    bool shader_load(HWND hwnd, ID3D11Device* device, WCHAR* vsFilename, WCHAR* psFilename);
    void Render();

    void Clear();
    void Present();

    void CreateDevice();
    void CreateResources();

    void OnDeviceLost();

    // Device resources.
    int                                             m_outputWidth;
    int                                             m_outputHeight;

    D3D_FEATURE_LEVEL                               m_featureLevel;
    Microsoft::WRL::ComPtr<ID3D11Device1>           m_d3dDevice;
    Microsoft::WRL::ComPtr<ID3D11DeviceContext1>    m_d3dContext;

    Microsoft::WRL::ComPtr<IDXGISwapChain1>         m_swapChain;
    Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
    Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

    // Rendering loop timer.
    DX::StepTimer                                   m_timer;

    // Geometry
    ID3D11Buffer* m_vertexBuffer;
    ID3D11Buffer* m_indexBuffer;
    int m_indices_count;

    // Shader related, in a way, you know
    ID3D11VertexShader* m_vertexShader;
    ID3D11PixelShader* m_pixelShader;
    ID3D11InputLayout* m_layout;
    ID3D11Buffer* m_matrixBuffer;

    DirectX::XMMATRIX m_projectionMatrix;
    DirectX::XMMATRIX m_worldMatrix;
    DirectX::XMMATRIX m_viewMatrix;
    DirectX::XMMATRIX m_orthoMatrix;
    D3D11_VIEWPORT m_viewport;

    Transforms cam_transforms;
};

