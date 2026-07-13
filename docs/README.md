# Retro-Go WebFlasher Documentation

This directory contains a web-based firmware programmer for Retro-Go. It uses Espressif's Web Serial-based flashing technology to allow end-users to install and update Retro-Go directly from Chrome, Edge, Brave, or Opera browsers.

## How it works

1. **Releases Discovery**: When a user opens the page, it uses the GitHub API to dynamically list the releases in the repository.
2. **Targets Discovery**: It dynamically fetches the targets from the repository's `components/retro-go/targets` directory, allowing it to adapt if you add or remove hardware targets in your fork.
3. **Chip Detection**: It reads the `env.py` file of the selected target to detect the ESP chip family (`ESP32`, `ESP32-S3`, `ESP32-P4`, etc.).
4. **Flashing**: It looks for an uploaded release asset ending in `.img` matching the selected target (e.g., `retro-go_v1.0.0_odroid-go.img`). It constructs a temporary installation manifest and flashes the merged image starting at offset `0x0` of the device flash memory.

---

## Developer Guide

### 1. Build and Package the Firmware
To make your builds available to the web flasher, they must be compiled and packaged as a single merged flash image (`.img`). The WebFlasher flashes this image starting at offset `0x0`.

For each target you want to support (e.g., `odroid-go`, `cyd`, `esp32-s3-devkit`):
1. Set up your ESP-IDF environment.
2. Run the release packing command:
   ```bash
   python rg_tool.py --target=odroid-go release
   ```
3. This creates a file in your project root named:
   `retro-go_<version>_odroid-go.img` (where `<version>` is your git tag or current version string).

### 2. Upload to GitHub Releases
1. Create a Release on GitHub corresponding to your version tag.
2. Upload the compiled `.img` files (e.g., `retro-go_v1.0.0_odroid-go.img`, `retro-go_v1.0.0_cyd.img`, etc.) as assets of that Release.
3. The WebFlasher will automatically parse the release assets, match them to the target selected by the user, and present the **Install** button.

### 3. Deploy to GitHub Pages
To host the WebFlasher on your fork's GitHub Pages:
1. Go to your repository settings on GitHub.
2. Under **Pages** (in the sidebar under Code and automation), set the Source to **Deploy from a branch**.
3. Choose your main/master branch, and select the `/docs` folder or repository root if your site is set up there.
4. If you only want to serve this specific subfolder, you can set up a GitHub Action workflow to publish the `webflash` folder to the `gh-pages` branch automatically.

#### Example GitHub Action (`.github/workflows/deploy-pages.yml`)
To deploy only this folder automatically on commits to the main branch:
```yaml
name: Deploy WebFlasher to Pages

on:
  push:
    branches: [main]
    paths:
      - 'webflash/**'
  workflow_dispatch:

permissions:
  contents: read
  pages: write
  id-token: write

concurrency:
  group: "pages"
  cancel-in-progress: false

jobs:
  deploy:
    environment:
      name: github-pages
      url: ${{ steps.deployment.outputs.page_url }}
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4
      - name: Setup Pages
        uses: actions/configure-pages@v4
      - name: Upload artifact
        uses: actions/upload-pages-artifact@v3
        with:
          path: './webflash'
      - name: Deploy to GitHub Pages
        id: deployment
        uses: actions/deploy-pages@v4
```

---

## Local Development & Testing

To run the WebFlasher locally:
1. Start a local HTTP server in the `webflash` folder:
   ```bash
   # Python 3
   python -m http.server 8000
   ```
2. Open `http://localhost:8000` in a supported browser (Chrome, Edge, etc.).
3. Click the **Settings** button inside the header card.
4. Input your GitHub username and repository name to query your fork's releases, and input an optional GitHub token if the repository is private or if you run into rate limits.
