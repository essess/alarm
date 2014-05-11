/**
 * Copyright (c) 2014 Sean Stasiak. All rights reserved.
 * Developed by: Sean Stasiak <sstasiak@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * with the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 *   -Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimers.
 *
 *   -Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimers in the documentation
 *    and/or other materials provided with the distribution.
 *
 *   -Neither Sean Stasiak, nor the names of other contributors may be used to
 *    endorse or promote products derived from this Software without specific
 *    prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS WITH
 * THE SOFTWARE.
 */

#include <stdlib.h>
#include <stdint.h>
#include "LPC11xx.h"

/**
 * -----------------------------------------
 */
static inline void
  drv_in1_low( void )
{
  LPC_GPIO3->DATA &= ~(1ul<<0);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_in1_hi( void )
{
  LPC_GPIO3->DATA |= (1ul<<0);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_in2_low( void )
{
  LPC_GPIO3->DATA &= ~(1ul<<1);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_in2_hi( void )
{
  LPC_GPIO3->DATA |= (1ul<<1);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_ena_low( void )
{
  LPC_GPIO3->DATA &= ~(1ul<<2);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_ena_hi( void )
{
  LPC_GPIO3->DATA |= (1ul<<2);
}

/**
 * -----------------------------------------
 */
static inline void
  drv_init( void )
{
  drv_in1_low();
  drv_in2_low();
  drv_ena_low();
  LPC_GPIO3->DIR |= (1ul<<0);
  LPC_GPIO3->DIR |= (1ul<<1);
  LPC_GPIO3->DIR |= (1ul<<2);
}

/**
 * -----------------------------------------
 */
static inline int
  sw_on( void )
{
  return !(LPC_GPIO3->DATA & (1ul<<3));
}

/**
 * -----------------------------------------
 */
static inline int
  do_buzz( void )
{
  return LPC_GPIO0->DATA & (1ul<<2);
}

/**
 * -----------------------------------------
 */
static inline void
  ld1_on( void )
{
  LPC_GPIO1->DATA &= ~(1ul<<8);
}

/**
 * -----------------------------------------
 */
static inline void
  ld1_off( void )
{
  LPC_GPIO1->DATA |= (1ul<<8);
}

/**
 * -----------------------------------------
 */
static inline void
  ld2_on( void )
{
  LPC_GPIO0->DATA &= ~(1ul<<1);
}

/**
 * -----------------------------------------
 */
static inline void
  ld2_off( void )
{
  LPC_GPIO0->DATA |= (1ul<<1);
}

/**
 * -----------------------------------------
 */
static inline void
  led_init( void )
{
  ld1_off();
  ld2_off();
  LPC_GPIO1->DIR |= (1ul<<8);
  LPC_GPIO0->DIR |= (1ul<<1);
}

/**
 * -----------------------------------------
 */
static inline void
  tmr_init( void )
{
  SysTick->LOAD = (1*48000)-1;        /**< 1ms @ 48MHz */
}

/**
 * -----------------------------------------
 */
static inline void
  tmr_wait( unsigned ms )
{
  SysTick->VAL  = ~0;
  SysTick->CTRL = (1ul<<2)|(1ul<<0);
  while( ms-- )  /**< need to manually prescale because systick is only 24bits */
    while(!(SysTick->CTRL&(1ul<<16)));
  SysTick->CTRL = 0;
}

/**
 * -----------------------------------------
 */
static inline void
  standby( void )
{
  drv_in1_hi();
  drv_in2_low();
}

/**
 * -----------------------------------------
 */
static inline void
  visual( void )
{
  drv_in1_low();
  tmr_wait( 10 );
  standby();
}

/**
 * -----------------------------------------
 */
static inline void
  command( void )
{
  drv_in1_low();
  drv_in2_hi();
  tmr_wait( 1 );
}

/**
 * -----------------------------------------
 */
static inline void
  horn_on( void )
{
  command();
  drv_in2_low();
  tmr_wait( 1 );
  drv_in2_hi();
  tmr_wait( 2 );
  standby();
}

/**
 * -----------------------------------------
 */
static inline void
  horn_off( void )
{
  command();
  drv_in2_low();
  tmr_wait( 2 );
  drv_in2_hi();
  tmr_wait( 1 );
  standby();
}

/**
 * -----------------------------------------
 */
static inline void
  temporal( void )
{
  /* 3 buzzes once - do 1:8 w/visual for NFPA code */
  command();
  drv_in2_low();
  tmr_wait( 3 );
  standby();
}

/**
 * -----------------------------------------
 */
static inline void
  march_time( void )
{
  /* continuous temporal and automatically clears the horn when put in standby */
  command();
  tmr_wait( 2 );
  drv_in2_low();
  tmr_wait( 1 );
  standby();
}

/**
 * -----------------------------------------
 */
int
  main( int  argc,
        char *argv[] )
{
  (void)argc;
  (void)argv;

  drv_init();
  led_init();
  tmr_init();

  drv_ena_hi();
  standby();
  tmr_wait( 1000 );                   /**< allow device(s) charge time  */

  while( 1 )
  {
    if( sw_on() )                     /**< poll switch state            */
    {
      ld1_on();
      tmr_wait( 100 );                /**< debounce ...                 */
      if( sw_on() )                   /**< once stable, gen alarm data  */
      {
        ld2_on();
        if( do_buzz() )
        {
          march_time();
          tmr_wait( 976 );
        }
        while( sw_on() )
        {
          visual();
          tmr_wait( 970 );
        }
        horn_off();
        ld2_off();
      }
      tmr_wait( 400 );                /**< trigger lockout              */
      ld1_off();
    }
    tmr_wait( 100 );
  }

  return EXIT_SUCCESS;
}