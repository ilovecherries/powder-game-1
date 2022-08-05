use std::borrow::BorrowMut;
use std::ffi::CStr;
use std::os::raw::{c_long, c_short, c_ulong, c_ushort};
use std::os::windows::ffi::OsStrExt;
use std::ptr::{addr_of, addr_of_mut, null_mut};
use std::time::{SystemTime, UNIX_EPOCH};
use std::{
	env,
	ffi::{c_void, OsStr},
	os::raw::{c_char, c_float, c_int},
};
use widestring::{u16str, utf16str};
use windows::core::{InParam, PCWSTR, PWSTR};
use windows::Win32::Foundation::{BOOL, HINSTANCE, HWND, LPARAM, LRESULT, RECT, WPARAM};
use windows::Win32::Graphics::Gdi::{BeginPaint, EndPaint, GetDC, GetSysColorBrush, StretchDIBits, BITMAPINFO, BITMAPINFOHEADER, BI_RGB, DIB_RGB_COLORS, HDC, PAINTSTRUCT, SRCCOPY, RGBQUAD, RedrawWindow, UpdateWindow, InvalidateRect};
use windows::Win32::System::LibraryLoader::GetModuleHandleW;
use windows::Win32::System::SystemInformation::GetTickCount;
use windows::Win32::System::Threading::{GetStartupInfoW, STARTUPINFOW};
use windows::Win32::UI::Controls::Dialogs::{OFN_FILEMUSTEXIST, OFN_PATHMUSTEXIST, OPENFILENAMEW};
use windows::Win32::UI::Input::KeyboardAndMouse::{
	VIRTUAL_KEY, VK_A, VK_D, VK_DOWN, VK_LEFT, VK_RETURN, VK_RIGHT, VK_S, VK_UP, VK_W,
};
use windows::Win32::UI::WindowsAndMessaging::{
	AdjustWindowRect, CreateWindowExW, DefWindowProcW, DestroyWindow, DispatchMessageW,
	LoadCursorW, PeekMessageW, PostQuitMessage, RegisterClassW, TranslateMessage, CS_HREDRAW,
	CS_VREDRAW, CW_USEDEFAULT, HMENU, IDC_ARROW, MSG, PM_REMOVE, WINDOW_EX_STYLE, WM_CLOSE,
	WM_DESTROY, WM_KEYDOWN, WM_MOUSEMOVE, WM_PAINT, WM_QUIT, WNDCLASSW, WS_OVERLAPPEDWINDOW,
	WS_VISIBLE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_RBUTTONDOWN, WM_RBUTTONUP, WM_KEYUP,
};

type Axis = i32;
type Color = u32;

pub type DWORD = c_ulong;
pub type WORD = c_ushort;
#[inline]
#[allow(non_snake_case)]
pub fn LOWORD(l: DWORD) -> WORD {
	(l & 0xffff) as WORD
}
#[inline]
#[allow(non_snake_case)]
pub fn HIWORD(l: DWORD) -> WORD {
	((l >> 16) & 0xffff) as WORD
}
#[inline]
#[allow(non_snake_case)]
pub fn GET_X_LPARAM(lp: LPARAM) -> c_int {
	let LPARAM(l) = lp;
	LOWORD(l as DWORD) as c_short as c_int
}
#[inline]
#[allow(non_snake_case)]
pub fn GET_Y_LPARAM(lp: LPARAM) -> c_int {
	let LPARAM(l) = lp;
	HIWORD(l as DWORD) as c_short as c_int
}

#[repr(C)]
#[allow(non_snake_case)]
#[allow(non_camel_case_types)]
struct Platform_Bitmap {
	width: Axis,
	height: Axis,
	data: *mut c_void,
}

#[repr(C)]
#[allow(non_snake_case)]
struct Point {
	x: c_float,
	y: c_float,
}

#[repr(C)]
#[allow(non_snake_case)]
struct ButtonState {
	gotPress: bool,
	gotRelease: bool,
	held: bool,
	heldNow: bool,
	wasHeld: bool,
}

#[repr(C)]
#[allow(non_snake_case)]
struct MouseState {
	pos: Point,
	oldPos: Point,
	buttons: [ButtonState; 3],
}

