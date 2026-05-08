

async function sonucuGoster() {
    console.log("sonucuGoster fonksiyonu çağrıldı");
    const nameInput = document.getElementById("isim");
    const resultBox = document.getElementById("sonuc");
    const name = nameInput.value.trim();

    if (!nameInput.checkValidity() || name.length === 0) {
        resultBox.textContent = "Geçerli bir isim giriniz.";
        nameInput.reportValidity();
        return;
    }

    resultBox.textContent = "İstek gönderiliyor...";

    try {
        const response = await fetch(`/api/hello`, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
                Accept: "application/json",
            },
            body: JSON.stringify({ name }),
        });

        const text = await response.text();
        console.log("API metin cevabı:", text);

        let data;
        try {
            data = JSON.parse(text);
        } catch (e) {
            console.error("JSON parse hatası:", e);
            resultBox.textContent = "API cevabı geçerli JSON değil";
            return;
        }

        console.log("Parsed data:", data);
        console.log("data.data.message:", data.data?.message);

        if (!response.ok) {
            resultBox.textContent = data.message || "API isteği başarısız oldu.";
            return;
        }

        if (data?.data?.message) {
            resultBox.textContent = data.data.message;
        } else if (data?.message) {
            resultBox.textContent = data.message;
        } else {
            resultBox.textContent = "Beklenmeyen API cevabı";
        }
    } catch (error) {
        console.error("Hata:", error);
        resultBox.textContent = "API bağlantısı kurulamadı.";
    }
}
