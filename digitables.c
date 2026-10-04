/* SPDX-License-Identifier: GPL-2.0-or-later
 * Copyright (C) 2026 irpina and contributors */
/* digitables: M8-style pitch tables for the Digitone mk1, OS 1.43.
 *
 * A table is up to 16 steps of semitone offsets, with a length and a loop
 * point; the project has 16 of them (the bank), saved and loaded with it.
 * Each note a voice starts on a sound whose TBL is on restarts that table
 * (core-dn1's ev_voice_on), and the sequencer's clock steps it: one step
 * every SPD ticks of 24 PPQN (6 are a 16th), or a fraction of a tick, down
 * to a step every audio block (MAX: 1500 a second, at any tempo). Each step
 * sets the voice's pitch word to the note it started on plus the step's
 * offset, which the render reads again every block: a block is as fast as
 * a pitch can change. After the last step it goes back to the loop point,
 * or holds the last step when the loop is off.
 *
 * The TBL page is AMP's third page: press AMP until it shows, or hold a
 * track key (T1-T4) on its own for core-dn1's Mod Menu and pick TABLES.
 * Its knobs are sound parameters (core-dn1's
 * parameter slots), kept in sound slots no stock knob uses and the sound
 * engine ignores, so the stock UI edits, shows and p-locks them:
 *   A  TBL  slot 26: OFF, or the table, 1-16
 *   B  SPD  slot 65: ticks of 24 PPQN a step, 1-24 (the slot holds 0-23),
 *           and faster below 1: 1/2 ... 1/32 of a tick (-1..-9), MAX (-10)
 * Hold a track key on the TBL page for the table editor (below).
 *
 * Addresses are OS 1.43's:
 *   0x41391f80  the voices' pitch words, note << 16
 *   0x80004614  the render's timeline: 5,400,000 units a 16th, 900,000 a
 *               24 PPQN tick, at any tempo
 *   0x800034c4  + 18 + 158 v + 2 k: voice v's parameter k (0x4009bec8)
 *   0x4138e220  the active kit; track t's sound slot k at + 0x2c + 326 t + 2 k
 */
#define PITCH     ((volatile unsigned long *)0x41391f80)
#define TIMELINE  (*(volatile unsigned long *)0x80004614)
#define VPARAM(v, k) (*(volatile short *)(0x800034c4 + 18 + 158 * (v) + 2 * (k)))
#define KIT       (*(volatile unsigned long *)0x4138e220)
#define SOUND_SLOT(t, k) (*(volatile short *)(KIT + 0x2c + 326 * (t) + 2 * (k)))

#define TICK      900000UL      /* timeline units in one 24 PPQN tick */
#define TBL_SLOT  26
#define SPD_SLOT  65
#define VOICES    8
#define SYNTHS    4             /* tracks 0-3 have voices; 4-7 are MIDI */

/* ---- The bank: 16 tables, saved with the project (core-dn1 2.3) -------- */
#define TABLES    16
#define STEPS     16
#define NOTE_MAX  48            /* offsets -48..+48 semitones */
#define LOOP_OFF  0xFF          /* hold the last step */

struct table {
    signed char step[STEPS];
    unsigned char len;          /* 1-16 */
    unsigned char loop;         /* 0..len-1, or LOOP_OFF */
    unsigned char pad[2];
};

struct table digitables_bank[TABLES];       /* global: the tests read it */

static void bank_defaults(void)
{
    static const signed char demo[] = {0, 12, 7, 3};
    int t, i;
    for (t = 0; t < TABLES; t++) {
        for (i = 0; i < STEPS; i++)
            digitables_bank[t].step[i] = 0;
        digitables_bank[t].len = STEPS;
        digitables_bank[t].loop = 0;
        digitables_bank[t].pad[0] = digitables_bank[t].pad[1] = 0;
    }
    for (i = 0; i < 4; i++)             /* table 1: an arpeggio to start */
        digitables_bank[0].step[i] = demo[i];
    digitables_bank[0].len = 4;
}

