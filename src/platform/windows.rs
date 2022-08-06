use std::{ffi::c_void, mem::size_of, ptr::addr_of_mut};

use windows::Win32::{
	Foundation::{HINSTANCE, HWND, RECT},
	Graphics::Gdi::{HDC, BITMAPINFOHEADER, BI_RGB, BITMAPINFO, StretchDIBits, DIB_RGB_COLORS, SRCCOPY},
	UI::WindowsAndMessaging::WNDCLASSW,
};

use super::{Platform, Color, Axis, PlatformBitmap};

type WindowsBitmapData = *mut c_void;

pub struct Windows {
	win: HWND,
	wc: WNDCLASSW,
	h_instance: HINSTANCE,
	hdc: HDC,
	rect: RECT,
}

impl Platform<WindowsBitmapData> for Windows {
	fn create_bitmap(
		data: *mut Color,
		width: Axis,
		height: Axis,
	) -> super::PlatformBitmap<WindowsBitmapData> {
		PlatformBitmap {
			width,
			height,
			data: data as *mut c_void
		}
	}

	fn draw_bitmap(
		self,
		bitmap: PlatformBitmap<WindowsBitmapData>,
		dx: i32,
		dy: i32,
		src_x: i32,
		src_y: i32,
		w: i32,
		h: i32,
	) {
		let header = BITMAPINFOHEADER {
			biSize: size_of::<BITMAPINFOHEADER>() as u32,
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
				self.hdc,
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
}
