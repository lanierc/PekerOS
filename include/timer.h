#ifndef TIMER_H
#define TIMER_H

// PIT (Programmable Interval Timer) fonksiyonları
void init_timer(unsigned int frequency);

// Zamanlama fonksiyonları
unsigned int timer_get_ticks();
unsigned int timer_get_seconds();
void sleep(unsigned int ms);

#endif
