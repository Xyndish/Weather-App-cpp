function getWeather() {
    const city = document.getElementById("cityInput").value.trim();
    const resultBox = document.getElementById("result");

    if (!city) {
        resultBox.textContent = "Enter the name of the city";
        return;
    }

    fetch(`http://localhost:8080/weather?city=${encodeURIComponent(city)}`)
        .then(response => response.json())
        .then(data => {
            if (data.error) {
                resultBox.textContent = ` Error: ${data.error}`;
            } else {
                resultBox.textContent =
                    `${data.city}, ${data.country}\n Temperature: ${data.temperature}°C\nWeather: ${data.condition}`;
            }
        })
        .catch(error => {
            resultBox.textContent = `Request error: ${error}`;
        });
}

