#include "raylib.h"
#include "config.h"

#define mcollide(x, y, w, h) \
    CheckCollisionPointRec(GetMousePosition(), (Rectangle){x, y, w, h})

#define lmbdown IsMouseButtonDown(MOUSE_LEFT_BUTTON)
#define lmbup IsMouseButtonReleased(MOUSE_LEFT_BUTTON)
#define focused(i) (i >= WINDOW_LIMIT - 1)

#define RENDER_WIDTH (SCREEN_WIDTH / SCALE)
#define RENDER_HEIGHT (SCREEN_HEIGHT / SCALE)

// _____________________________________________________________________________
//
// DrawTextRec was removed from raylib in 4.0, we need to re-implement it
// https://github.com/raysan5/raylib/blob/master/examples/text/text_rectangle_bounds.c
// _____________________________________________________________________________
//

// Draw text using font inside rectangle limits with support for text selection
static void DrawTextBoxedSelectable(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint, int selectStart, int selectLength, Color selectTint, Color selectBackTint)
{
    int length = TextLength(text);  // Total length in bytes of the text, scanned by codepoints in loop

    float textOffsetY = 0;          // Offset between lines (on line break '\n')
    float textOffsetX = 0.0f;       // Offset X to next character to draw

    float scaleFactor = fontSize/(float)font.baseSize;     // Character rectangle scaling factor

    // Word/character wrapping mechanism variables
    enum { MEASURE_STATE = 0, DRAW_STATE = 1 };
    int state = wordWrap? MEASURE_STATE : DRAW_STATE;

    int startLine = -1;         // Index where to begin drawing (where a line begins)
    int endLine = -1;           // Index where to stop drawing (where a line ends)
    int lastk = -1;             // Holds last value of the character position

    for (int i = 0, k = 0; i < length; i++, k++)
    {
        // Get next codepoint from byte string and glyph index in font
        int codepointByteCount = 0;
        int codepoint = GetCodepoint(&text[i], &codepointByteCount);
        int index = GetGlyphIndex(font, codepoint);

        // NOTE: Normally we exit the decoding sequence as soon as a bad byte is found (and return 0x3f)
        // but we need to draw all of the bad bytes using the '?' symbol moving one byte
        if (codepoint == 0x3f) codepointByteCount = 1;
        i += (codepointByteCount - 1);

        float glyphWidth = 0;
        if (codepoint != '\n')
        {
            glyphWidth = (font.glyphs[index].advanceX == 0) ? font.recs[index].width*scaleFactor : font.glyphs[index].advanceX*scaleFactor;

            if (i + 1 < length) glyphWidth = glyphWidth + spacing;
        }

        // NOTE: When wordWrap is ON we first measure how much of the text we can draw before going outside of the rec container
        // We store this info in startLine and endLine, then we change states, draw the text between those two variables
        // and change states again and again recursively until the end of the text (or until we get outside of the container).
        // When wordWrap is OFF we don't need the measure state so we go to the drawing state immediately
        // and begin drawing on the next line before we can get outside the container.
        if (state == MEASURE_STATE)
        {
            // TODO: There are multiple types of spaces in UNICODE, maybe it's a good idea to add support for more
            // Ref: http://jkorpela.fi/chars/spaces.html
            if ((codepoint == ' ') || (codepoint == '\t') || (codepoint == '\n')) endLine = i;

            if ((textOffsetX + glyphWidth) > rec.width)
            {
                endLine = (endLine < 1)? i : endLine;
                if (i == endLine) endLine -= codepointByteCount;
                if ((startLine + codepointByteCount) == endLine) endLine = (i - codepointByteCount);

                state = !state;
            }
            else if ((i + 1) == length)
            {
                endLine = i;
                state = !state;
            }
            else if (codepoint == '\n') state = !state;

            if (state == DRAW_STATE)
            {
                textOffsetX = 0;
                i = startLine;
                glyphWidth = 0;

                // Save character position when we switch states
                int tmp = lastk;
                lastk = k - 1;
                k = tmp;
            }
        }
        else
        {
            if (codepoint == '\n')
            {
                if (!wordWrap)
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }
            }
            else
            {
                if (!wordWrap && ((textOffsetX + glyphWidth) > rec.width))
                {
                    textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                    textOffsetX = 0;
                }

                // When text overflows rectangle height limit, just stop drawing
                if ((textOffsetY + font.baseSize*scaleFactor) > rec.height) break;

                // Draw selection background
                bool isGlyphSelected = false;
                if ((selectStart >= 0) && (k >= selectStart) && (k < (selectStart + selectLength)))
                {
                    DrawRectangleRec((Rectangle){ rec.x + textOffsetX - 1, rec.y + textOffsetY, glyphWidth, (float)font.baseSize*scaleFactor }, selectBackTint);
                    isGlyphSelected = true;
                }

                // Draw current character glyph
                if ((codepoint != ' ') && (codepoint != '\t'))
                {
                    DrawTextCodepoint(font, codepoint, (Vector2){ rec.x + textOffsetX, rec.y + textOffsetY }, fontSize, isGlyphSelected? selectTint : tint);
                }
            }

            if (wordWrap && (i == endLine))
            {
                textOffsetY += (font.baseSize + font.baseSize/2)*scaleFactor;
                textOffsetX = 0;
                startLine = endLine;
                endLine = -1;
                glyphWidth = 0;
                selectStart += lastk - k;
                k = lastk;

                state = !state;
            }
        }

        textOffsetX += glyphWidth;
    }
}


