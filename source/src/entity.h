enum							// static entity types
{
	NOTUSED = 0,				// entity slot not in use in map
	LIGHT,					  // lightsource, attr1 = radius, attr2 = intensity
	PLAYERSTART,				// attr1 = angle, attr2 = team
	I_CLIPS, I_AMMO, I_GRENADE,
	I_HEALTH, I_ARMOUR, I_AKIMBO,
	MAPMODEL,				   // attr1 = angle, attr2 = idx
	CARROT,					 // attr1 = tag, attr2 = type
	LADDER,
	CTF_FLAG,				   // attr1 = angle, attr2 = red/blue
	SOUND,
	CLIP,
	MAXENTTYPES
};

#define isitem(i) ((i) >= I_CLIPS && (i) <= I_AKIMBO)

struct persistent_entity		// map entity
{
	short x, y, z;			  // cube aligned position
	short attr1;
	uchar type;				 // type is one of the above
	uchar attr2, attr3, attr4;
	persistent_entity(short x, short y, short z, uchar type, short attr1, uchar attr2, uchar attr3, uchar attr4) : x(x), y(y), z(z), attr1(attr1), type(type), attr2(attr2), attr3(attr3), attr4(attr4) {}
	persistent_entity() {}
};

struct entity : public persistent_entity
{
	bool spawned;			   //the only dynamic state of a map entity
	entity(short x, short y, short z, uchar type, short attr1, uchar attr2, uchar attr3, uchar attr4) : persistent_entity(x, y, z, type, attr1, attr2, attr3, attr4), spawned(false) {}
	entity() {}
	bool fitsmode(int gamemode) { return !m_noitems && isitem(type) && !(m_noitemsnade && type!=I_GRENADE) && !(m_pistol && type==I_AMMO); }
	void transformtype(int gamemode)
	{
		if(m_noitemsnade && type==I_CLIPS) type = I_GRENADE;
		else if(m_pistol && type==I_AMMO) type = I_CLIPS;
	}
};

struct itemstat { int add, start, max, sound; };
static itemstat ammostats[] =
{
	{1,  1,   1,	 S_ITEMAMMO},   //knife dummy
	{24, 36,  72,	S_ITEMAMMO},   //pistol
	{20, 30,  60,	S_ITEMAMMO},   //shotgun
	{64, 96,  192,   S_ITEMAMMO},   //subgun
	{20, 30,  30,	S_ITEMAMMO},   //sniper
	{60, 90,  180,	S_ITEMAMMO},   //assault
	{2,  0,   3,	 S_ITEMAMMO},   //grenade
	{72, 0,   108,   S_ITEMAKIMBO}  //akimbo
};

static itemstat powerupstats[] =
{
	{35, 100, 120, S_ITEMHEALTH}, //health
	{40, 100, 100, S_ITEMARMOUR}, //armour
};

#define SGRAYS 32
#define SGSPREAD 4
#define SGGIB 130

struct guninfo { string modelname; short sound, reload, reloadtime, attackdelay, damage, range, endrange, rangeminus, projspeed, part, spread, kick, magsize, mdl_kick_rot, mdl_kick_back, recoil, maxrecoil, pushfactor; bool isauto; };
static guninfo guns[NUMGUNS] =
{
//	{ modelname;  snd,	  rldsnd,  rldtime, atkdelay,  dmg, rngstart, rngend, rngm,psd,ptt,spr,kick,magsz,mkrot,mkback,rcoil,maxrcl,pushf; auto;}
	{ "knife",    S_KNIFE,    S_NULL,     0,      560,    150,  3,    4,   150,   0,   0,  1,    1,   1,    0,  0,   0,    0,      5,   true },
	{ "pistol",   S_PISTOL,   S_RPISTOL,  1400,   90,     40,   40,  120,   20,   0,   0, 90,    9,   12,   6,  2,   50,   85,     1,   false},
	{ "shotgun",  S_SHOTGUN,  S_RSHOTGUN, 2400,   181,    12,   16,   32,    8,   0,   0,  1,   12,   10,   9,  5,   80,   80,     2,   true },
	{ "subgun",   S_SUBGUN,   S_RSUBGUN,  1858,   67,     40,   40,   90,   15,   0,   0, 70,    4,   32,   1,  3,   24,   60,     1,   true },
	{ "sniper",   S_SNIPER,   S_RSNIPER,  1950,   500,    140,  10,  100,   10,   0,   0,128,   18,   10,   4,  4,   70,   70,     1,   false},
	{ "assault",  S_ASSAULT,  S_RASSAULT, 2000,   73,     40,   50,   150,  14,   0,   0, 60,    3,   30,   0,  3,   25,   63,     1,   true },
	{ "grenade",  S_NULL,     S_NULL,     1000,   650,    200,  0,    8,    200,  20,  6,  1,    1,   1,    3,  1,   0,    0,      4,   false},
	{ "pistol",   S_PISTOL,   S_RAKIMBO,  1400,   60,     36,   160,  210,  16,   0,   0, 90,    9,   16,   6,  2,   35,   60,     3,   true },
}; 

