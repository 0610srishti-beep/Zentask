const $ = (selector) => document.querySelector(selector);
const taskList = $('#task-list');
const modal = $('#task-modal');
const form = $('#task-form');
let tasks = [];
let activeFilter = 'all';
let sortPriority = true;
let editingId = null;
let toastTimer;

const todayISO = () => {
  const now = new Date();
  return `${now.getFullYear()}-${String(now.getMonth() + 1).padStart(2, '0')}-${String(now.getDate()).padStart(2, '0')}`;
};
const formatDate = (value) => {
  if (!value) return '';
  const date = new Date(`${value}T12:00:00`);
  if (Number.isNaN(date.getTime())) return value;
  return date.toLocaleDateString(undefined, { month: 'short', day: 'numeric', year: 'numeric' });
};
const escapeText = (value) => String(value ?? '').replace(/[&<>"']/g, (char) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' })[char]);
const isToday = (task) => task.due === todayISO();
const isOverdue = (task) => Boolean(task.due && task.due < todayISO());
const priorityName = (priority) => Number(priority) <= 1 ? 'Urgent' : Number(priority) === 2 ? 'High' : Number(priority) === 3 ? 'Normal' : 'Low';
const priorityClass = (priority) => Number(priority) <= 1 ? 'p1' : Number(priority) === 2 ? 'p2' : Number(priority) === 3 ? 'p3' : 'p4';

async function api(url, options = {}) {
  const response = await fetch(url, {
    ...options,
    headers: { ...(options.body ? { 'Content-Type': 'application/json' } : {}), ...(options.headers || {}) }
  });
  const payload = await response.json().catch(() => ({}));
  if (!response.ok) throw new Error(payload.error || `Request failed (${response.status})`);
  return payload;
}

function showToast(message) {
  const toast = $('#toast');
  toast.textContent = message;
  toast.classList.add('show');
  clearTimeout(toastTimer);
  toastTimer = setTimeout(() => toast.classList.remove('show'), 2800);
}

function updateStats() {
  const todayCount = tasks.filter(isToday).length;
  const overdueCount = tasks.filter(isOverdue).length;
  $('#stat-total').textContent = tasks.length;
  $('#stat-today').textContent = todayCount;
  $('#stat-overdue').textContent = overdueCount;
  $('#nav-all').textContent = tasks.length;
  $('#nav-today').textContent = todayCount;
  $('#nav-overdue').textContent = overdueCount;
  const top = [...tasks].sort((a, b) => Number(a.priority) - Number(b.priority) || Number(a.id) - Number(b.id))[0];
  $('#top-task-title').textContent = top ? top.title : 'Nothing urgent';
  $('#top-task-note').textContent = top ? `Priority ${top.priority} · task #${top.id}` : 'You’re clear to choose what matters.';
}

function visibleTasks() {
  const query = $('#search-input').value.trim().toLowerCase();
  const priority = $('#priority-filter').value;
  let filtered = tasks.filter((task) => {
    const matchesView = activeFilter === 'all' || (activeFilter === 'today' && isToday(task)) || (activeFilter === 'overdue' && isOverdue(task));
    const matchesQuery = !query || `${task.title} ${task.description} ${task.due} ${task.id}`.toLowerCase().includes(query);
    const p = Number(task.priority);
    const matchesPriority = priority === 'all' || (priority === '4' ? p >= 4 : p === Number(priority));
    return matchesView && matchesQuery && matchesPriority;
  });
  filtered.sort((a, b) => sortPriority ? Number(a.priority) - Number(b.priority) || Number(a.id) - Number(b.id) : Number(b.id) - Number(a.id));
  return filtered;
}

function renderTasks() {
  updateStats();
  const filtered = visibleTasks();
  const names = { all: 'A clear list is a good place to start.', today: 'Just the tasks due today.', overdue: 'No shame — let’s figure out the next step.' };
  $('#list-subtitle').textContent = names[activeFilter];
  $('#result-count').textContent = `${filtered.length} ${filtered.length === 1 ? 'task' : 'tasks'}`;
  if (!filtered.length) {
    const isEmptyWorkspace = tasks.length === 0 && activeFilter === 'all' && !$('#search-input').value && $('#priority-filter').value === 'all';
    taskList.innerHTML = `<div class="empty-state"><div class="empty-icon">${isEmptyWorkspace ? '✳' : '⌕'}</div><h3>${isEmptyWorkspace ? 'Start with one small thing' : 'Nothing to see here'}</h3><p>${isEmptyWorkspace ? 'Add a task and get it out of your head.' : 'Try another filter or tweak your search.'}</p>${isEmptyWorkspace ? '<button class="primary-btn" id="empty-add-btn">＋ Add your first task</button>' : ''}</div>`;
    $('#empty-add-btn')?.addEventListener('click', () => openModal());
    return;
  }
  taskList.innerHTML = filtered.map((task) => {
    let dueLabel = task.due ? formatDate(task.due) : 'No due date';
    let dueClass = 'due-pill';
    if (isOverdue(task)) { dueLabel = `Overdue · ${formatDate(task.due)}`; dueClass += ' overdue'; }
    else if (isToday(task)) { dueLabel = 'Due today'; dueClass += ' today'; }
    return `<article class="task-row" data-id="${Number(task.id)}">
      <div class="task-main"><div class="task-title-line"><span class="task-title">${escapeText(task.title)}</span></div>
      ${task.description ? `<p class="task-description">${escapeText(task.description)}</p>` : ''}
      <div class="task-meta"><span class="priority-pill ${priorityClass(task.priority)}">${escapeText(priorityName(task.priority))} · P${Number(task.priority)}</span><span class="${dueClass}">${escapeText(dueLabel)}</span><span class="task-id">#${Number(task.id)}</span></div></div>
      <div class="task-actions"><button class="action-btn edit-btn" data-action="edit" data-id="${Number(task.id)}" aria-label="Edit ${escapeText(task.title)}" title="Edit task">✎</button><button class="action-btn delete" data-action="delete" data-id="${Number(task.id)}" aria-label="Delete ${escapeText(task.title)}" title="Delete task">⌫</button></div>
    </article>`;
  }).join('');
}

async function refreshTasks() {
  taskList.innerHTML = '<div class="loading-state">Getting your tasks ready…</div>';
  try {
    tasks = await api('/api/tasks');
    renderTasks();
  } catch (error) {
    taskList.innerHTML = `<div class="empty-state"><div class="empty-icon">!</div><h3>Can’t reach the ZenTask server</h3><p>${escapeText(error.message)}. Make sure <code>zentask-web.exe</code> is still running.</p></div>`;
    showToast('Server connection failed. Check the terminal window.');
  }
}

function openModal(task = null) {
  editingId = task ? Number(task.id) : null;
  form.reset();
  $('#form-error').textContent = '';
  $('#modal-title').textContent = task ? 'Edit task' : 'Add a task';
  $('#save-task-btn').innerHTML = task ? 'Save changes <span>→</span>' : 'Save task <span>→</span>';
  $('#task-title').value = task?.title || '';
  $('#task-description').value = task?.description || '';
  $('#task-priority').value = String(task?.priority ?? 3);
  $('#task-due').value = task?.due || '';
  modal.hidden = false;
  document.body.style.overflow = 'hidden';
  setTimeout(() => $('#task-title').focus(), 30);
}
function closeModal() { modal.hidden = true; document.body.style.overflow = ''; editingId = null; }

form.addEventListener('submit', async (event) => {
  event.preventDefault();
  const title = $('#task-title').value.trim();
  if (!title) { $('#form-error').textContent = 'Give your task a title first.'; return; }
  const payload = { title, description: $('#task-description').value.trim(), priority: Number($('#task-priority').value), due: $('#task-due').value };
  const wasEditing = Boolean(editingId);
  const button = $('#save-task-btn');
  button.disabled = true; button.textContent = 'Saving…'; $('#form-error').textContent = '';
  try {
    if (editingId) await api(`/api/tasks/${editingId}`, { method: 'PUT', body: JSON.stringify(payload) });
    else await api('/api/tasks', { method: 'POST', body: JSON.stringify(payload) });
    closeModal(); await refreshTasks(); showToast(wasEditing ? 'Task updated. Nice and tidy.' : 'Task added. One less thing to remember.');
  } catch (error) { $('#form-error').textContent = error.message; }
  finally { button.disabled = false; button.innerHTML = wasEditing ? 'Save changes <span>→</span>' : 'Save task <span>→</span>'; }
});

taskList.addEventListener('click', async (event) => {
  const button = event.target.closest('button[data-action]');
  if (!button) return;
  const id = Number(button.dataset.id);
  const task = tasks.find((item) => Number(item.id) === id);
  if (button.dataset.action === 'edit' && task) { openModal(task); return; }
  if (button.dataset.action === 'delete' && task) {
    if (!window.confirm(`Delete “${task.title}”? This will remove it from your saved task list.`)) return;
    try { await api(`/api/tasks/${id}`, { method: 'DELETE' }); await refreshTasks(); showToast('Task deleted.'); }
    catch (error) { showToast(error.message); }
  }
});

$('#add-task-btn').addEventListener('click', () => openModal());
$('#close-modal').addEventListener('click', closeModal);
$('#cancel-modal').addEventListener('click', closeModal);
modal.addEventListener('click', (event) => { if (event.target === modal) closeModal(); });
document.addEventListener('keydown', (event) => { if (event.key === 'Escape' && !modal.hidden) closeModal(); });
$('#search-input').addEventListener('input', renderTasks);
$('#priority-filter').addEventListener('change', renderTasks);
$('#refresh-btn').addEventListener('click', refreshTasks);
$('#sort-btn').addEventListener('click', () => { sortPriority = !sortPriority; $('#sort-btn').innerHTML = sortPriority ? '↕ <span>Priority</span>' : '↕ <span>Newest</span>'; renderTasks(); });
document.querySelectorAll('.nav-item[data-filter]').forEach((button) => button.addEventListener('click', () => {
  activeFilter = button.dataset.filter;
  document.querySelectorAll('.nav-item').forEach((item) => item.classList.toggle('active', item === button));
  renderTasks();
}));
$('#today-label').textContent = new Date().toLocaleDateString(undefined, { weekday: 'short', month: 'short', day: 'numeric' });
refreshTasks();
