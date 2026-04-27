'use strict';

const runBtn    = document.getElementById('run-btn');
const statusEl  = document.getElementById('status');
const loadingBar = document.getElementById('loading-bar');
const tableWrap = document.getElementById('table-wrap');
const emptyState = document.getElementById('empty-state');
const resultCount = document.getElementById('result-count');

let bgpModule = null;

function setStatus(msg, type = '') {
    statusEl.textContent = msg;
    statusEl.className = type;
}

function readFileAsText(file) {
    return new Promise((resolve, reject) => {
        const reader = new FileReader();
        reader.onload = e => resolve(e.target.result);
        reader.onerror = () => reject(new Error('Failed to read file: ' + file.name));
        reader.readAsText(file);
    });
}

function renderResults(data) {
    if (data.error) {
        setStatus('Error: ' + data.error, 'error');
        return;
    }

    const rows = data.results || [];

    resultCount.textContent = rows.length + ' prefix' + (rows.length !== 1 ? 'es' : '');
    resultCount.style.display = '';

    if (rows.length === 0) {
        tableWrap.innerHTML = '';
        tableWrap.appendChild(emptyState);
        emptyState.textContent = 'AS ' + data.asn + ' has no announcements after propagation.';
        return;
    }

    // Sort by prefix for stable display
    rows.sort((a, b) => a.prefix.localeCompare(b.prefix));

    const table = document.createElement('table');
    table.innerHTML = `
      <thead>
        <tr>
          <th>Prefix</th>
          <th>AS Path</th>
          <th>Received From</th>
        </tr>
      </thead>
    `;

    const tbody = document.createElement('tbody');
    for (const row of rows) {
        const tr = document.createElement('tr');
        const pathStr = row.as_path.join(' → ');
        const recvClass = 'recv-' + (row.recv_from || 'UNKNOWN');
        tr.innerHTML = `
          <td>${escapeHtml(row.prefix)}</td>
          <td class="as-path">${escapeHtml(pathStr)}</td>
          <td><span class="recv-badge ${recvClass}">${escapeHtml(row.recv_from || '')}</span></td>
        `;
        tbody.appendChild(tr);
    }

    table.appendChild(tbody);
    tableWrap.innerHTML = '';
    tableWrap.appendChild(table);
}

function escapeHtml(str) {
    return String(str)
        .replace(/&/g, '&amp;')
        .replace(/</g, '&lt;')
        .replace(/>/g, '&gt;')
        .replace(/"/g, '&quot;');
}

document.getElementById('sim-form').addEventListener('submit', async e => {
    e.preventDefault();
    if (!bgpModule) { setStatus('WASM module not loaded yet.', 'error'); return; }

    const topoFile = document.getElementById('topo-file').files[0];
    const annsFile = document.getElementById('anns-file').files[0];
    const targetAsn = parseInt(document.getElementById('target-asn').value, 10);
    const rovText   = document.getElementById('rov-input').value.trim();

    if (!topoFile || !annsFile || !targetAsn) {
        setStatus('Please fill in all required fields.', 'error');
        return;
    }

    runBtn.disabled = true;
    loadingBar.style.display = 'block';
    setStatus('Reading files…');
    tableWrap.innerHTML = '';
    resultCount.style.display = 'none';

    try {
        const [topoText, annsText] = await Promise.all([
            readFileAsText(topoFile),
            readFileAsText(annsFile),
        ]);

        setStatus('Running simulation…');
        // yield to browser so the UI updates before the synchronous WASM call
        await new Promise(r => setTimeout(r, 0));

        const jsonStr = bgpModule.runSimulation(topoText, annsText, rovText, targetAsn >>> 0);
        const data = JSON.parse(jsonStr);

        renderResults(data);
        if (!data.error) {
            const count = (data.results || []).length;
            setStatus(`Done — ${count} prefix${count !== 1 ? 'es' : ''} at AS ${targetAsn}.`, 'ok');
        }
    } catch (err) {
        setStatus('Unexpected error: ' + err.message, 'error');
        console.error(err);
    } finally {
        runBtn.disabled = false;
        loadingBar.style.display = 'none';
    }
});

// Load the WASM module
(async () => {
    try {
        // BgpModule is the factory function injected by bgp_simulator.js
        bgpModule = await BgpModule();
        runBtn.disabled = false;
        runBtn.textContent = 'Run Simulation';
        setStatus('Simulator ready.');
    } catch (err) {
        setStatus('Failed to load WASM: ' + err.message, 'error');
        console.error(err);
    }
})();