/* core_projdata's loaded(): the project's tables are in digitables_bank, or
 * zeros when it has none. A saved bank is checked as it comes in. */
static unsigned char bank_ready;       /* a project's tables are in */
unsigned short digitables_loads;            /* loads seen, and the last one's */
unsigned char digitables_load_found;        /* found: for tests and debugging */

void digitables_bank_loaded(int found)
{
    int t, i;
    bank_ready = 1;
    digitables_loads++;
    digitables_load_found = (unsigned char)found;
    if (!found) {
        bank_defaults();
        return;
    }
    for (t = 0; t < TABLES; t++) {
        struct table *b = &digitables_bank[t];
        if (b->len < 1 || b->len > STEPS)
            b->len = STEPS;
        if (b->loop != LOOP_OFF && b->loop >= b->len)
            b->loop = 0;
        for (i = 0; i < STEPS; i++) {
            if (b->step[i] > NOTE_MAX)
                b->step[i] = NOTE_MAX;
            else if (b->step[i] < -NOTE_MAX)
                b->step[i] = -NOTE_MAX;
        }
    }
}

const struct {
    char tag[4];
    long size;
    void *data;
    void (*loaded)(int found);
} digitables_projdata = { "TBL1", sizeof digitables_bank, digitables_bank, digitables_bank_loaded };

/* ---- TBL and SPD, parameters (core-dn1's parameter slots) --------------- */
#define TBL_ID    182
#define SPD_ID    183
#define SPD_MIN   (-10)                 /* MAX: a step every audio block */
#define SPD_MAX   23                    /* 24 ticks a step */
/* Faster than a tick (the slot at -1..-9): a step every 1/2 ... 1/32 of one. */
static const unsigned char spd_div[] = {2, 3, 4, 6, 8, 12, 16, 24, 32};
#define STR_AMP   ((const char *)0x401d63c3)   /* the firmware's "Amp" */
#define STR_EMPTY ((const char *)0x401ddbcd)   /* its empty string */
#define LOOK_PTIM 24            /* a plain knob, whole steps (core-dn1) */

struct param {
    long group, slot, min, max, def, flags, cc, w6, w7, w8;
    const char *name, *group_name, *short_name;
    void (*format)(int value, char *buf);
    const char *empty;
};

static char *put_num(char *buf, int v)
{
    if (v >= 10)
        *buf++ = (char)('0' + v / 10);
    *buf++ = (char)('0' + v % 10);
    *buf = 0;
    return buf;
}

/* A value's text, as the knob and its pop-up show it. */
void digitables_tbl_format(int value, char *buf)
{
    int v = value >> 8;
    if (v <= 0) {
        buf[0] = 'O'; buf[1] = 'F'; buf[2] = 'F'; buf[3] = 0;
        return;
    }
    put_num(buf, v);
}

void digitables_spd_format(int value, char *buf)
{
    int v = value >> 8;
    if (v >= 0) {
        put_num(buf, v + 1);                    /* ticks a step */
    } else if (v <= SPD_MIN) {
        buf[0] = 'M'; buf[1] = 'A'; buf[2] = 'X'; buf[3] = 0;
    } else {
        buf[0] = '1'; buf[1] = '/';             /* a fraction of a tick */
        put_num(buf + 2, spd_div[-v - 1]);
    }
}

/* core_params' entries: the id, the 60-byte record, the knob it borrows. */
const struct {
    long id;
    struct param rec;
    long look;
} digitables_tbl_param = {
    TBL_ID,
    { 1, TBL_SLOT, 0, TABLES << 8, 0, 0, -1, -1, -1, 0,
      "Table", STR_AMP, "TBL", digitables_tbl_format, STR_EMPTY },
    LOOK_PTIM
}, digitables_spd_param = {
    SPD_ID,
    { 1, SPD_SLOT, SPD_MIN * 256, SPD_MAX << 8, 0, 0, -1, -1, -1, 0,
      "Speed", STR_AMP, "SPD", digitables_spd_format, STR_EMPTY },
    LOOK_PTIM
};

