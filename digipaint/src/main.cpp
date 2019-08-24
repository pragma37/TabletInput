#include "GL_Utils.h"

#include <GLFW/glfw3native.h>

#include "math.h"

/*
#define EASYTAB_IMPLEMENTATION
#include "easytab.h"
*/
//*
#define W_INK
//#define WINTAB

#ifdef  WINTAB
	#include "WINTAB/WINTAB.H"
	#define PACKETDATA	(PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE)
	#define PACKETMODE	PK_BUTTONS
	#include "WINTAB/PKTDEF.H"
	#include "WINTAB/Utils.h"
	#include "WINTAB/MSGPACK.H"

	HCTX hCtx = NULL;
	LOGCONTEXT glogContext = { 0 };
	HCTX static NEAR TabletInit(HWND hWnd);
#endif //  WINTAB

const char* gpszProgramName = "PressureTest";
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

WNDPROC glfw_wndproc;

int main(void)
{
	printf("Hello World! \n");

	GLFWwindow* window;

	/* Initialize the library */
	if (!glfwInit())
		return -1;

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif

	glfwSetErrorCallback(glfwErrorOutput);

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	HWND handle = glfwGetWin32Window(window);


#ifdef WINTAB
	if (LoadWintab() && gpWTInfoA(0, 0, NULL))
	{
		hCtx = TabletInit(handle);
		if (hCtx)
		{
			gpWTEnable(hCtx, TRUE);
			gpWTOverlap(hCtx, TRUE);

			handle = glfwGetWin32Window(window);
			glfw_wndproc = (WNDPROC)GetWindowLongPtr(handle, GWLP_WNDPROC);
			SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR)&WndProc);
			printf("WINTAB SUCCESS\n");
		}
		else
		{
			ShowError("Could Not Open Tablet Context.");
		}
	}
	else
	{
		ShowError("WinTab Services Not Available.");
	}
#endif // WINTAB

#ifdef W_INK
	handle = glfwGetWin32Window(window);
	glfw_wndproc = (WNDPROC)GetWindowLongPtr(handle, GWLP_WNDPROC);
	SetWindowLongPtr(handle, GWLP_WNDPROC, (LONG_PTR)&WndProc);
#endif // W_INK


	if (!gladLoadGL())
	{
		glfwTerminate();
		return -1;
	}

#if _DEBUG
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback((GLDEBUGPROC)glDebugOutput, nullptr);
#endif

	const char* vertex = R"SHADER(
		
	#version 410 core
	layout (location = 0) in vec2 _position;
	layout (location = 1) in vec2 _uv;

	out vec2 position;
	out vec2 world_position;
	out vec2 uv;

	uniform mat3 matrix;
	//uniform mat4 model;

	void main()
	{
		position = _position;
		uv = _uv;
		gl_Position = vec4(matrix * vec3(_position, 1), 1);
		//gl_Position = vec4(_position + _uv, 1, 1);
	}

	)SHADER";
	const char* fragment = R"SHADER(

	#version 410 core
	in vec2 position;
	in vec2 uv;

	out vec4 FragColor;

	uniform vec4 color;

	void main()
	{
		FragColor = vec4(1,1,1,1);//color;
	}

)SHADER";

	std::vector<Vector> line = { {100, 100}, { 100,600 }, { 300,500 }, { 600,100 } };
	std::vector<Vector> tris = line_to_tris(line, 3);
	std::vector<Vertex> vertices = {};
	std::vector<int> indices = {};
	for (Vector tri : tris)
	{
		vertices.push_back(Vertex{ {tri.x, tri.y}, {0,0} });
	}

	unsigned int mesh = load_mesh(&vertices[0], vertices.size());
	unsigned int shader = load_shader(vertex, fragment);

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Poll for and process events */
		glfwPollEvents();
		
		/* Render here */
		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		Matrix matrix = Matrix::Identity();
		matrix.scale(Vector{ (float)w,(float)h } / 2.0);
		matrix.translate(Vector{ (float)w , (float)h } / 2.0);
		matrix.invert();

		float gpu_matrix[9];
		matrix.to_float9(gpu_matrix);

		glUseProgram(shader);
		glUniformMatrix3fv(glGetUniformLocation(shader, "matrix"), 1, false, gpu_matrix);
		//glUniform4f(glGetUniformLocation(shader, "color"), 1, 1, 1, 1);
		
		glBindVertexArray(mesh);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, vertices.size());

		/* Swap front and back buffers */
		glfwSwapBuffers(window);
	}

	glfwTerminate();
#ifdef WINTAB
	gpWTClose(hCtx);
	UnloadWintab();
#endif // WINTAB
	
	return 0;
}

