---
name: Smart Bus · Bàn trực an toàn
description: A depot safety desk painted like the body of the bus it watches; one glance at the reflective strip tells whether anyone is still on board.
colors:
  livery: "#f2b01e"
  livery-edge: "#c48d0c"
  ink: "#16181b"
  ink-2: "#3b4148"
  muted: "#545b63"
  steel: "#e4e8ec"
  steel-2: "#d5dbe1"
  panel: "#ffffff"
  line: "#c8cfd6"
  line-soft: "#e3e7eb"
  glass: "#121417"
  glass-ink: "#e4e8ec"
  glass-muted: "#a3abb4"
  reflect: "#c8201a"
  reflect-ink: "#a81a15"
  amber: "#f4a51c"
  amber-ink: "#8a5300"
  ok: "#1b7f47"
  ok-edge: "#0e4f2b"
  ok-soft: "#e3f2ea"
  warn-soft: "#fff1d6"
  alert-soft: "#fbe5e3"
typography:
  display:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "26px"
    fontWeight: 700
    lineHeight: 1.2
  headline:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "22px"
    fontWeight: 700
    lineHeight: 1.2
  readout:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "20px"
    fontWeight: 600
    lineHeight: 1.3
    fontFeature: "\"tnum\" 1"
  title:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "15px"
    fontWeight: 600
    lineHeight: 1.5
  body:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "15px"
    fontWeight: 400
    lineHeight: 1.5
  body-dense:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "14px"
    fontWeight: 500
    lineHeight: 1.5
  label:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "13px"
    fontWeight: 400
    lineHeight: 1.5
  axis:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "11px"
    fontWeight: 400
    fontFeature: "\"tnum\" 1"
  plate:
    fontFamily: "Be Vietnam Pro, system-ui, sans-serif"
    fontSize: "14px"
    fontWeight: 700
    letterSpacing: "0.06em"
    fontFeature: "\"tnum\" 1"
rounded:
  plate: "3px"
  control: "4px"
  panel: "6px"
  glass: "10px"
spacing:
  xs: "6px"
  sm: "8px"
  md: "12px"
  lg: "16px"
  xl: "20px"
  edge: "24px"
components:
  button:
    backgroundColor: "{colors.panel}"
    textColor: "{colors.ink}"
    typography: "{typography.body-dense}"
    rounded: "{rounded.control}"
    padding: "7px 14px"
    height: "38px"
  button-hover:
    backgroundColor: "{colors.steel}"
  button-active:
    backgroundColor: "{colors.steel-2}"
  button-primary:
    backgroundColor: "{colors.ink}"
    textColor: "{colors.panel}"
    typography: "{typography.body-dense}"
    rounded: "{rounded.control}"
    padding: "7px 14px"
    height: "38px"
  button-ghost:
    backgroundColor: "transparent"
    textColor: "{colors.ink}"
    rounded: "{rounded.control}"
    padding: "4px 10px"
    height: "32px"
  button-on-strip:
    backgroundColor: "{colors.ink}"
    textColor: "{colors.panel}"
    rounded: "{rounded.control}"
    padding: "7px 14px"
    height: "38px"
  input:
    backgroundColor: "{colors.panel}"
    textColor: "{colors.ink}"
    rounded: "{rounded.control}"
    padding: "8px 12px"
    height: "40px"
  livery-bar:
    backgroundColor: "{colors.livery}"
    textColor: "{colors.ink}"
    padding: "8px 24px"
    height: "56px"
  plate:
    backgroundColor: "{colors.panel}"
    textColor: "{colors.ink}"
    typography: "{typography.plate}"
    rounded: "{rounded.plate}"
    padding: "3px 10px"
  strip-normal:
    backgroundColor: "{colors.panel}"
    textColor: "{colors.ok}"
    typography: "{typography.headline}"
    height: "60px"
  strip-check:
    backgroundColor: "{colors.warn-soft}"
    textColor: "{colors.ink}"
    typography: "{typography.headline}"
    height: "60px"
  strip-check-label:
    backgroundColor: "{colors.amber}"
    textColor: "{colors.ink}"
  strip-warning:
    backgroundColor: "{colors.reflect}"
    textColor: "{colors.panel}"
    typography: "{typography.display}"
    height: "60px"
  strip-offline:
    backgroundColor: "{colors.steel-2}"
    textColor: "{colors.ink-2}"
    typography: "{typography.headline}"
    height: "60px"
  panel:
    backgroundColor: "{colors.panel}"
    textColor: "{colors.ink}"
    rounded: "{rounded.panel}"
  glass:
    backgroundColor: "{colors.glass}"
    textColor: "{colors.glass-ink}"
    rounded: "{rounded.glass}"
    padding: "10px 12px 12px"
  verdict-ok:
    backgroundColor: "{colors.ok-soft}"
    textColor: "{colors.ok-edge}"
    rounded: "{rounded.control}"
    padding: "8px 12px"
  verdict-warn:
    backgroundColor: "{colors.alert-soft}"
    textColor: "{colors.reflect-ink}"
    rounded: "{rounded.control}"
    padding: "8px 12px"
  verdict-idle:
    backgroundColor: "{colors.steel}"
    textColor: "{colors.ink-2}"
    rounded: "{rounded.control}"
    padding: "8px 12px"
