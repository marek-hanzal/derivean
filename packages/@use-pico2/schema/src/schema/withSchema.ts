import {proxyOf}         from "@use-pico2/utils";
import {type PicoSchema} from "../api/PicoSchema";

type Input<TInput extends PicoSchema> = Omit<TInput, "types" | "_parseAsync">;
type Output<TInput extends PicoSchema> =
    TInput
    & Pick<TInput, "types" | "_parseAsync">;

export const withSchema = <
    TInput extends PicoSchema,
>(
    input: Input<TInput>,
): Output<TInput> => {
    return {
        ...input,
        types: proxyOf,
        async _parseAsync(input, info) {
            return this._parse(input, info);
        },
    } as Output<TInput>;
};
