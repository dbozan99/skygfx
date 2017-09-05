#include "skygfx.h"
#include "MemoryMgr.h"
#include "d3d9.h"
#include "d3d9types.h"
#include <time.h>
#include "neo.h"
#define _USE_MATH_DEFINES
#include <cmath>

#include <injector\injector.hpp>
#include <injector\hooking.hpp>
#include <injector\calling.hpp>
#include <injector\assembly.hpp>
#include <injector\utility.hpp>

enum eCamMode
{
	MODE_NONE = 0,
	MODE_TOPDOWN, //=1, 
	MODE_GTACLASSIC,// =2, 
	MODE_BEHINDCAR, //=3, 
	MODE_FOLLOWPED, //=4, 
	MODE_AIMING, //=5, 
	MODE_DEBUG, //=6, 
	MODE_SNIPER, //=7, 
	MODE_ROCKETLAUNCHER, //=8,   
	MODE_MODELVIEW, //=9, 
	MODE_BILL, //=10, 
	MODE_SYPHON, //=11, 
	MODE_CIRCLE, //=12, 
	MODE_CHEESYZOOM, //=13, 
	MODE_WHEELCAM, //=14, 
	MODE_FIXED, //=15, 
	MODE_1STPERSON, //=16, 
	MODE_FLYBY, //=17, 
	MODE_CAM_ON_A_STRING, //=18,  
	MODE_REACTION, //=19, 
	MODE_FOLLOW_PED_WITH_BIND, //=20, 
	MODE_CHRIS, //21
	MODE_BEHINDBOAT, //=22, 
	MODE_PLAYER_FALLEN_WATER, //=23, 
	MODE_CAM_ON_TRAIN_ROOF, //=24, 
	MODE_CAM_RUNNING_SIDE_TRAIN, //=25,  
	MODE_BLOOD_ON_THE_TRACKS, //=26, 
	MODE_IM_THE_PASSENGER_WOOWOO, //=27, 
	MODE_SYPHON_CRIM_IN_FRONT,// =28, 
	MODE_PED_DEAD_BABY, //=29, 
	MODE_PILLOWS_PAPS, //=30, 
	MODE_LOOK_AT_CARS, //=31, 
	MODE_ARRESTCAM_ONE, //=32, 
	MODE_ARRESTCAM_TWO, //=33, 
	MODE_M16_1STPERSON, //=34, 
	MODE_SPECIAL_FIXED_FOR_SYPHON, //=35, 
	MODE_FIGHT_CAM, //=36, 
	MODE_TOP_DOWN_PED, //=37,
	MODE_LIGHTHOUSE, //=38
	MODE_SNIPER_RUNABOUT, //=39, 
	MODE_ROCKETLAUNCHER_RUNABOUT, //=40,  
	MODE_1STPERSON_RUNABOUT, //=41,
	MODE_M16_1STPERSON_RUNABOUT, //=42,
	MODE_FIGHT_CAM_RUNABOUT, //=43,
	MODE_EDITOR, //=44,
	MODE_HELICANNON_1STPERSON, //= 45
	MODE_CAMERA, //46
	MODE_ATTACHCAM,  //47
	MODE_TWOPLAYER,
	MODE_TWOPLAYER_IN_CAR_AND_SHOOTING,
	MODE_TWOPLAYER_SEPARATE_CARS,	//50
	MODE_ROCKETLAUNCHER_HS,
	MODE_ROCKETLAUNCHER_RUNABOUT_HS,
	MODE_AIMWEAPON,
	MODE_TWOPLAYER_SEPARATE_CARS_TOPDOWN,
	MODE_AIMWEAPON_FROMCAR, //55
	MODE_DW_HELI_CHASE,
	MODE_DW_CAM_MAN,
	MODE_DW_BIRDY,
	MODE_DW_PLANE_SPOTTER,
	MODE_DW_DOG_FIGHT, //60
	MODE_DW_FISH,
	MODE_DW_PLANECAM1,
	MODE_DW_PLANECAM2,
	MODE_DW_PLANECAM3,
	MODE_AIMWEAPON_ATTACHED //65
};

typedef uintptr_t addr;

#define MAXSIZE 15
#define MINSIZE 4
#define DROPFVF (D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX2)
#define RAD2DEG(x) (180.0f*(x)/M_PI)

float scaling;
#define SC(x) ((int)((x)*scaling))