---

# Design System: Smart Bus · Bàn trực an toàn

## Overview

**Creative North Star: "The Painted Bus Body"**

The screen is painted like the school bus it is watching. A yellow livery band carries the plate and the connection lights, a full-width reflective strip below it carries the safety state, the camera sits in a dark rounded window like the bus's own glass, and everything else is light grey painted steel with flat white panels riveted on by 1px lines. The world is industrial and plain: one sans family, tabular numbers, small corners, no shadows. Colour is paint and signal, never decoration.

Density is that of an operations desk, not a marketing page: a working grid that fills a laptop or a wall screen, compact 13–15px text, and three or four facts per panel. Calm is the default. When the bus is fine the strip is white with a green label; when a passenger may be left on board, the strip turns reflector red edge to edge with white-and-red tape, and nothing else on the page competes with it.

The world explicitly rejects the generic monitoring dashboard: equal-weight metric cards, a dark side navigation and a blue accent. Hierarchy comes from the bus metaphor (livery, strip, window, cabin plan) and from state, not from card grids.

**Key Characteristics:**
- Yellow livery band on top, grey steel desk below, white panels with 1px rules.
- A full-width reflective strip whose single state colour owns the whole band.
- The camera framed in a dark "glass" window with a three-fact readout column.
- A top-down cabin plan drawn only in 90° and 45° lines, sensors lit by real state.
- One family (Be Vietnam Pro), tabular figures for every number.
- Flat surfaces; depth only from the dark glass against light steel.

## Colors

A painted-metal palette: school-bus yellow and cool grey steel, with reflector red, signal amber and signal green reserved for live safety state.

