#include <fstream>
#include <d3dcompiler.h>

#include "Game.h"

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
