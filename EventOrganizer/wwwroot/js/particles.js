const SYMBOLS = [
    '{', '}', '[', ']', '(', ')', '<', '>', '/', '*', '=', '+', '-', ';', ':', '&', '|', '%', '$', '#', '@',
];

function initParticles() {
    if (document.getElementById('particles-container')) return;

    const container = document.createElement('div');
    container.id = 'particles-container';
    container.style.cssText = 'position:fixed;inset:0;pointer-events:none;z-index:0;overflow:hidden;';

    for (let i = 0; i < 18; i++) {
        const el = document.createElement('span');
        el.textContent = SYMBOLS[Math.floor(Math.random() * SYMBOLS.length)];

        const size     = 0.9 + Math.random() * 1.4;
        const left     = Math.random() * 100;
        const duration = 14 + Math.random() * 18;
        const delay    = -(Math.random() * duration);

        el.style.cssText = `
            position: fixed;
            left: ${left}%;
            bottom: -60px;
            font-size: ${size}rem;
            opacity: ${0.07 + Math.random() * 0.1};
            animation: particleFloat ${duration}s ${delay}s linear infinite;
            pointer-events: none;
            z-index: 0;
            user-select: none;
        `;

        container.appendChild(el);
    }

    document.body.prepend(container);
}

// Blazor enhanced navigation: container'ı navigasyon öncesi DOM'dan çıkar,
// sonra geri yerleştir. Bu sayede animasyonlar hiç kesilmez.
document.addEventListener('blazor:navigating', () => {
    const c = document.getElementById('particles-container');
    if (c) {
        c.remove();
        window.__particlesEl = c;
    }
});

document.addEventListener('blazor:navigated', () => {
    if (window.__particlesEl) {
        document.body.prepend(window.__particlesEl);
        window.__particlesEl = null;
    } else {
        initParticles();
    }
});

initParticles();
