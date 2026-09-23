const path = window.location.pathname;
const isEnglish = path.includes("/en/");

const componentPrefix = "../components/";

const components = {
  navbar: isEnglish ? "navbar-en.html" : "navbar-es.html",
  sidebar: isEnglish ? "sidebar-en.html" : "sidebar-es.html",
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

function updateLanguageLinks() {
  const page = path.split("/").pop() || "index.html";

  const spanish = document.querySelector('[data-language="es"]');
  const english = document.querySelector('[data-language="en"]');

  if (spanish) {
    spanish.href = `../es/${page}`;
  }

  if (english) {
    english.href = `../en/${page}`;
  }
}

async function init() {
  await loadComponent("navbar", components.navbar);
  await loadComponent("sidebar", components.sidebar);
  await loadComponent("footer", components.footer);

  updateLanguageLinks();
  markActiveLinks();
}

init();