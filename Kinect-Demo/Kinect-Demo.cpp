#include "../Kinect-win32.h"

#include <conio.h>
#include <windows.h>
#include <math.h>

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glu32.lib")

//#define GL_GLEXT_PROTOTYPES

#include "Glee-5.4/GLee.h"
#include <gl/glu.h>

#define BUILDDEPTHWINDOW
#define BUILDCOLORWINDOW
#define BUILDGLWINDOW

float const KinectMinDistance = -10;
float const KinectDepthScaleFactor = .0021f;

float KinectColorScaleFactor = .0023f;
void KinectDepthToWorld(float &x, float &y, float &z)
{
	float zz = z;
	float xx = (x - 320) * (zz + KinectMinDistance) * KinectDepthScaleFactor ;
	float yy = (y - 240) * (zz + KinectMinDistance) * KinectDepthScaleFactor ;
	
	zz-=200;
	zz*=-1;

	x = xx;
	y = yy;
	z = zz;
};
float  rgbxoffset = -1.8;
float  rgbyoffset = -2.4;

void WorldToRGBSpace(float &x, float &y, float z)
{
	z*=-1;
	z+=200;
	float ox,oy;
	ox = ((x + rgbxoffset) / (KinectColorScaleFactor))/ (z + KinectMinDistance);
	oy = ((y + rgbyoffset) / (KinectColorScaleFactor))/ (z + KinectMinDistance);

	ox+=320;
	oy+=240;


	x = __min(640,__max(0,ox));
	y = __min(480,__max(0,oy));

};


class Vertex
{
public:
	Vertex()
	{
	};

	Vertex(float _X, float _Y, float _Z)
	{
		x = _X;
		y = _Y;
		z = _Z;
	};
	float x,y,z; // position
	float u,v; // texture coord 1
	unsigned char r,g,b;

	void DepthToWorld()
	{
		KinectDepthToWorld(x,y,z);;
	};
};

class KinectTest: public Kinect::KinectListener
{
public:

	unsigned short mGammaMap[2048];
	unsigned char DepthColor[640*480*3];
	float DepthAverage[640*480];

	float mDepthBuffer[640*480];

	Kinect::Kinect *mKinect;
	float mMotorPosition;
	int mLedMode;


	void SetupWindow();
	void Run();

	float mLastValidAcceleroX;
	float mLastValidAcceleroY;
	float mLastValidAcceleroZ;

	int mLastDepthFrameCounter;
	int mDepthFrameCounter; 

#ifdef BUILDDEPTHWINDOW
	// members for test display 1: (depth)
	HWND W;
	HBITMAP theDib ;
	unsigned char *theBitmap ;
#endif

#ifdef BUILDCOLORWINDOW
	// members for test display 2: (color)
	HWND W2;
	HBITMAP theDib2 ;
	unsigned char *theBitmap2 ;
#endif

#ifdef BUILDGLWINDOW

	// members for test display 3: (opengl output);
	HWND W3;
	HGLRC mGLContext ;
	float mTime;
	float mAngle;
	float mTilt;
	void InitGL();
	void DrawGL(float w, float h);
	GLuint mDepthTexture;
	GLuint mColorTexture;
	GLuint mVBO;
	int mColorMode;
	Vertex mVertices[640*480];
	int mActualVertexCount;

	void DoGLChar(unsigned short KeyCode);
#endif

	void DoChar(unsigned short KeyCode)
	{
		switch (KeyCode)
		{
		case VK_LEFT:	mLedMode = (mLedMode + 1)%8; mKinect->SetLedMode(mLedMode); break;
		case VK_RIGHT:  mLedMode = (mLedMode + 8 - 1)%8; mKinect->SetLedMode(mLedMode); break;
		case VK_UP:  mMotorPosition = __min(1.0f, mMotorPosition + 0.05f); mKinect->SetMotorPosition(mMotorPosition);break;
		case VK_DOWN:  mMotorPosition = __max(0, mMotorPosition - 0.05f); mKinect->SetMotorPosition(mMotorPosition);break;
		case VK_ESCAPE: PostQuitMessage(0);break;
		case ' ': mColorMode = (mColorMode+1)%2;break;
		case 'Q':KinectColorScaleFactor += 0.0001;printf("colorscale: %f\n",KinectColorScaleFactor ); break;
		case 'A':KinectColorScaleFactor -= 0.0001;printf("colorscale: %f\n",KinectColorScaleFactor ); break;

		case 'W':rgbyoffset += 0.1;printf("yoff: %f\n",rgbyoffset ); break;
		case 'S':rgbyoffset -= 0.1;printf("yoff: %f\n",rgbyoffset ); break;

		case 'E':rgbxoffset += 0.1;printf("xoff: %f\n",rgbxoffset ); break;
		case 'D':rgbxoffset -= 0.1;printf("xoff: %f\n",rgbxoffset ); break;
		};
	};

