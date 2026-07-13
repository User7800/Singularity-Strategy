#include <time.h>
#include <stdio.h>
#include "raylib.h"
#include "config.h"
#include "rlwm.h"
#include "math.h"

#define PLAYER_SIZE 12

// _____________________________________________________________________________
//
//  Main
// _____________________________________________________________________________
//

int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "rlwm");
    SetTargetFPS(60);
    SetWindowIcon(LoadImage(TextFormat("%s/logo.png", ASSETS_FOLDER)));
    SetMouseScale(1 / SCALE, 1 / SCALE);

    RenderTexture rt = LoadRenderTexture(RENDER_WIDTH, RENDER_HEIGHT);

    if (FULLSCREEN) ToggleFullscreen();

    font = LoadFontEx(TextFormat("%s/font.ttf", ASSETS_FOLDER), FONT_SIZE, NULL, 95);
    boldFont = LoadFontEx(TextFormat("%s/font_bold.ttf", ASSETS_FOLDER), FONT_SIZE, NULL, 95);

    Texture bg = LoadTexture(TextFormat("%s/bg.png", ASSETS_FOLDER));

    // _________________________________________________________________________
    //
    //  Load button images
    // _________________________________________________________________________
    //

    Image buttonImage = LoadImage(TextFormat("%s/buttons.png", ASSETS_FOLDER));

    // window control buttons
    for (int i = 0; i < 8; i++)
    {
        Image button = ImageCopy(buttonImage);
        ImageCrop(&button, (Rectangle){i * 12, 0, 12, 12});
        winButtons[i] = LoadTextureFromImage(button);
        UnloadImage(button);
    }

    for (int i = 0; i < 2; i++)
    {
        // small buttons (48 × 16)
        Image sbutton = ImageCopy(buttonImage);
        ImageCrop(&sbutton, (Rectangle){i * 48, 12, 48, 16});
        smallButtons[i] = LoadTextureFromImage(sbutton);
        UnloadImage(sbutton);

        // large buttons (96 × 16)
        Image lbutton = ImageCopy(buttonImage);
        ImageCrop(&lbutton, (Rectangle){0, 28 + i * 16, 96, 16});
        largeButtons[i] = LoadTextureFromImage(lbutton);
        UnloadImage(lbutton);

        // start buttons (48 × 16)
        Image stbutton = ImageCopy(buttonImage);
        ImageCrop(&stbutton, (Rectangle){i * 48, 60, 48, 16});
        startButtons[i] = LoadTextureFromImage(stbutton);
        UnloadImage(stbutton);
    }

    UnloadImage(buttonImage);

    // _________________________________________________________________________
    //
    //  Load icons
    // _________________________________________________________________________
    //

    Image iconImage = LoadImage(TextFormat("%s/icons.png", ASSETS_FOLDER));

    for (int i = 0; i < IC_COUNT; i++)
    {
        Image icon = ImageCopy(iconImage);
        ImageCrop(&icon, (Rectangle){i * 32, 0, 32, 32});
        icons[i] = LoadTextureFromImage(icon);
        UnloadImage(icon);
    }

    UnloadImage(iconImage);

    // _________________________________________________________________________
    //
    //  Set up player
    // _________________________________________________________________________
    //
    Rectangle player1 = { 3*PLAYER_SIZE, 3*PLAYER_SIZE, PLAYER_SIZE, PLAYER_SIZE };
    Camera2D camera1 = { 0 };
    camera1.target = (Vector2){ player1.x, player1.y };
    camera1.offset = (Vector2){ 450.0f , 200.0f };
    camera1.rotation = 0.0f;
    camera1.zoom = 1.0f;

    RenderTexture screenCamera1 = LoadRenderTexture(SCREEN_WIDTH, SCREEN_HEIGHT);

    // _________________________________________________________________________
    //
    //  Main Loop
    // _________________________________________________________________________
    //

    createWindow((Window){
        .x = 50,
        .y = 80,
        .width = 224,
        .height = 100,
        .minWidth = 224,
        .minHeight = 100,
        .resizable = true,
        .function = messageBoxWindow,
        .title = "Testing",
        .message = "Example message box window\nPress A to create new windows",
        .icon = IC_LOGO});

    while (running && !WindowShouldClose())
    {
        // _____________________________________________________________________
        //
        //  Update
        // _____________________________________________________________________
        //

        if (IsKeyPressed(KEY_A))
        {
            createWindow((Window){
                .x = GetRandomValue(0, RENDER_WIDTH - 200),
                .y = GetRandomValue(0, RENDER_HEIGHT - 100),
                .width = 200,
                .height = 100,
                .minWidth = 125,
                .minHeight = 100,
                .resizable = true,
                .function = messageBoxWindow,
                .title = "New window",
                .message = "hello world",
                .icon = IC_ERROR});
        }

        SetMouseCursor(cursor);
        cursor = MOUSE_CURSOR_DEFAULT;

        if (IsKeyDown(KEY_UP)) player1.y -= 3.0f;
        else if (IsKeyDown(KEY_DOWN)) player1.y += 3.0f;
        if (IsKeyDown(KEY_RIGHT)) player1.x += 3.0f;
        else if (IsKeyDown(KEY_LEFT)) player1.x -= 3.0f;

        camera1.target = (Vector2){ player1.x, player1.y };

        // _____________________________________________________________________
        //
        //  Window focusing
        // _____________________________________________________________________
        //

        for (int i = WINDOW_LIMIT - 1; i > -1; i--)
        {
            Window win = windows[i];
            if (!win.active || win.minimized) continue;

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mcollide(win.x, win.y, win.width, win.height))
            {
                focusWindow(i);
                moving = false;
                resizing = false;
                break;
            }
        }

        // _____________________________________________________________________
        //
        //  Window movement
        // _____________________________________________________________________
        //

        Window *win = &windows[WINDOW_LIMIT - 1];

        if (lmbup)
        {
            moving = false;
            resizing = false;
        }

        // if titlebar is clicked on, start moving the window
        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && mcollide(win->x, win->y, win->width - 40, 16))
        {
            moving = true;
            resizing = false;
            hook.x = GetMouseX() - win->x;
            hook.y = GetMouseY() - win->y;
        }

        // if window is being moved, update its location
        if (moving)
        {
            // if the window was maximized, restore it
            if (win->maximized)
            {
                win->maximized = false;

                win->width = win->oldPos.width;
                win->height = win->oldPos.height;
                win->x = GetMouseX() - win->width / 2;
                win->y = 0;

                hook.x = GetMouseX() - win->x;
                hook.y = GetMouseY() - win->y;
            }
            cursor = MOUSE_CURSOR_RESIZE_ALL;
            win->x = GetMouseX() - hook.x;
            win->y = GetMouseY() - hook.y;
        }

        // _____________________________________________________________________
        //
        //  Window resizing
        // _____________________________________________________________________
        //

        // if bottom right corner is hovered over, change the cursor
        if (mcollide(win->x + win->width - 4, win->y + win->height - 4, 8, 8) && win->resizable)
        {
            cursor = MOUSE_CURSOR_RESIZE_NWSE;
            // if bottom right corner is clicked, start resizing
            if (lmbdown)
            {
                moving = false;
                resizing = true;
            }
        }

        if (resizing)
        {
            win->width = GetMouseX() - win->x;
            win->height = GetMouseY() - win->y;

            // make sure the window is not below its minimum size
            if (win->width < win->minWidth)
                win->width = win->minWidth;
            if (win->height < win->minHeight)
                win->height = win->minHeight;
        }

        // _____________________________________________________________________
        //
        //  Draw wallpaper
        // _____________________________________________________________________
        //

        BeginTextureMode(rt);
        ClearBackground(BLACK);
        BeginMode2D(camera1);


        // Draw full scene with first camera
        for (int i = camera1.target.x/PLAYER_SIZE - SCREEN_WIDTH/(4*PLAYER_SIZE); i < camera1.target.x/PLAYER_SIZE + SCREEN_WIDTH/(3*PLAYER_SIZE); i++)
        {
            if (i < 0) continue;
            if (i > 2001) break;
            DrawLineV((Vector2){(float)PLAYER_SIZE*i, (float)fmax(camera1.target.y - 200, 0)}, (Vector2){ (float)PLAYER_SIZE*i, (float)fmin(camera1.target.y + 350, 1001*PLAYER_SIZE)}, LIGHTGRAY);
        }

        for (int i = camera1.target.y/PLAYER_SIZE - SCREEN_HEIGHT/(2*PLAYER_SIZE); i < camera1.target.y/PLAYER_SIZE + SCREEN_HEIGHT/(2*PLAYER_SIZE); i++)
        {
            if (i < 0) continue;
            if (i > 1001) break;
            DrawLineV((Vector2){(float)fmax(camera1.target.x - 450, 0), (float)PLAYER_SIZE*i}, (Vector2){ (float)fmin(camera1.target.x + 550, 2001*PLAYER_SIZE), (float)PLAYER_SIZE*i}, LIGHTGRAY);
        }

        //for (int i = camera1.target.x/PLAYER_SIZE - 12; i < camera1.target.x/PLAYER_SIZE + 13; i++)
        //{
        //    for (int j = camera1.target.y/PLAYER_SIZE - 6; j < camera1.target.y/PLAYER_SIZE + 8; j++)
        //    {
        //        if (i < 0 || j < 0) continue;
        //        if (i > 2000 || j > 1000) continue;
        //        DrawText(TextFormat("[%i,\n%i]", i, j), 10 + PLAYER_SIZE*i, 15 + PLAYER_SIZE*j, 1, LIGHTGRAY);
        //    }
        //}

        DrawRectangleRec(player1, RED);
        EndMode2D();


        // draw tiled/scaled background
        //if (TILED_BACKGROUND)
        //{
        //    DrawTextureTiled(
        //        bg, (Rectangle){0, 0, bg.width, bg.height},
        //        (Rectangle){0, 0, RENDER_WIDTH, RENDER_HEIGHT},
        //        (Vector2){0, 0}, 0.0f, 1.0f, WHITE);
        //}
        //else
        //{
        //    DrawTexturePro(
        //        bg, (Rectangle){0, 0, bg.width, bg.height},
        //        (Rectangle){0, 0, RENDER_WIDTH, RENDER_HEIGHT},
        //        (Vector2){0, 0}, 0.0f, WHITE);
        //}

        // _____________________________________________________________________
        //
        //  Draw windows
        // _____________________________________________________________________
        //

        for (int i = 0; i < WINDOW_LIMIT + 1; i++)
        {
            Window *win = &windows[i];
            if (!win->active || win->minimized) continue;

            // draw window shadow
            DrawRectangle(
                win->x + SHADOW_OFFSET.x, win->y + SHADOW_OFFSET.y,
                win->width, win->height, SHADOW_COLOR);

            // draw window background and titlebar
            DrawRectangle(win->x, win->y, win->width, win->height, WINDOW_BG_COLOR);
            DrawRectangle(
                win->x + 1, win->y + 1, win->width - 2, 14,
                focused(i) ? TITLE_BG_COLOR : TITLE_UNFOCUSED_COLOR);

            // draw title text
            const char *title = win->title;
            if (resizing && focused(i))
                title = TextFormat("%d x %d", win->width, win->height);
            else if (moving && focused(i))
                title = TextFormat("%d, %d", win->x, win->y);
            DrawTextEx(boldFont, title, (Vector2){win->x + 2, win->y + 2}, FONT_SIZE, 0.0f, TITLE_TEXT_COLOR);

            // _________________________________________________________________
            //
            //  Draw window buttons
            // _________________________________________________________________
            //

            // close button
            bool hoverclose = mcollide(win->x + win->width - 14, win->y + 2, 12, 12);
            DrawTexture(
                winButtons[3 + (lmbdown && hoverclose) * 4],
                win->x + win->width - 14, win->y + 2, WHITE);
            if (hoverclose && lmbup) win->active = false;

            // maximize/restore button
            bool hovermax = mcollide(win->x + win->width - 27, win->y + 2, 12, 12);
            DrawTexture(
                winButtons[1 + win->maximized + (lmbdown && hovermax) * 4],
                win->x + win->width - 27, win->y + 2, WHITE);
            if (hovermax && lmbup)
            {
                win->maximized = !win->maximized;
                if (win->maximized)
                {
                    // if window is maximized, save its old coords in oldPos
                    win->oldPos = (Rectangle){win->x, win->y, win->width, win->height};
                    win->x = 0;
                    win->y = 0;
                    win->width = RENDER_WIDTH;
                    win->height = RENDER_HEIGHT - 18;
                }
                else
                {
                    // if window is restored, retrieve its coords from oldPos
                    win->x = win->oldPos.x;
                    win->y = win->oldPos.y;
                    win->width = win->oldPos.width;
                    win->height = win->oldPos.height;
                }
            }

            // minimize button
            bool hovermin = mcollide(win->x + win->width - 40, win->y + 2, 12, 12);
            DrawTexture(
                winButtons[0 + (lmbdown && hovermin) * 4],
                win->x + win->width - 40, win->y + 2, WHITE);
            if (hovermin && lmbup) win->minimized = true;

            // force window to be at least partially on screen
            if (win->x > RENDER_WIDTH - 5)
                win->x = RENDER_WIDTH - 5;
            if (win->y > RENDER_HEIGHT - 20)
                win->y = RENDER_HEIGHT - 20;
            if (win->width < 50)
                win->width = 50;
            if (win->height < 25)
                win->width = 24;

            win->function(win, i);
        }

