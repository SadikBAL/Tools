window._cropperInstance = null;

window.initCropper = function (imgSrc) {
    const img = document.getElementById('cropperImage');
    if (!img) return;

    if (window._cropperInstance) {
        window._cropperInstance.destroy();
        window._cropperInstance = null;
    }

    img.onload = function () {
        window._cropperInstance = new Cropper(img, {
            aspectRatio: 1200 / 630,
            viewMode: 1,
            dragMode: 'move',
            autoCropArea: 0.95,
            responsive: true,
            background: false,
            movable: true,
            rotatable: false,
            scalable: false,
            zoomable: true,
            zoomOnTouch: true,
            zoomOnWheel: true,
            cropBoxResizable: true,
            toggleDragModeOnDblclick: false,
        });
    };

    img.src = imgSrc;
};

// Sunucuya gönderilecek kırpma koordinatları (küçük veri)
window.getCropData = function () {
    if (!window._cropperInstance) return null;
    const d = window._cropperInstance.getData(true); // rounded integers
    return { x: d.x, y: d.y, width: d.width, height: d.height };
};

// Önizleme için küçük resim (200x105, düşük kalite — sadece display)
window.getCroppedPreview = function () {
    if (!window._cropperInstance) return null;
    const canvas = window._cropperInstance.getCroppedCanvas({ width: 200, height: 105 });
    return canvas ? canvas.toDataURL('image/jpeg', 0.5) : null;
};

window.destroyCropper = function () {
    if (window._cropperInstance) {
        window._cropperInstance.destroy();
        window._cropperInstance = null;
    }
};

window.resetFileInput = function (inputId) {
    const el = document.getElementById(inputId);
    if (el) el.value = '';
};
