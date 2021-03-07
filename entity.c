#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "common.h"
#include "vector.h"
#include "elements.h"
#include "menu.h" //todo: split this into sim properties (edge mode etc.) and others
#include "bg.h"
#include "part.h"
#include "input.h"
#include "entity.h"
#include "cell.h"

Entity entitys[Entity_MAX];
Entity* Entity_next = entitys;

void Entity_create(real x, real y, int type, int meta) {
	if (Entity_next >= entitys+Entity_MAX)
		return;
	Entity* oldPlayer = NULL;
	if (type==Elem_PLAYER || type==Elem_PLAYER2) {
		int totalPlayers=0;
		for (Entity* e=entitys; e<Entity_next; e++) {
			if (e->type == Entity_PLAYER) {
				totalPlayers++;
				oldPlayer = e;
			}
		}
		if (totalPlayers>=2 || (meta && !ELEMENTS[meta].playerValid))
			return;
	}
	*Entity_next = (Entity){
		.vel = {0,0},
		.type = Entity_FIGHTER,
		.age = 0,
		.held = 0,
		.meta = meta,
		.isPlayer2 = false,
	};
	// idk why this one goes to 20...
	for (int i=0; i<20; i++)
		Entity_next->parts[i].oldPos = Entity_next->parts[i].pos = (Point){x+Random_(4), y+Random_(4)};
	for (int i=0; i<Entity_PARTS; i++)
		Entity_next->parts[i].touching = 0;
	if (type==Elem_BOX) {
		if (meta != 10) {
			Entity_next->type = Entity_BOX;
		} else {
			Entity_next->type = Entity_CREATE;
			Entity_next->meta = 0;
			Entity_next->parts[0].pos = (Point){x,y};
			Entity_next->parts[0].oldPos = (Point){x,y};
		}
	} else if (type==Elem_PLAYER) {
		Entity_next->type = Entity_PLAYER;
		if (oldPlayer)
			Entity_next->isPlayer2 = !oldPlayer->isPlayer2;
		//Entity_next->isPlayer2 = q;
	} else if (type==Elem_PLAYER2) {
		Entity_next->type = Entity_PLAYER;
		if (!oldPlayer)
			Entity_next->isPlayer2 = false;
		else {
			Point pos = oldPlayer->parts[0].pos;
			int b = (int)pos.x>>2<<2;
			int c = (int)pos.y>>2<<2;
			if (x<b)
				Entity_next->isPlayer2 = true;
			else if (x>b)
				Entity_next->isPlayer2 = false;
			else if (y<c)
				Entity_next->isPlayer2 = true;
			else
				Entity_next->isPlayer2 = false;
			oldPlayer->isPlayer2 = !Entity_next->isPlayer2;
		}
	}
	Entity_next++;
}

void fighterWalk(Entity* a) {
	EntityNode* left = &a->parts[4];
	EntityNode* right = &a->parts[5];
	if (left->touching && right->touching) {
		int r = Random_int(100);
		if (r<5) {
			if (left->pos.x < right->pos.x)
				left->pos.xy += (Point){4,-4}.xy;
			else
				right->pos.xy += (Point){4,-4}.xy;
		} else if (r<10) {
			if (left->pos.x > right->pos.x)
				left->pos.xy += (Point){-4,-4}.xy;
			else
				right->pos.xy += (Point){-4,-4}.xy;
		}
	} else {
		if (left->touching) {
			if (Rnd_perchance(2))
				left->pos.xy += (Point){Random_2(-4,4),-4}.xy;
		} else if (right->touching) {
			if (Rnd_perchance(2))
				right->pos.xy += (Point){Random_2(-4,4),-4}.xy;
		}
	}
}

void fighterKick(Entity* a) {
	for (Entity* r=entitys; r<Entity_next; r++) {
		if (r==a) continue;
		if (r->type==Entity_FIGHTER || r->type==Entity_FIGHTER+1 || r->type==Entity_PLAYER) {
			for (int i=4; i<=5; i++) {
				EntityNode* foot = &a->parts[i];
				real g = abs(foot->pos.x - r->parts[0].pos.x);
				real q = foot->pos.y - r->parts[0].pos.y;
				if (g<=2 && q>=0 && q<=6) {
					r->vel.xy += Cxy(foot->pos.x-foot->oldPos.x, 2*(foot->pos.y-foot->oldPos.y));
					if (r->type==Entity_FIGHTER)
						r->type = Entity_FIGHTER+1;
					r->age = 0;
				}
			}
		}
	}
}