static inline int reloadtime(int gun) { return guns[gun].reloadtime; }
static inline int attackdelay(int gun) { return guns[gun].attackdelay; }
static inline int magsize(int gun) { return guns[gun].magsize; }
static inline int effectiveDamage(int gun, float dist) {
	if(dist <= guns[gun].range || (!guns[gun].range && !guns[gun].endrange)) return guns[gun].damage;
	if(dist >= guns[gun].endrange) return guns[gun].damage - guns[gun].rangeminus;
	else return guns[gun].damage - (short)((dist - (float)guns[gun].range) * guns[gun].rangeminus / (guns[gun].endrange - guns[gun].range));
}

#define isteam(a,b)   (m_teammode && a->team == b->team)

enum { TEAM_RED = 0, TEAM_BLUE, TEAM_NUM };
#define team_valid(t) (!strcmp(t, "BLUE") || !strcmp(t, "RED"))
#define team_string(t) ((t) ? "BLUE" : "RED")
#define team_int(t) (strcmp((t), "RED") == 0 ? TEAM_RED : TEAM_BLUE)
#define team_opposite(o) ((o) == TEAM_RED ? TEAM_BLUE : TEAM_RED)

enum { ENT_PLAYER = 0, ENT_BOT, ENT_CAMERA, ENT_BOUNCE };
enum { CS_ALIVE = 0, CS_DEAD, CS_SPAWNING, CS_LAGGED, CS_EDITING, CS_SPECTATE };
enum { CR_DEFAULT = 0, CR_ADMIN }; // TODO: update into new system
enum { PRIV_NONE = 0, PRIV_MASTER, PRIV_ADMIN, PRIV_OWNER };
enum { SM_NONE = 0, SM_DEATHCAM, SM_FOLLOW1ST, SM_FOLLOW3RD, SM_FOLLOW3RD_TRANSPARENT, SM_FLY, SM_NUM };

struct physent
{
	vec o, vel;						 // origin, velocity
	vec deltapos, newpos;					   // movement interpolation
	float yaw, pitch, roll;			 // used as vec in one place
	float pitchvel, pitchreturn;
	float maxspeed;					 // cubes per second, 24 for player
	int timeinair;					  // used for fake gravity
	float radius, eyeheight, maxeyeheight, aboveeye;  // bounding box size
	bool inwater;
	bool onfloor, onladder, jumpnext, crouching, trycrouch, cancollide, stuck;
	int lastsplash;
	char move, strafe;
	uchar state, type;
	float eyeheightvel;

	physent() : o(0, 0, 0), deltapos(0, 0, 0), newpos(0, 0, 0), yaw(270), pitch(0), pitchreturn(0), roll(0), pitchvel(0),
				crouching(false), trycrouch(false), cancollide(true), stuck(false), lastsplash(0), state(CS_ALIVE)
	{
		reset();
	}
	virtual ~physent() {}

	void resetinterp()
	{
		newpos = o;
		newpos.z -= eyeheight;
		deltapos = vec(0, 0, 0);
	}

	void reset()
	{
		vel.x = vel.y = vel.z = eyeheightvel = 0.0f;
		move = strafe = 0;
		timeinair = lastsplash = 0;
		onfloor = onladder = inwater = jumpnext = crouching = trycrouch = stuck = false;
	}

