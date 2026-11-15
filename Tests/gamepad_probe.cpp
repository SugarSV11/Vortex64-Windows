// Vortex64-Windows: manual gamepad probe (not part of ctest: it needs a person and a controller).
// Lists the controllers SDL sees (with the SDL GameController mapping state) and prints every button / axis
// event for the given number of seconds (default 20). Used to check a pad before configuring it in Vortex64.
//   gamepad-probe.exe [seconds]
#include <SDL.h>
#include <cstdio>
#include <cstdlib>

int main(int argc, char ** argv)
{
    const int Seconds = argc > 1 ? atoi(argv[1]) : 20;
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
    {
        printf("SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Delay(1000);
    SDL_PumpEvents();
    const int Count = SDL_NumJoysticks();
    printf("%d controller(s) detected\n", Count);
    for (int i = 0; i < Count; i++)
    {
        printf("  [%d] %s | SDL game controller mapping: %s\n", i, SDL_JoystickNameForIndex(i), SDL_IsGameController(i) ? "yes" : "no");
        if (SDL_IsGameController(i))
        {
            SDL_GameControllerOpen(i);
        }
        else
        {
            SDL_JoystickOpen(i);
        }
    }
    if (Count == 0)
    {
        printf("no controller: connect one and run again\n");
        SDL_Quit();
        return 2;
    }
    printf("press buttons / move sticks for %d s...\n", Seconds);
    const Uint32 End = SDL_GetTicks() + (Uint32)Seconds * 1000;
    int Events = 0;
    while (SDL_GetTicks() < End)
    {
        SDL_Event e;
        while (SDL_PollEvent(&e))
        {
            switch (e.type)
            {
            case SDL_CONTROLLERBUTTONDOWN: printf("button %s down\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)e.cbutton.button)); Events++; break;
            case SDL_CONTROLLERBUTTONUP: printf("button %s up\n", SDL_GameControllerGetStringForButton((SDL_GameControllerButton)e.cbutton.button)); Events++; break;
            case SDL_CONTROLLERAXISMOTION:
                if (abs(e.caxis.value) > 8000) { printf("axis %s = %d\n", SDL_GameControllerGetStringForAxis((SDL_GameControllerAxis)e.caxis.axis), e.caxis.value); Events++; }
                break;
            case SDL_JOYBUTTONDOWN: printf("joy button %d down\n", e.jbutton.button); Events++; break;
            case SDL_JOYAXISMOTION: if (abs(e.jaxis.value) > 8000) { printf("joy axis %d = %d\n", e.jaxis.axis, e.jaxis.value); Events++; } break;
            default: break;
            }
        }
        SDL_Delay(5);
    }
    printf("%d event(s) received\n", Events);
    SDL_Quit();
    return Events > 0 ? 0 : 3;
}
