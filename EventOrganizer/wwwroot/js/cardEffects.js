function initCardEffects() {
    document.querySelectorAll('.event-card').forEach(card => {
        if (card._cardInit) return;
        card._cardInit = true;

        card.addEventListener('mousemove', (e) => {
            const rect = card.getBoundingClientRect();
            const x = e.clientX - rect.left;
            const y = e.clientY - rect.top;
            const rotX = ((y / rect.height) - 0.5) * -22;
            const rotY = ((x / rect.width) - 0.5) * 22;
            const mx = Math.round((x / rect.width) * 100);
            const my = Math.round((y / rect.height) * 100);

            card.style.transform = `perspective(800px) rotateX(${rotX}deg) rotateY(${rotY}deg)`;
            card.style.setProperty('--mx', mx + '%');
            card.style.setProperty('--my', my + '%');
            card.style.setProperty('--shine-o', '1');
        });

        card.addEventListener('mouseleave', () => {
            card.style.transform = 'perspective(800px) rotateX(0deg) rotateY(0deg)';
            card.style.setProperty('--mx', '50%');
            card.style.setProperty('--my', '50%');
            card.style.setProperty('--shine-o', '0');
        });
    });
}

// İlk yükleme
initCardEffects();

// Blazor enhanced navigation
document.addEventListener('blazor:navigated', () => setTimeout(initCardEffects, 0));

// MutationObserver: DOM'a .event-card eklenince otomatik başlat
new MutationObserver(() => initCardEffects())
    .observe(document.body, { childList: true, subtree: true });