/* ---- The TBL page (core-dn1's mod pages) --------------------------------
 * Index 27, after AMP's second page (9). core writes view and pos when it
 * builds AMP's view, so this is not const. */
struct page_desc {
    long idx, after;
    const char *short_title, *title;
    long ids[8];
    long kind;
    void *view;
    long pos;
};

struct page_desc digitables_tbl_page = {
    27, 9, "TBL", "Table",
    { TBL_ID, SPD_ID, 0, 0, 0, 0, 0, 0 }, 9,
    0, 0
};

extern int core_page_open(void *brain, void *event, long key, struct page_desc *page);
extern int core_page_shown(void *brain, struct page_desc *page);

/* ---- The voices ---------------------------------------------------------- */
enum { IDLE, PENDING, PLAYING };

struct voice {
    unsigned long base;         /* the pitch word the note started with */
    unsigned long last;         /* the timeline at the last block */
    unsigned long phase;        /* timeline units into the current step */
    unsigned short n;           /* the step it is on */
    signed char step;           /* the step last written, -1 none */
    signed char off;            /* the offset written with it */
    unsigned char state;
    unsigned char track;
    unsigned char tbl;          /* the table, 0-15 */
};

struct voice digitables_voices[VOICES];     /* global: the tests read it */

/* What each track's latest note plays, for the editor: table + 1 (0 none)
 * and step. */
volatile unsigned char digitables_play_tbl[SYNTHS], digitables_play_step[SYNTHS];

void digitables_voice_on(int voice, int track, void *event)
{
    struct voice *s;
    (void)event;
    if ((unsigned)voice >= VOICES)
        return;
    s = &digitables_voices[voice];
    s->base = PITCH[voice];
    s->last = TIMELINE;
    s->phase = 0;
    s->n = 0;
    s->step = -1;
    s->track = (unsigned char)track;
    s->state = (unsigned)track < SYNTHS ? PENDING : IDLE;
}

static int table_len(const struct table *t)
{
    return t->len < 1 || t->len > STEPS ? STEPS : t->len;
}

/* The step after step n: on up to the length, then back to the loop point
 * over and over, or the last step held when the loop is off. The length
 * and loop may change while a note plays (the editor). */
static int table_next(const struct table *t, int n)
{
    int len = table_len(t);
    if (n + 1 < len)
        return n + 1;
    if (t->loop >= len)
        return len - 1;
    return t->loop;
}

/* Timeline units a step for voice v, or 0 for a step every audio block.
 * Read every block: turning SPD while a note plays changes its speed. */
static unsigned long step_len(int v)
{
    int spd = VPARAM(v, SPD_SLOT) >> 8;
    if (spd > SPD_MAX)
        spd = SPD_MAX;
    if (spd >= 0)
        return TICK * (unsigned long)(spd + 1);
    if (spd <= SPD_MIN)
        return 0;
    return TICK / spd_div[-spd - 1];
}

