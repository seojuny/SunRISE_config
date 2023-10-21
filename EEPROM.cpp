/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: EEPROM.cpp                                             */
/*                                                                       */
/*  Revision: V1.1     Date: 27.11.2003    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: EEPROM upload thread Radiospectrometer CALLISTO             */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 27.11.2003
// Updated by: Chr. Monstein 02.07.2004  slow down display (load)
// Updated by: Chr. Monstein 29.11.2004  restrict reading to '['-strings
// Updated by: Chr. Monstein 13.12.2004  error message for missing frq-file
// Updated by: Chr. Monstein 02.06.2005  locosc -> global value
// Updated by: Chr. Monstein 27.06.2005  sleep(1) -> sleep(4)
// Updated by: Chr. Monstein 27.06.2005  local oscillator frequency conversion abs for Lo's which are higher as the receiving band
// Updated by: Chr. Monstein 11.06.2009  read calibration file
// Updated by: Chr. Monstein 16.04.2009  new light curves
// Updated by: Chr. Monstein 04.08.2009  improvements
// Updated by: Chr. Monstein 01.01.2010  more (5) light curves possible
// Updated by: Chr. Monstein 09.01.2010  frequency file lower&upper case possible
// Updated by: Chr. Monstein 20.11.2010  allow autostart
// Updated by: Chr. Monstein 11.04.2011  new CB for CD1316LS/IV-3
// Updated by: Chr. Monstein 27.12.2014  format captial letters for frqflename

//---------------------------------------------------------------------------

#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <fstream.h>
#include <math.h>

#include "EEPROM.h"
#include "mainunit.h"
#include "RXRS232.H"
#include "Info.h"
#include "Yt.h"
#pragma package(smart_init)

//---------------------------------------------------------------------------

EEPROM *UploadThread;
bool handlefrq;         // event flag to read and upload frq-file
bool overwrite;         // shall the old frq-file be overwriten?
float frequency[501];   // max number of frequencies in one sweep for labeling 
float freqinlist[501];  // frequency which is in the frequency file e.g. 80.0 or 2045MHz
int fnumber;            // number of actual frequency file
float locosc = 0.0;     // local oscillator of front end downconverter  (MHz)

TTP testfrequency;      // test frequency structure (pointer and frequency)
Tcal CF[500];           // Calibration Factors
bool calibration = false;

Tolc OlcList[MaxNolc];  // table containing optional light curve infos
int Nolc;               // number of light curves
//---------------------------------------------------------------------------
extern char frqfilename[80];     // import from mainunit.cpp
extern char frqpath[120];        // import from mainunit.cpp
extern float low_band, mid_band; // import from mainunit.cpp, tuner barrier frequencies
extern int chargepump;           // import from mainunit.cpp, tuner configuration
extern int rows;                 // import from mainunit.cpp, number of Measurement Points Per Sweep
extern int nsps;                 // import from mainunit.cpp, Number of Sweeps Per Second
extern long columns;             // import from mainunit.cpp
extern long filetime;            // import from mainunit.cpp
extern AnsiString TimerSt;       // import from mainunit.cpp, to be logged...
extern AnsiString DatumSt;       // import from mainunit.cpp, to be logged...
extern bool eepromuploaded;      // import from mainunit.cpp
extern bool eepromCycleReady;    // import from mainunit.cpp
extern Byte *pszBuffer_RX1;      // import from mainunit.cpp
extern TWrite *RX_SerialThread;  // import from RXRS232.cpp, RX activities
extern bool YTfirst;             // import from Yt.cpp
extern int autostart;            // import from mainunit.cpp
extern bool Callisto_8bit;       // import from mainunit.cpp
extern float if_init;            // import from mainunit.cpp

