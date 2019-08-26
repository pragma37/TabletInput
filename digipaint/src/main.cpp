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
GLFWwindow* window;

std::vector<Vector> line = {};

static float zoom = 1;
static Vector pan = { 0 };

int main(void)
{
	printf("Hello World! \n");

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
		FragColor = color * (mod(uv.x, 100.0)/100.0);
		FragColor = color;
	}

)SHADER";

	/*
	//std::vector<Vector> line = { {100, 100}, { 100,600 }, { 300,500 }, { 600,100 } };
	std::vector<Vector> tris = line_to_tris(line, 1);
	std::vector<Vertex> vertices = {};
	std::vector<int> indices = {};
	for (Vector tri : tris)
	{
		vertices.push_back(Vertex{ {tri.x, tri.y}, {0,0} });
	}

	Mesh mesh = load_mesh(0, 0);
	*/

	unsigned int shader = load_shader(vertex, fragment);
	
	Mesh mesh = {};
	TriangulatedLine tris = {};

	Mesh mesh_line = {};
	TriangulatedLine tris_line = {};

	Mesh mesh_debug = {};
	TriangulatedLine tris_debug = {};

	/* Loop until the user closes the window */
	while (!glfwWindowShouldClose(window))
	{
		/* Poll for and process events */
		glfwPollEvents();

		//printf("FRAME\n");
		
		/* Render here */
		glClearColor(0.1, 0.1, 0.1, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
		
		int w, h;
		glfwGetWindowSize(window, &w, &h);
		glViewport(0, 0, w, h);

		Vector half_res = Vector{ (float)w ,(float)h } / 2.0;
		Matrix world = Matrix::Identity();
		world.translate(-half_res);
		world.translate(pan);
		world.scale({ zoom,zoom });
		world.translate(half_res);
		//printf("PAN X: %f Y: %f ZOOM: %f\n ", pan.x, pan.y, zoom);

		Matrix matrix = Matrix::Identity();
		matrix.scale(Vector{ (float)w,-(float)h } / 2.0);
		matrix.translate(Vector{ (float)w , (float)h } / 2.0);
		matrix.invert();

		matrix = matrix * world;

		float gpu_matrix[9];
		matrix.to_float9(gpu_matrix);

		glUseProgram(shader);
		glUniformMatrix3fv(glGetUniformLocation(shader, "matrix"), 1, false, gpu_matrix);
		glUniform4f(glGetUniformLocation(shader, "color"), 1, 0.5, 1, 1);
		/*
		std::vector<Vector> line = {};// { {100, 100}, { 100,600 }, { 300,500 }, { 600,100 } };
		for (int i = 0; i < 10; i++)
		{
			line.push_back({ (float)(rand() % w), (float)(rand() % h) });
		}
		*/
		if (line.size() > 1)
		{
			tris.vertices.clear();
			tris.indices.clear();

			line_to_tris(line, 30, tris);
			load_mesh(mesh, tris.vertices, tris.indices);
			
			glBindVertexArray(mesh.VAO);

			glUniform4f(glGetUniformLocation(shader, "color"), 1, 0.5, 1, 1);

			glDrawElements(GL_TRIANGLES, tris.indices.size(), GL_UNSIGNED_INT, 0);

			tris_line.vertices.clear();
			tris_line.indices.clear();

			line_to_tris(line, 1, tris_line);
			load_mesh(mesh_line, tris_line.vertices, tris_line.indices);

			glBindVertexArray(mesh_line.VAO);

			glUniform4f(glGetUniformLocation(shader, "color"), 0, 0, 0, 1);

			glDrawElements(GL_TRIANGLES, tris_line.indices.size(), GL_UNSIGNED_INT, 0);

			tris_debug.vertices.clear();
			tris_debug.indices.clear();

			line_to_debug_tris(line, 20, tris_debug);
			load_mesh(mesh_debug, tris_debug.vertices, tris_debug.indices);

			glBindVertexArray(mesh_debug.VAO);

			glUniform4f(glGetUniformLocation(shader, "color"), 0, 0, 0, 1);

			//glDrawElements(GL_TRIANGLES, tris_debug.indices.size(), GL_UNSIGNED_INT, 0);

		}
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

	static bool mouse = false;

	static int x = 0;
	static int y = 0;

	auto print_rect = [](RECT rect)
	{
		printf("L: %d | R: %d | B: %d | T: %d \n", rect.left, rect.right, rect.bottom, rect.top);
	};
	
	switch (message)
	{
	case WM_LBUTTONDOWN:
	{
		mouse = true;
		break;
	}
	case WM_LBUTTONUP:
	{
		mouse = false;
		break;
	}
	case WM_MOUSEMOVE:
	{
		int _x = LOWORD(lParam);
		int _y = HIWORD(lParam);
		if (mouse)
		{
			if (wParam & MK_SHIFT)
			{
				pan.x += (float)(_x - x) / zoom;
				pan.y += (float)(_y - y) / zoom;
			}
			else
			{
				line.push_back({ (float)x, (float)y });
			}
		}
		x = _x;
		y = _y;
		break;
	}
	case WM_MOUSEWHEEL:
	{
		int delta = GET_WHEEL_DELTA_WPARAM(wParam);
		float scalar_offset = 0.005;
		if (delta > 0)
		{
			for (int i = 0; i < delta; i++)
			{
				zoom *= 1.0 + scalar_offset;
			}
		}
		else
		{
			for (int i = 0; i > delta; i--)
			{
				zoom *= 1.0 - scalar_offset;
			}
		}
		break;
	}
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

		if (pointerPenInfo.pressure > 0.0f)
		{
			//glfwGetCursorPos(window, &localX, &localY);
			//line.push_back({ (float)(int)localX, (float)(int)localY });
			line.push_back({ (float)localX, (float)localY });
		}

		//printf("X: %f | Y: %f \n", localX, localY);

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
