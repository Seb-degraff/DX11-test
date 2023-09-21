//
// Most code from this post: https://rastertek.com/dx11win10tut03.html
// De-OOPified and simplified a bit.
//
// NOTE(seb): here I include libraries for linking via pragma comment, but another way is
//            to add them to Project -> Properties -> Linker -> Input -> 'Additional Dependencies'.
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

#include "framework.h"
#include <minwindef.h>
#include "utils.h"
#include <fstream>

#include <d3d11.h>
#include <directxmath.h>
#include <D3dcompiler.h>

using namespace DirectX;

struct MatrixBufferType
{
	XMMATRIX world;
	XMMATRIX view;
	XMMATRIX projection;
};

struct VertexType
{
	XMFLOAT3 position;
	XMFLOAT4 color;
};

#define CHECK_RESULT(check_fail_message) \
if (FAILED(result)) { \
	LOG(L"[ERROR] %s failed!", check_fail_message); \
	exit(1); \
}

#define CHECK(function_call) \
if (FAILED(function_call)) { \
	LOG(L"[ERROR] %s failed!", WIDE1(#function_call)); \
	DebugBreak(); \
	exit(1); \
}

const bool FULL_SCREEN = false;
const bool VSYNC_ENABLED = true;
const float SCREEN_DEPTH = 1000.0f;
const float SCREEN_NEAR = 0.3f;

bool m_vsync_enabled = true;
int m_videoCardMemory;
char m_videoCardDescription[128];
IDXGISwapChain* m_swapChain;
ID3D11Device* m_device;
ID3D11DeviceContext* m_deviceContext;
ID3D11RenderTargetView* m_renderTargetView;
ID3D11Texture2D* m_depthStencilBuffer;
ID3D11DepthStencilState* m_depthStencilState;
ID3D11DepthStencilView* m_depthStencilView;
ID3D11RasterizerState* m_rasterState;
XMMATRIX m_projectionMatrix;
XMMATRIX m_worldMatrix;
XMMATRIX m_viewMatrix;
XMMATRIX m_orthoMatrix;
D3D11_VIEWPORT m_viewport;

// shader-related stuff
ID3D11VertexShader* m_vertexShader;
ID3D11PixelShader* m_pixelShader;
ID3D11InputLayout* m_layout;
ID3D11Buffer* m_matrixBuffer;

// mesh related stuff
ID3D11Buffer* m_vertexBuffer, * m_indexBuffer;
int m_vertexCount, m_indexCount;


bool shader_load(HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename);
void shader_unload();
void shader_set_parameters();
void initialize_mesh_buffers();

bool renderer_init(int screen_width, int screen_height, HWND hwnd)
{
	LOG(L"initializing D3D11...");

	HRESULT result;
	IDXGIFactory* factory;
	IDXGIAdapter* adapter;
	IDXGIOutput* adapterOutput;
	unsigned int numModes, i;
	unsigned long long stringLength;
	DXGI_MODE_DESC* displayModeList;
	DXGI_ADAPTER_DESC adapterDesc;
	DXGI_SWAP_CHAIN_DESC swapChainDesc;
	D3D_FEATURE_LEVEL featureLevel;
	ID3D11Texture2D* backBufferPtr;
	D3D11_TEXTURE2D_DESC depthBufferDesc;
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	D3D11_RASTERIZER_DESC rasterDesc;
	float fieldOfView, screenAspect;

	// Create a DirectX graphics interface factory.
	result = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
	CHECK_RESULT(L"CreateDXGIFactory");

	// Use the factory to create an adapter for the primary graphics interface (video card).
	result = factory->EnumAdapters(0, &adapter);
	CHECK_RESULT(L"factory->EnumAdapters");

	// Enumerate the primary adapter output (monitor).
	result = adapter->EnumOutputs(0, &adapterOutput);
	CHECK_RESULT(L"adapter->EnumOutputs");

	// Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	CHECK_RESULT(L"adapterOutput->GetDisplayModeList");

	// Create a list to hold all the possible display modes for this monitor/video card combination.
	displayModeList = new DXGI_MODE_DESC[numModes]; // NOTE(seb): any good reason to allocate on the heap?

	// Now fill the display mode list structures.
	result = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList);
	CHECK_RESULT(L"adapterOutput->GetDisplayModeList");

	// Now go through all the display modes and find the one that matches the screen width and height.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
	int numerator = 0;
	int denominator = 0;
	bool found = false;
	for (i = 0; i < numModes; i++) {
		if (displayModeList[i].Width == (unsigned int)screen_width &&
		    displayModeList[i].Height == (unsigned int)screen_height
		) {
			found = true;
			numerator = displayModeList[i].RefreshRate.Numerator;
			denominator = displayModeList[i].RefreshRate.Denominator;
		}
	}
	
	if (!found) {
		LOG(L"Couldn't find display mode matching our screen dimentions (%i by %i)", screen_width, screen_height);
		exit(1);
	}

	// Get the adapter (video card) description.
	result = adapter->GetDesc(&adapterDesc);
	CHECK_RESULT(L"adapter->GetDesc");

	// Store the dedicated video card memory in megabytes.
	int dedicated_memory_mb = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);
	int shared_memory_mb = (int)(adapterDesc.SharedSystemMemory / 1024 / 1024);
	LOG(L"dedicated_memory_mb: %i", dedicated_memory_mb);
	LOG(L"shared_memory_mb: %i", shared_memory_mb);

	// Convert the name of the video card to a character array and store it.
	/*char adapter_desc[128];
	int error = wcstombs_s(&stringLength, adapter_desc, sizeof(adapter_desc), adapterDesc.Description, sizeof(adapter_desc));
	if (error != 0) {
		LOG(L"fucking hell");
		exit(1);
	}*/
	LOG(L"Adapter desc: %s", adapterDesc.Description);

	// Release the display mode list.
	delete[] displayModeList;
	displayModeList = 0;

	// Release the adapter output.
	adapterOutput->Release();
	adapterOutput = 0;

	// Release the adapter.
	adapter->Release();
	adapter = 0;

	// Release the factory.
	factory->Release();
	factory = 0;

	// Initialize the swap chain description.
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

	// Set to a single back buffer.
	swapChainDesc.BufferCount = 1; // NOTE(seb): sounds strange, shouldn't we have two to swap between them? Or is
	                               // it something else like the number of target buffers to draw to, where you could want multiple attached buffers for e.g. deffered rendering?

	// Set the width and height of the back buffer.
	swapChainDesc.BufferDesc.Width = screen_width;
	swapChainDesc.BufferDesc.Height = screen_height;

	// Set regular 32-bit surface for the back buffer.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the refresh rate of the back buffer.
	if (m_vsync_enabled) {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;
	}
	else {
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 0;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	}

	// Set the usage of the back buffer.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = hwnd;

	// Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;

	// Set to full screen or windowed mode.
	bool fullscreen = false;
	if (fullscreen) {
		swapChainDesc.Windowed = false;
	}
	else {
		swapChainDesc.Windowed = true;
	}

	// Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Discard the back buffer contents after presenting.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	// Don't set the advanced flags. 
	swapChainDesc.Flags = 0;

	// Set the feature level to DirectX 11.
	featureLevel = D3D_FEATURE_LEVEL_11_0;

	// Create the swap chain, Direct3D device, and Direct3D device context.
	UINT creationFlags = 0;
	#if defined(_DEBUG)
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	#endif
	result = D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, creationFlags, &featureLevel, 1,
		D3D11_SDK_VERSION, &swapChainDesc, &m_swapChain, &m_device, NULL, &m_deviceContext);

	CHECK_RESULT(L"D3D11CreateDeviceAndSwapChain");

	// Get the pointer to the back buffer.
	result = m_swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&backBufferPtr);
	CHECK_RESULT(L"m_swapChain->GetBuffer");

	// Create the render target view with the back buffer pointer.
	result = m_device->CreateRenderTargetView(backBufferPtr, NULL, &m_renderTargetView);
	CHECK_RESULT(L"m_device->CreateRenderTargetView");

	// Release pointer to the back buffer as we no longer need it.
	backBufferPtr->Release();
	backBufferPtr = 0;

	// Initialize the description of the depth buffer.
	ZeroMemory(&depthBufferDesc, sizeof(depthBufferDesc));

	// Set up the description of the depth buffer.
	depthBufferDesc.Width = screen_width;
	depthBufferDesc.Height = screen_height;
	depthBufferDesc.MipLevels = 1;
	depthBufferDesc.ArraySize = 1;
	depthBufferDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthBufferDesc.SampleDesc.Count = 1;
	depthBufferDesc.SampleDesc.Quality = 0;
	depthBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	depthBufferDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthBufferDesc.CPUAccessFlags = 0;
	depthBufferDesc.MiscFlags = 0;

	// Create the texture for the depth buffer using the filled out description.
	result = m_device->CreateTexture2D(&depthBufferDesc, NULL, &m_depthStencilBuffer);
	if (FAILED(result))
	{
		return false;
	}

	// Initialize the description of the stencil state.
	ZeroMemory(&depthStencilDesc, sizeof(depthStencilDesc));

	// Set up the description of the stencil state.
	depthStencilDesc.DepthEnable = true;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_LESS;

	depthStencilDesc.StencilEnable = true;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing.
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing.
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Create the depth stencil state.
	result = m_device->CreateDepthStencilState(&depthStencilDesc, &m_depthStencilState);
	if (FAILED(result))
	{
		return false;
	}

	// Set the depth stencil state.
	m_deviceContext->OMSetDepthStencilState(m_depthStencilState, 1);

	// Initialize the depth stencil view.
	ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));

	// Set up the depth stencil view description.
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Texture2D.MipSlice = 0;

	// Create the depth stencil view.
	result = m_device->CreateDepthStencilView(m_depthStencilBuffer, &depthStencilViewDesc, &m_depthStencilView);
	if (FAILED(result))
	{
		return false;
	}

	// Bind the render target view and depth stencil buffer to the output render pipeline.
	m_deviceContext->OMSetRenderTargets(1, &m_renderTargetView, m_depthStencilView);

	{
		// NOTE(seb): according to the turoria this step is completely optionnal:
		// "[..] we'll create a rasterizer state. This will give us control over how polygons
		// are rendered. We can do things like make our scenes render in wireframe mode or have
		// DirectX draw both the front and back faces of polygons. By default, DirectX already
		// has a rasterizer state set up and working the exact same as the one below but you
		// have no control to change it unless you set up one yourself."
		
		// Setup the raster description which will determine how and what polygons will be drawn.
		rasterDesc.AntialiasedLineEnable = false;
		//rasterDesc.CullMode = D3D11_CULL_BACK;
		rasterDesc.CullMode = D3D11_CULL_NONE;
		rasterDesc.DepthBias = 0;
		rasterDesc.DepthBiasClamp = 0.0f;
		rasterDesc.DepthClipEnable = true;
		rasterDesc.FillMode = D3D11_FILL_SOLID;
		//rasterDesc.FillMode = D3D11_FILL_WIREFRAME;
		rasterDesc.FrontCounterClockwise = false;
		rasterDesc.MultisampleEnable = false;
		rasterDesc.ScissorEnable = false;
		rasterDesc.SlopeScaledDepthBias = 0.0f;

		// Create the rasterizer state from the description we just filled out.
		result = m_device->CreateRasterizerState(&rasterDesc, &m_rasterState);
		if (FAILED(result))
		{
			return false;
		}

		// Now set the rasterizer state.
		m_deviceContext->RSSetState(m_rasterState);
	}

	// Setup the viewport for rendering.
	m_viewport.Width = (float) screen_width;
	m_viewport.Height = (float) screen_height;
	m_viewport.MinDepth = 0.0f;
	m_viewport.MaxDepth = 1.0f;
	m_viewport.TopLeftX = 0.0f;
	m_viewport.TopLeftY = 0.0f;

	// Create the viewport.
	m_deviceContext->RSSetViewports(1, &m_viewport);

	// Setup the projection matrix.
	fieldOfView = 3.141592654f / 4.0f;
	screenAspect = (float) screen_width / (float) screen_height;

	// Create the projection matrix for 3D rendering.
	m_projectionMatrix = XMMatrixPerspectiveFovLH(fieldOfView, screenAspect, SCREEN_NEAR, SCREEN_DEPTH);

	// Initialize the world matrix to the identity matrix.
	m_worldMatrix = XMMatrixIdentity();

	// Create an orthographic projection matrix for 2D rendering.
	m_orthoMatrix = XMMatrixOrthographicLH((float)screen_width, (float)screen_height, SCREEN_NEAR, SCREEN_DEPTH);

	shader_load(hwnd, (WCHAR*) L"color.vs", (WCHAR*) L"color.ps");

	initialize_mesh_buffers();

	return true;
}

