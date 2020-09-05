#include "GameBasics_TD.h"

internal loaded_bitmap 
LoadImage_STB(u8 *Path)
{
    loaded_bitmap Result = {};
    
    // TODO:: Small image is strange: https://github.com/nothings/stb/issues/161
    i32 BG_X, BG_Y, BG_N;
    Result.Pixels = (u32 *)stbi_load((const char *)Path, &BG_X, &BG_Y, &BG_N, 0);
    Result.Width = BG_X;
    Result.Height = BG_Y;
    if(Result.Pixels) 
    {
        Result.WasLoaded = true;
        switch(BG_N)
        {
            case 3:
            {
                Result.ColorFormat = colorFormat_RGB;
            } break;
            case 4:
            {
                Result.ColorFormat = colorFormat_RGBA;
            } break;
            
            InvalidDefaultCase
        }
    }
    
    return Result;
}

inline void
FreeImage_STB(loaded_bitmap Bitmap)
{
    stbi_image_free(Bitmap.Pixels);
}

internal b32
FindAllCodeFilesInFolder(memory_bucket_container *Bucket, string_compound *FolderPath, string_compound *SubPath, file_extensions *FileExtensions, code_files *ResultingFileInfo)
{
    b32 Result = false;
    string_compound FolderPathStar = NewStringCompound(&Bucket->Parent->Transient, 255);
    ConcatStringCompounds(3, &FolderPathStar, FolderPath, SubPath);
    AppendCharToCompound(&FolderPathStar, '*');
    
    string_w WideFolderPath = {};
    ConvertString8To16(&Bucket->Parent->Transient, &FolderPathStar, &WideFolderPath);
    
    WIN32_FIND_DATAW FileData = {};
    HANDLE FileHandle = FindFirstFileExW(WideFolderPath.S, 
                                         FindExInfoBasic, 
                                         &FileData, 
                                         FindExSearchNameMatch, 
                                         NULL, 
                                         FIND_FIRST_EX_LARGE_FETCH);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        b32 HasNextFile = true;
        string_compound FileType = NewStringCompound(&Bucket->Parent->Transient, 16);
        
        while(HasNextFile && ResultingFileInfo->Count < ResultingFileInfo->MaxCount)
        {
            string_c FileName = {};
            ConvertString16To8(&Bucket->Parent->Transient, FileData.cFileName, &FileName);
            
            if     (CompareStringAndCompound(&FileName, (u8 *)".") ) {}
            else if(CompareStringAndCompound(&FileName, (u8 *)"..")) {}
            else
            {
                if(FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    i32 PathLength = FileName.Pos+SubPath->Pos+1;
                    string_compound NewSubFolderPath = NewStringCompound(&Bucket->Parent->Transient, PathLength);
                    ConcatStringCompounds(3, &NewSubFolderPath, SubPath, &FileName);
                    AppendCharToCompound(&NewSubFolderPath, '\\');
                    
                    b32 NewResult = FindAllCodeFilesInFolder(Bucket, FolderPath, &NewSubFolderPath, 
                                                             FileExtensions, ResultingFileInfo);
                    Result = Result || NewResult;
                    DeleteStringCompound(&Bucket->Parent->Transient, &NewSubFolderPath);
                }
                else
                {
                    i32 LastDot = FindLastOccurrenceOfCharInStringCompound(&FileName, '.');
                    if(LastDot > 0)
                    {
                        PasteStringCompoundIntoCompound(&FileType, 0, &FileName, LastDot+1, FileName.Pos-(LastDot+1));
                        b32 CorrectExtension = false;
                        For(FileExtensions->Count)
                        {
                            if(CompareStringCompounds(&FileType, FileExtensions->Extension+It))
                            {
                                CorrectExtension = true;
                                break;
                            }
                        }
                        if(CorrectExtension)
                        {
                            if(ResultingFileInfo->Count < MAX_FILES)
                            {
                                ResultingFileInfo->SubPath[ResultingFileInfo->Count] = NewStringCompound(Bucket, SubPath->Pos);
                                AppendStringCompoundToCompound(ResultingFileInfo->SubPath+ResultingFileInfo->Count, SubPath);
                                ResultingFileInfo->FileName[ResultingFileInfo->Count] = NewStringCompound(Bucket, FileName.Pos);
                                AppendStringCompoundToCompound(ResultingFileInfo->FileName+ResultingFileInfo->Count, &FileName);
                                
                                string_c FilePath = NewStringCompound(&Bucket->Parent->Transient, 255);
                                ConcatStringCompounds(4, &FilePath, FolderPath, SubPath, &FileName);
                                if(!ReadEntireFile(Bucket, ResultingFileInfo->FileData+ResultingFileInfo->Count, FilePath.S)) Assert(false);
                                DeleteStringCompound(&Bucket->Parent->Transient, &FilePath);
                                
                                ResultingFileInfo->Count++;
                            }
                            Result = true;
                        }
                        ResetStringCompound(FileType);
                    }
                }
            }
            HasNextFile = FindNextFileW(FileHandle, &FileData);
            DeleteStringCompound(&Bucket->Parent->Transient, &FileName);
        } 
        DeleteStringCompound(&Bucket->Parent->Transient, &FileType);
    }
    
    char Out[555];
    sprintf_s(Out, "Found %i Files in folder %s.\n", ResultingFileInfo->Count, FolderPathStar.S);
    OutputDebugStringA(Out);
    
    DeleteStringW(&Bucket->Parent->Transient, &WideFolderPath);
    DeleteStringCompound(&Bucket->Parent->Transient, &FolderPathStar);
    return Result;
}

