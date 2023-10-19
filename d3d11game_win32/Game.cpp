//
// Game.cpp
//

#include "pch.h"
#include "Game.h"
#include <fstream>
#include <d3dcompiler.h>
#include "input.h"

extern void ExitGame() noexcept;

using namespace DirectX;

using Microsoft::WRL::ComPtr;

HWND g_window;

#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)

#define LOG(...) log_impl(WIDE1(__FILE__), __LINE__, __VA_ARGS__)
void log_impl(const wchar_t* file, int line, LPCWSTR format, ...);


#include <stdio.h>
#include <vadefs.h>

void log_impl(const wchar_t* file, int line, LPCWSTR format, ...)
{
    WCHAR buffer[2048];
    WCHAR buffer2[2048];
    va_list arg;
    va_start(arg, line);
    _vsnwprintf_s(buffer, sizeof(buffer), format, arg);
    _snwprintf_s(buffer2, sizeof(buffer2), L"%s(%i) %s\n", file, line, buffer);
    va_end(arg);

    OutputDebugString(buffer2);
}


Game::Game() noexcept :
    m_outputWidth(800),
    m_outputHeight(600),
    m_featureLevel(D3D_FEATURE_LEVEL_9_1)
{
}

// Initialize the Direct3D resources required to run.
void Game::Initialize(HWND window, int width, int height)
{
    g_window = window;
    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateDevice();

    CreateResources();

    // TODO: Change the timer settings if you want something other than the default variable timestep mode.
    // e.g. for 60 FPS fixed timestep update logic, call:
    /*
    m_timer.SetFixedTimeStep(true);
    m_timer.SetTargetElapsedSeconds(1.0 / 60);
    */
}

// Executes the basic game loop.
void Game::Tick()
{
    m_timer.Tick([&]()
    {
        Update(m_timer);
    });

    Render();
}


#define __M_PI   3.14159265358979323846264338327950288
#define degToRad(angleInDegrees) ((angleInDegrees) * __M_PI / 180.0)
#define radToDeg(angleInRadians) ((angleInRadians) * 180.0 / __M_PI)

// Updates the world.
void Game::Update(DX::StepTimer const& timer)
{
    float elapsedTime = float(timer.GetElapsedSeconds());

    input_tick();

    _player_rot.y += get_mouse_delta().x * 0.1f;
    _player_rot.x += get_mouse_delta().y * 0.1f;
    if (_player_rot.x > 45.f) _player_rot.x = 45.f;
    if (_player_rot.x < -45.f) _player_rot.x = -45.f;

    Vec2i wasd = get_wasd();
    vec2_t movement = vec2(0.03f * wasd.x, 0.03f * wasd.y);

    _player_pos.x += movement.x * cosf((float)degToRad(_player_rot.y)) + movement.y * sinf((float)degToRad(_player_rot.y));
    _player_pos.z += movement.y * cosf((float)degToRad(_player_rot.y)) + movement.x * -sinf((float)degToRad(_player_rot.y));

    LOG(L"PLAYER POS %f, %f", _player_pos.x, _player_pos.y);

    Transforms trans;
    trans.pos = _player_pos + vec3(0, 0.7f, 0);
    trans.rot = _player_rot;

    trans.pos = _player_pos + vec3(0, 0.7f, 0);
    trans.rot = _player_rot;

    cam_transforms = trans;

    // TODO: Add your game logic here.
    
}


