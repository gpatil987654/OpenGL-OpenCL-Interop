// Standard Headers
#include <windows.h> //Win 32 API
#include <stdio.h>	 //FileIO
#include <stdlib.h>	 //exit()
#include "ogl.h"	 //For Icon resource

#include <iostream>
// OpenGL Header Files
#include <GL/glew.h> //For Programmable Pipeline (Graphics Library Extension Wrangler) This must before gl.h header file
#include <gl/GL.h>

#include "vmath.h"

using namespace vmath;

// OpenCL related header Files
#include <CL/opencl.h>

// MACROS
#define WIN_WIDTH 800
#define WIN_HEIGHT 600

// Link with OpenGl Library
#pragma comment(lib, "glew32.lib")
#pragma comment(lib, "opengl32.lib")

// OpenCL Related Library
#pragma comment(lib, "opencl.lib")

// Global Function Declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Gloabal Variable Declaration
FILE *gpFILE = NULL;
HWND ghwnd = NULL;
DWORD dwStyle;
BOOL gbFullScreen = FALSE;
WINDOWPLACEMENT wpPrev = {sizeof(WINDOWPLACEMENT)};
BOOL gbActive = FALSE;

// OpenGL Related Global Variables
PIXELFORMATDESCRIPTOR pfd;
int iPixelFormatIndex = 0;
HDC ghdc = NULL;
HGLRC ghrc = NULL;

GLuint shaderProgramObject = 0;

enum
{
	AMC_ATTRIBUTE_POSITION = 0,
};

GLuint vao = 0;
GLuint vbo_cpu = 0;

GLuint mvpMatrixUniform = 0;

vmath::mat4 perspectiveProjectionMatrix;

// Opencl Realted Variable Declarations
#define MESH_ARRAY_SIZE 1024 * 1024 * 4

int mesh_width = 1024;
int mesh_height = 1024;

float position[1024][1024][4];

GLuint vbo_gpu;

BOOL bOnGPU = FALSE;

float animationTime = 0.0f;

cl_device_id oclDeviceID;
cl_context oclContext = NULL;
cl_command_queue oclCommandQueue = NULL;
cl_program oclProgram = NULL;
cl_kernel oclKernel = NULL;

cl_mem ocl_graphics_resource = NULL;
cl_int oclResult;

