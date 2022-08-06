use sdl2::{
	event::Event,
	keyboard::Keycode,
	pixels::PixelFormatEnum,
	rect::Rect,
	render::{Canvas, TextureCreator},
	surface::Surface,
	video::{Window, WindowContext},
	Sdl, VideoSubsystem,
};
use std::time::Duration;

use crate::{frame, redraw, WINDOW_HEIGHT, WINDOW_TITLE, WINDOW_WIDTH};

use super::{Axis, Color, Platform, PlatformBitmap};

pub type SDL2BitmapData<'a> = &'a mut [u32];

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
					_ => {}
				}
			}
			unsafe {
				frame();
				redraw();
			}
			self.canvas.present();
			::std::thread::sleep(Duration::new(0, 1_000_000_000u32 / 60));
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
		let copy = {
			let d = &mut bitmap
				.data
				.iter()
				.map(|x| x | 0xFF000000)
				.collect::<Vec<u32>>()[..];
			let cast = d.as_mut_ptr() as *mut u8;
			unsafe { std::slice::from_raw_parts_mut(cast, (bitmap.width * bitmap.height * 4) as usize) }
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
		todo!()
	}
}