void checkDrag(Entity* e, int last) {
	// try to start grab
	if (!e->held) {
		if (Menu_dragStart) {
			// find the closest node (within 20 units)
			real closest=20;
			for (int i=0; i<last; i++) {
				Point diff = Vec_sub2(Menu_pen, e->parts[i].oldPos);
				real dist = Vec_fastDist(diff);
				if (dist<closest) {
					closest = dist;
					e->held = i+1;
				}
			}
			printf("got node %d\n", e->held-1);
		}
	// being held
	} else if (Menu_dragging) {
		Point* p = &e->parts[e->held-1].pos;
		p->x += 0.2*(Pen_x-p->x);
		p->y += 0.2*(Pen_y-p->y);
	// release
	} else
		e->held = 0;
}

void Entity_remove(Entity* entity) {
	Entity* last = Entity_next-1;
	*entity = *last;
	Entity_next--;
}

// update node position
static void moveNode(EntityNode* node, real gravity, real slowdown) {
	Point movement = {.c= node->pos.c - node->oldPos.c};
	node->oldPos = node->pos;
	movement.y += gravity;
	node->pos.c += (movement.c * slowdown);
}

// try to make the nodes be a certain distance apart
// mul1 and mul2 control how much each node is pulled
// currently they are always = but perhaps you could
// use them to simulate a kind of "weight"
void pullNodes(Entity* e, int n1, int n2, real length, real mul1, real mul2) {
	EntityNode* node1 = &e->parts[n1];
	EntityNode* node2 = &e->parts[n2];
	Point diff = {.c= node2->pos.c - node1->pos.c};
	real dist = Vec_fastNormalize(&diff);
	if (dist) {
		length -= dist;
		node1->pos.c -= diff.c * (length*mul1);
		node2->pos.c += diff.c * (length*mul2);
	}
}

static void updateNode(EntityNode* node, real dd, bool noCollide, bool held) {
	Point e = Vec_sub2(node->pos, node->oldPos);
	node->pos = node->oldPos;
	if (dd!=0) {
		Block* cell = &Part_blocks[(int)node->pos.y/4][(int)node->pos.x/4];
		e.c += cell->vel.c*dd;
	}
	int d;
	if (!held) {
		real f = Vec_fastDist(e)+1;
		if (f>=8) {
			e.c *= 3.8/f;
			d = 2;
		} else if (f>=4) {
			e.c *= 0.5;
			d = 2;
		} else
			d = 1;
	} else {
		d = (int)(Vec_fastDist(e)/3)+1;
		e.c *= 1/d;
	}
	node->touching = 0;
	if (noCollide) {
		e.c *= d;
		node->pos.c += e.c;
		node->pos.x = clamp(node->pos.x, 4, WIDTH-5);
		node->pos.y = clamp(node->pos.y, 4, HEIGHT-5);
	} else {
		for (int c=0; c<d; c++) {
			double b = node->pos.y + e.y;
			if (b<4 || b>=H+12) {
				node->touching = Elem_EMPTY;
				break;
			}
			//added
			if (node->pos.x<0 || node->pos.x>=W) {
				node->touching = Elem_EMPTY;
				break;
			}
			Part* part = *Part_pos(node->pos.x, b);
			int type = part->type;
			if (part <= Part_BGFAN) {
				node->pos.y = b;
			} else if (part <= Part_BLOCK) {
				e.x *= 0.5;
				e.y = -e.y;
				node->touching = type;
			} else {
				e.x *= ELEMENTS[type].friction;
				node->touching = type;
				if (e.y<0)
					node->pos.y = b;
				else if (ELEMENTS[type].state==State_LIQUID && type != Elem_MERCURY)
					node->pos.y = b;
				else
					e.y = -e.y;
			}
			b = node->pos.x + e.x;
			if (b<4 || b>=WIDTH-4) {
				node->touching = Elem_EMPTY;
				break;
			}
			part = (*Part_at)[Part_ofs(b, part->pos.y)];
			type = part->type;
			if (part <= Part_BGFAN) {
				node->pos.x = b;
			} else if (part <= Part_BLOCK) {
				e.y *= 0.5;
				e.x = -e.x;
				node->touching = type;
			} else {
				e.y *= ELEMENTS[type].friction;
				e.x = -e.x;
				node->touching = type;
				if (ELEMENTS[type].state==State_LIQUID || type==Elem_WOOD)
					node->pos.x = b;
			}
		}
	}
}

