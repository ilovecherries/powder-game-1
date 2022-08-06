pub mod sdl2;

/// Axis parameter for bitmaps
pub type Axis = i32;
/// Color data for bitmaps
pub type Color = u32;

/// Platform agnostic bitmap representation
pub struct PlatformBitmap<T> {
	pub width: Axis,
	pub height: Axis,
	pub data: T,
}

/// Implements a platform target for Powder Game to interface with.
pub trait Platform<T> {
	/// Creates a bitmap from color data sent from Powder Game
	///
	/// # Arguments
	///
	/// * `data` - Bitmap color data sent from Powder Game.
	/// * `width` - Width of the bitmap data.
	/// * `height` - Height of the bitmap data.
	fn create_bitmap(
		&'static self,
		data: *mut Color,
		width: Axis,
		height: Axis,
	) -> PlatformBitmap<T>;
	/// Entry-point for the platform
	fn entry(&mut self) {}
	/// Draws platform bitmap onto the screen
	///
	/// # Arguments
	///
	/// * `bitmap` - Bitmap to draw onto the screen.
	/// * `dx` - X coordinate of where to draw on screen.
	/// * `dy` - Y coordinate of where to draw on screen.
	/// * `src_x` - X coordinate of the source bitmap to get data.
	/// * `src_y` - Y coordinate of the source bitmap to get data.
	/// * `w` - Width of bitmap data to draw onto the screen.
	/// * `h` - Height of bitmap data to draw onto the screen.
	fn draw_bitmap(
		&mut self,
		bitmap: &PlatformBitmap<T>,
		dx: i32,
		dy: i32,
		src_x: i32,
		src_y: i32,
		w: i32,
		h: i32,
	);
	/// Used to get nanoseconds from the platform
	fn nanosec(&self) -> u32;
}