float WaterDrops::ms_xOff, WaterDrops::ms_yOff;	// not quite sure what these are
WaterDrop WaterDrops::ms_drops[MAXDROPS];
int WaterDrops::ms_numDrops;
WaterDropMoving WaterDrops::ms_dropsMoving[MAXDROPSMOVING];
int WaterDrops::ms_numDropsMoving;

bool WaterDrops::ms_enabled;
bool WaterDrops::ms_movingEnabled;

int WaterDrops::ms_splashDuration;
CPlaceable_III *WaterDrops::ms_splashObject;

float WaterDrops::ms_distMoved, WaterDrops::ms_vecLen, WaterDrops::ms_rainStrength;
RwV3d WaterDrops::ms_vec;
RwV3d WaterDrops::ms_lastAt;
RwV3d WaterDrops::ms_lastPos;
RwV3d WaterDrops::ms_posDelta;

RwTexture *WaterDrops::ms_maskTex;
RwTexture *WaterDrops::ms_tex;
RwRaster *WaterDrops::ms_raster;
int WaterDrops::ms_fbWidth, WaterDrops::ms_fbHeight;
void *WaterDrops::ms_vertexBuf;
void *WaterDrops::ms_indexBuf;
VertexTex2 *WaterDrops::ms_vertPtr;
int WaterDrops::ms_numBatchedDrops;
int WaterDrops::ms_initialised;

extern IDirect3DDevice9 *&d3d9device;

uchar *TheCamera = (uchar*)0xB6F028;
RwCamera *&rwcam = *(RwCamera**)(0xC1703C);
extern float &CTimer__ms_fTimeStep;
float &CWeather__Rain = *(float*)(0xC81324);
float &CWeather__UnderWaterness = *(float*)(0xC8132C);
bool &CCutsceneMgr__ms_running = *(bool*)(0xB5F851);
int* CGame__currArea = (int*)0xB72914;
int* CEntryExitManager__ms_exitEnterState = (int*)0x96A7CC;

short getCamMode()
{
	uchar activeCam = *(TheCamera + 0x59);
	uchar *cam;

	cam = TheCamera + 0x174;
	cam += activeCam * 0x238;

	return *(short*)(cam + 0xC);
}

static uint32_t CCullZones__CamNoRain_A = (uint32_t)(0x72DDB0);
WRAPPER bool CCullZones__CamNoRain(void) { VARJMP(CCullZones__CamNoRain_A); }
static uint32_t CCullZones__PlayerNoRain_A = (uint32_t)(0x72DDC0);
WRAPPER bool CCullZones__PlayerNoRain(void) { VARJMP(CCullZones__PlayerNoRain_A); }
static uint32_t FindPlayerVehicle_A = (uint32_t)(0x56E0D0);
WRAPPER bool FindPlayerVehicle(uint32_t, uint8_t) { VARJMP(FindPlayerVehicle_A); }
static uint32_t DefinedState_A = (addr)(0x734650);
WRAPPER void DefinedState(void) { VARJMP(DefinedState_A); }

struct CPad
{
	static CPad *GetPad(int);
	bool GetLookBehindForCar(void);
	bool GetLookLeft(void);
	bool GetLookRight(void);
};
static uint32_t GetPad_A = 0x53FB70;
WRAPPER CPad *CPad::GetPad(int id) { VARJMP(GetPad_A); }
static uint32_t GetLookBehindForCar_A = 0x53FE70;
WRAPPER bool CPad::GetLookBehindForCar(void) { VARJMP(GetLookBehindForCar_A); }
static uint32_t GetLookLeft_A = 0x53FDD0;
WRAPPER bool CPad::GetLookLeft(void) { VARJMP(GetLookLeft_A); }
static uint32_t GetLookRight_A = 0x53FE10;
WRAPPER bool CPad::GetLookRight(void) { VARJMP(GetLookRight_A); }

