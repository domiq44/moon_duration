// chart.js — Graphique d'altitude lunaire : maximum, intervalles au-dessus de
// l'horizon, graduations et rendu.

"use strict";

import { hourTickLabels } from "./format.js?v=1";

export const CHART_STEPS = 144;
// Horizon du graphique, aligné sur MOON_HORIZON_ALTITUDE_DEG (include/moon_events.h).
export const CHART_HORIZON = 0.125;

// `dayHours` est la durée écoulée de la journée locale (23 ou 25 h lors d'un
// changement d'heure), pour que la recherche du maximum couvre tout le jour.
export function findMaximumHour(altitudeAt, steps = CHART_STEPS, dayHours = 24) {
  if (!Number.isInteger(steps) || steps <= 0) {
    throw new RangeError("steps must be a positive integer");
  }
  const stepSize = dayHours / steps;
  let maximumHour = 0;
  let maximumAltitude = altitudeAt(0);
  for (let step = 1; step <= steps; step += 1) {
    const hour = step * stepSize;
    const altitude = altitudeAt(hour);
    if (altitude > maximumAltitude) {
      maximumAltitude = altitude;
      maximumHour = hour;
    }
  }
  let low = Math.max(0, maximumHour - stepSize);
  let high = Math.min(dayHours, maximumHour + stepSize);
  for (let iteration = 0; iteration < 40; iteration += 1) {
    const first = low + (high - low) / 3;
    const second = high - (high - low) / 3;
    if (altitudeAt(first) < altitudeAt(second)) low = first;
    else high = second;
  }
  return (low + high) / 2;
}

// Intervalles, en heures écoulées, pendant lesquels l'altitude reste au-dessus
// de l'horizon. Les bornes sont affinées par dichotomie.
export function findIntervals(altitudeAt, horizon = CHART_HORIZON, steps = CHART_STEPS, dayHours = 24) {
  const stepSize = dayHours / steps;
  const intervals = [];
  let previousHour = 0;
  let previousAbove = altitudeAt(0) >= horizon;
  let intervalStart = previousAbove ? 0 : null;
  for (let step = 1; step <= steps; step += 1) {
    const hour = step * stepSize;
    const above = altitudeAt(hour) >= horizon;
    if (above !== previousAbove) {
      let low = previousHour;
      let high = hour;
      for (let iteration = 0; iteration < 16; iteration += 1) {
        const midpoint = (low + high) / 2;
        if ((altitudeAt(midpoint) >= horizon) === previousAbove) low = midpoint;
        else high = midpoint;
      }
      const crossing = (low + high) / 2;
      if (above) intervalStart = crossing;
      else {
        intervals.push([intervalStart, crossing]);
        intervalStart = null;
      }
    }
    previousHour = hour;
    previousAbove = above;
  }
  if (intervalStart !== null) intervals.push([intervalStart, dayHours]);
  return intervals;
}

export function drawHorizonLine(context, projection, horizon, colors) {
  const horizonY = projection.y(horizon);
  context.setLineDash([5, 5]);
  context.strokeStyle = colors.horizonGlow;
  context.lineWidth = 5;
  context.beginPath();
  context.moveTo(projection.plot.left, horizonY);
  context.lineTo(projection.plot.right, horizonY);
  context.stroke();
  context.strokeStyle = colors.horizon;
  context.lineWidth = 1.5;
  context.stroke();
  context.setLineDash([]);
}

export function drawMaximumMarker(context, projection, altitudeAt, maximumHour, horizon, colors) {
  const maximumX = projection.x(maximumHour);
  const maximumY = projection.y(altitudeAt(maximumHour));
  const horizonY = projection.y(horizon);
  const labelX = Math.min(Math.max(maximumX, projection.plot.left + 6), projection.plot.right - 6);
  context.beginPath();
  context.moveTo(maximumX, maximumY);
  context.lineTo(maximumX, horizonY);
  context.setLineDash([]);
  context.lineCap = "round";
  context.strokeStyle = colors.outline;
  context.lineWidth = 6;
  context.stroke();
  context.strokeStyle = colors.curve;
  context.lineWidth = 3;
  context.stroke();
  context.lineCap = "butt";
  context.font = "bold 12px Segoe UI, sans-serif";
  context.textAlign = "center";
  context.strokeStyle = colors.outline;
  context.lineWidth = 4;
  context.strokeText("M", labelX, horizonY + 12);
  context.fillStyle = colors.curve;
  context.fillText("M", labelX, horizonY + 12);
}

