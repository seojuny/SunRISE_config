/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: XYZ.cpp                                                */
/*                                                                       */
/*  Revision: V1.1     Date: 02.06.2005    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 3D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 06.09.2003 minor changes
// Updated by: Chr. Monstein 04.12.2003 change from scope-mode to strip-chart-mode
// Updated by: Chr. Monstein 04.12.2003 frequency axis included
// Updated by: Chr. Monstein 05.07.2004 color table and background subtraction
// Updated by: Chr. Monstein 29.11.2004 XYZ...[] changed to *XYZ...
// Updated by: Chr. Monstein 02.06.2005 new locosc
// Updated by: Chr. Monstein 30.12.2009 new title
// Updated by: Chr. Monstein 09.01.2010 x-axis [sec]
// Updated by: Chr. Monstein 06.06.2010 x-axis repaired
// Updated by: Chr. Monstein 31.08.2012 repair bug in light curve calibration
// Updated by: Chr. Monstein 15.09.2017 y-axis label format %f -> %i
// Updated by: Chr. Monstein 26.05.2019 Background color Radio Button Win10 compatible
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <math.h> // floor

#include "XY.h" // maxXYbufflen
#include "XYZ.h"
#include "mainunit.h"
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
TForm2 *Form2;

TCanvas *pPlott;

extern int rows;                   // import from mainunit.cpp, number of measurement points per sweep
extern int nsps;                   // import from mainunit.cpp
extern long XYZbuflen;             // import from mainunit.cpp
extern char frqfilename[80];       // import from mainunit.cpp
extern char titlecomment[80];      // additional info on top of API

extern float freqinlist[501];      // import from EEPROM.cpp
extern float frequency[501];       // import from EEPROM.cpp
extern float locosc;               // import from EEPROM.cpp
//---------------------------------------------------------------------------

Byte *XYZ_ringbuff;
long XYZ_in=0, XYZ_out=0;
float *BGbuffer; // background buffer

int lowcol=0; // color offset
int higcol=255; // top color
//---------------------------------------------------------------------------
__fastcall TForm2::TForm2(TComponent* Owner)
    : TForm(Owner)
{
    ScrollBar1->Left = Form2->Width-ScrollBar1->Width-57;
    Label1->Left     = ScrollBar1->Left;
    ScrollBar2->Left = ScrollBar1->Left+40;
    Label2->Left     = ScrollBar2->Left;
    Original->Left   = ScrollBar1->Left;
    Smooth->Left     = ScrollBar1->Left;
    Fixed->Left      = ScrollBar1->Left;

    Form2->Left = MainForm->Left + MainForm->Width + 10;
    Form2->Top  = Form1->Top + Form1->Height+10;
}
//---------------------------------------------------------------------------
long TForm2::Color(float in) // 0.0 <= in <= 1.0
{
    float color;
    switch((int)floor(5*in))
    {
        case 0: color=256*256*255+(int)(255*(1-in*5));
            break;
        case 1: color=256*256*255+256*(int)(255*(in*5-1));
            break;
        case 2: color=256*255+256*256*(int)(255*(3-in*5));
            break;
        case 3: color=256*255+(int)(255*(in*5-3));
            break;
        case 4: color=255+256*(int)(255*(5-in*5));
            break;
        default: color=255;
            break;
    }
    return color;
}
//---------------------------------------------------------------------------
void __fastcall TForm2::XYZ(void)
{
    // das Feld für Image1 muss >> sein (z.B. 1200x1600)
    // als das aktuelle Fenster (y.B. 500x300)
    // damit ausreichend gezoomt werden kann.
    static bool fixedBG = false; // flag to set fixed background subtaction
    char st[80];
    pPlott = Form2->Image1->Canvas;
    pPlott->Pen->Color=clBlack;
    float dx, dy;

    // Ausserer Rahmen (Bildflaeche)
    int Nx = XYZbuflen/rows; // number of sweeps to be plotted
    int ALeft = 0;
    int ATop = 0;
    int ARight = ClientWidth;
    int ABottom = ClientHeight;                  // 75, 20, 105
    int BLeft=30; int BTop=30; int BRight=ARight-155; int BBottom=ABottom-50;

    TRect Rahmen = Rect(ALeft,ATop,ARight, ABottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color=clWhite;//clYellow;
    pPlott->FillRect(Rahmen);

    // Innerer (Plott)Rahmen
    TRect Plottarea = Rect(BLeft,BTop,BRight, BBottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color=clNavy;//clYellow;clNavy;//clBlack;
    pPlott->FillRect(Plottarea);
    pPlott->FrameRect(Plottarea);

    // y-ticks and y-label
    char Label[80];
    pPlott->Font->Size = 10;
    pPlott->Brush->Color = clWhite; // Schriftfarbe
    pPlott->Pen->Width = 1; // tickbreite
    int Ny = rows;  // 10 number of zwischenräume
    dy = (float)(BBottom-BTop)/(float)Ny; // pixels between 2 ticks
    int i = 1;
    float y;
    int w, h;
    int modLine = Ny/20; // left ticks
    int modText = Ny/10; // label left + right
    if (modLine<1) modLine=1;
    if (modText<1) modText=1;
    do
    {
        y = (float)BTop+(float)i*dy;
        if ((i%modLine)==0)
        {
            pPlott->MoveTo(BLeft  ,y);
            pPlott->LineTo(BLeft-5,y);
        }
        if ((i%modText)==0)
        {
            sprintf(Label,"%5u",Ny-i); // y-label left
            w = pPlott->TextWidth(Label);
            h = pPlott->TextHeight(Label);
            pPlott->TextOut(BLeft-6-w,y-h/2,Label);

            if (i>0)
            {
                //sprintf(Label,"%05.0fMHz",freqinlist[Ny-i]); // y-label right
                sprintf(Label,"%5i MHz",int(freqinlist[Ny-i]+0.5)); // y-label right
                h = pPlott->TextHeight(Label);
                pPlott->TextOut(BRight+6,y-h/2-dy/2,Label);
            }
            else
            {
                if ((y-0) >= BTop)
                {
                    sprintf(Label,"%06.1fMHz",freqinlist[Ny-1]); // y-label right
                    h = pPlott->TextHeight(Label);
                    pPlott->TextOut(BRight+6,y-h/2-dy/2,Label);
                }
            }
            pPlott->MoveTo(BRight  ,y);
            pPlott->LineTo(BRight+5,y);
        }
        i++;
    } while (i<=Ny);
    sprintf(Label,"Channel"); // y-label
    pPlott->TextOut(ALeft+2,ATop+2,Label);

    // x-ticks
    dx = (float)(BRight-BLeft)/(float)Nx;
    i = 0;
    int y1 = BBottom; int y2 = y1 + 5; int y3 = y2 + 4;
    int di = ((int) (float)Nx/(float)(BRight-BLeft))*50.0; // Bedeutung für Sync
    di = (di/10)*10;  if (di<10) di=10;
    float x;
    int tx;
    do {
        x = (float)BLeft+(float)i*dx;
        pPlott->MoveTo(x,y1);
        pPlott->LineTo(x,y2);
        tx = (i-XYZbuflen/rows)/nsps;
        sprintf(Label,"%i",tx);
        w = pPlott->TextWidth(Label);
        h = pPlott->TextHeight(Label);
        pPlott->LineTo(x,y3);
        pPlott->TextOut(x-w/2,y2+h/2,Label);
        i = i + di;
    } while (i<=Nx);
    sprintf(Label,"Time [sec]"); // y-label
    w = pPlott->TextWidth(Label);
    h = pPlott->TextHeight(Label);
    pPlott->TextOut((BLeft+BRight)/2-w/2,y3+h+2,Label);

    // title
    sprintf(Label,"Intensity in time- &frequency domain.");
    pPlott->Font->Size=11;
    w = pPlott->TextWidth(Label);
    pPlott->TextOut(ARight-w-10,ATop+2,Label); // right aligned
    sprintf(st,"XYZ  %s, %s",frqfilename,titlecomment);
    Form2->Caption=st;

    // color table plot
    x=ScrollBar2->Left-13;
    for (i=0; i<=ScrollBar1->Height; i++)
    {
        y = ScrollBar1->Top+i-1;
        pPlott->Pen->Color=(TColor) Color((float)i/(float)ScrollBar1->Height);
        pPlott->MoveTo(x,y);
        pPlott->LineTo(x+10,y);
    }

    // function-plott
    float ygain = (float)(BBottom-BTop)/rows; // number of channels  per sweep  (vertical)
    float xgain = (float)(BRight-BLeft)/(float)Nx;
    float px;
    float py;
    Byte yL;
    float z, znorm, zrange;
    int p;
    float q;
    pPlott->Pen->Width=1;
    zrange = higcol-lowcol;
    while (zrange<=5)
    {
        ScrollBar1->Position--; // low
        ScrollBar2->Position++; // high
        zrange = higcol-lowcol;
    }
    XYZ_out = (XYZ_in-1)%XYZbuflen; // damit er rechts beginnt
    for (long i=0; i<XYZbuflen; i=i+1)
    {
        yL = XYZ_ringbuff[XYZ_out];
        XYZ_out = (XYZ_out+1)%XYZbuflen; // get RX

        q = i/rows; // define time slot (horizontal address)
        px = BLeft + q*xgain; // get x-axis position

        p = i%rows; // define channel number (vertical address)
        py = BBottom-ygain*(float)p; // get y-axis position
        TRect Pixarea = Rect(px-xgain,py-ygain-0.5,px+xgain, py+0.5); // put pixel
        z = (float)yL;

        if (Smooth->Checked)
            BGbuffer[p] = (10*BGbuffer[p] + z)/11.0;  // smoothed background

        if (Original->Checked) // clear bg-buffer
            BGbuffer[p] = 0;

        if ((Fixed->Checked) && (!fixedBG)) // get actual background only once
        {
            BGbuffer[p] = z;
            if (p>=(rows-1))
                fixedBG=true;
        }

        if (!Fixed->Checked) // reset
            fixedBG=false;

        if (Original->Checked)
            znorm = (z -(float)lowcol)/(float)(higcol-lowcol);
        else  // all backgrund subtraction methods ...
            znorm = ((127+z-BGbuffer[p]) -(float)lowcol)/(float)(higcol-lowcol);

        pPlott->Brush->Color=(TColor) Color(znorm);
        pPlott->FillRect(Pixarea);
    }
}
//---------------------------------------------------------------------------
void __fastcall TForm2::ScrollBar1Change(TObject *Sender)
{
    char st[40];
    lowcol=(int)ScrollBar1->Position;
    sprintf(st,"L%3u",lowcol);
    Label1->Caption=st;
}
//---------------------------------------------------------------------------
void __fastcall TForm2::ScrollBar2Change(TObject *Sender)
{
    char st[40];
    higcol=(int)ScrollBar2->Position;
    sprintf(st,"H%3u",higcol);
    Label2->Caption=st;
}
//---------------------------------------------------------------------------
void __fastcall TForm2::FormResize(TObject *Sender)
{
   if (ClientWidth > Screen->Width*0.37)
       ClientWidth = Screen->Width*0.37;
   if (ClientHeight > Screen->Height*0.6)
       ClientHeight = Screen->Height*0.6;

    ScrollBar1->Left = Form2->Width-ScrollBar1->Width-57;
    Label1->Left     = ScrollBar1->Left;
    ScrollBar2->Left = ScrollBar1->Left+40;
    Label2->Left     = ScrollBar2->Left;
    Original->Left   = ScrollBar1->Left;
    Smooth->Left     = ScrollBar1->Left;
    Fixed->Left      = ScrollBar1->Left;
}
//---------------------------------------------------------------------------
void __fastcall TForm2::SmoothClick(TObject *Sender)
{
   ScrollBar1->Position=105; // low
   ScrollBar2->Position=180; // high
}
//---------------------------------------------------------------------------
void __fastcall TForm2::FixedClick(TObject *Sender)
{
   ScrollBar1->Position=105; // low
   ScrollBar2->Position=180; // high
}
//---------------------------------------------------------------------------
void __fastcall TForm2::OriginalClick(TObject *Sender)
{
   ScrollBar1->Position=0; // low
   ScrollBar2->Position=255; // high
}
//---------------------------------------------------------------------------

