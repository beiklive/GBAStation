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
  selectionMode: false,
  selectedIds: new Set(),
};

const platforms = [
  ['GBA', 'GBA'],
  ['GBC', 'GBC'],
  ['GB', 'GB'],
  ['FC', 'FC'],
  ['SFC', 'SFC'],
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
  return game.platformName || ({ 1: 'GBA', 2: 'GBC', 3: 'GB', 4: 'FC', 5: 'SFC' }[game.platform] || 'OTHER');
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

async function openGameDialog(game) {
  state.selected = game;
  $('detailPlatform').textContent = platformOf(game);
  $('detailPlatform').className = `badge ${platformClass(platformOf(game))}`;
  $('detailTitle').textContent = game.title || '游戏详情';
  $('detailCover').src = `/api/game/${gameUrl(game)}/cover?t=${Date.now()}`;
  $('titleInput').value = game.title || '';
  $('playTimeText').textContent = formatPlayTime(game.playTime);
  $('playCountText').textContent = `${game.playCount || 0} 次`;
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
      <img alt="槽位 ${item.slot} 截图" src="${item.thumbUrl}">
      <figcaption><span>槽位 ${item.slot}</span><button class="danger" title="删除截图"><i class="fa-solid fa-trash-can"></i></button></figcaption>
    `;
    figure.querySelector('button').onclick = () => deleteSave(item.thumbPath);
    root.appendChild(figure);
  }
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

async function uploadFile(file, startUrl, finishKind = 'rom', extraStartData = {}) {
  const start = await api(startUrl, {
    method: 'POST',
    body: JSON.stringify({ name: file.name, size: file.size, kind: finishKind, ...extraStartData }),
  });
  const chunkSize = 1024 * 1024;
  let offset = 0;
  while (offset < file.size) {
    const chunk = file.slice(offset, offset + chunkSize);
    await api(`/api/upload/chunk?token=${encodeURIComponent(start.token)}&offset=${offset}`, {
      method: 'POST',
      body: chunk,
      headers: { 'Content-Type': 'application/octet-stream' },
    });
    offset += chunk.size;
    setProgress(`${file.name}`, file.size ? offset / file.size : 1);
  }
  return api('/api/upload/finish', { method: 'POST', body: JSON.stringify({ token: start.token }) });
}

async function uploadRoms(files) {
  if (!files.length) return;
  showProgress('批量导入 ROM');
  try {
    for (let i = 0; i < files.length; i++) {
      $('progressTitle').textContent = `导入 ${i + 1} / ${files.length}`;
      await uploadFile(files[i], '/api/upload/start', 'rom');
    }
    toast('导入完成，GameDB 已保存');
    await loadGames(true);
  } catch (err) {
    toast(err.message);
  } finally {
    hideProgress();
  }
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
  }
}

async function removeGame() {
  const game = state.selected;
  if (!game) return;
  const deleteFile = confirm('是否同时删除 ROM 文件？\n取消则仅从游戏库移除。');
  await api(`/api/game/${gameUrl(game)}`, { method: 'DELETE', body: JSON.stringify({ deleteFile }) });
  toast('游戏已移除，GameDB 已保存');
  closeGameDialog();
  await loadGames(true);
}

async function deleteSelectedGames() {
  const selectedGames = state.games.filter((game) => state.selectedIds.has(gameKey(game)));
  if (!selectedGames.length) return;
  if (!confirm(`确认从游戏库移除 ${selectedGames.length} 个游戏？`)) return;
  const deleteFile = confirm('是否同时删除这些 ROM 文件？\n取消则仅从游戏库移除。');

  showProgress('批量删除游戏');
  let done = 0;
  try {
    for (const game of selectedGames) {
      const title = game.title || game.path || '未命名游戏';
      $('progressTitle').textContent = `删除 ${done + 1} / ${selectedGames.length}`;
      setProgress(title, done / selectedGames.length);
      await api(`/api/game/${gameUrl(game)}`, { method: 'DELETE', body: JSON.stringify({ deleteFile }) });
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
  $('romInput').onchange = (e) => uploadRoms([...e.target.files]);
  $('uploadZone').ondragover = (e) => { e.preventDefault(); $('uploadZone').classList.add('active'); };
  $('uploadZone').ondrop = (e) => { e.preventDefault(); uploadRoms([...e.dataTransfer.files]); };
  $('searchInput').oninput = (e) => { state.search = e.target.value; state.page = 1; renderGames(); };
  $('prevPageBtn').onclick = () => { state.page--; renderGames(); };
  $('nextPageBtn').onclick = () => { state.page++; renderGames(); };
  $('closeGameDialogBtn').onclick = closeGameDialog;
  $('saveTitleBtn').onclick = () => saveTitle().catch((err) => toast(err.message));
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
  state.view = view;
  $('gridViewBtn').classList.toggle('active', view === 'grid');
  $('listViewBtn').classList.toggle('active', view === 'list');
  renderGames();
}

bindEvents();
setTheme(localStorage.getItem('bls-theme') || 'light');
loadGames().catch((err) => toast(err.message));
