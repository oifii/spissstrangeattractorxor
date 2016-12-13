/*
 * Copyright (c) 2010-2016 Stephane Poirier
 *
 * stephane.poirier@oifii.org
 *
 * Stephane Poirier
 * 3532 rue Ste-Famille, #3
 * Montreal, QC, H2X 2L1
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

// spissstrangeattractorxor.cpp : Defines the entry point for the console application.
// nakedsoftware.org, spi@oifii.org, stephane.poirier@oifii.org

#include "stdafx.h"

/*
int _tmain(int argc, _TCHAR* argv[])
{
	return 0;
}
*/



#include <windows.h>
#include  <scrnsave.h>
#include  <GL/gl.h>
#include <GL/glu.h>
#include <math.h>
//#include <ctime> //for random initialization
#include <stdio.h>
#include <vector>
#include "resource.h"

using namespace std; //for glfont.h
#include "glfont.h"

//std::ofstream myofstream;
FILE* pFILE = NULL;

BYTE global_alpha=200;


//get rid of these warnings:
//truncation from const double to float
//conversion from double to float
#pragma warning(disable: 4305 4244) 
                        

//Define a Windows timer

#define TIMER 1



//These forward declarations are just for readability,
//so the big three functions can come first 

void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC);
void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC);
void GetConfig();               
void WriteConfig(HWND hDlg);
void SetupAnimation(HDC hDC, int Width, int Height);
void CleanupAnimation();
void OnTimer(HDC hDC);


int Width, Height; //globals for size of screen

GLFont global_GLFont;
//PixelPerfectGLFont global_GLFont;

float	x = 0.1, y = 0.1,		// starting point
	a = -0.966918,			// coefficients for "The King's Dream"
	b = 2.879879,
	c = 0.765145,
	d = 0.744728;
int	initialIterations = 100,	// initial number of iterations
					// to allow the attractor to settle
	//iterations = 100;		// number of times to iterate through
	iterations = 100000;		// number of times to iterate through
	//iterations = 1000000;		// number of times to iterate through
					// the functions and draw a point

class glfloatpair
{
public:
	GLfloat x;
	GLfloat y;

	glfloatpair(GLfloat xx, GLfloat yy)
	{
		x = xx;
		y = yy;
	}
	glfloatpair(double xx, double yy)
	{
		x = xx;
		y = yy;
	}
};

std::vector<glfloatpair*> global_pairvector;
std::vector<glfloatpair*>::iterator it;


//////////////////////////////////////////////////
////   INFRASTRUCTURE -- THE THREE FUNCTIONS   ///
//////////////////////////////////////////////////


