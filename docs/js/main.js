async function loadComponent(id, path) {
  const element = document.getElementById(id);

  if (!element) return;

  const response = await fetch(path);
  const html = await response.text();

  element.innerHTML = html;
}

loadComponent("navbar", "components/navbar.html");
loadComponent("footer", "components/footer.html");