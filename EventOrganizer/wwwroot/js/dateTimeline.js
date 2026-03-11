(function () {
    const TR_MONTHS = ['Oca', 'Şub', 'Mar', 'Nis', 'May', 'Haz', 'Tem', 'Ağu', 'Eyl', 'Eki', 'Kas', 'Ara'];

    /* ---- Shared filter state ---- */
    window.CardFilters = window.CardFilters || { dateLo: null, dateHi: null, categories: new Set() };

    window.applyCardFilters = function () {
        const { dateLo, dateHi, categories } = window.CardFilters;
        const allCards = Array.from(document.querySelectorAll('#cardsGrid .event-card'));

        const nowVisible = [];

        allCards.forEach(card => {
            let hide = false;

            if (dateLo !== null && card.dataset.date) {
                const d = new Date(card.dataset.date).getTime();
                hide = hide || d < dateLo || d > dateHi;
            }

            if (categories.size > 0) {
                const cats = ['music', 'tech', 'art', 'sport'];
                const cardCat = cats.find(c => card.classList.contains(c)) || '';
                hide = hide || !categories.has(cardCat);
            }

            const wasHidden = card.classList.contains('dt-hidden');
            card.classList.toggle('dt-hidden', hide);

            if (!hide && wasHidden) nowVisible.push(card);
        });

        // Re-trigger entrance animation for cards that just became visible
        nowVisible.forEach((card, i) => {
            card.style.animation = 'none';
            card.style.opacity = '0';
            card.offsetHeight;
            card.style.opacity = '';
            card.style.animation = `cardEntrance 0.55s ${i * 0.06}s cubic-bezier(0.22, 1, 0.36, 1) both`;
        });
    };

    function init() {
        const container = document.getElementById('dateTimeline');
        if (!container) return;

        const cards = Array.from(document.querySelectorAll('#cardsGrid .event-card[data-date]'));
        if (!cards.length) return;

        const dates = cards.map(c => new Date(c.dataset.date));
        const minDate = new Date(Math.min(...dates));
        const maxDate = new Date(Math.max(...dates));

        const start = new Date(minDate.getFullYear(), minDate.getMonth(), 1);
        const end   = new Date(maxDate.getFullYear(), maxDate.getMonth() + 1, 0);
        const totalMs = end - start;

        let pStart = 0, pEnd = 1;

        container.innerHTML = `
            <div class="dt-label-row">
                <span class="dt-title">Tarih Aralığı</span>
                <span class="dt-range-text" id="dtRangeText"></span>
            </div>
            <div class="dt-track-wrap">
                <div class="dt-track" id="dtTrack">
                    <div class="dt-fill" id="dtFill"></div>
                    <div class="dt-handle" id="dtHandleA">
                        <div class="dt-handle-inner"></div>
                        <div class="dt-handle-tip" id="dtTipA"></div>
                    </div>
                    <div class="dt-handle" id="dtHandleB">
                        <div class="dt-handle-inner"></div>
                        <div class="dt-handle-tip" id="dtTipB"></div>
                    </div>
                </div>
            </div>
            <div class="dt-months" id="dtMonths"></div>
        `;

        const track     = document.getElementById('dtTrack');
        const fill      = document.getElementById('dtFill');
        const handleA   = document.getElementById('dtHandleA');
        const handleB   = document.getElementById('dtHandleB');
        const tipA      = document.getElementById('dtTipA');
        const tipB      = document.getElementById('dtTipB');
        const rangeText = document.getElementById('dtRangeText');
        const monthsEl  = document.getElementById('dtMonths');

        // Build month ticks & labels
        const months = [];
        let cur = new Date(start);
        while (cur <= end) {
            months.push(new Date(cur));
            cur = new Date(cur.getFullYear(), cur.getMonth() + 1, 1);
        }

        months.forEach((m, i) => {
            const p   = (m - start) / totalMs;
            const pct = (p * 100).toFixed(2) + '%';

            const tick = document.createElement('div');
            tick.className = 'dt-tick';
            tick.style.left = pct;
            track.appendChild(tick);

            const lbl = document.createElement('div');
            lbl.className = 'dt-month-label';
            lbl.dataset.index = i;
            lbl.style.left = pct;
            lbl.textContent = TR_MONTHS[m.getMonth()];
            monthsEl.appendChild(lbl);
        });

        function dateAtP(p) { return new Date(start.getTime() + p * totalMs); }
        function formatDate(d) { return TR_MONTHS[d.getMonth()] + ' ' + d.getFullYear(); }

        function render() {
            const lo = Math.min(pStart, pEnd);
            const hi = Math.max(pStart, pEnd);

            fill.style.left  = (lo * 100) + '%';
            fill.style.width = ((hi - lo) * 100) + '%';
            handleA.style.left = (pStart * 100) + '%';
            handleB.style.left = (pEnd   * 100) + '%';

            const dA = dateAtP(pStart), dB = dateAtP(pEnd);
            tipA.textContent = formatDate(dA);
            tipB.textContent = formatDate(dB);

            const loDate = dateAtP(lo), hiDate = dateAtP(hi);
            rangeText.textContent = formatDate(loDate) + ' – ' + formatDate(hiDate);

            monthsEl.querySelectorAll('.dt-month-label').forEach(lbl => {
                const mp = (months[parseInt(lbl.dataset.index)] - start) / totalMs;
                lbl.classList.toggle('active', mp >= lo - 0.01 && mp <= hi + 0.01);
            });

            window.CardFilters.dateLo = loDate.getTime();
            window.CardFilters.dateHi = hiDate.getTime() + 86400000;
            window.applyCardFilters();
        }

        function makeDraggable(handle, setP) {
            handle.addEventListener('mousedown', e => {
                e.preventDefault();
                handle.classList.add('dragging');
                const onMove = ev => {
                    const rect = track.getBoundingClientRect();
                    setP(Math.max(0, Math.min(1, (ev.clientX - rect.left) / rect.width)));
                    render();
                };
                const onUp = () => {
                    handle.classList.remove('dragging');
                    document.removeEventListener('mousemove', onMove);
                    document.removeEventListener('mouseup', onUp);
                };
                document.addEventListener('mousemove', onMove);
                document.addEventListener('mouseup', onUp);
            });
        }

        makeDraggable(handleA, v => { pStart = v; });
        makeDraggable(handleB, v => { pEnd   = v; });

        render();
    }

    window.initDateTimeline = init;

    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', init);
    } else {
        init();
    }
})();
