//ret addr
#pragma intrinsic(_ReturnAddress)
//#define WFPlayer 0x84A73B //old
#define WFPlayer 0xEA952A //new

//used for logging/cycling through values
bool logger = false;
int countnum = -1;

//green, red texture
//LPDIRECT3DTEXTURE9 texWarface, texBlackwood, texBlue, texYellow;

//texture
D3DLOCKED_RECT pLockedRect;
D3DSURFACE_DESC desc;
IDirect3DTexture9* pCurrentTexture;

//elementcount
D3DVERTEXELEMENT9 decl[MAXD3DDECLLENGTH];
UINT numElements;

//vertexshader
IDirect3DVertexShader9* vShader;
UINT vSize;

//pixelshader
IDirect3DPixelShader9* pShader;
UINT pSize;

//vertexshaderconstantf
//UINT mStartRegister;
//UINT mVector4fCount;

//crc
DWORD texCRC;

//viewport
D3DVIEWPORT9 Viewport; //global vp
float ScreenCenterX;
float ScreenCenterY;

//ret addr
#pragma intrinsic(_ReturnAddress)
bool								g_Init = false;
int									g_Index = -1;
char                                g_Text[256] = { '\0' };
std::vector<void*>					g_Vector;
void*								g_SelectedAddress = NULL;

// settings
int wallhack = 1;				//wallhack
int esp = 0;					//esp

//aimbot settings
int aimbot = 1;
int aimkey = 2;
DWORD Daimkey = VK_RBUTTON;		//aimkey
int aimsens = 2;
int aimfov = 1;					//aim fov in % 
int aimheight = 6;				//aim height

//autoshoot settings
int autoshoot = 1;
bool IsPressed = false;			//

int nosmoke = 1;

//timer
DWORD frametime = timeGetTime();

//=====================================================================================================================

// getdir & log
char dlldir[320];
char* GetDirectoryFile(char *filename)
{
	static char path[320];
	strcpy_s(path, dlldir);
	strcat_s(path, filename);
	return path;
}

void Log(const char *fmt, ...)
{
	if (!fmt)	return;

	char		text[4096];
	va_list		ap;
	va_start(ap, fmt);
	vsprintf_s(text, fmt, ap);
	va_end(ap);

	ofstream logfile(GetDirectoryFile("log.txt"), ios::app);
	if (logfile.is_open() && text)	logfile << text << endl;
	logfile.close();
}

DWORD QuickChecksum(DWORD *pData, int size)
{
	if (!pData) { return 0x0; }

	DWORD sum;
	DWORD tmp;
	sum = *pData;

	for (int i = 1; i < (size / 4); i++)
	{
		tmp = pData[i];
		tmp = (DWORD)(sum >> 29) + tmp;
		tmp = (DWORD)(sum >> 17) + tmp;
		sum = (DWORD)(sum << 3) ^ tmp;
	}

	return sum;
}

LPD3DXSPRITE lpSprite = NULL;
LPDIRECT3DTEXTURE9 lpSpriteImage = NULL;
bool bSpriteCreated = false;

bool CreateOverlaySprite(IDirect3DDevice9* pd3dDevice)
{
	HRESULT hr;

	hr = D3DXCreateTextureFromFile(pd3dDevice, GetDirectoryFile("target.png"), &lpSpriteImage); //png in hack dir
	if(FAILED(hr))
	{
		//Log("D3DXCreateTextureFromFile failed");
		bSpriteCreated = false;
		return false;
	}

	hr = D3DXCreateSprite(pd3dDevice, &lpSprite);
	if(FAILED(hr))
	{
		Log("D3DXCreateSprite failed");
		bSpriteCreated = false;
		return false;
	}

	bSpriteCreated = true;
	
	return true;
}

// COM utils
template<class COMObject>
void SafeRelease(COMObject*& pRes)
{
	IUnknown *unknown = pRes;
	if (unknown)
	{
		unknown->Release();
	}
	pRes = NULL;
}

// This will get called before Device::Clear(). If the device has been reset
// then all the work surfaces will be created again.
void PreClear(IDirect3DDevice9* device)
{
	if (!bSpriteCreated)
		CreateOverlaySprite(device);
}

