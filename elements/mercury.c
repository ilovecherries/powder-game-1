break; case Elem_MERCURY:
{
#ifdef UPDATE_PART
	Part_liquidUpdate(p, c, 0.2, 0.1,0.2, 0.01, 0.01,0.05, 0.9);
	int dir = Random_int(8);
	if (dir>3) dir=3;
	
	Part* g = Part_dirNear(p->pos, dir);
	if (g>=Part_0) {
		//solids (except stone),nitro,soapy, and saltwater, diffuse through water
		int type = g->type;
		if (dir>0 && (ELEMENTS[type].state==State_POWDER||(ELEMENTS[type].state==State_LIQUID&&type!=Elem_MERCURY))) {
			Part_swap(p, g);
			//freeze water
		} else if (Part_checkPump(p, g, dir))
			Part_KILL();
	}
#endif
}
