#include "GUI.h"

void SetRect(GUIRect *pRect, int left, int top, int right, int bottom) {
	pRect->left = left;
	pRect->top = top;
	pRect->right = right;
	pRect->bottom = bottom;
}