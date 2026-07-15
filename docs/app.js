// Retro-Go WebFlasher Application Logic

// Fallback lists and configurations
const FALLBACK_TARGETS = [
  'odroid-go', 'esp32-s3-devkit', 'esp32-p4-devkit', 't-deck-plus',
  'byteboi-rev1', 'crokpocket', 'brutzelboy', 'esplay-micro', 'fri3d-2024',
  'mrgc-g32', 'mrgc-gbm', 'nullnano', 'rachel-esp32',
  'redroid-go', 'retro-esp32', 'retro-ruler', 'vmu-s3'
];

const TARGET_CHIP_MAP = {
  'brutzelboy': 'ESP32-S3',
  'byteboi-rev1': 'ESP32',
  'crokpocket': 'ESP32-S3',
  'esp32-p4-devkit': 'ESP32-P4',
  'esp32-s3-devkit': 'ESP32-S3',
  'esplay-micro': 'ESP32',
  'fri3d-2024': 'ESP32-S3',
  'mrgc-g32': 'ESP32',
  'mrgc-gbm': 'ESP32',
  'nullnano': 'ESP32',
  'odroid-go': 'ESP32',
  'rachel-esp32': 'ESP32-S3',
  'redroid-go': 'ESP32',
  'retro-esp32': 'ESP32',
  'retro-ruler': 'ESP32-S3',
  't-deck-plus': 'ESP32-S3',
  'vmu-s3': 'ESP32-S3'
};