/// Window handled by Powder Game
static mut WIN: Option<HWND> = None;
static mut WC: Option<WNDCLASSW> = None;
static mut HINSTANCE2: Option<HINSTANCE> = None;
static mut DC: Option<HDC> = None;
static mut RECT2: Option<RECT> = None;

extern "C" {
	static mut mouse: MouseState;
	static mut Mouse_fallingDirection: c_int;
	static mut Mouse_risingClick: bool;
	static mut Keys: [ButtonState; 256];
	fn Platform_main(argc: i32, argv: *mut c_void);
	fn Platform_frame();
	fn Platform_redraw();
}

/// This converts a string to a vector of wide characters (UTF-16)
fn to_wchar(str: &str) -> Vec<u16> {
	OsStr::new(str)
		.encode_wide()
		.chain(Some(0).into_iter())
		.collect()
}

/// This converts a string to a PCWSTR
macro_rules! PCWSTR {
	($str:expr) => {
		PCWSTR(to_wchar(unsafe { CStr::from_ptr($str) }.to_str().unwrap()).as_ptr())
	};
}

/// This discards the value passed to it, just to make my life easier
macro_rules! discard {
	($exp:expr) => {{
		let _ = $exp;
	}};
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_nanosec() -> c_long {
	unsafe {
		(GetTickCount() as c_long).wrapping_mul(1000000 as c_long)
	}
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_createWindow(width: Axis, height: Axis, title: *mut c_char) {
	let style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	unsafe {
		RECT2 = Some(RECT {
			top: 0,
			left: 0,
			bottom: height,
			right: width,
		});
	}
	let win_title: PCWSTR = PCWSTR!(title);

	unsafe {
		let mut rect = RECT2.unwrap();
		AdjustWindowRect(addr_of_mut!(rect), style, false);
		WIN = Some(CreateWindowExW(
			WINDOW_EX_STYLE(0),
			WC.unwrap().lpszClassName,
			win_title,
			style,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			rect.right - rect.left,
			rect.bottom - rect.top,
			InParam::<HWND>::null().abi(),
			InParam::<HMENU>::null().abi(),
			HINSTANCE2.unwrap_or(HINSTANCE(0)),
			std::ptr::null_mut(),
		));
		DC = Some(GetDC(WIN));
	}
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_drawBitmap(
	bitmap: Platform_Bitmap,
	dx: c_int,
	dy: c_int,
	src_x: c_int,
	src_y: c_int,
	w: c_int,
	h: c_int,
) {
	let header = BITMAPINFOHEADER {
		biSize: std::mem::size_of::<BITMAPINFOHEADER>() as u32,
		biWidth: bitmap.width,
		biHeight: -bitmap.height,
		biPlanes: 1,
		biBitCount: 32,
		biCompression: BI_RGB as u32,
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
	unsafe {
		StretchDIBits(
			DC.unwrap(),
			dx,
			dy,
			w,
			h,
			src_y,
			src_x,
			w,
			h,
			bitmap.data,
			addr_of_mut!(info),
			DIB_RGB_COLORS,
			SRCCOPY,
		);
	}
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_createBitmap(
	data: *mut Color,
	width: Axis,
	height: Axis,
) -> Platform_Bitmap {
	return Platform_Bitmap {
		width,
		height,
		data: data as *mut c_void,
	};
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_openWrite(name: *mut c_void) -> i32 {
	// TODO: need to implement opening C file (from stdio.h) for writing
	return 0;
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_openRead(name: *mut c_void) -> i32 {
	// TODO: need to implement opening C file for reading
	return 0;
}

#[allow(non_snake_case)]
#[no_mangle]
extern "C" fn Platform_selectFile(mode: c_int) -> *mut c_void {
	let filter = utf16str!("All\0*.*\0Text\0*.TXT\0");
	let ext = utf16str!("txt");
	let ofn = OPENFILENAMEW {
		lStructSize: std::mem::size_of::<OPENFILENAMEW>() as u32,
		hwndOwner: unsafe { WIN }.unwrap(),
		// lpstrFile =
		lpstrFilter: PCWSTR(filter.as_ptr()),
		nFilterIndex: 1,
		lpstrFileTitle: InParam::<PWSTR>::null().abi(),
		lpstrInitialDir: InParam::<PCWSTR>::null().abi(),
		lpstrDefExt: PCWSTR(ext.as_ptr()),
		Flags: OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST,
		..Default::default()
	};
	null_mut()
}

fn handle_keypress(num: usize, state: bool) {
	let code = match VIRTUAL_KEY(num as u16) {
		VK_W => b'W',
		VK_A => b'A',
		VK_S => b'S',
		VK_D => b'D',
		VK_LEFT => 37,
		VK_UP => 38,
		VK_RIGHT => 39,
		VK_DOWN => 40,
		VK_RETURN => b'\r',
		_ => 0,
	} as usize;
	if code != 0 {
		unsafe {
			Keys[code].heldNow = state;
			if state {
				Keys[code].gotPress = true;
			} else {
				Keys[code].gotRelease = true;
			}
		}
	}
}

fn mouse_button(num: usize, state: bool) {
	unsafe {
		let btn = mouse.buttons[num].borrow_mut();
		btn.heldNow = state;
		if state {
			btn.gotPress = true;
		} else {
			btn.gotRelease = true
		}
	}
}

#[allow(non_snake_case)]
#[no_mangle]
unsafe extern "system" fn WndProc(
	hwnd: HWND,
	msg: u32,
	w_param: WPARAM,
	l_param: LPARAM,
) -> LRESULT {
	let WPARAM(w_param_val) = w_param;
	match msg {
		WM_PAINT => {
			let mut ps: PAINTSTRUCT = Default::default();
			DC = Some(BeginPaint(hwnd, addr_of_mut!(ps)));
			Platform_redraw();
			EndPaint(hwnd, addr_of_mut!(ps));
		}
		WM_CLOSE => {
			discard!(DestroyWindow(hwnd));
		}
		WM_DESTROY => PostQuitMessage(0),
		WM_MOUSEMOVE => {
			mouse.pos = Point {
				x: GET_X_LPARAM(l_param) as f32,
				y: GET_Y_LPARAM(l_param) as f32,
			}
		}
		WM_LBUTTONDOWN => mouse_button(0, true),
		WM_LBUTTONUP => mouse_button(0, false),
		WM_RBUTTONDOWN => mouse_button(2, true),
		WM_RBUTTONUP => mouse_button(2, false),
		WM_KEYDOWN => handle_keypress(w_param_val, true),
		WM_KEYUP => handle_keypress(w_param_val, false),
		_ => return DefWindowProcW(hwnd, msg, w_param, l_param),
	}
	return LRESULT(0);
}

fn main() {
	let mut si = STARTUPINFOW {
		cb: std::mem::size_of::<STARTUPINFOW>() as u32,
		..Default::default()
	};
	unsafe { GetStartupInfoW(&mut si) };

	let h_instance = unsafe { GetModuleHandleW(InParam::<PCWSTR>::null().abi()) }.unwrap();
	let class_name = u16str!("Powder Game");

	let class = WNDCLASSW {
		style: CS_HREDRAW | CS_VREDRAW,
		lpszClassName: PCWSTR(class_name.as_ptr()),
		hInstance: h_instance,
		hbrBackground: unsafe { GetSysColorBrush(15u32 as i32) },
		lpfnWndProc: Some(WndProc),
		hCursor: unsafe { LoadCursorW(HINSTANCE(0), IDC_ARROW) }.unwrap(),
		..Default::default()
	};

	unsafe {
		WC = Some(class);
		RegisterClassW(addr_of!(class));
	}

	let mut args = env::args().collect::<Vec<_>>();

	unsafe {
		Platform_main(
			args.len().try_into().unwrap(),
			args.as_mut_ptr() as *mut c_void,
		);
	}

	let mut _delta: c_float = Default::default();
	let mut msg: MSG = Default::default();
	loop {
		unsafe {
			while PeekMessageW(
				addr_of_mut!(msg),
				InParam::<HWND>::null().abi(),
				0,
				0,
				PM_REMOVE,
			) != BOOL(0)
			{
				if msg.message == WM_QUIT {
					return;
				}
				TranslateMessage(addr_of_mut!(msg));
				DispatchMessageW(addr_of_mut!(msg));
			}

			Platform_frame();
			Platform_redraw();
			let rect = RECT2.unwrap();
			InvalidateRect(WIN.unwrap(), addr_of!(rect), false);
			RECT2 = Some(rect);
		}
	}
}
