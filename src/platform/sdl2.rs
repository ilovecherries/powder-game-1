use sdl2::{
	event::Event,
	keyboard::Keycode,
	mouse::MouseButton,
	pixels::PixelFormatEnum,
	rect::Rect,
	render::{Canvas, TextureCreator},
	surface::Surface,
	video::{Window, WindowContext},
	Sdl,
};
use std::{
	borrow::BorrowMut,
	time::{Duration, SystemTime, UNIX_EPOCH},
};

use crate::{
	frame, mouse, redraw, ButtonState, Keys, Point, WINDOW_HEIGHT, WINDOW_TITLE, WINDOW_WIDTH,
};

use super::{Axis, Color, Platform, PlatformBitmap};

pub type SDL2BitmapData<'a> = &'a mut [u32];

pub struct SDL2Platform {
	/// SDL Context currently being used
	sdl_context: Sdl,
	/// Canvas that is used by the window
	canvas: Canvas<Window>,
	/// Texture creator from the canvas
	texture_creator: TextureCreator<WindowContext>,
}

fn get_mouse_button<'a>(button: MouseButton) -> Option<&'a mut ButtonState> {
	let index = match button {
		sdl2::mouse::MouseButton::Left => 0,
		sdl2::mouse::MouseButton::Middle => 1,
		sdl2::mouse::MouseButton::Right => 2,
		_ => return None,
	};
	unsafe { Some(mouse.buttons[index].borrow_mut()) }
}

fn get_key<'a>(key: Keycode) -> Option<&'a mut ButtonState> {
	let index = match key {
		Keycode::W => b'W',
		Keycode::A => b'A',
		Keycode::S => b'S',
		Keycode::D => b'D',
		Keycode::Left => 37,
		Keycode::Up => 38,
		Keycode::Right => 39,
		Keycode::Down => 40,
		Keycode::Return => b'\r',
		_ => return None,
	} as usize;
	unsafe { Some(Keys[index].borrow_mut()) }
}

impl Default for SDL2Platform {
	fn default() -> Self {
		let sdl_context = sdl2::init().unwrap();
		let video_subsystem = sdl_context.video().unwrap();
		let window = video_subsystem
			.window(WINDOW_TITLE, WINDOW_WIDTH as u32, WINDOW_HEIGHT as u32)
			.position_centered()
			.build()
			.unwrap();
		let canvas = window
			.into_canvas()
			.present_vsync()
			.accelerated()
			.build()
			.unwrap();
		let texture_creator = canvas.texture_creator();
		SDL2Platform {
			sdl_context,
			canvas,
			texture_creator,
		}
	}
}

impl<'a> Platform<SDL2BitmapData<'a>> for SDL2Platform {
	fn create_bitmap(
		&self,
		data: *mut Color,
		width: Axis,
		height: Axis,
	) -> PlatformBitmap<SDL2BitmapData> {
		// Cast to Rust array
		let d = unsafe { std::slice::from_raw_parts_mut(data, (width * height) as usize) };
		PlatformBitmap {
			width,
			height,
			data: d,
		}
	}

	fn entry(&mut self) {
		let mut event_pump = self.sdl_context.event_pump().unwrap();
		'running: loop {
			self.canvas.clear();
			for event in event_pump.poll_iter() {
				match event {
					Event::Quit { .. }
					| Event::KeyDown {
						keycode: Some(Keycode::Escape),
						..
					} => break 'running,
					Event::MouseMotion {
						x,
						y,
						timestamp: _,
						window_id: _,
						which: _,
						mousestate: _,
						xrel: _,
						yrel: _,
					} => unsafe {
						mouse.pos = Point {
							x: x as f32,
							y: y as f32,
						};
					},
					Event::MouseButtonDown {
						timestamp: _,
						window_id: _,
						which: _,
						mouse_btn,
						clicks: _,
						x: _,
						y: _,
					} => match get_mouse_button(mouse_btn) {
						Some(btn) => {
							btn.heldNow = true;
							btn.gotPress = true;
						}
						_ => {}
					},
					Event::MouseButtonUp {
						timestamp: _,
						window_id: _,
						which: _,
						mouse_btn,
						clicks: _,
						x: _,
						y: _,
					} => match get_mouse_button(mouse_btn) {
						Some(btn) => {
							btn.heldNow = false;
							btn.gotRelease = true;
						}
						_ => {}
					},
					Event::KeyDown {
						timestamp: _,
						window_id: _,
						keycode,
						scancode: _,
						keymod: _,
						repeat: _,
					} => match get_key(keycode.unwrap()) {
						Some(k) => {
							k.heldNow = true;
							k.gotPress = true;
						}
						None => {}
					},
					Event::KeyUp {
						timestamp: _,
						window_id: _,
						keycode,
						scancode: _,
						keymod: _,
						repeat: _,
					} => match get_key(keycode.unwrap()) {
						Some(k) => {
							k.heldNow = false;
							k.gotRelease = true;
						}
						None => {}
					},
					_ => {}
				}
			}
			unsafe {
				frame();
				redraw();
			}
			self.canvas.present();
			::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 120));
		}
	}

	fn draw_bitmap(
		&mut self,
		bitmap: &PlatformBitmap<SDL2BitmapData>,
		dx: i32,
		dy: i32,
		src_x: i32,
		src_y: i32,
		w: i32,
		h: i32,
	) {
		let copy: &mut [u8] = {
			let d = &mut bitmap
				.data
				.iter()
				.map(|x| x | 0xFF000000)
				.collect::<Vec<u32>>()[..];
			let cast = d.as_mut_ptr() as *mut u8;
			unsafe {
				std::slice::from_raw_parts_mut(cast, (bitmap.width * bitmap.height * 4) as usize)
			}
		};

		let surface = Surface::from_data(
			copy,
			bitmap.width as u32,
			bitmap.height as u32,
			(bitmap.width as u32) * 4,
			PixelFormatEnum::ARGB8888,
		)
		.unwrap();
		let texture = surface.as_texture(&self.texture_creator).unwrap();
		let src = Rect::new(src_x, src_y, w as u32, h as u32);
		let dest = Rect::new(dx, dy, w as u32, h as u32);
		self.canvas.copy(&texture, src, dest).unwrap();
	}

	fn nanosec(&self) -> u32 {
		SystemTime::now()
			.duration_since(UNIX_EPOCH)
			.unwrap()
			.as_millis() as u32
	}
}
