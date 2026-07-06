const API_BASE = "http://localhost:8080";

const $ = (id) => document.getElementById(id);

async function request(path, options = {}) {
    const response = await fetch(`${API_BASE}${path}`, {
        headers: {
            "Content-Type": "application/json",
            ...(options.headers || {})
        },
        ...options
    });

    const data = await response.json().catch(() => ({}));

    if (!response.ok) {
        throw new Error(data.error || "请求失败");
    }

    return data;
}

function showMessage(message, type = "info") {
    const el = $("message");
    el.textContent = message;
    el.className = `message ${type}`;
}

async function startBackup() {
    const payload = {
        source: $("backupSource").value.trim(),
        destination: $("backupDestination").value.trim(),
        compress: $("backupCompress").checked,
        encrypt: $("backupEncrypt").checked,
        password: $("backupPassword").value
    };

    if (!payload.source || !payload.destination) {
        showMessage("请输入源目录和备份目录", "error");
        return;
    }

    try {
        const result = await request("/api/backup", {
            method: "POST",
            body: JSON.stringify(payload)
        });

        showMessage(`备份任务已创建，任务编号：${result.taskId}`, "success");
        refreshTasks();
    } catch (error) {
        showMessage(error.message, "error");
    }
}

async function startRestore() {
    const payload = {
        backupPath: $("restorePath").value.trim(),
        destination: $("restoreDestination").value.trim(),
        password: $("restorePassword").value
    };

    if (!payload.backupPath || !payload.destination) {
        showMessage("请输入备份路径和还原目录", "error");
        return;
    }

    try {
        const result = await request("/api/restore", {
            method: "POST",
            body: JSON.stringify(payload)
        });

        showMessage(`还原任务已创建，任务编号：${result.taskId}`, "success");
        refreshTasks();
    } catch (error) {
        showMessage(error.message, "error");
    }
}

async function startWatch() {
    const payload = {
        source: $("watchSource").value.trim(),
        destination: $("watchDestination").value.trim(),
        intervalSeconds: Number($("watchInterval").value || 10)
    };

    if (!payload.source || !payload.destination) {
        showMessage("请输入监听目录和备份目录", "error");
        return;
    }

    try {
        const result = await request("/api/watch/start", {
            method: "POST",
            body: JSON.stringify(payload)
        });

        showMessage(result.message, "success");
    } catch (error) {
        showMessage(error.message, "error");
    }
}

async function stopWatch() {
    try {
        const result = await request("/api/watch/stop", {
            method: "POST",
            body: JSON.stringify({})
        });

        showMessage(result.message, "success");
    } catch (error) {
        showMessage(error.message, "error");
    }
}

async function refreshTasks() {
    try {
        const tasks = await request("/api/tasks");
        const tbody = $("taskBody");

        tbody.innerHTML = "";

        for (const task of tasks.reverse()) {
            const tr = document.createElement("tr");

            tr.innerHTML = `
        <td>${task.id}</td>
        <td>${escapeHtml(task.type)}</td>
        <td>${escapeHtml(task.source)}</td>
        <td>${escapeHtml(task.destination)}</td>
        <td><span class="status ${task.status}">${escapeHtml(task.status)}</span></td>
        <td>${escapeHtml(task.message || "")}</td>
        <td>${escapeHtml(task.createdAt)}</td>
      `;

            tbody.appendChild(tr);
        }
    } catch (error) {
        showMessage(error.message, "error");
    }
}

function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#039;");
}

function bindEvents() {
    $("backupBtn").addEventListener("click", startBackup);
    $("restoreBtn").addEventListener("click", startRestore);
    $("watchStartBtn").addEventListener("click", startWatch);
    $("watchStopBtn").addEventListener("click", stopWatch);
    $("refreshBtn").addEventListener("click", refreshTasks);

    setInterval(refreshTasks, 3000);
    refreshTasks();
}

document.addEventListener("DOMContentLoaded", bindEvents);