// Target SVG Icons for premium card layout
const TARGET_ICONS = {
  'odroid-go': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="5" y="2" width="14" height="20" rx="2"/><line x1="8" y1="18" x2="10" y2="18"/><line x1="14" y1="18" x2="16" y2="18"/><circle cx="12" cy="18" r="1"/><rect x="8" y="5" width="8" height="7" rx="1"/></svg>`,
  'cyd': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="3" y="5" width="18" height="14" rx="2"/><rect x="6" y="8" width="12" height="8"/><circle cx="19" cy="12" r="0.5"/></svg>`,
  'esp32-s3-devkit': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="6" y="2" width="12" height="20" rx="1"/><path d="M6 6h12M6 10h12M6 14h12M6 18h12"/></svg>`,
  'esp32-p4-devkit': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="4" y="4" width="16" height="16" rx="2"/><circle cx="12" cy="12" r="3"/><path d="M12 2v2M12 20v2M2 12h2M20 12h2"/></svg>`,
  't-deck-plus': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="4" y="3" width="16" height="18" rx="2"/><rect x="6" y="6" width="12" height="6" rx="1"/><rect x="6" y="14" width="12" height="4" rx="1"/></svg>`,
  'generic': `<svg viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><rect x="2" y="2" width="20" height="20" rx="2"/><path d="M6 12h12M12 6v12"/></svg>`
};

// Global App State
let state = {
  owner: 'DynaMight1124',
  repo: 'retro-go',
  token: '',
  releases: [],
  selectedRelease: null,
  targets: [],
  selectedTarget: '',
  selectedChip: '',
  localFirmwares: [],
  localFileBlobUrl: null
};

// Initialize Application
document.addEventListener('DOMContentLoaded', () => {
  detectRepository();
  loadSavedSettings();
  setupUIEventListeners();
  initializeAppState();
});

// Detect repository info from GitHub Pages URL structure
function detectRepository() {
  const hostname = window.location.hostname;
  const pathname = window.location.pathname;

  if (hostname.endsWith('.github.io')) {
    state.owner = hostname.split('.')[0];
    const paths = pathname.split('/').filter(Boolean);
    if (paths.length > 0) {
      state.repo = paths[0];
    }
  }
}

// Load configurations from localStorage
function loadSavedSettings() {
  const savedOwner = localStorage.getItem('rg_webflash_owner');
  const savedRepo = localStorage.getItem('rg_webflash_repo');
  const savedToken = localStorage.getItem('rg_webflash_token');

  if (savedOwner) state.owner = savedOwner;
  if (savedRepo) state.repo = savedRepo;
  if (savedToken) state.token = savedToken;

  // Populate config fields
  document.getElementById('input-owner').value = state.owner;
  document.getElementById('input-repo').value = state.repo;
  document.getElementById('input-token').value = state.token;

  updateRepoDisplay();
}

function updateRepoDisplay() {
  document.getElementById('repo-display-name').textContent = `${state.owner}/${state.repo}`;
}

// Bind UI event handlers
function setupUIEventListeners() {
  // Toggle advanced panel
  const toggleBtn = document.getElementById('btn-advanced-toggle');
  const panel = document.getElementById('advanced-panel');
  toggleBtn.addEventListener('click', () => {
    panel.classList.toggle('open');
    const isOpen = panel.classList.contains('open');
    toggleBtn.querySelector('.arrow').style.transform = isOpen ? 'rotate(180deg)' : 'rotate(0deg)';
  });

  // Apply settings button
  document.getElementById('btn-apply-settings').addEventListener('click', () => {
    state.owner = document.getElementById('input-owner').value.trim();
    state.repo = document.getElementById('input-repo').value.trim();
    state.token = document.getElementById('input-token').value.trim();

    localStorage.setItem('rg_webflash_owner', state.owner);
    localStorage.setItem('rg_webflash_repo', state.repo);
    localStorage.setItem('rg_webflash_token', state.token);

    updateRepoDisplay();
    panel.classList.remove('open');
    toggleBtn.querySelector('.arrow').style.transform = 'rotate(0deg)';

    // Re-fetch data
    initializeAppState();
  });

  // Reset settings button
  document.getElementById('btn-reset-settings').addEventListener('click', () => {
    localStorage.removeItem('rg_webflash_owner');
    localStorage.removeItem('rg_webflash_repo');
    localStorage.removeItem('rg_webflash_token');
    
    detectRepository();
    state.token = '';
    
    document.getElementById('input-owner').value = state.owner;
    document.getElementById('input-repo').value = state.repo;
    document.getElementById('input-token').value = '';

    updateRepoDisplay();
    panel.classList.remove('open');
    toggleBtn.querySelector('.arrow').style.transform = 'rotate(0deg)';

    initializeAppState();
  });

  // Release selection change handler
  document.getElementById('select-release').addEventListener('change', (e) => {
    const val = e.target.value;
    state.selectedRelease = state.releases.find(r => r.tag_name === val) || null;
    resetLocalFileState();
    renderTargetsGrid();
    updateInstallButtonAndDetails();
  });

  // Local file picker handler
  document.getElementById('input-local-file').addEventListener('change', handleLocalFileSelect);
}

// Kick off data fetching
async function initializeAppState() {
  showReleaseLoader(true);
  showTargetsLoader(true);

  // Fetch Releases, Targets, and Local Firmwares in parallel
  await Promise.all([
    fetchReleases(),
    fetchTargets(),
    fetchLocalFirmwares()
  ]);

  showReleaseLoader(false);
  showTargetsLoader(false);

  // Render targets grid dynamically based on the selected release
  renderTargetsGrid();
}

// Generate Headers with Optional GitHub Token
function getFetchHeaders() {
  const headers = {};
  if (state.token) {
    headers['Authorization'] = `token ${state.token}`;
  }
  return headers;
}

// Fetch releases from GitHub API
async function fetchReleases() {
  const selectEl = document.getElementById('select-release');
  selectEl.innerHTML = '';
  state.releases = [];
  state.selectedRelease = null;

  try {
    const res = await fetch(`https://api.github.com/repos/${state.owner}/${state.repo}/releases`, {
      headers: getFetchHeaders()
    });

    if (!res.ok) {
      if (res.status === 403) {
        const remaining = res.headers.get('x-ratelimit-remaining');
        if (remaining === '0') {
          const resetVal = res.headers.get('x-ratelimit-reset');
          const resetTime = resetVal ? new Date(parseInt(resetVal) * 1000).toLocaleTimeString() : 'later';
          throw new Error(`Rate limit reached. Resets at ${resetTime}`);
        }
        throw new Error(`Forbidden (403). Check your token or repository permissions.`);
      }
      if (res.status === 404) {
        throw new Error(`Repository not found (404). Check owner/repo or token (if private).`);
      }
      throw new Error(`HTTP Error ${res.status}: ${res.statusText || 'Unknown'}`);
    }

    const data = await res.json();
    state.releases = data.filter(r => !r.draft);

    if (state.releases.length === 0) {
      const opt = document.createElement('option');
      opt.textContent = 'No releases found';
      opt.disabled = true;
      selectEl.appendChild(opt);
      showWarning('No public GitHub Releases found in this repository. Please compile and upload your Retro-Go releases first.');
      return;
    }

    // Populate dropdown
    state.releases.forEach(rel => {
      const opt = document.createElement('option');
      opt.value = rel.tag_name;
      opt.textContent = `${rel.name || rel.tag_name} (${new Date(rel.published_at).toLocaleDateString()})${rel.prerelease ? ' [Pre-release]' : ''}`;
      selectEl.appendChild(opt);
    });

    // Select latest
    state.selectedRelease = state.releases[0];
    selectEl.value = state.selectedRelease.tag_name;
    hideWarning();

  } catch (err) {
    console.error(err);
    const opt = document.createElement('option');
    opt.textContent = 'API Error - Rate Limit?';
    selectEl.appendChild(opt);
    showWarning(`Could not fetch releases from GitHub API (${err.message}). Enter a GitHub Token in Settings if rate-limited or using a private repository.`);
  }
}

