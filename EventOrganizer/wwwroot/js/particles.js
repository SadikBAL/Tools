const SYMBOLS = [
    '{', '}', '[', ']', '(', ')', '<', '>', '/', '*', '=', '+', '-', ';', ':', '&', '|', '%', '$', '#', '@',
];

function initParticles() {
    let container = document.getElementById('particles-container');
    if (!container) {
        container = document.createElement('div');
        container.id = 'particles-container';
        container.style.cssText = 'position:fixed;inset:0;pointer-events:none;z-index:0;overflow:hidden;';
        document.body.prepend(container);
    } else {
        container.innerHTML = '';
    }

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
}

// Blazor navigation DOM'u yenilediğinde container kaldırılır — anında yeniden oluştur
new MutationObserver(() => {
    if (!document.getElementById('particles-container')) {
        initParticles();
    }
}).observe(document.body, { childList: true });

initParticles();
