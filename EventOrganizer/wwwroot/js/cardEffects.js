(function () {
    /* ---- Staggered entrance ---- */
    function startCardAnimations() {
        document.querySelectorAll('.event-card').forEach((card, i) => {
            card.style.animation = 'none';
            card.style.opacity = '0';
            card.offsetHeight; /* reflow - animasyonu sıfırlar */
            card.style.opacity = '';
            card.style.animation = `cardEntrance 0.75s ${i * 0.07}s cubic-bezier(0.22, 1, 0.36, 1) both`;
        });
    }

    startCardAnimations();

    /* Kart sayısı farklı olduğunda: DOM değişimini izle */
    let navTimer;
    const articleEl = document.querySelector('article.content') || document.body;
    const observer = new MutationObserver(() => {
        clearTimeout(navTimer);
        navTimer = setTimeout(startCardAnimations, 30);
    });
    observer.observe(articleEl, { childList: true, subtree: true, characterData: true });

    /* Kart sayısı aynı olduğunda: Blazor DOM'u morph eder, childList değişmez.
       blazor:navigated her navigasyonda kesin tetiklendiğinden buradan yakala. */
    document.addEventListener('blazor:navigated', () => {
        clearTimeout(navTimer);
        navTimer = setTimeout(startCardAnimations, 50);
    });

    /* ---- Card Preview ---- */
    function openPreview(card) {
        const overlay = document.createElement('div');
        overlay.className = 'card-overlay';

        const clone = card.cloneNode(true);
        clone.style.animation  = '';
        clone.style.opacity    = '';
        clone.style.transform  = '';
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
        if (e.target.closest('.card-overlay')) return; // preview içindeyse yoksay
        const card = e.target.closest('.event-card');
        if (card) openPreview(card);
    });

    /* ---- Hover 3D effect ---- */
    function cardScale(card) {
        return card.closest('.card-overlay') ? 'scale(2)' : '';
    }

    function updateCard(card, clientX, clientY) {
        const rect = card.getBoundingClientRect();
        const x = clientX - rect.left;
        const y = clientY - rect.top;
        const rotX = ((y / rect.height) - 0.5) * -22;
        const rotY = ((x / rect.width) - 0.5) * 22;
        card.style.transform = `perspective(800px) rotateX(${rotX}deg) rotateY(${rotY}deg) ${cardScale(card)}`;
        card.style.setProperty('--mx', Math.round((x / rect.width) * 100) + '%');
        card.style.setProperty('--my', Math.round((y / rect.height) * 100) + '%');
        card.style.setProperty('--shine-o', '1');
    }

    function resetCard(card) {
        card.style.transform = `perspective(800px) rotateX(0deg) rotateY(0deg) ${cardScale(card)}`;
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
