#include <string.h>
#include "iota/addresses.h"
#include "glyphs.h"
#include "api.h"
#include "ui.h"
#include "ui_common.h"
#include "nano_misc.h"
#include "nano_core.h"

// go to state with menu index
void state_go(uint8_t state, uint8_t idx)
{
    ui_state.state = state;
    ui_state.menu_idx = idx;
}

void backup_state()
{
    ui_state.backup_state = ui_state.state;
    ui_state.backup_menu_idx = ui_state.menu_idx;
}

void restore_state()
{
    state_go(ui_state.backup_state, ui_state.backup_menu_idx);

    ui_state.backup_state = STATE_MAIN_MENU;
    ui_state.backup_menu_idx = 0;
}

void abbreviate_addr(char *dest, const char *src)
{
    // copy the abbreviated address over
    strncpy(dest, src, 4);
    strncpy(dest + 4, " ... ", 5);
    strncpy(dest + 9, src + 77, 4);
    dest[13] = '\0';
}

/** @brief Returns buffer for corresponding position. */
static char *get_str_buffer(UI_TEXT_POS pos)
{
    switch (pos) {
    case TOP_H:
    case TOP:
        return ui_text.top_str;
    case BOT:
    case BOT_H:
        return ui_text.bot_str;
    case MID:
        return ui_text.mid_str;
#ifdef TARGET_NANOX
    case POS_X:
        return ui_text.x_str;
#endif
    default:
        THROW(INVALID_PARAMETER);
    }
}

void write_display(const char *string, UI_TEXT_POS pos)
{
    char *msg = get_str_buffer(pos);

    // NULL value sets line blank
    if (string == NULL) {
        msg[0] = '\0';
        return;
    }
    snprintf(msg, TEXT_LEN, "%s", string);
}

/* --------- STATE RELATED FUNCTIONS ----------- */

// Turns a single glyph on or off
void glyph_on(UI_GLYPH_TYPES_NANO g)
{
    FLAG_ON(g);
}

static void clear_text()
{
    write_display(NULL, TOP);
    write_display(NULL, MID);
    write_display(NULL, BOT);
#ifdef TARGET_NANOX
    write_display(NULL, POS_X);
#endif
}

static void clear_glyphs()
{
    // turn off all glyphs
    FLAG_OFF(GLYPH_UP);
    FLAG_OFF(GLYPH_DOWN);
    FLAG_OFF(GLYPH_LOAD);
    FLAG_OFF(GLYPH_DASH);
    FLAG_OFF(GLYPH_IOTA);
    FLAG_OFF(GLYPH_BACK);
#ifdef TARGET_NANOS
    FLAG_OFF(GLYPH_CONFIRM);
#else // NANOX
    FLAG_OFF(GLYPH_INFO);
    FLAG_OFF(GLYPH_CHECK);
    FLAG_OFF(GLYPH_CROSS);
#endif
}

void clear_display()
{
    clear_text();
    clear_glyphs();
}

// turns on 2 glyphs (often glyph on left + right)
void display_glyphs(UI_GLYPH_TYPES_NANO g1, UI_GLYPH_TYPES_NANO g2)
{
    clear_glyphs();

    // turn on ones we want
    glyph_on(g1);
    glyph_on(g2);
}

// combine glyphs with bars along top for confirm
void display_glyphs_confirm(UI_GLYPH_TYPES_NANO g1, UI_GLYPH_TYPES_NANO g2)
{
    clear_glyphs();

    // turn on ones we want
#ifdef TARGET_NANOS
    glyph_on(GLYPH_CONFIRM);
#endif
    glyph_on(g1);
    glyph_on(g2);
}

