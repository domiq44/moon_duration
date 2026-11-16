// city_picker.js — Autocomplétion de ville : liste, navigation clavier et
// attributs ARIA.

"use strict";

import { searchCities } from "./cities.js?v=1";

// Autocomplétion de la ville : rendu de la liste et navigation clavier.
// Le choix est délégué à `onSelect`, qui met à jour le formulaire.
export function createCityPicker({ input, list, onSelect }) {
  let activeIndex = -1;
  let visibleCities = [];

  function close() {
    list.hidden = true;
    input.setAttribute("aria-expanded", "false");
    input.removeAttribute("aria-activedescendant");
    activeIndex = -1;
  }

  function render() {
    visibleCities = searchCities(input.value);
    list.replaceChildren();
    visibleCities.forEach((city, index) => {
      const option = document.createElement("li");
      option.id = `city-option-${index}`;
      option.setAttribute("role", "option");
      option.setAttribute("aria-selected", String(index === activeIndex));
      const button = document.createElement("button");
      button.type = "button";
      button.textContent = city.name;
      option.append(button);
      list.append(option);
    });
    list.hidden = visibleCities.length === 0;
    input.setAttribute("aria-expanded", String(!list.hidden));
    if (activeIndex >= 0 && activeIndex < visibleCities.length) {
      input.setAttribute("aria-activedescendant", `city-option-${activeIndex}`);
    } else {
      input.removeAttribute("aria-activedescendant");
    }
  }

  // Nouvelle saisie : la sélection clavier repart du premier résultat.
  function refresh() {
    activeIndex = -1;
    render();
  }

  function select(index) {
    const city = visibleCities[index];
    if (!city) return;
    close();
    onSelect(city);
  }

  function move(delta) {
    if (list.hidden || visibleCities.length === 0) return;
    if (activeIndex === -1) {
      // Sans sélection, ArrowDown part du premier résultat et ArrowUp du dernier.
      activeIndex = delta > 0 ? 0 : visibleCities.length - 1;
    } else {
      activeIndex = (activeIndex + delta + visibleCities.length) % visibleCities.length;
    }
    render();
  }

  function handleKeydown(event) {
    if (event.key === "Escape") {
      close();
    } else if (event.key === "ArrowDown" && !list.hidden) {
      event.preventDefault();
      move(1);
    } else if (event.key === "ArrowUp" && !list.hidden) {
      event.preventDefault();
      move(-1);
    } else if (event.key === "Enter" && !list.hidden && visibleCities.length) {
      event.preventDefault();
      select(Math.max(activeIndex, 0));
    }
  }

  function handleClick(event) {
    const option = event.target.closest('[role="option"]');
    if (option) select(Number(option.id.replace("city-option-", "")));
  }

  return { close, render, refresh, handleKeydown, handleClick };
}