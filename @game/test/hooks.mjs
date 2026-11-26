import { registerHooks } from "node:module";

// The game's sources import each other without extensions, as the bundler resolves them. Tests run them in Node,
// which strips their types itself but needs the `.ts` named.
registerHooks({
  resolve(specifier, context, next) {
    try {
      return next(specifier, context);
    } catch (error) {
      if (!specifier.startsWith(".") || specifier.endsWith(".ts")) throw error;
      return next(`${specifier}.ts`, context);
    }
  },
});
