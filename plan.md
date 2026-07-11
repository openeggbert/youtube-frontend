# youtube-frontend — analýza, bugy a návrh vylepšení

Datum: 2026-07-11
Rozsah: celý C++ generátor (`src/`, `include/`) + vygenerovaný HTML/CSS výstup.

> **Stav: fáze A–D implementovány** (2026-07-11). Všechny bugy popsané níže
> jsou opravené, redesign (sdílený `assets/style.css`, CSS grid mřížka karet,
> přepracovaná stránka videa) je hotový a ověřený buildem + během nástroje na
> testovacích datech (validní HTML, funkční escapování, opravený
> `videos-per-row`). Zbytek dokumentu je ponechán jako záznam původní analýzy.

Co to je: nástroj generující statické HTML stránky (přehled kanálů/videí + stránka
jednotlivého videa s komentáři) nad archivem staženým přes ArchiveBox/yt-dlp.

---

## 1. Nalezené bugy

### Kritické

1. **Chybí HTML-escapování textů z YouTube.**
   `title`, `description`, `channel name`, komentáře (`author`, `text`) se vkládají
   do HTML přímo, bez escapování `<`, `>`, `&`, `"`.
   - `src/YoutubeVideoHtml.cpp:104` (`<title>`), `:145` (nadpis videa), `:200-206`
     (popis), `:213`, `:226-228` (komentáře)
   - `src/Main.cpp:269` (`<h1>` kanál), `:354` (titulek náhledu)
   - Reálný dopad: běžné znaky v názvech videí (`&`, `<`, uvozovky) rozbijí
     stránku nebo vloží neočekávaný markup. Není to jen kosmetika — je to
     nejčastější příčina „rozbitého vzhledu" generovaných stránek.

2. **Neplatné HTML v tabulce s náhledy videí.**
   `src/Main.cpp:305-308, 373-376, 403-405` — `<tr>` se otevírá opakovaně, ale
   **nikde se nezavírá `</tr>`**. Na konci se navíc přidá prázdný `<tr>` i když
   řádek nemá žádné `<td>`. Prohlížeče si s tím poradí automatickou opravou, ale
   je to křehké a znemožňuje to čistě stylovat řádky/mřížku přes CSS.

3. **`--videos-per-row` s hodnotou < 2 rozbije layout.**
   `src/Args.cpp:64-70` — při neplatné hodnotě se argument nastaví na `"0"`
   (string), ne na „nezadáno". `Main.cpp:271` pak počítá
   `max-width: (THUMBNAIL_WIDTH+20) * vpr` → **`max-width:0px`** na obalovém
   `<div>`, a `.value_or(4)` (Main.cpp:288) se nikdy neuplatní, protože `Args`
   vrátí platnou hodnotu `"0"`, ne `nullopt`. Výsledek: celý blok videí se
   může efektivně schovat/zmáčknout na nulovou šířku.

### Střední závažnost

4. **Tiché polykání chyb při zmenšování náhledu.**
   `src/Main.cpp:334-339` — `catch (...) {}` kolem `Utils::resizeImage`. Při
   chybě se pokračuje s prázdným `bytes`, vygeneruje se
   `data:image/jpg;base64,` (prázdný/rozbitý obrázek) bez jakéhokoli varování
   v logu.

5. **Špatný/pevně daný MIME typ pro base64 náhledy.**
   `src/Main.cpp:342` — vždy `image/jpg` bez ohledu na skutečný formát
   (může být `.webp`, `.png`...). Správně by měl být `image/jpeg` (ne `jpg`)
   nebo dynamický podle `getThumbnailFormat()`.

6. **`escapeForShell` je neúplné a míchá se s HTML escapováním.**
   `src/YoutubeVideoHtml.cpp:34-72` — neescapuje `$`, zpětné apostrofy,
   `<`, `>`, nový řádek. Navíc se výsledek (obsahující `\"` apod.) vkládá
   přímo do HTML atributu `value="..."` (řádky 185-189) **bez HTML-escapování**
   — název souboru s uvozovkou rozbije atribut a zbytek stránky.

7. **Neošetřené parsování `metadata` souboru.**
   `src/YoutubeVideo.cpp:82-124` (větev CASE 1, čtení z cache) — `std::stoll`
   / `std::stoi` na hodnotách z `props` mapy bez try/catch. Chybějící nebo
   poškozený řádek v souboru (např. starý formát bez `number=`) = pád celého
   programu uprostřed generování.

8. **Prázdné `miniThumbnail` → rozbitý název souboru.**
   `src/YoutubeVideo.cpp:150-162` — pokud žádný náhled nesplní práh šířky,
   `miniThumbnail` zůstane `""`, `getMiniThumbnailFormat()` vrátí `""` a
   vznikne cesta `mini-thumbnail.` (tečka bez přípony) → nenačtitelný obrázek.

### Menší / úklid kódu

