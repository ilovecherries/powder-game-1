#include <string.h>
#include <math.h> //todo: define macros for abs/fabs etc.
#include "common.h"
#include "menu.h"
#include "draw.h"
#include "elements.h"
#include "part.h"
#include "cell.h"
#include "bg.h"

extern Color grp[HEIGHT][WIDTH];
Color* grp0 = &grp[0][0];

BgPixel Bg_pixels[HEIGHT][WIDTH] = {0};
BgPixel* const Bg_pixels0 = Bg_pixels[0];
BgPixel* const Bg_pixels_end = &Bg_pixels[HEIGHT-1][WIDTH-1]+1;

void Bg_reset(void) {
	for (BgPixel* px=Bg_pixels[0]; px<Bg_pixels_end; px++)
		*px = (BgPixel){0};
}

// render background (this is the first render function to run)
void Bg_render(void) {
	axis x,y;
	Offset i;
	switch (Menu_bgMode) {
	case Bg_NONE:
	default:
		for (Offset i=0;i<WIDTH*HEIGHT;i++) {
			grp0[i] = Part_grid0[i]==Part_BLOCK ? 0x606060 : 0;
		}
		break;
	case Bg_AIR: case Bg_LINE:;
		for (y=2; y<(H+8)/4; y++) {
			for (x=2; x<(W+8)/4; x++) {
				Block* block = &Part_blocks[y][x];
				if (block->block == 1) {
					Draw_rectangle(x*4, y*4, 4, 4, 0x606060);
				} else {
					int q=0, g=0;
					if (block->pres>0) {
						g = block->pres*48;
						if (g>96) g=96;
					}
					if (block->pres<0) {
						q = -block->pres*48;
						if (q>96) q=96;
					}
					Draw_rectangle(x*4, y*4, 4, 4, RGB(0,g,q));
				}
			}
		}
		if (Menu_bgMode==Bg_LINE) {
			for (Block* c=Part_blocks[0]; c<Part_blocks_end; c++) {
				if (c->block==0) {
					Point e = c->vel;
					real r = Vec_fastNormalize(&e);
					if (r>=0.2) {
						if (r>8) r=8;
						int f = 48*r;
						if (f>96) f=96;
						int d = c-Part_blocks[0];
						int n = d/(WIDTH/4);
						d = d%(WIDTH/4);
						n*=4;
						d*=4;
						Draw_line(d+e.x*r*10, n+e.y*r*10, d, n, f<<16);
					}
				}
			}
		}
		break;
	case Bg_BLUR:
		for (i=(HEIGHT-8)*WIDTH; i>=8*400+128; i--) { //why 128
			if (Part_grid0[i]==Part_BLOCK)
				grp0[i] = 0x606060;
			else {
				int r = 230*RED(grp0[i])/256;
				int g = 230*GREEN(grp0[i])/256;
				int b = 230*BLUE(grp0[i])/256;
				grp0[i] = RGB(r,g,b);
			}
		}
		break;
	case Bg_SHADE:
		for (y=8;y<H+8;y++) {
			Color* row = grp[y];
			for (x=8;x<W+8-1;x++) {
				int r = (RED(row[x])+RED(row[x+1]))>>1;
				int g = (GREEN(row[x])+GREEN(row[x+1]))>>1;
				int b = (BLUE(row[x])+BLUE(row[x+1]))>>1;
				row[x] = RGB(r,g,b);
			}
			for (x=W+8-1;x>=8-1;x--) {
				int r = (RED(row[x])+RED(row[x-1]))>>1;
				int g = (GREEN(row[x])+GREEN(row[x-1]))>>1;
				int b = (BLUE(row[x])+BLUE(row[x-1]))>>1;
				row[x] = RGB(r,g,b);
			}
		}
		for (x=8;x<W+8;x++) {
			for (y=8;y<H+8-1;y++) {
				int r = (RED(grp[y][x])+RED(grp[y+1][x]))>>1;
				int g = (GREEN(grp[y][x])+GREEN(grp[y+1][x]))>>1;
				int b = (BLUE(grp[y][x])+BLUE(grp[y+1][x]))>>1;
				grp[y][x] = RGB(r,g,b);
			}
			for (y=H+8-1;y>=8-1;y--) {
				int r = (RED(grp[y][x])+RED(grp[y-1][x]))>>1;
				int g = (GREEN(grp[y][x])+GREEN(grp[y-1][x]))>>1;
				int b = (BLUE(grp[y][x])+BLUE(grp[y-1][x]))>>1;
				grp[y][x] = RGB(r,g,b);
			}
		}
		for (y=0;y<H+8;y++) {
			for (x=0;x<WIDTH;x++)
				if (Part_at[y][x] == Part_BLOCK)
					grp[y][x] = 0x606060;
		}
		break;
	case Bg_AURA:;
		memset(Bg_pixels, 0, sizeof(Bg_pixels));
		for (y=2;y<(HEIGHT/4)-2;y++) {
			for (x=2;x<(WIDTH/4)-2;x++) {
				Cell* e = &Part_blocks[y][x];
				real vx = fabs(e->vel.x);
				real vy = fabs(e->vel.y);
				if (vx!=0 || vy!=0) {
					real q = 1/(vx+vy);
					int gg = vx*q*0xFFFF;
					int qq = vy*q*0xFFFF;
					axis sx = sign(e->vel.x);
					axis sy = sign(e->vel.y);
					Offset r = (y*4*WIDTH)+(x*4);
					for (int a=0; a<16; a++) {
						int w = RED(grp0[r]);
						Bg_pixels0[r+sx].auraR += w*gg;
						Bg_pixels0[r+sy*WIDTH].auraR += w*qq;
						w = GREEN(grp0[r]);
						Bg_pixels0[r+sx].auraG += w*gg;
						Bg_pixels0[r+sy*WIDTH].auraG += w*qq;
						w = BLUE(grp0[r]);
						Bg_pixels0[r+sx].auraB += w*gg;
						Bg_pixels0[r+sy*WIDTH].auraB += w*qq;
						r += (Offset[]){1,1,1,WIDTH-3,1,1,1,WIDTH-3,1,1,1,WIDTH-3,1,1,1,WIDTH-3}[a];
					}
				}
			}
		}
		inline int ff(int x) {
			if (x>255) return 255;
			return x;
		}
		for (y=8;y<H+8;y++) {
			for (x=8;x<W+8;x++) {
				BgPixel* p = &Bg_pixels[y][x];
				grp[y][x] = Part_at[y][x]==Part_BLOCK ? 0x606060 : RGB(
					ff(p->auraR>>16),
					ff(p->auraG>>16),
					ff(p->auraB>>16)
				);
			}
		}
		
		break;
	case Bg_LIGHT:;
		for (i=(H+8)*WIDTH; i>=WIDTH*8; i--) {
			if (Part_grid0[i]==Part_BLOCK)
				grp0[i] = 0x606060;
			else {
				int r = 220*RED(grp0[i])/256;
				int g = 220*GREEN(grp0[i])/256;
				int b = 220*BLUE(grp0[i])/256;
				grp0[i] = RGB(r,g,b);
			}
		}
		break;
	case Bg_TOON:
		for (i=(H+8)*WIDTH; i>=WIDTH*7; i--) {
			Part* p = Part_grid0[i];
			if (p==Part_BLOCK)
				grp0[i] = 0x606060;
			else if (p==Part_BALL)
				grp0[i] = 0;
			else if (p==Part_BGFAN)
				grp0[i] = 0x8080FF;
			else if (p==Part_EMPTY) {
				grp0[i] = Part_grid0[i+1]>=Part_0 ?
					ELEMENTS[Part_grid0[i+1]->type].color :
					Part_grid0[i-1]>=Part_0 ?
					ELEMENTS[Part_grid0[i-1]->type].color :
					Part_at[1][i]>=Part_0 ?
					ELEMENTS[Part_at[1][i]->type].color :
					Part_at[-1][i]>=Part_0 ?
					ELEMENTS[Part_at[-1][i]->type].color :
					Part_at[1][i+1]>=Part_0 ?
					ELEMENTS[Part_at[1][i+1]->type].color :
					Part_at[1][i-1]>=Part_0 ?
					ELEMENTS[Part_at[1][i-1]->type].color :
					Part_at[-1][i+1]>=Part_0 ?
					ELEMENTS[Part_at[-1][i+1]->type].color :
					Part_at[-1][i-1]>=Part_0 ?
					ELEMENTS[Part_at[-1][i-1]->type].color :
					0x000000;
			} else if (p>=Part_0) {
				grp0[i] = ELEMENTS[p->type].color;
			}
		}
		for (Offset a=(H+8)*WIDTH;a>=WIDTH*8;a--) {
			if (grp0[a] == 0) {
				if (grp0[a+1] && grp0[a+1]!=0xEEEEEE)
					grp0[a] = 0xEEEEEE;
				else if (grp0[a-1] && grp0[a-1]!=0xEEEEEE)
					grp0[a] = 0xEEEEEE;
				if (grp[1][a] && grp[1][a]!=0xEEEEEE)
					grp0[a] = 0xEEEEEE;
				if (grp[-1][a] && grp[-1][a]!=0xEEEEEE)
					grp0[a] = 0xEEEEEE;
			}
		}
		break;
	case Bg_MESH:;
		for (i=(HEIGHT-8)*WIDTH; i>=0; i--)
			grp0[i] = Part_grid0[i]==Part_BLOCK ? 0x606060 : 0;
		for (y=2; y<(H+8)/4; y++) {
			for (x=2; x<(W+8)/4; x++) {
				Block* cell = &Part_blocks[y][x];
				if (cell->block!=0) continue;
				real vel = Vec_fastDist(cell->vel);
				if (vel<0.2) continue;
				if (vel>2) vel=2;
				int g = atMost(vel*48, 96);
				int r = 0;
				if (cell->pres>0)
					r = atMost(cell->pres*48*r, 96);
				int b = 0;
				if (cell->pres<0)
					b = atMost(-cell->pres*48*r, 96);
				int d = x*4 + 5*cell->vel.x;
				int n = y*4 + 5*cell->vel.y;
				cell = &Part_blocks[y][x+1];
				int w = (x+1)*4 + 5*cell->vel.x;
				int h = (y)*4 + 5*cell->vel.y;
				Draw_line(d,n,w,h,RGB(r,g,b));
				cell = &Part_blocks[y+1][x];
				w = (x)*4 + 5*cell->vel.x;
				h = (y+1)*4 + 5*cell->vel.y;
				Draw_line(d,n,w,h,RGB(r,g,b));
			}
		}
		break;
	case Bg_DARK:
		for (Offset a=(HEIGHT-8)*WIDTH; a>=0; a--) {
			BgPixel* px = &Bg_pixels0[a];
			Part* p = Part_grid0[a];
			if (p<Part_0)
				px->light = 8*px->light/9;
			else if (p->type==Elem_FIRE||p->type==Elem_MAGMA||p->type==Elem_LASER||p->type==Elem_SPARK)
				px->light = 25500;
			else if (p->type==Elem_TORCH||p->type==Elem_THUNDER)
				px->light = 255000;
			else
				px->light = 8*px->light/9;
		}
		for (Offset a=(HEIGHT-8)*WIDTH; a>=0; a--) {
			grp0[a] = Part_grid0[a]==Part_BLOCK ? 0x606060 : 0;
		}
		break;
	case Bg_SILUET:
		for (Offset a=(HEIGHT-8)*WIDTH; a>=WIDTH*8+128; a--) { //128??
			if (Part_grid0[a]==Part_BLOCK)
				grp0[a] = 0;
			else if (Part_grid0[a]==Part_EMPTY) {
				int r = 0xFF- (0xFF-RED(grp0[a]))/2;
				int g = 0xFF- (0xFF-GREEN(grp0[a]))/2;
				int b = 0xFF- (0xFF-BLUE(grp0[a]))/2;
				grp0[a] = RGB(r,g,b);
			}
		}
	}
}