void write_text_array(const char *array, uint8_t len)
{
    clear_display();
    clear_glyphs();

#ifdef TARGET_NANOS
    if (ui_state.menu_idx > 0) {
        write_display(array + (TEXT_LEN * (ui_state.menu_idx - 1)), TOP_H);
        glyph_on(GLYPH_UP);
    }

    write_display(array + (TEXT_LEN * ui_state.menu_idx), MID);

    if (ui_state.menu_idx < len - 1) {
        write_display(array + (TEXT_LEN * (ui_state.menu_idx + 1)), BOT_H);
        glyph_on(GLYPH_DOWN);
    }
#else
    if (ui_state.menu_idx > 0) {
        write_display(array + (TEXT_LEN * (ui_state.menu_idx - 1)), TOP_H);
        glyph_on(GLYPH_UP);
    }

    write_display(array + (TEXT_LEN * ui_state.menu_idx), MID);

    if (ui_state.menu_idx < len - 2) {
        write_display(array + (TEXT_LEN * (ui_state.menu_idx + 1)), BOT_H);
        glyph_on(GLYPH_DOWN);
    }
    if (ui_state.menu_idx < len - 1) {
        write_display(array + (TEXT_LEN * (ui_state.menu_idx + 2)), POS_X);
        glyph_on(GLYPH_DOWN);
    }
#endif
}

/* --------- FUNCTIONS FOR DISPLAYING BALANCE ----------- */

// displays full/readable value based on the ui_state
static bool display_value(int64_t val, UI_TEXT_POS pos)
{
    if (ui_state.display_full_value || ABS(val) < 1000) {
        write_full_val(val, get_str_buffer(pos), TEXT_LEN);
    }
    else {
        write_readable_val(val, get_str_buffer(pos), TEXT_LEN);
    }

    // return whether a shortened version is possible
    return ABS(val) >= 1000;
}

// swap between full value and readable value
void value_convert_readability()
{
    ui_state.display_full_value = !ui_state.display_full_value;
}

void display_advanced_tx_value()
{
    ui_state.val = api.bundle_ctx.values[menu_to_tx_idx()];

    if (ui_state.val > 0) { // outgoing tx
        // -1 is deny, -2 approve, -3 addr, -4 val of change
        if (ui_state.menu_idx == get_tx_arr_sz() - 4) {
            char msg[TEXT_LEN];
            // write the index along with Change
            snprintf(msg, sizeof msg, "Change: [%u]",
                     (unsigned int)
                         api.bundle_ctx.indices[api.bundle_ctx.last_tx_index]);

            write_display(msg, TOP);
        }
        else
            write_display("Output:", TOP);
    }
    else {
        // input tx (skip meta)
        char msg[TEXT_LEN];
        snprintf(msg, sizeof msg, "Input: [%u]",
                 (unsigned int)api.bundle_ctx.indices[menu_to_tx_idx()]);

        write_display(msg, TOP);
        ui_state.val = -ui_state.val;
    }

    // display_value returns true if readable form is possible
    if (display_value(ui_state.val, BOT))
        display_glyphs_confirm(GLYPH_UP, GLYPH_DOWN);
    else
        display_glyphs(GLYPH_UP, GLYPH_DOWN);
}

void display_advanced_tx_address()
{
    const unsigned char *addr_bytes =
        bundle_get_address_bytes(&api.bundle_ctx, menu_to_tx_idx());

    get_address_with_checksum(addr_bytes, ui_state.addr);

    char abbrv[14];
    abbreviate_addr(abbrv, ui_state.addr);

    write_display(abbrv, TOP);
    write_display("Chk: ", BOT);

    // copy the remaining 9 chars in the buffer
    memcpy(ui_text.bot_str + 5, ui_state.addr + 81, 9);

    display_glyphs_confirm(GLYPH_UP, GLYPH_DOWN);
}

uint8_t get_tx_arr_sz()
{
    uint8_t counter = 0;

    for (unsigned int i = 0; i <= api.bundle_ctx.last_tx_index; i++) {
        if (api.bundle_ctx.values[i] != 0) {
            counter++;
        }
    }

    return (counter * 2) + 2;
}
