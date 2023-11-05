import {tx} from "../translator/tx";

export const useTx = () => {
    const translations = {};

    return (input: TemplateStringsArray) => {
        return tx(translations)(input);
    };
};