void renderer_shutdown()
{
	shader_unload();

	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if (m_swapChain) {
		m_swapChain->SetFullscreenState(false, NULL);
	}

	if (m_rasterState) {
		m_rasterState->Release();
		m_rasterState = 0;
	}

	if (m_depthStencilView) {
		m_depthStencilView->Release();
		m_depthStencilView = 0;
	}

	if (m_depthStencilState) {
		m_depthStencilState->Release();
		m_depthStencilState = 0;
	}

	if (m_depthStencilBuffer) {
		m_depthStencilBuffer->Release();
		m_depthStencilBuffer = 0;
	}

	if (m_renderTargetView) {
		m_renderTargetView->Release();
		m_renderTargetView = 0;
	}

	if (m_deviceContext) {
		m_deviceContext->Release();
		m_deviceContext = 0;
	}

	if (m_device) {
		m_device->Release();
		m_device = 0;
	}

	if (m_swapChain) {
		m_swapChain->Release();
		m_swapChain = 0;
	}

	return;
}


void renderer_frame_begin(float cam_rot_y)
{
	static int frame_num = 0;
	frame_num++;
	float color[4];

	// Setup the color to clear the buffer to.
	if (frame_num & 0x0100) {
		color[0] = 0.8f;
		color[1] = 0.2f;
		color[2] = 0.5f;
		color[3] = 1.0f;
	}
	else {
		color[0] = 0.1f;
		color[1] = 0.4f;
		color[2] = 0.8f;
		color[3] = 1.0f;
	}

	// Clear the back buffer.
	m_deviceContext->ClearRenderTargetView(m_renderTargetView, color);

	// Clear the depth buffer.
	m_deviceContext->ClearDepthStencilView(m_depthStencilView, D3D11_CLEAR_DEPTH, 1.0f, 0);

	// code called "RenderBuffers" in the tutorial, which actually binds the vertex/index buffers
	{
		unsigned int stride;
		unsigned int offset;


		// Set vertex buffer stride and offset.
		stride = sizeof(VertexType);
		offset = 0;

		// Set the vertex buffer to active in the input assembler so it can be rendered.
		m_deviceContext->IASetVertexBuffers(0, 1, &m_vertexBuffer, &stride, &offset);

		// Set the index buffer to active in the input assembler so it can be rendered.
		m_deviceContext->IASetIndexBuffer(m_indexBuffer, DXGI_FORMAT_R32_UINT, 0);

		// Set the type of primitive that should be rendered from this vertex buffer, in this case triangles.
		m_deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
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

		position.x = 0.f;
		position.y = 0.f;
		position.z = -5.f;

		// Load it into a XMVECTOR structure.
		positionVector = XMLoadFloat3(&position);

		// Setup where the camera is looking by default.
		lookAt.x = -position.x;
		lookAt.y = -position.y;
		lookAt.z = -position.z;

		// Load it into a XMVECTOR structure.
		lookAtVector = XMLoadFloat3(&lookAt);

		// Set the yaw (Y axis), pitch (X axis), and roll (Z axis) rotations in radians.
		pitch = 0.f * 0.0174532925f;
		yaw = cam_rot_y * 0.0174532925f;
		roll = 0.f * 0.0174532925f;

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

	shader_set_parameters();

	// shader draw
	{
		// Set the vertex input layout.
		m_deviceContext->IASetInputLayout(m_layout);

		// Set the vertex and pixel shaders that will be used to render this triangle.
		m_deviceContext->VSSetShader(m_vertexShader, NULL, 0);
		m_deviceContext->PSSetShader(m_pixelShader, NULL, 0);

		// Render the triangle.
		m_deviceContext->DrawIndexed(m_indexCount, 0, 0);
	}
}

void renderer_frame_end()
{
	// Present the back buffer to the screen since rendering is complete.
	if (m_vsync_enabled) {
		// Lock to screen refresh rate.
		m_swapChain->Present(1, 0);
	}
	else {
		// Present as fast as possible.
		m_swapChain->Present(0, 0);
	}
}

void shader_output_error(ID3D10Blob* errorMessage, HWND hwnd, WCHAR* shaderFilename)
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

bool shader_load(HWND hwnd, WCHAR* vsFilename, WCHAR* psFilename)
{
	ID3D11Device* device = m_device;

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
	result = device->CreateVertexShader(vertexShaderBuffer->GetBufferPointer(), vertexShaderBuffer->GetBufferSize(), NULL, &m_vertexShader);
	CHECK_RESULT(L"CreateVertexShader");

	// Create the pixel shader from the buffer.
	result = device->CreatePixelShader(pixelShaderBuffer->GetBufferPointer(), pixelShaderBuffer->GetBufferSize(), NULL, &m_pixelShader);
	CHECK_RESULT(L"CreatePixelShader");

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
	CHECK_RESULT(result);

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
	result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	CHECK_RESULT(L"CreateBuffer");
}

void shader_unload()
{
	// Release the matrix constant buffer.
	if (m_matrixBuffer) {
		m_matrixBuffer->Release();
		m_matrixBuffer = 0;
	}

	// Release the layout.
	if (m_layout) {
		m_layout->Release();
		m_layout = 0;
	}

	// Release the pixel shader.
	if (m_pixelShader) {
		m_pixelShader->Release();
		m_pixelShader = 0;
	}

	// Release the vertex shader.
	if (m_vertexShader) {
		m_vertexShader->Release();
		m_vertexShader = 0;
	}
}

void shader_set_parameters()
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
	CHECK(m_deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));

	// Get a pointer to the data in the constant buffer.
	dataPtr = (MatrixBufferType*) mappedResource.pData;

	// Copy the matrices into the constant buffer.
	dataPtr->world = worldMatrix;
	dataPtr->view = viewMatrix;
	dataPtr->projection = projectionMatrix;

	// Unlock the constant buffer.
	m_deviceContext->Unmap(m_matrixBuffer, 0);

	// Set the position of the constant buffer in the vertex shader.
	bufferNumber = 0;

	// Finaly set the constant buffer in the vertex shader with the updated values.
	m_deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
}

