import type { GameBridge } from "../../../contract/GameBridge";

function readable(value: number | string) {
  if (typeof value === "string") return value;
  if (Number.isInteger(value)) return value.toLocaleString("en-US");
  return Math.abs(value) >= 1000 ? value.toLocaleString("en-US", { maximumFractionDigits: 1 }) : String(Number(value.toPrecision(4)));
}

// Shows whatever the engine reports about one ground position. The record's keys belong to the simulation, so
// the panel renders them as they come: grouped by what stands before a key's first dot, sentences in full width.
export function createInspector(panel: HTMLElement, bridge: Pick<GameBridge, "inspect">) {
  let request = 0;

  function show(title: string, ...body: HTMLElement[]) {
    const heading = document.createElement("h2");
    heading.textContent = title;
    panel.replaceChildren(heading, ...body);
    panel.hidden = false;
  }

  function note(text: string) {
    const paragraph = document.createElement("p");
    paragraph.textContent = text;
    return paragraph;
  }

  function sections(record: Record<string, number | string>) {
    const groups = new Map<string, HTMLElement[]>();
    for (const [key, value] of Object.entries(record)) {
      const dot = key.indexOf(".");
      const group = dot < 0 ? "" : key.slice(0, dot);
      const name = dot < 0 ? key : key.slice(dot + 1);
      const rows = groups.get(group) ?? [];
      groups.set(group, rows);
      if (typeof value === "string" && value.length > 36) {
        rows.push(note(`${name}: ${value}`));
        continue;
      }
      const term = document.createElement("dt");
      const detail = document.createElement("dd");
      term.textContent = name;
      detail.textContent = readable(value);
      rows.push(term, detail);
    }
    return [...groups].flatMap(([group, rows]) => {
      const list = document.createElement("dl");
      list.append(...rows);
      if (!group) return [list];
      const heading = document.createElement("h3");
      heading.textContent = group;
      return [heading, list];
    });
  }

  return {
    get open() { return !panel.hidden; },
    close() {
      request++;
      panel.hidden = true;
    },
    async inspect(x: number, z: number) {
      const mine = ++request;
      const title = `INSPECT  ${x.toFixed(1)}  ${z.toFixed(1)}`;
      show(title, note("Asking the world…"));
      try {
        const record = await bridge.inspect(x, z);
        if (mine !== request) return;
        const body = sections(record);
        show(title, ...(Object.keys(record).length ? body : [note("Nothing recorded here.")]));
      } catch (error) {
        if (mine === request) show(title, note(error instanceof Error ? error.message : String(error)));
      }
    },
  };
}
