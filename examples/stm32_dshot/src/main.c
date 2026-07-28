/**
 * @file main.c
 * @brief STM32 DShot Digital ESC Example Main Entry Point.
 */

extern int main_bare(void);
extern int main_sched(void);

int main(void)
{
#if defined(USE_BARE_LOOP)
    return main_bare();
#else
    return main_sched();
#endif
}
