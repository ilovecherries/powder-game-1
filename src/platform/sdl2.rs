use std::{borrow::Borrow, ffi::c_void};

use sdl2::{
	event::Event,
	keyboard::Keycode,
	pixels::PixelFormatEnum,
	render::{Canvas, Texture, TextureCreator},
	surface::Surface,
	video::{Window, WindowContext},
	Sdl, VideoSubsystem,
};

use crate::{frame, WINDOW_HEIGHT, WINDOW_TITLE, WINDOW_WIDTH};

use super::{Axis, Color, Platform, PlatformBitmap};

pub type SDL2BitmapData = Surface<'static>;

pub struct SDL2Platform {
	/// SDL Context currently being used
	sdl_context: Sdl,
	/// Representation of the video subsystem for the window
	video_subsystem: VideoSubsystem,
	/// Canvas that is used by the window
	canvas: Canvas<Window>,
	/// Texture creator from the canvas
	texture_creator: TextureCreator<WindowContext>,
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
			video_subsystem,
			canvas,
			texture_creator,
		}
	}
}

impl Platform<SDL2BitmapData> for SDL2Platform {
	fn create_bitmap(
		&self,
		data: *mut Color,
		width: Axis,
		height: Axis,
	) -> PlatformBitmap<SDL2BitmapData> {
		// Cast the data to *mut u8 so that we can convert it into a Rust array
		let data_cast = data as *mut u8;
		// Cast to Rust array
		let d = unsafe { std::slice::from_raw_parts_mut(data_cast, (width * height * 4) as usize) };
		let surface = Surface::from_data(
			d,
			width as u32,
			height as u32,
			(width as u32) * 4,
			PixelFormatEnum::RGBX8888,
		)
		.unwrap();
		PlatformBitmap {
			width,
			height,
			data: surface,
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
					_ => {}
				}
			}
			unsafe {
				frame();
			}
			self.canvas.present();
		}
	}

	fn draw_bitmap(
		&self,
		bitmap: PlatformBitmap<SDL2BitmapData>,
		dx: i32,
		dy: i32,
		src_x: i32,
		src_y: i32,
		w: i32,
		h: i32,
	) {
		todo!()
	}

	fn nanosec(&self) -> u32 {
		todo!()
	}
}
