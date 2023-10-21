/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: Yt.cpp                                                 */
/*                                                                       */
/*  Revision: V1.1     Date: 02.06.2005    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 2D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 01.12.2003
// Updated by: Chr. Monstein 29.11.2004 new function init
// Updated by: Chr. Monstein 13.12.2004 color equalized to XY
// Updated by: Chr. Monstein 02.06.2005 new locosc
// Updated by: Chr. Monstein 30.12.2009 new title
// Updated by: Chr. Monstein 09.01.2010 x-axis [sec], y-axis=auto [dB]
// Updated by: Chr. Monstein 23.03.2011 plot adapter to 8/10bit Callisto
// Updated by: Chr. Monstein 31.08.2012 repair bug in light curve calibration
//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include "Yt.h"
#include "XY.h"
#include "XYZ.h"
#include <stdio.h>
#include <math.h>
#include "mainunit.h"
#include "EEPROM.h"

//---------------------------------------------------------------------------
#pragma package(smart_init)
#pragma resource "*.dfm"
//---------------------------------------------------------------------------
TForm3 *Form3;
extern char frqfilename[80];       // import from mainunit.cpp
extern int rows;                   // import from mainunit.cpp
extern int nsps;                   // import from mainunit.cpp
extern long YTbuflen;              // import from mainunit.cpp
extern int adc_divider;            // import from mainunit.cpp
extern bool Callisto_8bit;         // import from mainunit.cpp
extern float detector_sens;        // import from mainunit.cpp
extern int db_scale;               // import from mainunit.cpp
extern char titlecomment[80];      // additional info on top of API

extern TTP testfrequency;          // import from EEPROM.cpp
extern float frequency[501];       // import from EEPROM.cpp
extern float freqinlist[501];      // import from EEPROM.cpp
extern float locosc;               // import from EEPROM.cpp

//---------------------------------------------------------------------------
int *Yt_ringbuff;
long Yt_in=0, Yt_out=0;

bool YTfirst=true;
//---------------------------------------------------------------------------
__fastcall TForm3::TForm3(TComponent* Owner)
    : TForm(Owner)
{
    ListBox1->Visible=false;
    //Form3->Left = Form1->Left + Form1->Width + 10;
    MainForm->Left = MainForm->Left;
    //Form3->Top  = Form1->Top;
    Form3->Top  = MainForm->Top + MainForm->Height + 5;
    //Form3->Height = Form1->Height + Form2->Height + 10;
    Form3->Height = MainForm->Height/2;
}
//---------------------------------------------------------------------------
void __fastcall TForm3::YTinit(void)
{
    char st[40];

    ListBox1->Clear();
    ListBox1->MultiSelect=false;

    for (int i=0; i<rows; i++)
    {
        sprintf(st,"[%04u]=%8.3fMHz",i+1,freqinlist[i]);
        ListBox1->Items->Add(st);
    }
    YTfirst = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm3::YT(void)
{
    // das Feld für Image1 muss >> sein (z.B. 1200x1600)
    // als das aktuelle Fenster (y.B. 500x300)
    // damit ausreichend gezoomt werden kann.
    char st[80];
    TCanvas *pPlott = Image1->Canvas;
    pPlott->Pen->Color=clBlack;

    int maxi = 0;
    int mini = 1024;

    if (YTfirst)
        YTinit();

    // Ausserer Rahmen (Bildflaeche)
    int ALeft   = 0;
    int ATop    = 0;
    int ARight  = ClientWidth;
    int ABottom = ClientHeight;

    TRect Rahmen = Rect(ALeft,ATop,ARight, ABottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color = clWhite;//clYellow;
    pPlott->FillRect(Rahmen);

    // Innerer (Plott)Rahmen
    int BLeft = 30; int BTop = 25; int BRight = ARight-20; int BBottom = ABottom-50;
    TRect Plottarea = Rect(BLeft,BTop,BRight, BBottom); // ALeft, ATop,  ARight,  ABottom
    pPlott->Brush->Color=clNavy;//clYellow;clNavy;//clBlack;
    pPlott->FillRect(Plottarea);
    pPlott->FrameRect(Plottarea);

    // y-ticks and y-label
    char Label[120];
    pPlott->Font->Size = 10;
    pPlott->Brush->Color = clWhite; // Schriftfarbe
    pPlott->Pen->Width = 1; // tickbreite
    long Yt_temp, p1 = 0;                   
    int yB;
    do // search for extreme values in light curve
    {
        Yt_temp = (Yt_in+p1)%YTbuflen;
        yB = Yt_ringbuff[Yt_temp];
        if (yB > 0)
        {
           if (yB > maxi) maxi = yB;
           if (yB < mini) mini = yB;
        }
    } while(p1++ < YTbuflen);

    int Ny = 8;  // number of zwischenräume
    while ((maxi-mini)<(64*adc_divider)) // range etwas vergrössern wegen digitalem Rauschen
    {
       mini--;
       maxi++;
    }
    if (((maxi-mini)%8)>0)
      mini++;

    //int Dy = (maxi - mini)/Ny;
    float ygain = (float)(BBottom-BTop)/(float)Ny; // pixels between 2 ticks
    int i = 0;
    int x1 = BLeft; int x2 = x1-5;
    float px, py;
    //float ddB = float(maxi-mini)/(adc_divider*8.0)*3.0; // (1 oder 4)*255D=2500mV, g=25.4mV/dB -> 3dB/8D
    float ddB = float(maxi-mini)/(adc_divider*256)*2500.0/detector_sens; // (1 oder 4)*255D=2500mV, g=25.4mV/dB -> 3dB/8D
    int w, h;
    do {
        py = (float)BTop+(float)i*ygain;
        pPlott->MoveTo(x1,py);
        pPlott->LineTo(x2,py);
        //sprintf(Label,"%5u",mini+(Ny-i)*Dy); // y-label
        sprintf(Label,"%5.0f",-ddB/2.0+float(Ny-i)*ddB/float(Ny)); // y-label
        w = pPlott->TextWidth(Label);
        h = pPlott->TextHeight(Label);
        pPlott->TextOut(BLeft-6-w,py-h/2,Label);
        i++;
    } while (i<=Ny);
    sprintf(Label,"Relative rf-power [dB]"); // y-label
    pPlott->TextOut(ALeft+2,ATop+2,Label);

    // x-ticks  
    float xgain=(float)(BRight-BLeft)/(float)YTbuflen;
    i = 0;
    int tx;
    int y1 = BBottom; int y2 = y1+5; int y3 = y2+4;
    do {
        px = (float)BLeft+(float)i*xgain;
        if ((i%(YTbuflen/50))==0)
        {
            pPlott->MoveTo(px,y1);
            pPlott->LineTo(px,y2);
        }
        if ((i%(YTbuflen/10))==0)
        {
            tx = (i-YTbuflen)/nsps;
            sprintf(Label,"%i",tx);
            w = pPlott->TextWidth(Label);
            h = pPlott->TextHeight(Label);
            pPlott->LineTo(px,y3);
            pPlott->TextOut(px-w/2,y2+h/2,Label);
        }
        i++;
    } while (i<=YTbuflen);
    sprintf(Label,"Time [sec]"); // y-label
    w = pPlott->TextWidth(Label);
    h = pPlott->TextHeight(Label);
    pPlott->TextOut((BLeft+BRight)/2-w/2,y3+h+2,Label);

    // title
    sprintf(Label,"Intensity in time domain.");
    pPlott->Font->Size = 11;
    w = pPlott->TextWidth(Label);
    pPlott->TextOut(ARight-w-10,ATop+2,Label); // right aligned

    // function-plott
    ygain = (float)(BBottom-BTop)/(maxi-mini);
    xgain = (float)(BRight-BLeft)/((float)YTbuflen+1);

    if (Yt_in>0)
    {
      pPlott->Font->Size = 9;
      Yt_temp = Yt_ringbuff[Yt_in-1];
      sprintf(Label,"Yt  f=%6.3fMHz, I=%uDigit, %s",
         freqinlist[testfrequency.poltp],Yt_temp,titlecomment); // y-label
      Form3->Caption = Label;
    }

    p1 = 0;
    static px_old = BRight, py_old = BTop;

    pPlott->Pen->Color=clYellow;
    do
    {
        Yt_temp = (Yt_in+p1)%YTbuflen;
        yB = Yt_ringbuff[Yt_temp];
        px = BLeft + ((float)p1*xgain);
        py = BBottom-ygain*(float(yB-mini));
        //if (py<BBottom) pPlott->Pixels[px][py] = clYellow;
        pPlott->MoveTo(px_old,py_old);
        if ((py < BBottom) && (py_old < BBottom) && (px > px_old)) // prevents vertical lines
           pPlott->LineTo(px,py);

        px_old = px;
        py_old = py;

    } while(p1++ < (YTbuflen-1));
}
//---------------------------------------------------------------------------
void __fastcall TForm3::Image1MouseDown(TObject *Sender,
      TMouseButton Button, TShiftState Shift, int X, int Y)
{
   ListBox1->Visible = true;
}
//---------------------------------------------------------------------------
void __fastcall TForm3::ListBox1Click(TObject *Sender)
{
   testfrequency.poltp = ListBox1->ItemIndex;
   ListBox1->Visible = false;
}
//---------------------------------------------------------------------------
void __fastcall TForm3::FormResize(TObject *Sender)  // prevent oversized images
{
   if (ClientWidth > Screen->Width*0.8)
       ClientWidth = Screen->Width*0.8;
   if (ClientHeight > Screen->Height*0.8)
       ClientHeight = Screen->Height*0.8;
}
//---------------------------------------------------------------------------