### Primary
- **Livery Yellow** (#f2b01e): the bus body colour. Fills the top livery bar and the browser theme colour; in small doses it also marks "a person is seen" (the active cabin-plan marker, the current-value dot on the count chart) and is the focus ring on dark glass and on inputs.
- **Livery Edge** (#c48d0c): the 1px seam under the livery bar.

### Secondary
- **Reflector Red** (#c8201a): the WARNING state. Fills the whole reflective strip and its tape, the alarm hull stroke, the alert marker, the ignition-off line on the chart and the critical log dot.
- **Reflector Ink** (#a81a15): red as text and edges on light surfaces: the warning strip message, PIR "Có chuyển động", failed commands, error hints, the warn verdict.

### Tertiary
- **Signal Amber** (#f4a51c): the CHECK countdown. The strip label plate and the receding amber-and-white tape; warning-severity log dots.
- **Amber Ink** (#8a5300): amber as text and heavy outline: the "checking" marker outline, sent and expired command states, the checking hull stroke.
- **Signal Green** (#1b7f47): NORMAL and "on": the normal strip label, connected link lights, OK markers, applied commands, fan running.
- **Green Edge** (#0e4f2b): the outline of every green fill and the text of the OK verdict.

### Neutral
- **Ink** (#16181b): text, primary and on-strip buttons, the chart line, the focus ring on light surfaces.
- **Ink Two** (#3b4148): secondary text, default button borders, plan hull and leader lines.
- **Muted Grey** (#545b63): metadata, freshness stamps, table heads, axis labels.
- **Painted Steel** (#e4e8ec): the page ground; hover fill for default buttons; idle verdict.
- **Steel Two** (#d5dbe1): pressed buttons, offline and loading strip, driver seat.
- **Panel White** (#ffffff): panels, the normal strip, inputs.
- **Rule** (#c8cfd6) and **Soft Rule** (#e3e7eb): panel borders, and the row dividers inside panels.
- **Glass** (#121417), **Glass Ink** (#e4e8ec), **Glass Muted** (#a3abb4): the camera window, its text and its labels.
- **Soft fills** — **Check Wash** (#fff1d6), **Alert Wash** (#fbe5e3), **OK Wash** (#e3f2ea): the CHECK strip ground and the warn and OK verdict grounds.

### Named Rules
**The Livery Bar Rule.** Livery yellow fills the top bar and nothing larger than a marker. It never fills a panel, a card, a button or the strip.

**The One State Owns the Strip Rule.** The reflective strip shows exactly one state colour at a time, across the full width. Red is WARNING, amber is CHECK, green text on white is NORMAL, steel grey is OFFLINE or loading.

**The Signal Is Bound to State Rule.** Red, amber and green only appear when a real reading puts them there. Each signal colour has a fill and a darker ink/edge partner; use the ink partner for text on light grounds.

## Typography

**Display Font:** Be Vietnam Pro (with system-ui, sans-serif)
**Body Font:** Be Vietnam Pro (with system-ui, sans-serif)

**Character:** A single plain grotesque with full Vietnamese diacritics, self-hosted at 400/500/600/700. It reads like the lettering on a bus fleet, legible under fluorescent light; hierarchy comes from size and weight, never from a second face.

### Hierarchy
- **Display** (700, 26px, 1.2): the reflective strip label in WARNING ("Còn người trên xe"); 21px at ≤720px.
- **Headline** (700, 22px, 1.2): the strip label in other states and the login title ("Đăng nhập ca trực"); 19px at ≤720px.
- **Readout** (600, 20px, 1.3, tabular): the three facts in the camera glass. Absent values drop to 500, 16px in Glass Muted.
- **Title** (600, 15px): panel and glass headings ("Ai đang trên xe", "Camera khoang xe").
- **Body** (400, 15px, 1.5): base text and the strip message; camera empty-state copy capped at 52ch.
- **Body Dense** (500, 14px): buttons, tables, control names, the verdict (600). Source values in the on-board table are 700, 16px.
- **Label** (400–600, 13px): panel meta, readout terms, legend, links, command rows, hints; table heads are 600, 12px.
- **Axis** (11px, tabular): chart axes and the cabin plan's zone label; ignition-off labels are 600 in Reflector Ink.
- **Plate** (700, 14px, 0.06em tracking, tabular): the BUS-01 licence plate only.

### Named Rules
**The One Family Rule.** Be Vietnam Pro only, weights 400–700. No display face, no monospace.

**The Tabular Number Rule.** Every count, clock time, percentage and timestamp uses tabular figures so values never jitter as they update each second.

**The Strip Speaks Loudest Rule.** The strip label is the largest text on the desk. Nothing else may be set larger than 22px.

## Layout

The page is a vertical stack: livery bar (min 56px, 8px 24px padding), reflective strip (min 60px), then the desk. The desk is a two-column grid, a fluid main column beside a fixed 400px side column, 20px gaps, 20px 24px 32px padding, capped at 1560px and centred. The main column holds the camera glass and the cabin plan (the plan panel stretches to fill the column height, with its legend pinned to the bottom). The side column holds, in order, "Ai đang trên xe", "Điều khiển từ xa" and the "Sĩ số theo thời gian" chart. The event log spans both columns below.

Inside the glass, the 16:9 camera frame and a readout column of three equal-weight facts sit side by side. The frame height is container-driven (the lesser of 38vh and the width left after the readout column, minimum 200px). Panels use a 16px inner inset; headings sit at 14px 16px 10px; rows are separated by Soft Rule lines rather than gaps.

Spacing runs on a small rhythm of 6, 8, 12, 16, 20 and 24px: 6–8px inside controls and legends, 12–16px inside panels, 20px between panels, 24px at the page edge.

Responsive behaviour: at ≤1100px the desk becomes one column. At ≤720px the page edge drops to 14px; the brand subtitle and link state words hide; the strip wraps into rows (label, message, the acknowledge button, then a 12px tape); the glass stacks the frame above a two-column readout whose third fact spans the full row; the cabin plan switches from its horizontal drawing to a vertical one (max 420px wide); and the event log turns each row into a two-line grid.

## Elevation & Depth

The system is flat. Panels sit on steel with a 1px Rule border and no shadow. The only depth cue is material: the dark Glass window set into light steel, like a bus window in a painted body. The one shadow in the build is a 1px hairline (`0 1px 0` Soft Rule) under the sticky event-log header, acting as a rule rather than elevation.

### Named Rules
**The Flat Steel Rule.** No drop shadows, glows or blurs. Separation comes from 1px rules, steel-versus-white grounds and the dark glass.

## Shapes

Small, utilitarian corners: the licence plate at 3px, buttons, inputs, verdicts and log chips at 4px or less, panels at 6px, and only the camera glass at 10px so it reads as a window. Cabin-plan seats and markers are 4px-rounded squares; status indicators in the livery bar are 9px rings, filled green when connected. Borders are 1px, with 1.5px for the plate, link rings and legend swatches. The cabin plan hull is the one drawn silhouette: a rounded-nose bus outline, with every inner line at 90° or 45°.

### Named Rules
**The Only the Window Is Round Rule.** Nothing but the camera glass goes above 6px radius.

## Components

### Buttons
Plain, firm and paired. Controls sit in two equal columns per device.
- **Shape:** gently squared (4px), minimum 38px tall.
- **Default:** Panel White with an Ink Two 1px border, Body Dense text, 7px 14px padding.
- **Hover / Active / Focus:** Painted Steel on hover, Steel Two when pressed, 150ms background and border transition (removed under reduced motion). Focus is a 2px Ink outline offset 2px (Livery Yellow inside the glass).
- **Disabled:** a light grey ground (#f3f5f7) with Rule border and grey text, `not-allowed` cursor; remote controls disable while the board is offline, with a hint explaining why.
- **Primary:** Ink fill, white text (login "Vào ca trực", 42px tall in the form).
- **Ghost:** transparent, 32px tall, a 55% Ink border, used on the livery bar for "Đăng xuất" with a 16px line icon.
- **On-strip:** the WARNING acknowledgement ("Đã nhận cảnh báo"), Ink fill, white 600 text, set at the right end of the red strip.

### Cards / Containers
- **Corner Style:** 6px.
- **Background:** Panel White on Painted Steel.
- **Shadow Strategy:** none (see Elevation & Depth).
- **Border:** 1px Rule; internal rows divided by Soft Rule.
- **Internal Padding:** 16px inset; heading row is Title text with Label meta right-aligned on the same baseline.

### Inputs / Fields
- **Style:** Panel White, 1px mid-grey stroke (#9aa3ad), 4px radius, 40px tall, 8px 12px padding; the label sits above in 500 14px.
- **Focus:** a 2px Livery Yellow outline flush to the field and the border shifts to Ink.
- **Error:** a Reflector Ink hint below the button, announced as an alert.

### Navigation
The livery bar is the only navigation: Livery Yellow, 56px, a Livery Edge seam. Left: the white BUS-01 plate, "Smart Bus" in 700 17px, and the role "Bàn trực an toàn" in a dark brown-on-yellow. Right: three connection lights (Máy chủ, Mạch ESP32, Camera), each a 9px ring that fills Signal Green when live, with a state word, followed by the ghost sign-out button. At ≤720px the role and state words hide.

### Reflective Strip (signature)
The full-width safety band under the livery. A bold state label on the left, a message, a tape that fills the remaining width, and the acknowledge button when WARNING.
- **NORMAL:** white band, Signal Green label "Bình thường", empty tape.
- **CHECK:** Check Wash band, the label set on an Amber plate, and amber-and-white tape (56px stripes) whose width shrinks linearly with the 15-second countdown.
- **WARNING:** the whole band Reflector Red; the label grows to Display size in white; the message sits on a white block in Reflector Ink 600; red-and-white tape wipes in from the left (420ms, `cubic-bezier(0.16, 1, 0.3, 1)`, removed under reduced motion). The strip carries `role="alert"`.
- **OFFLINE / loading:** Steel Two band, Ink Two label, no tape.

### Camera Glass (signature)
A Glass window, 10px radius, 10px 12px 12px padding, holding a Title header with freshness meta in Glass Muted, the 16:9 frame (6px radius, near-black letterbox) and a readout list of three facts ("Khung hình lúc", "Độ tin cậy YOLO", "Kết quả gửi xuống mạch") separated by 1px dark rules. A frame older than 5 seconds is greyscaled and dimmed with a centred "Hình cũ … giây" badge; with no frame the window shows a line icon and an explanation, never a placeholder image.

### Cabin Plan (signature)
A top-down SVG of the bus: rounded-nose hull, two rows of eight seats, a dashed aisle, the driver's seat, a door gap, and a dashed 45° camera sightline. Four 20px square markers (Camera, IR cửa, PIR, Nút cuối xe) with a bold label and a detail line:
- **OK:** Signal Green fill, Green Edge stroke.
- **Seen:** Livery Yellow fill, Ink stroke (a person is detected).
- **Checking:** white fill with a heavy (4px) Amber Ink outline.
- **Alert:** Reflector Red fill and edge; its detail text turns Reflector Ink 600.
- **No data:** white with an Ink Two outline.
The hull itself takes the state: Amber Ink stroke on a warm wash in CHECK, a 2.5px Reflector Red stroke on a pale red wash in WARNING. The plan is drawn horizontally on desktop and rotated to vertical at ≤720px, with a legend strip pinned under it.

### Count History Chart
A 160px-tall step line in 2px Ink inside the side column, only the 0 and max gridlines in Soft Rule, Axis labels, a Livery Yellow "now" dot with an Ink ring, and a dashed Reflector Red line labelled "Tắt khóa điện HH:MM" at each ignition-off.

### Verdict and Command States
The on-board panel ends with a 4px-radius verdict bar: OK Wash for "IR và camera khớp", Alert Wash with Reflector Ink for a mismatch, Painted Steel for "no fresh camera data". Recent commands never vanish: each row changes state word and colour (Amber Ink while sent, Signal Green when applied, Reflector Ink when failed, struck-through name with Amber Ink "Hết hạn" when expired). Event-log severity uses an 8px square dot before the type: green info, amber warning, red critical (critical text also turns Reflector Ink).

## Do's and Don'ts

### Do:
- **Do** let the reflective strip carry the safety state across the full width, one colour at a time, with the label at 22px (26px in WARNING).
- **Do** pair every signal colour with words: markers carry a label and detail line, the legend explains each fill, the strip says the state in text.
- **Do** show source and freshness next to every reading ("vừa xong", "12 giây trước"), and make stale data look stale: greyscale and dim the frame, drop readouts to Glass Muted, say "hết hiệu lực".
- **Do** set every number in tabular figures.
- **Do** draw the cabin plan in 90° and 45° lines only, and swap to the vertical drawing at ≤720px.
- **Do** keep remote-control buttons in equal pairs and disable them, with a reason, when the board is offline.

### Don't:
- **Don't** fill panels, buttons or the strip with Livery Yellow; it belongs to the top bar and to small "person seen" marks.
- **Don't** build equal-weight metric cards, a dark side navigation or a blue accent; this world rejects the generic dashboard.
- **Don't** add drop shadows or glows to panels; depth is the dark glass against light steel.
- **Don't** use red, amber or green as decoration; they appear only when a real reading puts them there.
- **Don't** introduce a second typeface or set anything larger than the strip label.
- **Don't** remove a sent command from the list; change its state instead.
- **Don't** round anything beyond 6px except the camera glass (10px).
