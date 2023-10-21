object Form2: TForm2
  Left = 548
  Top = 380
  Width = 484
  Height = 384
  HorzScrollBar.Visible = False
  VertScrollBar.Visible = False
  Caption = 'XYZ'
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -11
  Font.Name = 'MS Sans Serif'
  Font.Style = []
  Icon.Data = {
    0000010001001010100000000000280100001600000028000000100000002000
    00000100040000000000C0000000000000000000000000000000000000000000
    000000008000008000000080800080000000800080008080000080808000C0C0
    C0000000FF0000FF000000FFFF00FF000000FF00FF00FFFF0000FFFFFF00BBBB
    BBB5555555555BBBBB55555555555BBBBB5555555555555BBBBBBBBBBB5555BB
    BBBBBBBBBB555BBBBBB5B5555B55BBBB5555B5555555BBBB5555B5555555BBBB
    B555B5555555BB55BB55B5555555BB555BB5B5555555B55555BBB55555555555
    555BB555555555555555B5555555555555555555555555555555555555550000
    0000000000000000000000000000000000000000000000000000000000000000
    000000000000000000000000000000000000000000000000000000000000}
  OldCreateOrder = False
  OnResize = FormResize
  PixelsPerInch = 96
  TextHeight = 13
  object Image1: TImage
    Left = 0
    Top = 0
    Width = 1600
    Height = 1200
  end
  object Label1: TLabel
    Left = 286
    Top = 24
    Width = 12
    Height = 13
    Caption = 'L0'
  end
  object Label2: TLabel
    Left = 326
    Top = 24
    Width = 26
    Height = 13
    Caption = 'H255'
  end
  object ScrollBar1: TScrollBar
    Left = 326
    Top = 40
    Width = 25
    Height = 160
    Kind = sbVertical
    LargeChange = 10
    Max = 255
    PageSize = 0
    TabOrder = 0
    OnChange = ScrollBar1Change
  end
  object ScrollBar2: TScrollBar
    Left = 286
    Top = 40
    Width = 25
    Height = 160
    Kind = sbVertical
    LargeChange = 10
    Max = 255
    PageSize = 0
    Position = 255
    TabOrder = 1
    OnChange = ScrollBar2Change
  end
  object Original: TRadioButton
    Left = 280
    Top = 216
    Width = 81
    Height = 17
    Caption = 'Original'
    Checked = True
    Color = clInfoBk
    ParentColor = False
    TabOrder = 2
    TabStop = True
    OnClick = OriginalClick
  end
  object Smooth: TRadioButton
    Left = 280
    Top = 240
    Width = 81
    Height = 17
    Caption = 'Smooth'
    Color = clInfoBk
    ParentColor = False
    TabOrder = 3
    OnClick = SmoothClick
  end
  object Fixed: TRadioButton
    Left = 280
    Top = 264
    Width = 81
    Height = 17
    Caption = 'Fixed BG'
    Color = clInfoBk
    ParentColor = False
    TabOrder = 4
    OnClick = FixedClick
  end
end
