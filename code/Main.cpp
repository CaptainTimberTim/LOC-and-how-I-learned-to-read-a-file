#include <stdio.h>
#include <stdint.h>
#include <windows.h>

#define internal        static
#define local_persist   static 
#define global_variable static

typedef int8_t  i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32     b32;

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float  r32;
typedef double r64;

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

// NOTE:: For loop simplification macro. first param: Count until, second param(optional): Iterater name prefix ...It
#define For(until, ...) \
for(u32 (__VA_ARGS__##It) = 0; \
(__VA_ARGS__##It) < (until); \
++(__VA_ARGS__##It))
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define Combine1(X, Y) X##Y
#define Combine(X, Y) Combine1(X, Y)

#define DebugLog(Count, Text, ...) { \
char Combine(B, __LINE__)[Count]; \
sprintf_s(Combine(B, __LINE__), Text, __VA_ARGS__);\
OutputDebugStringA(Combine(B, __LINE__)); \
} 

#if DEBUG_TD
#define Assert(Expression)  {            \
if(!(Expression))                        \
{                                        \
DebugLog(1000, "Assert fired at:\nLine: %i\nFile: %s\n", __LINE__, __FILE__); \
*(int *)0 = 0;                       \
} }                                       
#else
#define Assert(Expression)
#endif
#define InvalidCodePath    Assert(!"InvalidCodePath")
#define NotImplemented     Assert(!"NotImplementedYet")
#define InvalidDefaultCase default: {Assert(false)}


// TODO:: Implement STBI_MALLOC, STBI_REALLOC and STBI_FREE!
#define STB_IMAGE_IMPLEMENTATION
//#define STBI_FAILURE_USERMSG
#define STBI_NO_FAILURE_STRINGS
#include "STB_Image.h"
#define STB_TRUETYPE_IMPLEMENTATION
#include "STB_Truetype.h"


struct time_management
{
    i32 GameHz;
    r32 GoalFrameRate;
    r32 dTime;
    r64 GameTime;
    r32 CurrentTimeSpeed;
};
internal b32 TryGetClipboardText(struct string_compound *String);

#include "Input_TD.cpp"
#include "String_TD.h"
#include "StandardUtilities_TD.cpp"
#include "Threading_TD.c"
#include "Math_TD.cpp"
#include "FileUtilities_TD.cpp"
#include "GL_TD.c"

#if DEBUG_TD
#define STBI_ASSERT(x) if(!(x)) {*(int *)0 = 0;}
#else
#define STBI_ASSERT(x)
#endif

global_variable game_state GlobalGameState;

global_variable i32 GlobalMinWindowWidth  = 653;
global_variable i32 GlobalMinWindowHeight = 250;

#include "UI_TD.c"
#include "Renderer_TD.cpp"
#include "GameBasics_TD.cpp"

global_variable b32 IsRunning;
HCURSOR ArrowCursor = 0;
HCURSOR DragCursor = 0;

global_variable ui_holder UIHolder = {};

internal void
WindowGotResized(game_state *GameState)
{
    if(GameState->Renderer.Window.GotResized)
    {
        renderer *Renderer = &GameState->Renderer;
        PerformScreenTransform(Renderer);
        
        r32 WWidth = (r32)Renderer->Window.FixedDim.Width;
        r32 Height = 0;
        if(UIHolder.LOCRT->Count > 0)
        {
            Height = UIHolder.LOCRT->CurrentP.y- UIHolder.LOCRT->StartP.y;
            SetLocalPosition(UIHolder.FileNamesRT, V2(0));
            SetLocalPosition(UIHolder.LOCTotalRT, V2(0));
            SetLocalPosition(UIHolder.LOCRT, V2(0));
        }
        else 
        {
            Height = GetSize(UIHolder.Slider->Background).y;
        }
        UpdateVerticalSlideHeightChanged(Renderer, UIHolder.Slider, Height);
        
        Renderer->Window.GotResized = false;
    }
    
}

internal LRESULT CALLBACK
WindowCallback(HWND Window,
               UINT Message,
               WPARAM WParam,
               LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_SIZE: 
        {
            if(IsRunning && WParam != 1) 
            {
                window_info *WinInfo = &GlobalGameState.Renderer.Window;
                RECT Rect = {};
                if(GetClientRect(Window, &Rect))
                {
                    WinInfo->CurrentDim.Width  = Rect.right - Rect.left;
                    WinInfo->CurrentDim.Height = Rect.bottom - Rect.top;
                    
                    RECT WRect = {};
                    GetWindowRect(Window, &WRect);
                    i32 WWidth  = WRect.right - WRect.left;
                    i32 WHeight = WRect.bottom - WRect.top;
                    if(WWidth < GlobalMinWindowWidth || WHeight < GlobalMinWindowHeight)
                    {
                        i32 NewWidth  = (WWidth < GlobalMinWindowWidth) ? GlobalMinWindowWidth : WWidth;
                        i32 NewHeight = (WHeight < GlobalMinWindowHeight) ? GlobalMinWindowHeight : WHeight;
                        
                        if(SetWindowPos(Window, HWND_TOP, WRect.left, WRect.top, NewWidth, NewHeight, 0))
                        {
                            // Stopped from making window too small
                        }
                    }
                }
                WinInfo->CurrentAspect = (r32)WinInfo->CurrentDim.Width/(r32)WinInfo->CurrentDim.Height;
                WinInfo->CurrentReverseAspect = (r32)WinInfo->CurrentDim.Height/(r32)WinInfo->CurrentDim.Width;
                
                GlobalGameState.Renderer.Window.GotResized = true;
                GlobalGameState.Renderer.Rerender = true;
                ReshapeGLWindow(&GlobalGameState.Renderer);
                WindowGotResized(&GlobalGameState);
                
                HDC DeviceContext = GetDC(Window);
                DisplayBufferInWindow(DeviceContext, &GlobalGameState.Renderer);
                ReleaseDC(Window, DeviceContext);
            }
        } break;
        case WM_CLOSE: { IsRunning = false; } break;
        case WM_ACTIVATEAPP: {} break;
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {} break;
        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            DisplayBufferInWindow(DeviceContext, &GlobalGameState.Renderer);
            EndPaint(Window, &Paint);
            ReleaseDC(Window, DeviceContext);
        } break;
        case WM_SETCURSOR: 
        {
            if (GlobalGameState.CursorState == cursorState_Arrow) 
                Result = DefWindowProcA(Window, Message, WParam, LParam);
            else if(GlobalGameState.CursorState == cursorState_Drag) 
                SetCursor(DragCursor);
        } break;
        case WM_SETFOCUS:
        {
            HandleFocusRegain(&GlobalGameState.Input);
        } break;
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
ProcessPendingMessages(input_info *Input, HWND Window)
{
    b32 PrevMouseMove = Input->_MouseMoved;
    ResetKeys(Input);
    
    MSG Message = {};
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT: { IsRunning = false; } break;
            case WM_CHAR: 
            {
                UpdateTypedCharacters(Input, (u8)Message.wParam);
            } break;
            case WM_SYSKEYDOWN: 
            case WM_SYSKEYUP: 
            case WM_KEYDOWN: 
            case WM_KEYUP:
            {
                u32 Scancode = (Message.lParam & 0x00ff0000) >> 16;
                i32 Extended  = (Message.lParam & 0x01000000) != 0;
                
                switch (Message.wParam) {
                    case VK_SHIFT:
                    {
                        u32 NewKeyCode = MapVirtualKey(Scancode, MAPVK_VSC_TO_VK_EX);
                        UpdateKeyChange(Input, NewKeyCode, Message.lParam);
                    } break;
                    
                    case VK_CONTROL:
                    {
                        u32 NewKeyCode = Extended ? VK_RCONTROL : VK_LCONTROL;
                        UpdateKeyChange(Input, NewKeyCode, Message.lParam);
                    } break;
                    
                    case VK_MENU:
                    {
                        u32 NewKeyCode = Extended ? VK_RMENU : VK_LMENU;
                        UpdateKeyChange(Input, NewKeyCode, Message.lParam);
                    } break;
                    
                    default:
                    {
                        if(!UpdateKeyChange(Input, (u32)Message.wParam, Message.lParam))
                        {
                            // If we don't process this message, try translating it to WM_CHAR
                            TranslateMessage(&Message);
                            DispatchMessage(&Message);
                        }
                    } break;    
                }
            } break;
            case WM_MOUSEMOVE:
            {
                Input->MouseP.x = (u16) Message.lParam; 
                Input->MouseP.y = (u16)(Message.lParam >> 16);
                Input->MouseP.y = GlobalGameState.Renderer.Window.CurrentDim.Height - Input->MouseP.y;
                
                if(!PrevMouseMove)
                {
                    if(Input->Pressed[KEY_LMB] && GetAsyncKeyState(VK_LBUTTON) == 0)
                        UpdateSingleKey(Input, KEY_LMB, true, false);
                    if(Input->Pressed[KEY_RMB] && GetAsyncKeyState(VK_RBUTTON) == 0) 
                        UpdateSingleKey(Input, KEY_RMB, true, false);
                    if(Input->Pressed[KEY_MMB] && GetAsyncKeyState(VK_MBUTTON) == 0) 
                        UpdateSingleKey(Input, KEY_MMB, true, false);
                }
                Input->_MouseMoved = true;
            } break;
            case WM_MOUSEWHEEL:
            {
                i32 Modifiers = GET_KEYSTATE_WPARAM(Message.wParam)/4;
                i32 ZDelta    = GET_WHEEL_DELTA_WPARAM(Message.wParam);
                
                i32 WheelPlus = (Modifiers == 2) ? 1000 : 1 + Modifiers*3;
                Input->WheelAmount += ((ZDelta/30)*-WheelPlus)*10;
            } break;
            case WM_LBUTTONDOWN:
            {
                UpdateSingleKey(Input, KEY_LMB, false, true); 
            } break;
            case WM_LBUTTONUP:
            {
                UpdateSingleKey(Input, KEY_LMB, true, false); 
            } break;
            case WM_RBUTTONDOWN:
            {
                UpdateSingleKey(Input, KEY_RMB, false, true); 
            } break;
            case WM_RBUTTONUP:
            {
                UpdateSingleKey(Input, KEY_RMB, false, true); 
            } break;
            case WM_MBUTTONDOWN:
            {
                UpdateSingleKey(Input, KEY_MMB, false, true); 
            } break;
            case WM_MBUTTONUP:
            {
                UpdateSingleKey(Input, KEY_MMB, false, true); 
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessage(&Message);
            } break;
        }
    }
}

