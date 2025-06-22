/*
 * wifi.h
 *
 *  Created on: 15 dic. 2020
 *      Author: jcgar
 *  Modified on: 11 May. 2025
 *  	Author: NRR
 */

#ifndef MAIN_WIFI_H_
#define MAIN_WIFI_H_


extern void wifi_init_sta(void);
extern void wifi_init_softap(void);
extern void wifi_initlib();
extern void wifi_change_to_AP(void);
extern void wifi_change_to_sta(void);

extern bool wifi_is_connected(void);

#endif /* MAIN_WIFI_H_ */