void digitables_render_out(void)
{
    unsigned long now = TIMELINE;
    int v;
    for (v = 0; v < VOICES; v++) {
        struct voice *s = &digitables_voices[v];
        const struct table *t;
        unsigned long len;
        int step, note;
        if (s->state == IDLE)
            continue;
        if (s->state == PENDING) {
            /* The sound and the step's locks are loaded now. */
            int tbl = VPARAM(v, TBL_SLOT) >> 8;
            if (tbl <= 0 || tbl > TABLES) {
                s->state = IDLE;
                continue;
            }
            s->tbl = (unsigned char)(tbl - 1);
            s->state = PLAYING;
        }
        t = &digitables_bank[s->tbl];
        len = step_len(v);
        if (len == 0) {                 /* MAX: a step every block */
            s->phase = 0;
            if (s->step >= 0)           /* (after the first block's step 1) */
                s->n = (unsigned short)table_next(t, s->n);
        } else {
            s->phase += now - s->last;
            if (s->phase >= len) {
                s->phase -= len;
                if (s->phase >= len)    /* faster than the blocks: one a block */
                    s->phase %= len;
                s->n = (unsigned short)table_next(t, s->n);
            }
        }
        s->last = now;
        step = s->n < table_len(t) ? s->n : table_len(t) - 1;
        if (s->track < SYNTHS) {
            digitables_play_tbl[s->track] = (unsigned char)(s->tbl + 1);
            digitables_play_step[s->track] = (unsigned char)step;
        }
        /* A new step, or this step edited meanwhile: write the note. */
        if (step == s->step && t->step[step] == s->off)
            continue;
        s->step = (signed char)step;
        s->off = t->step[step];
        note = (int)(s->base >> 16) + s->off;
        if (note < 0)
            note = 0;
        else if (note > 127)
            note = 127;
        PITCH[v] = ((unsigned long)note << 16) | (s->base & 0xFFFF);
    }
}

/* ---- Keys: the TBL page and the editor ---------------------------------
 * Holding a track key on its own is core-dn1's (2.2): it asks ev_hold's
 * handlers first, and opens the Mod Menu when none takes it. digitables lists
 * TABLES there (the TBL page), and takes the hold when the TBL page is on
 * screen (the editor opens) or the editor is open (it closes). Key events
 * (core's ev_key): +12 the key, +16 its flags: bit 0 down, 0x2 FUNC held,
 * 0x8 held (repeats). */
#define KEY_FUNC  1
#define KEY_PLAY  11
#define KEY_STOP  12
#define KEY_YES   13
#define KEY_NO    14
#define KEY_UP    15
#define KEY_DOWN  16
#define KEY_LEFT  17
#define KEY_RIGHT 18
#define KEY_TRIG  20            /* TRIG SYN1 SYN2 FLTR AMP LFO: 20-25 */
#define KEY_AMP   24
#define KEY_LFO   25
#define KEY_STEP1 26            /* trig keys 26-41 */
#define KEY_T1    42
#define EV_KEY(ev)   (*(unsigned long *)((char *)(ev) + 12))
#define EV_FLAGS(ev) (*(unsigned long *)((char *)(ev) + 16))
#define EV_ENC(ev)   (*(unsigned long *)((char *)(ev) + 12))
#define EV_DELTA(ev) (*(long *)((char *)(ev) + 16))

static long enc_acc[8];                 /* counts not yet a step, per knob */
#define COUNTS    4                     /* encoder counts a detent */

/* The editor: open, the table (0-15), the step (0-15). */
unsigned char digitables_ed_open, digitables_ed_tbl, digitables_ed_step;

static void clamp_step(struct table *t, int i, int v)
{
    if (v > NOTE_MAX)
        v = NOTE_MAX;
    else if (v < -NOTE_MAX)
        v = -NOTE_MAX;
    t->step[i] = (signed char)v;
}

static void editor_open(int track)
{
    int tbl = SOUND_SLOT(track, TBL_SLOT) >> 8;
    if (tbl >= 1 && tbl <= TABLES)
        digitables_ed_tbl = (unsigned char)(tbl - 1);
    digitables_ed_open = 1;
}

