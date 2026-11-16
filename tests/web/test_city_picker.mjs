// test_city_picker.mjs — Tests de l'autocomplétion : navigation clavier, ARIA,
// sélection et fermeture.

import assert from "node:assert/strict";
import test from "node:test";

import { createCityPicker } from "../../web/js/city_picker.js";

globalThis.document = { createElement };

// Stubs DOM minimalistes : le sélecteur est piloté sans navigateur.
function createElement(tagName) {
  return {
    tagName,
    id: "",
    type: "",
    textContent: "",
    hidden: false,
    children: [],
    attributes: new Map(),
    setAttribute(name, value) { this.attributes.set(name, String(value)); },
    getAttribute(name) { return this.attributes.get(name) ?? null; },
    removeAttribute(name) { this.attributes.delete(name); },
    append(child) { this.children.push(child); },
    replaceChildren() { this.children = []; },
    closest() { return this; }
  };
}

// Prépare un sélecteur neuf avec un `onSelect` qui enregistre les choix.
function setupPicker() {
  const input = createElement("input");
  const list = createElement("ul");
  const selected = [];
  const picker = createCityPicker({
    input,
    list,
    onSelect: (city) => selected.push(city)
  });
  return { input, list, selected, picker };
}

// Simule une touche en fournissant `preventDefault`.
function keydown(key) {
  return { key, preventDefault() {} };
}

// Vérifie que la navigation clavier sans sélection part du premier résultat.
test("ArrowDown sélectionne le premier résultat", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "";

  // When
  picker.render();
  picker.handleKeydown(keydown("ArrowDown"));

  // Then
  assert.equal(input.getAttribute("aria-activedescendant"), "city-option-0");
  assert.equal(list.children[0].getAttribute("aria-selected"), "true");
});

// Vérifie que ArrowUp sans sélection part du dernier résultat.
test("ArrowUp sélectionne le dernier résultat", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "";
  picker.render();
  const lastIndex = list.children.length - 1;

  // When
  picker.handleKeydown(keydown("ArrowUp"));

  // Then
  assert.equal(input.getAttribute("aria-activedescendant"), `city-option-${lastIndex}`);
  assert.equal(list.children[lastIndex].getAttribute("aria-selected"), "true");
});

// Vérifie que la navigation vers le bas boucle sur le premier résultat.
test("ArrowDown boucle après le dernier résultat", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "Paris";
  picker.render();
  picker.handleKeydown(keydown("ArrowUp"));

  // When
  picker.handleKeydown(keydown("ArrowDown"));

  // Then
  assert.equal(input.getAttribute("aria-activedescendant"), "city-option-0");
  assert.equal(list.children.length, 1);
});

// Vérifie qu'une nouvelle saisie efface la descendante active.
test("refresh efface aria-activedescendant", () => {
  // Given
  const { input, picker } = setupPicker();
  input.value = "";
  picker.render();
  picker.handleKeydown(keydown("ArrowDown"));
  assert.equal(input.getAttribute("aria-activedescendant"), "city-option-0");

  // When
  input.value = "Lyon";
  picker.refresh();

  // Then
  assert.equal(input.getAttribute("aria-activedescendant"), null);
});

// Vérifie qu'Entrée choisit la ville active et ferme la liste.
test("Entrée choisit la ville active", () => {
  // Given
  const { input, list, selected, picker } = setupPicker();
  input.value = "Paris";
  picker.render();
  picker.handleKeydown(keydown("ArrowDown"));

  // When
  picker.handleKeydown(keydown("Enter"));

  // Then
  assert.equal(selected.length, 1);
  assert.equal(selected[0].name, "Paris");
  assert.equal(list.hidden, true);
  assert.equal(input.getAttribute("aria-expanded"), "false");
});

// Vérifie qu'Échap ferme la liste et retire la descendante active.
test("Échap ferme la liste", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "Paris";
  picker.render();
  picker.handleKeydown(keydown("ArrowDown"));

  // When
  picker.handleKeydown(keydown("Escape"));

  // Then
  assert.equal(list.hidden, true);
  assert.equal(input.getAttribute("aria-expanded"), "false");
  assert.equal(input.getAttribute("aria-activedescendant"), null);
});

// Vérifie qu'un clic choisit la ville correspondante.
test("un clic choisit la ville", () => {
  // Given
  const { input, list, selected, picker } = setupPicker();
  input.value = "Paris";
  picker.render();

  // When
  picker.handleClick({ target: list.children[0] });

  // Then
  assert.equal(selected.length, 1);
  assert.equal(selected[0].name, "Paris");
});

// Vérifie qu'une recherche sans résultat masque la liste.
test("une recherche vide masque la liste", () => {
  // Given
  const { input, list, picker } = setupPicker();

  // When
  input.value = "zzzz";
  picker.refresh();

  // Then
  assert.equal(list.hidden, true);
  assert.equal(input.getAttribute("aria-expanded"), "false");
  assert.equal(input.getAttribute("aria-activedescendant"), null);
});

// Vérifie que close masque la liste même après une saisie avec résultats.
test("close masque la liste après une saisie", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "Paris";
  picker.render();
  picker.handleKeydown(keydown("ArrowDown"));
  assert.equal(list.hidden, false);

  // When
  picker.close();

  // Then
  assert.equal(list.hidden, true);
  assert.equal(input.getAttribute("aria-expanded"), "false");
  assert.equal(input.getAttribute("aria-activedescendant"), null);
});

// Vérifie qu'un rendu après fermeture rouvre la liste avec la saisie courante.
test("render après close rouvre la liste", () => {
  // Given
  const { input, list, picker } = setupPicker();
  input.value = "Paris";
  picker.render();
  picker.close();
  assert.equal(list.hidden, true);

  // When
  picker.render();

  // Then
  assert.equal(list.hidden, false);
  assert.equal(input.getAttribute("aria-expanded"), "true");
  assert.equal(list.children.length, 1);
});