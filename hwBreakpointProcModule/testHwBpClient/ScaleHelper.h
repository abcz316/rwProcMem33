#pragma once
#include <windows.h>
static size_t GetScaleWidth(size_t originWidth) {
	static int horizontalDPI = 0;
	if (horizontalDPI == 0) {
		HDC hDesktopDC = ::GetDC(0);
		horizontalDPI = GetDeviceCaps(hDesktopDC, LOGPIXELSX);
		::ReleaseDC(0, hDesktopDC);
	}
	return MulDiv(originWidth, horizontalDPI, 96/*不可以修改*/);
}
static size_t GetScaleHeight(size_t originHeight) {
	static int verticalDPI = 0;
	if (verticalDPI == 0) {
		HDC hDesktopDC = ::GetDC(0);
		verticalDPI = GetDeviceCaps(hDesktopDC, LOGPIXELSY);
		::ReleaseDC(0, hDesktopDC);
	}
	return MulDiv(originHeight, verticalDPI, 96/*不可以修改*/);
}