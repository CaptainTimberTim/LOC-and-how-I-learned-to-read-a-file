/* date = September 3rd 2020 8:17 am */
#ifndef _U_I__T_D_H
#define _U_I__T_D_H

#define BLINK_TIME 0.5f
#define BACKSPACE_CONTIUOUS_TIME 0.5f
#define BACKSPACE_CONTIUOUS_SPEED 0.05f

enum process_text_field_flag
{
    processTextField_TextChanged = 1<<0,
    processTextField_Confirmed   = 1<<1,
};
struct text_field_flag_result
{
    i32 Flag;
};

struct text_field
{
    v3 *TextColor;
    r32 Transparency;
    r32 ZValue;
    
    entry_id *Background;
    entry_id *LeftAlign;
    entry_id *Cursor;
    r32 dBlink;
    
    render_text Text;
    string_c TextString;
    string_c NoText;
    
    r32 dBackspacePress;
    r32 dBackspaceSpeed;
    
    b32 DoMouseHover;
    b32 IsActive;
};

internal text_field CreateTextField(renderer *Renderer, memory_bucket_container *Bucket, v2 Size, r32 ZValue, 
                                    u8 *EmptyFieldString, entry_id *Parent);
inline void Translate(text_field *TextField, v2 Translation);
inline void SetTextFieldActive(text_field *TextField, b32 MakeActive);
inline void UpdateTextField(renderer *Renderer, text_field *TextField);
internal text_field_flag_result ProcessTextField(renderer *Renderer, r32 dTime, input_info *Input, text_field *TextField);



struct slider
{
    entry_id *Background;
    entry_id *GrabThing;
    entry_id *Box;
    r32 OverhangP;
    r32 MaxSlidePix;
    r32 TranslationPercentage;
    
    r32 TotalSlideHeight;
    v2 MouseOffset;
    
    b32 SliderIsDragged;
};


internal void
AddDragable(drag_list *DragList, entry_id *Entry, drag_func_pointer OnDragStart, drag_func_pointer OnDragging, drag_func_pointer OnDragEnd);

#endif //_U_I__T_D_H