// Screen Saver Procedure
LRESULT WINAPI ScreenSaverProc(HWND hWnd, UINT message, 
                               WPARAM wParam, LPARAM lParam)
{
  static HDC hDC;
  static HGLRC hRC;
  static RECT rect;

	//debug
	//myofstream.open("debug.txt", std::ios::out);
	//myofstream << "test line\n";
    pFILE = fopen("debug.txt", "w");
	//fprintf(pFILE, "test1\n");

	/*
	//////////////////////////
	//initialize random number
	//////////////////////////
	srand((unsigned)time(0));
	*/

  switch ( message ) 
  {

  case WM_CREATE: 
		//spi, avril 2015, begin
		SetWindowLong(hWnd, GWL_EXSTYLE, GetWindowLong(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		SetLayeredWindowAttributes(hWnd, 0, global_alpha, LWA_ALPHA);
		//SetLayeredWindowAttributes(h, 0, 200, LWA_ALPHA);
		//spi, avril 2015, end

		// get window dimensions
		GetClientRect( hWnd, &rect );
		Width = rect.right;         
		Height = rect.bottom;
    
        //get configuration from registry
        GetConfig();

        // setup OpenGL, then animation
		InitGL( hWnd, hDC, hRC );
		SetupAnimation(hDC, Width, Height);


		//set timer to tick every 10 ms
		//SetTimer( hWnd, TIMER, 10, NULL );
		SetTimer( hWnd, TIMER, 10000, NULL );
		//SetTimer( hWnd, TIMER, 5000, NULL );
		//SetTimer( hWnd, TIMER, 2500, NULL );
		//SetTimer( hWnd, TIMER, 1000, NULL ); //original
		//SetTimer( hWnd, TIMER, 100, NULL );
		//SetTimer( hWnd, TIMER, 10, NULL );
    return 0;
 
  case WM_DESTROY:
		KillTimer( hWnd, TIMER );
        CleanupAnimation();
		CloseGL( hWnd, hDC, hRC );
		if(pFILE) fclose(pFILE);
		//erase vector
		for(it=global_pairvector.begin(); it!=global_pairvector.end(); it++)
		{
			if(*it) delete *it;
		}
		global_pairvector.clear();

    return 0;

  case WM_TIMER:
    OnTimer(hDC);       //animate!      
    return 0;                           

  }

  return DefScreenSaverProc(hWnd, message, wParam, lParam );

}

bool bTumble = true;


BOOL WINAPI
ScreenSaverConfigureDialog(HWND hDlg, UINT message, 
                           WPARAM wParam, LPARAM lParam)
{

  //InitCommonControls();  
  //would need this for slider bars or other common controls

  HWND aCheck;

  switch ( message ) 
  {

        case WM_INITDIALOG:
                LoadString(hMainInstance, IDS_DESCRIPTION, szAppName, 40);

                GetConfig();
				/*
                aCheck = GetDlgItem( hDlg, IDC_TUMBLE );
                SendMessage( aCheck, BM_SETCHECK, 
                bTumble ? BST_CHECKED : BST_UNCHECKED, 0 );
				*/

    return TRUE;

  case WM_COMMAND:
    switch( LOWORD( wParam ) ) 
                { 
			/*
            case IDC_TUMBLE:
                        bTumble = (IsDlgButtonChecked( hDlg, IDC_TUMBLE ) == BST_CHECKED);
                        return TRUE;
			*/
                //cases for other controls would go here

            case IDOK:
                        WriteConfig(hDlg);      //get info from controls
                        EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
                        return TRUE; 

                case IDCANCEL: 
                        EndDialog( hDlg, LOWORD( wParam ) == IDOK ); 
                        return TRUE;   
                }

  }     //end command switch

  return FALSE; 
}



// needed for SCRNSAVE.LIB
BOOL WINAPI RegisterDialogClasses(HANDLE hInst)
{
  return TRUE;
}


/////////////////////////////////////////////////
////   INFRASTRUCTURE ENDS, SPECIFICS BEGIN   ///
////                                          ///
////    In a more complex scr, I'd put all    ///
////     the following into other files.      ///
/////////////////////////////////////////////////


// Initialize OpenGL
static void InitGL(HWND hWnd, HDC & hDC, HGLRC & hRC)
{
  
  PIXELFORMATDESCRIPTOR pfd;
  ZeroMemory( &pfd, sizeof pfd );
  pfd.nSize = sizeof pfd;
  pfd.nVersion = 1;
  //pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL; //blaine's
  pfd.dwFlags = PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
  pfd.iPixelType = PFD_TYPE_RGBA;
  pfd.cColorBits = 24;
  
  hDC = GetDC( hWnd );
  
  int i = ChoosePixelFormat( hDC, &pfd );  
  SetPixelFormat( hDC, i, &pfd );

  hRC = wglCreateContext( hDC );
  wglMakeCurrent( hDC, hRC );

}

// Shut down OpenGL
static void CloseGL(HWND hWnd, HDC hDC, HGLRC hRC)
{
  wglMakeCurrent( NULL, NULL );
  wglDeleteContext( hRC );

  ReleaseDC( hWnd, hDC );
}


void SetupAnimation(HDC hDC, int Width, int Height)
{
		//glFont font
		GLuint textureName;
		glGenTextures(1, &textureName);
		global_GLFont.Create("arial-10.glf", textureName);
		//global_GLFont.Create("arial-20.glf", textureName);
		//global_GLFont.Create("arial-bold-10.glf", textureName);
		//global_GLFont.Create("segeo-script-10.glf", textureName);
		/*
		//spi, begin
		createPalette();
		stepX = (maxX-minX)/(GLfloat)Width; // calculate new value of step along X axis
		stepY = (maxY-minY)/(GLfloat)Height; // calculate new value of step along Y axis
		glShadeModel(GL_SMOOTH);
		glEnable(GL_DEPTH_TEST);
		//spi, end
		*/
        //window resizing stuff
        glViewport(0, 0, (GLsizei) Width, (GLsizei) Height);

        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        
		//spi, begin
        //glOrtho(-300, 300, -240, 240, 25, 75); //original
        //glOrtho(-300, 300, -8, 8, 25, 75); //spi, last
		glOrtho(-2.0f, 2.0f, -2.0f, 2.0f, ((GLfloat)-1), (GLfloat)1); //spi
        //glOrtho(-150, 150, -120, 120, 25, 75); //spi
		//spi, end
        glRotatef(180.0, 0.0, 0.0, 1.0);
        glRotatef(180.0, 0.0, 1.0, 0.0);

		
		glMatrixMode(GL_MODELVIEW);

        glLoadIdentity();
		/*
        gluLookAt(0.0, 0.0, 50.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0); //original
                //camera xyz, the xyz to look at, and the up vector (+y is up)
        */
		//glEnable(GL_TEXTURE_2D); //spi
        //background
        glClearColor(0.0, 0.0, 0.0, 0.0); //0.0s is black

		// set the foreground (pen) color
		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

		// enable blending
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		// enable point smoothing
		glEnable(GL_POINT_SMOOTH);
		glPointSize(1.0f);		
		
		/*
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
        glShadeModel(GL_SMOOTH); 
		*/

        //no need to initialize any objects
        //but this is where I'd do it

		/*
        glColor3f(0.1, 1.0, 0.3); //green
		*/
		// compute some initial iterations to settle into the orbit of the attractor
		//for (int i = 0; i &lt; initialIterations; i++) 
		for (int i = 0; i<initialIterations; i++) 
		{
 
			// compute a new point using the strange attractor equations
			float xnew = sin(y*b) + c*sin(x*b);
			float ynew = sin(x*a) + d*sin(y*a);
 
			// save the new point
			x = xnew;
			y = ynew;
		}
		//spi, begin
		//draw strange attractor once
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glPushMatrix();
		glBegin(GL_POINTS);
			// go through the equations many times, drawing a point for each iteration
			for (int i = 0; i<iterations; i++) 
			//for (int i = 0; i<10; i++) 
			{
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
		glEnd();
		glFlush();
        SwapBuffers(hDC);
        glPopMatrix();


		//spi, end
}


void OnTimer(HDC hDC) //increment and display
{
		char buffer[100];
        //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red

        glPushMatrix();

		//finally, enable xor drawing
		glEnable(GL_COLOR_LOGIC_OP);	
		glLogicOp(GL_XOR);

		//erase vector
		//xor draw over previous point
		glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
		glBegin(GL_POINTS);
			for(it=global_pairvector.begin(); it!=global_pairvector.end(); it++)
			{
					GLfloat xlabel = (*it)->x;
					GLfloat ylabel = (*it)->y; 
					glVertex3f(xlabel, ylabel, 0.0f);
			}
		glEnd();

		glEnable(GL_TEXTURE_2D);
		//glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
		//glColor4f(0.0f, 0.0f, 1.0f, 1.0f); //blue
		glScalef(0.02, 0.02, 0.02);
		int iii=0;
		for(it=global_pairvector.begin(); it!=global_pairvector.end(); it++)
		{
			//xor draw over previous label
			GLfloat xlabel = (*it)->x;
			GLfloat ylabel = (*it)->y; 
			sprintf_s(buffer, 100-1, "%d", iii); 
			global_GLFont.TextOut(buffer, xlabel/0.02,ylabel/0.02,0);
			//delete point
			if(*it) delete *it;
			//increment iii
			iii++;
		}
		glDisable(GL_TEXTURE_2D);
		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
		glScalef(1.0, 1.0, 1.0);
		glFlush();
		global_pairvector.clear();

		// draw some points
		glBegin(GL_POINTS);
			// go through the equations many times, drawing a point for each iteration
			for (int i = 0; i<1; i++) 
			//for (int i = 0; i<iterations; i++) 
			//for (int i = 0; i<10; i++) 
			{
 
				// compute a new point using the strange attractor equations
				float xnew = sin(y*b) + c*sin(x*b);
				float ynew = sin(x*a) + d*sin(y*a);
 
				// save the new point
				x = xnew;
				y = ynew;
 
				//add pair to vector
				global_pairvector.push_back(new glfloatpair(x,y));

				// draw the new point
				glVertex3f(x, y, 0.0f);
			}
		glEnd();

		glEnable(GL_TEXTURE_2D);
		//glColor4f(1.0f, 0.0f, 0.0f, 1.0f); //red
		//glColor4f(0.0f, 0.0f, 1.0f, 1.0f); //blue
		glScalef(0.02, 0.02, 0.02);
		for (int ii = 0; ii<1; ii++) 
		//for (int ii = 0; ii<iterations; ii++) 
		//for (int ii = 0; ii<10; ii++) 
		{
 
			// compute a new point using the strange attractor equations
			//float xnew = sin(y*b) + c*sin(x*b);
			//float ynew = sin(x*a) + d*sin(y*a);
 
			// save the new point
			//x = xnew;
			//y = ynew;
 			//spi, begin
			GLfloat xlabel = (global_pairvector[ii])->x;
			GLfloat ylabel = (global_pairvector[ii])->y; 

			sprintf_s(buffer, 100-1, "%d", ii); 
			global_GLFont.TextOut(buffer, xlabel/0.02,ylabel/0.02,0);
			//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
			//glScalef(1.0, 1.0, 1.0);
			//spi, end
			//glFlush();
		}
		glDisable(GL_TEXTURE_2D);
		//glColor4f(1.0f, 1.0f, 1.0f, 1.0f); //white
		glScalef(1.0, 1.0, 1.0);
		glFlush();
        SwapBuffers(hDC);
        glPopMatrix();
		glDisable(GL_COLOR_LOGIC_OP);
}

void CleanupAnimation()
{
    //didn't create any objects, so no need to clean them up
	//glDisable(GL_TEXTURE_2D);
}



/////////   REGISTRY ACCESS FUNCTIONS     ///////////

void GetConfig()
{

        HKEY key;
        //DWORD lpdw;

        if (RegOpenKeyEx( HKEY_CURRENT_USER,
                //"Software\\GreenSquare", //lpctstr
                L"Software\\Oifii\\Spissstrangeattractor", //lpctstr
                0,                      //reserved
                KEY_QUERY_VALUE,
                &key) == ERROR_SUCCESS) 
        {
                DWORD dsize = sizeof(bTumble);
                DWORD dwtype =  0;

                //RegQueryValueEx(key,"Tumble", NULL, &dwtype, (BYTE*)&bTumble, &dsize);
                RegQueryValueEx(key,L"Tumble", NULL, &dwtype, (BYTE*)&bTumble, &dsize);


                //Finished with key
                RegCloseKey(key);
        }
        else //key isn't there yet--set defaults
        {
                bTumble = true;
        }
        
}

void WriteConfig(HWND hDlg)
{

        HKEY key;
        DWORD lpdw;

        if (RegCreateKeyEx( HKEY_CURRENT_USER,
                //"Software\\GreenSquare", //lpctstr
                L"Software\\Oifii\Spissstrangeattractor", //lpctstr
                0,                      //reserved
                L"",                     //ptr to null-term string specifying the object type of this key
                REG_OPTION_NON_VOLATILE,
                KEY_WRITE,
                NULL,
                &key,
                &lpdw) == ERROR_SUCCESS)
                
        {
                //RegSetValueEx(key,"Tumble", 0, REG_DWORD, 
                RegSetValueEx(key,L"Tumble", 0, REG_DWORD, 
                        (BYTE*)&bTumble, sizeof(bTumble));

                //Finished with keys
                RegCloseKey(key);
        }

}