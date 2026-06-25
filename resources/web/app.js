const state = {
  games: [],
  filter: 'GBA',
  search: '',
  selected: null,
  cropImage: null,
  page: 1,
  pageSize: window.innerWidth <= 680 ? 12 : 36,
  pendingSaveReplace: null,
  sort: 'recent',
  view: 'grid',
  mode: 'library',
  fileRoot: '',
  filePath: '',
  fileParent: '',
  fileEntries: [],
  fileView: 'list',
  albumRoot: '',
  albumItems: [],
  albumView: 'grid',
  editingTextPath: '',
  selectionMode: false,
  selectedIds: new Set(),
  uploadTasks: [],
  uploadActive: false,
  uploadCancelAll: false,
  uploadNextId: 1,
  importNameMapping: false,
};

const romExtensions = new Set(['gba', 'gb', 'gbc', 'nes', 'fds', 'sfc', 'smc', 'nds']);

const platforms = [
  ['GBA', 'GBA'],
  ['GBC', 'GBC'],
  ['GB', 'GB'],
  ['FC', 'FC'],
  ['SFC', 'SFC'],
  ['NDS', 'NDS'],
];

const coreOptionsByPlatform = {
  GBA: [['mgba', 'mGBA']],
  GBC: [['mgba', 'mGBA']],
  GB: [['mgba', 'mGBA']],
  FC: [
    ['nestopia', 'Nestopia'],
    ['fceumm', 'FCEUmm'],
  ],
  SFC: [
    ['snes9x2005', 'Snes9x 2005'],
    ['snes9x', 'Snes9x'],
  ],
  NDS: [['melonds', 'melonDS']],
};

