/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: XY.cpp                                                 */
/*                                                                       */
/*  Revision: V1.1     Date: 02.06.2005    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 2D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 26.09.2003
// Updated by: Chr. Monstein 01.12.2003 title text changed
// Updated by: Chr. Monstein 29.11.2004 buffers changed to *XY...
// Updated by: Chr. Monstein 13.12.2004 x-axis label
// Updated by: Chr. Monstein 02.06.2005 new locosc
// Updated by: Chr. Monstein 30.12.2009 new title
// Updated by: Chr. Monstein 04.03.2010 XY plot in dB
// Updated by: Chr. Monstein 22.03.2011 XY plot update for 8/10bit
// Updated by: Chr. Monstein 31.08.2012 repair bug in light curve calibration
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "XY.h"
#include <stdio.h>
#include <math.h>
#include "mainunit.h"

#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
TForm1 *Form1;

extern int rows;                   // import from mainunit.cpp, number of measurement points per sweep
extern char frqfilename[80];       // import from mainunit.cpp
extern long XYbuflen;              // import from mainunit.cpp
extern float detector_sens;        // import from mainunit.cpp
extern int db_scale;               // import from mainunit.cpp
extern int adc_divider;            // import from mainunit.cpp
extern bool Callisto_8bit;         // import from mainunit.cpp

extern char titlecomment[80];      // additional info on top of API
extern float frequency[501];       // import from EEPROM.cpp
extern float freqinlist[501];      // import from EEPROM.cpp
extern float locosc;               // import from EEPROM.cpp

//---------------------------------------------------------------------------
int *XY_ringbuff;
long XY_in=1, XY_out=0; // ?????
float ref[500]; // background buffer
bool fixedBG = false; // flag to set fixed background subtaction
bool change = false; // to control visualization
//---------------------------------------------------------------------------
__fastcall TForm1::TForm1(TComponent* Owner)
    : TForm(Owner)
{
    Form1->Left = MainForm->Left + MainForm->Width + 10;
    Form1->Top  = MainForm->Top;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::XY(void)
{
    // das Feld für Image1 muss >> sein (z.B. 1200x1600)
    // als das aktuelle Fenster (y.B. 500x300)
    // damit ausreichend gezoomt werden kann.

    float dbcal = 1.0/(adc_divider*256.0)*2500.0/detector_sens;
    char st[80];
    TCanvas *pPlott = Form1->Image1->Canvas;
    pPlott->Pen->Color = clBlack;

    // Ausserer Rahmen (Bildflaeche)
    int ALeft = 0;
    int ATop = 0;
    int ARight = ClientWidth;
    int ABottom = ClientHeight;

    RadioButton1->Top = ABottom - 19;
    RadioButton2->Top = ABottom - 19;
    TRect Rahmen = Rect(ALeft,ATop,ARight, ABottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color = clWhite;//clYellow;
    pPlott->FillRect(Rahmen);

    // Innerer (Plott)Rahmen
    int BLeft=30; int BTop=25; int BRight=ARight-20; int BBottom=ABottom-50;
    if (!Callisto_8bit)
       BLeft = 40; // 1024 > 256
    TRect Plottarea = Rect(BLeft,BTop,BRight, BBottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color=clNavy;//clYellow;clNavy;//clBlack;
    pPlott->FillRect(Plottarea);
    pPlott->FrameRect(Plottarea);

    // y-ticks and y-label
    char Label[40];
    pPlott->Font->Size = 10;
    pPlott->Brush->Color = clWhite; // Schriftfarbe
    pPlott->Pen->Width = 1; // tickbreite
    int Ny = 8;  // number of zwischenräume
    int Dy; // value between 2 ticks
    int Dy0; // scale offset
    if (RadioButton1->Checked) // dB scale
    {
       Dy = db_scale; // value between 2 ticks dB (taken from callisto.cfg)
       Dy0 = Dy;
       sprintf(Label,"Relative power [dB]"); // y-label
    } else  // Digit scale
    {
       Dy = 32*adc_divider; // value between 2 ticks digits
       Dy0 = 0;
       sprintf(Label,"log(power) [digit]"); // y-label
    }
    pPlott->TextOut(ALeft+2,ATop+2,Label);

    float dy = (float)(BBottom-BTop)/(float)Ny; // pixels between 2 ticks
    int i = 0;
    int x1 = BLeft; int x2 = x1-5;
    float y;
    int w, h;
    do {
        y = (float)BTop+(float)i*dy;
        pPlott->MoveTo(x1,y);
        pPlott->LineTo(x2,y);
        sprintf(Label,"%5i",(Ny-i)*Dy-Dy0); // y-label
        w = pPlott->TextWidth(Label);
        h = pPlott->TextHeight(Label);
        pPlott->TextOut(BLeft-6-w,y-h/2,Label);
        i++;
    } while (i<=Ny);

    // x-ticks
    float dx = (float)(BRight-BLeft)/(float)rows;
    i = 0;
    int y1 = BBottom; int y2 = y1+5; int y3 = y2+4;
    float x;
    do {
        x = dx/2.0+(float)BLeft+(float)i*dx;
        pPlott->MoveTo(x,y1);
        pPlott->LineTo(x,y2);
        if (rows>100)
        {
            sprintf(Label,"%6.1f",freqinlist[i]); // x-label
            w = pPlott->TextWidth(Label);
            h = pPlott->TextHeight(Label);
            if ((i%10)==0) pPlott->LineTo(x,y3);
            if ((i%25)==0) pPlott->TextOut(x-w/2,y2+h/2,Label);
        } else
        if (rows>10)
        {
            sprintf(Label,"%6.1f",freqinlist[i]); // x-label
            w = pPlott->TextWidth(Label);
            h = pPlott->TextHeight(Label);
            if ((i%2)==0) pPlott->LineTo(x,y3);
            if ((i%5)==0) pPlott->TextOut(x-w/2,y2+h/2,Label);
        } else
        if (rows<=10)
        {
            sprintf(Label,"%6.1f",freqinlist[i]); // x-label
            w = pPlott->TextWidth(Label);
            h = pPlott->TextHeight(Label);
            pPlott->LineTo(x,y3);
            pPlott->TextOut(x-w/2,y2+h/2,Label);
        }
        i++;
    } while (i<rows);
    sprintf(Label,"Frequency [MHz]"); // y-label
    w = pPlott->TextWidth(Label);
    h = pPlott->TextHeight(Label);
    pPlott->TextOut((BLeft+BRight)/2-w/2,y3+h+2,Label);

    // title
    sprintf(Label,"Intensity in frequency domain.");
    pPlott->Font->Size=11;
    w = pPlott->TextWidth(Label);
    pPlott->TextOut(ARight-w-10,ATop+2,Label); // right aligned
    sprintf(st,"XY  %s, %s",frqfilename,titlecomment);
    Form1->Caption=st;

    // function-plott
    int y0 = BBottom;
    int x0 = BLeft;
    float gain = (float)(BBottom-BTop)/(float)(Ny*Dy);

    dx = (float)(BRight-BLeft)/(float)rows;
    pPlott->Pen->Width = 2;  // to prevent dark lines between spectrum lines

    float px;
    float py;
    int yL;
    px = x0 + dx/2; // restart left axis
    int p = 0;
    float dB;
    if ((RadioButton1->Checked))
      pPlott->Pen->Color = clLime;
    else
      pPlott->Pen->Color = clYellow;
    do
    {
        yL = XY_ringbuff[XY_out]; XY_out = (XY_out+1)%XYbuflen;

        if ((RadioButton1->Checked) && (!fixedBG)) // get actual background only once
            ref[p] = (float)yL;

        dB = ((float)yL - ref[p])*dbcal + Dy0;

        if ((RadioButton1->Checked) && (fixedBG))
          py = y0-gain*dB;
        else
          py = y0-gain*(float)yL;

        if (py > BBottom) py = y1; // keine dB < 0 anzeigen
        if (py <    BTop) py = BTop; // keine dB > max anzeigen

        pPlott->MoveTo(px,y0);
        if (!change)
           pPlott->LineTo(px,py);
        px = px+dx; // next position to the right

        if (abs((XY_in-1)-XY_out)%rows==0) break; // buffer probably empty
        p++;
   } while (1);
   fixedBG = true;
   change = false;
   if (!RadioButton1->Checked) // reset
     fixedBG = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm1::FormResize(TObject *Sender) // prevent too large images
{
   if (ClientWidth > Screen->Width/2)
       ClientWidth = Screen->Width/2;
   if (ClientHeight > Screen->Height/2)
       ClientHeight = Screen->Height/2;
}
//---------------------------------------------------------------------------

void __fastcall TForm1::RadioButton1Click(TObject *Sender)
{
   change = true;
}
//---------------------------------------------------------------------------

