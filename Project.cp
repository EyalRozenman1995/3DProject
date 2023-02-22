#include <windows.h> // Header File For Windows
#include <gl\gl.h>	 // Header File For The OpenGL32 Library
#include <gl\glu.h>	 // Header File For The GLu32 Library
#include <opencv2/core.hpp>
#include <iostream>
#include "Project.h"
#include <string>
#include <random>
#include <ctime>
#include <fstream>

#pragma warning(disable : 4996)		 // enable printing
#pragma comment(lib, "opengl32.lib") // Link OpenGL32.lib
#pragma comment(lib, "glu32.lib")	 // Link Glu32.lib

using namespace std;
using namespace cv;

#define HEIGHT_RATIO 1.5f // Ratio That The Y Is Scaled According To The X And Z ( NEW )

HDC hDC = NULL;			// Private GDI Device Context
HGLRC hRC = NULL;		// Permanent Rendering Context
HWND hWnd = NULL;		// Holds Our Window Handle
HINSTANCE hInstance;	// Holds The Instance Of The Application
bool fullscreen = TRUE; // Fullscreen Flag Set To TRUE By Default
bool bRender = TRUE;	// Polygon Flag Set To TRUE By Default ( NEW )
bool keys[256];			// Array Used For The Keyboard Routine
bool active = TRUE;		// Window Active Flag Set To TRUE By Default

// Global variabels
int stepSizeInPixels = 30;
Mat MAP;

string maps[] = { "color0.png", "color1.png", "color2.png", "color2_HQ.png" };
string IMG_PATH = maps[0]; // choose map to render
float scaleFactor = 0.15f;
float rotationAngle = 0.15f;
float diagonalRotationAngle = 0.15f;
float translateY = 0.f;
vector<Triangle> triangles;
int raindropCount = 0;
vector<Raindrop> raindrops;
bool isRaining = false;
float fogIntensity = 0.0;


// Forward declaration
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int DrawGLScene(GLvoid);
void initRaindrops();
void initTriangles();
void handleUserInput();
void scale();
void rotate();
void translate();
void renderTriangles();
void renderRaindrops();
void pickTriangle(int x, int y);
void renderFog();
void updateFog();
void smoothing();
float vertexHeight(int X, int Y);
void delay(int milliseconds);
void increaseRain();


void init()
{
	/*
		called from InitGL
		this function sets up the objects we will later render
	*/
	bool consolePrinting = false; // this exists purely for debugging
	if (consolePrinting)
	{
		// opens second window with console we can print to
		AllocConsole();
		freopen("CONIN$", "r", stdin);
		freopen("CONOUT$", "w", stdout);
		freopen("CONOUT$", "w", stderr);
	}
	MAP = imread(IMG_PATH, IMREAD_COLOR);

	initTriangles();
	initRaindrops();
}

void initTriangles()
{
	/*
		initiate triagnles by looping over the input image
		dividing the image to squres (STEP_SIZE * STEP_SIZE)
		each square is divided into 2 triangles
	*/

	int triangleIndex = 0;
	for (int x = 0; x < (MAP.rows - stepSizeInPixels); x += stepSizeInPixels)
	{
		for (int y = 0; y < (MAP.cols - stepSizeInPixels); y += stepSizeInPixels)
		{
			// Each square is defined by its lower left corner at (x, y) in the matrix 

			// First triangle
			position evenTrianglePos = position(x, y);
			vertex evenTriangleA = vertex(float(x), vertexHeight(x, y), float(y));
			vertex evenTriangleB = vertex(float(x) + stepSizeInPixels, vertexHeight(x + stepSizeInPixels, y), float(y));
			vertex evenTriangleC = vertex(float(x), vertexHeight(x, y + stepSizeInPixels), float(y) + stepSizeInPixels);
			Triangle evenTriangle = Triangle(evenTrianglePos, triangleIndex, evenTriangleA, evenTriangleB, evenTriangleC);
			triangles.push_back(evenTriangle);
			triangleIndex++;

			// Second triangle
			position oddTrianglePos = position(x + stepSizeInPixels, y + stepSizeInPixels);
			vertex oddTriangleA = vertex(float(x) + stepSizeInPixels, vertexHeight(x + stepSizeInPixels, y), float(y));
			vertex oddTriangleB = vertex(float(x) + stepSizeInPixels, vertexHeight(x + stepSizeInPixels, y + stepSizeInPixels), float(y) + stepSizeInPixels);
			vertex oddTriangleC = vertex(float(x), vertexHeight(x, y + stepSizeInPixels), float(y) + stepSizeInPixels);
			Triangle oddTriangle = Triangle(oddTrianglePos, triangleIndex, oddTriangleA, oddTriangleB, oddTriangleC);
			triangles.push_back(oddTriangle);
			triangleIndex++;
		}
	}
}

