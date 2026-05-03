const button = document.getElementById("postBtn");
const result = document.getElementById("result");

button.addEventListener("click", async () => {
    const response = await fetch("/api/echo", {
        method: "POST",
        headers: {
            "Content-Type": "text/plain",
        },
        body: "merhaba",
    });

    /*  const text = await response.text();
    result.textContent = text;*/
});
