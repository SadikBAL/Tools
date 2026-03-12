/* ---- Location Tooltip ---- */
(function () {
    let tip = null;
    let hideTimer = null;

    function getOrCreate() {
        if (!tip) {
            tip = document.createElement('div');
            tip.className = 'location-tooltip';
            tip.innerHTML = '<span class="location-tooltip-pin">📍</span><span class="location-tooltip-text"></span>';
            document.body.appendChild(tip);
        }
        return tip;
    }

    function show(el) {
        const text = el.dataset.location;
        if (!text) return;

        clearTimeout(hideTimer);
        const t   = getOrCreate();
        const row = el.closest('.card-info-row') || el;
        const rect = row.getBoundingClientRect();

        t.querySelector('.location-tooltip-text').textContent = text;
        t.classList.remove('visible');

        // Konum satırının hemen üstüne konumlandır (translateY(-100%-8px) ile CSS'te çözülür)
        t.style.left     = rect.left + 'px';
        t.style.top      = rect.top  + 'px';
        t.style.minWidth = rect.width + 'px';

        requestAnimationFrame(() => requestAnimationFrame(() => t.classList.add('visible')));
    }

    function hide(delay) {
        clearTimeout(hideTimer);
        hideTimer = setTimeout(() => {
            if (tip) tip.classList.remove('visible');
        }, delay || 0);
    }

    document.addEventListener('mouseover', (e) => {
        const el = e.target.closest('.card-location-text');
        if (el) show(el);
    });

    document.addEventListener('mouseout', (e) => {
        if (e.target.closest('.card-location-text')) hide(120);
    });
})();

/* ---- Unified card action registry (delete + join + leave) ---- */
window._cardRefs = {};

window.registerCard = function (eventId, dotNetRef) {
    window._cardRefs[eventId] = dotNetRef;
};

window.unregisterCard = function (eventId) {
    delete window._cardRefs[eventId];
};

/* ---- Custom delete confirmation overlay ---- */
function showDeleteConfirm(title) {
    return new Promise((resolve) => {
        const overlay = document.createElement('div');
        overlay.className = 'delete-confirm-overlay';

        const safe = title.replace(/</g, '&lt;').replace(/>/g, '&gt;');
        overlay.innerHTML = `
            <div class="delete-confirm-panel">
                <span class="delete-confirm-icon">🗑</span>
                <div class="delete-confirm-title">Etkinliği Sil</div>
                <div class="delete-confirm-message">
                    "<span class="delete-confirm-event-name">${safe}</span>"<br>
                    etkinliğini silmek istediğinize emin misiniz?
                </div>
                <div class="delete-confirm-actions">
                    <button class="delete-confirm-cancel">İptal</button>
                    <button class="delete-confirm-ok">Sil</button>
                </div>
            </div>`;

        function close(result) {
            overlay.classList.add('closing');
            overlay.addEventListener('animationend', () => {
                overlay.remove();
                resolve(result);
            }, { once: true });
        }

        const okBtn = overlay.querySelector('.delete-confirm-ok');
        let holdTimer = null;

        function startHold(e) {
            e.preventDefault();
            if (holdTimer) return;
            okBtn.classList.add('holding');
            holdTimer = setTimeout(() => {
                holdTimer = null;
                close(true);
            }, 1000);
        }

        function cancelHold() {
            if (!holdTimer) return;
            clearTimeout(holdTimer);
            holdTimer = null;
            okBtn.classList.remove('holding');
        }

        okBtn.addEventListener('mousedown',   startHold);
        okBtn.addEventListener('mouseup',     cancelHold);
        okBtn.addEventListener('mouseleave',  cancelHold);
        okBtn.addEventListener('touchstart',  startHold,  { passive: false });
        okBtn.addEventListener('touchend',    cancelHold);
        okBtn.addEventListener('touchcancel', cancelHold);

        overlay.querySelector('.delete-confirm-cancel').addEventListener('click', () => close(false));
        overlay.addEventListener('click', (e) => { if (e.target === overlay) close(false); });

        document.addEventListener('keydown', function onKey(e) {
            if (e.key === 'Escape') { document.removeEventListener('keydown', onKey); close(false); }
        });

        document.body.appendChild(overlay);
    });
}

/* ---- Global delete button handler (works in grid AND inside preview clone) ---- */
document.addEventListener('click', async (e) => {
    const btn = e.target.closest('.card-delete-btn');
    if (!btn) return;

    const card    = btn.closest('.event-card');
    const title   = card?.querySelector('.card-title')?.textContent?.trim() || '';
    const eventId = btn.dataset.eventId;
    if (!eventId) return;

    const confirmed = await showDeleteConfirm(title);
    if (confirmed) {
        const previewOverlay = document.querySelector('.card-overlay');
        if (previewOverlay) previewOverlay.remove();

        const ref = window._cardRefs?.[eventId];
        if (ref) await ref.invokeMethodAsync('ConfirmAndDelete');
    }
});

/* ---- Share toast ---- */
function showShareToast(msg) {
    document.querySelectorAll('.share-toast').forEach(t => t.remove());
    const t = document.createElement('div');
    t.className = 'share-toast';
    t.textContent = msg;
    document.body.appendChild(t);
    requestAnimationFrame(() => requestAnimationFrame(() => t.classList.add('visible')));
    setTimeout(() => {
        t.classList.remove('visible');
        setTimeout(() => t.remove(), 350);
    }, 2200);
}

