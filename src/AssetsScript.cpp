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

#include "AssetsScript.h"

namespace Assets
{
const std::string APP_JS = R"JS(
(function () {
  "use strict";

  // --- Theme selector (auto/light/dark), persisted across page loads ---
  var THEME_KEY = "yt-frontend-theme";
  var themeSelect = document.getElementById("theme-select");
  if (themeSelect) {
    var savedTheme = localStorage.getItem(THEME_KEY);
    if (savedTheme === "auto" || savedTheme === "light" || savedTheme === "dark") {
      themeSelect.value = savedTheme;
    }
    themeSelect.addEventListener("change", function () {
      localStorage.setItem(THEME_KEY, themeSelect.value);
    });
  }

  // --- Video title filter (each input filters the .card elements inside
  // the grid named by its data-filter-target id) ---
  var filterInputs = document.querySelectorAll("[data-filter-target]");
  for (var i = 0; i < filterInputs.length; i++) {
    (function (input) {
      var grid = document.getElementById(input.getAttribute("data-filter-target"));
      if (!grid) return;

      var cards = Array.prototype.slice.call(grid.querySelectorAll(".card"));
      input.addEventListener("input", function () {
        var query = input.value.trim().toLowerCase();
        cards.forEach(function (card) {
          var titleEl = card.querySelector(".card-title");
          var title = titleEl ? titleEl.textContent.toLowerCase() : "";
          var match = !query || title.indexOf(query) !== -1;
          card.style.display = match ? "" : "none";
        });
      });
    })(filterInputs[i]);
  }
})();
)JS";
}
