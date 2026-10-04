# digitables

M8-style pitch tables for the Digitone (mk1): a table is up to 16 steps of
semitone offsets with a length and a loop point, and the project holds 16 of
them. A sound with a table on plays every note through it: each note
restarts the table and the sequencer's clock steps it, so one trig plays an
arpeggio, a trill, a pitch drop or a riff, from a slow walk to a buzz at
1500 steps a second. A step can also ADD a note instead of moving the pitch,
so one trig can strum a chord.

It is an [elekloader](https://github.com/irpina/elekloader) mod for Digitone
OS 1.43. elekloader builds a custom OS file on your own machine, from your
stock OS file and the mods you pick; nothing from Elektron is distributed.

**Status.** The TBL page, the editor, playback, the fast speeds (1.1) and
the ADD steps (1.3, with core-dn1 2.2's Mod Menu) have run on a Digitone.
The Digitone Keys runs the same OS file but has not been tried.

## What it does

### The TBL page

Hold a track key (T1-T4) on its own for about half a second: the **Mod
Menu** opens (it is core-dn1's, and lists every mod that put an entry
there). Pick **TABLES** with YES (or trig key 1). The TBL page is also AMP's
third page: press AMP until it shows.

| knob | |
|---|---|
| **A: TBL** | the sound's table: OFF, or 1-16 |
| **B: SPD** | the table's speed: ticks of the 24 PPQN clock a step, 1-24 (6 is a 16th, 24 a quarter note), and faster to the left of 1: a step every 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24 or 1/32 of a tick, and **MAX**, a step every audio block (1500 a second, at any tempo), as fast as the Digitone can change a pitch. At 120 BPM a tick is 1/48 s, so 1 is 48 steps a second and 1/16 is 768. |

Both are sound parameters: saved with the kit, shown and edited like the
stock ones, and p-lockable per trig (hold a trig in grid recording and turn
the knob), so one trig can play another table, or none, or at another speed.
Turning SPD while a note plays changes its speed from the next step.

### The table editor

On the TBL page, hold a track key: the editor fills the screen with the
table, its 16 steps as bars around a zero line.

| control | |
|---|---|
| **trig keys 1-16**, LEFT/RIGHT | pick a step |
| **A**, UP/DOWN | the step's offset, -48 to +48 semitones (FUNC+UP/DOWN: an octave) |
| **B** | the step |
| **C** | the table's length, 1-16 |
| **D** | the loop point: OFF (the last step holds), or 1 up to the length |
| **H** | which table, 1-16 |
| **YES** | the step adds a note (ADD), or not; an ADD step is drawn hollow |
| **NO**, or hold a track key | back to the TBL page |
| a page key (TRIG ... LFO) | leave to that page |

PLAY, STOP, FUNC and the track keys still work while it is open. The table
opens on the one the track's sound plays; a marker under a step follows a
note playing that table. Edits sound at once, even on a note already
playing.

### How a table plays

A note starts at step 1, its own pitch plus step 1's offset, and moves on
one step at the SPD speed: 1, 2, ... up to the length, then from the loop
point to the length over and over, or the last step held when the loop is
OFF. The clock runs at the tempo whether the sequencer plays or not, so a
note played live from the trig keys (KEYBOARD) steps the same way. Table 1
starts as an arpeggio (0, +12, +7, +3); the others start empty (all 0,
length 16).

An **ADD** step works differently: when the table reaches it, the note
keeps its pitch and a new note starts at the note's pitch plus the step's
offset. The new note is a copy of the trig's note, with its sound, p-locks,
velocity and length, and it is released together with the note that added
it (at the end of the trig's length, or when you let go of the key). So 0,
+4 ADD, +7 ADD plays the note, then adds its third, then its fifth: a
strummed chord from one trig, low to high. Start high and add downward
(+12, +7 ADD, +4 ADD, 0 ADD) to strum down. SPD sets
the gap between the notes: 1 is a tick (21 ms at 120 BPM), 1/2 half that.
A plain step after ADD steps moves the first note on as usual; the added
notes keep theirs. Step 1 always sets the note's own pitch; marked ADD, it
adds a note only when a loop comes back to it.

A quick one to try: a C64 chord. In table 2, steps 0, +4, +7, length 3, loop
1; TBL 2 and SPD 1 on a bright sound with a short release, and hold a long
note. Minor: 0, +3, +7. At 1/16 to MAX small offsets (0, +1, or 0, +12)
turn into vibrato and FM-like buzz.

A strum: in table 3, steps 0, +4, +7, +12, with ADD (YES) on steps 2-4,
length 4, loop OFF; TBL 3, SPD 1 or 1/2, on a plucky sound. Every note (a
trig, or a key in KEYBOARD) becomes a major chord strummed upward. 0, +3, +7
is minor; on a trig that already holds a chord, every note of the chord
strums its own, so keep an eye on the voices (below).

### Saving

The tables are part of the project: SAVE PROJECT and SAVE PROJECT AS save
them, LOAD PROJECT loads them, and a new project starts with the defaults.
They live in 480 bytes of the project's file that the Digitone OS leaves
alone, so a project saved with digitables loads on the stock OS (which
ignores them) and keeps them through a save there.

## Install

You need:
- **elekloader**:
  - **Windows:** download `elekloader-<version>-windows.zip` from
    [elekloader's releases](https://github.com/irpina/elekloader/releases/latest),
    unzip it and run `elekloader.exe`.
  - **Other systems:** run elekloader from source with Python 3.9 or newer
    (see [its README](https://github.com/irpina/elekloader#install)).
- **This mod and its core:** `digitables-1.3.elemod` and
  `core-dn1-2.2.elemod`, from
  [this repository's releases](https://github.com/irpina/digitables/releases/latest).
  digitables needs core-dn1 2.2 (its parameter slots, mod pages, project
  data and Mod Menu), which elekloader's releases do not have yet: the
  apps bundle core-dn1 2.0a. Install the 2.2 core from here until they do.
- **The stock OS file:** `Digitone_and_Digitone_Keys_OS1.43.syx`, from
  [Elektron's Digitone downloads](https://www.elektron.se/support-downloads/digitone).
  The mod is for OS 1.43 only; elekloader recognises the file by its hash.

Then build your OS in elekloader's window:

1. **Change stock firmware...** (top right): choose
   `Digitone_and_Digitone_Keys_OS1.43.syx`.
2. **+ Install from file...**: choose `core-dn1-2.2.elemod`, then
   `digitables-1.3.elemod`.
3. **Tick digitables.** Make sure the core ticked with it is **core 2.2**
   (untick core 2.0a if it is ticked). The check below the list should say
   "No conflicts ... Ready to build". It links with
   [digihealth](https://github.com/irpina/digihealth) 1.1 too (checked by
   elekloader's lint; not yet run together).
4. **OS version shown**: the 4 characters the unit will show, for example
   `TB12`.
5. **BUILD FIRMWARE**, and save the `.syx`. elekloader verifies it before
   writing it.

Flash it with Elektron Transfer, as for any OS update: connect the unit
over USB, select it in Transfer and **Connect**, drag the `.syx` onto
**Drop files here**, and press **YES** on the unit. Don't turn it off until
the upgrade is done.

Or on the command line (elekloader from source):

```bash
python -m elekloader.patch --stock Digitone_and_Digitone_Keys_OS1.43.syx \
    --mod core-dn1-2.2.elemod --mod digitables-1.3.elemod \
    --out Digitone_OS1.43-digitables.syx --version TB12
```

**Recovery:** elekloader never changes the bootloader, so the stock OS
file always restores the unit. Hold **FUNC** while powering on for the
startup menu, and press **TRIG 4** for OS UPGRADE. Then send the stock
`.syx` with Transfer.

## Build it from source

With elekloader's SDK and the m68k cross compiler (elekloader's README
says how to get them):

```bash
python -m elekloader.sdk.build path/to/digitables --stock Digitone_and_Digitone_Keys_OS1.43.syx
python -m elekloader.lint path/to/digitables/out/digitables-1.3.elemod \
    --stock Digitone_and_Digitone_Keys_OS1.43.syx --with core-dn1-2.2.elemod
```

core-dn1 2.2 is elekloader's `mods/core-dn1` (pull requests
[#34](https://github.com/irpina/elekloader/pull/34) and
[#35](https://github.com/irpina/elekloader/pull/35)).

## What it uses

- core-dn1's events: `ev_voice_on` (a note starts: the table restarts, or
  an added note is recognised),
  `ev_render_out` (every audio block: the steps, written to the voices'
  pitch words), `ev_key`, `ev_enc`, `ev_tick` and `ev_draw` (the editor),
  and `ev_hold` (a hold on the TBL page opens the editor);
- two parameter slots, ids 182 and 183, on sound slots 26 and 65, which no
  stock parameter uses;
- mod page 27, after AMP's second page;
- project data `TBL1` (320 bytes);
- a Mod Menu entry, TABLES;
- for ADD steps, the firmware's note queue: an added note is queued as a
  copy of the trig's note event, the way the arpeggiator queues its notes,
  and released through the voice's length counter. digitables keeps its
  own copy of the event and its p-locks (never a pointer into the
  firmware's pools), and skips the note when a pool is short.

## Known limits

- Power-up: the Digitone keeps a working copy of the project for power-up,
  which the tables go into like the rest; the emulator cannot show that
  step, so it is unchecked until a unit runs it.
- The arpeggiator and portamento have not been tried with tables. On a
  note the arpeggiator plays, ADD steps add nothing (the note keeps its
  pitch).
- ADD steps use voices: the Digitone has 8, shared by the four tracks, and
  an added note takes one like any note (the oldest is stolen when none is
  free). A loop over ADD steps adds notes over and over, stealing voices
  as it goes.
- OS 1.43 only (core-dn1 2.2 has OS 1.44 too; digitables does not yet).

## Licence

GPL-2.0-or-later, as elekloader. See [LICENSE](LICENSE).