	void ParseDepth()
	{
		mDepthFrameCounter++;
		mKinect->ParseDepthBuffer();

		int i =0 ;
		for (int y = 0;y<480;y++)
		{
			unsigned char *destrow = DepthColor + ((479-y)*(640))*3;
			float *destdepth = mDepthBuffer + ((479-y)*(640));
			for (int x = 0;x<640;x++)
			{

				unsigned short Depth = mKinect->mDepthBuffer[i];
				if (Depth>0 && Depth != 0x07ff)
				{
					*destdepth++ = 100.0f/(-0.00307f * Depth + 3.33f);
				}
				else
				{
					*destdepth++  = -100000;
				}
				int pval = mGammaMap[Depth];
				int lb = pval & 0xff;
				switch (pval>>8) 
				{
				case 0:
					destrow[2] = 255;
					destrow[1] = 255-lb;
					destrow[0] = 255-lb;
					break;
				case 1:
					destrow[2] = 255;
					destrow[1] = lb;
					destrow[0] = 0;
					break;
				case 2:
					destrow[2] = 255-lb;
					destrow[1] = 255;
					destrow[0] = 0;
					break;
				case 3:
					destrow[2] = 0;
					destrow[1] = 255;
					destrow[0] = lb;
					break;
				case 4:
					destrow[2] = 0;
					destrow[1] = 255-lb;
					destrow[0] = 255;
					break;
				case 5:
					destrow[2] = 0;
					destrow[1] = 0;
					destrow[0] = 255-lb;
					break;
				default:
					destrow[2] = 0;
					destrow[1] = 0;
					destrow[0] = 0;
					break;
				}

				destrow+=3;
				i++;
			};
		};

	}
	KinectTest(Kinect::Kinect *inK)
	{
		mLastDepthFrameCounter = 0;
		mDepthFrameCounter = 0;
		mKinect = inK;
		for (int i=0; i<2048; i++) 
		{	
			mGammaMap[i] = (unsigned short)(float)(powf(i/2048.0f,3)*6*6*256);	
		};
		mMotorPosition = 1.0;
		mLedMode = 0;

	};

	~KinectTest()
	{
		mKinect->RemoveListener(this);
	};

	virtual void KinectDisconnected(Kinect::Kinect *K) 
	{
	};

	virtual void DepthReceived(Kinect::Kinect *K) 
	{
#ifdef BUILDDEPTHWINDOW
		InvalidateRect(W, NULL, false);
#else
#ifdef BUILDGLWINDOW
		ParseDepth();
#endif
#endif

#ifdef BUILDGLWINDOW
		InvalidateRect(W3, NULL, false);
#endif
	};

	virtual void ColorReceived(Kinect::Kinect *K) 
	{
#ifdef BUILDCOLORWINDOW
		InvalidateRect(W2, NULL, false);
#endif
	};

};

#ifdef BUILDDEPTHWINDOW
LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	KinectTest *KID = 	(KinectTest*)GetWindowLongPtr(hwnd,  GWLP_USERDATA);
	if (KID == NULL)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	Kinect::Kinect *K = KID->mKinect;

	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	static int w=250, h=150;

	switch(msg)  
	{
	case WM_KEYDOWN:
		KID->DoChar(wParam);
		break;
	case WM_ERASEBKGND:
		return false;
	case WM_TIMER:
		return false;
	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		w = LOWORD(lParam) + 1;
		h = HIWORD(lParam) + 1;
		InvalidateRect(hwnd, &rect, TRUE);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		{
			HDC hdcMem = CreateCompatibleDC(hdc);

			KID->ParseDepth();

			memcpy(KID->theBitmap, KID->DepthColor, 640*480*3);

			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, KID->theDib);
			RECT R;
			GetClientRect(hwnd, &R);
			int w = R.right-R.left;
			int h = R.bottom - R.top;
			StretchBlt(hdc, 0,0,w,h,hdcMem, 0,0,640,480,SRCCOPY);
			//BitBlt(hdc, 0, 0, 640,480, hdcMem, 0, 0, SRCCOPY);	
			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);

		};
		EndPaint(hwnd, &ps);
		break;
	case WM_QUIT:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif


#ifdef BUILDCOLORWINDOW
LRESULT CALLBACK WndProc2( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	KinectTest *KID = 	(KinectTest*)GetWindowLongPtr(hwnd,  GWLP_USERDATA);
	if (KID == NULL)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	Kinect::Kinect *K = KID->mKinect;

	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	static int w=250, h=150;

	switch(msg)  
	{
	case WM_ERASEBKGND:
		return false;
	case WM_KEYDOWN:
		KID->DoChar(wParam);
		break;

	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		w = LOWORD(lParam) + 1;
		h = HIWORD(lParam) + 1;
		InvalidateRect(hwnd, &rect, TRUE);
		break;
	case WM_TIMER:
		//		  InvalidateRect(hwnd, NULL,false);
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		{


			//				HBRUSH S = CreateSolidBrush(RGB(0,0,0));
			//				RECT R = {0,0,100,100};
			//				::FrameRect(hdc, &R, S);

			HDC hdcMem = CreateCompatibleDC(hdc);

			K->ParseColorBuffer();
			int i = 0;
			for (int y = 0;y<480;y++)
			{
				unsigned char *destroyw = KID->theBitmap2 + ((479-y)*(640))*3;

				for (int x = 0;x<640;x++)
				{
					unsigned char *c = &K->mColorBuffer[i];
					destroyw[2]   = *c++;
					destroyw[1]   = *c++;
					destroyw[0]   = *c++;
					destroyw+=3;
					i+=3;
				};
			};

			i =0 ;

			HBITMAP hbmOld = (HBITMAP)SelectObject(hdcMem, KID->theDib2);
			RECT R;
			GetClientRect(hwnd, &R);
			int w = R.right-R.left;
			int h = R.bottom - R.top;
			StretchBlt(hdc, 0,0,w,h,hdcMem, 0,0,640,480,SRCCOPY);

			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);

		};
		EndPaint(hwnd, &ps);
		break;
	case WM_QUIT:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

#ifdef BUILDGLWINDOW
LRESULT CALLBACK WndProc3( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{

	KinectTest *KID = 	(KinectTest*)GetWindowLongPtr(hwnd,  GWLP_USERDATA);
	if (KID == NULL)
	{
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	Kinect::Kinect *K = KID->mKinect;

	HDC hdc;
	PAINTSTRUCT ps;
	RECT rect;
	static int w=250, h=150;

	switch(msg)  
	{
	case WM_ERASEBKGND:
		return false;
	case WM_KEYDOWN:
		KID->DoGLChar(wParam);
		break;

	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		w = LOWORD(lParam) + 1;
		h = HIWORD(lParam) + 1;
		InvalidateRect(hwnd, &rect, TRUE);
		break;
	case WM_TIMER:
		InvalidateRect(hwnd, NULL,false);
		KID->mTime += 1/30.0f;
		break;
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		{
			RECT R;
			GetClientRect(hwnd, &R);
			int w = R.right-R.left;
			int h = R.bottom - R.top;

			wglMakeCurrent(hdc, KID->mGLContext);
			glViewport(0,0,w,h);
			KID->DrawGL((float)w,(float)h);			
			SwapBuffers( hdc );

		};
		EndPaint(hwnd, &ps);
		break;
	case WM_QUIT:
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
#endif

void KinectTest::SetupWindow()
{
	//W= CreateWindowA("edit" , "zephod kinect",WS_VISIBLE| WS_POPUP, 0,0,640,480,NULL,  0,0,0);

	RECT R = {0,0,640,480};
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW | WS_VISIBLE, false);
	int Width = R.right - R.left;
	int Height = R.bottom - R.top;


	BITMAPINFO bi;
	ZeroMemory(&bi, sizeof(BITMAPINFO));
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
	bi.bmiHeader.biBitCount = 24;
	bi.bmiHeader.biPlanes = 1;

	bi.bmiHeader.biCompression = BI_RGB;
	bi.bmiHeader.biHeight = 480;

	bi.bmiHeader.biWidth = 640;


#ifdef BUILDDEPTHWINDOW
	WNDCLASS wc1 = {0};


	wc1.lpszClassName = TEXT( "depth" );
	wc1.hInstance     = 0 ;
	wc1.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc1.lpfnWndProc   = WndProc ;
	wc1.hCursor       = LoadCursor(0, IDC_ARROW);

	RegisterClass(&wc1);
	W = CreateWindow( wc1.lpszClassName, TEXT("Zephod Kinect Demo - Depth - use cursor keys to control motor/leds"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, 0, Width, Height, NULL, NULL, 0, NULL);  
	SetWindowLongPtr(W, GWLP_USERDATA, (LONG)this);
	SetTimer(W, 0,1000,NULL);

	theBitmap = 0;
	theDib = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**)&theBitmap, NULL,0);

	ShowWindow(W, SW_SHOW);
	UpdateWindow(W);
#endif

#ifdef BUILDCOLORWINDOW

	WNDCLASS wc2 = {0};
	wc2.lpszClassName = TEXT( "color" );
	wc2.hInstance     = 0 ;
	wc2.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc2.lpfnWndProc   = WndProc2 ;
	wc2.hCursor       = LoadCursor(0, IDC_ARROW);
	RegisterClass(&wc2);

	W2 = CreateWindow( wc2.lpszClassName, TEXT("Zephod Kinect Demo - Color - use cursor keys to control motor/leds"),
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		Width, 0, Width, Height, NULL, NULL, 0, NULL);  
	SetWindowLongPtr(W2, GWLP_USERDATA, (LONG)this);

	theBitmap2 = 0;
	theDib2 = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**)&theBitmap2, NULL,0);


	SetTimer(W2, 0,30,NULL);
	ShowWindow(W2, SW_SHOW);
	UpdateWindow(W2);

#endif

#ifdef BUILDGLWINDOW
	WNDCLASS wc3 = {0};
	wc3.lpszClassName = TEXT( "glwindow" );
	wc3.hInstance     = 0 ;
	wc3.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc3.lpfnWndProc   = WndProc3 ;
	wc3.hCursor       = LoadCursor(0, IDC_ARROW);
	RegisterClass(&wc3);


	PIXELFORMATDESCRIPTOR pfd;  
	pfd.cColorBits = pfd.cDepthBits = 32; 
	pfd.dwFlags    = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;	


	W3 = CreateWindow( wc3.lpszClassName, TEXT("Zephod Kinect Demo - GL output - space to switch mode. use cursor keys to tilt/rotate"), WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		0, Height, Width, Height, NULL, NULL, 0, NULL);  
	SetWindowLongPtr(W3, GWLP_USERDATA, (LONG)this);

	HDC hDC = GetDC ( W3);     
	int PixelFormat = ChoosePixelFormat ( hDC, &pfd) ;
	SetPixelFormat ( hDC, PixelFormat , &pfd );
	mGLContext = wglCreateContext(hDC);
	wglMakeCurrent ( hDC, mGLContext );

	InitGL();

	ReleaseDC(W3, hDC);


	SetTimer(W3, 0,30,NULL);
	ShowWindow(W3, SW_SHOW);
	UpdateWindow(W3);
#endif

}


int main(int argc, char **argv)
{

#ifdef BUILDGLWINDOW

	GLeeInit();

#endif

	Kinect::KinectFinder KF;
	if (KF.GetKinectCount() < 1)
	{
		printf("Unable to find Kinect devices... Is one connected?\n");
		return 0;
	}

	Kinect::Kinect *K = KF.GetKinect();
	if (K == 0)
	{
		printf("error getting Kinect...\n");
		return 0;
	};

	KinectTest *KT = new KinectTest(K);	
	K->SetMotorPosition(1);
	K->SetLedMode(Kinect::Led_Yellow);
	float x,y,z;
	for (int i =0 ;i<10;i++)
	{
		if (K->GetAcceleroData(&x,&y,&z))
		{
			printf("accelerometer reports: %f,%f,%f\n", x,y,z);
		}
		Sleep(5);
	};
	KT->Run();
	K->SetLedMode(Kinect::Led_Red);
	delete KT;

	return 0;
};

void KinectTest::Run()
{
	SetupWindow();
	mKinect->AddListener(this);

	MSG m;

	while (GetMessage(&m, NULL, NULL, NULL))
	{
		TranslateMessage(&m);
		DispatchMessage(&m);
	};
};

#ifdef BUILDGLWINDOW
void KinectTest::InitGL()
{
	mColorMode = 1;
	glGenTextures(1, &mColorTexture);
	glGenTextures(1, &mDepthTexture);
	glBindTexture( GL_TEXTURE_2D, mColorTexture);

	unsigned char *blankimage = new unsigned char[1024*1024*3];
	for (int i= 0;i<1024*1024;i++)
	{
		int x = (i/32);
		int y = (x/32)/32;
		x%= (1024/32);
		unsigned char check = ((x+y)%2 ==1)?0xff:0;
		blankimage[i*3+0] = check;
		blankimage[i*3+1] = check;
		blankimage[i*3+2] = check;
	};
	glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_LINEAR);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
	::glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1024, 1024, 0,GL_RGB,GL_UNSIGNED_BYTE, blankimage);

	mTime = 0;
	mAngle = -2.3f;
	mTilt = 1;

	glGenBuffers(1, &mVBO);
	glBindBuffer(GL_ARRAY_BUFFER, mVBO);

	int id =0 ;
	for (int y = 0;y<480;y++)
	{
		for (int x = 0;x<640;x++)
		{
			mVertices[id].x = (float)x;
			mVertices[id].y = (float)y;
			mVertices[id].z = (float)sin(id*0.02);
			id++;
		};
	};
	glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex)*640*480, &mVertices[0].x, GL_STREAM_DRAW_ARB);



};

