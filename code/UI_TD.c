#include "UI_TD.h"

internal text_field
CreateTextField(renderer *Renderer, memory_bucket_container *Bucket, v2 Size, r32 ZValue, u8 *EmptyFieldString, entry_id *Parent, v3 *TextColor, v3 *BGColor)
{
    text_field Result = {};
    
    Result.TextColor = TextColor;
    Result.Transparency = 0.25f;
    Result.ZValue    = ZValue;
    Result.NoText    = NewStaticStringCompound(EmptyFieldString);
    
    Result.Background = CreateRenderRect(Renderer, Size, Result.ZValue, BGColor, Parent);
    Get(Result.Background)->Render = false;
    Result.LeftAlign = CreateRenderRect(Renderer, V2(0), Result.ZValue-0.000001f, 
                                        TextColor, Result.Background);
    SetLocalPosition(Result.LeftAlign, V2(-(Size.x-4)/2.0f, 0));
    
    Result.Cursor = CreateRenderRect(Renderer, V2(2, 35), 
                                     Result.ZValue-0.00001f, Result.TextColor,
                                     Result.LeftAlign);
    SetLocalPosition(Result.Cursor, V2(4, 0));
    Get(Result.Cursor)->Render = false;
    
    Result.TextString = NewStringCompound(Bucket, 255);
    Result.DoMouseHover = true;
    
    return Result;
}

inline void
Translate(text_field *TextField, v2 Translation)
{
    Translate(TextField->Background, Translation);
}

inline void
SetTextFieldActive(text_field *TextField, b32 MakeActive)
{
    TextField->IsActive = MakeActive;
    Get(TextField->Background)->Render = MakeActive;
    Get(TextField->Cursor)->Render = MakeActive;
}

inline void
UpdateTextField(renderer *Renderer, text_field *TextField)
{
    TextField->dBlink = 0.0f;
    Get(TextField->Cursor)->Render = true;
    
    RemoveRenderText(&TextField->Text);
    
    CreateRenderText(Renderer, Renderer->FontInfo.MediumFont, &TextField->TextString, 
                     TextField->TextColor, &TextField->Text, TextField->ZValue-0.000001f, TextField->LeftAlign);
    Translate(&TextField->Text, V2(12, 10));
    SetPosition(TextField->Cursor, V2(TextField->Text.CurrentP.x + 10, GetPosition(TextField->Cursor).y));
    
    if(TextField->TextString.Pos == 0)
    {
        RemoveRenderText(&TextField->Text);
        CreateRenderText(Renderer, Renderer->FontInfo.MediumFont, &TextField->NoText, 
                         TextField->TextColor, &TextField->Text, TextField->ZValue-0.000001f, TextField->LeftAlign);
        SetTransparency(&TextField->Text, TextField->Transparency);
        Translate(&TextField->Text, V2(12, 10));
    }
}

