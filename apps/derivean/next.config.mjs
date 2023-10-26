import withBundleAnalyzer from '@next/bundle-analyzer';
import withPlugins from 'next-compose-plugins';

const isDev = process.env.NODE_ENV === 'development';

const transpile = [
    '@use-pico/auth',
    '@use-pico/bulk',
    '@use-pico/calendar',
    '@use-pico/context',
    '@use-pico/file',
    '@use-pico/form',
    '@use-pico/hook',
    '@use-pico/i18n',
    '@use-pico/job',
    '@use-pico/navigation',
    '@use-pico/pagination',
    '@use-pico/query',
    '@use-pico/rpc',
    '@use-pico/schema',
    '@use-pico/sdk',
    '@use-pico/selection',
    '@use-pico/source',
    '@use-pico/store',
    '@use-pico/table',
    '@use-pico/types',
    '@use-pico/ui',
    '@use-pico/ui-extra',
    '@use-pico/utils',
];

export default withPlugins([
    withBundleAnalyzer({
        enabled: process.env.ANALYZE === 'true',
    }),
], {
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
