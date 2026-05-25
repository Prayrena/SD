#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <math.h>
#include <cassert>
#include <crtdbg.h>
#include "Engine/core/Vertex_PCU.hpp"
#include <string>
#include "Engine/core/EngineCommon.hpp"
#include "Game/App.hpp"
#include "Engine/Input/InputSystem.hpp"
#include <gl/gl.h>					// Include basic OpenGL constants and function declarations
#pragma comment( lib, "opengl32" )	// Link in the OpenGL32.lib static library

App*				g_theApp = nullptr;
extern InputSystem*	g_theInput;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( _In_ HINSTANCE applicationInstanceHandle, _In_opt_ HINSTANCE previousInstance, _In_ LPSTR commandLineString, _In_ int nShowCmd)
{
	UNUSED(applicationInstanceHandle);
	UNUSED(previousInstance);
	UNUSED(commandLineString);
	UNUSED(nShowCmd);

	g_theApp = new App();
	g_theApp -> Startup();

	g_theApp -> RunFrame(); 

	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}