9. **Globální statické proměnné jako počítadla layoutu.**
   `src/Main.cpp:46-47` (`iii`, `internalStaticVariableVideoNumberPerRow`) —
   fungují, ale jsou to file-scope statics měněné uvnitř funkce; matoucí a
   zbytečně křehké, měly by být lokální proměnné/parametry.

10. **Mrtvý kód v `Utils`.**
    `getCountOfSlashOccurrences`, `createDoubleDotSlash`, `listAllFilesInDir`,
    `copyFile` (`src/Utils.cpp`) — nikde v projektu se nepoužívají (ověřeno
    grepem). Buď dokončit zamýšlené použití, nebo smazat.

---

## 2. Vizuální / CSS vylepšení generovaného výstupu (hlavní požadavek)

### Co je špatně dnes
- Žádný sdílený CSS soubor — každá stránka má vlastní `<style>` blok jen se
  dvěma pravidly (`padding`, `font-family:Arial`), zbytek je **inline style
  na každém elementu** → nekonzistentní, needržovatelné, nejde snadno
  přidat dark mode ani responzivitu.
- Grid videí je `<table>` s pevnou šířkou podle `videos-per-row` — nereaguje
  na šířku okna, na mobilu/menším okně přeteče.
  - `Main.cpp:270-272, 292, 310-311`
- Žádné hover/focus stavy, žádné zaoblené rohy, žádné stíny, žádná typografická
  hierarchie — čistý Arial, černý text na bílém pozadí.
- Tlačítka Back/Next jsou syrové `<button>` s inline stylem, disabled stav řešen
  nekonzistentně (`disabled` atribut vs. `visibility:hidden`).
  - `YoutubeVideoHtml.cpp:150-171`
- Komentáře jsou odsazovány `margin-left: dotCount()*50px` — u hlubších vláken
  vlákna „utíkají" mimo obrazovku, protože nic neomezuje šířku ani nezalamuje
  layout doprava.

### Návrh řešení
1. **Vytáhnout CSS do jednoho sdíleného souboru** (`assets/style.css`),
   zkopírovaného do kořene výstupu, linkovaného přes `<link rel="stylesheet">`
   ve všech generovaných stránkách místo duplicitních `<style>` bloků a
   inline stylů. Jedno místo pro téma → snadná budoucí úprava.
2. **Tmavé téma ve stylu YouTube** (s fallbackem na `prefers-color-scheme`),
   čitelný systémový font-stack místo Arialu.
3. **CSS Grid místo `<table>`** pro mřížku náhledů:
   `grid-template-columns: repeat(auto-fill, minmax(240px, 1fr))` — responzivní
   samo o sobě, `videos-per-row` přestává být nutné pro layout (může zůstat jen
   jako horní limit šířky nebo se úplně zruší).
4. **Karty pro náhledy videí** — zaoblené rohy, jemný stín, hover efekt
   (zvětšení/zesvětlení), název videa max. 2 řádky s `line-clamp` a `…`.
5. **Přepracovaná stránka jednotlivého videa** — vycentrovaný přehrávač,
   pilulková tlačítka Zpět/Další se šipkami, přehledný řádek s metadaty
   (velikost, délka, datum) jako „chips", komentáře vizuálně provázané
   odsazením + jemnou vodicí čárou, avatar jako kolečko s iniciálou autora.
6. **Konzistentní stavy tlačítek** přes CSS třídy (`.btn`, `.btn:disabled`)
   místo mixu `disabled` / `visibility:hidden`.
7. Coby vedlejší efekt této přestavby: zavést `escapeHtml()` utilitu a použít
   ji všude, kde se dnes text vkládá bez escapování (řeší bug #1).

Připravil jsem **vizuální náhled** nového designu (mřížka kanálu + stránka
videa s komentáři) jako artifact, abychom si směr odsouhlasili dřív, než se
pustíme do C++ implementace — viz odkaz v chatu.

---

## 3. Navrhované fáze prací

| Fáze | Obsah | Riziko/náročnost |
|---|---|---|
| A | Oprava kritických bugů (#1 escapování, #2 `<tr>`, #3 `videos-per-row`) | nízké, rychle hotové |
| B | Extrakce CSS + redesign (grid, tmavé téma, karty, tlačítka) | střední, hlavní vizuální dopad |
| C | Oprava zbylých středních bugů (#4–#8: MIME, chybové stavy, parsování) | nízké–střední |
| D | Úklid kódu (#9 static proměnné, #10 mrtvý kód) | nízké, kosmetika kódu |

Doporučení: A + B dohromady dávají smysl dělat naráz (přestavba HTML šablon
je stejně nutná pro obojí), C a D lze odložit na později.

---

## 4. Otevřené otázky pro rozhodnutí
- Chceš tmavé téma jako výchozí, nebo světlé s automatickým přepnutím podle
  systému (`prefers-color-scheme`)?
- Má `videos-per-row` zůstat jako parametr (max. šířka mřížky), nebo ho čistě
  nahradit responzivním CSS gridem bez omezení?
- Priorita: pustit se rovnou do implementace (fáze A+B), nebo nejdřív jen
  doladit vizuální náhled?
