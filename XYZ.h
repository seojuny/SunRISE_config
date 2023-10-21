/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: XYZ.h                                                  */
/*                                                                       */
/*  Revision: V1.1     Date: 29.11.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 3D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 01.12.2003 test with different XYZbuflen
// Updated by: Chr. Monstein 29.11.2004 maxXYZbuflen deleted due to *operations
//---------------------------------------------------------------------------

#ifndef XYZH
#define XYZH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
// XYZbuflen defines the x-axis of 3D-plot, sweeps=XYZbuflen/rows/2
// to save recources, keep XYZbufeln low, e.g. 10'000
// for good resolution in time make it bif, e.g. 50'000
// kepp it below 16bit (<56535) and keep it must be dividable by 2!

//---------------------------------------------------------------------------
class TForm2 : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
    TImage *Image1;
    TScrollBar *ScrollBar1;
    TScrollBar *ScrollBar2;
    TLabel *Label1;
    TLabel *Label2;
    TRadioButton *Original;
    TRadioButton *Smooth;
    TRadioButton *Fixed;
    void __fastcall ScrollBar1Change(TObject *Sender);
    void __fastcall ScrollBar2Change(TObject *Sender);
    void __fastcall FormResize(TObject *Sender);
    void __fastcall SmoothClick(TObject *Sender);
    void __fastcall FixedClick(TObject *Sender);
    void __fastcall OriginalClick(TObject *Sender);
private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
    __fastcall TForm2(TComponent* Owner);
    void __fastcall XYZ(void);
    long Color(float in);

};
//---------------------------------------------------------------------------
extern PACKAGE TForm2 *Form2;
//---------------------------------------------------------------------------
#endif
 