	virtual void oncollision() {}
	virtual void onmoved(const vec &dist) {}
};

struct dynent : physent				 // animated ent
{
	bool k_left, k_right, k_up, k_down;		 // see input code

	animstate prev[2], current[2];			  // md2's need only [0], md3's need both for the lower&upper model
	int lastanimswitchtime[2];
	void *lastmodel[2];
	int lastrendered;

	void stopmoving()
	{
		k_left = k_right = k_up = k_down = jumpnext = false;
		move = strafe = 0;
	}

	void resetanim()
	{
		loopi(2)
		{
			prev[i].reset();
			current[i].reset();
			lastanimswitchtime[i] = -1;
			lastmodel[i] = NULL;
		}
		lastrendered = 0;
	}

	void reset()
	{
		physent::reset();
		stopmoving();
	}

	dynent() { reset(); resetanim(); }
	virtual ~dynent() {}
};

#define MAXNAMELEN 15
#define MAXTEAMLEN 4

struct bounceent;

#define POSHIST_SIZE 7

struct poshist
{
	int nextupdate, curpos, numpos;
	vec pos[POSHIST_SIZE];

	poshist() : nextupdate(0) { reset(); }

	const int size() const { return numpos; }

	void reset()
	{
		curpos = 0;
		numpos = 0;
	}

	void addpos(const vec &o)
	{
		pos[curpos] = o;
		curpos++;
		if(curpos>=POSHIST_SIZE) curpos = 0;
		if(numpos<POSHIST_SIZE) numpos++;
	}

	const vec &getpos(int i) const
	{
		i = curpos-1-i;
		if(i < 0) i += POSHIST_SIZE;
		return pos[i];
	}

	void update(const vec &o, int lastmillis)
	{
		if(lastmillis<nextupdate) return;
		if(o.dist(pos[0]) >= 4.0f) addpos(o);
		nextupdate = lastmillis + 100;
	}
};

struct playerstate
{
	int health, armour;
	int primary, nextprimary;
	int gunselect;
	bool akimbo;
	int ammo[NUMGUNS], mag[NUMGUNS], gunwait[NUMGUNS];

	playerstate() : primary(GUN_ASSAULT), nextprimary(GUN_ASSAULT) {}
	virtual ~playerstate() {}

	itemstat &itemstats(int type)
	{
		switch(type)
		{
			case I_CLIPS: return ammostats[GUN_PISTOL];
			case I_AMMO: return ammostats[primary];
			case I_GRENADE: return ammostats[GUN_GRENADE];
			case I_AKIMBO: return ammostats[GUN_AKIMBO];
			case I_HEALTH: return powerupstats[0]; // FIXME: unify
			case I_ARMOUR: return powerupstats[1];
			default:
				return *(itemstat *)0;
		}
	}

	bool canpickup(int type)
	{
		switch(type)
		{
			case I_CLIPS: return ammo[akimbo ? GUN_AKIMBO : GUN_PISTOL]<ammostats[akimbo ? GUN_AKIMBO : GUN_PISTOL].max;
			case I_AMMO: return ammo[primary]<ammostats[primary].max;
			case I_GRENADE: return mag[GUN_GRENADE]<ammostats[GUN_GRENADE].max;
			case I_HEALTH: return health<powerupstats[type-I_HEALTH].max;
			case I_ARMOUR: return armour<powerupstats[type-I_HEALTH].max;
			case I_AKIMBO: return !akimbo;
			default: return false;
		}
	}

	void additem(itemstat &is, int &v)
	{
		v += is.add;
		if(v > is.max) v = is.max;
	}

	void pickup(int type)
	{
		switch(type)
		{
			case I_CLIPS:
				additem(ammostats[GUN_PISTOL], ammo[GUN_PISTOL]);
				additem(ammostats[GUN_AKIMBO], ammo[GUN_AKIMBO]);
				break;
			case I_AMMO: additem(ammostats[primary], ammo[primary]); break;
			case I_GRENADE: additem(ammostats[GUN_GRENADE], mag[GUN_GRENADE]); break;
			case I_HEALTH: additem(powerupstats[type-I_HEALTH], health); break;
			case I_ARMOUR: additem(powerupstats[type-I_HEALTH], armour); break;
			case I_AKIMBO:
				akimbo = true;
				mag[GUN_AKIMBO] = guns[GUN_AKIMBO].magsize;
				additem(ammostats[GUN_AKIMBO], ammo[GUN_AKIMBO]);
				break;
		}
	}

