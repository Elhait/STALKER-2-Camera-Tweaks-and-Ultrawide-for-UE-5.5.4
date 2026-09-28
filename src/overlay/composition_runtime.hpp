#pragma once

#include <Windows.h>

extern "C" bool StartOverlayDiscovery(HMODULE module);
extern "C" void NotifyOverlayCameraCoreReady();
extern "C" void RequestOverlayRedraw();