void Game::shader_set_parameters()
{
    HRESULT result;
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    MatrixBufferType* dataPtr;
    unsigned int bufferNumber;

    // Transpose the matrices to prepare them for the shader.
    XMMATRIX worldMatrix = XMMatrixTranspose(m_worldMatrix);
    XMMATRIX viewMatrix = XMMatrixTranspose(m_viewMatrix);
    XMMATRIX projectionMatrix = XMMatrixTranspose(m_projectionMatrix);


    // Lock the constant buffer so it can be written to.
    DX::ThrowIfFailed(m_d3dContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

    // Get a pointer to the data in the constant buffer.
    dataPtr = (MatrixBufferType*)mappedResource.pData;

    // Copy the matrices into the constant buffer.
    dataPtr->world = worldMatrix;
    dataPtr->view = viewMatrix;
    dataPtr->projection = projectionMatrix;

    // Unlock the constant buffer.
    m_d3dContext->Unmap(m_matrixBuffer, 0);

    // Set the position of the constant buffer in the vertex shader.
    bufferNumber = 0;

    // Finaly set the constant buffer in the vertex shader with the updated values.
    m_d3dContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
}

void Game::shader_output_error(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
{
    char* compileErrors;
    unsigned long long bufferSize, i;
    std::ofstream fout;


    // Get a pointer to the error message text buffer.
    compileErrors = (char*)(errorMessage->GetBufferPointer());

    // Get the length of the message.
    bufferSize = errorMessage->GetBufferSize();

    // Open a file to write the error message to.
    fout.open("shader-error.txt");

    // Write out the error message.
    for (i = 0; i < bufferSize; i++)
    {
        fout << compileErrors[i];
    }

    // Close the file.
    fout.close();

    // Release the error message.
    errorMessage->Release();
    errorMessage = 0;

    // Pop a message up on the screen to notify the user to check the text file for compile errors.
    MessageBox(hwnd, L"Error compiling shader.  Check shader-error.txt for message.", shaderFilename, MB_OK);

    exit(1);
}

bool Game::shader_load(HWND hwnd, ID3D11Device* device, WCHAR* vsFilename, WCHAR* psFilename)
{
    HRESULT result;
    ID3D10Blob* errorMessage = 0;
    ID3D10Blob* vertexShaderBuffer = 0;
    ID3D10Blob* pixelShaderBuffer = 0;
    D3D11_INPUT_ELEMENT_DESC polygonLayout[2];
    unsigned int numElements;
    D3D11_BUFFER_DESC matrixBufferDesc;

    result = D3DCompileFromFile(vsFilename, NULL, NULL, "ColorVertexShader", "vs_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &vertexShaderBuffer, &errorMessage);

    if (FAILED(result)) {
        // If the shader failed to compile it should have writen something to the error message.
        if (errorMessage) {
            shader_output_error(errorMessage, hwnd, vsFilename);
        }
        // If there was  nothing in the error message then it simply could not find the shader file itself.
        else {
            MessageBox(hwnd, vsFilename, L"Missing vertex shader file", MB_OK);
        }

        return false;
    }

    // Compile the pixel shader code.
    result = D3DCompileFromFile(psFilename, NULL, NULL, "ColorPixelShader", "ps_5_0", D3D10_SHADER_ENABLE_STRICTNESS, 0,
        &pixelShaderBuffer, &errorMessage);
    if (FAILED(result)) {
        // If the shader failed to compile it should have writen something to the error message.
        if (errorMessage) {
            shader_output_error(errorMessage, hwnd, psFilename);
        }
        // If there was nothing in the error message then it simply could not find the file itself.
        else {
            MessageBox(hwnd, psFilename, L"Missing pixel shader file", MB_OK);
        }

        exit(1);
    }

    // Create the vertex shader from the buffer.
    DX::ThrowIfFailed(device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader));

    // Create the pixel shader from the buffer.
    DX::ThrowIfFailed(device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader));   

    // Create the vertex input layout description.
    // This setup needs to match the VertexType stucture in the ModelClass and in the shader.
    polygonLayout[0].SemanticName = "POSITION";
    polygonLayout[0].SemanticIndex = 0;
    polygonLayout[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
    polygonLayout[0].InputSlot = 0;
    polygonLayout[0].AlignedByteOffset = 0;
    polygonLayout[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[0].InstanceDataStepRate = 0;

    polygonLayout[1].SemanticName = "COLOR";
    polygonLayout[1].SemanticIndex = 0;
    polygonLayout[1].Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    polygonLayout[1].InputSlot = 0;
    polygonLayout[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
    polygonLayout[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
    polygonLayout[1].InstanceDataStepRate = 0;

    // Get a count of the elements in the layout.
    numElements = sizeof(polygonLayout) / sizeof(polygonLayout[0]);

    // Create the vertex input layout.
    result = device->CreateInputLayout(polygonLayout, numElements, vertexShaderBuffer->GetBufferPointer(),
        vertexShaderBuffer->GetBufferSize(), &m_layout);
    DX::ThrowIfFailed(result);

    // Release the vertex shader buffer and pixel shader buffer since they are no longer needed.
    vertexShaderBuffer->Release();
    vertexShaderBuffer = 0;

    pixelShaderBuffer->Release();
    pixelShaderBuffer = 0;

    // Setup the description of the dynamic matrix constant buffer that is in the vertex shader.
    matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
    matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    matrixBufferDesc.MiscFlags = 0;
    matrixBufferDesc.StructureByteStride = 0;

    // Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
    DX::ThrowIfFailed(device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer));
}


// Draws the scene.
void Game::Render()
{
    // Don't try to render anything before the first Update.
    if (m_timer.GetFrameCount() == 0)
    {
        return;
    }

    Clear();

    // TODO: Add your rendering code here.
    

    // Bind index and vertex buffers
    {
        unsigned int stride;
        unsigned int offset;


        // Set vertex buffer stride and offset.
        stride = sizeof(VertexType);
        offset = 0;

        // Set the vertex buffer to active in the input assembler so it can be rendered.
        m_d3dContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

        // Set the index buffer to active in the input assembler so it can be rendered.
        m_d3dContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

        // Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
        m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    }

    // set view matrix
    //XMVECTOR pos = XMVectorSet(0.f, 0.f, 0.f, 0.f);
    //XMVECTOR lookAt = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    //XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    //m_viewMatrix = XMMatrixLookAtLH(pos, lookAt, up);
    {
        XMFLOAT3 up, position, lookAt;
        XMVECTOR upVector, positionVector, lookAtVector;
        float yaw, pitch, roll;
        XMMATRIX rotationMatrix;


        // Setup the vector that points upwards.
        up.x = 0.0f;
        up.y = 1.0f;
        up.z = 0.0f;

        // Load it into a XMVECTOR structure.
        upVector = XMLoadFloat3(&up);

        // Setup the position of the camera in the world.
        /*position.x = sinf(frame_num / 60.f) * 5.f;
        position.y = 0.f;
        position.z = cosf(frame_num / 60.f) * -5.f;*/
;
        position.x = cam_transforms.pos.x;
        position.y = cam_transforms.pos.y;
        position.z = cam_transforms.pos.z;

        // Load it into a XMVECTOR structure.
        positionVector = XMLoadFloat3(&position);

        // Setup where the camera is looking by default.
        lookAt.x = 0;
        lookAt.y = 0;
        lookAt.z = 5.f;

        // Load it into a XMVECTOR structure.
        lookAtVector = XMLoadFloat3(&lookAt);

        // Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
        pitch = cam_transforms.rot.x * 0.0174532925f;
        yaw = cam_transforms.rot.y * 0.0174532925f;
        roll = cam_transforms.rot.z * 0.0174532925f;

        // Create the rotation matrix from the yaw, pitch, and roll values.
        rotationMatrix = XMMatrixRotationRollPitchYaw(pitch, yaw, roll);

        // Transform the lookAt and up vector by the rotation matrix so the view is correctly rotated at the origin.
        lookAtVector = XMVector3TransformCoord(lookAtVector, rotationMatrix);
        upVector = XMVector3TransformCoord(upVector, rotationMatrix);

        // Translate the rotated camera position to the location of the viewer.
        lookAtVector = XMVectorAdd(positionVector, lookAtVector);

        // Finally create the view matrix from the three updated vectors.
        m_viewMatrix = XMMatrixLookAtLH(positionVector, lookAtVector, upVector);
    }

    Game::shader_set_parameters();

    // shader draw
    {
        // Set the vertex input layout.
        m_d3dContext->IASetInputLayout(m_layout);

        // Set the vertex and pixel shaders that will be used to render this triangle.
        m_d3dContext->VSSetShader(m_vertexShader, NULL, 0);
        m_d3dContext->PSSetShader(m_pixelShader, NULL, 0);

        // Render the triangle.
        m_d3dContext->DrawIndexed(12, 0, 0);
    }

    Present();
}

// Helper method to clear the back buffers.
void Game::Clear()
{
    // Clear the views.
    m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
    m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

    m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

    // Set the viewport.
    D3D11_VIEWPORT viewport = { 0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight), 0.f, 1.f };
    m_d3dContext->RSSetViewports(1, &viewport);
}

// Presents the back buffer contents to the screen.
void Game::Present()
{
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = m_swapChain->Present(1, 0);

    // If the device was reset we must completely reinitialize the renderer.
    if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
    {
        OnDeviceLost();
    }
    else
    {
        DX::ThrowIfFailed(hr);
    }
}

// Message handlers
void Game::OnActivated()
{
    // TODO: Game is becoming active window.
}

void Game::OnDeactivated()
{
    // TODO: Game is becoming background window.
}

void Game::OnSuspending()
{
    // TODO: Game is being power-suspended (or minimized).
}

void Game::OnResuming()
{
    m_timer.ResetElapsedTime();

    // TODO: Game is being power-resumed (or returning from minimize).
}

void Game::OnWindowSizeChanged(int width, int height)
{
    if (!g_window)
        return;

    m_outputWidth = std::max(width, 1);
    m_outputHeight = std::max(height, 1);

    CreateResources();

    // TODO: Game window is being resized.
}

// Properties
void Game::GetDefaultSize(int& width, int& height) const noexcept
{
    // TODO: Change to desired default window size (note minimum size is 320x200).
    width = 800;
    height = 600;
}

struct Geometry
{
    VertexType* vertices;
    unsigned long* indices;
    int vertices_cap;
    int indices_cap;
    int vertices_current;
    int indices_current;
};

VertexType vertex(vec3_t pos, vec3_t col)
{
    VertexType ret;
    ret.position = XMFLOAT3(pos.x, pos.y, pos.z);
    ret.color = XMFLOAT4(col.x, col.y, col.z, 1);
    return ret;
}

void generate_quad(Geometry* g, vec3_t pos_a, vec3_t pos_b, vec3_t pos_c, vec3_t pos_d, vec3_t color)
{
    // TODO: optimize
    g->vertices[g->vertices_current + 0] = vertex(pos_a, color);
    g->vertices[g->vertices_current + 1] = vertex(pos_b, color);
    g->vertices[g->vertices_current + 2] = vertex(pos_c, color);
    g->vertices[g->vertices_current + 3] = vertex(pos_d, color);

    g->indices[g->indices_current + 0] = g->vertices_current + 0;
    g->indices[g->indices_current + 1] = g->vertices_current + 1;
    g->indices[g->indices_current + 2] = g->vertices_current + 2;
    g->indices[g->indices_current + 3] = g->vertices_current + 0;
    g->indices[g->indices_current + 4] = g->vertices_current + 2;
    g->indices[g->indices_current + 5] = g->vertices_current + 3;

    g->vertices_current += 4;
    g->indices_current += 6;
}

const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.02f;

// These are the resources that depend on the device.
void Game::CreateDevice()
{
    UINT creationFlags = 0;

#ifdef _DEBUG
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    static const D3D_FEATURE_LEVEL featureLevels [] =
    {
        // TODO: Modify for supported Direct3D feature levels
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1,
    };

    // Create the DX11 API device object, and get a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    DX::ThrowIfFailed(D3D11CreateDevice(
        nullptr,                            // specify nullptr to use the default adapter
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags,
        featureLevels,
        static_cast<UINT>(std::size(featureLevels)),
        D3D11_SDK_VERSION,
        device.ReleaseAndGetAddressOf(),    // returns the Direct3D device created
        &m_featureLevel,                    // returns feature level of device created
        context.ReleaseAndGetAddressOf()    // returns the device immediate context
        ));

#ifndef NDEBUG
    ComPtr<ID3D11Debug> d3dDebug;
    if (SUCCEEDED(device.As(&d3dDebug)))
    {
        ComPtr<ID3D11InfoQueue> d3dInfoQueue;
        if (SUCCEEDED(d3dDebug.As(&d3dInfoQueue)))
        {
#ifdef _DEBUG
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, true);
            d3dInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, true);
#endif
            D3D11_MESSAGE_ID hide [] =
            {
                D3D11_MESSAGE_ID_SETPRIVATEDATA_CHANGINGPARAMS,
                // TODO: Add more message IDs here as needed.
            };
            D3D11_INFO_QUEUE_FILTER filter = {};
            filter.DenyList.NumIDs = static_cast<UINT>(std::size(hide));
            filter.DenyList.pIDList = hide;
            d3dInfoQueue->AddStorageFilterEntries(&filter);
        }
    }