const gameConfigFields = [
  { key: 'path', label: 'ROM 路径', type: 'text', readonly: true, group: '基础信息' },
  { key: 'platformName', label: '平台名称', type: 'text', readonly: true, group: '基础信息' },
  { key: 'platform', label: '平台 ID', type: 'number', readonly: true, group: '基础信息' },
  { key: 'core', label: '模拟核心', type: 'select', group: '基础信息', optionsForGame: coreOptionsForGame },
  { key: 'crc32', label: 'CRC32', type: 'number', readonly: true, group: '基础信息' },
  { key: 'lastPlayed', label: '最后游玩', type: 'text', group: '基础信息' },
  { key: 'playCount', label: '游玩次数', type: 'number', group: '基础信息' },
  { key: 'playTime', label: '游玩时长(秒)', type: 'number', group: '基础信息' },
  { key: 'favourite', label: '收藏', type: 'boolean', group: '基础信息' },

  { key: 'savePath', label: '存档目录', type: 'text', group: '路径配置' },
  { key: 'screenShotPath', label: '截图目录', type: 'text', group: '路径配置' },
  { key: 'logoPath', label: '封面路径', type: 'text', group: '路径配置' },
  { key: 'cheatPath', label: '金手指路径', type: 'text', group: '路径配置' },
  { key: 'overlayPath', label: '遮罩路径', type: 'text', group: '路径配置' },
  { key: 'shaderPath', label: '着色器路径', type: 'text', group: '路径配置' },

  { key: 'overlayEnabled', label: '启用遮罩', type: 'boolean', group: '画面配置' },
  { key: 'shaderEnabled', label: '启用着色器', type: 'boolean', group: '画面配置' },
  { key: 'displayMode', label: '画面模式', type: 'number', group: '画面配置' },
  { key: 'integerAspectRatio', label: '整数倍比例', type: 'number', step: '0.01', group: '画面配置' },
  { key: 'customScale', label: '自定义缩放', type: 'number', step: '0.01', group: '画面配置' },
  { key: 'customOffsetX', label: '自定义 X 偏移', type: 'number', step: '0.01', group: '画面配置' },
  { key: 'customOffsetY', label: '自定义 Y 偏移', type: 'number', step: '0.01', group: '画面配置' },

  { key: 'ndsScreenLayout', label: 'NDS 屏幕布局', type: 'select', group: 'NDS 配置', ndsOnly: true, options: [
    ['', '默认'],
    ['vertical', '竖向'],
    ['horizontal', '横向'],
    ['custom', '自定义'],
    ['hybrid', '混合'],
    ['top', '仅上屏'],
    ['bottom', '仅下屏'],
  ] },
  { key: 'ndsScreenOrientation', label: 'NDS 旋转角度', type: 'select', group: 'NDS 配置', ndsOnly: true, options: [
    ['', '默认'],
    ['0', '0 度'],
    ['90', '90 度'],
    ['180', '180 度'],
    ['270', '270 度'],
  ] },
  { key: 'ndsInternalResolution', label: 'NDS 内部分辨率', type: 'select', group: 'NDS 配置', ndsOnly: true, options: [
    [1, '1x'],
    [2, '2x'],
    [3, '3x'],
    [4, '4x'],
  ] },
  { key: 'ndsIntegerScale', label: 'NDS 画面整数缩放', type: 'boolean', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsTopScale', label: '上屏缩放', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsTopOffsetX', label: '上屏 X 偏移', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsTopOffsetY', label: '上屏 Y 偏移', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsBottomScale', label: '下屏缩放', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsBottomOffsetX', label: '下屏 X 偏移', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },
  { key: 'ndsBottomOffsetY', label: '下屏 Y 偏移', type: 'number', step: '0.01', group: 'NDS 配置', ndsOnly: true },

  { key: 'shaderParaNames', label: '着色器参数名(JSON)', type: 'json', group: '着色器参数' },
  { key: 'shaderParaValues', label: '着色器参数值(JSON)', type: 'json', group: '着色器参数' },
];

const $ = (id) => document.getElementById(id);

function toast(text) {
  const el = $('toast');
  el.textContent = text;
  el.classList.add('show');
  clearTimeout(toast.timer);
  toast.timer = setTimeout(() => el.classList.remove('show'), 2200);
}

async function api(path, options = {}) {
  const res = await fetch(path, {
    ...options,
    headers: {
      ...(options.body instanceof Blob ? {} : { 'Content-Type': 'application/json' }),
      ...(options.headers || {}),
    },
  });
  const type = res.headers.get('content-type') || '';
  const data = type.includes('application/json') ? await res.json() : await res.text();
  if (!res.ok || data.ok === false) throw new Error(data.error || data || '请求失败');
  return data;
}

function platformOf(game) {
  return game.platformName || ({ 1: 'GBA', 2: 'GBC', 3: 'GB', 4: 'FC', 5: 'SFC', 6: 'NDS' }[game.platform] || 'OTHER');
}

function coreOptionsForGame(game) {
  return coreOptionsByPlatform[platformOf(game)] || [];
}

function defaultCoreForGame(game) {
  return coreOptionsForGame(game)[0]?.[0] || '';
}

function platformClass(platform) {
  return `platform-${String(platform || 'OTHER').toLowerCase()}`;
}

function gameUrl(game) {
  return encodeURIComponent(game.id || game.path);
}

function gameKey(game) {
  return game.id || game.path;
}

function escapeHtml(text) {
  return String(text).replace(/[&<>"']/g, (m) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' }[m]));
}

function formatPlayTime(seconds) {
  if (!seconds || seconds < 60) return '不到 1 分钟';
  const h = Math.floor(seconds / 3600);
  const m = Math.floor((seconds % 3600) / 60);
  return h ? `${h} 小时 ${m} 分钟` : `${m} 分钟`;
}

function formatSize(bytes) {
  if (!bytes) return '0 B';
  const units = ['B', 'KB', 'MB', 'GB'];
  let value = Number(bytes);
  let index = 0;
  while (value >= 1024 && index < units.length - 1) {
    value /= 1024;
    index++;
  }
  return `${value.toFixed(index ? 1 : 0)} ${units[index]}`;
}

function formatSpeed(bytesPerSecond) {
  return `${formatSize(bytesPerSecond)}/s`;
}

function configValueToString(value, field) {
  if (field.type === 'json') return JSON.stringify(value ?? [], null, 2);
  if (value === undefined || value === null) return '';
  return String(value);
}

function makeConfigInput(field, game) {
  const value = game[field.key];
  if (field.type === 'boolean') {
    const label = document.createElement('label');
    label.className = 'config-check';
    label.innerHTML = `<input type="checkbox" data-config-key="${field.key}" data-config-type="${field.type}"${value ? ' checked' : ''}${field.readonly ? ' disabled' : ''}><span>${field.label}</span>`;
    return label;
  }

  if (field.type === 'select') {
    const select = document.createElement('select');
    select.dataset.configKey = field.key;
    select.dataset.configType = field.type;
    select.disabled = !!field.readonly;
    const options = field.optionsForGame ? field.optionsForGame(game) : (field.options || []);
    for (const [optionValue, label] of options) {
      const option = document.createElement('option');
      option.value = String(optionValue);
      option.textContent = label;
      select.appendChild(option);
    }
    select.value = field.key === 'core' && !value ? defaultCoreForGame(game) : configValueToString(value, field);
    return select;
  }

  const input = document.createElement(field.type === 'json' ? 'textarea' : 'input');
  input.dataset.configKey = field.key;
  input.dataset.configType = field.type;
  if (field.type !== 'json') input.type = field.type === 'number' ? 'number' : 'text';
  if (field.step) input.step = field.step;
  if (field.readonly) input.readOnly = true;
  input.value = configValueToString(value, field);
  return input;
}

function renderGameConfig(game) {
  const root = $('gameConfigFields');
  if (!root) return;
  root.innerHTML = '';
  if (!game) {
    root.innerHTML = '<div class="empty compact-empty">未选择游戏</div>';
    return;
  }

  const isNds = platformOf(game) === 'NDS';
  let currentGroup = '';
  for (const field of gameConfigFields) {
    if (field.ndsOnly && !isNds) continue;
    if (field.group !== currentGroup) {
      currentGroup = field.group;
      const head = document.createElement('h4');
      head.textContent = currentGroup;
      root.appendChild(head);
    }

    const row = document.createElement(field.type === 'boolean' ? 'div' : 'label');
    row.className = `config-field${field.readonly ? ' readonly' : ''}${field.type === 'boolean' ? ' boolean-field' : ''}`;
    if (field.type === 'boolean') {
      row.appendChild(makeConfigInput(field, game));
    } else {
      const caption = document.createElement('span');
      caption.textContent = field.label;
      row.appendChild(caption);
      row.appendChild(makeConfigInput(field, game));
    }
    root.appendChild(row);
  }
}

function readConfigInput(input) {
  const key = input.dataset.configKey;
  const type = input.dataset.configType;
  if (!key || input.disabled || input.readOnly) return undefined;
  if (type === 'boolean') return input.checked;
  if (type === 'number') {
    if (input.value.trim() === '') return 0;
    const value = Number(input.value);
    if (!Number.isFinite(value)) throw new Error(`${key} 必须是数字`);
    return value;
  }
  if (type === 'json') {
    try {
      return input.value.trim() ? JSON.parse(input.value) : [];
    } catch {
      throw new Error(`${key} 不是有效 JSON`);
    }
  }
  if (type === 'select') {
    const field = gameConfigFields.find((item) => item.key === key);
    const options = field?.optionsForGame ? field.optionsForGame(state.selected) : (field?.options || []);
    const numeric = options.some(([value]) => typeof value === 'number');
    return numeric ? Number(input.value) : input.value;
  }
  return input.value;
}

function collectGameConfig() {
  const payload = {};
  for (const input of $('gameConfigFields').querySelectorAll('[data-config-key]')) {
    const value = readConfigInput(input);
    if (value !== undefined) payload[input.dataset.configKey] = value;
  }
  if ($('titleInput')) payload.title = $('titleInput').value.trim();
  return payload;
}

function hasNonAsciiPath(file) {
  return /[^\x00-\x7F]/.test(fileRelativePath(file));
}

function fileRelativePath(file) {
  return file.uploadRelativePath || file.webkitRelativePath || file.name || 'upload.bin';
}

function romFilesFromList(files) {
  const seen = new Set();
  const roms = [];
  for (const file of files || []) {
    const name = file.name || '';
    const ext = name.includes('.') ? name.split('.').pop().toLowerCase() : '';
    if (!romExtensions.has(ext)) continue;
    const key = `${file.webkitRelativePath || name}:${file.size}:${file.lastModified}`;
    if (seen.has(key)) continue;
    seen.add(key);
    roms.push(file);
  }
  return roms.sort((a, b) => (a.webkitRelativePath || a.name).localeCompare(b.webkitRelativePath || b.name, 'zh-Hans'));
}

async function loadGames(keepPage = false) {
  const data = await api('/api/games');
  state.games = data.games || [];
  pruneSelection();
  if (!keepPage) state.page = 1;
  renderTabs();
  renderGames();
}

function filteredGames() {
  const q = state.search.trim().toLowerCase();
  const list = state.games.filter((game) => {
    const p = platformOf(game);
    const platformMatch = p === state.filter;
    const searchMatch = !q || [game.title, game.path, p].join(' ').toLowerCase().includes(q);
    return platformMatch && searchMatch;
  });
  return sortGames(list);
}

function sortGames(list) {
  return [...list].sort((a, b) => {
    if (state.sort === 'playCount')
      return (b.playCount || 0) - (a.playCount || 0);
    if (state.sort === 'title')
      return String(a.title || '').localeCompare(String(b.title || ''), 'zh-Hans');
    return String(b.lastPlayed || '').localeCompare(String(a.lastPlayed || ''));
  });
}

function pageCount(list = filteredGames()) {
  return Math.max(1, Math.ceil(list.length / state.pageSize));
}

function currentPageItems() {
  const list = filteredGames();
  clampPage();
  const start = (state.page - 1) * state.pageSize;
  return list.slice(start, start + state.pageSize);
}

function clampPage() {
  state.page = Math.min(Math.max(1, state.page), pageCount());
}

function updateSelectionTools() {
  const count = state.selectedIds.size;
  const pageItems = currentPageItems();
  const pageKeys = pageItems.map((game) => gameKey(game));
  const allPageSelected = pageKeys.length > 0 && pageKeys.every((key) => state.selectedIds.has(key));
  $('multiSelectBtn').hidden = state.selectionMode;
  $('selectPageBtn').hidden = !state.selectionMode;
  $('deleteSelectedBtn').hidden = !state.selectionMode;
  $('cancelSelectionBtn').hidden = !state.selectionMode;
  $('selectPageBtn').disabled = pageKeys.length === 0;
  $('selectPageBtn').querySelector('span').textContent = allPageSelected ? '取消本页' : '全选本页';
  $('deleteSelectedBtn').disabled = count === 0;
  $('deleteSelectedBtn').querySelector('span').textContent = count ? `删除选中 (${count})` : '删除选中';
}

function pruneSelection() {
  const valid = new Set(state.games.map((game) => gameKey(game)));
  for (const id of [...state.selectedIds]) {
    if (!valid.has(id)) state.selectedIds.delete(id);
  }
}

function setSelectionMode(enabled) {
  state.selectionMode = enabled;
  if (!enabled) state.selectedIds.clear();
  updateSelectionTools();
  renderGames();
}

function toggleGameSelection(game) {
  const key = gameKey(game);
  if (state.selectedIds.has(key)) {
    state.selectedIds.delete(key);
  } else {
    state.selectedIds.add(key);
  }
  updateSelectionTools();
  renderGames();
}

function toggleCurrentPageSelection() {
  const pageItems = currentPageItems();
  const pageKeys = pageItems.map((game) => gameKey(game));
  const allPageSelected = pageKeys.length > 0 && pageKeys.every((key) => state.selectedIds.has(key));
  for (const key of pageKeys) {
    if (allPageSelected) {
      state.selectedIds.delete(key);
    } else {
      state.selectedIds.add(key);
    }
  }
  updateSelectionTools();
  renderGames();
}

function renderTabs() {
  const root = $('platformTabs');
  root.innerHTML = '';
  for (const [key, label] of platforms) {
    const count = state.games.filter((g) => platformOf(g) === key).length;
    const btn = document.createElement('button');
    btn.className = `${key === state.filter ? 'active ' : ''}${platformClass(key)}`;
    btn.innerHTML = `<span><i class="fa-solid fa-layer-group"></i> ${label}</span><b>${count}</b>`;
    btn.onclick = () => {
      state.filter = key;
      state.page = 1;
      renderTabs();
      renderGames();
    };
    root.appendChild(btn);
  }
}

function renderGames() {
  if (state.mode !== 'library') return;
  const list = filteredGames();
  clampPage();
  const totalPages = pageCount(list);
  const start = (state.page - 1) * state.pageSize;
  const pageItems = list.slice(start, start + state.pageSize);
  const selectionText = state.selectionMode ? `，已选择 ${state.selectedIds.size} 个` : '';
  $('summaryText').textContent = `共 ${state.games.length} 个游戏，当前筛选 ${list.length} 个，每页 ${state.pageSize} 个${selectionText}`;
  $('pageText').textContent = `${state.page} / ${totalPages}`;
  $('prevPageBtn').disabled = state.page <= 1;
  $('nextPageBtn').disabled = state.page >= totalPages;
  $('pager').hidden = list.length <= state.pageSize;
  updateSelectionTools();

  const root = $('gameSections');
  root.innerHTML = '';
  if (!pageItems.length) {
    root.innerHTML = '<div class="empty">没有匹配的游戏</div>';
    return;
  }

  const groups = new Map();
  for (const game of pageItems) {
    const p = platformOf(game);
    if (!groups.has(p)) groups.set(p, []);
    groups.get(p).push(game);
  }

  for (const [platform, games] of groups) {
    const section = document.createElement('section');
    section.innerHTML = `<div class="section-head"><h2><span class="platform-dot ${platformClass(platform)}"></span>${platform}</h2><span>${games.length} 个游戏</span></div>`;
    const grid = document.createElement('div');
    grid.className = state.view === 'list' ? 'game-list' : 'game-grid';
    for (const game of games) {
      const card = document.createElement('button');
      const selected = state.selectedIds.has(gameKey(game));
      card.className = `${state.view === 'list' ? 'game-card list-card' : 'game-card'}${state.selectionMode ? ' selectable' : ''}${selected ? ' selected' : ''}`;
      card.setAttribute('aria-pressed', state.selectionMode ? String(selected) : 'false');
      card.innerHTML = `
        ${state.selectionMode ? `<span class="select-indicator"><i class="fa-solid fa-check"></i></span>` : ''}
        <img loading="lazy" alt="${escapeHtml(game.title || 'cover')}" src="/api/game/${gameUrl(game)}/cover">
        <div>
          <strong>${escapeHtml(game.title || '未命名游戏')}</strong>
          <span class="badge ${platformClass(platform)}">${platform}</span>
          ${state.view === 'list' ? `<small>${formatPlayTime(game.playTime)} · ${game.playCount || 0} 次 · ${escapeHtml(game.lastPlayed || '从未游玩')}</small>` : ''}
        </div>
      `;
      card.onclick = () => state.selectionMode ? toggleGameSelection(game) : openGameDialog(game);
      grid.appendChild(card);
    }
    section.appendChild(grid);
    root.appendChild(section);
  }
}

function setMode(mode) {
  state.mode = mode;
  const library = mode === 'library';
  const files = mode === 'files';
  const album = mode === 'album';
  $('libraryModeBtn').classList.toggle('active', library);
  $('filesModeBtn').classList.toggle('active', files);
  $('albumModeBtn').classList.toggle('active', album);
  $('platformTabs').hidden = !library;
  $('gameSections').hidden = !library;
  $('pager').hidden = !library || filteredGames().length <= state.pageSize;
  $('filePanel').hidden = !files;
  $('albumPanel').hidden = !album;
  $('searchInput').disabled = !library;
  $('sortSelect').disabled = !library;
  $('gridViewBtn').disabled = false;
  $('listViewBtn').disabled = false;
  $('multiSelectBtn').disabled = !library;
  if (library) {
    setView(state.view);
    renderGames();
  } else if (files) {
    setSelectionMode(false);
    setView(state.fileView);
    $('summaryText').textContent = '正在浏览 Switch 内存卡';
    if (!state.filePath) {
      loadFiles().catch((err) => toast(err.message));
    } else {
      renderFiles();
    }
  } else {
    setSelectionMode(false);
    setView(state.albumView);
    $('summaryText').textContent = '正在读取 Switch 相册';
    if (!state.albumItems.length) {
      loadAlbum().catch((err) => toast(err.message));
    } else {
      renderAlbum();
    }
  }
}

async function loadFiles(path = state.filePath) {
  const data = await api(`/api/files/list?path=${encodeURIComponent(path || '')}`);
  state.fileRoot = data.root || '';
  state.filePath = data.path || state.fileRoot;
  state.fileParent = data.parent || '';
  state.fileEntries = data.entries || [];
  renderFiles();
}

function fileIcon(entry) {
  if (entry.isDir) return 'fa-folder';
  const ext = String(entry.ext || '').toLowerCase();
  if (['png', 'jpg', 'jpeg', 'webp', 'bmp'].includes(ext)) return 'fa-image';
  if (isVideoEntry(entry)) return 'fa-video';
  if (['zip', '7z', 'rar'].includes(ext)) return 'fa-file-zipper';
  return 'fa-file';
}

function isImageEntry(entry) {
  return !entry.isDir && ['png', 'jpg', 'jpeg', 'webp', 'bmp', 'gif'].includes(String(entry.ext || '').toLowerCase());
}

function isVideoEntry(entry) {
  return !entry.isDir && ['mp4', 'mov', 'm4v', 'webm', 'mkv', 'avi'].includes(String(entry.ext || '').toLowerCase());
}

function isTextEntry(entry) {
  return !entry.isDir && ['txt', 'log', 'cfg', 'ini', 'json', 'xml', 'md', 'cht', 'yaml', 'yml', 'csv'].includes(String(entry.ext || '').toLowerCase());
}

function renderFiles() {
  $('filePathInput').value = state.filePath || state.fileRoot || '';
  $('fileUpBtn').disabled = !state.fileParent;
  $('fileSummary').textContent = `${state.fileEntries.length} 个项目`;
  $('summaryText').textContent = `文件管理：${state.filePath || state.fileRoot || ''}`;

  const root = $('fileList');
  root.className = state.fileView === 'grid' ? 'file-grid' : 'file-list';
  root.innerHTML = '';
  if (!state.fileEntries.length) {
    root.innerHTML = '<div class="empty compact-empty">当前目录为空</div>';
    return;
  }

  for (const entry of state.fileEntries) {
    const row = document.createElement('div');
    row.className = `${state.fileView === 'grid' ? 'file-card' : 'file-row'}${entry.isDir ? ' dir' : ''}`;
    const preview = isImageEntry(entry)
      ? `<img class="file-thumb" loading="lazy" alt="${escapeHtml(entry.name)}" src="/api/files/view?path=${encodeURIComponent(entry.path)}">`
      : isVideoEntry(entry)
        ? `<video class="file-thumb" preload="metadata" muted playsinline src="/api/files/view?path=${encodeURIComponent(entry.path)}"></video>`
        : `<span class="file-thumb file-thumb-icon"><i class="fa-solid ${fileIcon(entry)}"></i></span>`;
    row.innerHTML = state.fileView === 'grid' ? `
      <button class="file-main" title="${escapeHtml(entry.path)}">
        ${preview}
        <span>${escapeHtml(entry.name)}</span>
      </button>
      <div class="file-meta-line">
        <span>${entry.isDir ? '文件夹' : formatSize(entry.size)}</span>
        <span>${escapeHtml(entry.modified || '')}</span>
      </div>
      <div class="file-row-actions">
        <button title="${entry.isDir ? '下载为 ZIP' : '下载'}"><i class="fa-solid fa-download"></i></button>
        <button title="改名"><i class="fa-solid fa-pen"></i></button>
        <button title="移动"><i class="fa-solid fa-arrows-up-down-left-right"></i></button>
        <button class="danger" title="删除"><i class="fa-solid fa-trash-can"></i></button>
      </div>
    ` : `
      <button class="file-main" title="${escapeHtml(entry.path)}">
        <i class="fa-solid ${fileIcon(entry)}"></i>
        <span>${escapeHtml(entry.name)}</span>
      </button>
      <span class="file-meta">${entry.isDir ? '文件夹' : formatSize(entry.size)}</span>
      <span class="file-meta">${escapeHtml(entry.modified || '')}</span>
      <div class="file-row-actions">
        <button title="${entry.isDir ? '下载为 ZIP' : '下载'}"><i class="fa-solid fa-download"></i></button>
        <button title="改名"><i class="fa-solid fa-pen"></i></button>
        <button title="移动"><i class="fa-solid fa-arrows-up-down-left-right"></i></button>
        <button class="danger" title="删除"><i class="fa-solid fa-trash-can"></i></button>
      </div>
    `;
    row.querySelector('.file-main').onclick = () => {
      openFileEntry(entry).catch((err) => toast(err.message));
    };
    const [downloadBtn, renameBtn, moveBtn, deleteBtn] = row.querySelectorAll('.file-row-actions button');
    downloadBtn.onclick = () => downloadFile(entry);
    renameBtn.onclick = () => renameFile(entry).catch((err) => toast(err.message));
    moveBtn.onclick = () => moveFile(entry).catch((err) => toast(err.message));
    deleteBtn.onclick = () => deleteFileEntry(entry).catch((err) => toast(err.message));
    root.appendChild(row);
  }
}

async function openFileEntry(entry) {
  if (entry.isDir) {
    await loadFiles(entry.path);
  } else if (isImageEntry(entry)) {
    openFileImage(entry);
  } else if (isVideoEntry(entry)) {
    openVideo(entry.name, `/api/files/view?path=${encodeURIComponent(entry.path)}&t=${Date.now()}`);
  } else if (isTextEntry(entry)) {
    await openTextEditor(entry);
  }
}

function downloadFile(entry) {
  window.location.href = `/api/files/download?path=${encodeURIComponent(entry.path)}`;
}

function openFileImage(entry) {
  $('fileImageTitle').textContent = entry.name || '图片预览';
  $('fileImagePreview').src = `/api/files/view?path=${encodeURIComponent(entry.path)}&t=${Date.now()}`;
  $('fileImageDialog').showModal();
}

function openVideo(title, url) {
  $('videoTitle').textContent = title || '视频预览';
  const video = $('videoPreview');
  video.src = url;
  video.load();
  $('videoDialog').showModal();
}

async function loadAlbum() {
  const data = await api('/api/album/list');
  state.albumRoot = data.root || '';
  state.albumItems = data.items || [];
  renderAlbum();
}

function renderAlbum() {
  $('albumSummary').textContent = `${state.albumItems.length} 个媒体文件 · ${state.albumRoot || 'sdmc:/emuMMC/SD00/Nintendo/Album'}`;
  $('summaryText').textContent = `相册浏览：${state.albumRoot || 'Album'}`;
  const root = $('albumList');
  root.className = state.albumView === 'grid' ? 'album-grid' : 'album-list';
  root.innerHTML = '';
  if (!state.albumItems.length) {
    root.innerHTML = '<div class="empty compact-empty">没有找到图片或视频</div>';
    return;
  }

  for (const item of state.albumItems) {
    const isVideo = item.type === 'video';
    const card = document.createElement('button');
    card.className = state.albumView === 'grid' ? 'album-card' : 'album-row';
    const media = isVideo
      ? `<video preload="metadata" muted playsinline src="${item.url}"></video>`
      : `<img loading="lazy" alt="${escapeHtml(item.name)}" src="${item.url}">`;
    card.innerHTML = `
      ${media}
      <span class="album-kind"><i class="fa-solid ${isVideo ? 'fa-video' : 'fa-image'}"></i>${isVideo ? '视频' : '图片'}</span>
      <strong>${escapeHtml(item.name)}</strong>
      <small>${formatSize(item.size)} · ${escapeHtml(item.modified || '')}</small>
    `;
    card.onclick = () => {
      if (isVideo) openVideo(item.name, `${item.url}&t=${Date.now()}`);
      else {
        $('fileImageTitle').textContent = item.name || '图片预览';
        $('fileImagePreview').src = `${item.url}&t=${Date.now()}`;
        $('fileImageDialog').showModal();
      }
    };
    root.appendChild(card);
  }
}

async function openTextEditor(entry) {
  const data = await api(`/api/files/text?path=${encodeURIComponent(entry.path)}`);
  state.editingTextPath = data.path || entry.path;
  $('textEditorTitle').textContent = data.name || entry.name || '文本编辑';
  $('textEditorPath').textContent = state.editingTextPath;
  $('textEditorContent').value = data.content || '';
  $('textEditorDialog').showModal();
}

async function saveTextEditor() {
  if (!state.editingTextPath) return;
  await api('/api/files/text', {
    method: 'PUT',
    body: JSON.stringify({ path: state.editingTextPath, content: $('textEditorContent').value }),
  });
  toast('文本已保存');
  $('textEditorDialog').close();
  await loadFiles();
}

async function renameFile(entry) {
  const name = prompt('输入新的名称', entry.name);
  if (!name || name === entry.name) return;
  await api('/api/files/rename', { method: 'POST', body: JSON.stringify({ path: entry.path, name }) });
  toast('已改名');
  await loadFiles();
}

async function moveFile(entry) {
  const destDir = prompt('移动到目录', state.filePath || state.fileRoot);
  if (!destDir) return;
  await api('/api/files/move', { method: 'POST', body: JSON.stringify({ path: entry.path, destDir }) });
  toast('已移动');
  await loadFiles();
}

async function deleteFileEntry(entry) {
  if (!confirm(`确认删除 ${entry.name}？${entry.isDir ? '\n文件夹内所有内容都会被删除。' : ''}`)) return;
  await api('/api/files/delete', { method: 'DELETE', body: JSON.stringify({ path: entry.path }) });
  toast('已删除');
  await loadFiles();
}

async function createFolder() {
  const name = prompt('新建文件夹名称');
  if (!name) return;
  await api('/api/files/mkdir', { method: 'POST', body: JSON.stringify({ path: state.filePath || state.fileRoot, name }) });
  toast('文件夹已创建');
  await loadFiles();
}

async function openGameDialog(game) {
  state.selected = game;
  $('detailPlatform').textContent = platformOf(game);
  $('detailPlatform').className = `badge ${platformClass(platformOf(game))}`;
  $('detailTitle').textContent = game.title || '游戏详情';
  $('detailCover').src = `/api/game/${gameUrl(game)}/cover?t=${Date.now()}`;
  $('titleInput').value = game.title || '';
  $('playTimeText').textContent = formatPlayTime(game.playTime);
  $('playCountText').textContent = `${game.playCount || 0} 次`;
  renderGameConfig(game);
  $('gameDialog').showModal();
  await loadSaves();
}

function closeGameDialog() {
  $('gameDialog').close();
}

async function loadSaves() {
  const game = state.selected;
  if (!game) return;
  const data = await api(`/api/game/${gameUrl(game)}/saves`);
  renderSaveThumbs(data.states || []);
  renderSaveList('stateSaveList', data.states || [], 'state');
  renderSaveList('batterySaveList', data.battery || [], 'battery');
}

function renderSaveThumbs(states) {
  const root = $('saveThumbs');
  const withThumbs = states.filter((item) => item.thumbUrl);
  if (!withThumbs.length) {
    root.innerHTML = '<div class="empty compact-empty">暂无存档截图</div>';
    return;
  }
  root.innerHTML = '';
  for (const item of withThumbs) {
    const figure = document.createElement('figure');
    figure.innerHTML = `
      <button class="save-thumb-preview" title="查看截图">
        <img alt="槽位 ${item.slot} 截图" src="${item.thumbUrl}">
      </button>
      <figcaption><span>槽位 ${item.slot}</span><button class="danger" title="删除截图"><i class="fa-solid fa-trash-can"></i></button></figcaption>
    `;
    figure.querySelector('.save-thumb-preview').onclick = () => openSaveThumb(item);
    figure.querySelector('figcaption button').onclick = () => deleteSave(item.thumbPath);
    root.appendChild(figure);
  }
}

function openSaveThumb(item) {
  $('fileImageTitle').textContent = `槽位 ${item.slot} 截图`;
  const sep = String(item.thumbUrl || '').includes('?') ? '&' : '?';
  $('fileImagePreview').src = `${item.thumbUrl}${sep}t=${Date.now()}`;
  $('fileImageDialog').showModal();
}

function renderSaveList(rootId, saves, type) {
  const root = $(rootId);
  root.innerHTML = '';
  if (!saves.length) {
    root.innerHTML = `<div class="empty compact-empty">${type === 'state' ? '暂无即时存档' : '暂无电池存档'}</div>`;
    return;
  }

  for (const item of saves) {
    const row = document.createElement('div');
    row.className = 'save-row';
    const title = type === 'state' ? `槽位 ${item.slot}` : '电池存档';
    row.innerHTML = `
      <div>
        <strong>${title}</strong>
        <span>${escapeHtml(item.name)} · ${formatSize(item.size)} · ${escapeHtml(item.modified || '')}</span>
      </div>
      <div class="save-row-actions">
        <button title="导出"><i class="fa-solid fa-download"></i></button>
        <button title="本地选择替换"><i class="fa-solid fa-file-import"></i></button>
        <button class="danger" title="删除"><i class="fa-solid fa-trash-can"></i></button>
      </div>
    `;
    const [exportBtn, replaceBtn, deleteBtn] = row.querySelectorAll('button');
    exportBtn.onclick = () => exportSave(item.path);
    replaceBtn.onclick = () => chooseSaveReplacement({ type, slot: item.slot || 0 });
    deleteBtn.onclick = () => deleteSave(item.path);
    root.appendChild(row);
  }
}

function exportSave(path) {
  if (!state.selected || !path) return;
  window.location.href = `/api/game/${gameUrl(state.selected)}/save/export?path=${encodeURIComponent(path)}`;
}

function chooseSaveReplacement(target) {
  state.pendingSaveReplace = target;
  $('saveInput').value = '';
  $('saveInput').click();
}

async function deleteSave(path) {
  if (!state.selected || !confirm('确认删除该文件？')) return;
  await api(`/api/game/${gameUrl(state.selected)}/save/delete`, {
    method: 'DELETE',
    body: JSON.stringify({ path }),
  });
  toast('存档已删除');
  await loadSaves();
}

async function uploadFile(file, startUrl, finishKind = 'rom', extraStartData = {}, callbacks = {}) {
  const start = await api(startUrl, {
    method: 'POST',
    body: JSON.stringify({ name: file.name, size: file.size, kind: finishKind, ...extraStartData }),
  });
  callbacks.onStart?.(start);
  const chunkSize = 2 * 1024 * 1024;
  let offset = 0;
  try {
    while (offset < file.size) {
      if (callbacks.isCancelled?.()) throw new DOMException('upload cancelled', 'AbortError');
      const chunk = file.slice(offset, offset + chunkSize);
      await api(`/api/upload/chunk?token=${encodeURIComponent(start.token)}&offset=${offset}`, {
        method: 'POST',
        body: chunk,
        headers: { 'Content-Type': 'application/octet-stream' },
        signal: callbacks.signal,
      });
      offset += chunk.size;
      callbacks.onProgress?.(offset);
      setProgress(`${file.name}`, file.size ? offset / file.size : 1);
    }
    if (callbacks.isCancelled?.()) throw new DOMException('upload cancelled', 'AbortError');
    return await api('/api/upload/finish', {
      method: 'POST',
      body: JSON.stringify({ token: start.token }),
      signal: callbacks.signal,
    });
  } catch (err) {
    if (start.token) {
      try {
        await api('/api/upload/cancel', {
          method: 'POST',
          body: JSON.stringify({ token: start.token }),
        });
      } catch (_) {
      }
    }
    throw err;
  }
}

async function uploadRoms(files) {
  const roms = romFilesFromList(files);
  if (!roms.length) {
    toast('没有找到支持的 ROM 文件');
    return;
  }
  const tasks = roms.map((file) => ({
    id: state.uploadNextId++,
    file,
    name: file.name,
    relativePath: file.webkitRelativePath || file.name,
    size: file.size,
    uploaded: 0,
    speed: 0,
    status: 'pending',
    kind: 'rom',
    startUrl: '/api/upload/start',
    extraStartData: { importNameMapping: state.importNameMapping },
    error: '',
    token: '',
    controller: null,
    startedAt: 0,
    lastBytes: 0,
    lastTime: 0,
  }));
  state.uploadTasks = tasks;
  state.uploadCancelAll = false;
  renderUploadDialog();
  $('uploadDialog').showModal();
  runUploadQueue().catch((err) => toast(err.message));
}

async function uploadBrowserFiles(files) {
  const list = [...(files || [])].filter((file) => file && file.name);
  if (!list.length) {
    toast('没有可上传的文件');
    return;
  }
  if (list.some(hasNonAsciiPath)) {
    const ok = confirm('文件名或目录名中包含中文字符，上传到 Switch 前会自动重命名为拼音，是否继续？');
    if (!ok) return;
  }
  const targetPath = state.filePath || state.fileRoot || '';
  const tasks = list
    .sort((a, b) => fileRelativePath(a).localeCompare(fileRelativePath(b), 'zh-Hans'))
    .map((file) => ({
      id: state.uploadNextId++,
      file,
      name: file.name,
      relativePath: fileRelativePath(file),
      size: file.size,
      uploaded: 0,
      speed: 0,
      status: 'pending',
      kind: 'file',
      startUrl: '/api/files/upload/start',
      extraStartData: { path: targetPath, relativePath: fileRelativePath(file) },
      error: '',
      token: '',
      controller: null,
      startedAt: 0,
      lastBytes: 0,
      lastTime: 0,
    }));
  state.uploadTasks = tasks;
  state.uploadCancelAll = false;
  renderUploadDialog();
  $('uploadDialog').showModal();
  runUploadQueue().catch((err) => toast(err.message));
}

async function runUploadQueue() {
  if (state.uploadActive) return;
  state.uploadActive = true;
  renderUploadDialog();
  try {
    for (const task of state.uploadTasks) {
      if (state.uploadCancelAll) {
        if (task.status === 'pending') task.status = 'cancelled';
        continue;
      }
      if (task.status !== 'pending') continue;
      await uploadQueueTask(task);
    }
  } finally {
    state.uploadActive = false;
    renderUploadDialog();
    const hasRom = state.uploadTasks.some((task) => (task.kind || 'rom') === 'rom');
    const hasFile = state.uploadTasks.some((task) => task.kind === 'file');
    if (hasRom) await loadGames(true);
    if (hasFile && state.mode === 'files') await loadFiles();
    const failed = state.uploadTasks.filter((task) => task.status === 'failed').length;
    const cancelled = state.uploadTasks.filter((task) => task.status === 'cancelled').length;
    const done = state.uploadTasks.filter((task) => task.status === 'done').length;
    const action = hasFile && !hasRom ? '上传' : '导入';
    if (done && !failed && !cancelled) toast(hasFile && !hasRom ? '上传完成' : '导入完成，GameDB 已保存');
    else if (done) toast(`已${action} ${done} 个，${failed + cancelled} 个未完成`);
  }
}

async function uploadQueueTask(task) {
  task.status = 'uploading';
  task.controller = new AbortController();
  task.startedAt = performance.now();
  task.lastTime = task.startedAt;
  task.lastBytes = 0;
  renderUploadDialog();
  try {
    await uploadFile(task.file, task.startUrl || '/api/upload/start', task.kind || 'rom', task.extraStartData || {}, {
      signal: task.controller.signal,
      isCancelled: () => task.status === 'cancelled' || state.uploadCancelAll,
      onStart: (session) => {
        task.token = session.token || '';
        renderUploadDialog();
      },
      onProgress: (uploaded) => {
        const now = performance.now();
        const elapsed = Math.max(1, now - task.lastTime) / 1000;
        task.speed = (uploaded - task.lastBytes) / elapsed;
        task.uploaded = uploaded;
        task.lastBytes = uploaded;
        task.lastTime = now;
        renderUploadDialog();
      },
    });
    task.uploaded = task.size;
    task.speed = 0;
    task.status = 'done';
  } catch (err) {
    task.speed = 0;
    if (task.status === 'cancelled' || err.name === 'AbortError') {
      task.status = 'cancelled';
      task.error = '已取消';
    } else {
      task.status = 'failed';
      task.error = err.message || '上传失败';
    }
  } finally {
    task.controller = null;
    renderUploadDialog();
  }
}

function cancelUploadTask(id) {
  const task = state.uploadTasks.find((item) => item.id === id);
  if (!task || task.status === 'done' || task.status === 'failed' || task.status === 'cancelled') return;
  task.status = 'cancelled';
  task.error = '已取消';
  if (task.controller) task.controller.abort();
  if (task.token) {
    api('/api/upload/cancel', { method: 'POST', body: JSON.stringify({ token: task.token }) }).catch(() => {});
  }
  renderUploadDialog();
}

function cancelAllUploads() {
  state.uploadCancelAll = true;
  for (const task of state.uploadTasks) {
    if (task.status === 'pending') {
      task.status = 'cancelled';
      task.error = '已取消';
    } else if (task.status === 'uploading') {
      cancelUploadTask(task.id);
    }
  }
  renderUploadDialog();
}

function uploadStatusText(status) {
  return {
    pending: '等待',
    uploading: '上传中',
    done: '完成',
    failed: '失败',
    cancelled: '已取消',
  }[status] || status;
}

function renderUploadDialog() {
  const totalBytes = state.uploadTasks.reduce((sum, task) => sum + task.size, 0);
  const uploadedBytes = state.uploadTasks.reduce((sum, task) => sum + Math.min(task.uploaded, task.size), 0);
  const totalSpeed = state.uploadTasks.reduce((sum, task) => sum + (task.status === 'uploading' ? task.speed : 0), 0);
  const doneCount = state.uploadTasks.filter((task) => task.status === 'done').length;
  const activeCount = state.uploadTasks.filter((task) => task.status === 'uploading').length;
  const pendingCount = state.uploadTasks.filter((task) => task.status === 'pending').length;
  const ratio = totalBytes ? uploadedBytes / totalBytes : 0;

  $('uploadSummaryText').textContent = `${state.uploadTasks.length} 个文件，${activeCount} 个上传中，${pendingCount} 个等待`;
  $('uploadTotalPercent').textContent = `${Math.round(ratio * 100)}%`;
  $('uploadTotalSpeed').textContent = formatSpeed(totalSpeed);
  $('uploadDoneCount').textContent = `${doneCount} / ${state.uploadTasks.length}`;
  $('uploadTotalBar').style.width = `${Math.max(0, Math.min(1, ratio)) * 100}%`;
  $('cancelAllUploadsBtn').disabled = !state.uploadTasks.some((task) => task.status === 'pending' || task.status === 'uploading');
  $('closeUploadDialogBtn').disabled = state.uploadActive;

  const root = $('uploadQueueList');
  root.innerHTML = '';
  if (!state.uploadTasks.length) {
    root.innerHTML = '<div class="empty compact-empty">暂无上传任务</div>';
    return;
  }

  for (const task of state.uploadTasks) {
    const ratio = task.size ? task.uploaded / task.size : (task.status === 'done' ? 1 : 0);
    const canCancel = task.status === 'pending' || task.status === 'uploading';
    const row = document.createElement('div');
    row.className = 'upload-row';
    row.innerHTML = `
      <div class="upload-row-main">
        <div class="upload-row-title">
          <span class="upload-status ${task.status}">${uploadStatusText(task.status)}</span>
          <strong title="${escapeHtml(task.relativePath)}">${escapeHtml(task.name)}</strong>
        </div>
        <div class="progress-track"><span style="width:${Math.max(0, Math.min(1, ratio)) * 100}%"></span></div>
        <div class="upload-row-meta">
          <span>${formatSize(task.uploaded)} / ${formatSize(task.size)}</span>
          <span>${task.status === 'uploading' ? formatSpeed(task.speed) : escapeHtml(task.error || task.relativePath)}</span>
        </div>
      </div>
      <div class="upload-row-actions">
        <button class="danger" ${canCancel ? '' : 'disabled'} title="取消上传"><i class="fa-solid fa-ban"></i><span>取消</span></button>
      </div>
    `;
    row.querySelector('button').onclick = () => cancelUploadTask(task.id);
    root.appendChild(row);
  }
}

function readEntryFiles(entry) {
  return new Promise((resolve) => {
    if (!entry) return resolve([]);
    if (entry.isFile) {
      entry.file((file) => {
        const relativePath = String(entry.fullPath || file.name).replace(/^\/+/, '');
        try {
          Object.defineProperty(file, 'uploadRelativePath', { value: relativePath });
        } catch (_) {
        }
        resolve([file]);
      }, () => resolve([]));
      return;
    }
    if (!entry.isDirectory) return resolve([]);

    const reader = entry.createReader();
    const entries = [];
    const readBatch = () => {
      reader.readEntries(async (batch) => {
        if (!batch.length) {
          const nested = await Promise.all(entries.map(readEntryFiles));
          resolve(nested.flat());
          return;
        }
        entries.push(...batch);
        readBatch();
      }, () => resolve([]));
    };
    readBatch();
  });
}

async function filesFromDropEvent(event) {
  const items = [...(event.dataTransfer.items || [])];
  const entries = items.map((item) => item.webkitGetAsEntry?.()).filter(Boolean);
  if (entries.length) {
    const nested = await Promise.all(entries.map(readEntryFiles));
    return nested.flat();
  }
  return [...(event.dataTransfer.files || [])];
}

function showProgress(title) {
  $('progressPanel').hidden = false;
  $('progressTitle').textContent = title;
  setProgress('准备上传', 0);
}

function setProgress(text, ratio) {
  $('progressText').textContent = text;
  $('progressBar').style.width = `${Math.max(0, Math.min(1, ratio)) * 100}%`;
}

function hideProgress() {
  $('progressPanel').hidden = true;
}

async function saveTitle() {
  const game = state.selected;
  if (!game) return;
  const title = $('titleInput').value.trim();
  await api(`/api/game/${gameUrl(game)}`, { method: 'PUT', body: JSON.stringify({ title }) });
  toast('名称已保存');
  await loadGames(true);
  const updated = state.games.find((g) => (g.id || g.path) === (game.id || game.path));
  if (updated) {
    state.selected = updated;
    $('detailTitle').textContent = updated.title || '游戏详情';
    $('titleInput').value = updated.title || '';
    renderGameConfig(updated);
  }
}

async function saveGameConfig() {
  const game = state.selected;
  if (!game) return;
  const payload = collectGameConfig();
  await api(`/api/game/${gameUrl(game)}`, { method: 'PUT', body: JSON.stringify(payload) });
  toast('配置已保存');
  await reloadSelectedGame();
}

async function removeGame() {
  const game = state.selected;
  if (!game) return;
  await api(`/api/game/${gameUrl(game)}`, { method: 'DELETE', body: JSON.stringify({ deleteFile: true }) });
  toast('游戏已移除，GameDB 已保存');
  closeGameDialog();
  await loadGames(true);
}

async function deleteSelectedGames() {
  const selectedGames = state.games.filter((game) => state.selectedIds.has(gameKey(game)));
  if (!selectedGames.length) return;
  if (!confirm(`确认从游戏库移除 ${selectedGames.length} 个游戏？`)) return;

  showProgress('批量删除游戏');
  let done = 0;
  try {
    for (const game of selectedGames) {
      const title = game.title || game.path || '未命名游戏';
      $('progressTitle').textContent = `删除 ${done + 1} / ${selectedGames.length}`;
      setProgress(title, done / selectedGames.length);
      await api(`/api/game/${gameUrl(game)}`, { method: 'DELETE', body: JSON.stringify({ deleteFile: true }) });
      state.selectedIds.delete(gameKey(game));
      done++;
      setProgress(title, done / selectedGames.length);
    }
    state.selectionMode = false;
    state.selectedIds.clear();
    toast('选中游戏已移除，GameDB 已保存');
  } finally {
    hideProgress();
    await loadGames(true);
    updateSelectionTools();
  }
}

async function replaceSave(file) {
  const game = state.selected;
  const target = state.pendingSaveReplace || { type: 'battery', slot: 0 };
  if (!game || !file) return;
  showProgress(target.type === 'state' ? `替换槽位 ${target.slot}` : '替换 SAV');
  try {
    await uploadFile(file, `/api/game/${gameUrl(game)}/save/start`, 'save', target);
    toast('存档已替换，GameDB 已保存');
    await loadSaves();
  } catch (err) {
    toast(err.message);
  } finally {
    hideProgress();
    state.pendingSaveReplace = null;
  }
}

async function openImageLibrary() {
  const game = state.selected;
  if (!game) return;
  const data = await api(`/api/images?gameId=${gameUrl(game)}`);
  const grid = $('imageGrid');
  grid.innerHTML = '';
  for (const img of data.images || []) {
    const btn = document.createElement('button');
    btn.innerHTML = `<img alt="${escapeHtml(img.name)}" src="${img.url}"><span>${escapeHtml(img.name)}</span>`;
    btn.onclick = async () => {
      $('imageDialog').close();
      await openCropperFromUrl(img.url, img.name);
    };
    grid.appendChild(btn);
  }
  if (!grid.children.length) {
    grid.innerHTML = '<div class="empty compact-empty">没有可选图片</div>';
  }
  $('imageDialog').showModal();
}

function openCoverChoice() {
  $('coverChoiceDialog').showModal();
}

async function reloadSelectedGame() {
  const key = state.selected ? (state.selected.id || state.selected.path) : '';
  await loadGames(true);
  const updated = state.games.find((g) => (g.id || g.path) === key);
  if (updated) {
    state.selected = updated;
    $('detailCover').src = `/api/game/${gameUrl(updated)}/cover?t=${Date.now()}`;
    $('detailTitle').textContent = updated.title || '游戏详情';
    $('titleInput').value = updated.title || '';
    $('playTimeText').textContent = formatPlayTime(updated.playTime);
    $('playCountText').textContent = `${updated.playCount || 0} 次`;
    renderGameConfig(updated);
  }
}

function openCropperFromImage(img) {
  state.cropImage = img;
  const canvas = $('cropCanvas');
  const halfW = Math.floor(canvas.width / 2);
  const halfH = Math.floor(canvas.height / 2);
  $('cropScale').value = '1';
  $('cropX').value = '0';
  $('cropY').value = '0';
  $('cropX').min = String(-halfW);
  $('cropX').max = String(halfW);
  $('cropY').min = String(-halfH);
  $('cropY').max = String(halfH);
  drawCrop();
  $('cropDialog').showModal();
}

function openCropper(file) {
  if (!file) return;
  const img = new Image();
  img.onload = () => {
    openCropperFromImage(img);
    URL.revokeObjectURL(img.src);
  };
  img.src = URL.createObjectURL(file);
}

async function openCropperFromUrl(url, name = 'switch-cover.png') {
  showProgress('读取 Switch 图片');
  try {
    const res = await fetch(url);
    if (!res.ok) throw new Error('Switch 图片读取失败');
    const blob = await res.blob();
    openCropper(new File([blob], name, { type: blob.type || 'image/png' }));
  } finally {
    hideProgress();
  }
}

function drawCrop() {
  const img = state.cropImage;
  if (!img) return;
  const canvas = $('cropCanvas');
  const ctx = canvas.getContext('2d');
  ctx.clearRect(0, 0, canvas.width, canvas.height);
  ctx.fillStyle = getComputedStyle(document.body).getPropertyValue('--panel-2');
  ctx.fillRect(0, 0, canvas.width, canvas.height);
  const scale = Number($('cropScale').value);
  const base = Math.max(canvas.width / img.width, canvas.height / img.height);
  const w = img.width * base * scale;
  const h = img.height * base * scale;
  const x = (canvas.width - w) / 2 + Number($('cropX').value);
  const y = (canvas.height - h) / 2 + Number($('cropY').value);
  ctx.drawImage(img, x, y, w, h);
}

async function uploadCroppedCover() {
  const game = state.selected;
  if (!game) return;
  $('cropCanvas').toBlob(async (blob) => {
    if (!blob) return;
    const file = new File([blob], `${game.title || 'cover'}.png`, { type: 'image/png' });
    showProgress('上传封面');
    try {
      await uploadFile(file, `/api/game/${gameUrl(game)}/cover/start`, 'cover');
      $('cropDialog').close();
      toast('封面已上传，GameDB 已保存');
      await reloadSelectedGame();
    } catch (err) {
      toast(err.message);
    } finally {
      hideProgress();
    }
  }, 'image/png');
}

function bindEvents() {
  $('uploadZone').onclick = () => $('romInput').click();
  $('chooseRomFilesBtn').onclick = () => $('romInput').click();
  $('chooseRomFolderBtn').onclick = () => $('romFolderInput').click();
  $('importNameMappingSwitch').onchange = (e) => {
    state.importNameMapping = e.target.checked;
  };
  $('romInput').onchange = (e) => {
    uploadRoms([...e.target.files]);
    e.target.value = '';
  };
  $('romFolderInput').onchange = (e) => {
    uploadRoms([...e.target.files]);
    e.target.value = '';
  };
  $('uploadZone').ondragover = (e) => { e.preventDefault(); $('uploadZone').classList.add('active'); };
  $('uploadZone').ondragleave = () => $('uploadZone').classList.remove('active');
  $('uploadZone').ondrop = async (e) => {
    e.preventDefault();
    $('uploadZone').classList.remove('active');
    uploadRoms(await filesFromDropEvent(e));
  };
  $('libraryModeBtn').onclick = () => setMode('library');
  $('filesModeBtn').onclick = () => setMode('files');
  $('albumModeBtn').onclick = () => setMode('album');
  $('fileUploadBtn').onclick = () => $('fileBrowserInput').click();
  $('folderUploadBtn').onclick = () => $('folderBrowserInput').click();
  $('fileBrowserInput').onchange = (e) => {
    uploadBrowserFiles([...e.target.files]);
    e.target.value = '';
  };
  $('folderBrowserInput').onchange = (e) => {
    uploadBrowserFiles([...e.target.files]);
    e.target.value = '';
  };
  $('newFolderBtn').onclick = () => createFolder().catch((err) => toast(err.message));
  $('fileHomeBtn').onclick = () => loadFiles(state.fileRoot).catch((err) => toast(err.message));
  $('fileUpBtn').onclick = () => state.fileParent && loadFiles(state.fileParent).catch((err) => toast(err.message));
  $('fileRefreshBtn').onclick = () => loadFiles().catch((err) => toast(err.message));
  $('fileGoBtn').onclick = () => loadFiles($('filePathInput').value.trim()).catch((err) => toast(err.message));
  $('filePathInput').onkeydown = (e) => {
    if (e.key === 'Enter') loadFiles(e.target.value.trim()).catch((err) => toast(err.message));
  };
  $('closeFileImageBtn').onclick = () => $('fileImageDialog').close();
  $('albumRefreshBtn').onclick = () => loadAlbum().catch((err) => toast(err.message));
  $('closeVideoBtn').onclick = () => {
    const video = $('videoPreview');
    video.pause();
    video.removeAttribute('src');
    video.load();
    $('videoDialog').close();
  };
  $('closeTextEditorBtn').onclick = () => $('textEditorDialog').close();
  $('saveTextEditorBtn').onclick = () => saveTextEditor().catch((err) => toast(err.message));
  $('searchInput').oninput = (e) => { state.search = e.target.value; state.page = 1; renderGames(); };
  $('prevPageBtn').onclick = () => { state.page--; renderGames(); };
  $('nextPageBtn').onclick = () => { state.page++; renderGames(); };
  $('closeGameDialogBtn').onclick = closeGameDialog;
  $('saveTitleBtn').onclick = () => saveTitle().catch((err) => toast(err.message));
  $('saveConfigBtn').onclick = () => saveGameConfig().catch((err) => toast(err.message));
  $('removeBtn').onclick = () => removeGame().catch((err) => toast(err.message));
  $('replaceBatteryBtn').onclick = () => chooseSaveReplacement({ type: 'battery', slot: 0 });
  $('refreshSavesBtn').onclick = () => loadSaves().catch((err) => toast(err.message));
  $('saveInput').onchange = (e) => replaceSave(e.target.files[0]);
  $('replaceCoverBtn').onclick = openCoverChoice;
  $('closeCoverChoiceBtn').onclick = () => $('coverChoiceDialog').close();
  $('chooseSwitchCoverBtn').onclick = () => {
    $('coverChoiceDialog').close();
    openImageLibrary().catch((err) => toast(err.message));
  };
  $('chooseLocalCoverBtn').onclick = () => {
    $('coverChoiceDialog').close();
    $('coverInput').click();
  };
  $('coverInput').onchange = (e) => openCropper(e.target.files[0]);
  $('closeImageDialogBtn').onclick = () => $('imageDialog').close();
  $('closeCropDialogBtn').onclick = () => $('cropDialog').close();
  $('cropScale').oninput = drawCrop;
  $('cropX').oninput = drawCrop;
  $('cropY').oninput = drawCrop;
  $('uploadCroppedBtn').onclick = uploadCroppedCover;
  $('sortSelect').onchange = (e) => { state.sort = e.target.value; state.page = 1; renderGames(); };
  $('gridViewBtn').onclick = () => setView('grid');
  $('listViewBtn').onclick = () => setView('list');
  $('multiSelectBtn').onclick = () => setSelectionMode(true);
  $('selectPageBtn').onclick = toggleCurrentPageSelection;
  $('deleteSelectedBtn').onclick = () => deleteSelectedGames().catch((err) => toast(err.message));
  $('cancelSelectionBtn').onclick = () => setSelectionMode(false);
  $('cancelAllUploadsBtn').onclick = cancelAllUploads;
  $('closeUploadDialogBtn').onclick = () => {
    if (!state.uploadActive) $('uploadDialog').close();
  };
  $('lightBtn').onclick = () => setTheme('light');
  $('darkBtn').onclick = () => setTheme('dark');
  window.addEventListener('resize', () => {
    const nextSize = window.innerWidth <= 680 ? 12 : 36;
    if (nextSize !== state.pageSize) {
      state.pageSize = nextSize;
      state.page = 1;
      renderGames();
    }
  });
}

function setTheme(theme) {
  document.body.classList.toggle('dark', theme === 'dark');
  $('lightBtn').classList.toggle('active', theme === 'light');
  $('darkBtn').classList.toggle('active', theme === 'dark');
  localStorage.setItem('bls-theme', theme);
}

function setView(view) {
  if (state.mode === 'files') {
    state.fileView = view;
    renderFiles();
  } else if (state.mode === 'album') {
    state.albumView = view;
    renderAlbum();
  } else {
    state.view = view;
    renderGames();
  }
  $('gridViewBtn').classList.toggle('active', view === 'grid');
  $('listViewBtn').classList.toggle('active', view === 'list');
}

bindEvents();
setTheme(localStorage.getItem('bls-theme') || 'light');
loadGames().catch((err) => toast(err.message));