	void respawn()
	{
		health = 100;
		armour = 0;
		gunselect = GUN_PISTOL;
		akimbo = false;
		loopi(NUMGUNS) ammo[i] = mag[i] = gunwait[i] = 0;
		ammo[GUN_KNIFE] = mag[GUN_KNIFE] = 1;
	}

	virtual void spawnstate(int gamemode)
	{
		if(m_pistol) primary = GUN_PISTOL;
		else if(m_osok) primary = GUN_SNIPER;
		else if(m_lss) primary = GUN_KNIFE;
		else primary = nextprimary;

		if(!m_nopistol)
		{
			ammo[GUN_PISTOL] = ammostats[GUN_PISTOL].max-magsize(GUN_PISTOL);
			mag[GUN_PISTOL] = magsize(GUN_PISTOL);
		}

		if(!m_noprimary)
		{
			ammo[primary] = ammostats[primary].start-magsize(primary);
			mag[primary] = magsize(primary);
		}

		gunselect = primary;

		if(m_osok) health = 1;
	}

	// just subtract damage here, can set death, etc. later in code calling this
	int dodamage(int damage)
	{
		int ad = damage*30/100; // let armour absorb when possible
		if(ad>armour) ad = armour;
		armour -= ad;
		damage -= ad;
		health -= damage;
		return damage;
	}
};

#define HEADSIZE 0.4f

struct playerent : dynent, playerstate
{
	int clientnum, lastupdate, plag, ping;
	int lifesequence;				   // sequence id for each respawn, used in damage test
	int lastloud; float lastloudpos[3]; // position and yaw stored for last shot
	int frags, flagscore, deaths;
	int lastaction, lastmove, lastpain, lastvoicecom;
	int clientrole;
	bool attacking;
	string name;
	int weaponchanging;
	int nextweapon; // weapon we switch to
	int team, skin;
	int spectatemode, followplayercn;
	int eardamagemillis;
	int respawnoffset;
	bool allowmove() { return state!=CS_DEAD || spectatemode==SM_FLY; }

	weapon *weapons[NUMGUNS];
	weapon *prevweaponsel, *weaponsel, *nextweaponsel, *primweap, *nextprimweap, *lastattackweapon;

	poshist history; // Previous stored locations of this player

	const char *skin_noteam, *skin_cla, *skin_rvsf;

	float deltayaw, deltapitch, newyaw, newpitch;
	int smoothmillis;

	vec head;

	playerent() : clientnum(-1), lastupdate(0), plag(0), ping(0), lifesequence(0), frags(0), flagscore(0), deaths(0), lastpain(0), lastvoicecom(0), clientrole(CR_DEFAULT),
				  skin(0), spectatemode(SM_NONE), followplayercn(-1), eardamagemillis(0), respawnoffset(0), lastloud(0),
				  prevweaponsel(NULL), weaponsel(NULL), nextweaponsel(NULL), primweap(NULL), nextprimweap(NULL), lastattackweapon(NULL),
				  smoothmillis(-1),
				  head(-1, -1, -1)
	{
		type = ENT_PLAYER;
		name[0] = 0;
		team = TEAM_BLUE;
		maxeyeheight = 4.5f;
		aboveeye = 0.7f;
		radius = 1.1f;
		maxspeed = 16.0f;
		skin_noteam = skin_cla = skin_rvsf = NULL;
		respawn();
	}

	virtual ~playerent()
	{
		extern void removebounceents(playerent *owner);
		extern void detachsounds(playerent *owner);
		extern void removedynlights(physent *owner);
		extern void zapplayerflags(playerent *owner);
		extern void cleanplayervotes(playerent *owner);
		extern physent *camera1;
		extern void togglespect();
		removebounceents(this);
		detachsounds(this);
		removedynlights(this);
		zapplayerflags(this);
		cleanplayervotes(this);
		if(this==camera1) togglespect();
	}

