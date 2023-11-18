import {type Config} from "tailwindcss";

module.exports = {
    content: [
        "./src/**/*.{js,ts,jsx,tsx,mdx}",
        "../../packages/**/*.{js,ts,jsx,tsx,mdx}",
    ],
    theme:   {
        extend: {},
    },
    plugins: [],
} satisfies Config;