#ifdef W_INK
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//printf("WND_PROC \n");
	static RECT device = {};
	static RECT display = {};
	static int id = 0;

	auto print_rect = [](RECT rect)
	{
		printf("L: %d | R: %d | B: %d | T: %d \n", rect.left, rect.right, rect.bottom, rect.top);
	};
	
	switch (message)
	{
	case WM_POINTERDOWN:
		break;
	case WM_POINTERUPDATE:
	{
		//https://github.com/sketchglass/receive-tablet-event/blob/master/src/EventReceiverWin.cc

		auto pointerId = GET_POINTERID_WPARAM(wParam);
		POINTER_INFO pointerInfo;
		if (!GetPointerInfo(pointerId, &pointerInfo)) {
			break;
		}
		if (pointerInfo.pointerType != PT_PEN) {
			break;
		}
		POINTER_PEN_INFO pointerPenInfo;
		if (!GetPointerPenInfo(pointerId, &pointerPenInfo)) {
			break;
		}

		int button = 0;
		if (pointerInfo.pointerFlags & POINTER_FLAG_FIRSTBUTTON) {
			button = 0;
		}
		else if (pointerInfo.pointerFlags & POINTER_FLAG_SECONDBUTTON) {
			button = 2;
		}
		else if (pointerInfo.pointerFlags & POINTER_FLAG_THIRDBUTTON) {
			button = 1;
		}
		else if (pointerInfo.pointerFlags & POINTER_FLAG_FOURTHBUTTON) {
			button = 3;
		}
		else if (pointerInfo.pointerFlags & POINTER_FLAG_FIFTHBUTTON) {
			button = 4;
		}

		RECT himetricRect, displayRect;
		GetPointerDeviceRects(pointerInfo.sourceDevice, &himetricRect, &displayRect);

		double deviceX = ((double)pointerInfo.ptHimetricLocation.x - himetricRect.left) / (himetricRect.right - himetricRect.left);
		double deviceY = ((double)pointerInfo.ptHimetricLocation.y - himetricRect.top) / (himetricRect.bottom - himetricRect.top);
		double globalX = displayRect.left + deviceX * (displayRect.right - displayRect.left);
		double globalY = displayRect.top + deviceY * (displayRect.bottom - displayRect.top);

		POINT origin{ 0, 0 };
		MapWindowPoints(pointerInfo.hwndTarget, NULL, &origin, 1);

		double localX = globalX - origin.x;
		double localY = globalY - origin.y;

		printf("X: %f | Y: %f \n", localX, localY);
		/*
		printf("POINTER UPDATE \n");
		//if (IS_POINTER_PRIMARY_WPARAM(wParam)) break;

		if (GET_POINTERID_WPARAM(wParam) != id)
		{
			id = GET_POINTERID_WPARAM(wParam);

			POINTER_PEN_INFO info = {};
			GetPointerPenInfo(id, &info);

			//printf("DEVICE X: %d || Y: %d \n", device.right - device.left, device.top - device.bottom);
		}

		POINTER_PEN_INFO info = {};
		GetPointerPenInfo(id, &info);
		GetPointerDeviceRects(info.pointerInfo.sourceDevice, &device, &display);


		POINTER_DEVICE_INFO device_info = {};
		bool success = GetPointerDevice(info.pointerInfo.sourceDevice, &device_info);
		if (success)
		{
			printf("SUCCESS \n");
		}
		else
		{
			printf("FAILED \n");
		}

		printf("%d \n", device_info.monitor);
		POINTER_INFO _info = {};
		GetPointerInfo(id, &_info);

		printf("%ls \n", device_info.productString);

		if (memcmp(&_info, &info.pointerInfo, sizeof(POINTER_INFO)))
		{
			printf("WTF \n");
		}
		else
		{
			printf("EQUAL \n");
		}

		MONITORINFO monitor_info = {};
		monitor_info.cbSize = sizeof(MONITORINFO);
		MONITORINFO* monitor_info_ptr = &monitor_info;
		success = GetMonitorInfo(device_info.monitor, monitor_info_ptr);
		if (success)
		{
			printf("SUCCESS \n");
		}
		else
		{
			printf("FAILED \n");
		}

		printf("MONITOR ");
		print_rect(monitor_info_ptr->rcMonitor);

		printf("WORK ");
		print_rect(monitor_info_ptr->rcWork);

		printf("DEVICE ");
		print_rect(device);

		printf("DISPLAY ");
		print_rect(display);

		float x = ((float)info.pointerInfo.ptHimetricLocationRaw.x / (float)(device.right - device.left)) * (float)(display.right - display.left);
		float y = ((float)info.pointerInfo.ptHimetricLocationRaw.y / (float)(device.top - device.bottom)) * (float)(display.top - display.bottom);
		float z = (float)info.pressure / 1024.0;
		
		//printf("DEVICE X: %d || Y: %d \n", device.right - device.left, device.top - device.bottom);
		//printf("DISPLAY X: %d || Y: %d \n", display.right - display.left, display.top - display.bottom);

		printf("X: %d || Y: %d || Z:%d \n", info.pointerInfo.ptHimetricLocationRaw.x, info.pointerInfo.ptHimetricLocationRaw.y, info.pressure);
		//printf("X: %f || Y: %f || Z:%f \n", x, y, z);
		*/

		break;
	}
	case WM_POINTERUP:
		break;
	}

	return CallWindowProc(glfw_wndproc, hWnd, message, wParam, lParam);
}
#endif // W_INK