#ifdef DEBUG_MOVERESIZE
        DrawText(TextFormat("Moving: %d  Resizing: %d", moving, resizing), 0, 0, 10, WHITE);
#endif

        // _____________________________________________________________________
        //
        //  Draw taskbar
        // _____________________________________________________________________
        //

        bool starthover = mcollide(1, RENDER_HEIGHT - 17, 48, 16);
        DrawRectangle(0, RENDER_HEIGHT - 18, RENDER_WIDTH, 18, TASKBAR_BG_COLOR);
        DrawTexture(startButtons[starthover && lmbdown], 1, RENDER_HEIGHT - 17, WHITE);

        if (starthover && lmbup)
        {
            createWindow((Window){
                .x = 0,
                .y = RENDER_HEIGHT / 2,
                .width = 100,
                .height = RENDER_HEIGHT / 2 - 18,
                .title = "Start menu",
                .function = startMenuWindow});
        }

        // draw buttons for minimized windows
        int x = 50;
        for (int i = 0; i < WINDOW_LIMIT; i++)
        {
            if (!windows[i].minimized) continue;

            bool winbtnhover = mcollide(x, RENDER_HEIGHT - 17, 96, 16);
            DrawTexture(largeButtons[winbtnhover && lmbdown], x, RENDER_HEIGHT - 17, WHITE);
            DrawTextEx(
                font, windows[i].title, (Vector2){x + 1, RENDER_HEIGHT - 16},
                FONT_SIZE, 0.0f, TASKBAR_TEXT_COLOR);

            if (winbtnhover && lmbup)
            {
                windows[i].minimized = false;
                focusWindow(i);
            }

            x += 97;
        }

        // draw current time on the taskbar
        time_t t = time(NULL);
        struct tm *tm = localtime(&t);

        strftime(timebuf, 16, "%H:%M:%S", tm);
        DrawTextEx(
            font, timebuf,
            (Vector2){RENDER_WIDTH - MeasureTextEx(font, timebuf, FONT_SIZE, 0.0f).x - 3, RENDER_HEIGHT - 15},
            FONT_SIZE, 0.0f, TASKBAR_TEXT_COLOR);

        // _____________________________________________________________________
        //
        //  Render to screen
        // _____________________________________________________________________
        //

        EndTextureMode();
        BeginDrawing();

        // render textures have to be vertically flipped when drawing them
        DrawTexturePro(
            rt.texture,
            (Rectangle){0, 0, RENDER_WIDTH, -RENDER_HEIGHT},
            (Rectangle){0, 0, SCREEN_WIDTH, SCREEN_HEIGHT},
            (Vector2){0, 0}, 0.0f, WHITE);

        EndDrawing();
    }

    // _________________________________________________________________________
    //
    //  Unload and exit
    // _________________________________________________________________________
    //

    UnloadFont(font);
    UnloadFont(boldFont);
    UnloadTexture(bg);
    UnloadRenderTexture(rt);

    for (int i = 0; i < IC_COUNT; i++) UnloadTexture(icons[i]);
    for (int i = 0; i < 8; i++) UnloadTexture(winButtons[i]);
    
    // small,large,start
    for (int i = 0; i < 2; i++)
    {
        UnloadTexture(smallButtons[i]);
        UnloadTexture(largeButtons[i]);
        UnloadTexture(startButtons[i]);
    }

    CloseWindow();
    return 0;
}