// Delete work surfaces when device gets reset
void DeleteRenderSurfaces()
{
	if (lpSprite != NULL)
	{
		//Log("SafeRelease(lpSprite)");
		SafeRelease(lpSprite);
	}

	bSpriteCreated = false;
}

// This gets called right before the frame is presented on-screen - Device::Present().
// First, create the display text, FPS and info message, on-screen. Then then call
// CopySurfaceToTextureBuffer() to downsample the image and copy to shared memory
void PrePresent(IDirect3DDevice9* Device, int cx, int cy)
{
	int textOffsetLeft;

	//draw sprite
	if (bSpriteCreated)
	{
		if (lpSprite != NULL && lpSprite != NULL)
		{
			D3DXVECTOR3 position;
			position.x = (float)cx;
			position.y = (float)cy;
			position.z = 0.0f;

			textOffsetLeft = (int)position.x; //for later to offset text from image

			lpSprite->Begin(D3DXSPRITE_ALPHABLEND);
			lpSprite->Draw(lpSpriteImage, NULL, NULL, &position, 0xFFFFFFFF);
			lpSprite->End();
		}
	}

	// draw text
}


HRESULT GenerateTexture(LPDIRECT3DDEVICE9 Device, IDirect3DTexture9 **ppD3Dtex, DWORD colour32)
{
	if (FAILED(Device->CreateTexture(8, 8, 1, 0, D3DFMT_A4R4G4B4, D3DPOOL_MANAGED, ppD3Dtex, NULL)))
		return E_FAIL;

	WORD colour16 = ((WORD)((colour32 >> 28) & 0xF) << 12)
		| (WORD)(((colour32 >> 20) & 0xF) << 8)
		| (WORD)(((colour32 >> 12) & 0xF) << 4)
		| (WORD)(((colour32 >> 4) & 0xF) << 0);

	D3DLOCKED_RECT d3dlr;
	(*ppD3Dtex)->LockRect(0, &d3dlr, 0, 0);
	WORD *pDst16 = (WORD*)d3dlr.pBits;

	for (int xy = 0; xy < 8 * 8; xy++)
		*pDst16++ = colour16;

	(*ppD3Dtex)->UnlockRect(0);

	return S_OK;
}

//drawpoint
void FillRGB(LPDIRECT3DDEVICE9 pDevice, int x, int y, int w, int h, D3DCOLOR color)
{
	D3DRECT rec = { x, y, x + w, y + h };
	pDevice->Clear(1, &rec, D3DCLEAR_TARGET, color, 0, 0);
}

//=====================================================================================================================

//get distance
float GetDistance(float Xx, float Yy, float xX, float yY)
{
	return sqrt((yY - Yy) * (yY - Yy) + (xX - Xx) * (xX - Xx));
}

//aim worldtoscreen
struct AimInfo_t
{
	float vOutX, vOutY;
	INT       iTeam;
	//float RealDistance;
	float CrosshairDistance;
};
std::vector<AimInfo_t>AimInfo;
float RealDistance;

void AddAim(LPDIRECT3DDEVICE9 Device, int iTeam)
{
	float xx, yy;
	D3DXMATRIX matrix, worldmatrix, m1;
	D3DXVECTOR4 position, input;

	//Device->GetViewport(&Viewport);

	Device->GetVertexShaderConstantF(36, m1, 4);
	Device->GetVertexShaderConstantF(0, worldmatrix, 3);

	input.x = worldmatrix._14;
	input.y = worldmatrix._24;
	input.z = worldmatrix._34 + (float)aimheight/5.0f;

	D3DXMatrixTranspose(&matrix, &m1);
	//D3DXVec4Transform(&position, &input, &matrix);

	position.x = input.x * matrix._11 + input.y * matrix._21 + input.z * matrix._31 + matrix._41;
	position.y = input.x * matrix._12 + input.y * matrix._22 + input.z * matrix._32 + matrix._42;
	position.z = input.x * matrix._13 + input.y * matrix._23 + input.z * matrix._33 + matrix._43;
	position.w = input.x * matrix._14 + input.y * matrix._24 + input.z * matrix._34 + matrix._44;

	RealDistance = position.w;// Viewport.MinZ + position.z * (Viewport.MaxZ - Viewport.MinZ); //real distance

		xx = ((position.x / position.w) * (Viewport.Width / 2.0f)) + Viewport.X + (Viewport.Width / 2.0f);
		yy = Viewport.Y + (Viewport.Height / 2.0f) - ((position.y / position.w) * (Viewport.Height / 2.0f));

	AimInfo_t pAimInfo = { static_cast<float>(xx), static_cast<float>(yy), iTeam };
	AimInfo.push_back(pAimInfo);
}

