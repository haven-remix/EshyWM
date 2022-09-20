#include "util.h"
#include <algorithm>
#include <sstream>
#include <vector>

std::string to_string(const XEvent& e) {
	static const char* const X_EVENT_TYPE_NAMES[] = {
		"",
		"",
		"KeyPress",
		"KeyRelease",
		"ButtonPress",
		"ButtonRelease",
		"MotionNotify",
		"EnterNotify",
		"LeaveNotify",
		"FocusIn",
		"FocusOut",
		"KeymapNotify",
		"Expose",
		"GraphicsExpose",
		"NoExpose",
		"VisibilityNotify",
		"CreateNotify",
		"DestroyNotify",
		"UnmapNotify",
		"MapNotify",
		"MapRequest",
		"ReparentNotify",
		"ConfigureNotify",
		"ConfigureRequest",
		"GravityNotify",
		"ResizeRequest",
		"CirculateNotify",
		"CirculateRequest",
		"PropertyNotify",
		"SelectionClear",
		"SelectionRequest",
		"SelectionNotify",
		"ColormapNotify",
		"ClientMessage",
		"MappingNotify",
		"GeneralEvent",
	};

	if (e.type < 2 || e.type >= LASTEvent) {
		std::ostringstream out;
		out << "Unknown (" << e.type << ")";
		return out.str();
	}

	//Compile properties we care about.
	std::vector<std::pair<std::string, std::string>> properties;
	switch (e.type) {
	case CreateNotify:
		properties.emplace_back("window", to_string(e.xcreatewindow.window));
		properties.emplace_back("parent", to_string(e.xcreatewindow.parent));
		properties.emplace_back("size", size<int>(e.xcreatewindow.width, e.xcreatewindow.height).to_string());
		properties.emplace_back("position", Position<int>(e.xcreatewindow.x, e.xcreatewindow.y).to_string());
		properties.emplace_back("border_width", to_string(e.xcreatewindow.border_width));
		properties.emplace_back("override_redirect", to_string(static_cast<bool>(e.xcreatewindow.override_redirect)));
		break;
	case DestroyNotify:
		properties.emplace_back("window", to_string(e.xdestroywindow.window));
		break;
	case MapNotify:
		properties.emplace_back("window", to_string(e.xmap.window));
		properties.emplace_back("event", to_string(e.xmap.event));
		properties.emplace_back("override_redirect", to_string(static_cast<bool>(e.xmap.override_redirect)));
		break;
	case UnmapNotify:
		properties.emplace_back("window", to_string(e.xunmap.window));
		properties.emplace_back("event", to_string(e.xunmap.event));
		properties.emplace_back("from_configure", to_string(static_cast<bool>(e.xunmap.from_configure)));
		break;
	case ConfigureNotify:
		properties.emplace_back("window", to_string(e.xconfigure.window));
		properties.emplace_back("size", size<int>(e.xconfigure.width, e.xconfigure.height).to_string());
		properties.emplace_back("position", Position<int>(e.xconfigure.x, e.xconfigure.y).to_string());
		properties.emplace_back("border_width", to_string(e.xconfigure.border_width));
		properties.emplace_back("override_redirect", to_string(static_cast<bool>(e.xconfigure.override_redirect)));
		break;
	case ReparentNotify:
		properties.emplace_back("window", to_string(e.xreparent.window));
		properties.emplace_back("parent", to_string(e.xreparent.parent));
		properties.emplace_back("position", Position<int>(e.xreparent.x, e.xreparent.y).to_string());
		properties.emplace_back("override_redirect", to_string(static_cast<bool>(e.xreparent.override_redirect)));
		break;
	case MapRequest:
		properties.emplace_back("window", to_string(e.xmaprequest.window));
		break;
	case ConfigureRequest:
		properties.emplace_back("window", to_string(e.xconfigurerequest.window));
		properties.emplace_back("parent", to_string(e.xconfigurerequest.parent));
		properties.emplace_back("value_mask", XConfigureWindowValueMaskToString(e.xconfigurerequest.value_mask));
		properties.emplace_back("position", Position<int>(e.xconfigurerequest.x, e.xconfigurerequest.y).to_string());
		properties.emplace_back("size", size<int>(e.xconfigurerequest.width, e.xconfigurerequest.height).to_string());
		properties.emplace_back("border_width", to_string(e.xconfigurerequest.border_width));
		break;
	case ButtonPress:
	case ButtonRelease:
		properties.emplace_back("window", to_string(e.xbutton.window));
		properties.emplace_back("button", to_string(e.xbutton.button));
		properties.emplace_back("position_root", Position<int>(e.xbutton.x_root, e.xbutton.y_root).to_string());
		break;
	case MotionNotify:
		properties.emplace_back("window", to_string(e.xmotion.window));
		properties.emplace_back("position_root", Position<int>(e.xmotion.x_root, e.xmotion.y_root).to_string());
		properties.emplace_back("state", to_string(e.xmotion.state));
		properties.emplace_back("time", to_string(e.xmotion.time));
		break;
	case KeyPress:
	case KeyRelease:
		properties.emplace_back("window", to_string(e.xkey.window));
		properties.emplace_back("state", to_string(e.xkey.state));
		properties.emplace_back("keycode", to_string(e.xkey.keycode));
		break;
	default:
		//No properties are printed for unused events.
		break;
	};

	//Build final string.
	const std::string properties_string = Join(properties, ", ", [] (const std::pair<std::string, std::string> &pair)
	{
		return pair.first + ": " + pair.second;
	});

	std::ostringstream out;
	out << X_EVENT_TYPE_NAMES[e.type] << " { " << properties_string << " }";

	return out.str();
}

