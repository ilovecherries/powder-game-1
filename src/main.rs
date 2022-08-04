use std::ffi::CStr;
use std::{env, ffi::{c_void, OsStr}, os::raw::{c_char, c_int}};
use std::os::windows::ffi::OsStrExt;
use windows::Win32::Graphics::Gdi::GetSysColorBrush;
use windows::Win32::UI::WindowsAndMessaging::{
	CreateWindowExW,
	WS_OVERLAPPEDWINDOW,
	WS_VISIBLE,
	AdjustWindowRect,
	HMENU,
	CW_USEDEFAULT,
	WINDOW_EX_STYLE,
	WNDCLASSW,
	WNDCLASS_STYLES,
	CS_HREDRAW,
	CS_VREDRAW,
	COLOR_3DFACE,
	LoadCursorW,
	IDC_ARROW
};
use window::Win32::Graphics::Gdi::{
	GetDC,
	HDC,
	GetSysColorBrush,
	StretchDIBits,
	BITMAPINFO,
	BITMAPINFOHEADER,
	BI_RGB,
	DIR_RGB_COLORS,
	SRCCOPY
};
use windows::core::{PCWSTR, InParam};
use windows::Win32::Foundation::{RECT, HWND, HINSTANCE};
use windows::Win32::System::Threading::{GetStartupInfoW, STARTUPINFOW};
use windows::Win32::System::LibraryLoader::GetModuleHandleW;

type Axis = i32;
type Color = u32;

#[repr(C)]
#[allow(non_snake_case)]
#[allow(non_camel_case_types)]
struct Platform_Bitmap {
	width: axis,
	height: axis,
	data: *mut c_void
}

/// Window handled by Powder Game
static mut WIN: Option<HWND> = None;
static mut WC: Option<WNDCLASSW> = None;
static mut HINSTANCE2: Option<HINSTANCE> = None;
static mut DC: Option<HDC> = None;

extern "C" {
	fn Platform_main(argc: i32, argv: *mut c_void);
}

fn to_wchar(str : &str) -> Vec<u16> {
	OsStr::new(str).encode_wide(). chain(Some(0).into_iter()).collect()
}

macro_rules! PCWSTR {
	($str:expr) => {
		PCWSTR(to_wchar(unsafe { CStr::from_ptr(str) }.to_str().unwrap()).as_ptr())
	};
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_createWindow(width: Axis, height: Axis,
		title: *mut c_char) {
	let style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	let mut rect = RECT {
		top: 0,
		left: 0,
		bottom: height,
		right: width
	};
	let rect_ptr = &mut rect as *mut RECT;
	let win_title: PCWSTR = PCWSTR!(title);

	unsafe {
		AdjustWindowRect(rect_ptr, style, false);
		WIN = Some(CreateWindowExW(WINDOW_EX_STYLE(0),
			InParam::<PCWSTR>::null().abi(),
			win_title,
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rect.right-rect.left,
			rect.bottom-rect.top,
			InParam::<HWND>::null().abi(),
			InParam::<HMENU>::null().abi(),
			HINSTANCE2.unwrap_or(HINSTANCE(0)), 
			std::ptr::null_mut()));
		DC = Some(GetDC(WIN));
	}
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_drawBitmap(bitmap: Platform_Bitmap, dx: c_int,
		dy: c_int, srcx: c_int, srcy: c_int, w: c_int, h: c_int) {
	let mut header = BITMAPINFOHEADER {
		biSize: std::mem::size_ofBITMAPINFOHEADER>(),
		biWidth: bitmap.width,
		biHeight: -bitmap.height,
		biPlanes: 1,
		biBitCount: 32,
		biCompression: BI_RGB,
		biSizeImage: 0,
		biXPelsPerMeter: 0,
		biYPelsPerMeter: 0,
		biClrUsed: 0,
		biClrImportant: 0,
	};
	let mut info = BITMAPINFO {
		bmiHeader: header,
		..Default::default()
	};
	StretchDIBits(
		HDC,
		dx, dy, w, h,
		srcy, srcx, w, h,
		bitmap.data,
		info as *mut BITMAPINFO,
		DIR_RGB_COLORS,
		SRCCOPY
	)
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_createBitmap(data: *mut Color, width: axis,
		height: axis) {
	return Platform_Bitmap {
		width,
		height,
		data: data as *mut c_void
	}
}

#[allow(non_snake_case)]
extern "C" fn Platform_openWrite(name: *mut c_void) -> i32 {
	// TODO: need to implement opening C file (from stdio.h) for writing
	return 0;
}

#[allow(non_snake_case)]
extern "C" fn Platform_openRead(name: *mut c_void) -> i32 {
	// TODO: need to implement opening C file for reading
	return 0;
}

// need to use this later when we use args from WinMain
// https://stackoverflow.com/a/68326741
fn main() {
	let mut si = STARTUPINFOW {
		cb: std::mem::size_of::<STARTUPINFOW>() as u32,
		..Default::default()
	};
	unsafe { GetStartupInfoW(&mut si) };

	let hInstance = GetModuleHandleW(InParam::<PCWSTR>::null().abi());
	
	wc = WNDCLASSW {
		style: CS_HREDRAW | CS_VREDRAW,
		lpszClassName: PCWSTR!("Powder Game"),
		hInstance,
		hbrBackground:  unsafe { GetSysColorBrush(COLOR_3DFACE) },
		lpfnWndProc: /* NEEDT TO FILL THIS IN */,
		hCursor: unsafe { LoadCursorW(HINSTANCE(0), IDC_ARROW) },
		..Default::default()
	};

	let mut args = env::args().collect::<Vec<_>>();

	unsafe {
		Platform_main(args.len().try_into().unwrap(), args.as_mut_ptr() as *mut c_void);
	}

	print!("Hello, World!\n");
}
