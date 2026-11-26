import type { WorldOptions } from "../../../contract/GameBridge";
import { select } from "../../dom/fn/select";
import { formatAge, parseAge } from "../../units/fn/age";
import { ControlAges, DefaultAge, DefaultSeed } from "../constant/ages";

// The world form: seed, age and spawn. `onChange` fires with a complete, valid choice each time it differs from
// the last one reported; invalid input is marked and held back.
export function createWorldLauncher(root: HTMLElement, onChange: (options: WorldOptions) => void) {
  const form = select<HTMLFormElement>(root, "#world-form");
  const seed = select<HTMLInputElement>(root, "#seed");
  const age = select<HTMLInputElement>(root, "#age");
  const ageReadout = select(root, "#age-readout");
  const presets = select(root, "#age-presets");
  const spawnX = select<HTMLInputElement>(root, "#spawn-x");
  const spawnZ = select<HTMLInputElement>(root, "#spawn-z");
  const progress = select(root, "#progress");
  let reported = "";

  seed.value = String(DefaultSeed);
  age.value = formatAge(DefaultAge);
  for (const years of ControlAges) {
    const button = document.createElement("button");
    button.type = "button";
    button.dataset.age = String(years);
    button.textContent = formatAge(years);
    presets.append(button);
  }

  function mark(input: HTMLInputElement, valid: boolean) {
    input.setAttribute("aria-invalid", String(!valid));
    return valid;
  }

  function read(): WorldOptions | undefined {
    const seedValue = Number(seed.value.trim());
    const years = parseAge(age.value);
    const x = spawnX.value.trim();
    const z = spawnZ.value.trim();
    const spawn: [number, number] | undefined = x || z ? [Number(x), Number(z)] : undefined;
    const spawnValid = !spawn || (x !== "" && z !== "" && spawn.every(Number.isFinite));
    ageReadout.textContent = Number.isNaN(years) ? "years, or 250 Myr, 4.6 Gyr" : `${formatAge(years)} · ${years.toLocaleString("en-US")} yr`;
    for (const button of presets.querySelectorAll<HTMLButtonElement>("button")) {
      button.setAttribute("aria-pressed", String(Number(button.dataset.age) === years));
    }
    const valid = [
      mark(seed, /^\d+$/.test(seed.value.trim()) && Number.isSafeInteger(seedValue)),
      mark(age, !Number.isNaN(years)),
      mark(spawnX, spawnValid),
      mark(spawnZ, spawnValid),
    ].every(Boolean);
    return valid ? { seed: seedValue, age: years, ...(spawn ? { spawn } : {}) } : undefined;
  }

  function commit() {
    const options = read();
    const key = JSON.stringify(options);
    if (!options || key === reported) return;
    reported = key;
    onChange(options);
  }

  presets.addEventListener("click", (event) => {
    const years = (event.target as HTMLElement).closest<HTMLButtonElement>("button")?.dataset.age;
    if (years === undefined) return;
    age.value = formatAge(Number(years));
    commit();
  });
  form.addEventListener("change", commit);
  form.addEventListener("input", read);
  form.addEventListener("keydown", (event) => {
    if (event.key === "Enter") commit();
  });

  return {
    // Reports the initial choice.
    open: commit,
    // Lets the same choice be reported again, for a retry after the world failed to start.
    forget() { reported = ""; },
    progress(text: string) { progress.textContent = text; },
  };
}
