window.loadGoogleMapsScript = function (apiKey) {
    if (window.google && window.google.maps && window.google.maps.places) {
        return Promise.resolve();
    }

    if (document.querySelector('script[src*="maps.googleapis.com"]')) {
        return new Promise(resolve => {
            const id = setInterval(() => {
                if (window.google?.maps?.places) { clearInterval(id); resolve(); }
            }, 80);
        });
    }

    return new Promise((resolve, reject) => {
        const script = document.createElement('script');
        script.src = `https://maps.googleapis.com/maps/api/js?key=${apiKey}&libraries=places`;
        script.async = true;
        script.onload  = resolve;
        script.onerror = () => reject(new Error('Google Maps yüklenemedi'));
        document.head.appendChild(script);
    });
};

window.initMapsAutocomplete = function (inputId, dotNetRef) {
    const input = document.getElementById(inputId);
    if (!input || !window.google?.maps?.places) return;

    const ac = new google.maps.places.Autocomplete(input, {
        fields: ['name', 'formatted_address']
    });

    ac.addListener('place_changed', () => {
        const place = ac.getPlace();
        let display = '';
        if (place.name && place.formatted_address) {
            display = place.formatted_address.startsWith(place.name)
                ? place.formatted_address
                : `${place.name}, ${place.formatted_address}`;
        } else {
            display = place.formatted_address || place.name || '';
        }
        dotNetRef.invokeMethodAsync('OnPlaceSelected', display);
    });
};
