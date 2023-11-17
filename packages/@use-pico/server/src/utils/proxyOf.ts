/**
 * Simple dummy proxy, useful when an object is used only as a type.
 */
export const proxyOf: any = new Proxy(() => proxyOf, {
    get: () => proxyOf,
});
