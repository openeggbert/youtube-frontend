# youtube-frontend — analysis, bugs and improvement plan

Date: 2026-07-11
Scope: the whole C++ generator (`src/`, `include/`) + the generated HTML/CSS output.

> **Status: Phases A–D implemented** (2026-07-11). All bugs described below
> are fixed, the redesign (shared `assets/style.css`, CSS grid card layout,
> reworked video page) is done and verified via a clean build plus a real
> run of the tool against test data (valid HTML, escaping works, the
> `videos-per-row` bug is fixed). The generated HTML output and this
> document have since been translated to English.
>
> **Phase E1 implemented** (2026-07-11) — items E1.1–E1.4 below are done:
> broken zero-args fallback replaced with `--help`/usage + unknown-flag
> validation, videos with no channel now get a real "Unknown channel" page
> and their video page instead of disappearing, and orphaned comment
> replies are now shown as top-level comments with a warning instead of
> silently dropped. Verified with a synthetic fixture covering all four
> cases.
>
> **Phase E2 implemented** (2026-07-11) — items E2.5–E2.7 below are done:
> large channels now paginate across `channels/<id>[-page<N>].html` via a
> new `--videos-per-page` option (video numbering stays global so
> individual video pages' Back/Next keep working); base64 thumbnail
> encoding is parallelized per page via a shared `ThreadPool` (now in
> `include/ThreadPool.h`, reused from `YoutubeVideo.cpp` too); and the
> encoded thumbnail is cached to a `<file>.b64` sidecar and reused across
> runs unless `--always-generate-metadata 1` forces a refresh. Verified
> with a forced small `--videos-per-page` and cache-hit/cache-bypass
> mtime checks. **Phases E3–E4 are still just a backlog**, not implemented.

What this is: a tool that generates static HTML pages (channel/video overview
+ a single-video page with comments) over an archive downloaded via
ArchiveBox/yt-dlp.

---

## 1. Bugs found (Phase A–D scope — all fixed)

### Critical

1. **Missing HTML escaping of YouTube-derived text.**
   `title`, `description`, `channel name`, comments (`author`, `text`) were
   inserted into HTML verbatim, without escaping `<`, `>`, `&`, `"`.
   - `src/YoutubeVideoHtml.cpp:104` (`<title>`), `:145` (video heading),
     `:200-206` (description), `:213`, `:226-228` (comments)
   - `src/Main.cpp:269` (channel `<h1>`), `:354` (card title)
   - Real-world impact: ordinary characters in video titles (`&`, `<`,
     quotes) broke the page or injected unexpected markup. Not just
     cosmetic — the single most common cause of the "broken-looking"
     generated pages.
   - **Status: fixed.** `Utils::escapeHtml()` added and applied everywhere
     user/YouTube-derived text is inserted.

2. **Invalid HTML in the video thumbnail table.**
   `src/Main.cpp:305-308, 373-376, 403-405` — `<tr>` was opened repeatedly
   but **never closed with `</tr>`**, plus a stray empty `<tr>` was appended
   at the end even with no `<td>`s. Browsers silently error-correct this,
   but it's fragile and made it impossible to cleanly style rows/grid via CSS.
   - **Status: fixed.** The table was replaced entirely by a CSS grid
     (`.grid` / `.card`), so there is no `<tr>` markup left at all.

3. **`--videos-per-row` with a value < 2 broke the layout.**
   `src/Args.cpp:64-70` — an invalid value was stored as the string `"0"`
   instead of "not set". `Main.cpp` then computed
   `max-width: (THUMBNAIL_WIDTH+20) * vpr` → **`max-width:0px`** on the
   wrapping `<div>`, and `.value_or(4)` never kicked in because `Args`
   returned a valid value (`"0"`), not `nullopt`. Result: the whole video
   grid could effectively collapse to zero width.
   - **Status: fixed.** An invalid value is now skipped (not stored), so
     the documented default (4) applies via `Args`' own default-value
     mechanism.

### Medium severity

4. **Silent error swallowing when resizing thumbnails.**
   `src/Main.cpp:334-339` (old) — `catch (...) {}` around
   `Utils::resizeImage`. On failure it continued with an empty `bytes`
   buffer, producing `data:image/jpg;base64,` (an empty/broken image) with
   no warning logged anywhere.
   - **Status: fixed.** The catch now logs a `[Warning]` with the actual
     exception message and falls back to a direct file link instead of
     emitting a broken image.

5. **Wrong/hardcoded MIME type for base64 thumbnails.**
   `src/Main.cpp:342` (old) — always `image/jpg` regardless of the actual
   format (could be `.webp`, `.png`...). Correct would be `image/jpeg` (not
   `jpg`) or a MIME type derived from `getThumbnailFormat()`.
   - **Status: fixed.** `mimeTypeForImageFormat()` now maps the real
     thumbnail extension to the correct MIME type.

6. **`escapeForShell` was incomplete and conflated with HTML escaping.**
   `src/YoutubeVideoHtml.cpp:34-72` (old) — didn't escape `$`, backticks,
   `<`, `>`, newlines. Worse, the result (containing sequences like `\"`)
   was inserted directly into the HTML attribute `value="..."`
   **without HTML-escaping** — a filename containing a quote would break
   the attribute and the rest of the page.
   - **Status: fixed.** `escapeForShell` now also escapes `$`, backticks
     and backslashes, and the resulting shell command string is passed
     through `Utils::escapeHtml()` before being placed in the HTML
     attribute.

7. **Unguarded parsing of the `metadata` cache file.**
   `src/YoutubeVideo.cpp:82-124` (CASE 1, reading from cache) — `std::stoll`
   / `std::stoi` on values from the `props` map with no try/catch. A
   missing or corrupt line in the file (e.g. an older cache format without
   `number=`) crashed the entire run mid-generation.
   - **Status: fixed.** Parsing is now wrapped in try/catch; on failure it
     logs a warning and falls through to regenerate the metadata from
     scratch (CASE 2) instead of crashing.

8. **Empty `miniThumbnail` → malformed filename.**
   `src/YoutubeVideo.cpp:150-162` (old) — if no candidate thumbnail met the
   width threshold, `miniThumbnail` stayed `""`, `getMiniThumbnailFormat()`
   returned `""`, and the resulting path was `mini-thumbnail.` (a trailing
   dot with no extension) → an image that could never be downloaded or
   displayed.
   - **Status: fixed.** Falls back to the main `thumbnail` field when no
     candidate meets the threshold.

### Minor / code cleanup

9. **Global static variables used as layout counters.**
   `src/Main.cpp:46-47` (old) (`iii`, `internalStaticVariableVideoNumberPerRow`)
   — worked, but were file-scope statics mutated inside a function; confusing
   and needlessly fragile; should have been local variables/parameters.
   - **Status: fixed.** Replaced with plain local variables and a
     `processedVideos` reference parameter; no file-scope statics remain
     for this purpose.

10. **Dead code in `Utils`.**
    `getCountOfSlashOccurrences`, `createDoubleDotSlash`, `listAllFilesInDir`,
    `copyFile` (`src/Utils.cpp`) — unused anywhere in the project (verified
    via grep).
    - **Status: fixed.** Removed.

---

## 2. Visual / CSS redesign of the generated output (implemented)

### What was wrong
- No shared CSS file — every page had its own `<style>` block with just two
  rules (`padding`, `font-family:Arial`); everything else was **inline style
  on every single element** → inconsistent, unmaintainable, no easy way to
  add dark mode or responsiveness.
- The video grid was a `<table>` with a fixed width driven by
  `videos-per-row` — didn't respond to window width, overflowed on
  mobile/narrow windows.
- No hover/focus states, no rounded corners, no shadows, no typographic
  hierarchy — plain Arial, black text on white.
- Back/Next buttons were raw `<button>`s with inline styles; the disabled
  state was handled inconsistently (`disabled` attribute vs.
  `visibility:hidden`).
- Comments were indented via `margin-left: dotCount()*50px` — deep threads
  ran off-screen since nothing constrained width or wrapped the layout.

### Implemented solution
1. **Extracted CSS into one shared file** (`assets/style.css`, embedded at
   compile time via `AssetsStyle.cpp` and written out once per run), linked
   via `<link rel="stylesheet">` from every generated page instead of
   duplicated `<style>` blocks and inline styles.
2. **Dark theme by default with a light fallback** via
   `prefers-color-scheme`, readable system font stack instead of Arial.
3. **CSS Grid instead of `<table>`** for the thumbnail grid:
   `grid-template-columns: repeat(auto-fill, minmax(220px, 1fr))` —
   responsive on its own; `videos-per-row` now only acts as a soft
   max-width cap on the grid rather than a hard column count.
4. **Cards for video thumbnails** — rounded corners, subtle shadow, hover
   lift effect, 2-line clamped titles with `…`.
5. **Reworked single-video page** — framed player, pill-shaped
   Back/Next buttons with arrows, a metadata "chip" row (size, duration,
   date, download, YouTube link), comments visually linked via indentation
   (`--depth` custom property) plus an initial-letter avatar per author.
6. **Consistent button states** via CSS classes (`.pill`, `.pill.disabled`)
   instead of a `disabled`/`visibility:hidden` mix.
7. As a side effect of this rebuild: introduced the `Utils::escapeHtml()`
   utility and used it everywhere text is inserted (fixes bug #1).

A visual mockup of the new design (channel grid + video page with comments)
was reviewed as an artifact before implementation; the real generated
output was verified afterwards the same way.

---

## 3. Work phases

| Phase | Content | Status |
|---|---|---|
| A | Critical bug fixes (#1 escaping, #2 `<tr>`, #3 `videos-per-row`) | ✅ done |
| B | CSS extraction + redesign (grid, dark theme, cards, buttons) | ✅ done |
| C | Remaining medium-severity bugs (#4–#8: MIME, error handling, parsing) | ✅ done |
| D | Code cleanup (#9 static variables, #10 dead code) | ✅ done |
| E1 | CLI/help + no-channel videos + orphaned comments (see below) | ✅ done |
| E2 | Pagination + parallel/cached thumbnail encoding (see below) | ✅ done |
| E3–E4 | Generated-site UX, code quality backlog (see below) | 🆕 not started |

---

## 4. Decisions already made (previously open questions)

- **Theme default**: light by default, dark via `prefers-color-scheme`
  (matches the reviewed mockup).
- **`videos-per-row`**: kept as a soft max-width cap on the responsive grid
  rather than a hard column count or being removed entirely.
- **HTML language**: generated output and this document are in English
  (`lang="en"`); UI strings were translated from the initial Czech draft.
- **`videos-per-page` default**: 60, chosen as a reasonable page length for
  the card grid; adjustable via `--videos-per-page`.

---

## 5. Phase E — backlog from post-implementation review

Found while reviewing the finished Phase A–D work. **E1 and E2 are
implemented**; E3–E4 are still just a backlog for discussion/prioritization
before any code changes.

### E1. Found while implementing (done)

1. **The zero-args fallback is broken and personally-scoped.**
   `src/Main.cpp` (the `if (args.size() < 1)` branch) — running the binary
   with no arguments used a hardcoded developer path
   (`/rv/big/foreign-blupi-videos-on-youtube`) and flags `--video_` /
   `--channel_`. The real flag names are `--video` / `--channel` (no
   trailing underscore) — `Args.cpp` requires an exact match, so these two
   never actually registered. Effect: running with no args silently processed
   the **entire archive** instead of the one video/channel it looked like
   it was meant to filter to.
   - **Status: fixed.** The hardcoded fallback is gone. Zero args now prints
     usage and exits with status 1; `-h`/`--help` prints the same usage and
     exits 0. Usage text is generated from `ArgType`'s name/default/description
     metadata (single source of truth, see `printUsage()` in `Main.cpp`).

2. **No CLI argument validation or `--help`.**
   An unknown or mistyped flag (`--videos-per-row5`,
   `--thumbnail-as-base65`...) was **silently ignored** — `Args.cpp` just
   failed to find a match and moved on.
   - **Status: fixed.** `Args.cpp` now throws `YoutubedlFrontendException`
     for any unrecognized `--`-prefixed token, and `main()` wraps its body
     in try/catch to print `Error: <message>` and exit 1 instead of an
     unhandled-exception crash.

3. **Videos with no `channelName` disappear entirely from the output.**
   `Main.cpp` built the channel list only from videos with a non-empty
   `channelName`. A video without a recognized channel never appeared in any
   `<a>` loop, so its `videos/<id>.html` page was **never generated at all**
   — not just missing from navigation.
   - **Status: fixed.** `YoutubeVideo::loadYoutubeVideos` now normalizes an
     empty `channelName` to the shared `YoutubeVideo::UNCATEGORIZED_CHANNEL_NAME`
     ("Unknown channel") before sorting, so these videos get a real
     `channels/uncategorized.html` page and their own video pages like any
     other channel. The fake "Channel on YouTube" external link is skipped
     for this synthetic bucket (empty `channelUrl`).

4. **Orphaned comment replies are silently dropped.**
   `YoutubeComment::getChildren` filtered strictly by `parentId` — if a
   reply's parent comment was missing from the data (deleted/edge case), the
   reply was never rendered, with no warning.
   - **Status: fixed.** `YoutubeComment::sort` now detects comments
     unreachable from `"root"`, logs a `[Warning] N comment(s) reference a
     missing parent...`, and appends them as top-level comments instead of
     dropping them.

Verified with an extended synthetic fixture: `--help`/`-h` output, zero-args
exit code, an unknown-flag error, a video with empty channel metadata (now
gets `channels/uncategorized.html` + its own video page, shows up in the
master list, no external channel link rendered), and a comment replying to
a non-existent parent (now rendered top-level with the warning logged). All
generated HTML still passes the well-formedness check.

### E2. Performance / scalability (done)

5. **One channel = one giant HTML page.** No pagination or limit — a
   channel with 500+ videos generates a single very large file. Combined
   with `--thumbnail-as-base64 1` this means hundreds of inline base64
   images on one page (megabytes of inline HTML).
   - **Status: fixed.** New `--videos-per-page` option (default 60) splits
     a channel's grid across `channels/<id>.html`, `channels/<id>-page2.html`,
     `channels/<id>-page3.html`, ... with a Previous/Next pager
     (`pagerHtml()` in `Main.cpp`). Video numbering (`#N` on the card, and
     the `#N / total` counter on each video's own page) is assigned once
     across the whole channel *before* splitting into pages, so it stays
     stable regardless of pagination and Back/Next between individual video
     pages is unaffected.
6. **Base64 thumbnail encoding is sequential**, outside the `ThreadPool`
   used for the initial metadata loading phase — could be parallelized for
   large archives.
   - **Status: fixed.** The `ThreadPool` class was extracted out of
     `YoutubeVideo.cpp` into `include/ThreadPool.h` so it can be shared.
     `generateChannelPages()` in `Main.cpp` creates one pool per channel
     (only when `--thumbnail-as-base64 1`) and encodes each page's batch of
     thumbnails concurrently instead of one at a time.
7. With `--always-generate-html-files 1`, **the resize + base64 encode is
   redone on every run** even when the source thumbnail hasn't changed —
   could be cached alongside the metadata file.
   - **Status: fixed.** `buildThumbnailSrc()` writes the encoded data URI to
     a `<mini-thumbnail file>.b64` sidecar next to the source thumbnail and
     reuses it on later runs. `--always-generate-metadata 1` (which already
     meant "regenerate cached derived data") now also bypasses this cache,
     so the two "regenerate everything" flags stay consistent.

Verified by forcing pagination with a small `--videos-per-page` value on a
test channel (page 1/2 files, correct pager links, stable global numbering,
both videos' own pages still generated), and by checking the `.b64` cache
file's mtime is unchanged across a repeat run (`--always-generate-metadata 0`)
but does change when forced (`--always-generate-metadata 1`). All generated
HTML still passes the well-formedness check.

### E3. Generated-site UX

8. **Stats and warnings only ever go to the console.** The
   duration/size-sorted lists and the "Snapshots without videos" warning
   are never written to the generated site, even though they'd make a
   useful `stats.html`/`warnings.html` page.
9. **No search/filter across videos.** The output is static with no JS at
   all. A small vanilla-JS title filter (progressive enhancement, no build
   step, works without JS too) would help larger archives.
10. **No manual dark/light toggle** — currently follows
    `prefers-color-scheme` only. Could be done in pure CSS (checkbox hack),
    no JS needed.
11. The README documents two ffmpeg conversion variants (full quality and
    a 480p-scaled version), but the generated video page only ever offers
    one (crf 18, no scaling). Could offer both command boxes.

### E4. Code quality / maintenance

12. **No automated tests** — neither unit nor integration. Good first
    candidates: `Utils::escapeHtml`, `Utils::formatDurationShort`,
    `Args` parsing edge cases, `YoutubeComment::sort` threading logic.
13. **No CI** (e.g. GitHub Actions) — the build is clean today; nothing
    currently guards against that regressing on future pushes/PRs.
14. **No compiler warning flags** in `CMakeLists.txt`
    (`-Wall -Wextra -Wpedantic`) — some of the bugs fixed in Phase A–D
    (unguarded `stoi`/`stoll`, etc.) are exactly the kind of thing a
    stricter warning level or static analysis would have caught earlier.
15. The build emits deprecation warnings for `SHA512_Init`/`Update`/`Final`
    (deprecated by OpenSSL 3.0 in favor of the EVP API) — not functionally
    broken today, but worth migrating before a future OpenSSL removes them.

No priority order has been assigned yet within Phase E — to be discussed
before picking what to implement next.
