/*
 * MIT License
 * Copyright (c) 2024-2025 Robert Vokac
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "AssetsStyle.h"

namespace Assets
{
const std::string STYLE_CSS = R"CSS(
:root {
  --bg: #F1EEE8;
  --surface: #FFFFFF;
  --surface-2: #F7F4EC;
  --border: #DEDACD;
  --border-strong: #C9C2AE;
  --text: #211C15;
  --text-dim: #766C5C;
  --accent: #B9761F;
  --accent-ink: #FFFFFF;
  --ok: #2E7D5B;
  --ok-bg: #E4F1EA;
  --radius-s: 5px;
  --radius-m: 9px;
  --mono: ui-monospace, "SF Mono", "Cascadia Mono", "Roboto Mono", Menlo, Consolas, monospace;
  --sans: -apple-system, BlinkMacSystemFont, "Segoe UI", ui-sans-serif, "Helvetica Neue", Arial, sans-serif;
}

@media (prefers-color-scheme: dark) {
  :root {
    --bg: #131110;
    --surface: #1C1815;
    --surface-2: #221D18;
    --border: #362F26;
    --border-strong: #4A4030;
    --text: #EDE7DE;
    --text-dim: #A89C89;
    --accent: #E3A339;
    --accent-ink: #1C1407;
    --ok: #63B894;
    --ok-bg: #1B2E27;
  }
}

* { box-sizing: border-box; }
html, body { margin: 0; padding: 0; }
body {
  background: var(--bg);
  color: var(--text);
  font-family: var(--sans);
  line-height: 1.5;
  -webkit-font-smoothing: antialiased;
}
a { color: inherit; }
img { max-width: 100%; }

/* ---------- header ---------- */
.site-header {
  border-bottom: 1px solid var(--border);
  background: var(--surface);
}
.site-header .inner {
  max-width: 1180px;
  margin: 0 auto;
  padding: 14px 24px;
  display: flex;
  align-items: center;
}
.brand {
  font-weight: 800;
  font-size: 15px;
  letter-spacing: -0.01em;
  text-decoration: none;
  display: inline-flex;
  align-items: center;
  gap: 8px;
}
.brand .dot {
  width: 8px; height: 8px; border-radius: 50%;
  background: var(--accent);
}

.page {
  max-width: 1180px;
  margin: 0 auto;
  padding: 28px 24px 96px;
}

/* ---------- headings / nav ---------- */
.crumb {
  font-family: var(--mono);
  font-size: 12px;
  color: var(--text-dim);
  margin: 0 0 8px;
}
.crumb a {
  color: var(--text-dim);
  text-decoration: none;
  border-bottom: 1px solid var(--border-strong);
}
.crumb a:hover { color: var(--text); }

.channel-block + .channel-block {
  margin-top: 44px;
  padding-top: 36px;
  border-top: 1px solid var(--border);
}
.channel-block-head {
  display: flex;
  justify-content: space-between;
  align-items: flex-end;
  gap: 16px;
  flex-wrap: wrap;
  margin-bottom: 20px;
}
.page-title {
  margin: 0;
  font-size: 26px;
  font-weight: 800;
  letter-spacing: -0.01em;
  text-wrap: balance;
}
.head-meta {
  display: flex;
  align-items: center;
  gap: 14px;
  flex-wrap: wrap;
}
.count {
  font-family: var(--mono);
  font-size: 12.5px;
  color: var(--text-dim);
  font-variant-numeric: tabular-nums;
}
.channel-links {
  display: flex;
  gap: 10px;
  flex-wrap: wrap;
}

/* ---------- pill buttons / links ---------- */
.pill {
  font-family: var(--sans);
  font-size: 13px;
  font-weight: 600;
  padding: 8px 16px;
  border-radius: 999px;
  border: 1px solid var(--border);
  background: var(--surface-2);
  color: var(--text);
  display: inline-flex;
  align-items: center;
  gap: 6px;
  cursor: pointer;
  text-decoration: none;
  transition: border-color .15s ease, background .15s ease;
}
.pill:hover { border-color: var(--border-strong); }
.pill:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.pill.primary { background: var(--accent); color: var(--accent-ink); border-color: transparent; }
.pill.ghost { background: transparent; }
.pill[disabled], .pill.disabled {
  opacity: .4;
  cursor: not-allowed;
  pointer-events: none;
}
.nav-row {
  display: flex;
  align-items: center;
  gap: 10px;
  margin: 16px 0 18px;
  flex-wrap: wrap;
}
.nav-row .n {
  font-family: var(--mono);
  font-size: 12px;
  color: var(--text-dim);
  font-variant-numeric: tabular-nums;
}

