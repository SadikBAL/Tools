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

/* ---- Participant list popup ---- */
document.addEventListener('click', (e) => {
    const row = e.target.closest('.participant-avatars');
    if (!row) return;
    e.stopPropagation();

    let participants;
    try { participants = JSON.parse(row.dataset.participants || '[]'); } catch { return; }
    if (!participants.length) return;

    const eventId       = row.dataset.eventId      || '';
    const ownerId       = row.dataset.ownerId       || '';
    const currentUserId = row.dataset.currentUserId || '';
    const isOwnerViewing = currentUserId && currentUserId === ownerId;

    document.querySelector('.participants-overlay')?.remove();

    const overlay = document.createElement('div');
    overlay.className = 'participants-overlay';

    const panel = document.createElement('div');
    panel.className = 'participants-panel';

    const countBadge = document.createElement('span');
    countBadge.className = 'participants-count';
    countBadge.textContent = participants.length;

    const header = document.createElement('div');
    header.className = 'participants-header';
    header.innerHTML = `<span class="participants-title">Katılımcılar</span><button class="participants-close">✕</button>`;
    header.querySelector('.participants-title').after(countBadge);

    const list = document.createElement('div');
    list.className = 'participants-list';

    const buildRow = (p) => {
        const item = document.createElement('div');
        item.className = 'participants-item';

        const name    = p.name       || '?';
        const pic     = p.pictureUrl || '';
        const userId  = p.userId     || '';
        const isOwner = userId === ownerId;

        // Avatar
        const avatarWrap = document.createElement('div');
        avatarWrap.style.flexShrink = '0';
        if (pic) {
            avatarWrap.innerHTML = `<img class="participants-avatar" src="${pic}" referrerpolicy="no-referrer" alt="${name}" />`;
        } else {
            const initial = name.length > 0 ? name[0].toUpperCase() : '?';
            avatarWrap.innerHTML = `<div class="participants-avatar participants-avatar-initial">${initial}</div>`;
        }

        // Name
        const nameEl = document.createElement('span');
        nameEl.className = 'participants-name';
        nameEl.textContent = name;

        // Role badge
        const roleEl = document.createElement('span');
        roleEl.className = 'participants-role ' + (isOwner ? 'role-owner' : 'role-member');
        roleEl.textContent = isOwner ? 'Kurucu' : 'Üye';

        item.appendChild(avatarWrap);
        item.appendChild(nameEl);
        item.appendChild(roleEl);

        // Kick button — only for owner viewing non-owner participants
        if (isOwnerViewing && !isOwner && userId) {
            const kickBtn = document.createElement('button');
            kickBtn.className = 'participants-kick';
            kickBtn.title = 'Ç\u0131kar';
            kickBtn.textContent = '✕';
            kickBtn.addEventListener('click', async (ev) => {
                ev.stopPropagation();
                kickBtn.disabled = true;
                const ref = window._cardRefs?.[eventId];
                if (!ref) return;
                const ok = await ref.invokeMethodAsync('KickParticipant', userId);
                if (ok) {
                    item.style.opacity = '0';
                    item.style.transition = 'opacity 0.2s';
                    setTimeout(() => item.remove(), 200);
                    const newCount = parseInt(countBadge.textContent) - 1;
                    countBadge.textContent = newCount;
                } else {
                    kickBtn.disabled = false;
                }
            });
            item.appendChild(kickBtn);
        }

        return item;
    };

    participants.forEach(p => list.appendChild(buildRow(p)));

    panel.appendChild(header);
    panel.appendChild(list);
    overlay.appendChild(panel);
    document.body.appendChild(overlay);

    requestAnimationFrame(() => overlay.classList.add('visible'));

    const close = () => {
        overlay.classList.remove('visible');
        setTimeout(() => overlay.remove(), 220);
    };

    header.querySelector('.participants-close').addEventListener('click', close);
    overlay.addEventListener('click', (ev) => { if (ev.target === overlay) close(); });
    document.addEventListener('keydown', function esc(ev) {
        if (ev.key === 'Escape') { close(); document.removeEventListener('keydown', esc); }
    });
});

