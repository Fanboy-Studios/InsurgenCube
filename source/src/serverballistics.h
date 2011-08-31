// normal shots (ray through sphere and cylinder check)
static inline int hitplayer(const vec &from, float yaw, float pitch, const vec &to, const vec &target, const vec &head, vec *end = NULL){
	// intersect head
	float dist;
	if(!head.iszero() && intersectsphere(from, to, head, HEADSIZE, dist)){
		if(end) (*end = to).sub(from).mul(dist).add(from);
		return HIT_HEAD;
	}
	float y = yaw*RAD, p = (pitch/4+90)*RAD, c = cosf(p);
	vec bottom(target), top(sinf(y)*c, -cosf(y)*c, sinf(p));
	bottom.z -= PLAYERHEIGHT;
	top.mul(PLAYERHEIGHT/* + d->aboveeye*/).add(bottom); // space above shoulders removed
	// torso
	bottom.sub(top).mul(TORSOPART).add(top);
	if(intersectcylinder(from, to, bottom, top, PLAYERRADIUS, dist))
	{
		if(end) (*end = to).sub(from).mul(dist).add(from);
		return HIT_TORSO;
	}
	// restore to body
	bottom.sub(top).div(TORSOPART).add(top);
	// legs
	top.sub(bottom).mul(LEGPART).add(bottom);
	if(intersectcylinder(from, to, bottom, top, PLAYERRADIUS, dist)){
		if(end) (*end = to).sub(from).mul(dist).add(from);
		return HIT_LEG;
	}
	return HIT_NONE;
}

// apply spread
void applyspread(const vec &from, vec &to, int spread, float factor){
	if(spread <=1 ) return;
	#define RNDD (rnd(spread)-spread/2.f)*factor
	vec r(RNDD, RNDD, RNDD);
	#undef RNDD
	to.add(r);
}

bool checkcrit(float dist, float m, int base = 0, int min = 1, int max = 100){
	return m_real || !rnd(base + clamp<int>(ceil(dist) * m, min, max));
}

// easy to send shot damage messages
inline void sendhit(client &actor, int gun, const float *o){
	sendf(-1, 1, "ri3f3", N_PROJ, actor.clientnum, gun, o[0], o[1], o[2]);
}

inline vec generateHead(const vec &o, float yaw){ // approximate location for the heads
	return vec(.2f, -.25f, .25f).rotate_around_z(yaw * RAD).add(o);
}

// explosions
int explosion(client &owner, const vec &o2, int weap){
	int damagedealt = 0;
	vec o(o2);
	checkpos(o);
	sendhit(owner, weap, o.v);
	loopv(clients){
		client &target = *clients[i];
		if(target.type == ST_EMPTY || target.state.state != CS_ALIVE) continue;
		float dist = target.state.o.dist(o);
		if(dist >= guns[weap].endrange) continue;
		vec ray(target.state.o);
		ray.sub(o).normalize();
		if(sraycube(o, ray) < dist) continue;
		ushort dmg = effectiveDamage(weap, dist, true);
		int expflags = FRAG_GIB;
		if((weap == WEAP_BOW && !dist) ||
			(weap == WEAP_GRENADE && owner.clientnum != i && o.z >= target.state.o.z)) expflags |= FRAG_FLAG;
		if(checkcrit(dist, 1.5f)){
			expflags |= FRAG_CRITICAL;
			dmg *= 2;
		}
		damagedealt += dmg;
		serverdamage(&target, &owner, dmg, weap, expflags, o);
	}
	return damagedealt;
}

// hit checks
client *nearesthit(client &actor, const vec &from, const vec &to, int &hitzone, client *exclude, vec *end = NULL){
	client *result = NULL;
	float dist = 4e6f; // 1 million meters...
	clientstate &gs = actor.state;
	loopv(clients){
		client &t = *clients[i];
		clientstate &ts = t.state;
		// basic checks
		if(t.type == ST_EMPTY || ts.state != CS_ALIVE || &t == exclude) continue;
		const float d = gs.o.dist(from);
		if(d > dist) continue;
		vec head = generateHead(ts.o, ts.aim[0]);
		const int hz = hitplayer(from, gs.aim[0], gs.aim[1], to, ts.o, head, end);
		if(!hz) continue;
		result = &t;
		dist = d;
		hitzone = hz;
	}
	return result;
}

