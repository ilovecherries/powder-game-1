#include <stdio.h>
#include "menu.h"
#include "save.h"
#include "platform.h"

#define DEFCALL(name) void name(void); name()

void Platform_main(int argc, void** argv) {
	if (argc>=2)
		Save_Load_test(argv[1]);
}

void render() {
	DEFCALL(Bg_render);
	
	DEFCALL(Dot_render);
	DEFCALL(Wheel_render);
	DEFCALL(Bubble_render);
	DEFCALL(Entity_render);
	DEFCALL(Ball_render);
	// todo: more bg stuff
	
	DEFCALL(Menu_render);
	DEFCALL(Scale_render);
}

void Platform_frame(void) {
	DEFCALL(Input_update);
	DEFCALL(Random_update);
	
	DEFCALL(Menu_input);
	DEFCALL(Menu_update);
	
	for (int i=0; i<1<<Menu_gameSpeed; i++) {
		long n = Platform_millisec();
		void status(void) {
			//long m = Platform_millisec();
			//printf("%3ld, ", m-n);
			//n=m;
		}
		DEFCALL(Cell_update);
		status();
		DEFCALL(Part_update);
		status();
		DEFCALL(Wheel_update);
		status();
		DEFCALL(Bubble_update);
		status();
		DEFCALL(Entity_update);
		status();
		DEFCALL(Ball_update);
		status();
		//puts("");
	}
	render();
}