#ifdef WINTAB
HCTX static NEAR TabletInit(HWND hWnd)
{
	HINSTANCE hInst = GetModuleHandle(NULL);

	HCTX hctx = NULL;
	UINT wDevice = 0;
	UINT wExtX = 0;
	UINT wExtY = 0;
	UINT wWTInfoRetVal = 0;
	AXIS TabletX = { 0 };
	AXIS TabletY = { 0 };

	// Set option to move system cursor before getting default system context.
	glogContext.lcOptions |= CXO_SYSTEM;

	// Open default system context so that we can get tablet data
	// in screen coordinates (not tablet coordinates).
	wWTInfoRetVal = gpWTInfoA(WTI_DEFSYSCTX, 0, &glogContext);
	WACOM_ASSERT(wWTInfoRetVal == sizeof(LOGCONTEXT));

	WACOM_ASSERT(glogContext.lcOptions & CXO_SYSTEM);

	// modify the digitizing region
	wsprintf(glogContext.lcName, "PrsTest Digitizing %x", hInst);

	// We process WT_PACKET (CXO_MESSAGES) messages.
	glogContext.lcOptions |= CXO_MESSAGES;

	// What data items we want to be included in the tablet packets
	glogContext.lcPktData = PACKETDATA;

	// Which packet items should show change in value since the last
	// packet (referred to as 'relative' data) and which items
	// should be 'absolute'.
	glogContext.lcPktMode = PACKETMODE;

	// This bitfield determines whether or not this context will receive
	// a packet when a value for each packet field changes.  This is not
	// supported by the Intuos Wintab.  Your context will always receive
	// packets, even if there has been no change in the data.
	glogContext.lcMoveMask = PACKETDATA;

	// Which buttons events will be handled by this context.  lcBtnMask
	// is a bitfield with one bit per button.
	glogContext.lcBtnUpMask = glogContext.lcBtnDnMask;

	// Set the entire tablet as active
	wWTInfoRetVal = gpWTInfoA(WTI_DEVICES + 0, DVC_X, &TabletX);
	WACOM_ASSERT(wWTInfoRetVal == sizeof(AXIS));

	wWTInfoRetVal = gpWTInfoA(WTI_DEVICES, DVC_Y, &TabletY);
	WACOM_ASSERT(wWTInfoRetVal == sizeof(AXIS));

	glogContext.lcInOrgX = 0;
	glogContext.lcInOrgY = 0;
	glogContext.lcInExtX = TabletX.axMax;
	glogContext.lcInExtY = TabletY.axMax;

	// Guarantee the output coordinate space to be in screen coordinates.  
	glogContext.lcOutOrgX = GetSystemMetrics(SM_XVIRTUALSCREEN);
	glogContext.lcOutOrgY = GetSystemMetrics(SM_YVIRTUALSCREEN);
	glogContext.lcOutExtX = GetSystemMetrics(SM_CXVIRTUALSCREEN); //SM_CXSCREEN );

	// In Wintab, the tablet origin is lower left.  Move origin to upper left
	// so that it coincides with screen origin.
	glogContext.lcOutExtY = -GetSystemMetrics(SM_CYVIRTUALSCREEN);	//SM_CYSCREEN );

	// Leave the system origin and extents as received:
	// lcSysOrgX, lcSysOrgY, lcSysExtX, lcSysExtY

	// open the region
	// The Wintab spec says we must open the context disabled if we are 
	// using cursor masks.  
	hctx = gpWTOpenA(hWnd, &glogContext, FALSE);

	WacomTrace("HCTX: %i\n", hctx);

	return hctx;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//printf("WNDPROC \n");
	static POINT ptOld, ptNew;
	static UINT prsOld, prsNew;
	PACKET pkt;

	switch (message)
	{
	case WT_PACKET:
		printf("PACKET\n");
		if (gpWTPacket((HCTX)lParam, wParam, &pkt))
		{
			if (HIWORD(pkt.pkButtons) == TBN_DOWN)
			{
				MessageBeep(0);
			}
			ptOld = ptNew;
			prsOld = prsNew;

			ptNew.x = pkt.pkX;
			ptNew.y = pkt.pkY;

			printf("X : %d || Y : %d || PRESSURE : %d \n", pkt.pkX, pkt.pkY, pkt.pkNormalPressure);

			prsNew = pkt.pkNormalPressure;

			if (ptNew.x != ptOld.x ||
				ptNew.y != ptOld.y ||
				prsNew != prsOld)
			{
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		break;

	case WM_ACTIVATE:
		printf("ACTIVATE\n");
		//if switching in the middle, disable the region
		if (hCtx)
		{
			gpWTEnable(hCtx, GET_WM_ACTIVATE_STATE(wParam, lParam));
			if (hCtx && GET_WM_ACTIVATE_STATE(wParam, lParam))
			{
				gpWTOverlap(hCtx, TRUE);
			}
		}
		break;
	}
	
	return CallWindowProc(glfw_wndproc, hWnd, message, wParam, lParam);
}
#endif // WINTAB
