/*-----------------------------------------------------------------------*/
/*  (C) Copyright Institute of Astronomy ETHZ 8092 Zuerich Switzerland   */
/*-----------------------------------------------------------------------*/
/*  Programmname: Yt.h                                                   */
/*                                                                       */
/*  Revision: V1.1     Date: 29.11.2004    Autor: Chr. Monstein          */
/*                                                                       */
/*  Purpose: 2D-plott Radiospectrometer CALLISTO                         */
/*                                                                       */
/*  Compiler: BORLAND C++Builder 6                                       */
/*-----------------------------------------------------------------------*/

// Created by: Chr. Monstein 01.12.2003
// Updated by: Chr. Monstein 29.11.2004 maxBuflen deleted due to *YTbuflen
//---------------------------------------------------------------------------

#ifndef YtH
#define YtH
//---------------------------------------------------------------------------
#include <Classes.hpp>
#include <Controls.hpp>
#include <StdCtrls.hpp>
#include <Forms.hpp>
#include <ExtCtrls.hpp>
#include <Menus.hpp>

//---------------------------------------------------------------------------
class TForm3 : public TForm
{
__published:	// Von der IDE verwaltete Komponenten
    TImage *Image1;
    TListBox *ListBox1;
    void __fastcall Image1MouseDown(TObject *Sender, TMouseButton Button,
          TShiftState Shift, int X, int Y);
    void __fastcall ListBox1Click(TObject *Sender);
        void __fastcall FormResize(TObject *Sender);
private:	// Anwender-Deklarationen
public:		// Anwender-Deklarationen
    __fastcall TForm3(TComponent* Owner);
    void __fastcall YT(void);
    void __fastcall YTinit(void);

};
//---------------------------------------------------------------------------
extern PACKAGE TForm3 *Form3;
//---------------------------------------------------------------------------
#endif