//==========================================================================================================================

// colors
#define Green				D3DCOLOR_ARGB(255, 000, 255, 000)
#define Red					D3DCOLOR_ARGB(255, 255, 000, 000)
#define Blue				D3DCOLOR_ARGB(255, 000, 000, 255)
#define Orange				D3DCOLOR_ARGB(255, 255, 165, 000)
#define Yellow				D3DCOLOR_ARGB(255, 255, 255, 000)
#define Pink				D3DCOLOR_ARGB(255, 255, 192, 203)
#define Cyan				D3DCOLOR_ARGB(255, 000, 255, 255)
#define Purple				D3DCOLOR_ARGB(255, 160, 032, 240)
#define Black				D3DCOLOR_ARGB(255, 000, 000, 000) 
#define White				D3DCOLOR_ARGB(255, 255, 255, 255)
#define Grey				D3DCOLOR_ARGB(255, 112, 112, 112)
#define SteelBlue			D3DCOLOR_ARGB(255, 033, 104, 140)
#define LightSteelBlue		D3DCOLOR_ARGB(255, 201, 255, 255)
#define LightBlue			D3DCOLOR_ARGB(255, 026, 140, 306)
#define Salmon				D3DCOLOR_ARGB(255, 196, 112, 112)
#define Brown				D3DCOLOR_ARGB(255, 168, 099, 020)
#define Teal				D3DCOLOR_ARGB(255, 038, 140, 140)
#define Lime				D3DCOLOR_ARGB(255, 050, 205, 050)
#define ElectricLime		D3DCOLOR_ARGB(255, 204, 255, 000)
#define Gold				D3DCOLOR_ARGB(255, 255, 215, 000)
#define OrangeRed			D3DCOLOR_ARGB(255, 255, 69, 0)
#define GreenYellow			D3DCOLOR_ARGB(255, 173, 255, 047)
#define AquaMarine			D3DCOLOR_ARGB(255, 127, 255, 212)
#define SkyBlue				D3DCOLOR_ARGB(255, 000, 191, 255)
#define SlateBlue			D3DCOLOR_ARGB(255, 132, 112, 255)
#define Crimson				D3DCOLOR_ARGB(255, 220, 020, 060)
#define DarkOliveGreen		D3DCOLOR_ARGB(255, 188, 238, 104)
#define PaleGreen			D3DCOLOR_ARGB(255, 154, 255, 154)
#define DarkGoldenRod		D3DCOLOR_ARGB(255, 255, 185, 015)
#define FireBrick			D3DCOLOR_ARGB(255, 255, 048, 048)
#define DarkBlue			D3DCOLOR_ARGB(255, 000, 000, 204)
#define DarkerBlue			D3DCOLOR_ARGB(255, 000, 000, 153)
#define DarkYellow			D3DCOLOR_ARGB(255, 255, 204, 000)
#define LightYellow			D3DCOLOR_ARGB(255, 255, 255, 153)
#define DarkOutline			D3DCOLOR_ARGB(255, 37,   48,  52)
#define TBlack				D3DCOLOR_ARGB(180, 000, 000, 000) 

//==========================================================================================================================

//-----------------------------------------------------------------------------
// Name: Save()
// Desc: Saves Menu Item states for later Restoration
//-----------------------------------------------------------------------------

//void Save(char* szSection, char* szKey, int iValue, LPCSTR file)
//{
	//char szValue[255];
	//sprintf_s(szValue, "%d", iValue);
	//WritePrivateProfileString(szSection, szKey, szValue, file);
//}

