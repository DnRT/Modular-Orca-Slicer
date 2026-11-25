<div align="center">

# 🧩 Modular OrcaSlicer
### Next-Gen Extensible Slicing Architecture & Algorithm Marketplace

[![License: AGPL v3](https://img.shields.io/badge/License-AGPLv3-blue.svg)](LICENSE)
[![Platform: Windows x64](https://img.shields.io/badge/Platform-Windows%20x64-green.svg)](https://github.com/DnRT/Modular-Orca-Slicer)
[![Engine: Modular C++20](https://img.shields.io/badge/Engine-Modular%20C%2B%2B20-orange.svg)](https://github.com/DnRT/Modular-Orca-Slicer)

**Modular OrcaSlicer** is an architectural evolution of OrcaSlicer engineered to eliminate "monolithic fork fragmentation" in the 3D printing ecosystem. By decoupling the core slicing pipeline into clean, standardized subsystem hooks, Modular OrcaSlicer allows developers and researchers to implement, test, and share cutting-edge toolpath algorithms through an integrated Marketplace without modifying the core codebase.

</div>

---

## 🌟 Key Features of this Modular Fork

### 1. 🏬 Built-In Extensions Marketplace (Native UI)
A dedicated **"Plugins"** tab integrated directly into OrcaSlicer's top-level navigation bar (alongside *Prepare*, *Preview*, *Device*, and *Project*).
* **Category Filtering**: Browse extensions categorized by *Performance*, *Overhangs*, *Kinematics*, and *Installed*.
* **One-Click Activation**: Instant toggle switches with state persistence stored cleanly in `%APPDATA%\ModularOrcaSlicer\modpack.json`.
* **Zero Security Risk**: Operates on a safe compile-time modular strategy pattern with runtime registry dispatch, eliminating external unverified DLL injection vulnerabilities.

### 2. ⚡ Adaptive 2D Contour Simplifier
* **Universal Nozzle Scaling (0.2 mm to 1.0 mm)**: Dynamically scales Douglas-Peucker 2D contour decimation proportionally to nozzle diameter ($D_{\text{nozzle}} \times 0.035$).
* **Eliminates High-Poly Bottlenecks**: Drastically cuts slice times by 3x–5x on ultra-dense models (>1M–3M triangles such as Moon Lamps, lithophanes, and detailed 3D scans) by decimating sub-micrometer redundant vertices *before* feeding Arachne/Voronoi, with **zero visible loss of dimensional accuracy or surface detail**.

### 3. 🧹 Eager Layer Buffer Flusher
* **Memory Curve Stabilization**: Slicing massive models with 1,000+ layers traditionally causes monotonic linear RAM accumulation.
* **Rolling Deallocation**: Proactively deallocates non-essential intermediate buffers (`raw_slices`, non-overlap fill boundary polygons) as each layer completes its perimeter and infill calculations, keeping RAM consumption completely flat throughout long slices.

### 4. 🌊 Multi-Strategy Overhang Engine
* **Wave Overhangs**: Continuous adaptive sinusoidal toolpaths enabling self-supporting overhangs up to 85° without vertical support trees.
* **Arc Overhangs (Steven McCulloch Algorithm)**: Concentric circular arcs rooted on lower-layer anchors, spanning unsupported gaps mid-air.
* **Dynamic Algorithm Switching**: Toggle seamlessly between classic perimeters, wave overhangs, and arc overhangs directly from the UI.

### 5. 🧠 Low-RAM Model Optimizer
* Automatically caps concurrent slice worker allocations for huge meshes (>250k triangles) on RAM-limited hardware, preventing OS disk thrashing/paging and keeping the PC responsive.

### 6. 🔌 Plugin System & Developer API
A clean, modular Application Binary Interface (ABI) allowing community developers to introduce new algorithmic strategies with zero overhead:
* **Lifecycle & Hook Interfaces**: Standardized extension points defined in `src/libslic3r/Modular/ModuleABI.hpp` for `OverhangStrategy`, `InfillPattern`, `ResourceOptimizer`, and `KinematicsEngine`.
* **Ready-to-Use Templates**: Pre-configured boilerplates and manifests located in [`src/libslic3r/Modular/Templates/`](src/libslic3r/Modular/Templates/):
  - `TemplateOverhangPlugin.hpp`: Custom geometric overhang generator.
  - `TemplateInfillPlugin.hpp`: Custom infill and non-crossing wall strategies.
  - `TemplateResourceOptimizerPlugin.hpp`: Adaptive concurrency and memory scheduler.
  - `TemplateModExport.cpp`: C/C++ dynamic library export boilerplate (`create_orca_mod` / `destroy_orca_mod`).
  - `plugin.json.template`: Standard manifest specification for community packages.
* **Architecture & Community Marketplace Guide**: For detailed specifications on ABI hooks, lifecycle management, and the MakerWorld-style community distribution platform design, see [`docs/MODULAR_ARCHITECTURE_GUIDE.md`](docs/MODULAR_ARCHITECTURE_GUIDE.md).

---

## 🛠️ Windows Compilation Guide (Optimized)

To compile Modular OrcaSlicer with optimal multi-core throughput while keeping your PC completely responsive for everyday tasks:

```powershell
# Run in PowerShell (allocates balanced compiler workers while leaving threads and RAM free for multitasking):
& "C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\MSBuild\Current\Bin\amd64\MSBuild.exe" "build\src\OrcaSlicer_app_gui.vcxproj" -p:Configuration=Release -p:Platform=x64 -m:1 -p:CL_MPCount=8
```

---

<div align="center">

<picture>
  <img alt="OrcaSlicer logo" src="resources/images/OrcaSlicer.png" width="15%" height="15%">
</picture>

<a href="https://trendshift.io/repositories/15552" target="_blank"><img src="https://trendshift.io/api/badge/repositories/15552" alt="OrcaSlicer%2FOrcaSlicer | Trendshift" style="width: 250px; height: 55px;" width="250" height="55"/></a>

[![GitHub Repo stars](https://img.shields.io/github/stars/OrcaSlicer/OrcaSlicer)](https://github.com/OrcaSlicer/OrcaSlicer/stargazers) [![Build all](https://img.shields.io/github/actions/workflow/status/OrcaSlicer/OrcaSlicer/main_build_status.yml?label=Build%20all)](https://github.com/OrcaSlicer/OrcaSlicer/actions/workflows/build_all.yml)

OrcaSlicer: an open source Next-Gen Slicing Software for Precision 3D Prints.  
Optimize your prints with ultra-fast slicing, intelligent support generation, and seamless printer compatibility—engineered for perfection.

# Official links and community

#### Official Website:

<a href="https://www.orcaslicer.com/" style="font-size:2em;">OrcaSlicer.com</a>

#### Github Repository:

<a href="https://github.com/OrcaSlicer/OrcaSlicer"><img src="https://img.shields.io/badge/OrcaSlicer-181717?style=flat&logo=github&logoColor=white" width="200" alt="GitHub Logo"/> </a>

#### Follow us:

<a href="https://twitter.com/real_OrcaSlicer"><img src="https://img.shields.io/badge/real__OrcaSlicer-000000?style=flat&logo=x&logoColor=white" width="200" alt="X Logo"/> </a>  
<a href="https://www.youtube.com/@OfficialOrcaSlicer"><img src="https://img.shields.io/badge/OfficialOrcaSlicer-FF0000?style=flat&logo=youtube&logoColor=white" width="200" alt="YouTube Logo"/> </a>

#### Join our Discord community:

<a href="https://discord.gg/P4VE9UY9gJ"><img src="https://img.shields.io/badge/-Discord-5865F2?style=flat&logo=discord&logoColor=fff" width="200" alt="discord logo"/> </a>

<table border="2" style="border-color: #ffa500; background-color:rgb(232, 220, 180); color: #856404;">
<tr>
<td>
<strong>⚠️ CAUTION:</strong><br>
Several clickbait and malicious websites, such as <b>orca-slicer[.]com</b> and <b>orcaslicer[.]net</b>, are pretending to be the official OrcaSlicer site. These sites may redirect you to dangerous downloads or contain misleading information.<br>
<b>Our only official website is <a href="https://www.orcaslicer.com/">www.orcaslicer.com</a>.</b><br><br>
If you come across any of these in search results, please <b>report them</b> as unsafe or phishing to help keep the community secure with:<br>
 - <a href="https://safebrowsing.google.com/safebrowsing/report_phish/">Google Safe Browsing</a><br>
 - <a href="https://www.microsoft.com/en-us/wdsi/support/report-unsafe-site">Microsoft Security Intelligence</a><br>
 - <a href="https://ipthreat.net/tools/reportphishing">IPThreat</a>
</td>
</tr>
</table>

</div>

# Main features

- **[Advanced Calibration Tools](https://www.orcaslicer.com/wiki/calibration_guide)**  
  Comprehensive suite: temperature towers, flow rate, retraction & more for optimal performance.
- **[Precise Wall](https://www.orcaslicer.com/wiki/quality_settings_precision#precise-wall) and [Seam Control](https://www.orcaslicer.com/wiki/quality_settings_seam)**  
  Adjust outer wall spacing and apply scarf seams to enhance print accuracy.
- **[Sandwich Mode](https://www.orcaslicer.com/wiki/quality_settings_wall_and_surfaces#innerouterinner) and [Polyholes](https://www.orcaslicer.com/wiki/quality_settings_precision#polyholes) Support**  
  Use varied infill [patterns](https://www.orcaslicer.com/wiki/strength_settings_patterns) and accurate hole shapes for improved clarity.
- **[Overhang](https://www.orcaslicer.com/wiki/quality_settings_overhangs) and [Support Optimization](https://www.orcaslicer.com/wiki#support-settings)**  
  Modify geometry for printable overhangs with precise support placement.
- **[Granular Controls and Customization](https://www.orcaslicer.com/wiki#process-settings)**  
  Fine-tune print speed, layer height, pressure, and temperature with precision.
- **Network Printer Support**  
  Seamless integration with Klipper, PrusaLink, and OctoPrint for remote control.
- **[Mouse Ear Brims](https://www.orcaslicer.com/wiki/others_settings_brim) & [Adaptive Bed Mesh](https://www.orcaslicer.com/wiki/printer_basic_information_adaptive_bed_mesh)**  
  Automatic brims and adaptive mesh calibration ensure consistent adhesion.
- **User-Friendly Interface**  
  Intuitive drag-and-drop design with pre-made profiles for popular printers.
- **[Open-Source](https://github.com/OrcaSlicer/OrcaSlicer) & [Community Driven](https://discord.gg/P4VE9UY9gJ)**  
  Regular updates fueled by continuous community contributions.
- **Wide Printer Compatibility**  
  Supports a broad range of printers: Bambu Lab, Prusa, Creality, Voron, and more.
- **[Belt Printer Support](https://www.orcaslicer.com/wiki/belt_printing)**  
  Slice for belt / conveyor (infinite-Z) printers, with belt-aware supports and a tilted-bed preview. Contributed by [Joseph Robertson (@HarrierPigeon)](https://github.com/HarrierPigeon).
- **[IDEX/IQEX Parallel Printing Support](https://www.orcaslicer.com/wiki/idex_iqex_parallel_printing)**  
  Print copies or mirror images of a part on every carriage of an IDEX or IQEX printer at once, with the mode chosen per plate and nozzle clearance zones shown on the bed. Contributed by [Clifford (@cgarwood82)](https://github.com/cgarwood82).
- Additional features can be found in the [change notes](https://github.com/OrcaSlicer/OrcaSlicer/releases/).

# Wiki

The [wiki](https://www.orcaslicer.com/wiki) aims to provide a detailed explanation of the slicer settings, including how to maximize their use and how to calibrate and set up your printer.

- **[Access the wiki here](https://www.orcaslicer.com/wiki)**
- **[Contribute to the wiki](https://www.orcaslicer.com/wiki/how_to_wiki)**

# Download

## Stable Release

📥 **[Download the Latest Stable Release](https://github.com/OrcaSlicer/OrcaSlicer/releases/latest)**  
Visit our GitHub Releases page for the latest stable version of OrcaSlicer, recommended for most users.  
Windows and macOS release and nightly builds are code signed.

## Nightly Builds

🌙 **[Download the Latest Nightly Build](https://github.com/OrcaSlicer/OrcaSlicer/releases/tag/nightly-builds)**  
Explore the latest developments in OrcaSlicer with our nightly builds. Feedback on these versions is highly appreciated.

# How to install

## Windows

Download the **Windows Installer exe** for your preferred version from the [releases page](https://github.com/OrcaSlicer/OrcaSlicer/releases). Both `x64` and `arm64` installers are published — pick the one matching your CPU.

- *For convenience there is also a portable build available.*
    <details>
    <summary>Troubleshooting</summary>

  - *If you have troubles to run the build, you might need to install following runtimes:*
  - [MicrosoftEdgeWebView2RuntimeInstallerX64](https://github.com/OrcaSlicer/OrcaSlicer/releases/download/v1.0.10-sf2/MicrosoftEdgeWebView2RuntimeInstallerX64.exe)
    - [Details of this runtime](https://aka.ms/webview2)
    - [Alternative Download Link Hosted by Microsoft](https://go.microsoft.com/fwlink/p/?LinkId=2124703)
  - [vcredist2019_x64](https://github.com/OrcaSlicer/OrcaSlicer/releases/download/v1.0.10-sf2/vcredist2019_x64.exe)
    - [Alternative Download Link Hosted by Microsoft](https://aka.ms/vs/17/release/vc_redist.x64.exe)
    - This file may already be available on your computer if you've installed visual studio.  Check the following location: `%VCINSTALLDIR%Redist\MSVC\v142`
    </details>

### Microsoft Store

Install from the [Microsoft Store](https://apps.microsoft.com/detail/9mv6gl23xm59) when you prefer a Store-signed package (helps on Windows 11 Smart App Control).

### Windows Package Manager

```shell
winget install --id=SoftFever.OrcaSlicer -e
```

## Mac

1. Download the universal DMG, which runs on both Apple Silicon and Intel Macs.
2. Drag OrcaSlicer.app to Application folder.
3. *If you want to run a build from a PR, you also need to follow the instructions below:*

    <details>
    <summary>Quarantine</summary>

    - Option 1 (You only need to do this once. After that the app can be opened normally.):
      - Step 1: Hold _cmd_ and right click the app, from the context menu choose **Open**.
      - Step 2: A warning window will pop up, click _Open_

    - Option 2:
      Execute this command in terminal:

      ```shell
      xattr -dr com.apple.quarantine /Applications/OrcaSlicer.app
      ```

    - Option 3:
        - Step 1: open the app, a warning window will pop up  
            ![mac_cant_open](./SoftFever_doc/mac_cant_open.png)
        - Step 2: in `System Settings` -> `Privacy & Security`, click `Open Anyway`:  
            ![mac_security_setting](./SoftFever_doc/mac_security_setting.png)
    </details>

### Homebrew Cask

```shell
brew install --cask orcaslicer
```

The [Homebrew cask](https://formulae.brew.sh/cask/orcaslicer) installs the official macOS DMG from [GitHub Releases](https://github.com/OrcaSlicer/OrcaSlicer/releases).

## Linux

### Flathub (Recommended)

OrcaSlicer is available through FlatHub:

<a href='https://flathub.org/apps/com.orcaslicer.OrcaSlicer'><img width='240' alt='Download on Flathub' src='https://dl.flathub.org/assets/badges/flathub-badge-en.png'/></a>

Install from the command line:

```shell
flatpak install flathub com.orcaslicer.OrcaSlicer
flatpak run com.orcaslicer.OrcaSlicer
```

It can also be installed through graphical software managers (KDE Discover, GNOME Software, etc.) when Flathub is enabled. Search for **OrcaSlicer** in your software center.

### AppImage

AppImages are published for both **x86_64** and **aarch64** (ARM64). Pick the file matching your CPU — the ARM64 build has `aarch64` in its name (e.g. `OrcaSlicer_Linux_AppImage_Ubuntu2404_aarch64_*.AppImage`).

 1. Download App image from the [releases page](https://github.com/OrcaSlicer/OrcaSlicer/releases).
 2. Double click the downloaded file to run it.

 3. If you run into trouble executing it, try this command in the terminal:
    `chmod +x /path_to_appimage/OrcaSlicer_Linux.AppImage`

# How to Compile

All updated build instructions for Windows, macOS, and Linux are now available on the official [OrcaSlicer Wiki - How to build](https://www.orcaslicer.com/wiki/how_to_build) page.

Please refer to the wiki to ensure you're following the latest and most accurate steps for your platform.

# Klipper Note

If you're running Klipper, it's recommended to add the following configuration to your `printer.cfg` file.

```gcode
# Enable object exclusion
[exclude_object]

# Enable arcs support
[gcode_arcs]
resolution: 0.1
```

# Supports

**OrcaSlicer** is an open-source project, and we're deeply grateful to all our sponsors and backers.  
Their generous support helps fund filaments and other essential 3D printing materials for the project.  
Thank you! :)

## Sponsors

<table>
<tr>
<td>
<a href="https://qidi3d.com/" style="display:inline-block; border-radius:8px; background:#fff;">
  <img src="SoftFever_doc\sponsor_logos\QIDI.png" alt="QIDI" width="100" height="100">
</a>
</td>
<td>
<a href="https://bigtree-tech.com/" style="display:inline-block; border-radius:8px; background:#222;">
    <img src="SoftFever_doc\sponsor_logos\BigTreeTech.png" alt="BIGTREE TECH" width="100" height="100">
</a>
</td>
</tr>
</table>

## Backers:

**Ko-fi supporters** ☕: [Backers list](https://github.com/user-attachments/files/16147016/Supporters_638561417699952499.csv)

## Support the project

<a href="https://github.com/sponsors/SoftFever"><img src="https://img.shields.io/badge/GitHub%20Sponsors-30363D?style=flat&logo=GitHub-Sponsors&logoColor=EA4AAA" height="50"></a>  
<a href="https://ko-fi.com/G2G5IP3CP"><img src="https://img.shields.io/badge/Support_me_on_Ko--fi-FF5E5B?style=flat&logo=ko-fi&logoColor=white" height="50"></a>  
<a href="https://paypal.me/softfever3d"><img src="https://img.shields.io/badge/PayPal-003087?style=flat&logo=paypal&logoColor=fff" height="50"></a>

## Some Background

Open-source slicing has always been built on a tradition of collaboration and attribution. [Slic3r](https://github.com/Slic3r/Slic3r), created by Alessandro Ranellucci and the RepRap community, laid the foundation. [PrusaSlicer](https://github.com/prusa3d/PrusaSlicer) by Prusa Research built on Slic3r and acknowledged that heritage. [Bambu Studio](https://github.com/bambulab/BambuStudio) in turn forked from PrusaSlicer, and [SuperSlicer](https://github.com/supermerill/SuperSlicer) by @supermerill extended PrusaSlicer with community-driven enhancements. Each project carried the work of its predecessors forward, crediting those who came before.

OrcaSlicer began in that same spirit, drawing from BambuStudio, PrusaSlicer, and ideas inspired by CuraSlicer and SuperSlicer. But it has since grown far beyond its origins. Through relentless innovation — introducing advanced calibration tools, precise wall and seam control, tree supports, adaptive slicing, and hundreds of other features — OrcaSlicer has become the most widely used and actively developed open-source slicer in the 3D printing community. Many of its innovations have been adopted by other slicers, making it a driving force for the entire industry.

The OrcaSlicer logo was designed by community member [Justin Levine](https://github.com/jal-co).

# License

- **OrcaSlicer** is licensed under the GNU Affero General Public License, version 3.
- The **GNU Affero General Public License**, version 3 ensures that if you use any part of this software in any way (even behind a web server), your software must be released under the same license.
- OrcaSlicer includes a **pressure advance calibration pattern test** adapted from Andrew Ellis' generator, which is licensed under GNU General Public License, version 3. Ellis' generator is itself adapted from a generator developed by Sineos for Marlin, which is licensed under GNU General Public License, version 3.
- The **Bambu networking plugin** is based on non-free libraries from BambuLab. It is optional to OrcaSlicer and provides extended functionalities for Bambulab printer users.
