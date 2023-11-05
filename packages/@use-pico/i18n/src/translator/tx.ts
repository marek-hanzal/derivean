export const tx = (translations: Record<string, {
    id: string,
    value: string
}>, values?: Record<string, any>) => {
    return (input: TemplateStringsArray) => {
        /**
         * Here will be the interesting part of translation implementation
         */
        return translations[input.join("")]?.["value"] ?? input;
    };
};
