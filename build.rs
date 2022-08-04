fn main() {
	std::env::set_var("CC", "clang");
	cc::Build::new()
		.file("ball.c")
		.file("block.c")
		.file("bubble.c")
		.file("dot.c")
		.file("elements.c")
		.file("font.c")
		.file("input.c")
		.file("main.c")
		.file("menu-input.c")
		.file("menu.c")
		.file("object.c")
		.file("random.c")
		.file("reset.c")
		.file("save.c")
		.file("vector.c")
		.file("wheel-frames.c")
		.file("wheel.c")
		// ok final stretch
		.file("render/ball.c")
		.file("render/bg.c")
		.file("render/bubble.c")
		.file("render/dot.c")
		.file("render/draw.c")
		.file("render/menu.c")
		.file("render/object.c")
		.file("render/scale.c")
		.file("render/wheel.c")
		.warnings(false)
		.compile("pg1.exe");
	println!("cargo:rerun-if-changed=*.c");
}