void RenderFrustum(float x, float y,float z)
{
	glPushMatrix();
	glTranslatef(x,y,z);
	Vertex A(0,0,30);
	Vertex B(640,0,30);
	Vertex C(640,480,30);
	Vertex D(0,480,30);
	Vertex E(0,0,2048);
	Vertex F(640,0,2048);
	Vertex G(640,480,2048);
	Vertex H(0,480,2048);
	A.DepthToWorld();
	B.DepthToWorld();
	C.DepthToWorld();
	D.DepthToWorld();
	E.DepthToWorld();
	F.DepthToWorld();
	G.DepthToWorld();
	H.DepthToWorld();
	
	Vertex *verts[] = {&A,&B,
				  &B,&C,
				  &C,&D,
				  &D,&A,
				  &E,&F,
				  &F,&G,
				  &G,&H,
				  &H,&E,
				  &A,&E,
				  &B,&F,
				  &C,&G,
				  &D,&H};

	glColor4f(1,1,1,0.3);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBegin(GL_LINES);

	for (int i = 0;i<12;i++)
	{
		glVertex3fv(&verts[i*2]->x);
		glVertex3fv(&verts[i*2+1]->x);
	};
	
	for (int i = 0;i<40;i++)
	{
		float lZ = 30 + ((2048-30)/40)*i;
		Vertex sA(0,0,lZ);
		Vertex sB(640,0,lZ);
		Vertex sC(640,480,lZ);
		Vertex sD(0,480,lZ);
	
		sA.DepthToWorld();
		sB.DepthToWorld();
		sC.DepthToWorld();
		sD.DepthToWorld();

		glVertex3fv(&sA.x);
		glVertex3fv(&sB.x);
		glVertex3fv(&sB.x);
		glVertex3fv(&sC.x);
		glVertex3fv(&sC.x);
		glVertex3fv(&sD.x);
		glVertex3fv(&sD.x);
		glVertex3fv(&sA.x);
	}
	glEnd();
	glPopMatrix();
};

