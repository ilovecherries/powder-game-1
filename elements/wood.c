break; case Elem_WOOD:
{
#ifdef UPDATE_PART
	Vector airvel = p->vel;
	Vec_mul(&p->vel, 0.3);
	Part_blow(p, &airvel);
	//not burning
	if (p->meta==0) {
		int x = p->pos.x + Random_int(5)-2;
		int y = p->pos.y + Random_int(7)-3;
		Part* near = Part_at[y][x];
		if (near >= Part_0) {
			switch (near->type) {
			when(Elem_FIRE): case Elem_MAGMA:
				if (Rnd_perchance(50))
					p->meta = 1;
				//wood creates seed when near water
			when(Elem_WATER):
				//if(Parts_limits[Menu_dotLimit]-Parts_used<1000) return 0; TODO IMPORTANT
				x = p->pos.x + Random_int(3)-1;
				y = p->pos.y + Random_int(3)-1;
				if (Part_at[y][x] <= Part_BGFAN && Rnd_perchance(10))
					Part_create(x, y, Elem_SEED);
			}
		}
		//burning
	} else if (p->meta==1) {
		int x = p->pos.x + Random_int(3)-1;
		int y = p->pos.y + Random_int(3)-1;
		Part* g = Part_at[y][x];
		//make fires
		if (g <= Part_BGFAN) {
			if (Rnd_perchance(30)) {
				g = Part_create(x, y, Elem_FIRE);
				if (g>=Part_0)
					g->meta = 1;
			}
			//water puts out the fire and breaks the wood into powder
		} else if (g->type==Elem_WATER) {
			p->meta = 0;
			p->type = Elem_POWDER;
		}
		//generate powder sometimes
		if (Random_(1000)<5)
			p->type = Elem_POWDER;
	}

#elif defined UPDATE_BALL
	if (touched==Elem_TORCH)
		Ball_break(ball, 0, Elem_TORCH, 0, 0, 0, 0);
	else if (touched==Elem_ACID)
		Ball_break(ball, 0, Elem_POWDER, 0, 0, 0, 0);
	else if (touched==Elem_THUNDER)
		Ball_break(ball, 0, Elem_POWDER, 0, 0, 0, 1);
	else if (touched>=0 && ELEMENTS[touched].state==State_HOT) {
		if (ball->meta==1) //if oiled
			Ball_break(ball, 0, Elem_WOOD, 1, 0, 0, 0);
		else
			Ball_break(ball, 0, Elem_FIRE, 1, 0, 0, 0);
	}

#elif defined UPDATE_BALL_PART
	if (part->type==Elem_SEED)
		part->meta = 1;
	else if (part->type==Elem_OIL)
		ball->meta = 1; //make burn longer
#endif
}