WRAPPER RwStream* RwStreamOpen(RwStreamType type, RwStreamAccessType accessType, const void* pData) { EAXJMP(0x7ECEF0); }
WRAPPER RwBool RwStreamFindChunk(RwStream* stream, RwUInt32 type, RwUInt32* lengthOut, RwUInt32* versionOut) { EAXJMP(0x7ED2D0); }
WRAPPER RwTexDictionary* RwTexDictionaryStreamRead(RwStream* stream) { EAXJMP(0x804C30); }
WRAPPER RwBool RwStreamClose(RwStream* stream, void* pData) { EAXJMP(0x7ECE20); }
WRAPPER RwTexDictionary* RwTexDictionarySetCurrent(RwTexDictionary* pDict) { EAXJMP(0x7F3A70); }
WRAPPER RwTexture* RwTextureRead(const char *name, const char *maskName) { EAXJMP(0x7F3AC0); }

typedef void(*voidfunc)(void);
#define INJECTRESET(x) \
	addr reset_call_##x; \
	void reset_hook_##x(void){ \
		((voidfunc)reset_call_##x)(); \
		WaterDrops::Reset(); \
	}

INJECTRESET(1)
INJECTRESET(2)
INJECTRESET(3)
INJECTRESET(4)
INJECTRESET(5)

void hookWaterDrops()
{
	static injector::hook_back<void(*)()> CMotionBlurStreaksRender;
	auto CMotionBlurStreaksRenderHook = []()
	{
		CMotionBlurStreaksRender.fun();
		if(neoTxd && config->neoWaterDrops){
			WaterDrops::Process();
			WaterDrops::Render();
		}
	}; CMotionBlurStreaksRender.fun = injector::MakeCALL(0x726AD0, static_cast<void(*)()>(CMotionBlurStreaksRenderHook), true).get();

	static injector::hook_back<void(__fastcall*)(void* _this, int edx, int id, RwV3d* point)> CAEFireAudioEntityAddAudioEvent;
	auto CAEFireAudioEntityAddAudioEventHook = [](void* _this, int edx, int id, RwV3d* point)
	{
		WaterDrops::RegisterSplash(point, 20.0f, 20);
		return CAEFireAudioEntityAddAudioEvent.fun(_this, edx, id, point);
	}; CAEFireAudioEntityAddAudioEvent.fun = injector::MakeCALL(0x4AAE2D, static_cast<void(__fastcall*)(void*, int, int, RwV3d*)>(CAEFireAudioEntityAddAudioEventHook), true).get(); //water_hydrant
	injector::MakeCALL(0x4AAE4B, static_cast<void(__fastcall*)(void*, int, int, RwV3d*)>(CAEFireAudioEntityAddAudioEventHook), true); //water_fountain
	injector::MakeCALL(0x4AAE69, static_cast<void(__fastcall*)(void*, int, int, RwV3d*)>(CAEFireAudioEntityAddAudioEventHook), true); //water_fnt_tme

	//water_splash_big
	static injector::hook_back<void*(__fastcall*)(void* _this, int edx, char* name, RwV3d* point, RwMatrix* m, char flag)> CParticleDataCreateParticle;
	auto CParticleDataCreateParticleHook = [](void* _this, int edx, char* name, RwV3d* point, RwMatrix* m, char flag) -> void*
	{
		WaterDrops::RegisterSplash(point, 10.0f, 1);

		return CParticleDataCreateParticle.fun(_this, edx, name, point, m, flag);
	}; CParticleDataCreateParticle.fun = injector::MakeCALL(0x4A10C9, static_cast<void*(__fastcall*)(void*, int, char*, RwV3d*, RwMatrix*, char)>(CParticleDataCreateParticleHook), true).get();

	//car splash
	struct splashhook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.eax = regs.esp + 0x90;
			WaterDrops::FillScreenMoving(1.0f, false);
		}
	}; injector::MakeInline<splashhook>(0x6C38EA, 0x6C38EA + 7);

	//blood
	struct bloodhook
	{
		void operator()(injector::reg_pack& regs)
		{
			regs.ecx = 0x2710;
	
			RwV3d dist;
			RwV3dSub(&dist, (RwV3d*)regs.edi, &WaterDrops::ms_lastPos);
	
			if (RwV3dLength(&dist) <= 5.0f)
				WaterDrops::FillScreenMoving(0.5f, true);
		}
	}; injector::MakeInline<bloodhook>(0x49EC4D);

	//chainsaw
	static injector::hook_back<void(__fastcall*)(void* _this, int edx, int eventID)> CAEPedWeaponAudioEntityAddAudioEvent;
	auto CAEPedWeaponAudioEntityAddAudioEventHook = [](void* _this, int edx, int eventID)
	{
		WaterDrops::FillScreenMoving(1.0f, true);
		return CAEPedWeaponAudioEntityAddAudioEvent.fun(_this, edx, eventID);
	}; CAEPedWeaponAudioEntityAddAudioEvent.fun = injector::MakeCALL(0x61CD73, static_cast<void(__fastcall*)(void*, int, int)>(CAEPedWeaponAudioEntityAddAudioEventHook), true).get();
}