// Entry Point Function
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, int iCmdShow)
{
	// Function Declarations
	int initialise(void);
	void uninitialise(void);
	void display(void);
	void update(void);

	// Local Variable Declaration
	WNDCLASSEX wndclass;
	HWND hwnd;
	MSG msg;
	TCHAR szAppName[] = TEXT("MyWindow");
	int iResult = 0;
	BOOL bDone = FALSE;

	// code
	// gpFILE = fopen("Log.txt", "w");
	// fopen_s for windows
	if (!AttachConsole(ATTACH_PARENT_PROCESS))
	{
		AllocConsole();
	}

	freopen("CONOUT$", "w", stdout);
	std::cout << "Log On Console" << std::endl;

	if (fopen_s(&gpFILE, "Log.txt", "w") != 0)
	{
		MessageBox(NULL, TEXT("LOG FILE OPENING ERROR"), TEXT("ERROR"), MB_OK | MB_ICONERROR);
		exit(0);
	}

	fprintf(gpFILE, "Programme Started Successfully\n");
	fprintf(gpFILE, "------------------------------------------------------------\n\n");

	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.lpfnWndProc = WndProc;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(MyIcon));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hInstance = hInstance;
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszClassName = szAppName;
	wndclass.lpszMenuName = NULL;
	wndclass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(MyIcon));

	RegisterClassEx(&wndclass);

	hwnd = CreateWindowEx(
		WS_EX_APPWINDOW,
		szAppName,
		TEXT("MyWindow"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS | WS_VISIBLE,
		GetSystemMetrics(SM_CXSCREEN) / 2.0 - WIN_WIDTH / 2.0,
		GetSystemMetrics(SM_CYSCREEN) / 2.0 - WIN_HEIGHT / 2.0,
		WIN_WIDTH,
		WIN_HEIGHT,
		NULL,
		NULL,
		hInstance,
		NULL);

	ghwnd = hwnd;

	// Initialisation
	iResult = initialise();

	if (iResult != 0)
	{
		MessageBox(hwnd, TEXT("Initialisation Failed"), TEXT("ERROR"), MB_OK | MB_ICONERROR);
		DestroyWindow(hwnd);
	}

	ShowWindow(hwnd, iCmdShow);
	SetForegroundWindow(hwnd);
	SetFocus(hwnd);

	// GameLoop
	while (bDone == FALSE)
	{

		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{

			if (msg.message == WM_QUIT)
				bDone = TRUE;
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		else
		{

			if (gbActive == TRUE)
			{
				// Render
				display();

				// Update
				update();
			}
		}
	}

	// Uninitialisation
	uninitialise();

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	// Function Declarations
	void ToggleFullScreen(void);
	void resize(int, int);

	switch (iMsg)
	{
	case WM_SETFOCUS:
		gbActive = TRUE;
		break;

	case WM_KILLFOCUS:
		gbActive = FALSE;
		break;

	case WM_SIZE:
		resize(LOWORD(lParam), HIWORD(lParam));
		break;

		break;
	case WM_ERASEBKGND:
		return 0;

	case WM_KEYDOWN:

		switch (LOWORD(wParam))
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);

			break;
		}

		break;

	case WM_CHAR:

		switch (LOWORD(wParam))
		{
		case 'g':
		case 'G':
			bOnGPU = TRUE;
			break;

		case 'c':
		case 'C':
			bOnGPU = FALSE;
			break;

		case 'f':
		case 'F':

			ToggleFullScreen();
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		PostQuitMessage(0);
		break;

	default:
		break;
	}

	return DefWindowProc(hwnd, iMsg, wParam, lParam);
}

void ToggleFullScreen(void)
{
	MONITORINFO mi = {sizeof(MONITORINFO)};

	// code
	if (gbFullScreen == FALSE)
	{ // 1
		// A
		dwStyle = GetWindowLong(ghwnd, GWL_STYLE);

		if (dwStyle & WS_OVERLAPPEDWINDOW)
		{
			// a b c
			if (GetWindowPlacement(ghwnd, &wpPrev) && GetMonitorInfo(MonitorFromWindow(ghwnd, MONITORINFOF_PRIMARY), &mi))
			{
				// i
				SetWindowLong(ghwnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);

				// ii
				SetWindowPos(ghwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left, mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_NOZORDER | SWP_FRAMECHANGED);
			}
		}

		// B
		ShowCursor(FALSE);
		gbFullScreen = TRUE;
	}
	else
	{ // 2

		// A
		SetWindowPlacement(ghwnd, &wpPrev);

		// B
		SetWindowLong(ghwnd, GWL_STYLE, dwStyle | WS_OVERLAPPEDWINDOW);

		// C
		SetWindowPos(ghwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE);

		// D
		ShowCursor(TRUE);

		gbFullScreen = FALSE;
	}
}