/* ---- Global share button handler ---- */
document.addEventListener('click', async (e) => {
    const btn = e.target.closest('.card-share-btn');
    if (!btn) return;
    e.stopPropagation();

    const eventId = btn.dataset.eventId;
    const title   = btn.dataset.eventTitle || 'Etkinlik';
    if (!eventId) return;

    const url = window.location.origin + '/event/' + eventId;

    if (navigator.share) {
        try { await navigator.share({ title, url }); } catch (_) { /* iptal */ }
    } else {
        try {
            await navigator.clipboard.writeText(url);
            showShareToast('🔗 Link kopyalandı!');
        } catch (_) {
            showShareToast('Link: ' + url);
        }
    }
});

/* ---- Global join / leave button handler (works in grid AND inside preview clone) ---- */
document.addEventListener('click', async (e) => {
    const btn = e.target.closest('.card-action-btn');
    if (!btn) return;

    const eventId = btn.dataset.eventId;
    const action  = btn.dataset.action;
    if (!eventId || !action) return;

    const ref = window._cardRefs?.[eventId];
    if (!ref) return;

    const previewOverlay = document.querySelector('.card-overlay');
    if (previewOverlay) previewOverlay.remove();

    if (action === 'join')  await ref.invokeMethodAsync('ConfirmJoin');
    if (action === 'leave') await ref.invokeMethodAsync('ConfirmLeave');
});

(function () {
    /* ---- Staggered entrance ---- */
    function startCardAnimations() {
        document.querySelectorAll('.event-card').forEach((card, i) => {
            card.style.animation = 'none';
            card.style.opacity = '0';
            card.offsetHeight; /* reflow */
            card.style.opacity = '';
            card.style.animation = `cardEntrance 0.75s ${i * 0.07}s cubic-bezier(0.22, 1, 0.36, 1) both`;
        });
    }

    startCardAnimations();

    /* Observer sadece #cardsGrid'i izler — filtre/slider DOM değişikliklerini atlar */
    let navTimer;
    let _observer = null;

    function attachObserver() {
        if (_observer) _observer.disconnect();
        const grid = document.getElementById('cardsGrid');
        if (!grid) return;
        _observer = new MutationObserver(() => {
            clearTimeout(navTimer);
            navTimer = setTimeout(startCardAnimations, 30);
        });
        _observer.observe(grid, { childList: true, subtree: true });
    }

    attachObserver();

    /* blazor:navigated her navigasyonda kesin tetiklendiğinden buradan da yakala */
    document.addEventListener('blazor:navigated', () => {
        clearTimeout(navTimer);
        navTimer = setTimeout(() => {
            startCardAnimations();
            attachObserver(); // yeni sayfadaki #cardsGrid'e yeniden bağlan
        }, 50);
    });

    /* ---- Card Preview ---- */
    function openPreview(card) {
        const overlay = document.createElement('div');
        overlay.className = 'card-overlay';

        const clone = card.cloneNode(true);
        clone.style.animation = 'none';
        clone.style.opacity   = '1';
        clone.style.transform = 'perspective(600px) rotateX(0deg) rotateY(0deg) scale(2)';
        overlay.appendChild(clone);
        document.body.appendChild(overlay);

        function close() {
            overlay.classList.add('closing');
            overlay.addEventListener('animationend', () => overlay.remove(), { once: true });
            document.removeEventListener('keydown', onKey);
        }

        overlay.addEventListener('click', (e) => {
            if (!e.target.closest('.event-card')) close();
        });

        function onKey(e) { if (e.key === 'Escape') close(); }
        document.addEventListener('keydown', onKey);
    }

    document.addEventListener('click', (e) => {
        if (e.target.closest('.card-overlay')) return;
        if (e.target.closest('.card-delete-btn')) return;
        if (e.target.closest('.card-action-btn')) return;
        if (e.target.closest('.card-share-btn'))    return;
        if (e.target.closest('.card-location-text')) return;
        if (e.target.closest('.card-date-link'))     return;
        const card = e.target.closest('.event-card');
        if (card) openPreview(card);
    });

    /* Event detail sayfasında ilk kartı otomatik preview olarak aç */
    window.openEventPreview = function () {
        const card = document.querySelector('.event-card');
        if (card) openPreview(card);
    };

    /* ---- Hover 3D effect ---- */
    // Rotasyonu .card-rotator'a uyguluyoruz: .event-card üzerindeki animasyonlarla
    // (cardEntrance, previewIn !important) çakışmıyor, bağımsız çalışıyor.

    function updateCard(card, clientX, clientY) {
        const rotator = card.querySelector('.card-rotator');
        if (!rotator) return;
        const rect = card.getBoundingClientRect();
        const x = clientX - rect.left;
        const y = clientY - rect.top;
        const rotX = ((y / rect.height) - 0.5) * -20;
        const rotY = ((x / rect.width) - 0.5) * 20;
        rotator.style.transform = `perspective(600px) rotateX(${rotX}deg) rotateY(${rotY}deg)`;
        card.style.setProperty('--mx', Math.round((x / rect.width) * 100) + '%');
        card.style.setProperty('--my', Math.round((y / rect.height) * 100) + '%');
        card.style.setProperty('--shine-o', '1');
    }

    function resetCard(card) {
        const rotator = card.querySelector('.card-rotator');
        if (rotator) rotator.style.transform = '';
        card.style.setProperty('--mx', '50%');
        card.style.setProperty('--my', '50%');
        card.style.setProperty('--shine-o', '0');
    }

    document.addEventListener('mousemove', (e) => {
        const card = e.target.closest('.event-card');
        if (card) updateCard(card, e.clientX, e.clientY);
    });

    document.addEventListener('mouseout', (e) => {
        const card = e.target.closest('.event-card');
        if (card && !card.contains(e.relatedTarget)) resetCard(card);
    });
})();