internal b32
TryGetClipboardText(string_c *String)
{
    b32 Result = false;
    Assert(String);
    
    // Try opening the clipboard
    if (OpenClipboard(0))
    {
        // Get handle of clipboard object for ANSI text
        HANDLE Data = GetClipboardData(CF_TEXT);
        if (Data)
        {
            // Lock the handle to get the actual text pointer
            u8 *Text = (u8 *)GlobalLock(Data);
            if (Text)
            {
                Result = true;
                
                if(StringLength(Text) >= String->Length) Text[String->Length-1] = 0;
                else AppendStringToCompound(String, Text);
                
                // Release the lock
                GlobalUnlock(Data);
                
                // Release the clipboard
                CloseClipboard();
            }
        }
    }
    return Result;
}

inline void
GetWindowDimensions(HWND Window, v2i *Dim)
{
    PAINTSTRUCT Paint;
    HDC DeviceContext = BeginPaint(Window, &Paint);
    Dim->x = Paint.rcPaint.right - Paint.rcPaint.left; 
    Dim->y = Paint.rcPaint.bottom - Paint.rcPaint.top;
    EndPaint(Window, &Paint);
    ReleaseDC(Window, DeviceContext);
}

inline LARGE_INTEGER
GetWallClock()
{
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

inline r32
GetSecondsElapsed(i64 PerfCountFrequency, LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart) / (r32)PerfCountFrequency);
    return(Result);
}