/* A key in the editor: -> 1 to take it. */
static int editor_key(unsigned long key, unsigned long flags)
{
    struct table *t = &digitables_bank[digitables_ed_tbl];
    int press = flags & 1;                      /* held keys repeat (0x9) */
    int fine = (flags & 2) ? 12 : 1;            /* FUNC: octaves */
    if (key == KEY_PLAY || key == KEY_STOP || key == KEY_FUNC
        || (key >= KEY_T1 && key <= KEY_T1 + 3))
        return 0;                               /* transport, FUNC, tracks */
    if (key >= KEY_TRIG && key <= KEY_LFO) {    /* a page key: leave */
        if (press)
            digitables_ed_open = 0;
        return 0;
    }
    if (!press)
        return 1;
    if (key == KEY_NO) {
        digitables_ed_open = 0;
    } else if (key >= KEY_STEP1 && key < KEY_STEP1 + STEPS) {
        digitables_ed_step = (unsigned char)(key - KEY_STEP1);
    } else if (key == KEY_LEFT) {
        if (digitables_ed_step > 0)
            digitables_ed_step--;
    } else if (key == KEY_RIGHT) {
        if (digitables_ed_step < STEPS - 1)
            digitables_ed_step++;
    } else if (key == KEY_UP) {
        clamp_step(t, digitables_ed_step, t->step[digitables_ed_step] + fine);
    } else if (key == KEY_DOWN) {
        clamp_step(t, digitables_ed_step, t->step[digitables_ed_step] - fine);
    }
    return 1;
}

int digitables_key(void *brain, void *ev)
{
    (void)brain;
    return digitables_ed_open ? editor_key(EV_KEY(ev), EV_FLAGS(ev)) : 0;
}

/* core-dn1's ev_hold: a track key held on its own. -> 1 to take it. */
int digitables_hold(void *brain, void *ev, int track)
{
    (void)ev;
    if (digitables_ed_open) {
        digitables_ed_open = 0;
        return 1;
    }
    if (core_page_shown(brain, &digitables_tbl_page)) {
        editor_open(track);
        return 1;
    }
    return 0;
}

/* The Mod Menu's TABLES (core_menu): the TBL page. */
void digitables_menu_open(void *brain, void *ev, int track)
{
    (void)track;
    digitables_ed_open = 0;
    core_page_open(brain, ev, KEY_AMP, &digitables_tbl_page);
}

const struct {
    const char *name;
    void (*open)(void *brain, void *ev, int track);
} digitables_menu = { "TABLES", digitables_menu_open };

/* Knobs (core's ev_enc): +12 the encoder, 1-8 for A-H; +16 the turn. In the
 * editor: A the step's offset, B the step, C the length, D the loop point
 * (OFF, 1..length), H the table. */
int digitables_enc(void *brain, void *ev)
{
    unsigned long e = EV_ENC(ev);
    long d;
    struct table *t;
    (void)brain;
    if (!digitables_ed_open || e < 1 || e > 8)
        return 0;
    /* The event carries encoder counts, COUNTS a detent: a step each. */
    enc_acc[e - 1] += EV_DELTA(ev);
    d = enc_acc[e - 1] / COUNTS;
    enc_acc[e - 1] -= d * COUNTS;
    if (d == 0)
        return 1;
    t = &digitables_bank[digitables_ed_tbl];
    if (e == 1) {
        clamp_step(t, digitables_ed_step, t->step[digitables_ed_step] + d);
    } else if (e == 2) {
        long s = digitables_ed_step + d;
        digitables_ed_step = (unsigned char)(s < 0 ? 0 : s >= STEPS ? STEPS - 1 : s);
    } else if (e == 3) {
        long n = t->len + d;
        t->len = (unsigned char)(n < 1 ? 1 : n > STEPS ? STEPS : n);
        if (t->loop != LOOP_OFF && t->loop >= t->len)
            t->loop = (unsigned char)(t->len - 1);
    } else if (e == 4) {
        long l = (t->loop == LOOP_OFF ? -1 : t->loop) + d;
        if (l < -1)
            l = -1;
        else if (l > t->len - 1)
            l = t->len - 1;
        t->loop = l < 0 ? LOOP_OFF : (unsigned char)l;
    } else if (e == 8) {
        long n = digitables_ed_tbl + d;
        digitables_ed_tbl = (unsigned char)(n < 0 ? 0 : n >= TABLES ? TABLES - 1 : n);
    }
    return 1;
}