static void DrawTextRec(Font font, const char *text, Rectangle rec, float fontSize, float spacing, bool wordWrap, Color tint)
{
    DrawTextBoxedSelectable(font, text, rec, fontSize, spacing, wordWrap, tint, 0, 0, WHITE, WHITE);
}

// _____________________________________________________________________________
//
//  Structs & Variables
// _____________________________________________________________________________
//

typedef struct Window
{
    int x, y;
    int width, height;
    int minWidth, minHeight; // minimum size of the window (if resizable)
    bool active;             // if true, this window slot is taken and the window is shown on screen
    bool minimized, maximized;
    bool resizable;
    Rectangle oldPos;   // old window coords are saved here when the window is maximized
    void (*function)(struct Window *window, int index); // pointer to the function that is executed on this window every frame
    void *data;         // storage for window related variables
    const char *title;

    // for message boxes
    const char *message;
    int icon; // index of the icons array
} Window;


typedef enum
{
    IC_ERROR,
    IC_LOGO,
    IC_NOSLOTS,
    IC_ENDSESSION,
    IC_COUNT
} Icon;

Font font = {0};
Font boldFont = {0};
Texture icons[IC_COUNT];
Texture winButtons[8]; // close, maximize, minimize, etc
Texture smallButtons[2];
Texture largeButtons[2];
Texture startButtons[2];

Window windows[WINDOW_LIMIT + 1]; // last window slot is reserved, second to last window is focused
bool moving = false;              // is the focused window being moved?
bool resizing = false;            // is the focused window being resized?
Vector2 hook = {0};               // mouse position relative to the focused window when it is started to be moved

int cursor = MOUSE_CURSOR_DEFAULT; // mouse cursor style, updated every frame
char timebuf[16];                  // string to store current time
bool running = true;               // if set to false, clean up and exit

// _____________________________________________________________________________
//
//  Utility functions
// _____________________________________________________________________________
//

// Gives focus to the specified window.
void focusWindow(int index)
{
    Window temp = windows[index];
    for (int i = index; i < WINDOW_LIMIT; i++)
        windows[i] = windows[i + 1];

    windows[WINDOW_LIMIT - 1] = temp;
}