void initialize_mesh_buffers()
{
	VertexType* vertices;
	unsigned long* indices;
	D3D11_BUFFER_DESC vertexBufferDesc, indexBufferDesc;
	D3D11_SUBRESOURCE_DATA vertexData, indexData;
	HRESULT result;

	// Set the number of vertices in the vertex array.
	m_vertexCount = 3;

	// Set the number of indices in the index array.
	m_indexCount = 3;

	// Create the vertex array.
	vertices = new VertexType[m_vertexCount];

	// Create the index array.
	indices = new unsigned long[m_indexCount];

	// Load the vertex array with data.
	vertices[0].position = XMFLOAT3(-1.0f, -1.0f, 0.0f);  // Bottom left.
	vertices[0].color = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);

	vertices[1].position = XMFLOAT3(0.0f, 1.0f, 0.0f);  // Top middle.
	vertices[1].color = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);

	vertices[2].position = XMFLOAT3(1.0f, -1.0f, 0.0f);  // Bottom right.
	vertices[2].color = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	// Load the index array with data.
	indices[0] = 0;  // Bottom left.
	indices[1] = 1;  // Top middle.
	indices[2] = 2;  // Bottom right.

	// Set up the description of the static vertex buffer.
	vertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexBufferDesc.ByteWidth = sizeof(VertexType) * m_vertexCount;
	vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexBufferDesc.CPUAccessFlags = 0;
	vertexBufferDesc.MiscFlags = 0;
	vertexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the vertex data.
	vertexData.pSysMem = vertices;
	vertexData.SysMemPitch = 0;
	vertexData.SysMemSlicePitch = 0;

	// Now create the vertex buffer.
	CHECK(m_device->CreateBuffer(&vertexBufferDesc, &vertexData, &m_vertexBuffer));

	// Set up the description of the static index buffer.
	indexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	indexBufferDesc.ByteWidth = sizeof(unsigned long) * m_indexCount;
	indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexBufferDesc.CPUAccessFlags = 0;
	indexBufferDesc.MiscFlags = 0;
	indexBufferDesc.StructureByteStride = 0;

	// Give the subresource structure a pointer to the index data.
	indexData.pSysMem = indices;
	indexData.SysMemPitch = 0;
	indexData.SysMemSlicePitch = 0;

	// Create the index buffer.
	CHECK(m_device->CreateBuffer(&indexBufferDesc, &indexData, &m_indexBuffer));
	
	// Release the arrays now that the vertex and index buffers have been created and loaded.
	delete[] vertices;
	vertices = 0;

	delete[] indices;
	indices = 0;
}