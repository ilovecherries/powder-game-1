use std::{os::raw::{c_int, c_long}, ffi::c_void};

use platform::{
	sdl2::{SDL2Platform, SDL2BitmapData},
	Platform, PlatformBitmap, Color,
};

pub mod platform;

const W: usize = 400;
const H: usize = 300;
/// Width of the game to render
const WIDTH: usize = 8+W+8;
/// Height of the game to render
const HEIGHT: usize = 8+H+8;
/// Width of the menu to render.
const MENU_WIDTH: usize = 400;
/// Height of the menu to render.
const MENU_HEIGHT: usize = 156;
/// Width of the window/viewport to create.
const WINDOW_WIDTH: usize = MENU_WIDTH;
/// Height of the window/viewport to create.
const WINDOW_HEIGHT: usize = 300 + MENU_HEIGHT;
/// Name of the window to be created
const WINDOW_TITLE: &str = "Powder Game";

/// Bitmap that is used for rendering the viewport onto
static mut SIM_BITMAP: Option<PlatformBitmap<SDL2BitmapData>> = None;
/// Bitmap that is used for rendering the menu onto
static mut MENU_BITMAP: Option<PlatformBitmap<SDL2BitmapData>> = None;
/// Platform
static mut PLATFORM: Option<SDL2Platform> = None;

extern "C" {
	static mut Menu_gameSpeed: c_int;
	static mut Menu_paused: bool;

	// static mut grp: [[Color; WIDTH]; HEIGHT];
	static mut grp: [Color; WIDTH * HEIGHT];
	// static mut Menu_grp: [[Color; MENU_WIDTH]; MENU_HEIGHT];
	static mut Menu_grp: [Color; MENU_WIDTH * MENU_HEIGHT];

	fn Menu_render();

	fn Input_update();
	fn Random_update();
	fn Menu_input();
	fn Menu_update();
	fn Block_update1();
	fn Block_update();
	fn Wheel_update1();
	fn Dot_update();
	fn Wheel_update();
	fn Bubble_update();
	fn Object_update();
	fn Ball_update();
	fn Input_update2();

	fn Bg_render();
	fn Dot_render();
	fn Wheel_render();
	fn Bubble_render();
	fn Object_render();
	fn Ball_render();
	fn Bg_render2();
	fn Scale_render();
}

unsafe fn render() {
	Bg_render();

	Dot_render();
	Wheel_render();
	Bubble_render();
	Object_render();
	Ball_render();

	Bg_render2();

	Menu_render();
	Scale_render();
}

// these are some temporary functions until i implement the menu in rust and
// therefore don't need these linked in the build process
/// Provides a C interface for nanoseconds
#[allow(non_snake_case)]
#[no_mangle]
unsafe extern "C" fn Platform_nanosec() -> c_long {
	0
}
/// Provides a C interface for selecting the file
#[allow(non_snake_case)]
#[no_mangle]
unsafe extern "C" fn Platform_selectFile(_mode: c_int) -> *mut c_void {
	std::ptr::null_mut()
}
/// Provides a C interface for opening a file to write to
#[allow(non_snake_case)]
#[no_mangle]
unsafe extern "C" fn Platform_openWrite(_name: *mut c_void) -> c_int {
	-1
}

unsafe fn frame() {
	Menu_render();
	Input_update();
	Random_update();

	Menu_input();
	Menu_update();

	let mut i = 0;
	while i < 1 << Menu_gameSpeed {
		Block_update1();
		if Menu_paused {
			continue;
		}
		Block_update();
		Wheel_update1();
		Dot_update();
		Wheel_update();
		Bubble_update();
		Object_update();
		Ball_update();

		i += 1;
	}
	render();
	Input_update2();
}

fn main() {
	unsafe {
		PLATFORM = Some(SDL2Platform::default());
		SIM_BITMAP = Some(PLATFORM.unwrap().create_bitmap(grp.as_mut_ptr(), WIDTH as i32, HEIGHT as i32));
		PLATFORM.unwrap().entry();
	}
}