// Draws text inside a window.
void winDrawText(Window *window, const char *text, int x, int y)
{
    DrawTextRec(
        font, text,
        (Rectangle){
            window->x + 2 + x,
            window->y + 16 + y,
            window->width - 2 - x,
            window->height - 16 - y},
        FONT_SIZE, 0.0f, true,
        WINDOW_TEXT_COLOR);

#ifdef DEBUG_WINDRAWTEXT
    DrawRectangleLines(
        window->x + 2 + x,
        window->y + 16 + y,
        window->width - 2 - x,
        window->height - 16 - y,
        BLACK);
#endif
}

// Draws a texture inside a window.
void winDrawTexture(Window *window, Texture *texture, int x, int y)
{
    DrawTexture(*texture, window->x + 2 + x, window->y + 16 + y, WHITE);
}

// Draws a button inside a window, returns true if the button was clicked.
bool winButton(Window *window, int index, const char *text, int x, int y, bool large)
{
    bool hovered = focused(index) && mcollide((window->x + 2 + x), (window->y + 16 + y), (48 * (large + 1)), 16);
    Texture texture = (large ? largeButtons : smallButtons)[hovered && lmbdown];

    winDrawTexture(window, &texture, x, y);
    winDrawText(window, text, x + 2, y + 2);

    return hovered && lmbup;
}

void messageBoxWindow(Window *window, int index);

// Opens a new window.
bool createWindow(Window window)
{
    window.active = true;
    window.minimized = false;
    window.maximized = false;

    // find a free window slot
    int slot = -1;
    for (int i = 0; i < WINDOW_LIMIT; i++)
        if (!windows[i].active) slot = i;

    if (slot != -1)
    {
        // if a slot was found, assign it to the window and focus it
        windows[slot] = window;
        focusWindow(slot);
        return true;
    }
    else
    {
        // if all window slots are taken, show an error message
        windows[WINDOW_LIMIT] = (Window){
            .x = RENDER_WIDTH / 2 - 100,
            .y = RENDER_HEIGHT / 2 - 50,
            .width = 200,
            .height = 100,
            .active = true,
            .function = messageBoxWindow,
            .title = "Error",
            .message = "Out of window slots! Close some windows and try again.",
            .icon = IC_NOSLOTS};
        return false;
    }
}

// _____________________________________________________________________________
//
//  Window controller functions
// _____________________________________________________________________________
//

void messageBoxWindow(Window *window, int index)
{
    winDrawTexture(window, &icons[window->icon], 8, 8);
    winDrawText(window, window->message, 48, 8);

    if (winButton(window, index, "OK", 48, 64, false))
        window->active = false;
}

void endSessionWindow(Window *window, int index)
{
    winDrawTexture(window, &icons[IC_ENDSESSION], 8, 8);
    winDrawText(window, "Are you sure you want to end your session?", 48, 8);

    if (winButton(window, index, "Yes", 48, 64, false))
        running = false;
    if (winButton(window, index, "No", 100, 64, false))
        window->active = false;
}

// Window used for demonstrating window-bound variable storage.
void testWindow(Window *window, int index)
{
    winDrawText(window, TextFormat("%d", window->data), 0, 0);

    if (winButton(window, index, "Increase", 0, 20, 1))
        window->data++;
    if (winButton(window, index, "Decrease", 0, 36, 1))
        window->data--;
}