void WaterDrops::RegisterSplash(RwV3d* point, float distance, int duration)
{
	RwV3d dist;
	RwV3dSub(&dist, point, &ms_lastPos);

	if (RwV3dLength(&dist) <= distance)
		ms_splashDuration = duration;
}

void WaterDrops::RegisterSplash(CPlaceable_III *plc, float distance)
{
	RwV3d dist;
	RwV3dSub(&dist, &plc->matrix.matrix.pos, &ms_lastPos);
	//RwV3dSub(&dist, &((CPlaceable*)plc)->matrix.matrix.pos, &ms_lastPos);

	if (RwV3dLength(&dist) <= distance)
	{
		ms_splashDuration = 14;
		ms_splashObject = plc;
	}
}

void WaterDrops::InitialiseRender(RwCamera *cam)
{
	srand(time(NULL));
	ms_fbWidth = cam->frameBuffer->width;
	ms_fbHeight = cam->frameBuffer->height;

	scaling = ms_fbHeight / 480.0f;

	IDirect3DVertexBuffer9 *vbuf;
	IDirect3DIndexBuffer9 *ibuf;
	d3d9device->CreateVertexBuffer(MAXDROPS * 4 * sizeof(VertexTex2), D3DUSAGE_WRITEONLY, DROPFVF, D3DPOOL_MANAGED, &vbuf, nullptr);
	ms_vertexBuf = vbuf;
	d3d9device->CreateIndexBuffer(MAXDROPS * 6 * sizeof(short), D3DUSAGE_WRITEONLY, D3DFMT_INDEX16, D3DPOOL_MANAGED, &ibuf, nullptr);
	ms_indexBuf = ibuf;
	RwUInt16 *idx;
	ibuf->Lock(0, 0, (void**)&idx, 0);
	for (int i = 0; i < MAXDROPS; i++) {
		idx[i * 6 + 0] = i * 4 + 0;
		idx[i * 6 + 1] = i * 4 + 1;
		idx[i * 6 + 2] = i * 4 + 2;
		idx[i * 6 + 3] = i * 4 + 0;
		idx[i * 6 + 4] = i * 4 + 2;
		idx[i * 6 + 5] = i * 4 + 3;
	}
	ibuf->Unlock();

	int w, h;
	for (w = 1; w < cam->frameBuffer->width; w <<= 1);
	for (h = 1; h < cam->frameBuffer->height; h <<= 1);
	ms_raster = RwRasterCreate(w, h, 0, 5);
	ms_tex = RwTextureCreate(ms_raster);
	ms_tex->filterAddressing = 0x3302;
	RwTextureAddRef(ms_tex);

	ms_initialised = 1;
}

void WaterDrops::Process()
{
	if (!ms_initialised)
		InitialiseRender(rwcam);
	WaterDrops::CalculateMovement();
	WaterDrops::SprayDrops();
	WaterDrops::ProcessMoving();
	WaterDrops::Fade();
}

void WaterDrops::CalculateMovement()
{
	RwMatrix *modelMatrix;
	modelMatrix = &RwCameraGetFrame(rwcam)->modelling;
	RwV3dSub(&ms_posDelta, &modelMatrix->pos, &ms_lastPos);
	ms_distMoved = RwV3dLength(&ms_posDelta);
	// RwV3d pos;
	// pos.x = (modelMatrix->at.x - ms_lastAt.x) * 10.0f;
	// pos.y = (modelMatrix->at.y - ms_lastAt.y) * 10.0f;
	// pos.z = (modelMatrix->at.z - ms_lastAt.z) * 10.0f;
	// ^ result unused for now
	ms_lastAt = modelMatrix->at;
	ms_lastPos = modelMatrix->pos;

	ms_vec.x = -RwV3dDotProduct(&modelMatrix->right, &ms_posDelta);
	ms_vec.y = RwV3dDotProduct(&modelMatrix->up, &ms_posDelta);
	ms_vec.z = RwV3dDotProduct(&modelMatrix->at, &ms_posDelta);
	RwV3dScale(&ms_vec, &ms_vec, 10.0f);
	ms_vecLen = sqrt(ms_vec.y*ms_vec.y + ms_vec.x*ms_vec.x);

	short mode = getCamMode();
	bool istopdown = mode == MODE_TOPDOWN || mode == MODE_TWOPLAYER_SEPARATE_CARS_TOPDOWN;
	bool carlookdirection = 0;
	if (mode == MODE_1STPERSON && FindPlayerVehicle(-1, 0)) {
		CPad *p = CPad::GetPad(0);
		if (p->GetLookBehindForCar() || p->GetLookLeft() || p->GetLookRight())
			carlookdirection = 1;
	}
	ms_enabled = !istopdown && !carlookdirection;
	ms_movingEnabled = !istopdown && !carlookdirection;

	float c = modelMatrix->at.z;
	if (c > 1.0f) c = 1.0f;
	if (c < -1.0f) c = -1.0f;
	ms_rainStrength = RAD2DEG(acos(c));
}