int initialise(void)
{
	// Function Declarations
	void resize(int width, int height);
	void printGLInfo(void);
	void uninitialise(void);

	// code
	ZeroMemory(&pfd, sizeof(PIXELFORMATDESCRIPTOR));

	// Initialisation of PIXELFORMATDESRIPTOR
	pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
	pfd.nVersion = 1;
	pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER;
	pfd.iPixelType = PFD_TYPE_RGBA;
	pfd.cColorBits = 32;
	pfd.cRedBits = 8;
	pfd.cGreenBits = 8;
	pfd.cBlueBits = 8;
	pfd.cAlphaBits = 8;
	pfd.cDepthBits = 32;

	ghdc = GetDC(ghwnd);

	if (ghdc == NULL)
	{
		fprintf(gpFILE, "GetDC() Failed\n");
		return -1;
	}

	iPixelFormatIndex = ChoosePixelFormat(ghdc, &pfd);

	if (iPixelFormatIndex == 0)
	{
		fprintf(gpFILE, "ChoosePixelFormat()  Failed\n");
		return -2;
	}

	if (SetPixelFormat(ghdc, iPixelFormatIndex, &pfd) == FALSE)
	{
		fprintf(gpFILE, "SetPixelFormat() Failed\n");
		return -3;
	}

	// Create OpenGL context from device context
	ghrc = wglCreateContext(ghdc);

	if (ghrc == NULL)
	{
		fprintf(gpFILE, "wglCreateContext() Failed\n");
		return -4;
	}

	// Make Rendering Context Current
	if (wglMakeCurrent(ghdc, ghrc) == FALSE)
	{
		fprintf(gpFILE, "wglMakeCurrent() Failed\n");
		return -5;
	}

	// Initialise GLEW - Must for PP
	if (glewInit() != GLEW_OK)
	{
		fprintf(gpFILE, "glewInit() Failed\n");
		return -6;
	}

	// Here OpenGL Starts
	// Set The Clear Color Of Window To Blue
	printGLInfo();
	fprintf(gpFILE, "------------------------------------------------------------------------------------\n\n");

	// Initialisation of OpenCL
	// Check openCL Support and if suppported set opencl device default to zero
	cl_platform_id oclPlatformID;
	cl_uint devCount = 0;
	cl_device_id *oclDeviceIDs = NULL;

	// Get default openCL Platform ID
	oclResult = clGetPlatformIDs(1, &oclPlatformID, NULL);
	if (oclResult != CL_SUCCESS)
	{
		fprintf(gpFILE, "clGetPlatformIDs() Failed\n");
		uninitialise();
		exit(EXIT_FAILURE);
	}

	// Get OpenCL Device Count
	oclResult = clGetDeviceIDs(oclPlatformID, CL_DEVICE_TYPE_GPU, 0, NULL, &devCount);
	if (oclResult != CL_SUCCESS)
	{
		fprintf(gpFILE, "clGetDeviceIDs() Failed to get device count\n");
		uninitialise();
		exit(EXIT_FAILURE);
	}
	else if (devCount == 0)
	{
		fprintf(gpFILE, "There are no OpenCL supported Devices\n");
		uninitialise();
		exit(EXIT_FAILURE);
	}
	else
	{
		oclDeviceIDs = (cl_device_id *)malloc(sizeof(cl_device_id) * devCount);
		if (oclDeviceIDs == NULL)
		{
			fprintf(gpFILE, "Memory Allocation Failed for oclDeviceIDs\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Fill the oclDeviceIDs Array
		oclResult = clGetDeviceIDs(oclPlatformID, CL_DEVICE_TYPE_GPU, devCount, oclDeviceIDs, NULL);

		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clGetDeviceIDs() Failed to fill the oclDeviceIDs array\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Set 0th OpenCL device as defualt device
		oclDeviceID = oclDeviceIDs[0];

		// Free malloced array
		free(oclDeviceIDs);
		oclDeviceIDs = NULL;

		// Declare opencl context properties array
		cl_context_properties oclContextProperties[] =
			{
				CL_GL_CONTEXT_KHR, (cl_context_properties)wglGetCurrentContext(),
				CL_WGL_HDC_KHR, (cl_context_properties)wglGetCurrentDC(),
				CL_CONTEXT_PLATFORM, (cl_context_properties)oclPlatformID,
				0

			};

		// Create OpenCL context
		oclContext = clCreateContext(oclContextProperties, 1, &oclDeviceID, NULL, NULL, &oclResult);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clCreateContext() Failed\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// create Command Queue
		oclCommandQueue = clCreateCommandQueueWithProperties(oclContext, oclDeviceID, 0, &oclResult);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clCreateCommandQueue() Failed\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Declare openCL kernel Code
		const char *oclKernelSourceCode =
			"__kernel void sineWaveKernel(__global float4 *pos,int width,int height,float time)"
			"{"
			"	int x = get_global_id(0);"
			"	int y = get_global_id(1);"
			"	float u = (float)x / (float)width;"
			"	float v = (float)y / (float)height;"
			"  	u = u * 2.0f - 1.0f;"
			"	v = v * 2.0f - 1.0f;"
			"	float frequency = 4.0f;"
			"	float w = sin(u * frequency + time) * cos(v * frequency + time) * 0.5f;"
			"	pos[y * width + x] = (float4)(u, w, v, 1.0f);"
			"}";

		// Create Program form above source code
		oclProgram = clCreateProgramWithSource(oclContext, 1, (const char **)&oclKernelSourceCode, NULL, &oclResult);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clCreateProgramWithSource() Failed\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Build Above Program
		oclResult = clBuildProgram(oclProgram, 0, NULL, "-cl-fast-relaxed-math", NULL, NULL);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clBuildProgram() Failed : %d\n", oclResult);

			size_t len;
			char buffer[1024];
			cl_int oclBuildResult;

			oclBuildResult = clGetProgramBuildInfo(oclProgram, oclDeviceID, CL_PROGRAM_BUILD_LOG, sizeof(buffer), buffer, &len);

			if (oclBuildResult != CL_SUCCESS)
			{
				fprintf(gpFILE, "clGetProgramBuildInfo() Failed\n");
			}
			else
			{
				fprintf(gpFILE, "program Build Log : %s\n", buffer);
			}

			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Creat ekernel from build program
		oclKernel = clCreateKernel(oclProgram, "sineWaveKernel", &oclResult);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clCreateKernel() Failed\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}
	}

	// Step1 - Write Vertex Shader
	const GLchar *vertexShaderSourceCode =
		"#version 460 core"
		"\n"
		"in vec4 aPosition;"
		"uniform mat4 uMVPMatrix;"
		"void main(void)"
		"{"
		"gl_Position = uMVPMatrix * aPosition;"
		"}";

	// Step2 - Create Vertex Shader Object
	GLuint vertexShaderObject = glCreateShader(GL_VERTEX_SHADER);

	// Step3 - Assign Vertex Shader source code to Vertex shader object
	glShaderSource(vertexShaderObject, 1, (const char **)&vertexShaderSourceCode, NULL);

	// Step4 - Compile Vertex Shader
	glCompileShader(vertexShaderObject);

	// Step5 - check for vertex shader compilation errors if any
	//
	GLint status = 0;
	GLint infoLogLength = 0;
	GLchar *szInfoLog = NULL;

	glGetShaderiv(vertexShaderObject, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		glGetShaderiv(vertexShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			szInfoLog = (char *)malloc(infoLogLength + 1);
			if (szInfoLog != NULL)
			{
				glGetShaderInfoLog(vertexShaderObject, infoLogLength + 1, NULL, szInfoLog);
				fprintf(gpFILE, "Vertex Shader Compilation Error Log : %s\n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
			}
		}
		uninitialise();
	}

	// Step6 - Write Source code Of Fragment Shader
	const GLchar *fragmentShaderSourceCode =
		"#version 460 core"
		"\n"
		"out vec4 FragColor;"
		"void main(void)"
		"{"
		"FragColor = vec4(1.0f,0.5f,0.0f,1.0f);"
		"}";

	// Create Fragmnet Shader Object
	GLuint fragmentShaderObject = glCreateShader(GL_FRAGMENT_SHADER);

	// Assign Fragment shader source code fragment shader object
	glShaderSource(fragmentShaderObject, 1, (const GLchar **)&fragmentShaderSourceCode, NULL);

	// Compile Fragment shader object
	glCompileShader(fragmentShaderObject);

	// Compilation Error Checking
	status = 0;
	infoLogLength = 0;
	szInfoLog = NULL;

	glGetShaderiv(fragmentShaderObject, GL_COMPILE_STATUS, &status);

	if (status == GL_FALSE)
	{
		glGetShaderiv(fragmentShaderObject, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			szInfoLog = (char *)malloc(infoLogLength + 1);
			if (szInfoLog != NULL)
			{
				glGetShaderInfoLog(fragmentShaderObject, infoLogLength + 1, NULL, szInfoLog);
				fprintf(gpFILE, "Fragment Shader Compilation Error Log : %s\n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
				// glDeleteShader(vertexShaderObject);
			}
		}
		uninitialise();
	}

	// Shader Program
	shaderProgramObject = glCreateProgram();

	// Attach vertex shader to program object
	glAttachShader(shaderProgramObject, vertexShaderObject);

	// Attach fragment shader to program object
	glAttachShader(shaderProgramObject, fragmentShaderObject);

	// Bind Attirbute Locations with Shader program Object
	glBindAttribLocation(shaderProgramObject, AMC_ATTRIBUTE_POSITION, "aPosition");

	// Link Shader Program
	glLinkProgram(shaderProgramObject);

	// Error Checking
	status = 0;
	infoLogLength = 0;
	szInfoLog = NULL;

	glGetProgramiv(shaderProgramObject, GL_LINK_STATUS, &status);

	if (status == GL_FALSE)
	{
		glGetProgramiv(shaderProgramObject, GL_INFO_LOG_LENGTH, &infoLogLength);
		if (infoLogLength > 0)
		{
			szInfoLog = (char *)malloc(infoLogLength + 1);
			if (szInfoLog != NULL)
			{
				glGetProgramInfoLog(shaderProgramObject, infoLogLength + 1, NULL, szInfoLog);
				fprintf(gpFILE, "Shader Program Linking Error Log  : %s\n", szInfoLog);
				free(szInfoLog);
				szInfoLog = NULL;
			}
		}
		uninitialise();
	}

	// Get Shaders Uniform Locations
	mvpMatrixUniform = glGetUniformLocation(shaderProgramObject, "uMVPMatrix");

	// Init Global position array to 0.0f
	for (int i = 0; i < mesh_width; i++)
	{
		for (int j = 0; j < mesh_height; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				position[i][j][k] = 0.0f;
			}
		}
	}

	// Declare position and color array

	// VAO ->Vertex array Object
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// VBO for CPU
	glGenBuffers(1, &vbo_cpu);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_cpu);
	glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// VBO For GPU
	glGenBuffers(1, &vbo_gpu);
	glBindBuffer(GL_ARRAY_BUFFER, vbo_gpu);
	glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), NULL, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	// Register OpenGL Buffer To openCL graphis Resource
	ocl_graphics_resource = clCreateFromGLBuffer(oclContext, CL_MEM_WRITE_ONLY, vbo_gpu, &oclResult);
	if (oclResult != CL_SUCCESS)
	{
		fprintf(gpFILE, "clCreateFromGLBuffer() Failed\n");
		uninitialise();
		exit(EXIT_FAILURE);
	}

	// Enabling Depth
	glClearDepth(1.0f);		 // Compulsory
	glEnable(GL_DEPTH_TEST); // Compulsory
	glDepthFunc(GL_LEQUAL);	 // Compulsory

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

	// Initialise orthographic projection matrix
	perspectiveProjectionMatrix = vmath::mat4::identity();
	resize(WIN_WIDTH, WIN_HEIGHT);
	return 0;
}