//-----------------------------------------------------------------------------
// Name: Load()
// Desc: Loads Menu Item States From Previously Saved File
//-----------------------------------------------------------------------------

//int Load(char* szSection, char* szKey, int iDefaultValue, LPCSTR file)
//{
	//int iResult = GetPrivateProfileInt(szSection, szKey, iDefaultValue, file);
	//return iResult;
//}

#include <string>
#include <fstream>
void SaveSettings()
{
	ofstream fout;
	fout.open(GetDirectoryFile("wfsettings.ini"), ios::trunc);
	fout << "Wallhack " << wallhack << endl;
	fout << "Aimbot " << aimbot << endl;
	fout << "Aimkey " << aimkey << endl;
	fout << "Aimsens " << aimsens << endl;
	fout << "Aimheight " << aimheight << endl;
	fout << "Aimfov " << aimfov << endl;
	fout << "Esp " << esp << endl;
	fout << "Autoshoot " << autoshoot << endl;
	fout << "Nosmoke " << nosmoke << endl;
	fout.close();
}

void LoadSettings()
{
	ifstream fin;
	string Word = "";
	fin.open(GetDirectoryFile("wfsettings.ini"), ifstream::in);
	fin >> Word >> wallhack;
	fin >> Word >> aimbot;
	fin >> Word >> aimkey;
	fin >> Word >> aimsens;
	fin >> Word >> aimheight;
	fin >> Word >> aimfov;
	fin >> Word >> esp;
	fin >> Word >> autoshoot;
	fin >> Word >> nosmoke;
	fin.close();
}

//==========================================================================================================================

// menu

int MenuSelection = 0;
int Current = true;

int PosX = 30;
int PosY = 27;

int Show = false; //off by default

POINT cPos;

#define ItemColorOn Green
#define ItemColorOff Red
#define ItemCurrent White
#define GroupColor Yellow
#define KategorieFarbe Yellow
#define ItemText White

LPD3DXFONT pFont; //font
bool m_bCreated = false;

int CheckTabs(int x, int y, int w, int h)
{
	if (Show)
	{
		GetCursorPos(&cPos);
		ScreenToClient(GetForegroundWindow(), &cPos);
		if (cPos.x > x && cPos.x < x + w && cPos.y > y && cPos.y < y + h)
		{
			if (GetAsyncKeyState(VK_LBUTTON) & 1)
			{
				return 1;
			}
			return 2;
		}
	}
	return 0;
}

HRESULT DrawRectangle(LPDIRECT3DDEVICE9 Device, FLOAT x, FLOAT y, FLOAT w, FLOAT h, DWORD Color)
{
	HRESULT hRet;

	const DWORD D3D_FVF = (D3DFVF_XYZRHW | D3DFVF_DIFFUSE);

	struct Vertex
	{
		float x, y, z, ht;
		DWORD vcolor;
	}
	V[4] =
	{
		{ x, (y + h), 0.0f, 0.0f, Color },
		{ x, y, 0.0f, 0.0f, Color },
		{ (x + w), (y + h), 0.0f, 0.0f, Color },
		{ (x + w), y, 0.0f, 0.0f, Color }
	};

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		Device->SetPixelShader(0); //fix black color
		Device->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		Device->SetFVF(D3D_FVF);
		Device->SetTexture(0, NULL);
		hRet = Device->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, V, sizeof(Vertex));
	}

	return hRet;
}

VOID DrawBorder(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, INT px, DWORD BorderColor)
{
	DrawRectangle(Device, x, (y + h - px), w, px, BorderColor);
	DrawRectangle(Device, x, y, px, h, BorderColor);
	DrawRectangle(Device, x, y, w, px, BorderColor);
	DrawRectangle(Device, (x + w - px), y, px, h, BorderColor);
}

VOID DrawBoxWithBorder(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, DWORD BoxColor, DWORD BorderColor)
{
	DrawRectangle(Device, x, y, w, h, BoxColor);
	DrawBorder(Device, x, y, w, h, 1, BorderColor);
}

VOID DrawBox(LPDIRECT3DDEVICE9 Device, INT x, INT y, INT w, INT h, DWORD BoxColor)
{
	DrawBorder(Device, x, y, w, h, 1, BoxColor);
}