#ifdef CONSOLE_APP
int main(int argv, char** arcs) 
#else
i32 CALLBACK 
WinMain(HINSTANCE Instance, 
        HINSTANCE PrevInstance, 
        LPSTR CmdLine, 
        i32 ShowCmd)
#endif
{
    // Hey, baby! Check out the nil-value _I'm_ dereferencing.
    
    ArrowCursor = LoadCursor(0, IDC_ARROW);
    DragCursor = LoadCursor(0, IDC_SIZEWE);
    
    WNDCLASSA WindowClass = {};
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = WindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = ArrowCursor;
    WindowClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    WindowClass.lpszClassName = "MPlay3ClassName";
    
    if(RegisterClassA(&WindowClass))
    {
        u32 InitialWindowWidth  = 1200;
        u32 InitialWindowHeight = 800;
        HWND Window = CreateWindowExA(0, WindowClass.lpszClassName, "LOC", 
                                      WS_OVERLAPPEDWINDOW|WS_VISIBLE, CW_USEDEFAULT, 
                                      CW_USEDEFAULT, InitialWindowWidth, InitialWindowHeight, 
                                      0, 0, Instance, 0);
        
        if(Window)
        {
            HDC DeviceContext = GetDC(Window);
            InitOpenGL(Window);
            
            // Initializing GameState
            game_state *GameState = &GlobalGameState;
            *GameState = {};
            GameState->DataPath = NewStaticStringCompound("..\\data\\");
            GameState->Time.GameHz           = 60; // TODO:: Get monitor refresh rate!?
            GameState->Time.GoalFrameRate    = 1.0f/GameState->Time.GameHz;
            GameState->Time.dTime            = 0.0f;
            GameState->Time.GameTime         = 0.0f;
            GameState->Time.CurrentTimeSpeed = 1.0f;
            GameState->Input                 = {};
            input_info *Input = &GameState->Input;
            
            // Initializing clock
            LARGE_INTEGER PerfCountFrequencyResult;
            QueryPerformanceFrequency(&PerfCountFrequencyResult);
            i64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;
            LARGE_INTEGER PrevCycleCount = GetWallClock();
            LARGE_INTEGER FlipWallClock  = GetWallClock();
            
            SetRandomSeed(FlipWallClock.QuadPart);
            
            // Initializing Allocator
            GameState->Bucket = {};
            if(!CreateBucketAllocator(&GameState->Bucket, Gigabytes(2), Megabytes(500))) return -1;
            if(!CreateBucketAllocator(&GameState->SoundThreadBucket, Megabytes(1), Megabytes(250))) return -1;
            u8 BucketStatus[100];
            BucketAllocatorFillStatus(&GameState->Bucket, BucketStatus);
            printf("%s", BucketStatus);
            
            // ********************************************
            // Threading***********************************
            // ********************************************
            
            HANDLE JobHandles[THREAD_COUNT];
            circular_job_queue *JobQueue = &GameState->JobQueue;
            *JobQueue = {};
            job_thread_info JobInfos[THREAD_COUNT];
            InitializeJobThreads(&GameState->Bucket, JobHandles, JobQueue, JobInfos);
            
            sound_thread_data SoundThreadData = {};
            
            renderer *Renderer = &GameState->Renderer;
            *Renderer = InitializeRenderer(GameState, Window);
            
            ReshapeGLWindow(&GameState->Renderer);
            b32 *ReRender = &Renderer->Rerender;
            
            
            
            // ********************************************
            // UI rendering stuff   ***********************
            // ********************************************
            r32 WWidth = (r32)Renderer->Window.FixedDim.Width;
            r32 WMid    = WWidth*0.5f;
            r32 WHeight = (r32)Renderer->Window.FixedDim.Height;
            
            Renderer->FontInfo.BigFont    = InitSTBBakeFont(GameState, 75);
            Renderer->FontInfo.MediumFont = InitSTBBakeFont(GameState, 50);
            Renderer->FontInfo.SmallFont  = InitSTBBakeFont(GameState, 25);
            
            
            
            string_c PrevFolderPath = TryLoadSettingsFile(GameState);
            string_c SearchFolder = NewStringCompound(&GameState->Bucket.Fixed, 500);
            AppendStringCompoundToCompound(&SearchFolder, &PrevFolderPath);
            
            v3 BGColor = {0.65f, 0.75f, 0.65f};
            v3 BGColorMuted = {0.75f, 0.85f, 0.75f};
            v3 TextColor = {0.15f, 0.15f, 0.15f};
            v3 UIBGColor   = {0.35f, 0.5f, 0.35f};
            v3 UIBGMutedColor = {0.45f, 0.55f, 0.45f};
            v2 TFSize    = {80000, 50};
            
            rect TotalBG = {{-WWidth/2, -WHeight/2},{WWidth/2, WHeight/2}};
            entry_id *BackgroundRect = CreateRenderRect(Renderer, TotalBG, 0.9f, 0, &BGColor);
            SetPosition(BackgroundRect, V2(WWidth/2, WHeight/2));
            TransformWithScreen(&Renderer->TransformList, BackgroundRect, fixedTo_Center, scaleAxis_XY);
            
            rect UpperBGRect = {{-WWidth/2, -45},{WWidth/2, 45}};
            entry_id *UpperBackground = CreateRenderRect(Renderer, UpperBGRect, 0.0f, 0, &BGColor);
            Translate(UpperBackground, V2(WWidth/2, WHeight-45));
            TransformWithScreen(&Renderer->TransformList, UpperBackground, fixedTo_TopCenter, scaleAxis_X);
            
            rect LowerBGRect = {{-WWidth/2, -15},{WWidth/2, 15}};
            entry_id *LowerBackground = CreateRenderRect(Renderer, LowerBGRect, 0.0f, 0, &BGColor);
            Translate(LowerBackground, V2(WWidth/2, 15));
            TransformWithScreen(&Renderer->TransformList, LowerBackground, fixedTo_BottomCenter, scaleAxis_X);
            
            rect RightBGRect = {{-40, -WHeight/2},{40, WHeight/2}};
            entry_id *RightBackground = CreateRenderRect(Renderer, RightBGRect, 0.0f, 0, &BGColor);
            Translate(RightBackground, V2(WWidth - 40, WHeight/2));
            TransformWithScreen(&Renderer->TransformList, RightBackground, fixedTo_RightCenter, scaleAxis_Y);
            
            text_field TextField = CreateTextField(Renderer, &GameState->Bucket.Fixed, TFSize, -0.1f, 
                                                   (u8 *)"Code path...", 0, &TextColor, &UIBGColor);
            Translate(&TextField, V2(TFSize.x*0.5f+25, WHeight-TFSize.y*0.5f - 25));
            SetTextFieldActive(&TextField, true);
            TextField.DoMouseHover = false;
            TranslateWithScreen(&Renderer->TransformList, TextField.Background, fixedTo_TopLeft);
            AppendStringCompoundToCompound(&TextField.TextString, &SearchFolder);
            UpdateTextField(Renderer, &TextField);
            
            entry_id *Hider = CreateRenderRect(Renderer, V2(80, TFSize.y), -0.1001f, &BGColor, 0);
            Translate(Hider, V2(WWidth-40, GetLocalPosition(TextField.Background).y));
            TranslateWithScreen(&Renderer->TransformList, Hider, fixedTo_TopRight);
            
            // BUTTON **********************
            Renderer->ButtonBase = LoadImage_STB((u8 *)"..\\data\\Buttons\\PlayPause.png"); 
            Renderer->ButtonBaseID = CreateGLTexture(Renderer->ButtonBase);
            Renderer->ButtonIcons[Renderer->ButtonIconCount] = LoadImage_STB((u8 *)"..\\data\\Buttons\\Confirm_Icon.png"); 
            u32 ConfirmID = CreateGLTexture(Renderer->ButtonIcons[Renderer->ButtonIconCount++]);
            
            r32 BtnSize = 25;
            rect BtnRect = {{-BtnSize, -BtnSize},{BtnSize, BtnSize}};
            button *ConfirmBtn = NewButton(Renderer, BtnRect, -0.91f, false, Renderer->ButtonBaseID, 
                                           &UIBGColor, &TextColor, &UIBGMutedColor, ConfirmID, &TextColor, 0);
            SetButtonTranslation(ConfirmBtn, V2(WWidth-BtnSize-15, WHeight-BtnSize-25));
            ConfirmBtn->OnPressed = {OnConfirm, GameState};
            TranslateWithScreen(&Renderer->TransformList, ConfirmBtn->Entry, fixedTo_TopRight);
            
            
            slider Slider = {};
            CreateVerticalSlider(Renderer, &Slider, V2(50, WHeight-120), -0.002f, &UIBGColor,  &UIBGMutedColor);
            Translate(&Slider, V2(WWidth-40, WHeight*0.5f-30));
            TransformWithScreen(&Renderer->TransformList, Slider.Background, fixedTo_RightCenter, scaleAxis_Y);
            
            
            // File counting stuff *****************************
            file_extensions FileExtensions = TryLoadFileExtensionsFile(GameState);
            
            code_files CodeFiles = {};
            CodeFiles.MaxCount = 5000;
            CodeFiles.SubPath  = PushArrayOnBucket(&GameState->Bucket.Fixed, CodeFiles.MaxCount, string_c);
            CodeFiles.FileName = PushArrayOnBucket(&GameState->Bucket.Fixed, CodeFiles.MaxCount, string_c);
            CodeFiles.FileData = PushArrayOnBucket(&GameState->Bucket.Fixed, CodeFiles.MaxCount, read_file_result);
            //string_c SearchFolder = NewStaticStringCompound("..\\code\\");
            string_c SubPath      = NewStaticStringCompound("");
            
            string_c LineBreak = NewStaticStringCompound("\n");
            string_c FileNamesS = NewStringCompound(&GameState->Bucket.Fixed, 100000);
            string_c LOCTotalS  = NewStringCompound(&GameState->Bucket.Fixed, 100000);
            string_c LOCS       = NewStringCompound(&GameState->Bucket.Fixed, 100000);
            render_text FileNamesRT = {};
            render_text LOCTotalRT  = {};
            render_text LOCRT       = {};
            
            UIHolder.LOCRT = &LOCRT;
            UIHolder.LOCTotalRT = &LOCTotalRT;
            UIHolder.FileNamesRT = &FileNamesRT;
            UIHolder.FileNamesS = &FileNamesS;
            UIHolder.LOCTotalS = &LOCTotalS;
            UIHolder.LOCS = &LOCS;
            UIHolder.Slider = &Slider;
            
            entry_id *TextAnchorNames = CreateRenderRect(Renderer, {}, 0, &BGColor, 0);
            SetPosition(TextAnchorNames, V2(25, WHeight - 100));
            TranslateWithScreen(&Renderer->TransformList, TextAnchorNames, fixedTo_TopLeft);
            
            entry_id *TextAnchorTotal = CreateRenderRect(Renderer, {}, 0, &BGColor, 0);
            SetPosition(TextAnchorTotal, V2(WWidth/3*2 - 100, WHeight - 100));
            TranslateWithScreen(&Renderer->TransformList, TextAnchorTotal, fixedTo_FixXToGiven_YTop, 0.75f);
            
            entry_id *TextAnchorNoWhite = CreateRenderRect(Renderer, {}, 0, &BGColor, 0);
            SetPosition(TextAnchorNoWhite, V2(WWidth/3*2+100, WHeight - 100));
            TranslateWithScreen(&Renderer->TransformList, TextAnchorNoWhite, fixedTo_FixXToGiven_YTop, 0.85f);
            
            u32 RowCount = 0;
            entry_id **Rows = PushArrayOnBucket(&GameState->Bucket.Fixed, 2500, entry_id*);
            UIHolder.Rows = &Rows;
            UIHolder.RowCount = &RowCount;
            
            b32 DisplayInfoBeforeFileLOCing = false;
            // ********************************************
            // FPS ****************************************
            // ********************************************
            
            
#if DEBUG_TD
            r32 FPSList[100] = {};
            u32 FPSCount = 0;
            v3 NOP = {};
            entry_id *FPSParent = CreateRenderRect(Renderer, {{},{}}, -0.9f, 0, &NOP);
            SetPosition(FPSParent, V2(Renderer->Window.CurrentDim.Dim) - V2(60, 11));
            TranslateWithScreen(&Renderer->TransformList, FPSParent, fixedTo_TopRight);
            render_text FPSText = {};
            r32 dUpdateRate = 0.0f;
#endif
            
            b32 SongChangedIsCurrentlyDecoding = false;
            IsRunning = true;
            while(IsRunning)
            {
                LARGE_INTEGER CurrentCycleCount = GetWallClock();
                GameState->Time.dTime = GetSecondsElapsed(PerfCountFrequency, PrevCycleCount, CurrentCycleCount);
                PrevCycleCount = CurrentCycleCount;
                GameState->Time.GameTime += GameState->Time.dTime;
                
#if DEBUG_TD
                FPSList[FPSCount] = (1.0f/GameState->Time.dTime);
                r32 CurrentFPS = 0;
                For(100) CurrentFPS += FPSList[It];
                CurrentFPS /= 100.0f;
                
                if(dUpdateRate >= 1.0f)
                {
                    dUpdateRate = 0.0f;
                    char FPSString[100];
                    sprintf_s(FPSString, "%.2f", CurrentFPS);
                    string_c FPSComp = NewStringCompound(&GameState->Bucket.Transient, 10);
                    AppendStringToCompound(&FPSComp, (u8 *)FPSString);
                    RemoveRenderText(&FPSText);
                    CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, 
                                     &FPSComp, &TextColor, &FPSText, -0.9f, FPSParent);
                    DeleteStringCompound(&GameState->Bucket.Transient, &FPSComp);
                }
                else dUpdateRate += GameState->Time.dTime/0.1f;
                FPSCount = (FPSCount+1)%100;
#endif
                
                // *******************************************
                // Thread handling ****************************
                // *******************************************
                EmptyJobQueueWhenPossible(JobQueue);
                
                // *******************************************
                // Input handling ****************************
                // *******************************************
                ProcessPendingMessages(Input, Window);
                
                if(Input->KeyChange[KEY_ESCAPE] == KeyDown)
                {
                    IsRunning = false;
                    continue;
                }
                
                UpdateButtons(Renderer, Input);
                
                if(Input->KeyChange[KEY_LMB] == KeyDown)
                {
                    OnDraggingStart(&Renderer->DragableList, Renderer, Input->MouseP);
                }
                if(Input->Pressed[KEY_LMB] && 
                   Renderer->DragableList.DraggingID >= 0)
                {
                    OnDragging(&Renderer->DragableList, Renderer, Input->MouseP);
                }
                else if(Renderer->DragableList.DraggingID >= 0) 
                {
                    OnDraggingEnd(&Renderer->DragableList, Renderer, Input->MouseP);
                }
                
                text_field_flag_result TFResult = ProcessTextField(Renderer, GameState->Time.dTime, Input, &TextField);
                if(TFResult.Flag & processTextField_Confirmed)
                {
                    OnConfirm(GameState);
                }
                else if(TFResult.Flag & processTextField_TextChanged)
                {
                    RemoveFileStuff(&UIHolder);
                }
                
                if(Input->WheelAmount != 0)
                {
                    UpdateVerticalSliderGrabThingPosition(&Slider, (r32)-Input->WheelAmount);
                }
                
                // *******************************************
                // Game logic handling************************
                // *******************************************
                
                if(GameState->DoCodeFileSearch)
                {
                    GameState->DoCodeFileSearch = false;
                    RemoveFileStuff(&UIHolder);
                    string_c BeforeLOCInfo = NewStaticStringCompound("Starting to scan folder.\nDepending on the amount of files, this may take some time.");
                    CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, &BeforeLOCInfo, 
                                     &TextColor, &FileNamesRT, 0.1f, TextAnchorNames);
                    DisplayInfoBeforeFileLOCing = true;
                }
                else if(DisplayInfoBeforeFileLOCing)
                {
                    DisplayInfoBeforeFileLOCing = false;
                    u32 TotalCount = 0;
                    u32 NoWhitespaceCount = 0;
                    CodeFiles.Count = 0;
                    RemoveFileStuff(&UIHolder);
                    ResetStringCompound(SearchFolder);
                    AppendStringCompoundToCompound(&SearchFolder, &TextField.TextString);
                    
                    if(SearchFolder.Pos > 0 && SearchFolder.S[SearchFolder.Pos-1] != '\\') 
                        AppendCharToCompound(&SearchFolder, '\\'); 
                    
                    if(FindAllCodeFilesInFolder(&GameState->Bucket.Fixed, &SearchFolder, &SubPath, &FileExtensions,
                                                &CodeFiles))
                    {
                        
                        AppendStringToCompound(&FileNamesS, (u8 *)"Filenames\n\n");
                        AppendStringToCompound(&LOCTotalS, (u8 *)"LOC Total\n\n");
                        AppendStringToCompound(&LOCS,      (u8 *)"LOC No Whitespaces\n\n");
                        
                        loc_data LOCData = {};
                        LOCData.Count     = CreateArray(&GameState->Bucket.Transient, CodeFiles.Count);
                        LOCData.LOC       = CreateArray(&GameState->Bucket.Transient, CodeFiles.Count);
                        array_u32 LOC_All = CreateArray(&GameState->Bucket.Transient, CodeFiles.Count);
                        array_u32 LOC     = CreateArray(&GameState->Bucket.Transient, CodeFiles.Count);
                        For(CodeFiles.Count)
                        {
                            u32 LOCTotal = CountEveryLinebreak(CodeFiles.FileData[It]);
                            Push(&LOC_All, LOCTotal);
                            TotalCount += LOCTotal;
                            u32 LOC_    = CountLinebreaksAndNoWhitespaces(CodeFiles.FileData[It]);
                            Push(&LOC, LOC_);
                            Push(&LOCData.LOC, LOC_);
                            NoWhitespaceCount += LOC_;
                            Push(&LOCData.Count, It);
                        }
                        
                        QuickSortLOC(0, CodeFiles.Count-1, &LOCData);
                        
                        string_c Space = NewStaticStringCompound("  ");
                        string_c ShortenedPath = NewStaticStringCompound("..\\");
                        For(CodeFiles.Count)
                        {
                            I32ToString(&FileNamesS, It+1);
                            AppendCharToCompound(&FileNamesS, '.');
                            For((CountNumerals(CodeFiles.Count)-CountNumerals(It+1)+1), In) 
                                AppendStringCompoundToCompound(&FileNamesS, &Space);
                            
                            u32 ID = Get(&LOCData.Count, It);
                            if(CodeFiles.SubPath[ID].Pos > 25) 
                            {
                                CodeFiles.SubPath[ID].Pos = 25;
                                AppendStringCompoundToCompound(&FileNamesS, CodeFiles.SubPath+ID);
                                AppendStringCompoundToCompound(&FileNamesS, &ShortenedPath);
                            }
                            else AppendStringCompoundToCompound(&FileNamesS, CodeFiles.SubPath+ID);
                            ConcatStringCompounds(3, &FileNamesS, CodeFiles.FileName+ID, &LineBreak);
                            I32ToString(&LOCTotalS, Get(&LOC_All, ID));
                            AppendStringCompoundToCompound(&LOCTotalS, &LineBreak);
                            I32ToString(&LOCS, Get(&LOC, ID));
                            AppendStringCompoundToCompound(&LOCS, &LineBreak);
                        }
                        
                        AppendStringToCompound(&FileNamesS, (u8 *)"\nTotals: ");
                        AppendStringToCompound(&LOCTotalS, (u8 *)"\n");
                        I32ToString(&LOCTotalS, TotalCount);
                        AppendStringToCompound(&LOCS, (u8 *)"\n");
                        I32ToString(&LOCS, NoWhitespaceCount);
                        AppendStringToCompound(&LOCS, (u8 *)"\n");
                        
                        CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, &FileNamesS, 
                                         &TextColor, &FileNamesRT, 0.1f, TextAnchorNames);
                        CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, &LOCTotalS, 
                                         &TextColor, &LOCTotalRT, 0.1f, TextAnchorTotal);
                        CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, &LOCS, 
                                         &TextColor, &LOCRT, 0.1f, TextAnchorNoWhite);
                        
                        UpdateVerticalSlideHeightChanged(Renderer, &Slider, LOCRT.CurrentP.y-LOCRT.StartP.y);
                        Slider.SliderIsDragged = true;
                        
                        entry_id *RowParent = FileNamesRT.Base;
                        r32 OffsetY = -2;
                        RowCount = 0;
                        For(CodeFiles.Count+2)
                        {
                            if(It%2==0)
                            {
                                Rows[RowCount] = CreateRenderRect(Renderer, V2(WWidth*10, 22), 0.5f, &BGColorMuted, RowParent);
                                Translate(Rows[RowCount], V2(0, OffsetY));
                                ScaleWithScreen(&Renderer->TransformList, Rows[RowCount], scaleAxis_X);
                                
                                OffsetY -= 44;
                                RowCount++;
                            }
                        }
                    }
                    else
                    {
                        AppendStringToCompound(&FileNamesS, (u8 *)"\nNot successful finding code files.");
                        CreateRenderText(Renderer, Renderer->FontInfo.SmallFont, &FileNamesS, 
                                         &TextColor, &FileNamesRT, 0.0f, TextAnchorNames);
                    }
                    DebugLog(255, "Total LOC: %i, No Whitespace LOC: %i\n", TotalCount, NoWhitespaceCount);
                }
                
                if(Slider.SliderIsDragged)
                {
                    Slider.SliderIsDragged = false;
                    r32 NewYOffset = GetOverhang(&Slider)*Slider.TranslationPercentage;
                    SetLocalPosition(&FileNamesRT, V2(0, NewYOffset));
                    SetLocalPosition(&LOCTotalRT, V2(0, NewYOffset));
                    SetLocalPosition(&LOCRT, V2(0, NewYOffset));
                }
                
                // *******************************************
                // Rendering *********************************
                // *******************************************
                *ReRender = true;
                DisplayBufferInWindow(DeviceContext, Renderer);
                
                FlipWallClock = GetWallClock();
                
            }
            
            SaveSettingsFile(GameState, &SearchFolder);
        }
        else
        {
            DebugLog(255, "%i\n", GetLastError());
        }
    }
    return 0;
}