static int checkTouching(Entity* e, int start, int end) {
	int b=0;
	for (; start<end; start++) {
		int touching = e->parts[start].touching;
		if (touching == Elem_EMPTY)
			return -5;
		else if (touching<0)
			b = 1;
		else if (ELEMENTS[touching].state == State_HOT)
			return 3;
		else if (touching==Elem_ACID)
			return -5;
		else if (touching)
			b = 1;
	}
	return b;
}

typedef struct Player {
	int Xe; //walk animation cooldown
	int Ye; //jump  "
	bool facing;
} Player;

Player players[2];

void Entity_update(void) {
	for (Entity* a=entitys; a<Entity_next; a++) {
		void copyPos(int dest, int src) {
			a->parts[dest].pos = a->parts[src].pos;
			a->parts[dest].oldPos = a->parts[src].oldPos;
		}
		if (a->type == Entity_PLAYER) {
			/// todo: held || gotPress perhaps?
			bool left = a->isPlayer2==0 ? Keys[37].held : Keys[65].held||Keys[97].held;
			bool right = a->isPlayer2==0 ? Keys[39].held : Keys[68].held||Keys[100].held;
			bool up = a->isPlayer2==0 ? Keys[38].held : Keys[87].held||Keys[119].held;
			bool down = a->isPlayer2==0 ? Keys[40].held : Keys[83].held||Keys[115].held;
			Player* player = &players[a->isPlayer2];
			a->age++;
			bool w =
				Part_pos2(a->parts[4].pos)[Part_ofs(0,1)]>Part_BGFAN ||
				Part_pos2(a->parts[4].pos)[0]>Part_BGFAN;
			bool rightFoot =
				Part_pos2(a->parts[5].pos)[Part_ofs(0,1)]>Part_BGFAN ||
				Part_pos2(a->parts[5].pos)[0]>Part_BGFAN;
			if (down && a->meta == Elem_BIRD) {
				for (int b=0;b<6;b++)
					moveNode(&a->parts[b], 0.01, 0.997);
			} else { //dubious
				moveNode(&a->parts[0], -0.2, 0.995);
				moveNode(&a->parts[1], -0.1, 0.995);
				moveNode(&a->parts[2], 0, 0.995);
				moveNode(&a->parts[3], 0, 0.995);
				moveNode(&a->parts[4], 0.3, 0.995);
				moveNode(&a->parts[5], 0.3, 0.995);
			}
			checkDrag(a, 6);
			a->parts[0].pos.c += a->vel.c;
			a->vel.c *= 0.5;
			if (a->vel.x!=0)
				a->type = Entity_PLAYER+2;
			if (player->Xe>0)
				player->Xe--;
			if (player->Xe!=0 || w!=1 || rightFoot!=1) {
				if (right) {
					if (a->parts[1].pos.x - a->parts[1].oldPos.x < 0)
						a->parts[1].pos.x += 0.1;
					player->facing = 1;
				} else if (left) {
					if (a->parts[1].pos.x - a->parts[1].oldPos.x > 0)
						a->parts[1].pos.x -= 0.1;
					player->facing = 0;
				}
			} else {
				real b = 0.8;
				if (right) {
					player->Xe = 15;
					if (a->parts[4].pos.x < a->parts[5].pos.x) {
						a->parts[2].pos.x += 4*b;
						a->parts[2].pos.y -= 3*b;
					} else {
						a->parts[3].pos.x += 4*b;
						a->parts[3].pos.y -= 3*b;
					}
					player->facing = 1;
				} else if (left) {
					player->Xe = 15;
					if (a->parts[4].pos.x > a->parts[5].pos.x) {
						a->parts[2].pos.x -= 4*b;
						a->parts[2].pos.y -= 3*b;
					} else {
						a->parts[3].pos.x -= 4*b;
						a->parts[3].pos.y -= 3*b;
					}
					player->facing = 0;
				}
			}
			if (player->Ye>1)
				player->Ye--;
			if (player->Ye>0 && (w==1||rightFoot==1))
				player->Ye--;
			if (player->Ye==0 && up) {
				player->Ye=50;
				a->parts[4].pos.y -= 6;
				a->parts[5].pos.y -= 6;
			}
			pullNodes(a, 0, 1, 4, 0.5, 0.5);
			pullNodes(a, 1, 2, 4, 0.5, 0.5);
			pullNodes(a, 1, 3, 4, 0.5, 0.5);
			pullNodes(a, 2, 4, 5, 0.5, 0.5);
			pullNodes(a, 3, 5, 5, 0.5, 0.5);
			pullNodes(a, 2, 3, 5, 0.1, 0.1);
			for (int i=0;i<6;i++)
				updateNode(&a->parts[i], 0.1, (i<4), a->held>0);
			for (int y=0;y<3;y++) {
				for (int x=-1;x<2;x++) {
					Part* p = Part_pos2(a->parts[0].oldPos)[Part_ofs(x,y)];
					if (p == Part_BGFAN)
						a->meta = Elem_FAN;
					else if (p >= Part_0 && ELEMENTS[p->type].playerValid==1)
						a->meta = p->type;
				}
			}
			if (!(up && (left||right)) && down && a->meta) {
				int w = a->parts[0].pos.x;
				int b = a->parts[0].pos.y+1;
				// nitro fly
				if (a->meta == Elem_NITRO) {
					w += player->facing ? -8 : 8;
					b += 12;
				}
				w=clamp(w,8,W+8-1);
				b=clamp(b,8,H+8-1);
				// fan blow air
				if (a->meta == Elem_FAN) {
					Block* cell = &Part_blocks[b>>2][w>>2];
					if (cell->block == 0)
						cell->vel.x += player->facing ? 1 : -1;
					//spit
				} else if (a->meta != Elem_BIRD && Part_at[b][w] == Part_EMPTY) {
					Part* f = Part_create(w, b, a->meta);
					if (f>=Part_0) {
						if (player->facing==0)
							f->vel.x -= 20;
						if (player->facing==1)
							f->vel.x += 20;
						f->vel.y += Random_(3)+1;
						switch (a->meta) {
						when(Elem_FIRE):
							f->vel.x *= 3;
							f->vel.y += 18;
							f->meta = 2;
						when(Elem_SUPERBALL):
							f->vel.y = 20;
						when(Elem_STONE):;
							f->vel.c *= 0.1;
						when(Elem_LASER):
							f->meta = player->facing ? 1 : 5;
						}
					}
				}
			}
			// damage
			if (checkTouching(a,0,6)==3 || checkTouching(a,0,6)==-5)
				a->type = Entity_PLAYER+2;
			// edge
			if (Menu_edgeMode==1) {
				for (int r=4;r<=5;r++) {
					int w=0,b=0;
					if (a->parts[r].pos.x<8) {
						w = W+6;
						b = a->parts[r].oldPos.y;
					} else if (a->parts[r].pos.x>=W-8) {//CHECK
						w = 10;
						b = a->parts[r].oldPos.y;
					}
					if (w+b!=0) {
						a->type = Entity_PLAYER;
						Part* part = Part_at[b][w];
						if (part > Part_BGFAN) {
							if (part < Part_0) {
								a->parts[r].pos = a->parts[r].oldPos;
								continue;
							} else if (ELEMENTS[part->type].state != State_LIQUID) {
								a->parts[r].pos = a->parts[r].oldPos;
								continue;
							}
						}
						Point e = Vec_sub2(a->parts[r].pos, a->parts[r].oldPos);
						Vec_fastNormalize(&e);
						for (int d=0; d<=10;d++) {
							a->parts[d].pos.x = w+e.x+Random_2(-0.1,0.1);
							a->parts[d].pos.y = b+e.y+Random_2(-0.1,0.1);
							a->parts[d].oldPos = (Point){w,b};
						}
						break;
					}
				}
			}
		} else if (a->type == Entity_PLAYER+2) {
			copyPos(10, 5);
			copyPos(9, 4);
			copyPos(8, 3);
			copyPos(7, 3);
			copyPos(6, 2);
			copyPos(5, 2);
			copyPos(4, 1);
			copyPos(3, 1);
			copyPos(2, 1);
			copyPos(1, 0);
			copyPos(0, 0);
			a->vel.y -= 1;
			a->age = 0;
			a->type = Entity_PLAYER+3;
		} else if (a->type == Entity_PLAYER+3) {
			a->age++;
			for (int b=0;b<11;b++) {
				moveNode(&a->parts[b], 0.1, 0.999);
				a->parts[b].pos.c += a->vel.c;
			}
			a->vel.c *= 0.5;
			real e = (150-a->age)/150;
			pullNodes(a, 1, 2, 4*e,0.5,0.5);
			pullNodes(a, 3, 5, 4*e,0.5,0.5);
			pullNodes(a, 4, 7, 4*e,0.5,0.5);
			pullNodes(a, 6, 9, 5*e,0.5,0.5);
			pullNodes(a, 8, 10, 5*e,0.5,0.5);
			for (int b=0;b<11;b++)
				updateNode(&a->parts[b], 0.1, false, false);
			if (a->age>150)
				Entity_remove(a--);
		} else if (a->type==Entity_FIGHTER||a->type==Entity_FIGHTER+1) {
			a->age++;
			if (a->type==Entity_FIGHTER) {
				moveNode(&a->parts[0],-0.2,0.995);
				moveNode(&a->parts[1],-0.1,0.995);
				moveNode(&a->parts[2],0,0.995);
				moveNode(&a->parts[3],0,0.995);
				moveNode(&a->parts[4],0.3,0.995);
				moveNode(&a->parts[5],0.3,0.995);
			} else {
				moveNode(&a->parts[0],0.1,0.995);
				moveNode(&a->parts[1],0.1,0.995);
				moveNode(&a->parts[2],0.1,0.995);
				moveNode(&a->parts[3],0.1,0.995);
				moveNode(&a->parts[4],0.1,0.995);
				moveNode(&a->parts[5],0.1,0.995);
			}
			checkDrag(a, 6);
			a->parts[0].pos.c += a->vel.c;
			a->vel.c *= 0.5;
			if (a->type==Entity_FIGHTER) {
				// check feets
				fighterWalk(a);
				fighterKick(a);
			} else if (a->age>10 && (a->parts[4].touching||a->parts[5].touching) && Rnd_perchance(10)) {
				a->type = Entity_FIGHTER;
				a->age = 0;
			}
			pullNodes(a, 0, 1, 4, 0.5,0.5);
			pullNodes(a, 1, 2, 4, 0.5,0.5);
			pullNodes(a, 1, 3, 4, 0.5,0.5);
			pullNodes(a, 2, 4, 5, 0.5,0.5);
			pullNodes(a, 3, 5, 5, 0.5,0.5);
			pullNodes(a, 2, 3, 5, 0.1,0.1);
			for (int i=0;i<6;i++)
				updateNode(&a->parts[i], 0.1, (i<4), a->held>0);
			int t = checkTouching(a,0,6);
			if (t==3 || t==-5)
				a->type = Entity_FIGHTER+2;
			if (a->type==Entity_FIGHTER && checkTouching(a,0,6))
				a->age = 0;
			else if (a->age>50)
				a->type = Entity_FIGHTER+1;
			if (Menu_edgeMode==1) {
				for (int r=4;r<=5;r++) {
					int w=0,b=0;
					if (a->parts[r].pos.x<8) {
						w = W+6;
						b = a->parts[r].oldPos.y;
					} else if (a->parts[r].pos.x>=WIDTH-8) {
						w = 10;
						b = a->parts[r].oldPos.y;
					}
					if (w+b!=0) {
						a->type = Entity_FIGHTER;
						a->age = 0;
						Part* part = Part_at[b][w];
						if (part > Part_BGFAN) {
							if (part < Part_0) {
								a->parts[r].pos = a->parts[r].oldPos;
								continue;
							} else if (ELEMENTS[part->type].state != State_LIQUID) {
								a->parts[r].pos = a->parts[r].oldPos;
								continue;
							}
						}
						Point e = Vec_sub2(a->parts[r].pos, a->parts[r].oldPos);
						Vec_fastNormalize(&e);
						for (int d=0; d<=10;d++) {
							a->parts[d].pos.x = w+e.x+Random_2(-0.1,0.1);
							a->parts[d].pos.y = b+e.y+Random_2(-0.1,0.1);
							a->parts[d].oldPos = (Point){w,b};
						}
						break;
					}
				}
			}
		} else if (a->type==Entity_FIGHTER+2) {
			copyPos(10, 5);
			copyPos(9, 4);
			copyPos(8, 3);
			copyPos(7, 3);
			copyPos(6, 2);
			copyPos(5, 2);
			copyPos(4, 1);
			copyPos(3, 1);
			copyPos(2, 1);
			copyPos(1, 0);
			a->vel.y -= 1;
			a->age = 0;
			a->type = Entity_FIGHTER+3;
		} else if (a->type==Entity_FIGHTER+3) {
			a->age++;
			for (int b=0; b<11; b++) {
				moveNode(&a->parts[b], 0.1, 0.999);
				a->parts[b].pos.c += a->vel.c;
			}
			a->vel.c *= 0.5;
			real e = (150-a->age)/150;
			pullNodes(a, 1, 2, 4*e,0.5,0.5);
			pullNodes(a, 3, 5, 4*e,0.5,0.5);
			pullNodes(a, 4, 7, 4*e,0.5,0.5);
			pullNodes(a, 6, 9, 5*e,0.5,0.5);
			pullNodes(a, 8, 10, 5*e,0.5,0.5);
			for (int b=0;b<11;b++)
				updateNode(&a->parts[b], 0.1, false, false);
			if (a->age>150)
				Entity_remove(a--);
		} else if (a->type==Entity_BOX) {
			a->age++;
			for (int b=0;b<4;b++)
				moveNode(&a->parts[b],0.1,1);
			checkDrag(a, 4);
			// kick
			for (int b=0;b<4;b++) {
				for (Entity* r=entitys; r<Entity_next; r++) {
					if (r->type==Entity_FIGHTER||r->type==Entity_FIGHTER+1||r->type==Entity_PLAYER) {
						for (int i=4;i<=5;i++) {
							EntityNode* part = &r->parts[i];
							real g = abs(part->pos.x - a->parts[b].pos.x);
							real q = abs(part->pos.y - a->parts[b].pos.y);
							if (g<=3 && q<=3) {
								a->parts[b].pos.x += 1*(part->pos.x - part->oldPos.x);
								a->parts[b].pos.y += 2*(part->pos.y - part->oldPos.y);
							}
						}
					}
				}
			}
			real r = 4*(a->meta+1);
			pullNodes(a,0,1,r,0.5,0.5);
			pullNodes(a,1,2,r,0.5,0.5);
			pullNodes(a,2,3,r,0.5,0.5);
			pullNodes(a,3,0,r,0.5,0.5);
			pullNodes(a,0,2,1.4142135*r,0.5,0.5);
			pullNodes(a,1,3,1.4142135*r,0.5,0.5);
			for (int b=0;b<4;b++)
				updateNode(&a->parts[b],0.5,false,true);
			int t = checkTouching(a,0,6);
			if (t==3 || t==-5)
				a->type = Entity_BOX+1;
		} else if (a->type == Entity_BOX+1) {
			copyPos(7, 0);
			copyPos(6, 3);
			copyPos(5, 3);
			copyPos(4, 2);
			copyPos(3, 2);
			copyPos(2, 1);
			a->held = 0;
			a->age = 0;
			a->type = checkTouching(a,0,4)==-5 ? Entity_BOX+3 : Entity_BOX+2;
		} else if (a->type==Entity_BOX+2 || a->type==Entity_BOX+3) {
			a->age++;
			checkDrag(a, 8);
			for (int b=0;b<8;b++)
				moveNode(&a->parts[b], 0.1, 0.999);
			real r=(150-a->age)/150*(a->meta+1)*4;
			pullNodes(a, 0, 1, r, 0.5, 0.5);
			pullNodes(a, 2, 3, r, 0.5, 0.5);
			pullNodes(a, 4, 5, r, 0.5, 0.5);
			pullNodes(a, 6, 7, r, 0.5, 0.5);
			if (a->type==Entity_BOX+2/*&&Parts_limits[Menu_dotLimit]-Parts_used>=1000*/) {
				for (int b=0;b<5;b+=2) {
					Point e = {.c=
						(a->parts[b+1].oldPos.c - a->parts[b].oldPos.c) * Random_(1) + a->parts[b].oldPos.c
					};
					if (Part_pos2(e)[0]<=Part_BGFAN)
						Part_create(e.x, e.y, Elem_FIRE);
				}
			}
			for (int b=0;b<8;b++)
				updateNode(&a->parts[b],0.1,false,false);
			if (a->age>150)
				Entity_remove(a--);
		}
	}
}
