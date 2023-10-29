import withPlugins from 'next-compose-plugins';
import { transpile } from './transpile.mjs';

export default withPlugins([], {
    distDir:                     'dist',
    images:                      {
        unoptimized: true,
    },
    swcMinify:                   true,
    poweredByHeader:             false,
    productionBrowserSourceMaps: false,
    transpilePackages:           transpile,
    /**
     * This piece looks strange, but when a library supports it, it's possible to rewrite
     * barrel imports into direct module imports, thus optimizing page size, speed, and overall
     * world is nicer, better and with more rainbows everywhere.
     */
    modularizeImports: transpile.reduce((acc, item) => {
        acc[item] = {
            transform:             `${item}/src/$export/{{member}}`,
            skipDefaultConversion: true,
        };
        return acc;
    }, {}),
    reactStrictMode:   true,
});
