#pragma once
#include "Renderer_TD.h"

#define MAX_FILES 1500
global_variable string_c SETTINGS_FILE_NAME NewStaticStringCompound("MPlay3Settings.save");
struct settings
{
    r32 Volume = 0.5f;
    i32 PlayingSongID = -1;
    u32 ColorPaletteID = 0;
    r32 GenreArtistEdgeXPercent = -1.0f;
    r32 ArtistAlbumEdgeXPercent = -1.0f;
    r32 AlbumSongEdgeXPercent = -1.0f;
};

enum cursor_state
{
    cursorState_Arrow,
    cursorState_Drag,
};

struct game_state
{
    string_c DataPath;
    bucket_allocator Bucket;
    
    input_info Input;
    
    // Time management
    time_management Time;
    
    renderer Renderer;
    
    cursor_state CursorState;
    
    // Threading stuff
    bucket_allocator SoundThreadBucket;
    circular_job_queue JobQueue;
    
    
    b32 DoCodeFileSearch;
};

struct code_files
{
    string_c *SubPath;
    string_c *FileName;
    
    read_file_result *FileData;
    
    u32 Count;
    u32 MaxCount;
};

struct ui_holder
{
    render_text *LOCRT;
    render_text *LOCTotalRT;
    render_text *FileNamesRT;
    string_c *FileNamesS;
    string_c *LOCTotalS;
    string_c *LOCS;
    entry_id ***Rows;
    u32 *RowCount;
    struct slider *Slider;
};

struct file_extensions
{
    string_c *Extension;
    u32 Count;
    u32 MaxCount;
};

internal loaded_bitmap LoadImage_STB(u8 *Path);
inline void FreeImage_STB(loaded_bitmap Bitmap);