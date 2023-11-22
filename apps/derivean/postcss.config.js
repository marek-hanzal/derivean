module.exports = {
    plugins: {
        '@unocss/postcss': {
            content: [
                './src/*.{html,js,ts,jsx,tsx}',
                // '../../packages/**/*.{js,ts,jsx,tsx,mdx}',
            ],
        },
        autoprefixer:      {},
    },
};
