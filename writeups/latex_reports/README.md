# FastRobox 1.0 Write-Up LaTeX Source Files (Team T14)

## Overview
This folder contains the complete, compilation-ready LaTeX source files for all 12 Levels of the FastRobox 1.0 Techathon Grand Finale at BUBT.

Each write-up is formatted to strictly satisfy the official participant rulebook:
1. **Maximum 2 pages** per level write-up PDF.
2. Contains the **mandatory 5 sections**:
   - `1. Design`
   - `2. Where it shows` (Screenshots of Serial Monitor, Local status page, Web Dashboard, Database records)
   - `3. Test results` (Table with Video Time, Measured Values, Official Limits, Status)
   - `4. Problems and fixes` (Technical challenge log)
   - `5. Commit and tag` (Git tag and commit hash)

---

## Files in this Directory
- `T14_L01_writeup.tex` through `T14_L12_writeup.tex` — Pre-filled reports for Levels 1 to 12.
- `template_writeup.tex` — Universal 2-page template for any custom level.
- `screenshots/` — Folder containing 4 image files:
  - `serial_monitor.png` (Surface 1)
  - `local_page.png` (Surface 2)
  - `web_dashboard.png` (Surface 3)
  - `database_records.png` (Surface 4)

---

## How to Compile to PDF
### Option 1: On Overleaf (Easiest & Recommended)
1. Upload the `T14_FastRobox_Writeup_LaTeX.zip` directly to [Overleaf](https://www.overleaf.com) (`New Project` -> `Upload Project`).
2. Select any `T14_L<NN>_writeup.tex` file as the main document.
3. Click **Recompile** — it will produce a clean, publication-grade 2-page PDF immediately!

### Option 2: Locally using pdflatex or TeX Live / MiKTeX
```bash
pdflatex T14_L01_writeup.tex
```

---

## How to Add Your Own Live Screenshots Later
Whenever you capture screenshots during judging or testing:
1. Save or replace the images inside `screenshots/`:
   - `serial_monitor.png`
   - `local_page.png`
   - `web_dashboard.png`
   - `database_records.png`
2. Re-compile the `.tex` file. The new screenshots will automatically fit inside the 2x2 grid without altering the 2-page layout!