void printGLInfo(void)
{
	// Variable Declarations
	GLint numExtensions;
	GLint i;
	// code
	// Vendor -> Implementor of OpenGL
	fprintf(gpFILE, "OpenGL Vendor   : %s\n", glGetString(GL_VENDOR));
	fprintf(gpFILE, "OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
	fprintf(gpFILE, "OpenGL Version  : %s\n", glGetString(GL_VERSION));
	fprintf(gpFILE, "GLSL Version    : %s\n\n", glGetString(GL_SHADING_LANGUAGE_VERSION));

	// Listing Of Supported Extensions
	glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

	for (i = 0; i < numExtensions; i++)
	{
		fprintf(gpFILE, "%s\n", glGetStringi(GL_EXTENSIONS, i));
	}
}

void resize(int width, int height)
{
	// code
	if (height <= 0)
		height = 1;

	glViewport(0, 0, (GLsizei)width, (GLsizei)height);

	// Set Perspective Projection Matrix
	perspectiveProjectionMatrix = vmath::perspective(45.0f, (float)width / (float)height, 0.1f, 100.0f);
}

void display(void)
{
	// Function Declaration
	void launchCPUKernel(int width, int height, float time);
	void uninitialise(void);

	// code
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramObject);

	// Transformations
	vmath::mat4 modelViewMatrix = vmath::mat4::identity();

	vmath::mat4 modelviewProjectionMatrix = perspectiveProjectionMatrix * modelViewMatrix;

	// Push above ModelView Projection matrix into vertex shader's mvpUniform
	glUniformMatrix4fv(mvpMatrixUniform, 1, GL_FALSE, modelviewProjectionMatrix);

	glBindVertexArray(vao);

	if (bOnGPU == TRUE)
	{
		oclResult = clSetKernelArg(oclKernel, 0, sizeof(cl_mem), (void *)&ocl_graphics_resource);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clSetKernelArg() Failed for 0th argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		oclResult = clSetKernelArg(oclKernel, 1, sizeof(cl_int), (void *)&mesh_width);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clSetKernelArg() Failed for 1st argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		oclResult = clSetKernelArg(oclKernel, 2, sizeof(cl_int), (void *)&mesh_height);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clSetKernelArg() Failed for 2nd argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		oclResult = clSetKernelArg(oclKernel, 3, sizeof(cl_float), (void *)&animationTime);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clSetKernelArg() Failed for 3rd argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// map openCL graphics Resource
		oclResult = clEnqueueAcquireGLObjects(oclCommandQueue, 1, &ocl_graphics_resource, 0, NULL, NULL);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clEnqueueAcquireGLObjects() Failed for 3rd argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Launch OpenCL Kernel
		size_t globalWorkSize[2];
		globalWorkSize[0] = mesh_width;
		globalWorkSize[1] = mesh_height;

		oclResult = clEnqueueNDRangeKernel(oclCommandQueue, oclKernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);

		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clEnqueueAcquireGLObjects() Failed for 3rd argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}
		//

		// unmap OpenCL Grpahics Resource
		oclResult = clEnqueueReleaseGLObjects(oclCommandQueue, 1, &ocl_graphics_resource, 0, NULL, NULL);
		if (oclResult != CL_SUCCESS)
		{
			fprintf(gpFILE, "clEnqueueReleaseGLObjects() Failed for 3rd argument\n");
			uninitialise();
			exit(EXIT_FAILURE);
		}

		// Finish
		clFinish(oclCommandQueue);

		glBindBuffer(GL_ARRAY_BUFFER, vbo_gpu);
	}
	else
	{
		// On CPU
		launchCPUKernel(mesh_width, mesh_height, animationTime);
		glBindBuffer(GL_ARRAY_BUFFER, vbo_cpu);
		glBufferData(GL_ARRAY_BUFFER, MESH_ARRAY_SIZE * sizeof(float), position, GL_DYNAMIC_DRAW);
	}

	// Where we Mapped openGL Buffer
	glVertexAttribPointer(AMC_ATTRIBUTE_POSITION, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(AMC_ATTRIBUTE_POSITION);

	glDrawArrays(GL_POINTS, 0, mesh_width * mesh_height);

	glBindVertexArray(0);
	glUseProgram(0);

	SwapBuffers(ghdc);
}