// Fetch supported targets list
async function fetchTargets() {
  state.targets = [];
  const container = document.getElementById('targets-grid');
  container.innerHTML = '';

  try {
    // Try to fetch targets from the 'webflash' branch first (where the webflasher is typically hosted)
    let res = await fetch(`https://api.github.com/repos/${state.owner}/${state.repo}/contents/components/retro-go/targets?ref=webflash`, {
      headers: getFetchHeaders()
    });

    // If webflash branch does not exist, fall back to the default branch
    if (!res.ok) {
      res = await fetch(`https://api.github.com/repos/${state.owner}/${state.repo}/contents/components/retro-go/targets`, {
        headers: getFetchHeaders()
      });
    }

    if (!res.ok) {
      throw new Error(`Failed to list targets folder: HTTP ${res.status} ${res.statusText || 'Error'}`);
    }

    const EXCLUDED_TARGETS = ['sdl2'];
    const items = await res.json();
    state.targets = items
      .filter(item => item.type === 'dir' && !EXCLUDED_TARGETS.includes(item.name.toLowerCase()))
      .map(item => item.name);

  } catch (err) {
    console.warn('GitHub API target lookup failed. Falling back to local hardcoded targets.', err);
    state.targets = [...FALLBACK_TARGETS];
  }

}

// Fetch list of locally hosted firmwares in docs/firmware/ folder
async function fetchLocalFirmwares() {
  state.localFirmwares = [];
  try {
    const res = await fetch(`https://api.github.com/repos/${state.owner}/${state.repo}/contents/docs/firmware?ref=webflash`, {
      headers: getFetchHeaders()
    });

    if (res.ok) {
      const items = await res.json();
      state.localFirmwares = items
        .filter(item => item.type === 'file' && item.name.toLowerCase().endsWith('.img'))
        .map(item => item.name);
    }
  } catch (err) {
    console.warn('Unable to lookup docs/firmware/ folder. Falling back to external release URLs.', err);
  }
}