internal text_field_flag_result
ProcessTextField(renderer *Renderer, r32 dTime, input_info *Input, text_field *TextField)
{
    text_field_flag_result Result = {};
    if(TextField->IsActive)
    {
        // Do cursor blinking
        if(TextField->dBlink >= 1.0f)
        {
            Get(TextField->Cursor)->Render = !Get(TextField->Cursor)->Render;
            TextField->dBlink = 0.0f;
        }
        else TextField->dBlink += dTime/BLINK_TIME;
        
        if(IsInRect(TextField->Background, Input->MouseP) && TextField->DoMouseHover)
        {
            SetTransparency(TextField->Background, TextField->Transparency);
            SetTransparency(TextField->Cursor, TextField->Transparency);
            if(TextField->TextString.Pos != 0) SetTransparency(&TextField->Text, TextField->Transparency);
        }
        else 
        {
            SetTransparency(TextField->Background, 1.0f);
            SetTransparency(TextField->Cursor, 1.0f);
            if(TextField->TextString.Pos != 0) SetTransparency(&TextField->Text, 1.0f);
        }
        
        b32 StringIsPasted = false;
        if((Input->Pressed[KEY_CONTROL_LEFT] || Input->Pressed[KEY_CONTROL_RIGHT]) && Input->KeyChange[KEY_V] == KeyDown)
            StringIsPasted = true;
        
        // Only process input when no modifier is pressed
        if(((!Input->Pressed[KEY_CONTROL_LEFT] || Input->Pressed[KEY_ALT_RIGHT]) && !Input->Pressed[KEY_CONTROL_RIGHT] &&
            !Input->Pressed[KEY_ALT_LEFT]) || StringIsPasted)
        {
            b32 TextChanged = false;
            
            if(StringIsPasted) // Process pasted symbols
            {
                string_c PastedString = NewStringCompound(&GlobalGameState.Bucket.Transient, 1500);
                if(TryGetClipboardText(&PastedString))
                {
                    if(PastedString.Pos > TextField->TextString.Length-TextField->TextString.Pos)
                    {
                        PastedString.Pos = TextField->TextString.Length-TextField->TextString.Pos;
                    }
                    AppendStringCompoundToCompound(&TextField->TextString, &PastedString);
                    TextChanged = true;
                }
                DeleteStringCompound(&GlobalGameState.Bucket.Transient, &PastedString);
            }
            else if(Input->CharCount > 0) // Process all symbols
            {
                For(Input->CharCount)
                {
                    if(TextField->TextString.Pos < TextField->TextString.Length)
                    {
                        AppendCharToCompound(&TextField->TextString, Input->Chars[It]);
                    }
                    else DebugLog(255, "Reached searchbar character limit.\n");
                }
                TextChanged = true;
            }
            
            if(Input->Pressed[KEY_BACKSPACE]) // Delete a symbol
            {
                if(TextField->TextString.Pos > 0)
                {
                    // If Input->KeyChange[KEY_BackSPACE] == KeyDown then fall through
                    if(TextField->dBackspacePress >= 1 ||
                       Input->KeyChange[KEY_BACKSPACE] == KeyDown) // If pressed longer, start after a while to delete more letters
                    {
                        if(TextField->dBackspaceSpeed >= 1 || 
                           Input->KeyChange[KEY_BACKSPACE] == KeyDown) // Letter deletion interval
                        {
                            TextField->dBackspaceSpeed = 0.0f;
                            
                            TextField->TextString.Pos -= 1;
                            TextChanged = true;
                        }
                        else TextField->dBackspaceSpeed += dTime/BACKSPACE_CONTIUOUS_SPEED;
                    }
                    else TextField->dBackspacePress += dTime/BACKSPACE_CONTIUOUS_TIME;
                }
            }
            else 
            {
                TextField->dBackspacePress = 0.0f;
                TextField->dBackspaceSpeed = 0.0f;
            }
            
            if(TextChanged)
            {
                UpdateTextField(Renderer, TextField);
                Result.Flag |= processTextField_TextChanged;
            }
            if(Input->KeyChange[KEY_ENTER] == KeyDown)
            {
                Result.Flag |= processTextField_Confirmed;
            }
        }
    }
    return Result;
}

inline r32
GetOverhang(slider *Slider)
{
    r32 Result = 0;
    
    Result = Slider->TotalSlideHeight - GetSize(Slider->Background).y;
    
    return Result;
}


inline void
Translate(slider *Slider, v2 T)
{
    Translate(Slider->Background, T);
}

internal void
UpdateVerticalSlideHeightChanged(renderer *Renderer, slider *Slider, r32 TotalSlideHeight)
{
    v2 TotalScale     = GetSize(Slider->Background);
    Slider->OverhangP = Clamp(TotalScale.y/TotalSlideHeight, 0.01f, 1.0f);
    Slider->TotalSlideHeight = TotalSlideHeight;
    
    v2 NewScale = {GetSize(Slider->GrabThing).x, Max(TotalScale.y*Slider->OverhangP, 5.0f)};
    SetSize(Slider->GrabThing, NewScale);
    Slider->MaxSlidePix = (TotalScale.y - NewScale.y)/2.0f;
    
    SetLocalPosition(Slider->GrabThing, V2(0.0f, TotalScale.y*0.5f - NewScale.y*0.5f));
}

