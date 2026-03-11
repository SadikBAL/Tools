(function () {
    function updateCard(card, clientX, clientY) {
        const rect = card.getBoundingClientRect();
        const x = clientX - rect.left;
        const y = clientY - rect.top;
        const rotX = ((y / rect.height) - 0.5) * -22;
        const rotY = ((x / rect.width) - 0.5) * 22;
        card.style.transform = `perspective(800px) rotateX(${rotX}deg) rotateY(${rotY}deg)`;
        card.style.setProperty('--mx', Math.round((x / rect.width) * 100) + '%');
        card.style.setProperty('--my', Math.round((y / rect.height) * 100) + '%');
        card.style.setProperty('--shine-o', '1');
    }

    function resetCard(card) {
        card.style.transform = 'perspective(800px) rotateX(0deg) rotateY(0deg)';
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