void KinectTest::DrawGL(float w, float h)
{
	// clear to black
	glClearColor(0,0,0,1);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);

	// define camera properties
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	::gluPerspective(40,w/h, 0.05, 1000);

	// set up the camera position
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(sin(mAngle*0.2)*5,mTilt,cos(mAngle*0.2)*5,0,0,0,0,1,0);

	// draw a reference cube using red dots
	glPointSize(3);
	glColor4f(1,0,0,1);
	glBegin(GL_POINTS);
	glVertex3f(-1,-1,-1);
	glVertex3f(1,-1,-1);
	glVertex3f(1,1,-1);
	glVertex3f(-1,1,-1);
	glVertex3f(-1,-1,1);
	glVertex3f(1,-1,1);
	glVertex3f(1,1,1);
	glVertex3f(-1,1,1);
	glEnd();

	//	glEnable(GL_TEXTURE_2D);
	
	glPushMatrix();
	glScalef(0.01f,0.01f,0.01f);
	//glBegin(GL_POINTS);

	if (mDepthFrameCounter!=mLastDepthFrameCounter)
	{
		mLastDepthFrameCounter = mDepthFrameCounter;
		
		int id =0;
		for (int y  = 0;y<480;y+=3)
		{
			for (int x  = 0;x<640;x+=3)
			{
				//				glTexCoord2f(0,0);glVertex3f(-1,-1,-1);
				float zz = mDepthBuffer[(x+y*640)];
				if (zz>-90000)
				{
					
					/*float fac=4.6;
					int u = __min(639,__max(0,xx*fac + 320));
					int v = __min(479,__max(0,yy*fac + 240));
					int idx = (u+(479-v)*640)*3;*/
					

					
					//glColor3ub(mKinect->mColorBuffer[idx+0],mKinect->mColorBuffer[idx+1],mKinect->mColorBuffer[idx+2]);
					//glColor3ub(mKinect->mColorBuffer[idx+0],mKinect->mColorBuffer[idx+1],mKinect->mColorBuffer[idx+2]);
					mVertices[id].x = (float)x;
					mVertices[id].y = (float)y;
					mVertices[id].z = zz;
					KinectDepthToWorld(mVertices[id].x,mVertices[id].y,mVertices[id].z);
					
					if (mColorMode == 0)
					{
						mVertices[id].r = DepthColor[(x+y*640)*3+2];
						mVertices[id].g = DepthColor[(x+y*640)*3+1];
						mVertices[id].b = DepthColor[(x+y*640)*3+0];
					}
					else
					{

						mVertices[id].u = mVertices[id].x;
						mVertices[id].v = mVertices[id].y;
						WorldToRGBSpace(mVertices[id].u,mVertices[id].v, mVertices[id].z);

						int idx = ((int)(mVertices[id].u)+(479-(int)mVertices[id].v)*640)*3;
						float B = mKinect->mColorBuffer[idx+1]/255.0;
						mVertices[id].r = mKinect->mColorBuffer[idx+0];
						mVertices[id].g = mKinect->mColorBuffer[idx+1];
						mVertices[id].b = mKinect->mColorBuffer[idx+2];
					};
					//	glVertex3f(xx,yy,zz);
					
					id++;
				};
			};
		};
		//	glEnd();



		mActualVertexCount = id;

		glBindBuffer(GL_ARRAY_BUFFER_ARB, mVBO);
		glColor4f(1,1,1,1);
		float *buf = (float*)glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY);
		if (buf)
		{
			memcpy(buf, mVertices, sizeof(Vertex)*mActualVertexCount);
			glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
		};

		glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
		glFlush();
	};

	glDisable(GL_BLEND);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, mVBO);

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(Vertex), 0);  
	glColorPointer(3, GL_UNSIGNED_BYTE, sizeof(Vertex), (GLvoid*)20);  
	glDrawArrays(GL_POINTS, 0, mActualVertexCount);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
	glBindBuffer(GL_ARRAY_BUFFER_ARB, 0);
	RenderFrustum(0,0,0);


	glPopMatrix();
	glBindTexture(GL_TEXTURE_2D, mColorTexture);
	glEnable(GL_TEXTURE_2D);
	glColor4f(1,1,1,0.5);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (0)
	{
	glBegin(GL_QUADS);
		glTexCoord2f(0,0);glVertex3f(-1,-1,0);
		glTexCoord2f(1,0);glVertex3f(1,-1,0);
		glTexCoord2f(1,1);glVertex3f(1,1,0);
		glTexCoord2f(0,1);glVertex3f(-1,1,0);
	glEnd();
	};
}

void KinectTest::DoGLChar(unsigned short KeyCode)
{
	switch (KeyCode)
	{
	case VK_LEFT:
		mAngle += 0.1f;
		break;
	case VK_RIGHT:
		mAngle -= 0.1f;
		break;
	case VK_UP:
		mTilt += 0.1f;
		break;
	case VK_DOWN:
		mTilt -= 0.1f;
		break;
	default:
		DoChar(KeyCode);
		break;
	};
};

#endif