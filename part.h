#pragma once
#include <stdbool.h>
#include "common.h"
#include "vector.h"
#include "cell.h"

typedef struct Part {
	Point pos;
	Point vel;
	Elem type; //what if we stored type as a pointer to the element table?
	int meta; //I think this is at least 16 bits.
	unsigned char pumpType;
	bool held;
} Part;

int* Part_updateCounts(void);
void Part_shuffle(void);
Part* Part_create(real x, real y, unsigned char element);
void Part_blow(Part* part, Point airvel);
void Part_swap(Part* part1, Part* part2);
void Part_remove(Part* part);
void Part_doRadius(axis x, axis y, axis radius, void (*func)(axis, axis, axis, axis));
bool Part_checkPump(Part* part, Part* pump, int dir);
void Part_liquidUpdate(Part* part, Block* cell, real adv, real x1, real x2, real xr1, real yr1, real yr2, real frc);
extern Part* Part_at[HEIGHT][WIDTH];
extern Part* const Part_EMPTY;
extern Part* const Part_BGFAN;
extern Part* const Part_WHEEL;
extern Part* const Part_BALL;
extern Part* const Part_BLOCK;
extern Part* const Part_0;
extern Part* Part_next;
void Part_reset(int a);
void Part_save(int saveData[W*H], int saveMeta[W*H]);

#define Part_pos(x,y) (&Part_at[(int)(y)][(int)(x)])
#define Part_pos2(pos...) (&Part_at[(int)(pos).y][(int)(pos).x])
#define Part_ofs(x,y) ((int)(x)+(int)(y)*WIDTH)
#define Part_pos3(pos,x,y) (Part_pos2(pos)[Part_ofs(x,y)])

void Part_paint(axis x, axis y, int replace, int type, int meta);