void WriteText(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	pFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_LEFT, color);
}

void lWriteText(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	pFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_RIGHT, color);
}

void cWriteText(int x, int y, DWORD color, char *text)
{
	RECT rect;
	SetRect(&rect, x, y, x, y);
	pFont->DrawText(0, text, -1, &rect, DT_NOCLIP | DT_CENTER, color);
}

HRESULT DrawString(LPD3DXFONT pFont, INT X, INT Y, DWORD dColor, CONST PCHAR cString, ...)
{
	HRESULT hRet;

	CHAR buf[512] = { NULL };
	va_list ArgumentList;
	va_start(ArgumentList, cString);
	_vsnprintf_s(buf, sizeof(buf), sizeof(buf) - strlen(buf), cString, ArgumentList);
	va_end(ArgumentList);

	RECT rc[2];
	SetRect(&rc[0], X, Y, X, 0);
	SetRect(&rc[1], X, Y, X + 50, 50);

	hRet = D3D_OK;

	if (SUCCEEDED(hRet))
	{
		pFont->DrawTextA(NULL, buf, -1, &rc[0], DT_NOCLIP, 0xFF000000);
		hRet = pFont->DrawTextA(NULL, buf, -1, &rc[1], DT_NOCLIP, dColor);
	}

	return hRet;
}

void Category(LPDIRECT3DDEVICE9 pDevice, char *text)
{
	if (Show)
	{
		int Check = CheckTabs(PosX, PosY + (Current * 15), 190, 10);
		DWORD ColorText;

		ColorText = KategorieFarbe;

		if (Check == 2)
			ColorText = ItemCurrent;

		if (MenuSelection == Current)
			ColorText = ItemCurrent;

		WriteText(PosX - 5, PosY + (Current * 15) - 1, ColorText, text);
		lWriteText(PosX + 175, PosY + (Current * 15) - 1, ColorText, "[-]");
		Current++;
	}
}

void AddItem(LPDIRECT3DDEVICE9 pDevice, char *text, int &var, char **opt, int MaxValue)
{
	if (Show)
	{
		int Check = CheckTabs(PosX, PosY + (Current * 15), 190, 10);
		DWORD ColorText;

		if (var)
		{
			DrawBox(pDevice, PosX, PosY + (Current * 15), 10, 10, Green);
			ColorText = ItemColorOn;
		}
		if (var == 0)
		{
			DrawBox(pDevice, PosX, PosY + (Current * 15), 10, 10, Red);
			ColorText = ItemColorOff;
		}

		if (Check == 1)
		{
			var++;
			if (var > MaxValue)
				var = 0;
		}

		if (Check == 2)
			ColorText = ItemCurrent;

		if (MenuSelection == Current)
		{
			if (GetAsyncKeyState(VK_RIGHT) & 1)
			{
				var++;
				if (var > MaxValue)
					var = 0;
			}
			else if (GetAsyncKeyState(VK_LEFT) & 1)
			{
				var--;
				if (var < 0)
					var = MaxValue;
			}
		}

		if (MenuSelection == Current)
			ColorText = ItemCurrent;

		WriteText(PosX + 13, PosY + (Current * 15) - 1, ColorText, text);
		lWriteText(PosX + 148, PosY + (Current * 15) - 1, ColorText, opt[var]);
		Current++;
	}
}

//==========================================================================================================================

// menu part
char *opt_OnOff[] = { "[OFF]", "[ON]" };
char *opt_Teams[] = { "[OFF]", "[Warface]", "[Blackwood]", "[All]" };
char *opt_Keys[] = { "[OFF]", "[Shift]", "[RMouse]", "[LMouse]", "[Ctrl]", "[Alt]", "[Space]", "[X]", "[C]" };
char *opt_Sensitivity[] = { "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]" };
char *opt_Aimheight[] = { "[0]", "[1]", "[2]", "[3]", "[4]", "[5]", "[6]", "[7]", "[8]", "[9]" };
char *opt_Aimfov[] = { "[0]", "[10%]", "[20%]", "[30%]", "[40%]", "[50%]", "[60%]", "[70%]", "[80%]", "[90%]" };
char *opt_Autoshoot[] = { "[OFF]", "[OnKeyDown]", "[Auto]" };