void startMenuWindow(Window *window, int index)
{
    // if this window loses focus, close it
    if (!focused(index)) window->active = false;

    // check each window, if another start menu is open, don't create a new one
    for (int i = 0; i < WINDOW_LIMIT + 1; i++)
    {
        if (windows[i].active && windows[i].function == startMenuWindow && i != index)
        {
            window->active = false;
        }
    }

    // force the start menu to stay in one location
    window->x = 0;
    window->y = RENDER_HEIGHT / 2;

    if (winButton(window, index, "Exit", 0, 0, true))
    {
        window->active = false;

        createWindow((Window){
            .x = RENDER_WIDTH / 2 - 100,
            .y = RENDER_HEIGHT / 2 - 50,
            .width = 200,
            .height = 100,
            .title = "End session",
            .function = endSessionWindow});
    }

    if (winButton(window, index, "window.data test", 0, 16, true))
    {
        window->active = false;

        createWindow((Window){
            .x = RENDER_WIDTH / 2 - 100,
            .y = RENDER_HEIGHT / 2 - 50,
            .width = 200,
            .height = 100,
            .title = "window.data test",
            .function = testWindow});
    }
}

// Draw part of a texture (defined by a rectangle) with rotation and scale tiled into dest
void DrawTextureTiled(Texture2D texture, Rectangle source, Rectangle dest, Vector2 origin, float rotation, float scale, Color tint)
{
    if ((texture.id <= 0) || (scale <= 0.0f)) return;  // Wanna see a infinite loop?!...just delete this line!
    if ((source.width == 0) || (source.height == 0)) return;

    int tileWidth = (int)(source.width*scale), tileHeight = (int)(source.height*scale);
    if ((dest.width < tileWidth) && (dest.height < tileHeight))
    {
        // Can fit only one tile
        DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, ((float)dest.height/tileHeight)*source.height},
                    (Rectangle){dest.x, dest.y, dest.width, dest.height}, origin, rotation, tint);
    }
    else if (dest.width <= tileWidth)
    {
        // Tiled vertically (one column)
        int dy = 0;
        for (;dy+tileHeight < dest.height; dy += tileHeight)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, source.height}, (Rectangle){dest.x, dest.y + dy, dest.width, (float)tileHeight}, origin, rotation, tint);
        }

        // Fit last tile
        if (dy < dest.height)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)dest.width/tileWidth)*source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                        (Rectangle){dest.x, dest.y + dy, dest.width, dest.height - dy}, origin, rotation, tint);
        }
    }
    else if (dest.height <= tileHeight)
    {
        // Tiled horizontally (one row)
        int dx = 0;
        for (;dx+tileWidth < dest.width; dx += tileWidth)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, source.width, ((float)dest.height/tileHeight)*source.height}, (Rectangle){dest.x + dx, dest.y, (float)tileWidth, dest.height}, origin, rotation, tint);
        }

        // Fit last tile
        if (dx < dest.width)
        {
            DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, ((float)dest.height/tileHeight)*source.height},
                        (Rectangle){dest.x + dx, dest.y, dest.width - dx, dest.height}, origin, rotation, tint);
        }
    }
    else
    {
        // Tiled both horizontally and vertically (rows and columns)
        int dx = 0;
        for (;dx+tileWidth < dest.width; dx += tileWidth)
        {
            int dy = 0;
            for (;dy+tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture, source, (Rectangle){dest.x + dx, dest.y + dy, (float)tileWidth, (float)tileHeight}, origin, rotation, tint);
            }

            if (dy < dest.height)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                    (Rectangle){dest.x + dx, dest.y + dy, (float)tileWidth, dest.height - dy}, origin, rotation, tint);
            }
        }

        // Fit last column of tiles
        if (dx < dest.width)
        {
            int dy = 0;
            for (;dy+tileHeight < dest.height; dy += tileHeight)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, source.height},
                        (Rectangle){dest.x + dx, dest.y + dy, dest.width - dx, (float)tileHeight}, origin, rotation, tint);
            }

            // Draw final tile in the bottom right corner
            if (dy < dest.height)
            {
                DrawTexturePro(texture, (Rectangle){source.x, source.y, ((float)(dest.width - dx)/tileWidth)*source.width, ((float)(dest.height - dy)/tileHeight)*source.height},
                    (Rectangle){dest.x + dx, dest.y + dy, dest.width - dx, dest.height - dy}, origin, rotation, tint);
            }
        }
    }
}