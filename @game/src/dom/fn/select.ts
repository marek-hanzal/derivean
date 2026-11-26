// The element the interface is built around; its absence is a broken page, not a state to handle.
export function select<T extends HTMLElement>(root: ParentNode, selector: string) {
  const element = root.querySelector<T>(selector);
  if (!element) throw new Error(`Missing interface element ${selector}`);
  return element;
}