void BuildMenu(LPDIRECT3DDEVICE9 pDevice)
{
	if (GetAsyncKeyState(VK_INSERT) & 1)
	{
		Show = !Show;

		//save settings
		SaveSettings();
	}

	if (Show && pFont)
	{
		if (GetAsyncKeyState(VK_UP) & 1)
			MenuSelection--;

		if (GetAsyncKeyState(VK_DOWN) & 1)
			MenuSelection++;

		//Background
		FillRGB(pDevice, 25, 38, 157, 142, TBlack);

		DrawBox(pDevice, 20, 15, 168, 20, DarkOutline);
		cWriteText(105, 18, White, "WFbot");
		DrawBox(pDevice, 20, 34, 168, Current * 15, DarkOutline);

		Current = 1;
		//Category(pDevice, " [D3D]");
		AddItem(pDevice, " Wallhack", wallhack, opt_OnOff, 1);
		AddItem(pDevice, " Aimbot", aimbot, opt_Teams, 3);
		AddItem(pDevice, " Aimkey", aimkey, opt_Keys, 8);
		AddItem(pDevice, " Aimsens", aimsens, opt_Sensitivity, 8);
		AddItem(pDevice, " Aimheight", aimheight, opt_Aimheight, 9);
		AddItem(pDevice, " Aimfov", aimfov, opt_Aimfov, 9);
		AddItem(pDevice, " Autoshoot", autoshoot, opt_Autoshoot, 2);
		AddItem(pDevice, " Esp", esp, opt_Teams, 3);
		AddItem(pDevice, " Nosmoke", nosmoke, opt_OnOff, 1);

		if (MenuSelection >= Current)
			MenuSelection = 1;

		if (MenuSelection < 1)
			MenuSelection = 9;//Current;
	}
}

//=====================================================================================================================

//retaddr
bool IsAddressPresent(void* Address)
{
	for (auto it = g_Vector.begin(); it != g_Vector.end(); ++it)
	{
		if (*it == Address)
			return true;
	}
	return false;
}

