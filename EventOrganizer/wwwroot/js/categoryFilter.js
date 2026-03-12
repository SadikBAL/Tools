(function () {
    const CATEGORIES = [
        { key: 'music', label: 'Müzik',     color: '#9b7ec8' },
        { key: 'tech',  label: 'Teknoloji', color: '#5a9ab5' },
        { key: 'art',   label: 'Sanat',     color: '#c4834a' },
        { key: 'sport', label: 'Spor',      color: '#5a9e6f' },
    ];

    window.initCategoryFilter = function () {
        const container = document.getElementById('categoryFilter');
        if (!container) return;
        if (container.querySelector('.cf-label-row')) return; // zaten dolu

        // Önceki navigasyondan kalan orphan dropdown varsa temizle
        document.querySelectorAll('.cf-dropdown').forEach(d => d.remove());

        window.CardFilters = window.CardFilters || { dateLo: null, dateHi: null, categories: new Set() };
        window.CardFilters.categories = new Set();

        let dropdown = null;

        container.innerHTML = `
            <div class="cf-label-row">
                <span class="cf-title">Kategoriler</span>
                <span class="cf-count" id="cfCount"></span>
            </div>
            <div class="cf-tags-area" id="cfTagsArea">
                <button class="cf-add-btn" id="cfAddBtn" title="Kategori ekle">+</button>
            </div>
        `;

        const tagsArea = container.querySelector('#cfTagsArea');
        const addBtn   = container.querySelector('#cfAddBtn');
        const countEl  = container.querySelector('#cfCount');

        function updateCount() {
            const n = window.CardFilters.categories.size;
            countEl.textContent = n ? n + ' seçili' : 'Tümü';
        }

        function renderTags() {
            tagsArea.querySelectorAll('.cf-tag').forEach(t => t.remove());
            window.CardFilters.categories.forEach(key => {
                const cat  = CATEGORIES.find(c => c.key === key);
                const chip = document.createElement('div');
                chip.className  = `cf-tag cf-tag--${key}`;
                chip.dataset.key = key;
                chip.innerHTML  = `${cat.label}<span class="cf-tag-x">×</span>`;
                chip.addEventListener('click', () => removeTag(key));
                tagsArea.insertBefore(chip, addBtn);
            });
            updateCount();
            if (window.applyCardFilters) window.applyCardFilters();
        }

        function addTag(key)    { window.CardFilters.categories.add(key);    renderTags(); updateDropdown(); }
        function removeTag(key) { window.CardFilters.categories.delete(key); renderTags(); updateDropdown(); }

        function updateDropdown() {
            if (!dropdown) return;
            CATEGORIES.forEach(cat => {
                const item = dropdown.querySelector(`[data-key="${cat.key}"]`);
                if (item) item.classList.toggle('selected', window.CardFilters.categories.has(cat.key));
            });
        }

        function openDropdown() {
            if (dropdown) return;
            dropdown = document.createElement('div');
            dropdown.className = 'cf-dropdown';
            const rect = container.getBoundingClientRect();
            dropdown.style.position = 'fixed';
            dropdown.style.left     = (rect.left + 28) + 'px';
            dropdown.style.top      = (rect.bottom + 8) + 'px';
            dropdown.style.zIndex   = '99999';
            CATEGORIES.forEach(cat => {
                const item = document.createElement('div');
                item.className   = 'cf-dropdown-item';
                item.dataset.key = cat.key;
                if (window.CardFilters.categories.has(cat.key)) item.classList.add('selected');
                item.innerHTML = `<span class="cf-dot" style="background:${cat.color}"></span>${cat.label}`;
                item.addEventListener('click', () => addTag(cat.key));
                dropdown.appendChild(item);
            });
            document.body.appendChild(dropdown);
            setTimeout(() => document.addEventListener('click', onOutsideClick), 0);
        }

        function closeDropdown() {
            if (!dropdown) return;
            dropdown.remove(); dropdown = null;
            document.removeEventListener('click', onOutsideClick);
        }

        function onOutsideClick(e) {
            if (!container.contains(e.target) && !dropdown?.contains(e.target)) closeDropdown();
        }

        addBtn.addEventListener('click', e => {
            e.stopPropagation();
            dropdown ? closeDropdown() : openDropdown();
        });

        renderTags();
    };
})();
