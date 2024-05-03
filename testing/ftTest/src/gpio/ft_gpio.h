#ifndef _FT_GPIO_H_
#define _FT_GPIO_H_

#include <stdint.h>


/*INPUT TESET*/
void ft_gpio_in_oddhigh_evenlow_test(void);
void ft_gpio_in_oddlow_evenhigh_test(void);

/*OUTPUT TESET*/
void ft_gpio_out_oddhigh_evenlow_test(void);
void ft_gpio_out_oddlow_evenhigh_test(void);

/*Pullup/Pulldown TESET*/
void ft_gpio_oddpu_evenpd_test(void);
void ft_gpio_oddpd_evenpu_test(void);

#ifdef GPIO_THRESHOLD_TEST
/*Threshold test*/
void ft_gpio_in_threshold_test(void);
#endif

#ifdef GPIO_ROLLOVER_TEST
/*Rollover test*/
void ft_gpio_rollover_test(void);
#endif

#ifdef GPIO_BANGDING_TEST
/*Rollover test*/
void ft_gpio_banding_test(void);
#endif


#endif	//_GPIO_H_