#endif

    DX::ThrowIfFailed(device.As(&m_d3dDevice));
    DX::ThrowIfFailed(context.As(&m_d3dContext));

    // TODO: Initialize device dependent objects here (independent of window size).
       
    shader_load(g_window, m_d3dDevice.Get(), (WCHAR*)L"../../../../../color.vs", (WCHAR*)L"../../../../../color.ps");

    {
        VertexType* vertices;
        unsigned long* indices;
        D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
        D3D11_SUBRESOURCE_DATA vertexData, indexData;
        HRESULT result;

        // Set the number of vertices in the vertex array.
        int vertexCap = 10000;

        // Set the number of indices in the index array.
        int indexCap = 30000;

        // Create the vertex array.
        vertices = new VertexType[vertexCap];

        // Create the index array.
        indices = new unsigned long[indexCap];

        Geometry geom = { 0 };
        geom.indices = indices;
        geom.vertices = vertices;
        geom.indices_cap = indexCap;
        geom.vertices_cap = vertexCap;
        geom.indices_current = 0;
        geom.vertices_current = 0;

        generate_quad(&geom, vec3(0.6f, 0, 0), vec3(0, 1, 0), vec3(-0.6f, 0, 0), vec3(0, -1, 0), vec3(1, 1, 1));
        generate_quad(&geom, vec3(0.6f, 0, 0), vec3(0, -1, 0), vec3(-0.6f, 0, 0), vec3(0, 1, 0), vec3(1, 1, 1));
        //m_vertexCount = geom.vertices_current;
        //m_indexCount = geom.indices_current;

        //vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // Bottom left.
        //vertices[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

        //vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // Top middle.
        //vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

        //vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // Bottom right.
        //vertices[2].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

        // Load the index array with data.
        //indices[0] = 0;  // Bottom left.
        //indices[1] = 1;  // Top middle.
        //indices[2] = 2;  // Bottom right.

        // Set up the description of the static vertex buffer.
        vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        vertexBufferDesc.ByteWidth = sizeof(VertexType) * geom.vertices_current;
        vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        vertexBufferDesc.CPUAccessFlags = 0;
        vertexBufferDesc.MiscFlags = 0;
        vertexBufferDesc.StructureByteStride = 0;

        // Give the subresource structure a pointer to the vertex data.
        vertexData.pSysMem = vertices;
        vertexData.SysMemPitch = 0;
        vertexData.SysMemSlicePitch = 0;

        // Now create the vertex buffer.
        DX::ThrowIfFailed(device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer));

        // Set up the description of the static index buffer.
        indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
        indexBufferDesc.ByteWidth = sizeof(unsigned long) * geom.indices_current;
        indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        indexBufferDesc.CPUAccessFlags = 0;
        indexBufferDesc.MiscFlags = 0;
        indexBufferDesc.StructureByteStride = 0;

        // Give the subresource structure a pointer to the index data.
        indexData.pSysMem = indices;
        indexData.SysMemPitch = 0;
        indexData.SysMemSlicePitch = 0;

        // Create the index buffer.
        DX::ThrowIfFailed(device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer));

        // Release the arrays now that the vertex and index buffers have been created and loaded.
        delete[] vertices;
        vertices = 0;

        delete[] indices;
        indices = 0;
    }
}

