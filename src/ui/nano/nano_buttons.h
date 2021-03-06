#ifndef NANO_BUTTONS_H
#define NANO_BUTTONS_H

#include <stdint.h>

int8_t button_main_menu(uint8_t button_mask);
int8_t button_about(uint8_t button_mask);
void button_version(uint8_t button_mask);
int8_t button_more_info(uint8_t button_mask);
int8_t button_bip_path(uint8_t button_mask);
int8_t button_disp_addr(uint8_t button_mask);
void button_disp_addr_chk(uint8_t button_mask);
int8_t button_tx_addr(uint8_t button_mask);
void button_prompt_tx(uint8_t button_mask);
void button_handle_menu_idx(uint8_t button_mask, uint8_t array_sz);

#endif // NANO_BUTTONS_H