void initRaindrops()
{
	// make sure to only spawn raindrops within the map borders
	int mapWidth = MAP.cols;
	int mapHeight = MAP.rows;
	for (int i = 0; i < raindropCount; i++)
	{
		Raindrop raindrop;
		// random position for each raindrop
		raindrop.x = float(rand() % mapHeight);
		raindrop.y = float(rand() % 100);
		raindrop.z = float(rand() % mapWidth);
		raindrop.gravityAcceleration = -0.1f;
		raindrop.velocity = 0.0f;
		raindrop.spawnY = float(rand() % 100);
		raindrop.speedFactor = float((rand() % 100)) / 10.0;
		raindrops.push_back(raindrop);
	}
}

void renderFog()
{
	/*
		fog effect, using openGL built in fog functionality
	*/
	GLfloat fogColor[4] = { 0.5, 0.5, 0.5, 0.2 };

	if (fogIntensity > 0.0)
	{
		// set the background color to the fog color so it will look natural
		glClearColor(0.5, 0.5, 0.5, 0.2);
	}
	else
	{
		// restart background color to black
		glClearColor(0., 0., 0., 1.);
	}

	glFogi(GL_FOG_MODE, GL_EXP2);
	glFogfv(GL_FOG_COLOR, fogColor);
	glFogf(GL_FOG_DENSITY, fogIntensity);
	glHint(GL_FOG_HINT, GL_DONT_CARE);
	glFogf(GL_FOG_START, 1.0f);
	glFogf(GL_FOG_END, 50.0f);
	glEnable(GL_FOG);
	glFogi(GL_FOG_MODE, GL_EXP2);
}

void addFog()
{
	fogIntensity += 0.01;
}

void removeFog()
{
	fogIntensity = 0.0;
}

GLvoid ReSizeGLScene(GLsizei width, GLsizei height) // Resize And Initialize The GL Window
{
	/*
		function from Nehe's tutorial
	*/

	if (height == 0) // Prevent A Divide By Zero By
	{
		height = 1; // Making Height Equal One
	}

	glViewport(0, 0, width, height); // Reset The Current Viewport

	glMatrixMode(GL_PROJECTION); // Select The Projection Matrix
	glLoadIdentity();			 // Reset The Projection Matrix

	// Calculate The Aspect Ratio Of The Window
	gluPerspective(45.0f, (GLfloat)width / (GLfloat)height, 0.1f, 500.0f);

	glMatrixMode(GL_MODELVIEW); // Select The Modelview Matrix
	glLoadIdentity();			// Reset The Modelview Matrix
}

int InitGL(GLvoid) // All Setup For OpenGL Goes Here
{
	/*
		function from Nehe's tutorial
		we also call our own init function from here
	*/

	glShadeModel(GL_SMOOTH);						   // Enable Smooth Shading
	glClearColor(0.0f, 0.0f, 0.0f, 0.5f);			   // Black Background
	glClearDepth(1.0f);								   // Depth Buffer Setup
	glEnable(GL_DEPTH_TEST);						   // Enables Depth Testing
	glDepthFunc(GL_LEQUAL);							   // The Type Of Depth Testing To Do
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST); // Really Nice Perspective Calculations

	init();
	return TRUE;
}

