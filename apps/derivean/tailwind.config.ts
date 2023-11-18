import {type Config} from "tailwindcss";
import colors        from "tailwindcss/colors";

module.exports = {
    content: [
        "./src/**/*.{js,ts,jsx,tsx,mdx}",
        "../../packages/**/*.{js,ts,jsx,tsx,mdx}",
    ],
    theme:   {
        colors: {
            ...colors,
            primary:   colors.sky,
            secondary: colors.slate,
        },
        extend: {},
    },
    plugins: [],
} satisfies Config;