// hitscans
int shot(client &owner, const vec &from, vec &to, int weap, vec &surface, client *exclude, float dist = 0){
	int shotdamage = 0;
	const int mulset = (weap == WEAP_SNIPER || weap == WEAP_BOLT) ? MUL_SNIPER : MUL_NORMAL;
	int hitzone = HIT_NONE; vec end = to;
	// calculate the hit
	client *hit = nearesthit(owner, from, to, hitzone, exclude, &end);
	// damage check
	const float dist2 = end.dist(from);
	int damage = effectiveDamage(weap, dist+dist2);
	if(hit && damage){
		// damage multipliers
		switch(hitzone){
			case HIT_HEAD: damage *= muls[mulset].head; break;
			case HIT_TORSO: damage *= muls[mulset].torso; break;
			case HIT_LEG: default: damage *= muls[mulset].leg; break;
		}
		// gib check
		const bool gib = weap == WEAP_KNIFE || hitzone == HIT_HEAD;
		int style = gib ? FRAG_GIB : FRAG_NONE;
		// critical shots
		if(checkcrit(dist, 2.5)){
			style |= FRAG_CRITICAL;
			damage *= 2.5f;
		}
		if(weap == WEAP_KNIFE && weap == WEAP_SWORD){
			if(hitzone == HIT_HEAD) style |= FRAG_FLAG;
			if(!isteam((&owner), hit)){
				hit->state.lastbleed = gamemillis;
				hit->state.lastbleedowner = owner.clientnum;
				sendf(-1, 1, "ri2", N_BLEED, hit->clientnum);
			}
		}
		else sendhit(owner, weap, end.v);
		serverdamage(hit, &owner, hit==&owner?1: damage, weap, style, from);
		// distort ray and continue through...
		vec dir(to = end), newsurface;
		dir.sub(from).normalize().rotate_around_z((rnd(71)-35)*RAD).add(end); // 35 degrees (both ways = 70 degrees) distortion
		// retrace
		straceShot(end, dir, &newsurface);
		shotdamage += shot(owner, end, dir, weap, newsurface, hit, dist + 40); // 10 meters penalty for penetrating the player
		sendf(-1, 1, "ri3f6", N_RICOCHET, owner.clientnum, weap, end.x, end.y, end.z, dir.x, dir.y, dir.z);
	}
	else if(!dist && from.dist(to) < 100 && surface.magnitude() && !melee_weap(weap)){ // ricochet before 25 meters or going through a player
		vec dir(to), newsurface;
		// calculate reflected ray from incident ray and surface normal
		dir.sub(from).normalize();
		// r = i - 2 n (i . n)
		const float dot = dir.dot(surface);
		loopi(3) dir[i] = dir[i] - (2 * surface[i] * dot);
		dir.add(to);
		// retrace
		straceShot(to, dir, &newsurface);
		shotdamage += shot(owner, to, dir, weap, newsurface, NULL, dist + 60); // 15 meters penalty for ricochet
		sendf(-1, 1, "ri3f6", N_RICOCHET, owner.clientnum, weap, to.x, to.y, to.z, dir.x, dir.y, dir.z);
	}
	return shotdamage;
}

int shotgun(client &owner, const vec &from, const vec &to){
	int damagedealt = 0;
	clientstate &gs = owner.state;
	loopv(clients){ // many rays many hits, but we want each client to get all the damage at once...
		client &t = *clients[i];
		clientstate &ts = t.state;
		// basic checks
		if(i == owner.clientnum || t.type == ST_EMPTY || ts.state != CS_ALIVE) continue;

		int damage = 0;
		loopj(SGRAYS){ // check rays and sum damage
			vec head = generateHead(ts.o, ts.aim[0]), end;
			const int hitzone = hitplayer(from, gs.aim[0], gs.aim[1], gs.sg[j], ts.o, head, &end);
			if(!hitzone) continue;
			damage += effectiveDamage(WEAP_SHOTGUN, end.dist(gs.o)) * muls[MUL_SHOTGUN].val[hitzone == HIT_HEAD ? 0 : hitzone == HIT_TORSO ? 1 : 2];
		}
		damagedealt += damage;
		sendhit(owner, WEAP_SHOTGUN, ts.o.v);
		serverdamage(&t, &owner, damage, WEAP_SHOTGUN, damage >= SGGIB ? FRAG_GIB : FRAG_NONE, from);
	}
	return damagedealt;
}