// Trace la courbe d'altitude lunaire : aires au-dessus de l'horizon, repères,
// maximum et position actuelle.
export function drawMoonChart(canvas, {
  altitudeAt,
  dayStartJulianDay,
  currentJulianDay,
  timezone,
  selectedDateKey,
  latitude,
  longitude,
  dayHours = 24
}) {
  const bounds = canvas.getBoundingClientRect();
  if (!bounds.width || !bounds.height) return;
  const scale = window.devicePixelRatio || 1;
  canvas.width = Math.round(bounds.width * scale);
  canvas.height = Math.round(bounds.height * scale);
  const context = canvas.getContext("2d");
  context.scale(scale, scale);
  const width = bounds.width;
  const height = bounds.height;
  const plot = { left: 38, right: width - 9, top: 12, bottom: height - 26 };
  const x = (hour) => plot.left + hour / dayHours * (plot.right - plot.left);
  const y = (altitude) => plot.top + (90 - altitude) / 180 * (plot.bottom - plot.top);
  const colors = {
    night: "#0b1220",
    above: "rgba(56, 189, 248, 0.28)",
    grid: "rgba(226, 232, 240, 0.18)",
    axis: "#cbd5e1",
    curve: "#d9b45b",
    outline: "#0b1220",
    horizon: "#ff6b6b",
    horizonGlow: "rgba(255, 107, 107, 0.16)",
    current: "#f8fafc"
  };
  const intervals = findIntervals(altitudeAt, CHART_HORIZON, CHART_STEPS, dayHours);

  context.clearRect(0, 0, width, height);
  context.fillStyle = colors.night;
  context.fillRect(0, 0, width, height);
  context.fillStyle = colors.above;
  for (const [start, end] of intervals) {
    context.fillRect(x(start), plot.top, x(end) - x(start), plot.bottom - plot.top);
  }

  context.font = "10px Segoe UI, sans-serif";
  context.textBaseline = "middle";
  context.fillStyle = colors.axis;
  context.strokeStyle = colors.grid;
  context.lineWidth = 1;
  for (const altitude of [-60, 0, 60]) {
    const position = y(altitude);
    context.beginPath();
    context.moveTo(plot.left, position);
    context.lineTo(plot.right, position);
    context.stroke();
    context.textAlign = "right";
    context.fillText(`${altitude}°`, plot.left - 8, position);
  }

  context.beginPath();
  for (let step = 0; step <= CHART_STEPS; step += 1) {
    const hour = step / CHART_STEPS * dayHours;
    const pointX = x(hour);
    const pointY = y(altitudeAt(hour));
    if (step === 0) context.moveTo(pointX, pointY);
    else context.lineTo(pointX, pointY);
  }
  context.strokeStyle = colors.outline;
  context.lineWidth = 5.5;
  context.lineJoin = "round";
  context.stroke();
  context.strokeStyle = colors.curve;
  context.lineWidth = 2.5;
  context.stroke();

  drawHorizonLine(context, { y, plot }, CHART_HORIZON, colors);

  const maximumHour = findMaximumHour(altitudeAt, CHART_STEPS, dayHours);
  drawMaximumMarker(context, { x, y, plot }, altitudeAt, maximumHour, CHART_HORIZON, colors);

  const currentHour = (currentJulianDay - dayStartJulianDay) * 24;
  const showsCurrent = currentJulianDay >= dayStartJulianDay &&
    currentJulianDay < dayStartJulianDay + dayHours / 24;
  if (showsCurrent) {
    const currentX = x(currentHour);
    const currentAltitude = altitudeAt(currentHour);

    if (Number.isFinite(currentAltitude)) {
      const currentY = y(currentAltitude);

      context.beginPath();
      context.moveTo(currentX, currentY);
      context.lineTo(currentX, y(CHART_HORIZON));
      context.strokeStyle = colors.outline;
      context.lineWidth = 4;
      context.stroke();
      context.strokeStyle = colors.current;
      context.lineWidth = 1.5;
      context.stroke();

      context.font = "34px Segoe UI Symbol, sans-serif";
      context.textAlign = "center";
      context.strokeStyle = colors.outline;
      context.lineWidth = 2;
      context.strokeText("☾", currentX, currentY);
      context.fillStyle = colors.current;
      context.fillText("☾", currentX, currentY);
    }

    const labelX = Math.min(Math.max(currentX, plot.left + 34), plot.right - 34);
    context.font = "10px Segoe UI, sans-serif";
    context.fillStyle = colors.axis;
    context.textAlign = "center";
    context.fillText("Maintenant", labelX, plot.top + 8);
  }

  const tickHours = Array.from({ length: 5 }, (_, index) => index * dayHours / 4);
  const tickLabels = hourTickLabels(dayStartJulianDay, timezone, selectedDateKey, tickHours);
  context.fillStyle = colors.axis;
  context.textAlign = "center";
  tickHours.forEach((hour, index) => {
    context.fillText(tickLabels[index], x(hour), height - 10);
  });
  canvas.setAttribute("aria-label", `Hauteur lunaire en heure locale. Latitude ${latitude.toFixed(2)} degrés, longitude ${longitude.toFixed(2)} degrés.${showsCurrent ? " La position actuelle est indiquée par un croissant." : ""} Le maximum est indiqué par la lettre M.`);
}