/* ---- Global edit button handler (works in grid AND inside preview clone) ---- */
document.addEventListener('click', (e) => {
    const btn = e.target.closest('.card-edit-btn');
    if (!btn) return;
    e.stopPropagation();
    const eventId = btn.dataset.eventId;
    if (eventId) window.location.href = '/edit-event/' + eventId;
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
const _pendingActions = new Set();

document.addEventListener('click', async (e) => {
    const btn = e.target.closest('.card-action-btn');
    if (!btn) return;

    const eventId = btn.dataset.eventId;
    const action  = btn.dataset.action;
    if (!eventId || !action) return;

    // Ön yüzdeki "Gir" butonu → kart arkasını aç, oradan katıl
    if (action === 'join' && btn.classList.contains('card-join-btn')) {
        const card = btn.closest('.event-card');
        card?.querySelector('.card-flip-btn:not(.card-flip-btn-back)')?.click();
        return;
    }

    if (_pendingActions.has(eventId)) return;

    const ref = window._cardRefs?.[eventId];
    if (!ref) return;

    _pendingActions.add(eventId);
    btn.disabled = true;

    try {
        if (action === 'join-slot') {
            const slotIndex = parseInt(btn.dataset.slotIndex, 10);
            if (!isNaN(slotIndex)) await ref.invokeMethodAsync('ConfirmJoinSlot', slotIndex);
        } else if (action === 'move-slot') {
            const slotIndex = parseInt(btn.dataset.slotIndex, 10);
            if (!isNaN(slotIndex)) await ref.invokeMethodAsync('ConfirmMoveSlot', slotIndex);
        } else if (action === 'leave') {
            await ref.invokeMethodAsync('ConfirmLeave');
        }
    } finally {
        _pendingActions.delete(eventId);
        // Blazor DOM güncellemesi gelmeden önce buton disabled kalmasın
        if (btn.isConnected) btn.disabled = false;
    }

    // Blazor'un yeniden render etmesini bekle, sonra sadece değişen parçaları güncelle
    await new Promise(r => setTimeout(r, 300));

    const overlay = document.querySelector('.card-overlay');
    if (!overlay) return;

    const originalCard = Array.from(document.querySelectorAll('.event-card'))
        .find(c => !c.closest('.card-overlay') && c.querySelector(`[data-event-id="${eventId}"]`));

    if (originalCard) {
        const overlayCard = overlay.querySelector('.event-card');
        if (!overlayCard) return;

        const sync = (sel) => {
            const src = originalCard.querySelector(sel);
            const dst = overlayCard.querySelector(sel);
            if (src && dst) dst.innerHTML = src.innerHTML;
        };

        sync('.capacity-count');
        sync('.participant-avatars');
        sync('.card-footer');
        sync('.card-slots-grid');
    }
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

        // Clone, orijinalin flip state'ini taşır — display'i senkronize et
        const cFlipper = clone.querySelector('.card-flipper');
        const cFront   = clone.querySelector('.card-front');
        const cBack    = clone.querySelector('.card-back');
        const cloneFlipped = cFlipper && cFlipper.dataset.flipped === '1';
        if (cFront) cFront.style.display = cloneFlipped ? 'none' : '';
        if (cBack)  cBack.style.display  = cloneFlipped ? 'flex'  : 'none';

        // Viewport'a göre scale hesapla — ekranın %80'ini kaplasın, max 2
        const scaleW = (window.innerWidth  * 0.80) / 245;
        const scaleH = (window.innerHeight * 0.85) / 342;
        const scale  = Math.min(scaleW, scaleH, 2);
        const mobile = scale < 2;

        if (mobile) {
            // Mobil: gyro rotasyonu card-rotator'a değil bir wrapper'a uygulanır.
            // Clone transformStyle:flat → butonlar güvenilir 2D hit-test alanına
            // sahip olur; artık preserve-3d kaynaklı click sentezi sorunu yok.
            clone.style.setProperty('animation', 'none', 'important');
            clone.style.transformStyle = 'flat';
            clone.style.opacity        = '0';
            clone.style.transform      = `scale(${(scale * 0.75).toFixed(3)})`;
            clone.style.transition     = 'opacity 0.25s ease, transform 0.32s cubic-bezier(0.22,1,0.36,1)';

            // Flat-mode: clone display'i zaten openPreview başında senkronize edildi

            const wrapper = document.createElement('div');
            wrapper.className = 'card-tilt-wrapper';
            wrapper.appendChild(clone);
            overlay.appendChild(wrapper);
        } else {
            // Desktop: inline 'none' — CSS previewIn !important override eder
            clone.style.animation = 'none';
            clone.style.opacity   = '1';
            clone.style.transform = `scale(${scale})`;
            overlay.appendChild(clone);
        }

        document.body.appendChild(overlay);

        if (mobile) {
            requestAnimationFrame(() => requestAnimationFrame(() => {
                clone.style.opacity   = '1';
                clone.style.transform = `scale(${scale.toFixed(3)})`;
            }));
        }

        let _closed = false;
        function close() {
            if (_closed) return;
            _closed = true;
            document.removeEventListener('keydown', onKey);
            if (mobile) {
                clone.style.opacity   = '0';
                clone.style.transform = `scale(${(scale * 0.75).toFixed(3)})`;
                overlay.style.transition = 'opacity 0.22s ease';
                overlay.style.opacity    = '0';
                setTimeout(() => overlay.remove(), 230);
            } else {
                overlay.classList.add('closing');
                overlay.addEventListener('animationend', () => overlay.remove(), { once: true });
            }
        }

        // Kart dışı tıklama/tap → kapat
        function isInsideCard(clientX, clientY) {
            const rect = clone.getBoundingClientRect();
            return clientX >= rect.left && clientX <= rect.right &&
                   clientY >= rect.top  && clientY <= rect.bottom;
        }

        overlay.addEventListener('click', (e) => {
            if (e.target.closest('.card-flip-btn, .card-action-btn, .card-share-btn')) return;
            if (!isInsideCard(e.clientX, e.clientY)) close();
        });

        // Mobile: kart dışına tap → overlay'i hemen kapat
        overlay.addEventListener('touchend', (e) => {
            const t = e.changedTouches[0];
            if (!t) return;
            if (e.target.closest('.card-flip-btn, .card-action-btn, .card-share-btn')) return;
            if (!isInsideCard(t.clientX, t.clientY)) {
                e.preventDefault();
                close();
            }
        }, { passive: false });

        function onKey(e) { if (e.key === 'Escape') close(); }
        document.addEventListener('keydown', onKey);
    }

    /* ---- Flip button handler ---- */
    const FLIP_HALF = 260; // ms — animasyonun yarısı

    document.addEventListener('click', (e) => {
        const btn = e.target.closest('.card-flip-btn');
        if (!btn) return;
        e.stopPropagation();

        const card    = btn.closest('.event-card');
        if (!card) return;
        const flipper = card.querySelector('.card-flipper');
        if (!flipper) return;
        const front   = card.querySelector('.card-front');
        const back    = card.querySelector('.card-back');
        if (!front || !back) return;

        // Animasyon sırasında tekrar tıklamayı engelle
        if (flipper.dataset.animating) return;
        flipper.dataset.animating = '1';

        const nextFlipped = flipper.dataset.flipped !== '1';

        // Flat-mode (mobil preview): animasyon yok, direkt swap
        if (card.style.transformStyle === 'flat') {
            front.style.display = nextFlipped ? 'none' : '';
            back.style.display  = nextFlipped ? 'flex' : 'none';
            flipper.dataset.flipped   = nextFlipped ? '1' : '';
            delete flipper.dataset.animating;
            return;
        }

        // Hover tilt'i sıfırla — flip animasyonuyla çakışmasın
        const rotator = card.querySelector('.card-rotator');
        if (rotator) rotator.style.transform = '';

        // 1. yarı: mevcut yüzü kenara döndür (edge-on)
        flipper.style.animation = `cardFlipHalf1 ${FLIP_HALF}ms ease-in forwards`;

        setTimeout(() => {
            // Ortada yüzleri değiştir
            front.style.display = nextFlipped ? 'none' : '';
            back.style.display  = nextFlipped ? 'flex' : 'none';

            // 2. yarı: yeni yüzü karşıdan getir
            flipper.style.animation = `cardFlipHalf2 ${FLIP_HALF}ms ease-out forwards`;

            setTimeout(() => {
                flipper.style.animation  = '';
                flipper.dataset.flipped  = nextFlipped ? '1' : '';
                delete flipper.dataset.animating;
            }, FLIP_HALF);
        }, FLIP_HALF);
    });

    document.addEventListener('click', (e) => {
        if (e.target.closest('.card-overlay')) return;
        if (e.target.closest('.card-delete-btn'))      return;
        if (e.target.closest('.card-edit-btn'))        return;
        if (e.target.closest('.card-action-btn'))      return;
        if (e.target.closest('.card-share-btn'))       return;
        if (e.target.closest('.card-flip-btn'))        return;
        if (e.target.closest('.participant-avatars'))  return;
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

    // pointerType: 'mouse' filtresi — touch tap'lerden sentezlenen sahte
    // mouse eventleri rotasyonu tetiklemez; sadece gerçek fare hareketi döndürür.
    document.addEventListener('pointermove', (e) => {
        if (e.pointerType !== 'mouse') return;
        const card = e.target.closest('.event-card');
        if (card) updateCard(card, e.clientX, e.clientY);
    });

    document.addEventListener('pointerout', (e) => {
        if (e.pointerType !== 'mouse') return;
        const card = e.target.closest('.event-card');
        if (card && !card.contains(e.relatedTarget)) resetCard(card);
    });
})();

/* ---- Card back slots drag-to-scroll ---- */
(function () {
    let drag = null;
    const THRESHOLD = 6; // px — bu kadar hareket etmeden capture alma, tap/click çalışsın

    document.addEventListener('pointerdown', (e) => {
        const grid = e.target.closest('.card-slots-grid');
        if (!grid) return;
        // Tıklanabilir slot butonuna basıldıysa drag başlatma — click çalışsın
        if (e.target.closest('.card-slot-joinable, .card-slot-moveable')) return;
        // Sadece bekle — henüz capture alma, click'i bozmasın
        drag = { grid, startY: e.clientY, startScroll: grid.scrollTop, id: e.pointerId, active: false };
    });

    document.addEventListener('pointermove', (e) => {
        if (!drag || drag.id !== e.pointerId) return;

        const moved = Math.abs(e.clientY - drag.startY);

        // Eşiği geçince drag moduna gir ve pointer'ı yakala
        if (!drag.active) {
            if (moved < THRESHOLD) return;
            drag.active = true;
            drag.grid.setPointerCapture(e.pointerId);
            drag.grid.classList.add('is-dragging');
        }

        e.preventDefault();
        e.stopPropagation();
        drag.grid.scrollTop = drag.startScroll - (e.clientY - drag.startY);
    }, { passive: false });

    function endDrag(e) {
        if (!drag || drag.id !== e.pointerId) return;
        drag.grid.classList.remove('is-dragging');
        drag = null;
    }

    document.addEventListener('pointerup',     endDrag);
    document.addEventListener('pointercancel', endDrag);
})();

/* ---- Gyroscope 3D tilt (sadece mobil/dokunmatik cihazlar) ---- */
(function () {
    // PC'de hiç çalışma — yalnızca gerçek dokunmatik/mobil cihazlarda
    const isTouchDevice = navigator.maxTouchPoints > 0 && /Mobi|Android|iPhone|iPad|iPod/i.test(navigator.userAgent);
    if (!isTouchDevice) return;
    if (!window.DeviceOrientationEvent) return;

    let baseBeta    = null;
    let targetGamma = 0, targetBeta = 0;
    let smoothGamma = 0, smoothBeta = 0;
    let rafId       = null;
    let gyroActive  = false;

    function applyTilt() {
        rafId = null;
        smoothGamma += (targetGamma - smoothGamma) * 0.10;
        smoothBeta  += (targetBeta  - smoothBeta)  * 0.10;

        const rotY  = Math.max(-10, Math.min(10,  smoothGamma / 3.5));
        const rotX  = Math.max(-10, Math.min(10, -(smoothBeta - baseBeta) / 3.5));
        const mxPct = Math.round(((rotY + 10) / 20) * 100) + '%';
        const myPct = Math.round(((rotX + 10) / 20) * 100) + '%';

        document.querySelectorAll('.event-card').forEach(card => {
            // Mobil preview: rotasyonu wrapper'a uygula — clone flat kalır,
            // card-rotator'ı döndürme (hit-test bozulur).
            const wrapper = card.closest('.card-tilt-wrapper');
            if (wrapper) {
                wrapper.style.transform = `perspective(600px) rotateX(${rotX.toFixed(2)}deg) rotateY(${rotY.toFixed(2)}deg)`;
                card.style.setProperty('--mx', mxPct);
                card.style.setProperty('--my', myPct);
                card.style.setProperty('--shine-o', '0.55');
                return;
            }

            const rotator = card.querySelector('.card-rotator');
            if (!rotator) return;
            rotator.style.transform = `perspective(600px) rotateX(${rotX.toFixed(2)}deg) rotateY(${rotY.toFixed(2)}deg)`;
            card.style.setProperty('--mx', mxPct);
            card.style.setProperty('--my', myPct);
            card.style.setProperty('--shine-o', '0.55');
        });

        if (Math.abs(targetGamma - smoothGamma) > 0.05 || Math.abs(targetBeta - smoothBeta) > 0.05)
            rafId = requestAnimationFrame(applyTilt);
    }

    function onOrientation(e) {
        // Jiroskop verisi yoksa veya sıfırsa çalışma
        if (e.gamma === null || e.beta === null) return;
        if (baseBeta === null) baseBeta = e.beta;
        targetGamma = e.gamma;
        targetBeta  = e.beta;
        if (!rafId) rafId = requestAnimationFrame(applyTilt);
    }

    async function enableGyro() {
        if (gyroActive) return;

        // iOS izin akışı
        if (typeof DeviceOrientationEvent.requestPermission === 'function') {
            try {
                const res = await DeviceOrientationEvent.requestPermission();
                if (res !== 'granted') return; // izin reddedildi → kartlar 0 eğimde kalır
            } catch {
                return; // hata → kartlar 0 eğimde kalır
            }
        }

        // Gerçekten veri gelip gelmediğini test et
        const hasData = await new Promise(resolve => {
            const t = setTimeout(() => resolve(false), 500);
            window.addEventListener('deviceorientation', e => {
                clearTimeout(t);
                resolve(e.gamma !== null && e.beta !== null);
            }, { once: true });
        });

        if (!hasData) return; // jiroskop yok → kartlar 0 eğimde kalır

        gyroActive = true;
        window.addEventListener('deviceorientation', onOrientation);
    }

    // Buton/link olmayan ilk click'te etkinleştir.
    // touchstart yerine click kullanıyoruz: iOS'ta touchstart sırasında
    // requestPermission dialog'u çıkarsa butonun click event'i kesilir.
    document.addEventListener('click', function gyroStarter(e) {
        if (e.target.closest('button, a, [role="button"]')) return;
        document.removeEventListener('click', gyroStarter);
        enableGyro();
    });
})();