internal void
OnSliderDragStart(renderer *Renderer, v2 AdjustedMouseP, entry_id *Dragable, void *Data)
{
    slider *Slider = (slider *)Data;
    
    Slider->MouseOffset = AdjustedMouseP - GetPosition(Slider->GrabThing);
}

internal void
UpdateSliderChange(slider *Slider, r32 NewYPosition)
{
    r32 BGYPos = GetPosition(Slider->Background).y;
    r32 NewY   = Clamp(NewYPosition, BGYPos - Slider->MaxSlidePix, BGYPos + Slider->MaxSlidePix);
    
    SetLocalPosition(Slider->GrabThing, V2(GetLocalPosition(Slider->GrabThing).x, NewY-BGYPos));
    
    r32 TotalSliderScale = GetSize(Slider->Background).y;
    r32 GrabThingSize    = GetSize(Slider->GrabThing).y;
    r32 RemainingScale   = TotalSliderScale-GrabThingSize;
    r32 TopPositionY     = BGYPos+Slider->MaxSlidePix;
    
    Slider->TranslationPercentage = SaveDiv((TopPositionY-NewY),(Slider->MaxSlidePix*2));
    if(Slider->TranslationPercentage > 0.999f) Slider->TranslationPercentage = 1;
    
    Slider->SliderIsDragged = true;
}

internal void
OnVerticalSliderDrag(renderer *Renderer, v2 AdjustedMouseP, entry_id *Dragable, void *Data)
{
    slider *Slider = (slider *)Data;
    
    r32 GrabThingHalfHeight  = GetSize(Slider->GrabThing).y/2.0f;
    if(Slider->MouseOffset.y < GrabThingHalfHeight && Slider->MouseOffset.y > -GrabThingHalfHeight) 
        AdjustedMouseP.y -= Slider->MouseOffset.y;
    
    UpdateSliderChange(Slider, AdjustedMouseP.y);
}

internal void
UpdateVerticalSliderGrabThingPosition(slider *Slider, r32 ChangeAmount)
{
    UpdateSliderChange(Slider, GetPosition(Slider->GrabThing).y+ChangeAmount);
}

internal void
AddDragable(drag_list *DragList, entry_id *Entry, 
            drag_func_pointer OnDragStart, drag_func_pointer OnDragging, drag_func_pointer OnDragEnd)
{
    Assert(DragList->Count < DRAGABLE_MAX_COUNT);
    DragList->Dragables[DragList->Count] = Entry;
    DragList->OnDragStart[DragList->Count] = OnDragStart;
    DragList->OnDragging[DragList->Count] = OnDragging;
    DragList->OnDragEnd[DragList->Count++] = OnDragEnd;
}

internal void
CreateVerticalSlider(renderer *Renderer, slider *SliderResult, v2 Size, r32 Depth, v3 *BGColor, v3 *GrabColor)
{
    
    SliderResult->Background = CreateRenderRect(Renderer, Size, Depth, BGColor);
    SliderResult->GrabThing  = CreateRenderRect(Renderer, V2(Size.x-4, 50), Depth-0.00001f, GrabColor, 
                                                SliderResult->Background);
    SetLocalPosition(SliderResult->GrabThing, V2(0, Size.y*0.5f - 25));
    
    
    UpdateVerticalSlideHeightChanged(Renderer, SliderResult, Size.y);
    OnVerticalSliderDrag(Renderer, V2(0, (r32)Renderer->Window.CurrentDim.Height), SliderResult->Background, SliderResult);
    SliderResult->SliderIsDragged = false;
    
    AddDragable(&Renderer->DragableList, SliderResult->Background, {OnSliderDragStart, SliderResult}, {OnVerticalSliderDrag, SliderResult}, {});
    
}