// Select a device target
async function selectTarget(target) {
  state.selectedTarget = target;
  resetLocalFileState();

  // Visual selection styling
  document.querySelectorAll('.target-card').forEach(el => el.classList.remove('active'));
  const card = document.getElementById(`target-card-${target}`);
  if (card) {
    card.classList.add('active');
    card.scrollIntoView({ behavior: 'smooth', block: 'nearest' });
  }

  // Find chip family
  state.selectedChip = TARGET_CHIP_MAP[target] || 'ESP32';
  document.getElementById('info-target-chip').textContent = `${state.selectedChip} (Loading details...)`;

  // Fetch env.py to confirm chip family dynamically (asynchronous)
  try {
    const ref = state.selectedRelease ? state.selectedRelease.tag_name : 'main';
    const envUrl = `https://api.github.com/repos/${state.owner}/${state.repo}/contents/components/retro-go/targets/${target}/env.py?ref=${ref}`;
    const res = await fetch(envUrl, { headers: getFetchHeaders() });
    
    if (res.ok) {
      const data = await res.json();
      const content = atob(data.content);
      const match = content.match(/IDF_TARGET\s*=\s*["']([^"']+)["']|IDF_TARGET["']\s*\]\s*=\s*["']([^"']+)["']/);
      if (match) {
        const idfTarget = match[1] || match[2];
        state.selectedChip = mapTargetToChipFamily(idfTarget);
      }
    }
  } catch (err) {
    console.warn(`Unable to fetch env.py for target ${target}, using fallback map.`, err);
  }

  document.getElementById('info-target-chip').textContent = state.selectedChip;
  updateInstallButtonAndDetails();
}

// Map IDF Target to esp-web-tools chipFamily name
function mapTargetToChipFamily(idfTarget) {
  const targetLower = idfTarget.toLowerCase().replace(/[^a-z0-9]/g, '');
  if (targetLower === 'esp32s3') return 'ESP32-S3';
  if (targetLower === 'esp32p4') return 'ESP32-P4';
  if (targetLower === 'esp32s2') return 'ESP32-S2';
  if (targetLower === 'esp32c3') return 'ESP32-C3';
  if (targetLower === 'esp32c6') return 'ESP32-C6';
  if (targetLower === 'esp32h2') return 'ESP32-H2';
  if (targetLower === 'esp32') return 'ESP32';
  return 'ESP32';
}

// Dynamically generate firmware manifest and update esp-web-tools button
function updateInstallButtonAndDetails() {
  const detailTarget = document.getElementById('info-target-name');
  const detailFile = document.getElementById('info-filename');
  const detailSize = document.getElementById('info-filesize');
  const buttonContainer = document.getElementById('button-container');

  detailTarget.textContent = state.selectedTarget || 'None';
  detailFile.textContent = 'Searching release assets...';
  detailSize.textContent = '-';
  buttonContainer.innerHTML = ''; // Clear flash button

  // Revoke old blob URL
  if (state.manifestBlobUrl) {
    URL.revokeObjectURL(state.manifestBlobUrl);
    state.manifestBlobUrl = null;
  }

  // Get manual file section elements
  const manualFileSec = document.getElementById('manual-file-section');
  const manualDlBtn = document.getElementById('btn-manual-download');

  const targetLower = state.selectedTarget ? state.selectedTarget.toLowerCase() : '';
  
  // Find compiled .img asset in release if a release is selected and we have a target
  let matchedAsset = null;
  if (state.selectedRelease && targetLower) {
    matchedAsset = state.selectedRelease.assets.find(asset => {
      const nameLower = asset.name.toLowerCase();
      // Matches if name contains target name and ends with .img
      return nameLower.includes(targetLower) && nameLower.endsWith('.img');
    });
  }

  // Determine flash path and display details
  let flashPath = '';

  if (state.localFileBlobUrl) {
    // Manually selected local file is present! (CORS-safe, works offline!)
    flashPath = state.localFileBlobUrl;
    const fileInput = document.getElementById('input-local-file');
    detailFile.textContent = (fileInput && fileInput.files[0]) ? fileInput.files[0].name : 'Local File';
    detailSize.textContent = '-';
    hideWarning();
    // Keep manual section visible so they know a local file is loaded
  } else if (matchedAsset) {
    // Found in release! Update details
    hideWarning();
    detailFile.textContent = matchedAsset.name;
    detailSize.textContent = formatBytes(matchedAsset.size);

    const isLocal = state.localFirmwares.includes(matchedAsset.name);
    if (isLocal) {
      // Served directly from GitHub Pages (CORS-safe, fast, supports chunked streams!)
      flashPath = `firmware/${matchedAsset.name}`;
      manualFileSec.style.display = 'none';
    } else {
      // Loaded directly from GitHub Releases (Will fail with CORS error in browser)
      flashPath = matchedAsset.browser_download_url;
      
      // Update manual download link and show manual selection section
      manualDlBtn.href = matchedAsset.browser_download_url;
      manualDlBtn.download = matchedAsset.name;
      manualDlBtn.style.display = 'inline-flex';
      manualFileSec.style.display = 'block';
    }
  } else {
    // No release asset found or offline/rate-limited
    detailFile.textContent = 'No firmware selected (offline/rate-limited)';
    detailSize.textContent = '-';
    
    // We are offline or rate-limited. Let user flash a local file manually!
    showWarning(`⚠️ Offline Mode / API Limit: Could not retrieve firmware releases. You can still select a device target above and flash a local firmware file from your computer using Step 2 below.`);
    
    // Hide Step 1 (Download) because we don't have a release link, but show Step 2 (Select file)
    manualDlBtn.style.display = 'none';
    manualFileSec.style.display = 'block';
    return; // Don't render install button until a local file is chosen
  }

  // Construct Manifest Object
  const manifest = {
    name: "Retro-Go",
    version: state.selectedRelease ? state.selectedRelease.tag_name : "Local",
    builds: [
      {
        chipFamily: state.selectedChip,
        parts: [
          { path: flashPath, offset: 0 }
        ]
      }
    ]
  };

  // Convert to Blob URL
  const blob = new Blob([JSON.stringify(manifest, null, 2)], { type: 'application/json' });
  state.manifestBlobUrl = URL.createObjectURL(blob);

  // Create installation button
  const installButton = document.createElement('esp-web-install-button');
  installButton.id = 'esp-install-btn';
  installButton.manifest = state.manifestBlobUrl;

  buttonContainer.appendChild(installButton);
}

// Helper formats
function formatBytes(bytes, decimals = 2) {
  if (bytes === 0) return '0 Bytes';
  const k = 1024;
  const dm = decimals < 0 ? 0 : decimals;
  const sizes = ['Bytes', 'KB', 'MB', 'GB'];
  const i = Math.floor(Math.log(bytes) / Math.log(k));
  return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
}

// UI Alert boxes helper
function showWarning(message) {
  const warningEl = document.getElementById('warning-box');
  warningEl.querySelector('.alert-text').textContent = message;
  warningEl.style.display = 'flex';
}

function hideWarning() {
  document.getElementById('warning-box').style.display = 'none';
}

// Loaders helper
function showReleaseLoader(show) {
  const container = document.getElementById('select-release-container');
  if (show) {
    container.classList.add('skeleton');
    document.getElementById('select-release').style.opacity = '0.3';
  } else {
    container.classList.remove('skeleton');
    document.getElementById('select-release').style.opacity = '1';
  }
}

function showTargetsLoader(show) {
  const container = document.getElementById('targets-grid');
  if (show) {
    container.innerHTML = `
      <div class="target-card skeleton" style="height: 100px;"></div>
      <div class="target-card skeleton" style="height: 100px;"></div>
      <div class="target-card skeleton" style="height: 100px;"></div>
      <div class="target-card skeleton" style="height: 100px;"></div>
    `;
  }
}

// Local file helpers for CORS fallback
function resetLocalFileState() {
  if (state.localFileBlobUrl) {
    URL.revokeObjectURL(state.localFileBlobUrl);
    state.localFileBlobUrl = null;
  }
  const fileInput = document.getElementById('input-local-file');
  if (fileInput) fileInput.value = '';
  const statusEl = document.getElementById('local-file-status');
  if (statusEl) {
    statusEl.textContent = '';
    statusEl.style.display = 'none';
  }
}

function handleLocalFileSelect(e) {
  const file = e.target.files[0];
  if (!file) return;

  // Revoke old blob URL if it exists
  if (state.localFileBlobUrl) {
    URL.revokeObjectURL(state.localFileBlobUrl);
  }

  // Create new Blob URL
  state.localFileBlobUrl = URL.createObjectURL(file);

  // Update UI status
  const statusEl = document.getElementById('local-file-status');
  statusEl.innerHTML = `✓ Loaded: ${file.name} (${formatBytes(file.size)})`;
  statusEl.style.display = 'block';

  // Re-generate manifest and installer button
  updateInstallButtonAndDetails();
}

// Parse list of targets dynamically from the selected release assets (.img files)
function getTargetsFromRelease(release) {
  if (!release || !release.assets) return [];
  const releaseTargets = new Set();
  release.assets.forEach(asset => {
    const nameLower = asset.name.toLowerCase();
    if (nameLower.endsWith('.img') && nameLower.startsWith('retro-go_')) {
      const parts = nameLower.split('_');
      if (parts.length >= 3) {
        // Target is the part of the filename after the last underscore, before .img
        const target = parts[parts.length - 1].replace(/\.img$/, '');
        // Exclude generic or unwanted helper names
        if (target !== 'sdl2') {
          releaseTargets.add(target);
        }
      }
    }
  });
  return Array.from(releaseTargets);
}

// Render targets dynamically based on the current selection
function renderTargetsGrid() {
  const container = document.getElementById('targets-grid');
  container.innerHTML = '';

  let targetsToRender = [];

  if (state.selectedRelease) {
    // Release is selected: parse targets from its assets
    const releaseTargets = getTargetsFromRelease(state.selectedRelease);
    
    targetsToRender = releaseTargets.map(name => {
      return {
        name: name,
        isPlaceholder: !state.targets.includes(name)
      };
    });
  } else {
    // Offline/Rate-limited: show all repo targets so user can upload local file
    targetsToRender = state.targets.map(name => {
      return {
        name: name,
        isPlaceholder: false
      };
    });
  }

  if (targetsToRender.length === 0) {
    container.innerHTML = '<div style="color:var(--text-secondary); padding:2rem; text-align:center; grid-column:1/-1;">No compatible firmware targets found in this release.</div>';
    return;
  }

  targetsToRender.forEach(item => {
    const target = item.name;
    const card = document.createElement('div');
    card.className = 'target-card';
    card.id = `target-card-${target}`;
    card.dataset.targetName = target;

    // Get chip family from hardcoded map or guess based on suffix
    let matchedChip = TARGET_CHIP_MAP[target];
    if (!matchedChip) {
      if (target.endsWith('-s3') || target.includes('-s3-')) matchedChip = 'ESP32-S3';
      else if (target.endsWith('-p4') || target.includes('-p4-')) matchedChip = 'ESP32-P4';
      else if (target.endsWith('-s2') || target.includes('-s2-')) matchedChip = 'ESP32-S2';
      else if (target.endsWith('-c3') || target.includes('-c3-')) matchedChip = 'ESP32-C3';
      else matchedChip = 'ESP32'; // fallback for CYD etc.
    }

    const badgeHtml = item.isPlaceholder ? '<span class="badge" style="font-size:0.65rem; background:rgba(255,255,255,0.06); color:var(--accent-cyan); margin-left:0.25rem; padding:0.15rem 0.35rem; vertical-align:middle; border-radius:0.25rem;">Release Asset</span>' : '';

    card.innerHTML = `
      <div class="target-icon">${TARGET_ICONS['odroid-go']}</div>
      <div class="target-name" style="display:flex; align-items:center; justify-content:center; gap:0.25rem;">${target}${badgeHtml}</div>
      <div class="target-chip">${matchedChip}</div>
    `;

    card.addEventListener('click', () => selectTarget(target));
    container.appendChild(card);
  });

  // Automatically select a target if the currently selected one is not valid for this release
  const exists = targetsToRender.some(t => t.name === state.selectedTarget);
  if (!exists && targetsToRender.length > 0) {
    const hasOdroid = targetsToRender.some(t => t.name === 'odroid-go');
    selectTarget(hasOdroid ? 'odroid-go' : targetsToRender[0].name);
  } else if (state.selectedTarget) {
    const activeCard = document.getElementById(`target-card-${state.selectedTarget}`);
    if (activeCard) activeCard.classList.add('active');
  }
}
