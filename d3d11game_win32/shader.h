#include <fstream>
#include <d3dcompiler.h>


#define WIDE2(x) L##x
#define WIDE1(x) WIDE2(x)

#define LOG(...) log_impl(WIDE1(__FILE__), __LINE__, __VA_ARGS__)
void log_impl(const wchar_t* file, int line, LPCWSTR format, ...);

//#include "utils.h"
//#include "framework.h"

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

// shader-related stuff
ID3D11VertexShader* m_vertexShader;
ID3D11PixelShader* m_pixelShader;
ID3D11InputLayout* m_layout;
ID3D11Buffer* m_matrixBuffer;

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

bool shader_load(HWND hwnd, ID3D11Device* device, WCHAR* vsFilename, WCHAR* psFilename)
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
	/*matrixBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	matrixBufferDesc.ByteWidth = sizeof(MatrixBufferType);
	matrixBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	matrixBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	matrixBufferDesc.MiscFlags = 0;
	matrixBufferDesc.StructureByteStride = 0;*/

	// Create the constant buffer pointer so we can access the vertex shader constant buffer from within this class.
	/*result = device->CreateBuffer(&matrixBufferDesc, NULL, &m_matrixBuffer);
	CHECK_RESULT(L"CreateBuffer");*/
}

//void shader_unload()
//{
//	// Release the matrix constant buffer.
//	if (m_matrixBuffer) {
//		m_matrixBuffer->Release();
//		m_matrixBuffer = 0;
//	}
//
//	// Release the layout.
//	if (m_layout) {
//		m_layout->Release();
//		m_layout = 0;
//	}
//
//	// Release the pixel shader.
//	if (m_pixelShader) {
//		m_pixelShader->Release();
//		m_pixelShader = 0;
//	}
//
//	// Release the vertex shader.
//	if (m_vertexShader) {
//		m_vertexShader->Release();
//		m_vertexShader = 0;
//	}
//}
//
//void shader_set_parameters()
//{
//	HRESULT result;
//	D3D11_MAPPED_SUBRESOURCE mappedResource;
//	MatrixBufferType* dataPtr;
//	unsigned int bufferNumber;
//
//	// Transpose the matrices to prepare them for the shader.
//	XMMATRIX worldMatrix = XMMatrixTranspose(m_worldMatrix);
//	XMMATRIX viewMatrix = XMMatrixTranspose(m_viewMatrix);
//	XMMATRIX projectionMatrix = XMMatrixTranspose(m_projectionMatrix);
//
//	// Lock the constant buffer so it can be written to.
//	CHECK(m_deviceContext->Map(m_matrixBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource));
//
//	// Get a pointer to the data in the constant buffer.
//	dataPtr = (MatrixBufferType*)mappedResource.pData;
//
//	// Copy the matrices into the constant buffer.
//	dataPtr->world = worldMatrix;
//	dataPtr->view = viewMatrix;
//	dataPtr->projection = projectionMatrix;
//
//	// Unlock the constant buffer.
//	m_deviceContext->Unmap(m_matrixBuffer, 0);
//
//	// Set the position of the constant buffer in the vertex shader.
//	bufferNumber = 0;
//
//	// Finaly set the constant buffer in the vertex shader with the updated values.
//	m_deviceContext->VSSetConstantBuffers(bufferNumber, 1, &m_matrixBuffer);
//}