internal u32
CountEveryLinebreak(read_file_result File)
{
    u32 Result = 1;
    u8 *C = File.Data;
    
    while(*C) if(*C++ == '\n') Result++;
    
    return Result;
}

inline void
AdvanceToNewline(u8 **String)
{
    while((*String)[0] != '\n') 
    {
        if((*String)[0] == 0) return;
        (*String)++;
    }
    (*String)++;
}

inline b32 // false if new comment ended on the same line
AdvanceToMultLineCommentEnd(u8 **String)
{
    b32 Result = false;
    while((*String)[1] != 0 && ((*String)[0] != '*' || (*String)[1] != '/'))
    {
        if((*String)[0] == 0) return Result;
        if((*String)[0] == '\n') Result = true;
        (*String)++;
    }
    (*String)++;
    (*String)++;
    return Result;
}

inline b32 // false if ended on multline comment
AdvanceToNewlineOrMultilineComment(u8 **String)
{
    while((*String)[0] != '\n') 
    {
        if((*String)[0] == 0) return true;
        if((*String)[1] && (*String)[0] == '/' && (*String)[1] == '*') return false;
        (*String)++;
    }
    (*String)++;
    
    return true;
}

internal u32
CountLinebreaksAndNoWhitespaces(read_file_result File)
{
    u32 Result = 0;
    u8 *C = File.Data;
    
    while(*C)
    {
        if(C[1] && C[0] == '/' && C[1] == '/') // Case: normal comment, advance to next line
        { AdvanceToNewline(&C); continue; } 
        
        if(C[1] && C[0] == '/' && C[1] == '*') // Case: multline comment, skip it and move on
        { AdvanceToMultLineCommentEnd(&C); } 
        
        if(C[0] == '\n' || (C[1] && C[0] == '\r' && C[1] == '\n')) // Case: Newline with nothing on it (or after ml-comment)
        { AdvanceToNewline(&C); continue; }
        
        Result++; // Case: On this line is content
        if(!AdvanceToNewlineOrMultilineComment(&C))
        { 
            // Case: If the found ml-comment is on the same line, skip the rest as it is already counted
            if(!AdvanceToMultLineCommentEnd(&C)) 
                AdvanceToNewline(&C); 
        }
    }
    
    return Result;
}

inline void
OnConfirm(void *Data)
{
    game_state *GameState = (game_state *)Data;
    
    GameState->DoCodeFileSearch = true;
    
}


