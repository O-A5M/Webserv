document.addEventListener("DOMContentLoaded", () => {
  // Setup interactions and ONLY start the text intro first
  setupCardInteractions();
  runTypewriterIntro();
});

async function runTypewriterIntro() {
  const phrases = [
    "wake up walid...",
    "wake up achraf...",
    "wake up othman..."
  ];

  const textElem = document.getElementById("typewriter");
  const introDiv = document.getElementById("matrix-intro");
  const hiddenElements = document.querySelectorAll(".init-hidden");

  const typePhrase = async (phrase) => {
    for (let i = 0; i < phrase.length; i++) {
      textElem.innerHTML += phrase[i];
      // Randomize typing speed for realism
      await new Promise((r) => setTimeout(r, 60 + Math.random() * 80));
    }
    // Wait before clearing
    await new Promise((r) => setTimeout(r, 1200));
    textElem.innerHTML = "";
  };

  // Initial black screen pause
  await new Promise((r) => setTimeout(r, 1000));

  for (const phrase of phrases) {
    await typePhrase(phrase);
  }

  // Hide the typing div entirely
  introDiv.classList.add("hidden");

  // Reveal the normal UI
  hiddenElements.forEach((el) => el.classList.add("visible"));

  // Start the Matrix digital rain ONLY after the intro text finishes
  initMatrixRain();
}

function setupCardInteractions() {
  const cards = document.querySelectorAll(".team-member");
  cards.forEach((card) => {
    card.addEventListener("mousedown", () => {
      card.style.transform = "scale(0.95)";
    });

    card.addEventListener("mouseup", () => {
      card.style.transform = "translateY(-10px)";
    });

    card.addEventListener("mouseleave", () => {
      card.style.transform = ""; // resets to hover CSS transition
    });
  });
}

function initMatrixRain() {
  const canvas = document.getElementById("networkCanvas");
  const ctx = canvas.getContext("2d");

  let columns, drops;
  const fontSize = 18;
  const chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$%^&*()ｱｲｳｴｵｶｷｸｹｺｻｼｽｾｿﾀﾁﾂﾃﾄﾅﾆﾇﾈﾉﾊﾋﾌﾍﾎﾏﾐﾑﾒﾓﾔﾕﾖﾗﾘﾙﾚﾛﾜﾝ".split(
      "",
    );

  function resize() {
    canvas.width = window.innerWidth;
    canvas.height = window.innerHeight;
    columns = Math.floor(canvas.width / fontSize);
    drops = [];
    for (let i = 0; i < columns; i++) {
      drops[i] = 1;
    }
  }

  function draw() {
    // Translucent black background creates the fading effect
    ctx.fillStyle = "rgba(0, 0, 0, 0.05)";
    ctx.fillRect(0, 0, canvas.width, canvas.height);

    ctx.fillStyle = "#0F0"; // The Matrix Green
    ctx.font = fontSize + "px monospace";

    for (let i = 0; i < drops.length; i++) {
      const text = chars[Math.floor(Math.random() * chars.length)];

      // x = i*fontSize, y = value of drops[i]*fontSize
      ctx.fillText(text, i * fontSize, drops[i] * fontSize);

      // Sending the drop back to the top randomly after it has crossed the screen
      if (drops[i] * fontSize > canvas.height && Math.random() > 0.975) {
        drops[i] = 0;
      }

      // Incrementing y coordinate
      drops[i]++;
    }
  }

  window.addEventListener("resize", resize);

  resize();
  setInterval(draw, 35);
}