	void damageroll(float damage)
	{
		extern void clamproll(physent *pl);
		float damroll = 2.0f*damage;
		roll += roll>0 ? damroll : (roll<0 ? -damroll : (rnd(2) ? damroll : -damroll)); // give player a kick
		clamproll(this);
	}

	void hitpush(int damage, const vec &dir, playerent *actor, int gun)
	{
		if(gun<0 || gun>NUMGUNS) return;
		vec push(dir);
		push.mul(damage/100.0f*guns[gun].pushfactor);
		vel.add(push);
		extern int lastmillis;
		if(gun==GUN_GRENADE && damage > 50) eardamagemillis = lastmillis+damage*100;
	}

	void resetspec()
	{
		spectatemode = SM_NONE;
		followplayercn = -1;
	}

	void respawn()
	{
		dynent::reset();
		playerstate::respawn();
		history.reset();
		if(weaponsel) weaponsel->reset();
		lastaction = 0;
		lastattackweapon = NULL;
		attacking = false;
		weaponchanging = 0;
		resetspec();
		eardamagemillis = 0;
		eyeheight = maxeyeheight;
	}

	void spawnstate(int gamemode)
	{
		playerstate::spawnstate(gamemode);
		prevweaponsel = weaponsel = weapons[gunselect];
		primweap = weapons[primary];
		nextprimweap = weapons[nextprimary];
	}

	void selectweapon(int w) { prevweaponsel = weaponsel = weapons[(gunselect = w)]; }
	void setprimary(int w) { primweap = weapons[(primary = w)]; }
	void setnextprimary(int w) { nextprimweap = weapons[(nextprimary = w)]; }
	bool isspectating() { return state==CS_SPECTATE || (state==CS_DEAD && spectatemode > SM_NONE); }
	void weaponswitch(weapon *w)
	{
		if(!w) return;
		extern int lastmillis;
		weaponsel->ondeselecting();
		weaponchanging = lastmillis;
		prevweaponsel = weaponsel;
		nextweaponsel = w;
		w->onselecting();
	}
};



class CBot;

struct botent : playerent
{
	// Added by Rick
	CBot *pBot; // Only used if this is a bot, points to the bot class if we are the host,
				// for other clients its NULL
	// End add by Rick

	playerent *enemy;					  // monster wants to kill this entity
	// Added by Rick: targetpitch
	float targetpitch;					// monster wants to look in this direction
	// End add
	float targetyaw;					// monster wants to look in this direction

	botent() : pBot(NULL), enemy(NULL) { type = ENT_BOT; }
	~botent() { }

	int deaths() { return lifesequence; }
};

enum { CTFF_INBASE = 0, CTFF_STOLEN, CTFF_DROPPED, CTFF_IDLE };

struct flaginfo
{
	int team;
	entity *flagent;
	int actor_cn;
	playerent *actor;
	vec pos;
	int state; // one of CTFF_*
	flaginfo() : flagent(0), actor(0), state(CTFF_INBASE) {}
};

enum { BT_NONE, BT_NADE, BT_GIB };

struct bounceent : physent // nades, gibs
{
	int millis, timetolive, bouncetype; // see enum above
	float rotspeed;
	playerent *owner;

	bounceent() : bouncetype(BT_NONE), rotspeed(1.0f), owner(NULL)
	{
		type = ENT_BOUNCE;
		maxspeed = 40;
		radius = 0.2f;
		eyeheight = maxeyeheight = 0.3f;
		aboveeye = 0.0f;
	}

	virtual ~bounceent() {}

	bool isalive(int lastmillis) { return lastmillis - millis < timetolive; }
	virtual void destroy() {}
	virtual bool applyphysics() { return true; }
};

struct hitmsg
{
	int target, lifesequence, info;
	float dir[3];
};

struct grenadeent : bounceent
{
	bool local;
	int nadestate;
	float distsincebounce;
	grenadeent(playerent *owner, int millis = 0);
	~grenadeent();
	void activate(const vec &from, const vec &to);
	void _throw(const vec &from, const vec &vel);
	void explode();
	void splash();
	virtual void destroy();
	virtual bool applyphysics();
	void moveoutsidebbox(const vec &direction, playerent *boundingbox);
	void oncollision();
	void onmoved(const vec &dist);
};