GLvoid KillGLWindow(GLvoid) // Properly Kill The Window
{
	/*
		function from Nehe's tutorial
	*/

	if (fullscreen) // Are We In Fullscreen Mode?
	{
		ChangeDisplaySettings(NULL, 0); // If So Switch Back To The Desktop
		ShowCursor(TRUE);				// Show Mouse Pointer
	}

	if (hRC) // Do We Have A Rendering Context?
	{
		if (!wglMakeCurrent(NULL, NULL)) // Are We Able To Release The DC And RC Contexts?
		{
			MessageBox(NULL, "Release Of DC And RC Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}

		if (!wglDeleteContext(hRC)) // Are We Able To Delete The RC?
		{
			MessageBox(NULL, "Release Rendering Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		}
		hRC = NULL; // Set RC To NULL
	}

	if (hDC && !ReleaseDC(hWnd, hDC)) // Are We Able To Release The DC
	{
		MessageBox(NULL, "Release Device Context Failed.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hDC = NULL; // Set DC To NULL
	}

	if (hWnd && !DestroyWindow(hWnd)) // Are We Able To Destroy The Window?
	{
		MessageBox(NULL, "Could Not Release hWnd.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hWnd = NULL; // Set hWnd To NULL
	}

	if (!UnregisterClass("OpenGL", hInstance)) // Are We Able To Unregister Class
	{
		MessageBox(NULL, "Could Not Unregister Class.", "SHUTDOWN ERROR", MB_OK | MB_ICONINFORMATION);
		hInstance = NULL; // Set hInstance To NULL
	}
}

BOOL CreateGLWindow(char *title, int width, int height, int bits, bool fullscreenflag)
{
	/*
		function from Nehe's tutorial
	*/

	GLuint PixelFormat;				  // Holds The Results After Searching For A Match
	WNDCLASS wc;					  // Windows Class Structure
	DWORD dwExStyle;				  // Window Extended Style
	DWORD dwStyle;					  // Window Style
	RECT WindowRect;				  // Grabs Rectangle Upper Left / Lower Right Values
	WindowRect.left = (long)0;		  // Set Left Value To 0
	WindowRect.right = (long)width;	  // Set Right Value To Requested Width
	WindowRect.top = (long)0;		  // Set Top Value To 0
	WindowRect.bottom = (long)height; // Set Bottom Value To Requested Height

	fullscreen = fullscreenflag; // Set The Global Fullscreen Flag

	hInstance = GetModuleHandle(NULL);			   // Grab An Instance For Our Window
	wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC; // Redraw On Size, And Own DC For Window.
	wc.lpfnWndProc = (WNDPROC)WndProc;			   // WndProc Handles Messages
	wc.cbClsExtra = 0;							   // No Extra Window Data
	wc.cbWndExtra = 0;							   // No Extra Window Data
	wc.hInstance = hInstance;					   // Set The Instance
	wc.hIcon = LoadIcon(NULL, IDI_WINLOGO);		   // Load The Default Icon
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);	   // Load The Arrow Pointer
	wc.hbrBackground = NULL;					   // No Background Required For GL
	wc.lpszMenuName = NULL;						   // We Don't Want A Menu
	wc.lpszClassName = "OpenGL";				   // Set The Class Name

	if (!RegisterClass(&wc)) // Attempt To Register The Window Class
	{
		MessageBox(NULL, "Failed To Register The Window Class.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	if (fullscreen) // Attempt Fullscreen Mode?
	{
		DEVMODE dmScreenSettings;								// Device Mode
		memset(&dmScreenSettings, 0, sizeof(dmScreenSettings)); // Makes Sure Memory's Cleared
		dmScreenSettings.dmSize = sizeof(dmScreenSettings);		// Size Of The Devmode Structure
		dmScreenSettings.dmPelsWidth = width;					// Selected Screen Width
		dmScreenSettings.dmPelsHeight = height;					// Selected Screen Height
		dmScreenSettings.dmBitsPerPel = bits;					// Selected Bits Per Pixel
		dmScreenSettings.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;

		// Try To Set Selected Mode And Get Results.  NOTE: CDS_FULLSCREEN Gets Rid Of Start Bar.
		if (ChangeDisplaySettings(&dmScreenSettings, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		{
			// If The Mode Fails, Offer Two Options.  Quit Or Use Windowed Mode.
			if (MessageBox(NULL, "The Requested Fullscreen Mode Is Not Supported By\nYour Video Card. Use Windowed Mode Instead?", "NeHe GL", MB_YESNO | MB_ICONEXCLAMATION) == IDYES)
			{
				fullscreen = FALSE; // Windowed Mode Selected.  Fullscreen = FALSE
			}
			else
			{
				// Pop Up A Message Box Letting User Know The Program Is Closing.
				MessageBox(NULL, "Program Will Now Close.", "ERROR", MB_OK | MB_ICONSTOP);
				return FALSE; // Return FALSE
			}
		}
	}

	if (fullscreen) // Are We Still In Fullscreen Mode?
	{
		dwExStyle = WS_EX_APPWINDOW; // Window Extended Style
		dwStyle = WS_POPUP;			 // Windows Style
		ShowCursor(FALSE);			 // Hide Mouse Pointer
	}
	else
	{
		dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE; // Window Extended Style
		dwStyle = WS_OVERLAPPEDWINDOW;					// Windows Style
	}

	AdjustWindowRectEx(&WindowRect, dwStyle, FALSE, dwExStyle); // Adjust Window To True Requested Size

	// Create The Window
	if (!(hWnd = CreateWindowEx(dwExStyle,							// Extended Style For The Window
		"OpenGL",							// Class Name
		title,								// Window Title
		dwStyle |							// Defined Window Style
		WS_CLIPSIBLINGS |				// Required Window Style
		WS_CLIPCHILDREN,				// Required Window Style
		0, 0,								// Window Position
		WindowRect.right - WindowRect.left, // Calculate Window Width
		WindowRect.bottom - WindowRect.top, // Calculate Window Height
		NULL,								// No Parent Window
		NULL,								// No Menu
		hInstance,							// Instance
		NULL)))								// Dont Pass Anything To WM_CREATE
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Window Creation Error.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	static PIXELFORMATDESCRIPTOR pfd = // pfd Tells Windows How We Want Things To Be
	{
		sizeof(PIXELFORMATDESCRIPTOR), // Size Of This Pixel Format Descriptor
		1,							   // Version Number
		PFD_DRAW_TO_WINDOW |		   // Format Must Support Window
			PFD_SUPPORT_OPENGL |	   // Format Must Support OpenGL
			PFD_DOUBLEBUFFER,		   // Must Support Double Buffering
		PFD_TYPE_RGBA,				   // Request An RGBA Format
		bits,						   // Select Our Color Depth
		0, 0, 0, 0, 0, 0,			   // Color Bits Ignored
		0,							   // No Alpha Buffer
		0,							   // Shift Bit Ignored
		0,							   // No Accumulation Buffer
		0, 0, 0, 0,					   // Accumulation Bits Ignored
		16,							   // 16Bit Z-Buffer (Depth Buffer)
		0,							   // No Stencil Buffer
		0,							   // No Auxiliary Buffer
		PFD_MAIN_PLANE,				   // Main Drawing Layer
		0,							   // Reserved
		0, 0, 0						   // Layer Masks Ignored
	};

	if (!(hDC = GetDC(hWnd))) // Did We Get A Device Context?
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Can't Create A GL Device Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	if (!(PixelFormat = ChoosePixelFormat(hDC, &pfd))) // Did Windows Find A Matching Pixel Format?
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Can't Find A Suitable PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	if (!SetPixelFormat(hDC, PixelFormat, &pfd)) // Are We Able To Set The Pixel Format?
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Can't Set The PixelFormat.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	if (!(hRC = wglCreateContext(hDC))) // Are We Able To Get A Rendering Context?
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Can't Create A GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	if (!wglMakeCurrent(hDC, hRC)) // Try To Activate The Rendering Context
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Can't Activate The GL Rendering Context.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	ShowWindow(hWnd, SW_SHOW);	  // Show The Window
	SetForegroundWindow(hWnd);	  // Slightly Higher Priority
	SetFocus(hWnd);				  // Sets Keyboard Focus To The Window
	ReSizeGLScene(width, height); // Set Up Our Perspective GL Screen

	if (!InitGL()) // Initialize Our Newly Created GL Window
	{
		KillGLWindow(); // Reset The Display
		MessageBox(NULL, "Initialization Failed.", "ERROR", MB_OK | MB_ICONEXCLAMATION);
		return FALSE; // Return FALSE
	}

	return TRUE; // Success
}

float vertexHeight(int X, int Y) // This Returns The Height From A Height Map Index
{
	/*
		get the height of a point by taking its color and performing some interpolation
		based on the assumption that we use heigt maps that sets low points to blue and high points to red
	*/

	int x = X % MAP.rows; // Error Check Our x Value
	int y = Y % MAP.cols; // Error Check Our y Value

	Point pos = Point(y, x);
	Vec3b rgb = MAP.at<Vec3b>(pos);

	// Normalize the RGB values
	float r = rgb[0] / 255.0f;
	float g = rgb[1] / 255.0f;
	float b = rgb[2] / 255.0f;

	// Assign the elevation values
	float lowestElevation = b;
	float highestElevation = r;
	float avgElevation = g;

	// Interpolate the height value
	float height = lowestElevation + (highestElevation - lowestElevation) * avgElevation;

	// multiply by 30 so we can actually see height differences
	return 30. * height;
}

array<uchar, 3> getColorFromMap(int X, int Y)
{
	/*
		returns the rgb values in a given point in the map
		we use it to color our triangles
	*/
	int x = X % MAP.rows; // Error Check Our x Value
	int y = Y % MAP.cols; // Error Check Our y Value

	// return 61. * MAP.at<Vec3b>(Point(y, x)).val[0] / 255.0;
	Point pos = Point(y, x);
	Vec3b rgb = MAP.at<Vec3b>(pos);

	// NOTE: vec3b is BGR not RGB when using byte
	uchar r = rgb[2];
	uchar g = rgb[1];
	uchar b = rgb[0];

	return { r, g, b };
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/*
		function from Nehe's tutorial
		manage user input including mouseclick and keyboard
		we later choose wat to do with the input in handleInput function
	*/

	switch (uMsg) // Check For Windows Messages
	{
	case WM_ACTIVATE: // Watch For Window Activate Message
	{
		if (!HIWORD(wParam)) // Check Minimization State
		{
			active = TRUE; // Program Is Active
		}
		else
		{
			active = FALSE; // Program Is No Longer Active
		}

		return 0; // Return To The Message Loop
	}

	case WM_SYSCOMMAND: // Intercept System Commands
	{
		switch (wParam) // Check System Calls
		{
		case SC_SCREENSAVE:	  // Screensaver Trying To Start?
		case SC_MONITORPOWER: // Monitor Trying To Enter Powersave?
			return 0;		  // Prevent From Happening
		}
		break; // Exit
	}

	case WM_CLOSE: // Did We Receive A Close Message?
	{
		PostQuitMessage(0); // Send A Quit Message
		return 0;			// Jump Back
	}

	case WM_LBUTTONDOWN: // Did We Receive A Left Mouse Click?
	{
		POINT pointerPosition;
		GetCursorPos(&pointerPosition);
		ScreenToClient(hWnd, &pointerPosition); // translate form absolute xy to xy in window
		pickTriangle(pointerPosition.x, pointerPosition.y);

		bRender = !bRender; // Change Rendering State Between Fill/Wire Frame
		return 0;			// Jump Back
	}

	case WM_KEYDOWN: // Is A Key Being Held Down?
	{
		keys[wParam] = TRUE; // If So, Mark It As TRUE
		return 0;			 // Jump Back
	}

	case WM_KEYUP: // Has A Key Been Released?
	{
		keys[wParam] = FALSE; // If So, Mark It As FALSE
		return 0;			  // Jump Back
	}

	case WM_SIZE: // Resize The OpenGL Window
	{
		ReSizeGLScene(LOWORD(lParam), HIWORD(lParam)); // LoWord=Width, HiWord=Height
		return 0;									   // Jump Back
	}
	}

	// Pass All Unhandled Messages To DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	/*
		function from Nehe's tutorial
		this is the main loop, we use it to call funtions like drawGLsene and handleInput
		that will run constantly in a loop

	*/
	MSG msg;		   // Windows Message Structure
	BOOL done = FALSE; // Bool Variable To Exit Loop
	fullscreen = FALSE;

	// Create Our OpenGL Window
	if (!CreateGLWindow((char *)"Terrain Map Mini-Project", 640, 480, 16, fullscreen))
	{
		return 0; // Quit If Window Was Not Created
	}

	while (!done) // Loop That Runs While done=FALSE
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) // Is There A Message Waiting?
		{
			if (msg.message == WM_QUIT) // Have We Received A Quit Message?
			{
				done = TRUE; // If So done=TRUE
			}
			else // If Not, Deal With Window Messages
			{
				TranslateMessage(&msg); // Translate The Message
				DispatchMessage(&msg);	// Dispatch The Message
			}
		}
		else // If There Are No Messages
		{
			// Draw The Scene.  Watch For ESC Key And Quit Messages From DrawGLScene()
			if ((active && !DrawGLScene()) || keys[VK_ESCAPE]) // Active?  Was There A Quit Received?
			{
				done = TRUE; // ESC or DrawGLScene Signalled A Quit
			}
			else if (active) // Not Time To Quit, Update Screen
			{
				// SwapBuffers(hDC); // Swap Buffers (Double Buffering)
			}

			handleUserInput();
		}
	}

	// Shutdown
	KillGLWindow();		 // Kill The Window
	return (msg.wParam); // Exit The Program
}

int DrawGLScene(GLvoid) // Here's Where We Do All The Drawing
{
	/*
		render the scene
	*/
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();
	gluLookAt(220, 60, 200, // eye position
		185, 55, 170, // look at this point
		0.0, 1.0, 0.0); // up direcction
	glScalef(scaleFactor, scaleFactor * HEIGHT_RATIO, scaleFactor); // zoom in / out

	// translateion and rotation
	// this is were we use the values that updated in the 'scale', 'translate' and 'rotate' functions
	glTranslatef(MAP.rows / 2., 0., MAP.cols / 2.); // move 0,0,0 to the middle of the map
	glTranslatef(0, 10.0 * translateY, 0); // up/down
	glRotatef(rotationAngle, 0., 1., 0.); // rotate around y axis
	glRotatef(diagonalRotationAngle, 10., 0., 10.); // rotate around (10, 0, 10)
	glTranslatef(MAP.rows / -2., 0., MAP.cols / -2.);

	renderTriangles();
	renderFog();
	renderRaindrops();

	SwapBuffers(hDC);
	return TRUE;
}

void renderRaindrops()
{
	/*
		update each raindrop position and respawn the drop if its out of bounds
	*/
	for (int i = 0; i < raindrops.size(); i++)
	{
		Raindrop raindrop = raindrops[i];
		glBegin(GL_LINES);
		array<float, 3> randomRGB = Raindrop::RandomRainColor();
		glColor4f(randomRGB[0], randomRGB[1], randomRGB[2], 0.5);
		glVertex3f(raindrop.x, raindrop.y, raindrop.z);
		glVertex3f(raindrop.x, raindrop.y - 0.5, raindrop.z);
		glEnd();
		raindrops[i].y += raindrops[i].velocity / (raindrops[i].speedFactor * 100);
		raindrops[i].velocity += raindrops[i].gravityAcceleration;
		if (raindrops[i].y < 0) // respawn
		{
			raindrops[i].y = raindrops[i].spawnY;
			raindrops[i].velocity = 0.0f;
		}
	}
}

void renderTriangles()
{
	for (auto &t : triangles)
	{
		t.render();
	}
}

void handleUserInput()
{
	scale();
	rotate();
	translate();
	updateFog();
	smoothing();
	increaseRain();
}

void scale()
{
	if (keys[VK_UP])
	{
		scaleFactor += 0.01f;
	}

	if (keys[VK_DOWN])
	{
		scaleFactor -= 0.01f;
	}
}

void rotate()
{
	// Keys are same as ascii values for capital letters
	if (keys[int('A')])
	{
		rotationAngle += 0.4f;
	}

	if (keys[int('D')])
	{
		rotationAngle -= 0.4f;
	}

	if (keys[int('Z')])
	{
		diagonalRotationAngle += 0.2f;
	}

	if (keys[int('X')])
	{
		diagonalRotationAngle -= 0.2f;
	}
}

void translate()
{
	if (keys[int('W')])
	{
		translateY += 0.2f;
	}

	if (keys[int('S')])
	{
		translateY -= 0.2f;
	}
}


void updateFog()
{
	/*
		increse fog intinsity. if its too much cycle back to no fog at all
	*/
	if (keys[int('F')])
	{
		// we use the delay so keyboard input will only register once
		delay(100);
		fogIntensity += 0.001;
	}

	if (fogIntensity > 0.020)
	{
		fogIntensity = 0.0;
	}
}

void smoothing()
{
	/*
		increase resultion by rendering smaller and smaller triangles
		if they become too small we cyle back to stepSizeInPixels = 50
	*/
	if (keys[int('E')])
	{
		delay(100);
		stepSizeInPixels -= 1;
		if (stepSizeInPixels == 1)
		{
			stepSizeInPixels = 50;
		}
		// make sure to only render the new triangles
		triangles.clear();
		initTriangles();
		cout << "stepSizeInPixels: " << stepSizeInPixels << endl;
		cout << "# triangles: " << triangles.size() << endl;
	}
}

void increaseRain()
{
	if (keys[int('Q')]) {
		delay(100);
		raindropCount = (raindropCount + 1000) % 15000;
		raindrops.clear();
		initRaindrops();
	}
}

void pickTriangle(int x, int y)
{
	/*
		handle color picking
		called when the mous is clicked with its x, y coordiante on the window
	*/

	// fog mess up the colors
	glDisable(GL_FOG);

	// draw unique colors
	for (auto &t : triangles)
	{
		t.pickingColor();
	}

	// adjust y value so the origin is bottom left instead of top left because thats what glReadPixel expects
	int viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	int reversedY = viewport[3] - y;

	// read the pixel color under the mouse cursor from the back buffer where we renderd the unique colors
	unsigned char pixel[3];
	glReadPixels(x, reversedY, 1, 1, GL_RGB, GL_UNSIGNED_BYTE, pixel);

	//printf("Pixel values: %d %d %d\n", pixel[0], pixel[1], pixel[2]);

	// look up the triangle id corresponding to the pixel color
	int triangleID = Triangle::getTriangleID(pixel[0], pixel[1], pixel[2]);
	if (triangleID >= 0 && triangleID < triangles.size())
	{
		cout << triangleID << endl;
		triangles[triangleID].changeColor(1, 0, 0); // red just to make it noticable which triangle we picked
	}

	glEnable(GL_FOG);

}

void delay(int milliseconds)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}