// Allocate all memory resources that change on a window SizeChanged event.
void Game::CreateResources()
{
    // Clear the previous window size specific context.
    m_d3dContext->OMSetRenderTargets(0, nullptr, nullptr);
    m_renderTargetView.Reset();
    m_depthStencilView.Reset();
    m_d3dContext->Flush();

    const UINT backBufferWidth = static_cast<UINT>(m_outputWidth);
    const UINT backBufferHeight = static_cast<UINT>(m_outputHeight);
    const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_B8G8R8A8_UNORM;
    const DXGI_FORMAT depthBufferFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    constexpr UINT backBufferCount = 2;

    // If the swap chain already exists, resize it, otherwise create one.
    if (m_swapChain)
    {
        HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, backBufferWidth, backBufferHeight, backBufferFormat, 0);

        if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
        {
            // If the device was removed for any reason, a new device and swap chain will need to be created.
            OnDeviceLost();

            // Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
            // and correctly set up the new device.
            return;
        }
        else
        {
            DX::ThrowIfFailed(hr);
        }
    }
    else
    {
        // First, retrieve the underlying DXGI Device from the D3D Device.
        ComPtr<IDXGIDevice1> dxgiDevice;
        DX::ThrowIfFailed(m_d3dDevice.As(&dxgiDevice));

        // Identify the physical adapter (GPU or card) this device is running on.
        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(dxgiDevice->GetAdapter(dxgiAdapter.GetAddressOf()));

        // And obtain the factory object that created it.
        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(dxgiFactory.GetAddressOf())));

        // Create a descriptor for the swap chain.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = backBufferWidth;
        swapChainDesc.Height = backBufferHeight;
        swapChainDesc.Format = backBufferFormat;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = backBufferCount;

        DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
        fsSwapChainDesc.Windowed = TRUE;

        // Create a SwapChain from a Win32 window.
        DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForHwnd(
            m_d3dDevice.Get(),
            g_window,
            &swapChainDesc,
            &fsSwapChainDesc,
            nullptr,
            m_swapChain.ReleaseAndGetAddressOf()
            ));

        // This template does not support exclusive fullscreen mode and prevents DXGI from responding to the ALT+ENTER shortcut.
        DX::ThrowIfFailed(dxgiFactory->MakeWindowAssociation(g_window, DXGI_MWA_NO_ALT_ENTER));
    }

    // Obtain the backbuffer for this window which will be the final 3D rendertarget.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(backBuffer.GetAddressOf())));

    // Create a view interface on the rendertarget to use on bind.
    DX::ThrowIfFailed(m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_renderTargetView.ReleaseAndGetAddressOf()));

    // Allocate a 2-D surface as the depth/stencil buffer and
    // create a DepthStencil view on this surface to use on bind.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(depthBufferFormat, backBufferWidth, backBufferHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, depthStencil.GetAddressOf()));

    DX::ThrowIfFailed(m_d3dDevice->CreateDepthStencilView(depthStencil.Get(), nullptr, m_depthStencilView.ReleaseAndGetAddressOf()));

    // TODO: Initialize windows-size dependent objects here.

    // Setup the projection matrix.
    float fieldOfView = 3.141592654f / 4.0f;
    float screenAspect = (float)backBufferWidth / (float)backBufferHeight;

    // Create the projection matrix for 3D rendering.
    m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, SCREEN_NEAR, SCREEN_DEPTH);

    // Initialize the world matrix to the identity matrix.
    m_worldMatrix = XMMatrixIdentity();

    // Create an orthographic projection matrix for 2D rendering.
    m_orthoMatrix = XMMatrixOrthographicLH((float)backBufferWidth, (float)backBufferHeight, SCREEN_NEAR, SCREEN_DEPTH);
}

void Game::OnDeviceLost()
{
    // TODO: Add Direct3D resource cleanup here.

    m_depthStencilView.Reset();
    m_renderTargetView.Reset();
    m_swapChain.Reset();
    m_d3dContext.Reset();
    m_d3dDevice.Reset();

    CreateDevice();

    CreateResources();
}