/* ---------- video grid / cards ---------- */
.grid {
  display: grid;
  grid-template-columns: repeat(auto-fill, minmax(220px, 1fr));
  gap: 18px;
}
.card {
  background: var(--surface);
  border: 1px solid var(--border);
  border-radius: var(--radius-m);
  overflow: hidden;
  display: block;
  text-decoration: none;
  color: var(--text);
  transition: transform .15s ease, border-color .15s ease, box-shadow .15s ease;
}
.card:hover {
  transform: translateY(-3px);
  border-color: var(--border-strong);
  box-shadow: 0 10px 24px -14px rgba(0,0,0,.45);
}
.card:focus-visible { outline: 2px solid var(--accent); outline-offset: 2px; }
.thumb-wrap {
  position: relative;
  aspect-ratio: 16 / 9;
  background: var(--surface-2);
  overflow: hidden;
}
.thumb-wrap img {
  width: 100%; height: 100%;
  object-fit: cover;
  display: block;
}
.idx-chip, .dur-chip {
  position: absolute;
  font-family: var(--mono);
  font-size: 11px;
  color: #fff;
  background: rgba(0,0,0,.65);
  padding: 2px 6px;
  border-radius: 4px;
  font-variant-numeric: tabular-nums;
}
.idx-chip { left: 6px; top: 6px; }
.dur-chip { right: 6px; bottom: 6px; }
.card-body { padding: 11px 12px 13px; display: flex; flex-direction: column; gap: 6px; }
.card-title {
  font-size: 13.5px;
  font-weight: 600;
  line-height: 1.35;
  display: -webkit-box;
  -webkit-line-clamp: 2;
  -webkit-box-orient: vertical;
  overflow: hidden;
}
.card-meta {
  display: flex;
  justify-content: space-between;
  font-family: var(--mono);
  font-size: 11px;
  color: var(--text-dim);
}

/* ---------- single video page ---------- */
.video-page {
  display: grid;
  grid-template-columns: minmax(0,1fr) 280px;
  gap: 26px;
}
@media (max-width: 760px) {
  .video-page { grid-template-columns: 1fr; }
}
.player {
  border: 1px solid var(--border);
  border-radius: var(--radius-m);
  overflow: hidden;
  background: var(--surface-2);
}
.player video, .player img {
  display: block;
  width: 100%;
  height: auto;
}
.video-title {
  font-size: 20px;
  font-weight: 800;
  letter-spacing: -0.01em;
  margin: 16px 0 4px;
  text-wrap: balance;
}

.chip-row { display: flex; gap: 8px; flex-wrap: wrap; margin-bottom: 18px; }
.chip {
  font-family: var(--mono);
  font-size: 11.5px;
  color: var(--text-dim);
  background: var(--surface-2);
  border: 1px solid var(--border);
  padding: 5px 10px;
  border-radius: var(--radius-s);
  font-variant-numeric: tabular-nums;
  text-decoration: none;
}
.chip.dl { color: var(--text); font-weight: 600; }
.chip.ok { color: var(--ok); background: var(--ok-bg); border-color: transparent; }

.desc {
  background: var(--surface-2);
  border: 1px solid var(--border);
  border-radius: var(--radius-m);
  padding: 14px 16px;
  font-size: 13.5px;
  color: var(--text-dim);
  white-space: pre-wrap;
  margin: 0 0 8px;
  font-family: var(--sans);
}

.cmd-box { margin-bottom: 18px; }
.cmd-box label {
  display: block;
  font-family: var(--mono);
  font-size: 11px;
  letter-spacing: .04em;
  text-transform: uppercase;
  color: var(--text-dim);
  margin-bottom: 6px;
}
.cmd-input {
  width: 100%;
  font-family: var(--mono);
  font-size: 12.5px;
  padding: 9px 10px;
  border-radius: var(--radius-s);
  border: 1px solid var(--border);
  background: var(--surface-2);
  color: var(--text);
}
.cmd-input:focus-visible { outline: 2px solid var(--accent); outline-offset: 1px; }

.side { display: flex; flex-direction: column; gap: 12px; }
.side .box {
  background: var(--surface-2);
  border: 1px solid var(--border);
  border-radius: var(--radius-m);
  padding: 14px;
}
.side .box h2 {
  margin: 0 0 8px;
  font-size: 11px;
  letter-spacing: 0.08em;
  text-transform: uppercase;
  color: var(--text-dim);
  font-weight: 700;
}
.kv { display: flex; justify-content: space-between; gap: 10px; font-size: 13px; padding: 4px 0; }
.kv .k { color: var(--text-dim); }
.kv .v { font-family: var(--mono); font-variant-numeric: tabular-nums; word-break: break-all; text-align: right; }

/* ---------- comments ---------- */
.comments { margin-top: 34px; }
.comments h2 {
  font-size: 13px;
  letter-spacing: 0.06em;
  text-transform: uppercase;
  color: var(--text-dim);
  margin: 0 0 16px;
  font-weight: 700;
}
.thread { display: flex; flex-direction: column; }
.c {
  display: flex;
  gap: 10px;
  padding: 12px 0;
  border-top: 1px solid var(--border);
  margin-left: calc(var(--depth, 0) * 36px);
}
.avatar {
  width: 30px; height: 30px; border-radius: 50%;
  background: var(--surface-2);
  border: 1px solid var(--border);
  color: var(--text);
  display: flex; align-items: center; justify-content: center;
  font-weight: 700; font-size: 12px;
  flex: none;
}
.c-body { flex: 1; min-width: 0; }
.c-head { display: flex; align-items: baseline; gap: 8px; margin-bottom: 3px; flex-wrap: wrap; }
.c-author { font-weight: 700; font-size: 13px; }
.c-time { font-family: var(--mono); font-size: 11px; color: var(--text-dim); font-variant-numeric: tabular-nums; }
.c-text {
  font-size: 13.5px;
  color: var(--text);
  opacity: .92;
  white-space: pre-wrap;
}

@media (prefers-reduced-motion: reduce) {
  .card { transition: none; }
}
)CSS";
}
