/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: mainunit.cpp                                           */
/*                                                                       */
/*  Revision: V1.1     Date: 05.04.2006    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: Hostcontroller Radiospectrometer CALLISTO                   */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 28.05.2003 progressbar included in mainform
// Updated by: Chr. Monstein 17.06.2003 all port related things shiftet to there modul
// Updated by: Chr. Monstein 23.09.2003 scheduler read process implemented
// Updated by: Chr. Monstein 16.10.2003 GPS, HygroWin and FPU exported
// Updated by: Chr. Monstein 27.11.2003 UploadThread included
// Updated by: Chr. Monstein 23.01.2004 new EraseBuffer
// Updated by: Chr. Monstein 12.02.2004 scheduler simplified
// Updated by: Chr. Monstein 05.12.2004 MyClock without paramter
// Updated by: Chr. Monstein 05.04.2006 MyGPS structure included
// Updated by: Chr. Monstein 16.04.2006 new light curves
// Updated by: Chr. Monstein 01.01.2010 more (5) light curves possible
//---------------------------------------------------------------------------
#ifndef mainunitH
#define mainunitH
//---------------------------------------------------------------------------
#include <vcl\Classes.hpp>
#include <vcl\Controls.hpp>
#include <vcl\StdCtrls.hpp>
#include <vcl\Forms.hpp>
#include <vcl\Menus.hpp>
#include <vcl\ExtCtrls.hpp>
#include <Mask.hpp>
#include <Buttons.hpp>
#include <Dialogs.hpp>
#include <ComCtrls.hpp>

#define STX 2

//---------------------------------------------------------------------------
class TMainForm : public TForm
{
__published:	// IDE-managed Components
    TTimer *Timer;
    TLabel *Label1;
    TLabel *Label5;

    TSaveDialog *SaveDialog1;
    TProgressBar *ProgressBar1;
    TButton *EEPROMButton;
    TOpenDialog *OpenDialog1;
    TButton *StartButton;
    TButton *StopButton;
    TButton *SpectrumButton2D;
    TButton *SpectrumButton3D;
    TPopupMenu *PopupMenu1;
    TMenuItem *FileView1;
    TMenuItem *Schedulercfg1;
    TMainMenu *MainMenu1;
    TMenuItem *Edit1;
    TLabel *Label20;
    TButton *Button5;
    TMemo *Receivermessages;
    TBevel *Bevel5;
    TBevel *Bevel6;
    TBevel *Bevel8;
    TLabel *Percent;
    TRadioButton *RadioButton1;
    TRadioButton *RadioButton2;
    TMenuItem *Help1;
    TPopupMenu *PopupMenu2;
    TMenuItem *OperatingManual1;
    TLabel *Label3;
    TButton *Overview;
    TTimer *Timer1;
    TTimer *Timer2;
    TMenuItem *test1;
    TMenuItem *DownloadLatestHardwareManual1;
    TMenuItem *GotoCallistoFolder1;
    TMenuItem *OnlineInfos1;
    TLabel *Label2;
        TMenuItem *DeviceManager1;
        TMenuItem *TaskManager1;
        TTimer *Timer3;
    void __fastcall TimerTimer(TObject *Sender);
    void __fastcall EEPROMButtonClick(TObject *Sender);
    void __fastcall StartButtonClick(TObject *Sender);
    void __fastcall StopButtonClick(TObject *Sender);
    void __fastcall SpectrumButton2DClick(TObject *Sender);
    void __fastcall SpectrumButton3DClick(TObject *Sender);
    void __fastcall FileView1Click(TObject *Sender);
    void __fastcall Schedulercfg1Click(TObject *Sender);
    void __fastcall Edit1Click(TObject *Sender);
    void __fastcall FormClose(TObject *Sender, TCloseAction &Action);
    void __fastcall Button5Click(TObject *Sender);
    void __fastcall RadioButton1Click(TObject *Sender);
    void __fastcall RadioButton2Click(TObject *Sender);
    void __fastcall Help1Click(TObject *Sender);
    void __fastcall OperatingManual1Click(TObject *Sender);
    void __fastcall Info1Click(TObject *Sender);
    void __fastcall OverviewClick(TObject *Sender);
    void __fastcall SaveLightcurves(float t, float M1, float M2, float M3, float M4, float M5, float M6, float M7, float M8, float M9, float M10);
    void __fastcall Timer1Timer(TObject *Sender);
    void __fastcall Timer2Timer(TObject *Sender);
    void __fastcall test1Click(TObject *Sender);
    void __fastcall DownloadLatestHardwareManual1Click(TObject *Sender);
    void __fastcall GotoCallistoFolder1Click(TObject *Sender);
    void __fastcall OnlineInfos1Click(TObject *Sender);
        void __fastcall DeviceManager1Click(TObject *Sender);
        void __fastcall TaskManager1Click(TObject *Sender);
        void __fastcall Timer3Timer(TObject *Sender);


private:	// User declarations
public:		// User declarations
	__fastcall TMainForm(TComponent* Owner);
    void __fastcall MyClock();
    void __fastcall EvalRX(void);
    void __fastcall ReadConfig(bool first);
    void __fastcall DisplayMmode(char mode);
    void __fastcall OpenLogging(void);
    void __fastcall Logging(char *par);
    void __fastcall CloseLogging(void);
    void __fastcall OpenOverview(void);
    void __fastcall SaveOverview(char *par);
    void __fastcall CloseOverview(void);
    void __fastcall EraseBuffer(void);  // Evaluate receiver data
    void __fastcall InfoUpdates(void);
    float __fastcall Digit2SFU(int ch); // adu -> flux
};

//---------------------------------------------------------------------------
extern TMainForm *MainForm;

typedef struct
{
  int hh, mm, ss; // schedul-time
  int FCx;    // scheduled Focuscodes for both states/tuner
  char mmode; // scheduled measurement - mode
  char fprog[15];
} Tscheduler;

typedef struct
{
    int Year;
    int Month;
    int Day;
    int Hour;
    int Minute;
    int Second;
    int MilliSec;
    float TimeSec; // Decimal time just 1 second precision
    float TimeMsec; // Decimal time full time including msec
    float Longitude, Latitude, Height;
    char LatCode, LonCode; // N/S, E,W
    int Sats;
} TGPSdata;

//---------------------------------------------------------------------------
#endif
