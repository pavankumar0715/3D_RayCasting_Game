#include <iostream>
#include <vector>
#include <utility>
#include <algorithm>
#include <chrono>
#include <stdio.h>
#include <math.h>
#include <ncurses.h>

using namespace std;
using namespace chrono;

int main()
{

    initscr();             // Initialises the window
    noecho();              // to disable automatic echoing of user input
    curs_set(0);           // to hide the cursor
    keypad(stdscr, TRUE);  // to identify special keys
    nodelay(stdscr, TRUE); // to disable waiting for input from getch()

    // Create Map of world space (# is wall block, . is empty space)
    wstring map;
    map += L"#########.......";
    map += L"#...............";
    map += L"#.......########";
    map += L"#..............#";
    map += L"#.....###......#";
    map += L"#......##......#";
    map += L"#..............#";
    map += L"###............#";
    map += L"###............#";
    map += L"#......####..###";
    map += L"#......#.......#";
    map += L"#......#.......#";
    map += L"#..............#";
    map += L"#......#########";
    map += L"#..............#";
    map += L"################";

    // Initialise dimensions of the window
    int screenWidth = 190;
    int screenHeight = 52;
    int mapWidth = 16;
    int mapHeight = 16;

    // Inititalise the window
    WINDOW *win = newwin(screenHeight, screenWidth, 0, 0);
    wrefresh(win);

    // Initialise player position
    float playerX = 12.0f;
    float playerY = 5.0f;
    float playerA = 0.0f;        // Angle of player in radians
    float fov = 3.14159f / 3.0f; // Field of view in radians
    float depth = 16.0f;
    float playerSpeed = 45.0f;   // Change the Speed as required

    // Initialise the screen buffer
    wchar_t *screen = new wchar_t[screenWidth * screenHeight];

    auto t1 = system_clock::now();
    auto t2 = system_clock::now();

    int tempval = 0;
    while (1)
    {
        // To measure each frame duration
        t2 = system_clock::now();
        duration<float> elapsedTime = t2 - t1;
        t1 = t2;
        float frameTimePeriod = elapsedTime.count();

        // Handle left rotation
        if (tempval == 'a')
            playerA -= (playerSpeed * 0.75f) * frameTimePeriod;

        // Handle right rotation
        if (tempval == 'd')
            playerA += (playerSpeed * 0.75f) * frameTimePeriod;

        // Handle forward movement & collision
        if (tempval == 'w')
        {
            playerX += sinf(playerA) * playerSpeed * frameTimePeriod;
            playerY += cosf(playerA) * playerSpeed * frameTimePeriod;
            if (map.c_str()[(int)playerX * mapWidth + (int)playerY] == '#')
            {
                playerX -= sinf(playerA) * playerSpeed * frameTimePeriod;
                playerY -= cosf(playerA) * playerSpeed * frameTimePeriod;
            }
        }

        // Handle backward movement & collision
        if (tempval == 's')
        {
            playerX -= sinf(playerA) * playerSpeed * frameTimePeriod;
            playerY -= cosf(playerA) * playerSpeed * frameTimePeriod;
            if (map.c_str()[(int)playerX * mapWidth + (int)playerY] == '#')
            {
                playerX += sinf(playerA) * playerSpeed * frameTimePeriod;
                playerY += cosf(playerA) * playerSpeed * frameTimePeriod;
            }
        }

        // Angle of corner to eye
        // to quit the game
        if (tempval == 'q')
            break;

        // Decide each character, navigate by column then by row
        for (int x = 0; x < screenWidth; x++)
        {
            // For each column, calculate the projected ray angle into world space
            float rayAngle = (playerA - fov / 2.0f) + ((float)x / (float)screenWidth) * fov;

            // Find distance to wall
            float stepSize = 0.1f; // smaller step size gives greater resolution
            float distanceToWall = 0.0f;

            bool hitWall = false;  // when ray hits wall block
            bool boundary = false; // when ray hits boundary between two wall blocks

            float eyeX = sinf(rayAngle); // Unit vector for ray in player space
            float eyeY = cosf(rayAngle);

            // Incrementally cast ray from player, along ray angle, testing for
            // intersection with a block
            while (!hitWall && distanceToWall < depth)
            {
                distanceToWall += stepSize;
                int testX = (int)(playerX + eyeX * distanceToWall);
                int testY = (int)(playerY + eyeY * distanceToWall);

                // Test if ray is out of bounds
                if (testX < 0 || testX >= mapWidth || testY < 0 || testY >= mapHeight)
                {
                    hitWall = true; // Just set distance to maximum depth
                    distanceToWall = depth;
                }
                else
                {
                    // Ray is inbounds so test to see if the ray cell is a wall block
                    if (map.c_str()[testX * mapWidth + testY] == '#')
                    {
                        // Ray has hit wall
                        hitWall = true;

                        // To highlight cell boundaries (each cell is "#" in the map)
                        // Test each corner of hit tile, storing the distance from
                        // the player, and the calculated dot product of the two rays
                        vector<pair<float, float>> p;

                        for (int tx = 0; tx < 2; tx++)
                            for (int ty = 0; ty < 2; ty++)
                            {
                                float vy = (float)testY + ty - playerY;
                                float vx = (float)testX + tx - playerX;
                                float d = sqrt(vx * vx + vy * vy);
                                float dot = (eyeX * vx / d) + (eyeY * vy / d);
                                p.push_back(make_pair(d, dot));
                            }

                        // Sort Pairs from closest to farthest
                        sort(p.begin(), p.end(), [](const pair<float, float> &left, const pair<float, float> &right)
                             { return left.first < right.first; });

                        // Since we see four edges of a cube in 3d
                        float bound = 0.01;
                        if (acos(p[0].second) < bound)
                            boundary = true;
                        if (acos(p[1].second) < bound)
                            boundary = true;
                        if (acos(p[2].second) < bound)
                            boundary = true;
                    }
                }
            }

            // Calculate distance to ceiling and floor
            int nCeiling = (float)(screenHeight / 2.0) - screenHeight / ((float)distanceToWall);
            int nFloor = screenHeight - nCeiling;

            // Shader walls based on distance, unicodes do not work on all machines
            short shade = ' ';
            if (distanceToWall <= depth / 4.0f)
                shade = '~'; // Very close
            else if (distanceToWall < depth / 3.0f)
                shade = '~';
            else if (distanceToWall < depth / 2.0f)
                shade = '~';
            else if (distanceToWall < depth)
                shade = '-';
            else
                shade = ' '; // Too far away

            if (boundary)
                shade = ' '; // Black it out

            for (int y = 0; y < screenHeight; y++)
            {
                // Each Row
                if (y <= nCeiling)
                    screen[y * screenWidth + x] = ' ';
                else if (y > nCeiling && y <= nFloor)
                    screen[y * screenWidth + x] = shade;
                else // Floor
                {
                    // Shade floor based on distance
                    float b = 1.0f - (((float)y - screenHeight / 2.0f) / ((float)screenHeight / 2.0f));
                    if (b < 0.25)
                        shade = '#';
                    else if (b < 0.5)
                        shade = 'x';
                    else if (b < 0.75)
                        shade = '.';
                    else if (b < 0.9)
                        shade = '-';
                    else
                        shade = ' ';
                    screen[y * screenWidth + x] = shade;
                }
            }
        }

        // Display Map
        for (int nx = 0; nx < mapWidth; nx++)
            for (int ny = 0; ny < mapWidth; ny++)
            {
                screen[(ny + 1) * screenWidth + nx] = map[ny * mapWidth + nx];
            }
        screen[((int)playerX + 1) * screenWidth + (int)playerY] = 'P';

        // Display Frame
        for (int y = 0; y < screenHeight; y++)
        {
            for (int x = 0; x < screenWidth; x++)
            {
                mvwaddch(win, y + 1, x + 1, screen[y * screenWidth + x]);
            }
        }

        // Display Stats
        mvwprintw(win, 0, 0, "X=%3.2f, Y=%3.2f, A=%3.2f FPS=%3.2f ", playerX, playerY, playerA, 1.0f / frameTimePeriod);

        wrefresh(win);     // refreshes window to match whats in memory
        tempval = getch(); // waits for user input and returns int value
    }

    endwin(); // deallocates the memory and ends ncurses

    delete[] screen;

    return 0;
}