bool WaterDrops::NoRain()
{
	return CCullZones__CamNoRain() || CCullZones__PlayerNoRain() || CWeather__UnderWaterness > 0.0f || *CGame__currArea != 0 || *CEntryExitManager__ms_exitEnterState != 0;
}

WaterDrop* WaterDrops::PlaceNew(float x, float y, float size, float ttl, bool fades, int R = 0xFF, int G = 0xFF, int B = 0xFF)
{
	WaterDrop *drop;
	int i;

	if (NoRain())
		return NULL;

	for (i = 0, drop = ms_drops; i < MAXDROPS; i++, drop++)
		if (ms_drops[i].active == 0)
			goto found;
	return NULL;
found:
	ms_numDrops++;
	drop->x = x;
	drop->y = y;
	drop->size = size;
	drop->uvsize = (SC(MAXSIZE) - size + 1.0f) / (SC(MAXSIZE) - SC(MINSIZE) + 1.0f);
	drop->fades = fades;
	drop->active = 1;
	drop->r = R;
	drop->g = G;
	drop->b = B;
	drop->alpha = 0xFF;
	drop->time = 0.0f;
	drop->ttl = ttl;
	return drop;
}

void WaterDrops::NewDropMoving(WaterDrop *drop)
{
	WaterDropMoving *moving;
	for (moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
		if (moving->drop == NULL)
			goto found;
	return;
found:
	ms_numDropsMoving++;
	moving->drop = drop;
	moving->dist = 0.0f;
}

void WaterDrops::FillScreenMoving(float amount, bool isBlood = false)
{
	int n = (ms_vec.z <= 5.0f ? 1.0f : 1.5f)*amount*20.0f;
	float x, y, time;
	WaterDrop *drop;

	if(isBlood && !config->neoBloodDrops)
		return;

	while (n--)
		if (ms_numDrops < MAXDROPS && ms_numDropsMoving < MAXDROPSMOVING) {
			x = rand() % ms_fbWidth;
			y = rand() % ms_fbHeight;
			time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			if (!isBlood)
				drop = PlaceNew(x, y, time, 2000.0f, 1);
			else
				drop = PlaceNew(x, y, time, 2000.0f, 1, 0xFF, 0x00, 0x00);
			if (drop)
				NewDropMoving(drop);
		}
}

void WaterDrops::SprayDrops()
{
	static int ndrops[] = {
		125, 250, 500, 1000, 1000,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0
	};

	if (CWeather__Rain != 0.0f && ms_enabled) {
		int tmp = 180.0f - ms_rainStrength;
		if (tmp < 40) tmp = 40;
		FillScreenMoving((tmp - 40.0f) / 150.0f * CWeather__Rain * 0.5f);
	}
	if (ms_splashDuration >= 0) {
		if (ms_numDrops < MAXDROPS) {
			//if (isIII()) 
			//{
			//	if (ms_splashObject)
			//	{
			//      RwV3d dist;
			//		RwV3dSub(&dist, &ms_splashObject->matrix.matrix.pos, &ms_lastPos);
			//		int n = ndrops[ms_splashDuration] * (1.0f - (RwV3dLength(&dist) - 5.0f) / 15.0f);
			//		while (n--)
			//			if (ms_numDrops < MAXDROPS) {
			//				float x = rand() % ms_fbWidth;
			//				float y = rand() % ms_fbHeight;
			//				float time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			//				PlaceNew(x, y, time, 10000.0f, 0);
			//			}
			//	}
			//	else
			//		FillScreenMoving(1.0f);
			//}
			//else
				FillScreenMoving(1.0f); // VC does STRANGE things here
		}
		ms_splashDuration--;
	}
}

void WaterDrops::NewTrace(WaterDropMoving *moving)
{
	if (ms_numDrops < MAXDROPS) {
		moving->dist = 0.0f;
		PlaceNew(moving->drop->x, moving->drop->y, SC(MINSIZE), 500.0f, 1, moving->drop->r, moving->drop->g, moving->drop->b);
	}
}

void WaterDrops::MoveDrop(WaterDropMoving *moving)
{
	WaterDrop *drop = moving->drop;
	if (!ms_movingEnabled)
		return;
	if (!drop->active) {
		moving->drop = NULL;
		ms_numDropsMoving--;
		return;
	}
	if (ms_vec.z <= 0.0f || ms_distMoved <= 0.3f)
		return;

	short mode = getCamMode();
	if (ms_vecLen <= 0.5f || mode == MODE_1STPERSON) {
		// movement out of center 
		float d = ms_vec.z*0.2f;
		float dx, dy, sum;
		dx = drop->x - ms_fbWidth*0.5f + ms_vec.x;
		if (mode == MODE_1STPERSON)
			dy = drop->y - ms_fbHeight*1.2f - ms_vec.y;
		else
			dy = drop->y - ms_fbHeight*0.5f - ms_vec.y;
		sum = fabs(dx) + fabs(dy);
		if (sum >= 0.001f) {
			dx *= (1.0 / sum);
			dy *= (1.0 / sum);
		}
		moving->dist += d;
		if (moving->dist > 20.0f)
			NewTrace(moving);
		drop->x += dx * d;
		drop->y += dy * d;
	}
	else {
		// movement when camera turns
		moving->dist += ms_vecLen;
		if (moving->dist > 20.0f)
			NewTrace(moving);
		// flipped sign, why no longer check for look right/left? [aap]
		drop->x -= ms_vec.x;
		drop->y += ms_vec.y;
	}

	if (drop->x < 0.0f || drop->y < 0.0f ||
		drop->x > ms_fbWidth || drop->y > ms_fbHeight) {
		moving->drop = NULL;
		ms_numDropsMoving--;
	}
}

void WaterDrops::ProcessMoving()
{
	WaterDropMoving *moving;
	if (!ms_movingEnabled)
		return;
	for (moving = ms_dropsMoving; moving < &ms_dropsMoving[MAXDROPSMOVING]; moving++)
		if (moving->drop)
			MoveDrop(moving);
}

void WaterDrop::Fade()
{
	int delta = CTimer__ms_fTimeStep * 1000.0f / 50.0f;
	this->time += delta;
	if (this->time >= this->ttl) {
		WaterDrops::ms_numDrops--;
		this->active = 0;
	}
	else if (this->fades)
		this->alpha = 255 - time / ttl * 255;
}

void WaterDrops::Fade()
{
	WaterDrop *drop;
	for (drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if (drop->active)
			drop->Fade();
}

void WaterDrops::AddToRenderList(WaterDrop *drop)
{
	static float xy[] = {
		-1.0f, -1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f, -1.0f
	};
	static float uv[] = {
		0.0f, 0.0f, 0.0f, 1.0f,
		1.0f, 1.0f, 1.0f, 0.0f
	};
	int i;
	float scale;

	float u1_1, u1_2;
	float v1_1, v1_2;
	float tmp;

	tmp = drop->uvsize*(300.0f - 40.0f) + 40.0f;
	u1_1 = drop->x + ms_xOff - tmp;
	v1_1 = drop->y + ms_yOff - tmp;
	u1_2 = drop->x + ms_xOff + tmp;
	v1_2 = drop->y + ms_yOff + tmp;
	u1_1 = (u1_1 <= 0.0f ? 0.0f : u1_1) / ms_raster->width;
	v1_1 = (v1_1 <= 0.0f ? 0.0f : v1_1) / ms_raster->height;
	u1_2 = (u1_2 >= ms_fbWidth ? ms_fbWidth : u1_2) / ms_raster->width;
	v1_2 = (v1_2 >= ms_fbHeight ? ms_fbHeight : v1_2) / ms_raster->height;

	scale = drop->size * 0.5f;

	for (i = 0; i < 4; i++, ms_vertPtr++) {
		ms_vertPtr->x = drop->x + xy[i * 2] * scale + ms_xOff;
		ms_vertPtr->y = drop->y + xy[i * 2 + 1] * scale + ms_yOff;
		ms_vertPtr->z = 0.0f;
		ms_vertPtr->rhw = 1.0f;
		ms_vertPtr->emissiveColor = D3DCOLOR_ARGB(drop->alpha, drop->r, drop->g, drop->b);
		ms_vertPtr->u0 = uv[i * 2];
		ms_vertPtr->v0 = uv[i * 2 + 1];
		ms_vertPtr->u1 = i >= 2 ? u1_2 : u1_1;
		ms_vertPtr->v1 = i % 3 == 0 ? v1_2 : v1_1;
	}
	ms_numBatchedDrops++;
}

void WaterDrops::Render()
{
	bool nofirstperson = FindPlayerVehicle(-1, 0) == 0 && getCamMode() == MODE_1STPERSON;
	if (!ms_enabled || ms_numDrops <= 0 || nofirstperson || CCutsceneMgr__ms_running)
		return;

	IDirect3DVertexBuffer9 *vbuf = (IDirect3DVertexBuffer9*)ms_vertexBuf;
	vbuf->Lock(0, 0, (void**)&ms_vertPtr, 0);
	ms_numBatchedDrops = 0;
	for (auto drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		if (drop->active)
			AddToRenderList(drop);
	vbuf->Unlock();

	RwRasterPushContext(ms_raster);
	RwRasterRenderFast(RwCameraGetRaster(rwcam), 0, 0);
	RwRasterPopContext();

	DefinedState();
	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)1);

	RwD3D9SetTexture(ms_maskTex, 0);
	RwD3D9SetTexture(ms_tex, 1);

	RwD3D9SetTextureStageState(0, D3DTSS_TEXCOORDINDEX, 0);
	RwD3D9SetTextureStageState(1, D3DTSS_TEXCOORDINDEX, 1);
	RwD3D9SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_DIFFUSE);
	RwD3D9SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_DIFFUSE);
	RwD3D9SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_MODULATE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLORARG1, D3DTA_TEXTURE);
	RwD3D9SetTextureStageState(1, D3DTSS_COLORARG2, D3DTA_CURRENT);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAARG1, D3DTA_CURRENT);
	RwD3D9SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	RwD3D9SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	RwD3D9SetFVF(DROPFVF);
	RwD3D9SetStreamSource(0, vbuf, 0, sizeof(VertexTex2));
	RwD3D9SetIndices(ms_indexBuf);
	RwD3D9DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, ms_numBatchedDrops * 4, 0, ms_numBatchedDrops * 6);

	RwD3D9SetTexture(NULL, 1);
	RwD3D9SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE);
	RwD3D9SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE);
	DefinedState();

	RwRenderStateSet(rwRENDERSTATEFOGENABLE, 0);
	RwRenderStateSet(rwRENDERSTATEZTESTENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATEZWRITEENABLE, (void*)1);
	RwRenderStateSet(rwRENDERSTATETEXTURERASTER, NULL);
	RwRenderStateSet(rwRENDERSTATEVERTEXALPHAENABLE, (void*)0);
}

/////////////////
void WaterDrops::FillScreen(int n)
{
	float x, y, time;
	WaterDrop *drop;

	if (!ms_initialised)
		return;
	ms_numDrops = 0;
	for (drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++) {
		drop->active = 0;
		if (drop < &ms_drops[n]) {
			x = rand() % ms_fbWidth;
			y = rand() % ms_fbHeight;
			time = rand() % (SC(MAXSIZE) - SC(MINSIZE)) + SC(MINSIZE);
			PlaceNew(x, y, time, 2000.0f, 1);
		}
	}
}

void WaterDrops::Clear()
{
	WaterDrop *drop;
	for (drop = &ms_drops[0]; drop < &ms_drops[MAXDROPS]; drop++)
		drop->active = 0;
	ms_numDrops = 0;
}

void WaterDrops::Reset()
{
	Clear();
	ms_splashDuration = -1;
	ms_splashObject = NULL;
}