/* ---- Drawing the editor (core's ev_tick, ev_draw) ----------------------
 * Stock drawing routines (OS 1.43, digihealth's dn143.inc); the bitmap's
 * y = 0 is the bottom row, the screen 128 x 64. */
typedef void (*rect_fn)(void *bmp, int x0, int y0, int x1, int y1, int colour);
typedef void (*text_fn)(void *bmp, const void *font, int x, int y, int maxlen, const char *fmt, ...);
#define FILLRECT  ((rect_fn)0x400dd292)
#define FRAMERECT ((rect_fn)0x400dd076)
#define TEXTF     ((text_fn)0x400dde68)
#define FONT5     ((const void *)0x402315c8)

#define ZERO_Y    31            /* the graph's zero line */
#define BAR_H     20            /* a pixel a semitone, up to this; past it a notch */

void digitables_tick(void *ctrl)
{
    if (!bank_ready) {                  /* no project load reached us: */
        bank_defaults();                /* the first boot's install */
        bank_ready = 1;
    }
    if (digitables_ed_open)
        *((unsigned char *)ctrl + 0x20) = 1;    /* redraw: the playhead moves */
}

void digitables_draw(void *bmp, void *ctrl)
{
    const struct table *t = &digitables_bank[digitables_ed_tbl];
    int i, play = -1, v;
    (void)ctrl;
    if (!digitables_ed_open)
        return;
    for (i = 0; i < SYNTHS; i++)
        if (digitables_play_tbl[i] == digitables_ed_tbl + 1)
            play = digitables_play_step[i];
    FILLRECT(bmp, 0, 0, 127, 63, 0);
    TEXTF(bmp, FONT5, 1, 57, -1, "TABLE %02d", digitables_ed_tbl + 1);
    if (t->loop == LOOP_OFF)
        TEXTF(bmp, FONT5, 64, 57, -1, "LEN %02d LOOP OFF", t->len);
    else
        TEXTF(bmp, FONT5, 64, 57, -1, "LEN %02d LOOP %02d", t->len, t->loop + 1);
    FILLRECT(bmp, 0, 54, 127, 54, 1);
    for (i = 0; i < STEPS; i++) {
        int x0 = 8 * i, h = t->step[i], over = h > BAR_H || h < -BAR_H;
        if (h > BAR_H)
            h = BAR_H;
        else if (h < -BAR_H)
            h = -BAR_H;
        if (i < t->len) {
            if (h > 0)
                FILLRECT(bmp, x0 + 2, ZERO_Y, x0 + 5, ZERO_Y + h, 1);
            else if (h < 0)
                FILLRECT(bmp, x0 + 2, ZERO_Y + h, x0 + 5, ZERO_Y, 1);
            else
                FILLRECT(bmp, x0 + 2, ZERO_Y, x0 + 5, ZERO_Y, 1);
            if (over)                                   /* past the scale */
                FILLRECT(bmp, x0 + 3, ZERO_Y + h - (h > 0 ? 1 : -1),
                         x0 + 4, ZERO_Y + h - (h > 0 ? 1 : -1), 0);
        } else {
            FILLRECT(bmp, x0 + 3, ZERO_Y, x0 + 4, ZERO_Y, 1);
        }
        if (t->loop != LOOP_OFF && i == t->loop)
            FILLRECT(bmp, x0, 49, x0 + 1, 52, 1);       /* the loop point */
        if (i == play)
            FILLRECT(bmp, x0 + 2, 8, x0 + 5, 9, 1);     /* the playhead */
        if (i == digitables_ed_step)
            FRAMERECT(bmp, x0, 7, x0 + 7, 53, 1);
    }
    v = t->step[digitables_ed_step];
    TEXTF(bmp, FONT5, 1, 1, -1, "STEP %02d  NOTE %c%02d", digitables_ed_step + 1,
          v < 0 ? '-' : '+', v < 0 ? -v : v);
}
