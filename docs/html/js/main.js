const path = window.location.pathname;
const isEnglish = path.includes("/en/");

const componentPrefix = "../components/";

const components = {
  navbar: isEnglish ? "navbar-en.html" : "navbar-es.html",
  sidebar: isEnglish ? "sidebar-en.html" : "sidebar-es.html",
  docPath: isEnglish ? "doc-path-en.html" : "doc-path-es.html",
  footer: isEnglish ? "footer-en.html" : "footer-es.html",
};

async function loadComponent(id, fileName) {
  const element = document.getElementById(id);
  if (!element) return;

  const response = await fetch(componentPrefix + fileName);
  const html = await response.text();

  element.innerHTML = html;
}

function markActiveLinks() {
  const section = document.body.dataset.section;
  if (!section) return;

  const links = document.querySelectorAll("[data-link]");

  links.forEach(link => {
    if (link.dataset.link === section) {
      link.classList.add("active");
    }
  });
}

function updateDocPath() {
  const section = document.body.dataset.section;
  if (!section) return;

  const items = Array.from(document.querySelectorAll("[data-path]"));
  const currentIndex = items.findIndex(item => item.dataset.path === section);

  if (currentIndex === -1) return;

  const current = items[currentIndex];
  const currentLink = current.querySelector("a");

  current.classList.add("current");

  if (currentLink) {
    const label = currentLink.textContent;
    current.textContent = `↓ ${label} ← ${isEnglish ? "You are here" : "Estás aquí"}`;
  }

  const next = items[currentIndex + 1];

  if (next) {
    next.classList.add("recommended");
    next.append(` → ${isEnglish ? "Recommended" : "Recomendado"}`);
  }
}

function updateLanguageLinks() {
  const languageMatch = path.match(/\/(?:es|en)\/(.+)$/);
  const page = languageMatch ? languageMatch[1] : "index.html";

  const spanish = document.querySelector('[data-language="es"]');
  const english = document.querySelector('[data-language="en"]');

  if (spanish) {
    spanish.href = `../es/${page}`;
  }

  if (english) {
    if (document.body.dataset.section === "machine") {
      english.removeAttribute("href");
      english.setAttribute("aria-disabled", "true");
      english.title = "Machine todavía no está disponible en inglés";
    } else {
      english.href = `../en/${page}`;
    }
  }
}

async function init() {
  await loadComponent("navbar", components.navbar);
  await loadComponent("sidebar", components.sidebar);
  await loadComponent("doc-path", components.docPath);
  await loadComponent("footer", components.footer);

  updateLanguageLinks();
  markActiveLinks();
  updateDocPath();
}

init();