struct DirectX9VTable
{
	DWORD QueryInterface; // 0
	DWORD AddRef; // 1
	DWORD Release; // 2
	DWORD TestCooperativeLevel; // 3
	DWORD GetAvailableTextureMem; // 4
	DWORD EvictManagedResources; // 5
	DWORD GetDirect3D; // 6
	DWORD GetDeviceCaps; // 7
	DWORD GetDisplayMode; // 8
	DWORD GetCreationParameters; // 9
	DWORD SetCursorProperties; // 10
	DWORD SetCursorPosition; // 11
	DWORD ShowCursor; // 12
	DWORD CreateAdditionalSwapChain; // 13
	DWORD GetSwapChain; // 14
	DWORD GetNumberOfSwapChains; // 15
	DWORD Reset; // 16
	DWORD Present; // 17
	DWORD GetBackBuffer; // 18
	DWORD GetRasterStatus; // 19
	DWORD SetDialogBoxMode; // 20
	DWORD SetGammaRamp; // 21
	DWORD GetGammaRamp; // 22
	DWORD CreateTexture; // 23
	DWORD CreateVolumeTexture; // 24
	DWORD CreateCubeTexture; // 25
	DWORD CreateVertexBuffer; // 26
	DWORD CreateIndexBuffer; // 27
	DWORD CreateRenderTarget; // 28
	DWORD CreateDepthStencilSurface; // 29
	DWORD UpdateSurface; // 30
	DWORD UpdateTexture; // 31
	DWORD GetRenderTargetData; // 32
	DWORD GetFrontBufferData; // 33
	DWORD StretchRect; // 34
	DWORD ColorFill; // 35
	DWORD CreateOffscreenPlainSurface; // 36
	DWORD SetRenderTarget; // 37
	DWORD GetRenderTarget; // 38
	DWORD SetDepthStencilSurface; // 39
	DWORD GetDepthStencilSurface; // 40
	DWORD BeginScene; // 41
	DWORD EndScene; // 42
	DWORD Clear; // 43
	DWORD SetTransform; // 44
	DWORD GetTransform; // 45
	DWORD MultiplyTransform; // 46
	DWORD SetViewport; // 47
	DWORD GetViewport; // 48
	DWORD SetMaterial; // 49
	DWORD GetMaterial; // 50
	DWORD SetLight; // 51
	DWORD GetLight; // 52
	DWORD LightEnable; // 53
	DWORD GetLightEnable; // 54
	DWORD SetClipPlane; // 55
	DWORD GetClipPlane; // 56
	DWORD SetRenderState; // 57
	DWORD GetRenderState; // 58
	DWORD CreateStateBlock; // 59
	DWORD BeginStateBlock; // 60
	DWORD EndStateBlock; // 61
	DWORD SetClipStatus; // 62
	DWORD GetClipStatus; // 63
	DWORD GetTexture; // 64
	DWORD SetTexture; // 65
	DWORD GetTextureStageState; // 66
	DWORD SetTextureStageState; // 67
	DWORD GetSamplerState; // 68
	DWORD SetSamplerState; // 69
	DWORD ValidateDevice; // 70
	DWORD SetPaletteEntries; // 71
	DWORD GetPaletteEntries; // 72
	DWORD SetCurrentTexturePalette; // 73
	DWORD GetCurrentTexturePalette; // 74
	DWORD SetScissorRect; // 75
	DWORD GetScissorRect; // 76
	DWORD SetSoftwareVertexProcessing; // 77
	DWORD GetSoftwareVertexProcessing; // 78
	DWORD SetNPatchMode; // 79
	DWORD GetNPatchMode; // 80
	DWORD DrawPrimitive; // 81
	DWORD DrawIndexedPrimitive; // 82
	DWORD DrawPrimitiveUP; // 83
	DWORD DrawIndexedPrimitiveUP; // 84
	DWORD ProcessVertices; // 85
	DWORD CreateVertexDeclaration; // 86
	DWORD SetVertexDeclaration; // 87
	DWORD GetVertexDeclaration; // 88
	DWORD SetFVF; // 89
	DWORD GetFVF; // 90
	DWORD CreateVertexShader; // 91
	DWORD SetVertexShader; // 92
	DWORD GetVertexShader; // 93
	DWORD SetVertexShaderConstantF; // 94
	DWORD GetVertexShaderConstantF; // 95
	DWORD SetVertexShaderConstantI; // 96
	DWORD GetVertexShaderConstantI; // 97
	DWORD SetVertexShaderConstantB; // 98
	DWORD GetVertexShaderConstantB; // 99
	DWORD SetStreamSource; // 100
	DWORD GetStreamSource; // 101
	DWORD SetStreamSourceFreq; // 102
	DWORD GetStreamSourceFreq; // 103
	DWORD SetIndices; // 104
	DWORD GetIndices; // 105
	DWORD CreatePixelShader; // 106
	DWORD SetPixelShader; // 107
	DWORD GetPixelShader; // 108
	DWORD SetPixelShaderConstantF; // 109
	DWORD GetPixelShaderConstantF; // 110
	DWORD SetPixelShaderConstantI; // 111
	DWORD GetPixelShaderConstantI; // 112
	DWORD SetPixelShaderConstantB; // 113
	DWORD GetPixelShaderConstantB; // 114
	DWORD DrawRectPatch; // 115
	DWORD DrawTriPatch; // 116
	DWORD DeletePatch; // 117
	DWORD CreateQuery; // 118
};

bool bCompare(const BYTE* pData, const BYTE* bMask, const char* szMask)
{
	for (; *szMask; ++szMask, ++pData, ++bMask)
		if (*szMask == 'x' && *pData != *bMask)
			return false;

	return (*szMask) == NULL;
}

DWORD FindPattern(DWORD dwAddress, DWORD dwLen, BYTE *bMask, char * szMask)
{
	for (DWORD i = 0; i < dwLen; i++)
		if (bCompare((BYTE*)(dwAddress + i), bMask, szMask))
			return (DWORD)(dwAddress + i);

	return 0;
}