//---------------------------------------------------------------------------
__fastcall EEPROM::EEPROM(bool CreateSuspended)
    : TThread(CreateSuspended)
{
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::Init()
{
    handlefrq = false;
    overwrite = false;

    testfrequency.poltp = 1;
    testfrequency.foltp = 1;

    Nolc = 0;
    for (int i = 0; i < MaxNolc; i++)
    {
       OlcList[i].channel = 0; // channel number in frequency file
       OlcList[i].count   = 0; // number of integrations per LC-pixel
       OlcList[i].MHz     = 0.0;
    }
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::Upload(bool upload)
{
    handlefrq = true;
    overwrite = upload;
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::Execute()
{
    while(1) // almost forever...
    {
        if(Terminated) return; // exit thread
        Sleep(20); // bisher 20
        StateMachine();
    }
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::Close()
{
    Terminate();
    WaitFor();
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::ReadCalibrationFile() // lese EXCEL/IDL parameter
{
    char buff[80], calfilename[40], frequencyfilepath[240];
    int i = 0;

    sscanf(StrUpper(frqfilename),"FRQ%05u.CFG",&fnumber);
    sprintf(calfilename,"CAL%05u.prn",fnumber);
    sprintf(frequencyfilepath,"%s",frqpath);
    StrCat(frequencyfilepath,calfilename);
    if (FileExists(frequencyfilepath))
    {
        ifstream infile(frequencyfilepath); // complete path
        infile.getline(buff,80); // read and forget header
        while (infile)
        {
            infile.getline(buff,80);
            sscanf(buff,"%u,%f,%f,%f,%f,%f",&CF[i].channel,&CF[i].f_MHz,
                  &CF[i].a,&CF[i].b,&CF[i].cf,&CF[i].Tb);
            i++;
        }
        infile.close();
        calibration = true;
        sprintf(buff,"$HST:File %s read",frequencyfilepath);
        MainForm->Logging(buff);
        sprintf(buff,"Filename calibration file: %s",calfilename);
        Form7->Label14->Caption=buff;
    } else
    {
        calibration = false;
        sprintf(buff,"$HST:No calibration file found",frequencyfilepath);
        MainForm->Logging(buff);
        sprintf(buff,"Filename calibration file: not available");
        Form7->Label14->Caption = buff;
    }
}
//---------------------------------------------------------------------------
void __fastcall EEPROM::StateMachine() // every 20msec
{
  static int state = 0;
  static char buff[240], tmp[240], frequencyfilepath[240];
  char txt[40];
  AnsiString st;
  char target[20]  ="Phoenix-2";
  int testpos = 0;
  float percf;
  int perc;
  int adr; // counts frequency entries within sweep
  int olcp = 0;

  //float if_init = (10.70+27.0); // IF-frequency #1 = IF-frequency #2 + Quarz-frequency [MHz]
  float synthesizer_resolution = 0.0625;  // MHz

  long divider;
  Byte db1, db2; // divider byte 1 and 2
  Byte cb, bb;  // address byte, control byte 1 and 2

  switch (state)
  {
    case 0: // idle
    {
        if (handlefrq)
        {
            eepromuploaded = false;
            handlefrq = false;
            Nolc = 0; // no optional light curve up to now
            state++;
        }
        break;
    }
    case 1: // preamble
    {
        MainForm->StartButton->Enabled = false; // prevents start while loading frq*
        sscanf(StrUpper(frqfilename),"FRQ%05u.CFG",&fnumber);
        sprintf(frequencyfilepath,"%s",frqpath);
        StrCat(frequencyfilepath,frqfilename);
        ifstream infile(frequencyfilepath); // complete path
        if (infile)
        {
          for (int i = 0; i < MaxNolc; i++)
          {
             OlcList[i].channel = 0; // channel number in frequency file
             OlcList[i].count   = 0; // number of integrations per LC-pixel
             OlcList[i].MHz     = 0.0;
          }
          
          while (infile)
          {
              infile.getline(buff,80);
              st = LowerCase(AnsiString(buff));
              if (buff[0]=='[')
              {
                if (strstr(st.c_str(),"[target]="))                           sscanf(buff,"[target]=%s",&target);
                if (strstr(st.c_str(),"[on_line_testpoint_number]="))         sscanf(buff,"[on_line_testpoint_number]=%u",&testpos);
                if (strstr(st.c_str(),"[number_of_measurements_per_sweep]=")) sscanf(buff,"[number_of_measurements_per_sweep]=%u",&rows);
                if (strstr(st.c_str(),"[number_of_sweeps_per_second]="))      sscanf(buff,"[number_of_sweeps_per_second]=%u",&nsps);
                if (strstr(st.c_str(),"[external_lo]="))                      sscanf(buff,"[external_lo]=%f",&locosc);

                if (overwrite)
                {
                  for (int j=1; j<=rows; j++)
                  {
                        sprintf(tmp,"[%04u]=",j);  // create identifier [9999]=
                        if (strstr(st.c_str(),tmp)) // is something comparable to see?
                        {
                            sprintf(tmp,"[%04u]=%%f,%%i",j); // create format-string [9999]=%f
                            adr = j-1;
                            sscanf(buff,tmp,&frequency[adr],&olcp); // get frequency
                            if (olcp > 0)
                            {
                                OlcList[Nolc].channel = adr; // channel number in frequency file
                                OlcList[Nolc].count   = olcp; // number of integrations per LC-pixel
                                OlcList[Nolc].MHz     = frequency[adr]; // frequenczy of light curve
                                Nolc++; // one more light curve
                                if (Nolc > MaxNolc) Nolc = MaxNolc; // limitation of light curves
                            }
                            freqinlist[adr] = frequency[adr];
                            frequency[adr] = fabs(frequency[adr]-locosc); // correct for external local oscillator, f(fx)=f(in)-f(LO)

                             // estimate oscillator divider bytes
                            divider = (unsigned int)( (frequency[adr] + if_init)/synthesizer_resolution); // MHz->KHz->channel
                            db1 = (char) (divider / 256); 	// evaluate first data byte
                            db2 = (char) (divider % 256); 	// evaluate second data byte

                            if(Callisto_8bit)
                            {
                              if (chargepump > 0)
                                  cb = 128 + 64 + 4 + 2; // tuner control byte with CP=on
                              else
                                  cb = 128      + 4 + 2; // evaluate tuner control byte with CP=off
                            } else
                                  cb = 128 + 64 + 4 + 2; // tuner control byte CD1316LS/IV-3, IHP

                            // bb setting according to oscillator frequency (band select)
                            if (frequency[adr] <= low_band)   // low band 51-171MHz = 00000001
                                bb = 1;
                            else
                            {
                                if (frequency[adr] <= mid_band) // mid band 178-450MHz = 00000010
                                    bb = 2;
                                else 			// high band 458-858MHz = 00000100
                                    bb = 4;
                            }

                            sprintf(tmp,"FE%u,%03u,%03u,%03u,%03u\r", adr+1,db1,db2,cb,bb); // create eeprom-command

                            if (testpos==j)
                            {
                                testfrequency.poltp = adr;
                                testfrequency.foltp = frequency[adr];
                            }

                            percf = (float)j/(float)rows*100;
                            MainForm->ProgressBar1->Position = percf;

                            if (MainForm->Visible)
                            {
                              sprintf(txt,"Yield =%4.0f%%",percf);
                              int k = ((int)percf)%4;

                              switch(k) // draw ascii symbol to show eeprom-progress
                              {
                                  case 0:
                                  {
                                      MainForm->Percent->Caption = txt;
                                      MainForm->EEPROMButton->Caption="Loading - ";
                                      break;
                                  }
                                  case 1:
                                  {
                                      MainForm->EEPROMButton->Caption="Loading \\ ";
                                      break;
                                  }
                                  case 2:
                                  {
                                      MainForm->EEPROMButton->Caption="Loading | ";
                                      break;
                                  }
                                  case 3:
                                  {
                                      MainForm->EEPROMButton->Caption="Loading / ";
                                      break;
                                  }
                              } // end switch
                            } // end visible

                            RX_SerialThread->Send(tmp);
                            Sleep(6); // nom. 6, wait a bit (depends on machine)
                            if (eepromCycleReady==true)
                                eepromCycleReady = false;
                            else
                                Sleep(6); // 6, bisher 20msec, TBD
                        } // end if (keyword)
                  } // end for
                } // end upload
              } // end if '['
          } // eof
        } // if (infile)
        else // no frequencyfile available
        {
            sprintf(target,"Unknown");
            testpos = 1; // unknown
            rows = 1; // unknown, just to prevent div zero
            nsps = 1; // unknown
            locosc = 0.0;  // unknown
            testfrequency.poltp = 0; // pointer
            testfrequency.foltp = 100.0; // unknown
            Application->MessageBox("Frequencyfile not available", "Warning!", MB_OK);

        }
        infile.close();
        state++;
        break;
    }
    case 2: // post amble
    {

        sprintf(tmp,"$HST:File %s read",frequencyfilepath);
        MainForm->Logging(tmp);

        ReadCalibrationFile(); // test only

        sprintf(tmp,"$HST:Plot buffer erased...");
        MainForm->Logging(tmp);
        MainForm->EraseBuffer();

        MainForm->Logging("$HST:Parametrisation RCU");
        RX_SerialThread->Parametrisation();
        columns = nsps*filetime; // total number of sweeps per file

        delete [] pszBuffer_RX1;
        pszBuffer_RX1 = new Byte[rows*columns];

        MainForm->InfoUpdates(); // Update Infopanel

        YTfirst = true; // to update frequency list in Yt
        eepromuploaded = true; // now START is allowed ...
        state = 0;

        MainForm->EEPROMButton->Caption="Select frequencyfile";
        MainForm->StartButton->Enabled = true; // frq is loaded, allow start
        for (int i=0; i<501; i++)
           frequency[i]=0;  // clear buffer

        if (autostart > 0)
        {
            MainForm->Timer2->Enabled = true;
            autostart = 0;
        }

        break;
    }
  } // end switch(state)
}
//---------------------------------------------------------------------------