// this runs after everything is drawn
void Bg_render2(void) {
	if (Menu_bgMode!=Bg_DARK) return;
	for (axis y=8; y<H+8; y++) {
		for (axis x=8; x<W+7; x++) // yes 7
			Bg_pixels[y][x].light = (Bg_pixels[y][x].light+Bg_pixels[y][x+1].light)/2;
		for (axis x=W+7; x>8; x--) // yes,
			Bg_pixels[y][x].light = (Bg_pixels[y][x].light+Bg_pixels[y][x-1].light)/2;
		
	}
	for (axis x=8; x<W+8; x++) {
		for (axis y=8; y<H+7; y++)
			Bg_pixels[y][x].light = (Bg_pixels[y][x].light+Bg_pixels[y+1][x].light)/2;
		for (axis y=H+7; y>8; y--)
			Bg_pixels[y][x].light = (Bg_pixels[y][x].light+Bg_pixels[y-1][x].light)/2;
	}
	for (Offset a=(HEIGHT-8)*WIDTH; a>=0; a--) {
		int l = Bg_pixels0[a].light;
		if (l<1)
			grp0[a] = 0;
		else {
			if (l>255) l=255;
			int r = ((RED(grp0[a]))*l)/256;
			int g = ((GREEN(grp0[a]))*l)/256;
			int b = ((BLUE(grp0[a]))*l)/256;
			grp0[a] = RGB(r,g,b);
		}
	}
}