internal string_c
TryLoadSettingsFile(game_state *GameState)
{
    string_c Result = {};
    
    read_file_result Data = {};
    string_c FilePath = NewStaticStringCompound("..\\data\\LOCSettings.save");
    if(ReadEntireFile(&GameState->Bucket.Transient, &Data, FilePath.S))
    {
        Result = NewStringCompound(&GameState->Bucket.Fixed, Data.Size);
        AppendStringToCompound(&Result, Data.Data);
        
        FreeFileMemory(&GameState->Bucket.Transient, Data.Data);
    }
    return Result;
}

internal void
SaveSettingsFile(game_state *GameState, string_c *Path)
{
    read_file_result Data = {};
    string_c FilePath = NewStaticStringCompound("..\\data\\LOCSettings.save");
    
    
    if(WriteEntireFile(&GameState->Bucket, FilePath.S, Path->Pos+1, Path->S))
    {
        DebugLog(255, "Successfully wrote out settings file.\n");
    }
}

inline u32 
CountToNewline(u8 *String)
{
    u32 Result = 0;
    
    while(*String != '\n' && *String != '\r' && *String++ != 0) Result++;
    
    return Result;
}

internal file_extensions
TryLoadFileExtensionsFile(game_state *GameState)
{
    file_extensions Result = {};
    
    read_file_result Data = {};
    string_c FilePath = NewStaticStringCompound("..\\data\\LOCFileExtensions.save");
    if(ReadEntireFile(&GameState->Bucket.Transient, &Data, FilePath.S))
    {
        u8 *C = Data.Data;
        
        Result.MaxCount = CountLinebreaksAndNoWhitespaces(Data);
        Result.Extension = PushArrayOnBucket(&GameState->Bucket.Fixed, Result.MaxCount, string_c);
        
        For(Result.MaxCount)
        {
            u32 Count = CountToNewline(C);
            Result.Extension[It] = NewStringCompound(&GameState->Bucket.Fixed, Count);
            For(Count, Char) AppendCharToCompound(Result.Extension+It, C[CharIt]);
            AdvanceToNewline(&C);
            Result.Count++;
        }
        
        FreeFileMemory(&GameState->Bucket.Transient, Data.Data);
    }
    return Result;
}

struct loc_data
{
    array_u32 Count;
    array_u32 LOC;
};

inline void
Swap(loc_data *SortArray, u32 SmallID, u32 HighID)
{
    u32 TMP = Get(&SortArray->Count, SmallID);
    ReplaceAt(&SortArray->Count, SmallID, Get(&SortArray->Count, HighID));
    ReplaceAt(&SortArray->Count, HighID, TMP);
    
    TMP = Get(&SortArray->LOC, SmallID);
    ReplaceAt(&SortArray->LOC, SmallID, Get(&SortArray->LOC, HighID));
    ReplaceAt(&SortArray->LOC, HighID, TMP);
}

internal i32
QuickSortPartitionLOC(i32 Low, i32 High, loc_data *SortArray) 
{ 
    i32 Pivot   = High;
    i32 SmallID = (Low - 1);
    
    for(i32 HighID = Low; HighID <= High- 1; HighID++) 
    { 
        if (Get(&SortArray->LOC, HighID) > Get(&SortArray->LOC, Pivot)) 
        { 
            SmallID++;
            Swap(SortArray, SmallID, HighID); 
        } 
    } 
    Swap(SortArray, SmallID+1, High); 
    return (SmallID + 1); 
} 

internal void 
QuickSortLOC(i32 Low, i32 High, loc_data *SortArray) 
{ 
    if(Low < High)
    {
        i32 PartitionID = QuickSortPartitionLOC(Low, High, SortArray); 
        
        QuickSortLOC(Low, PartitionID - 1, SortArray); 
        QuickSortLOC(PartitionID + 1, High, SortArray); 
    }
} 

inline u32
CountNumerals(u32 Number)
{
    u32 Result = 1;
    
    u32 Counter = 10;
    while(Number >= Counter) 
    {
        Result++;
        Counter *= 10;
    }
    
    
    return Result;
}
























