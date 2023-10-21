/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: XY.h                                                   */
/*                                                                       */
/*  Revision: V1.1     Date: 29.11.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 2D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 05.05.2003
// Updated by: Chr. Monstein 29.11.2004 maxXYbuflen deleted due to *operations
//---------------------------------------------------------------------------

#ifndef XYH
#define XYH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>
//---------------------------------------------------------------------------
class TForm1 : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
    TImage *Image1;
   TRadioButton *RadioButton1;
   TRadioButton *RadioButton2;
        void __fastcall FormResize(TObject *Sender);
   void __fastcall RadioButton1Click(TObject *Sender);
private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
    __fastcall TForm1(TComponent* Owner);
    void __fastcall XY(void);

};
//---------------------------------------------------------------------------
extern PACKAGE TForm1 *Form1;
//---------------------------------------------------------------------------
#endif
