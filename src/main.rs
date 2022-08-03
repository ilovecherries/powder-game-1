use std::ffi::CStr;
use std::{env, ffi::{c_void, OsStr}, os::raw::c_char};
use std::os::windows::ffi::OsStrExt;

use windows::Win32::UI::WindowsAndMessaging::{
	CreateWindowExW,
	WS_OVERLAPPEDWINDOW,
	WS_VISIBLE,
	AdjustWindowRect,
	HMENU,
	CW_USEDEFAULT,
	WINDOW_EX_STYLE,
	WNDCLASSW
};
use windows::core::{PCWSTR, InParam};
use windows::Win32::Foundation::{RECT, HWND, HINSTANCE};


type Axis = i32;

/// Window handled by Powder Game
static mut WIN: Option<HWND> = None;
static mut WC: Option<WNDCLASSW> = None;
static mut HINSTANCE2: Option<HINSTANCE> = None;

extern "C" {
	fn Platform_main(argc: i32, argv: *mut c_void);
}

fn to_wchar(str : &str) -> Vec<u16> {
	OsStr::new(str).encode_wide(). chain(Some(0).into_iter()).collect()
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
	let win_title: PCWSTR = PCWSTR(to_wchar(unsafe { CStr::from_ptr(title) }.to_str().unwrap()).as_ptr());

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
			std::ptr::null_mut()))
	}
}

// need to use this later when we use args from WinMain
// https://stackoverflow.com/a/68326741
fn main() {
	let mut args = env::args().collect::<Vec<_>>();

	unsafe {
		Platform_main(args.len().try_into().unwrap(), args.as_mut_ptr() as *mut c_void);
	}

	print!("Hello, World!\n");
}