void update(void)
{
	// code
	animationTime += 0.01f;
}

void launchCPUKernel(int width, int height, float time)
{
	for (int i = 0; i < width; i++)
	{
		for (int j = 0; j < height; j++)
		{
			for (int k = 0; k < 4; k++)
			{
				// u and v in range [0,1]
				float u = (float)i / (float)width;
				float v = (float)j / (float)height;

				// Normalised device cordinate
				// u and v in [-1,1]
				u = 2.0f * u - 1.0f;
				v = 2.0f * v - 1.0f;

				float frequency = 4.0f;

				float w = sinf(u * frequency + time) * cosf(v * frequency + time) * 0.5f;

				if (k == 0) // x
					position[i][j][k] = u;

				if (k == 1) // y
					position[i][j][k] = w;

				if (k == 2) // z
					position[i][j][k] = v;

				if (k == 3) // w
					position[i][j][k] = 1.0f;
			}
		}
	}
}

void uninitialise(void)
{
	// Function Declaration
	void ToggleFullScreen(void);

	// code
	// Delete Shaders objects and Shader Program object
	if (shaderProgramObject)
	{
		glUseProgram(shaderProgramObject);

		GLint numShaders;
		glGetProgramiv(shaderProgramObject, GL_ATTACHED_SHADERS, &numShaders);

		if (numShaders > 0)
		{
			GLuint *pShaders = (GLuint *)malloc(sizeof(GLuint) * numShaders);

			if (pShaders != NULL)
			{
				glGetAttachedShaders(shaderProgramObject, numShaders, NULL, pShaders);

				for (GLint i = 0; i < numShaders; i++)
				{
					glDetachShader(shaderProgramObject, pShaders[i]);
					glDeleteShader(pShaders[i]);
					pShaders[i] = 0;
				}

				free(pShaders);
				pShaders = NULL;
			}
		}

		glUseProgram(0);
		glDeleteProgram(shaderProgramObject);
		shaderProgramObject = 0;
	}

	// Delete VBO of Position
	if (vbo_gpu)
	{
		if (ocl_graphics_resource)
		{
			clReleaseMemObject(ocl_graphics_resource);
			ocl_graphics_resource = NULL;
		}

		glDeleteBuffers(1, &vbo_gpu);
		vbo_gpu = 0;
	}
	// RElease oclKernel
	if (oclKernel)
	{
		clReleaseKernel(oclKernel);
		oclKernel = NULL;
	}

	if (oclProgram)
	{
		clReleaseProgram(oclProgram);
		oclProgram = NULL;
	}

	if (oclCommandQueue)
	{
		clReleaseCommandQueue(oclCommandQueue);
		oclCommandQueue = NULL;
	}

	if (oclContext)
	{
		clReleaseContext(oclContext);
		oclContext = NULL;
	}

	if (vbo_cpu)
	{
		glDeleteBuffers(1, &vbo_cpu);
		vbo_cpu = 0;
	}

	// Delete VAO
	if (vao)
	{
		glDeleteVertexArrays(1, &vao);
		vao = 0;
	}

	// If Application is Exiting in fullScreen
	if (gbFullScreen == TRUE)
		ToggleFullScreen();

	// Make the ghdc as current Dc
	if (wglGetCurrentContext() == ghrc)
	{
		wglMakeCurrent(NULL, NULL);
	}
	// Destroy Rendering context
	if (ghrc)
	{
		wglDeleteContext(ghrc);
		ghrc = NULL;
	}

	// Relase ghdc
	if (ghdc)
	{
		ReleaseDC(ghwnd, ghdc);
		ghdc = NULL;
	}

	// Destroy Window
	if (ghwnd)
	{
		fclose(stdout);
		FreeConsole();

		DestroyWindow(ghwnd);
		ghwnd = NULL;
	}

	// close the log File
	if (gpFILE)
	{
		fprintf(gpFILE, "------------------------------------------------------------------------------------\n\n");
		fprintf(gpFILE, "Programme Ended Successfully\n");
		fclose(gpFILE);
		gpFILE = NULL;
	}
}

// cl.exe /EHsc /I "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\include\CL\"  HelloOpenCL.c /link "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\lib\x64\"
// cl.exe HelloOpenCL.c /I "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\include" /link /LIBPATH:"C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\lib\x64" OpenCL.lib

// C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\include
// C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.2\lib\x64
