#include "../common.h"
#include "../dot.h"
#include "../elements.h"
#include "../ball.h"

ELEMENTDEF(ONE) {
	ELEMENTS[1] = (ElementDef){
		.name = "Elem_1",
		.color = 0x606060,
		.friction = 0.5,
	};
}