std::string XConfigureWindowValueMaskToString(unsigned long value_mask) {
	std::vector<std::string> masks;

	if (value_mask & CWX)
	{
		masks.emplace_back("X");
	}
	if (value_mask & CWY)
	{
		masks.emplace_back("Y");
	}
	if (value_mask & CWWidth)
	{
		masks.emplace_back("Width");
	}
	if (value_mask & CWHeight)
	{
		masks.emplace_back("Height");
	}
	if (value_mask & CWBorderWidth)
	{
		masks.emplace_back("BorderWidth");
	}
	if (value_mask & CWSibling)
	{
		masks.emplace_back("Sibling");
	}
	if (value_mask & CWStackMode)
	{
		masks.emplace_back("StackMode");
	}

	return Join(masks, "|");
}

std::string XRequestCodeToString(unsigned char request_code) {
	static const char* const X_REQUEST_CODE_NAMES[] = {
		"",
		"CreateWindow",
		"ChangeWindowAttributes",
		"GetWindowAttributes",
		"DestroyWindow",
		"DestroySubwindows",
		"ChangeSaveSet",
		"ReparentWindow",
		"MapWindow",
		"MapSubwindows",
		"UnmapWindow",
		"UnmapSubwindows",
		"ConfigureWindow",
		"CirculateWindow",
		"GetGeometry",
		"QueryTree",
		"InternAtom",
		"GetAtomName",
		"ChangeProperty",
		"DeleteProperty",
		"GetProperty",
		"ListProperties",
		"SetSelectionOwner",
		"GetSelectionOwner",
		"ConvertSelection",
		"SendEvent",
		"GrabPointer",
		"UngrabPointer",
		"GrabButton",
		"UngrabButton",
		"ChangeActivePointerGrab",
		"GrabKeyboard",
		"UngrabKeyboard",
		"GrabKey",
		"UngrabKey",
		"AllowEvents",
		"GrabServer",
		"UngrabServer",
		"QueryPointer",
		"GetMotionEvents",
		"TranslateCoords",
		"WarpPointer",
		"SetInputFocus",
		"GetInputFocus",
		"QueryKeymap",
		"OpenFont",
		"CloseFont",
		"QueryFont",
		"QueryTextExtents",
		"ListFonts",
		"ListFontsWithInfo",
		"SetFontPath",
		"GetFontPath",
		"CreatePixmap",
		"FreePixmap",
		"CreateGC",
		"ChangeGC",
		"CopyGC",
		"SetDashes",
		"SetClipRectangles",
		"FreeGC",
		"ClearArea",
		"CopyArea",
		"CopyPlane",
		"PolyPoint",
		"PolyLine",
		"PolySegment",
		"PolyRectangle",
		"PolyArc",
		"FillPoly",
		"PolyFillRectangle",
		"PolyFillArc",
		"PutImage",
		"GetImage",
		"PolyText8",
		"PolyText16",
		"ImageText8",
		"ImageText16",
		"CreateColormap",
		"FreeColormap",
		"CopyColormapAndFree",
		"InstallColormap",
		"UninstallColormap",
		"ListInstalledColormaps",
		"AllocColor",
		"AllocNamedColor",
		"AllocColorCells",
		"AllocColorPlanes",
		"FreeColors",
		"StoreColors",
		"StoreNamedColor",
		"QueryColors",
		"LookupColor",
		"CreateCursor",
		"CreateGlyphCursor",
		"FreeCursor",
		"RecolorCursor",
		"QueryBestsize",
		"QueryExtension",
		"ListExtensions",
		"ChangeKeyboardMapping",
		"GetKeyboardMapping",
		"ChangeKeyboardControl",
		"GetKeyboardControl",
		"Bell",
		"ChangePointerControl",
		"GetPointerControl",
		"SetScreenSaver",
		"GetScreenSaver",
		"ChangeHosts",
		"ListHosts",
		"SetAccessControl",
		"SetCloseDownMode",
		"KillClient",
		"RotateProperties",
		"ForceScreenSaver",
		"SetPointerMapping",
		"GetPointerMapping",
		"SetModifierMapping",
		"GetModifierMapping",
		"NoOperation",
	};

  	return X_REQUEST_CODE_NAMES[request_code];
}